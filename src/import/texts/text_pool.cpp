// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/texts.h"

#include <algorithm>
#include <format>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "import/support/enigma_text.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace texts {
namespace {

using musx::dom::Cmper;

// The text pool is the one pre-2007 pool that is not made of records, and the one place the
// legacy format describes itself in words. It holds `^keyword(n) ... ^end` chunks end to end,
// exactly as ETF prints its own text section, so the class of each chunk is stated by the
// file rather than inferred from a comparator or a position.
//
// The pool has its own block per epoch: 0x0004 uncompressed, 0x0012 DCL, 0x0017 zlib. A
// chunk's number is the comparator musxdom keys the pool by.
//
// **The Coda-banner epoch is uncovered here, by intent rather than by oversight.** That era has
// a text region -- two length-prefixed chunks after the last record pool, opening with the same
// `^text` and `^lyrics` markers ETF uses -- but those chunks are empty in every document seen,
// and the era's block text lives in the `HT` record family instead, interleaved with binary
// layout data whose framing is unresolved. Reading `HT` is separate work; nothing here covers
// it.
struct TextKeyword
{
    std::string_view keyword;
    void (*create)(const musx::dom::DocumentPtr& document, Cmper number, std::string&& text);
    std::string_view nodeName;
    /// @brief Optional test on the record's number, for a class that constrains it.
    /// @details `texts::FileInfoText` throws on a number outside its own enumeration, so a
    /// malformed chunk would abort the import rather than being skipped. Nothing else in the
    /// pool restricts its comparator.
    bool (*accepts)(Cmper number);
};

template <typename Target>
void createText(const musx::dom::DocumentPtr& document, Cmper number, std::string&& text)
{
    auto instance = std::make_shared<Target>(
        document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, number);
    instance->text = std::move(text);
    document->getTexts()->add(Target::XmlNodeName, instance);
}

template <typename Target>
constexpr TextKeyword textKeyword(std::string_view keyword, bool (*accepts)(Cmper) = nullptr)
{
    return TextKeyword{keyword, &createText<Target>, Target::XmlNodeName, accepts};
}

bool isFileInfoType(Cmper number)
{
    using FileInfoTarget = musx::dom::texts::FileInfoText;
    return number > 0 && number <= Cmper(FileInfoTarget::TextType::Subtitle);
}

// The keywords, and the musxdom class each one names.
//
// `block`, `smartshape`, `expression` and `verse` are established. **Unverified: `chorus` and
// `section`,** which no document seen so far contains. They follow `verse` because Finale
// treats the three lyric kinds as one family everywhere else -- musxdom names them verse,
// chorus and section, and so does the ETF lyrics section. A wrong spelling costs nothing
// silently: an unrecognized keyword is reported by name below, which is how the right one
// would be found.
constexpr TextKeyword textKeywords[] = {
    textKeyword<musx::dom::texts::BlockText>("block"),
    textKeyword<musx::dom::texts::LyricsVerse>("verse"),
    textKeyword<musx::dom::texts::LyricsChorus>("chorus"),
    textKeyword<musx::dom::texts::LyricsSection>("section"),
    textKeyword<musx::dom::texts::SmartShapeText>("smartshape"),
    textKeyword<musx::dom::texts::ExpressionText>("expression"),
    // File Info starts out in the header and becomes ordinary pool records in a later era.
    // Where in between the move happens does not matter here: the header pass fills in only
    // the types the pool did not supply, so each document states for itself which way it
    // stores them. The number is musxdom's own `FileInfoText::TextType`.
    textKeyword<musx::dom::texts::FileInfoText>("fileInfo", &isFileInfoType),
};

constexpr std::string_view recordTerminator = "^end";

bool isLetter(std::uint8_t value)
{
    return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z');
}

/// @brief One chunk as the stream states it, before any of it is interpreted.
struct RawRecord
{
    std::string_view keyword;
    Cmper number{};
    std::span<const std::uint8_t> body;
    /// @brief Offset of the first byte after this chunk's terminator.
    std::size_t next{};
    /// @brief Offset of the chunk's own first byte, for reporting.
    std::size_t start{};
};

/// @brief Reads the chunk beginning at @p at, or nothing when the bytes are not one.
std::optional<RawRecord> readRecord(std::span<const std::uint8_t> stream, std::size_t at)
{
    if (at >= stream.size() || stream[at] != '^') {
        return std::nullopt;
    }
    std::size_t cursor = at + 1;
    const std::size_t keywordStart = cursor;
    while (cursor < stream.size() && isLetter(stream[cursor])) {
        ++cursor;
    }
    if (cursor == keywordStart || cursor >= stream.size() || stream[cursor] != '(') {
        return std::nullopt;
    }
    RawRecord record;
    record.start = at;
    record.keyword = std::string_view(
        reinterpret_cast<const char*>(stream.data() + keywordStart), cursor - keywordStart);
    ++cursor;
    // A comparator is sixteen bits, so a longer run of digits is not one, whatever it is.
    // Bounding it here is what stops a malformed stream from wrapping into a plausible number.
    constexpr std::size_t maximumComparatorDigits = 5;
    const std::size_t numberStart = cursor;
    std::uint32_t number = 0;
    while (cursor < stream.size() && stream[cursor] >= '0' && stream[cursor] <= '9'
        && cursor - numberStart < maximumComparatorDigits) {
        number = number * 10 + static_cast<std::uint32_t>(stream[cursor] - '0');
        ++cursor;
    }
    if (cursor == numberStart || cursor >= stream.size() || stream[cursor] != ')'
        || number > (std::numeric_limits<std::uint16_t>::max)()) {
        return std::nullopt;
    }
    record.number = static_cast<Cmper>(number);
    ++cursor;

    // The terminator is a plain search rather than a parse. A caret inside the body is either
    // a command, which cannot spell `end` and then stop, or an escaped `^^`, whose second
    // caret cannot begin `^end` either without the first having consumed it.
    const std::string_view remaining(
        reinterpret_cast<const char*>(stream.data() + cursor), stream.size() - cursor);
    const auto terminator = remaining.find(recordTerminator);
    if (terminator == std::string_view::npos) {
        return std::nullopt;
    }
    record.body = stream.subspan(cursor, terminator);
    record.next = cursor + terminator + recordTerminator.size();
    return record;
}

void reportUnread(ImportReport& report, const std::vector<std::uint8_t>& codes,
    const std::vector<std::string>& effects, const std::vector<std::string>& keywords)
{
    if (!codes.empty()) {
        std::string message = "Legacy text commands this reader could not read were "
            "dropped; their codes are";
        for (const auto code : codes) {
            message += ' ' + std::format("0x{:02x}", code);
        }
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Info, message + "."});
    }
    if (!effects.empty()) {
        std::string message = "Legacy text effects with no known bit were ignored:";
        for (const auto& effect : effects) {
            message += ' ' + effect;
        }
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Info, message + "."});
    }
    if (!keywords.empty()) {
        std::string message = "The text pool named record kinds this reader does not import:";
        for (const auto& keyword : keywords) {
            message += ' ' + keyword;
        }
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning, message + "."});
    }
}

void rememberTextPoolName(std::vector<std::string>& list, std::string_view value)
{
    if (std::find(list.begin(), list.end(), value) == list.end()) {
        list.emplace_back(value);
    }
}

} // namespace

void importTextPool(const ImportContext& context)
{
    const auto stream = context.index.getTexts();
    if (stream.empty()) {
        return;
    }

    // Finale 2012 converted stored text to Unicode, which is the same boundary the option
    // records cross when a symbol codepoint widens from a word to a long. It is one change to
    // how the file stores characters, so it is asked for once rather than named again here.
    const text::EnigmaTextSource source{context.document,
        versions::storesUnicodeCodepoints(context.profile.version),
        text::platformCodePage(context.profile.platform)};

    std::vector<std::uint8_t> unknownCodes;
    std::vector<std::string> unknownEffects;
    std::vector<std::string> unknownKeywords;
    std::size_t at = 0;
    while (at < stream.size()) {
        const auto record = readRecord(stream, at);
        if (!record) {
            // Stopping is deliberate. The chunks are packed end to end with nothing between
            // them, so bytes that are not a chunk mean the stream is not a text pool, and
            // scanning ahead for the next plausible keyword would slice some other structure
            // into text blocks that were never there.
            context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
                "The text pool stopped making sense at offset " + std::to_string(at)
                    + " of " + std::to_string(stream.size())
                    + "; the text after that point was not imported."});
            break;
        }
        at = record->next;

        const auto found = std::find_if(std::begin(textKeywords), std::end(textKeywords),
            [&](const TextKeyword& entry) { return entry.keyword == record->keyword; });
        if (found == std::end(textKeywords)) {
            rememberTextPoolName(unknownKeywords, record->keyword);
            continue;
        }
        if (found->accepts && !found->accepts(record->number)) {
            rememberTextPoolName(unknownKeywords,
                std::string(record->keyword) + '(' + std::to_string(record->number) + ')');
            continue;
        }

        auto converted = text::toModernEnigmaText(record->body, source);
        for (const auto code : converted.unreadCommandCodes) {
            if (std::find(unknownCodes.begin(), unknownCodes.end(), code) == unknownCodes.end()) {
                unknownCodes.push_back(code);
            }
        }
        for (const auto& effect : converted.unknownEffectNames) {
            rememberTextPoolName(unknownEffects, effect);
        }


        FieldInfo info;
        info.target = "texts." + std::string(found->nodeName) + '['
            + std::to_string(record->number) + "].text";
        info.origin = ValueOrigin::LegacyMus;
        info.decodedOffset = record->start;
        info.rawValue = static_cast<std::int64_t>(converted.text.size());
        context.report.fields.push_back(std::move(info));

        found->create(context.document, record->number, std::move(converted.text));
    }

    reportUnread(context.report, unknownCodes, unknownEffects, unknownKeywords);
}

} // namespace texts
} // namespace finale_mus_reader
