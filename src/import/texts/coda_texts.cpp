// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Text recovery for the Coda-banner epoch, which stores it two ways and neither is the text
// pool of the later eras.
//
// Block text is in the `HT` and `HS` others families, one pair per block. `HT` holds the
// characters and `HS` holds the style, and they are keyed alike: `HS` incidence n describes the
// nth `HT` record of the same comparator.
//
// Lyric text is in the text region that follows the last record pool -- two length-prefixed
// chunks, of which the second carries `^verse(n)`, `^chorus(n)` and `^section(n)` records in
// spelled-out Enigma. The first chunk is a `^text` header, empty in every document seen,
// because block text goes to `HT` instead.

#include "import/texts.h"
#include "import/texts/internal.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "import/support/enigma_text.h"
#include "import/support/legacy_mapping.h"
#include "import/support/text_encoding.h"
#include "reader/timing.h"
#include "records/legacy_record_index.h"

#include "musx/musx.h"

namespace finale_mus_reader {
namespace texts {
namespace {

using musx::dom::Cmper;
using records::LegacyRow;
using CodaTextFontType = musx::dom::options::FontOptions::FontType;

constexpr records::LegacyTag codaStyleRecord = records::packTag("HS");
constexpr records::LegacyTag codaTextRecord = records::packTag("HT");

// A block's characters occupy four consecutive `HT` incidences, so 48 bytes, however short the
// string is. What follows the terminator is a previous save's bytes and layout data, neither of
// which this reads.
constexpr std::uint32_t codaTextIncidences = 4;

// `HS` word 2 packs the two things a block needs in order to be drawn: the font comparator
// above the point size. Word 3 is the `nfx` style mask. Words 0 and 1 are the block's position,
// which musxdom keeps elsewhere.
constexpr std::size_t codaStyleFontSizeSlot = 2;
constexpr std::size_t codaStyleEffectsSlot = 3;
constexpr std::size_t codaStyleInsertArgumentSlot = 4;
constexpr std::size_t codaStyleFlagsSlot = 5;
constexpr unsigned codaStyleFontShift = 8;
constexpr std::uint16_t codaStyleSizeMask = 0x00ff;

// This era writes an insert as a single character rather than as a command, and the same
// character stands for whichever insert the block carries. Which one it is comes from the style
// record: word 5 selects it and word 4 is its argument -- a page offset for `^page`, a format
// ordinal for `^date`, the seconds flag for `^time`.
//
// **Believed: the selector is the two bits above the low pair of word 5.** Three values are
// established, one per insert the era offers, and every block with no insert leaves them clear.
// Whether they are a two-bit field or two independent flags cannot be told apart by three
// observations; an unlisted value is reported rather than guessed at.
constexpr char codaInsertCharacter = '#';
constexpr unsigned codaInsertSelectorShift = 2;
constexpr std::uint16_t codaInsertSelectorMask = 0x3;

constexpr std::string_view codaInsertCommands[] = {"page", "date", "time"};

/// @brief The characters of one block, ending at the first terminator.
std::string readCodaBlockText(const records::LegacyRowPool& pool,
    std::span<const LegacyRow> family, std::uint32_t first)
{
    std::string result;
    for (std::uint32_t offset = 0; offset < codaTextIncidences; ++offset) {
        const auto row = std::find_if(family.begin(), family.end(),
            [&](const LegacyRow& candidate) { return candidate.inci == first + offset; });
        if (row == family.end()) {
            break;
        }
        for (const auto byte : pool.payloadOf(*row)) {
            if (byte == 0) {
                return result;
            }
            result.push_back(static_cast<char>(byte));
        }
    }
    return result;
}

/// @brief Restates one block as an Enigma string, which is the form musxdom reads.
/// @details The style commands are synthesized because the era states them in a record rather
/// than in the text, and only what `HS` carries is written. A document whose page offset is
/// zero still gets `^page(0)` wherever its text holds the insert, the insert being what the
/// document states.
std::string spellCodaBlock(
    const LegacyRow& style, std::string_view characters, bool& unknownInsert)
{
    const auto packed = static_cast<std::uint16_t>(style.words[codaStyleFontSizeSlot]);
    std::string result
        = "^font(Font" + std::to_string(packed >> codaStyleFontShift) + ")";
    result += "^size(" + std::to_string(packed & codaStyleSizeMask) + ")";
    result += "^nfx(" + std::to_string(style.words[codaStyleEffectsSlot]) + ")";

    const auto selector = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(style.words[codaStyleFlagsSlot]) >> codaInsertSelectorShift)
        & codaInsertSelectorMask);
    std::string insert;
    if (selector < std::size(codaInsertCommands)) {
        insert = '^' + std::string(codaInsertCommands[selector]) + '('
            + std::to_string(style.words[codaStyleInsertArgumentSlot]) + ')';
    }
    for (std::size_t at = 0; at < characters.size(); ++at) {
        const auto character = characters[at];
        if (character == codaInsertCharacter) {
            // A doubled insert character is escaped literal content and becomes one hash,
            // parallel to Enigma's `^^` spelling for a literal caret.
            if (at + 1 < characters.size() && characters[at + 1] == codaInsertCharacter) {
                result.push_back(codaInsertCharacter);
                ++at;
                continue;
            }
            // A selector with no command behind it leaves the character as it stands: the
            // block still says something, and inventing an insert would say the wrong thing.
            if (insert.empty()) {
                unknownInsert = true;
                result.push_back(character);
            } else {
                result += insert;
            }
        } else if (character == '^') {
            // A literal caret survives as the escape every era spells it with.
            result += "^^";
        } else {
            result.push_back(character);
        }
    }
    return result;
}

/// @brief Converts a synthesized Enigma string and adds it to the document.
template <typename Target>
void addCodaText(const ImportContext& context, const text::EnigmaTextSource& source,
    Cmper number, const std::string& spelled, CodaTextFontType defaultFontType)
{
    FINALE_MUS_READER_TIMING_INCREMENT(timing::Counter::TextRecords, 1);
    FINALE_MUS_READER_TIMING_INCREMENT(timing::Counter::TextRecordBytes, spelled.size());
    auto recordSource = source;
    recordSource.initialFont = musx::dom::options::FontOptions::getFontInfoOrNull(
        context.document, defaultFontType);
    auto converted = text::toModernEnigmaText(
        std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(spelled.data()), spelled.size()),
        recordSource);
    auto instance = std::make_shared<Target>(
        context.document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, number);
    instance->text = std::move(converted.text);
    context.document->getTexts()->add(Target::XmlNodeName, instance);

    FieldInfo info;
    info.target = "texts." + std::string(Target::XmlNodeName) + '[' + std::to_string(number) + ']';
    info.origin = ValueOrigin::LegacyMus;
    recordTextFieldInfo(context.report, info.target, converted);
    context.report.fields.push_back(std::move(info));
}

void importCodaBlockTexts(const ImportContext& context, const text::EnigmaTextSource& source)
{
    const auto& pool = context.index.getOthers();
    Cmper number = 0;
    for (const auto cmper : pool.cmpersForTag(codaTextRecord)) {
        const auto characters = pool.getArray(codaTextRecord, cmper);
        const auto records
            = static_cast<std::uint32_t>(characters.size() / codaTextIncidences);
        for (std::uint32_t record = 0; record < records; ++record) {
            const auto* style = pool.get(codaStyleRecord, cmper, 0, record);
            if (!style) {
                // Without the style record there is no font, size or page offset to state, and
                // no way to tell the insert from a literal `#`. Reporting the gap is better
                // than writing a block that says something the document does not.
                context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
                    "A legacy text block carries characters with no matching style record and "
                    "was not imported."});
                continue;
            }
            ++number;
            bool unknownInsert = false;
            auto spelled = spellCodaBlock(*style,
                readCodaBlockText(pool, characters, record * codaTextIncidences), unknownInsert);
            if (unknownInsert) {
                context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
                    "A legacy text block carries an insert this reader has no command for; the "
                    "insert character was kept as it stands."});
            }
            addCodaText<musx::dom::texts::BlockText>(
                context, source, number, spelled, CodaTextFontType::TextBlock);
        }
    }
}

// The lyric keywords this era spells, and the musxdom class each names. `verse`, `chorus` and
// `section` are the same three the later text pool uses; only the framing around them differs.
struct CodaLyricKeyword
{
    std::string_view keyword;
    void (*add)(const ImportContext&, const text::EnigmaTextSource&, Cmper, const std::string&,
        CodaTextFontType);
    CodaTextFontType defaultFontType;
};

constexpr CodaLyricKeyword codaLyricKeywords[] = {
    {"verse", &addCodaText<musx::dom::texts::LyricsVerse>, CodaTextFontType::LyricVerse},
    {"chorus", &addCodaText<musx::dom::texts::LyricsChorus>, CodaTextFontType::LyricChorus},
    {"section", &addCodaText<musx::dom::texts::LyricsSection>, CodaTextFontType::LyricSection},
};

/// @brief The chunks of the text region, each a four-byte big-endian count and that many bytes.
std::vector<std::string_view> readCodaChunks(std::span<const std::uint8_t> region)
{
    constexpr std::size_t countBytes = 4;
    std::vector<std::string_view> result;
    std::size_t at = 0;
    while (at + countBytes <= region.size()) {
        std::size_t length = 0;
        for (std::size_t i = 0; i < countBytes; ++i) {
            length = (length << 8U) | region[at + i];
        }
        at += countBytes;
        if (length > region.size() - at) {
            break;
        }
        result.emplace_back(
            reinterpret_cast<const char*>(region.data() + at), length);
        at += length;
    }
    return result;
}

/// @brief The keyword and number a record opens with, when it opens with one.
struct CodaLyricRecord
{
    std::size_t keyword{};
    Cmper number{};
    std::size_t bodyStart{};
};

std::optional<CodaLyricRecord> readCodaLyricHeader(std::string_view chunk, std::size_t at)
{
    if (at >= chunk.size() || chunk[at] != '^') {
        return std::nullopt;
    }
    for (std::size_t keyword = 0; keyword < std::size(codaLyricKeywords); ++keyword) {
        const auto name = codaLyricKeywords[keyword].keyword;
        if (chunk.compare(at + 1, name.size(), name) != 0 || chunk[at + 1 + name.size()] != '(') {
            continue;
        }
        std::size_t digits = at + 2 + name.size();
        Cmper number = 0;
        while (digits < chunk.size() && chunk[digits] >= '0' && chunk[digits] <= '9') {
            number = static_cast<Cmper>(number * 10 + (chunk[digits] - '0'));
            ++digits;
        }
        if (digits >= chunk.size() || chunk[digits] != ')') {
            return std::nullopt;
        }
        // One separating space belongs to the framing rather than to the text. Finale drops it
        // when it converts these records, and a lyric that began with a space it never had
        // would shift every syllable.
        std::size_t body = digits + 1;
        if (body < chunk.size() && chunk[body] == ' ') {
            ++body;
        }
        return CodaLyricRecord{keyword, number, body};
    }
    return std::nullopt;
}

void importCodaLyricTexts(const ImportContext& context, const text::EnigmaTextSource& source)
{
    const auto chunks = readCodaChunks(context.index.getTexts());
    for (const auto chunk : chunks) {
        std::size_t at = 0;
        while (at < chunk.size()) {
            const auto header = readCodaLyricHeader(chunk, at);
            if (!header) {
                ++at;
                continue;
            }
            // A record runs to the next one, there being no terminator: the commands inside a
            // record also begin with a caret, so only a keyword this reader knows ends it.
            std::size_t end = header->bodyStart;
            while (end < chunk.size() && !readCodaLyricHeader(chunk, end)) {
                ++end;
            }
            auto body = std::string(chunk.substr(header->bodyStart, end - header->bodyStart));
            while (!body.empty() && body.back() == '\0') {
                body.pop_back();
            }
            const auto& keyword = codaLyricKeywords[header->keyword];
            keyword.add(context, source, header->number, body, keyword.defaultFontType);
            at = end;
        }
    }
}

} // namespace

void importCodaStoredTexts(const ImportContext& context)
{
    if (context.profile.epoch != FormatEpoch::CodaBanner) {
        return;
    }
    // This era predates Unicode by a wide margin, so its bytes are always a code page.
    text::EnigmaFontResolutionCache fontResolutionCache;
    const text::EnigmaTextSource source{context.document, /*utf8*/ false,
        context.profile.platform, nullptr,
        context.profile.symbolFontNames, &fontResolutionCache};
    importCodaBlockTexts(context, source);
    importCodaLyricTexts(context, source);
}

} // namespace texts
} // namespace finale_mus_reader
