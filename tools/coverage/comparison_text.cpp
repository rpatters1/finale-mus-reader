// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/comparison_text.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "coverage/common/font_info.h"
#include "coverage/identity.h"
#include "coverage/registry.h"
#include "coverage/schema.h"
#include "import/support/text_encoding.h"
#include "musx/dom/CommonClasses.h"
#include "musx/musx.h"
#include "musx/util/EnigmaString.h"

namespace finale_mus_reader {
namespace coverage {
namespace comparison_text {

struct TextChunk
{
    using CharsetBank = musx::dom::others::FontDefinition::CharacterSetBank;

    std::string text;
    std::shared_ptr<musx::dom::FontInfo> font;
    musx::util::EnigmaStyles::CategoryTracking category{};
    musx::dom::Evpu baseline{};
    musx::dom::Evpu superscript{};
    int tracking{};
    std::optional<std::pair<CharsetBank, int>> charset;
};

std::vector<TextChunk> enigmaChunks(const musx::dom::DocumentPtr& document,
                                    const std::string& rawText, bool dropTime)
{
    std::vector<TextChunk> result;
    auto insert =
        [dropTime](const std::vector<std::string>& components) -> std::optional<std::string> {
        if (components.empty()) return std::string{};
        if (dropTime && components.front() == "time") return std::string{};
        std::string rebuilt = '^' + components.front() + '(';
        for (std::size_t index = 1; index < components.size(); ++index) {
            if (index != 1) rebuilt += ',';
            rebuilt += components[index];
        }
        return rebuilt + ')';
    };
    musx::util::EnigmaString::parseEnigmaText(
        document, musx::dom::SCORE_PARTID, rawText,
        [&](const std::string& text, const musx::util::EnigmaStyles& styles) {
            std::optional<std::pair<TextChunk::CharsetBank, int>> charset;
            if (styles.font) {
                const auto definition =
                    document->getOthers()->get<musx::dom::others::FontDefinition>(
                        musx::dom::SCORE_PARTID, styles.font->fontId);
                if (definition) charset.emplace(definition->charsetBank, definition->charsetVal);
            }
            result.push_back({text, styles.font, styles.categoryFont, styles.baseline,
                              styles.superscript, styles.tracking, charset});
            return true;
        },
        insert);
    return result;
}

std::optional<std::vector<TextChunk>> tryEnigmaChunks(const musx::dom::DocumentPtr& document,
                                                      const std::string& rawText, bool dropTime)
{
    try {
        return enigmaChunks(document, rawText, dropTime);
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    }
}

bool sameFont(const std::shared_ptr<musx::dom::FontInfo>& left,
              const std::shared_ptr<musx::dom::FontInfo>& right)
{
    if (!left || !right) return left == right;
    try {
        return sameFontName(left->getName(), right->getName()) &&
               left->fontSize == right->fontSize &&
               left->getSizeIsPercent() == right->getSizeIsPercent() &&
               left->getEnigmaStyles() == right->getEnigmaStyles();
    } catch (...) {
        return false;
    }
}

bool sameChunkState(const TextChunk& left, const TextChunk& right)
{
    return sameFont(left.font, right.font) && left.category == right.category &&
           left.baseline == right.baseline && left.superscript == right.superscript &&
           left.tracking == right.tracking;
}

std::optional<std::string> decodeRawCodepointsAsMacRoman(const std::string& value)
{
    std::string bytes;
    for (std::size_t at = 0; at < value.size();) {
        bool matched = false;
        for (std::uint16_t byte = 0; byte <= 0xff; ++byte) {
            const auto encoded = musx::util::EnigmaString::toU8(byte);
            if (encoded.empty()) continue;
            if (value.compare(at, encoded.size(), encoded) != 0) continue;
            bytes.push_back(static_cast<char>(byte));
            at += encoded.size();
            matched = true;
            break;
        }
        if (!matched) return std::nullopt;
    }
    return text::toUtf8(bytes, musx::dom::others::FontDefinition::CharacterSetBank::MacOS, 0);
}

bool symbolFontEncodingGlitch(const std::vector<TextChunk>& source,
                              const std::vector<TextChunk>& companion,
                              bool companionHasExplicitSymbolCharset)
{
    if (source.empty() || source.size() != companion.size()) return false;
    bool affected = false;
    for (std::size_t index = 0; index < source.size(); ++index) {
        const auto& left = source[index];
        const auto& right = companion[index];
        if (!sameChunkState(left, right)) return false;
        if (left.text == right.text) continue;
        bool symbolFont = companionHasExplicitSymbolCharset;
        for (const auto& font : {left.font, right.font}) {
            if (!font) continue;
            try {
                symbolFont = symbolFont || font->calcIsSymbolFont();
            } catch (const std::invalid_argument&) {
            }
        }
        const auto converted = decodeRawCodepointsAsMacRoman(left.text);
        if (!symbolFont || !converted || *converted != right.text) return false;
        affected = true;
    }
    return affected;
}

bool chunksEqual(const std::vector<TextChunk>& source, const std::vector<TextChunk>& companion);

std::optional<std::string> reinterpretEncoding(const std::string& value,
                                               TextChunk::CharsetBank sourceBank, int sourceValue,
                                               TextChunk::CharsetBank targetBank, int targetValue)
{
    std::string bytes;
    for (std::size_t at = 0; at < value.size();) {
        bool matched = false;
        for (std::uint16_t byte = 0; byte <= 0xff; ++byte) {
            const char raw = static_cast<char>(byte);
            const auto encoded = text::toUtf8(std::string_view(&raw, 1), sourceBank, sourceValue);
            if (encoded.empty() || value.compare(at, encoded.size(), encoded) != 0) continue;
            bytes.push_back(raw);
            at += encoded.size();
            matched = true;
            break;
        }
        if (!matched) return std::nullopt;
    }
    return text::toUtf8(bytes, targetBank, targetValue);
}

bool wrongPlatformEncodingGlitch(const std::vector<TextChunk>& source,
                                 const std::vector<TextChunk>& companion)
{
    using Bank = TextChunk::CharsetBank;
    if (source.empty() || source.size() != companion.size()) return false;
    auto converted = source;
    bool affected = false;
    for (std::size_t index = 0; index < source.size(); ++index) {
        if (!sameChunkState(source[index], companion[index])) return false;
        if (!source[index].charset) continue;
        const auto [bank, value] = *source[index].charset;
        std::optional<std::pair<Bank, int>> target;
        if (bank == Bank::MacOS && value == 0)
            target.emplace(Bank::Windows, 0);
        else if (bank == Bank::MacOS && value == 29)
            target.emplace(Bank::MacOS, 0);
        else if (bank == Bank::Windows && (value == 0 || value == 1)) {
            target.emplace(Bank::MacOS, 0);
        } else if (bank == Bank::Windows && value == 238) {
            target.emplace(Bank::MacOS, 0);
        }
        if (!target) continue;
        const auto changed =
            reinterpretEncoding(source[index].text, bank, value, target->first, target->second);
        if (!changed) return false;
        converted[index].text = *changed;
        affected = affected || *changed != source[index].text;
    }
    return affected && chunksEqual(converted, companion);
}

std::optional<std::vector<char32_t>> utf8Codepoints(std::string_view value)
{
    std::vector<char32_t> result;
    for (std::size_t at = 0; at < value.size();) {
        const auto lead = static_cast<unsigned char>(value[at]);
        const std::size_t length = lead < 0x80             ? 1
                                   : (lead & 0xe0) == 0xc0 ? 2
                                   : (lead & 0xf0) == 0xe0 ? 3
                                   : (lead & 0xf8) == 0xf0 ? 4
                                                           : 0;
        if (length == 0 || at + length > value.size()) return std::nullopt;
        static constexpr unsigned char masks[] = {0, 0x7f, 0x1f, 0x0f, 0x07};
        char32_t codepoint = lead & masks[length];
        for (std::size_t offset = 1; offset < length; ++offset) {
            const auto continuation = static_cast<unsigned char>(value[at + offset]);
            if ((continuation & 0xc0) != 0x80) return std::nullopt;
            codepoint = (codepoint << 6) | (continuation & 0x3f);
        }
        result.push_back(codepoint);
        at += length;
    }
    return result;
}

bool utf16BytePairGlitch(const std::vector<TextChunk>& source,
                         const std::vector<TextChunk>& companion)
{
    using Bank = TextChunk::CharsetBank;
    if (source.empty() || source.size() != companion.size()) return false;
    static const std::regex command(R"(\^[A-Za-z]+\([^)]*\))");
    for (std::size_t index = 0; index < source.size(); ++index) {
        if (!source[index].charset) continue;
        const auto [bank, value] = *source[index].charset;
        if (value != 0 || (bank != Bank::MacOS && bank != Bank::Windows)) continue;
        const auto codepoints = utf8Codepoints(companion[index].text);
        if (!codepoints) continue;
        std::string rebuilt;
        std::string paired;
        std::size_t pairedCharacters = 0;
        const auto flush = [&] {
            if (paired.empty()) return;
            rebuilt += text::toUtf8(paired, bank, value);
            paired.clear();
        };
        for (const auto codepoint : *codepoints) {
            if (codepoint > 0xff && codepoint <= 0xffff) {
                paired.push_back(static_cast<char>(codepoint & 0xff));
                paired.push_back(static_cast<char>(codepoint >> 8));
                ++pairedCharacters;
            } else {
                flush();
                rebuilt += musx::util::EnigmaString::toU8(codepoint);
            }
        }
        flush();
        rebuilt = text::normalizeLineBreaks(std::move(rebuilt));
        if (pairedCharacters >= 2 && source[index].text == rebuilt) return true;
        const auto sourcePlain = std::regex_replace(source[index].text, command, "");
        const auto rebuiltPlain = std::regex_replace(rebuilt, command, "");
        const auto meaningful =
            std::count_if(rebuiltPlain.begin(), rebuiltPlain.end(),
                          [](unsigned char character) { return !std::isspace(character); });
        std::size_t sourceAt = 0;
        bool subsequence = true;
        for (const auto character : rebuiltPlain) {
            sourceAt = sourcePlain.find(character, sourceAt);
            if (sourceAt == std::string::npos) {
                subsequence = false;
                break;
            }
            ++sourceAt;
        }
        if (subsequence && pairedCharacters >= 2 && meaningful >= 8) return true;
    }
    return false;
}

bool chunksEqual(const std::vector<TextChunk>& source, const std::vector<TextChunk>& companion)
{
    if (source.size() != companion.size()) return false;
    for (std::size_t index = 0; index < source.size(); ++index) {
        const auto& left = source[index];
        const auto& right = companion[index];
        if (left.text != right.text || !sameFont(left.font, right.font) ||
            left.baseline != right.baseline || left.superscript != right.superscript ||
            left.tracking != right.tracking)
            return false;
    }
    return true;
}

std::vector<TextChunk> nonWhitespaceChunks(const std::vector<TextChunk>& chunks)
{
    std::vector<TextChunk> result;
    for (const auto& chunk : chunks) {
        std::size_t start = 0;
        while (start < chunk.text.size()) {
            while (start < chunk.text.size() &&
                   std::isspace(static_cast<unsigned char>(chunk.text[start])))
                ++start;
            auto end = start;
            while (end < chunk.text.size() &&
                   !std::isspace(static_cast<unsigned char>(chunk.text[end])))
                ++end;
            if (start != end) {
                auto nonWhitespace = chunk;
                nonWhitespace.text = chunk.text.substr(start, end - start);
                result.push_back(std::move(nonWhitespace));
            }
            start = end;
        }
    }
    return result;
}

bool differsOnlyByWhitespace(const std::string& source, const std::string& companion,
                             const std::vector<TextChunk>& sourceChunks,
                             const std::vector<TextChunk>& companionChunks)
{
    if (source == companion) return false;
    const auto containsWhitespace = [](const std::string& value) {
        return std::any_of(value.begin(), value.end(),
                           [](unsigned char character) { return std::isspace(character); });
    };
    return (containsWhitespace(source) || containsWhitespace(companion)) &&
           chunksEqual(nonWhitespaceChunks(sourceChunks), nonWhitespaceChunks(companionChunks));
}

std::string plainText(const std::vector<TextChunk>& chunks)
{
    std::string result;
    for (const auto& chunk : chunks)
        result += chunk.text;
    return result;
}

using TextComparison = TextClassificationResult;

TextComparison compareText(const std::string& className, const std::string& path,
                           const std::string& source, const std::string& companion,
                           const musx::dom::DocumentPtr& sourceDocument,
                           const musx::dom::DocumentPtr& companionDocument,
                           bool partNameText);

void realignCodaBlockTexts(SurveySnapshot& source, SurveySnapshot& companion,
                           const musx::dom::DocumentPtr& sourceDocument,
                           const musx::dom::DocumentPtr& companionDocument,
                           ComparisonResult& result)
{
    auto sourceFound = source.find("block_texts");
    auto companionFound = companion.find("block_texts");
    if (sourceFound == source.end() || companionFound == companion.end() ||
        !sourceFound->second.isArray() || !companionFound->second.isArray())
        return;
    auto& sourceItems = sourceFound->second.asArray();
    auto& companionItems = companionFound->second.asArray();
    const auto parseTexts = [](const Value::Array& items, const musx::dom::DocumentPtr& document) {
        std::vector<std::optional<std::vector<TextChunk>>> result;
        result.reserve(items.size());
        for (const auto& item : items) {
            const auto* value = item.find("text");
            result.push_back(value && value->isString()
                                 ? tryEnigmaChunks(document, value->asString(), false)
                                 : std::nullopt);
        }
        return result;
    };
    const auto sourceChunks = parseTexts(sourceItems, sourceDocument);
    const auto companionChunks = parseTexts(companionItems, companionDocument);
    std::set<std::size_t> matchedSource;
    std::set<std::size_t> matchedCompanion;
    for (std::size_t sourceIndex = 0; sourceIndex < sourceItems.size(); ++sourceIndex) {
        if (!sourceChunks[sourceIndex]) continue;
        for (std::size_t companionIndex = 0; companionIndex < companionItems.size();
             ++companionIndex) {
            if (matchedCompanion.contains(companionIndex)) continue;
            if (companionChunks[companionIndex] &&
                chunksEqual(*sourceChunks[sourceIndex], *companionChunks[companionIndex])) {
                matchedSource.insert(sourceIndex);
                matchedCompanion.insert(companionIndex);
                ++result.transformations[ComparisonTransformation::SemanticallyPairedCodaBlockText];
                ++result.classes[std::string(surveyorPool("block_texts"))]["block_texts"].same;
                break;
            }
        }
    }
    Value::Array remainingSource;
    Value::Array remainingCompanion;
    for (std::size_t index = 0; index < sourceItems.size(); ++index) {
        if (!matchedSource.contains(index)) {
            auto item = sourceItems[index];
            item.asObject().insert_or_assign("_report_match_key",
                                             Value("source-" + std::to_string(index)));
            remainingSource.push_back(std::move(item));
        }
    }
    for (std::size_t index = 0; index < companionItems.size(); ++index) {
        if (!matchedCompanion.contains(index)) {
            auto item = companionItems[index];
            item.asObject().insert_or_assign("_report_match_key",
                                             Value("companion-" + std::to_string(index)));
            remainingCompanion.push_back(std::move(item));
        }
    }
    sourceItems = std::move(remainingSource);
    companionItems = std::move(remainingCompanion);
}

bool enigmaTextIsPageInsertOnly(const std::string& value)
{
    static const std::regex pattern(
        R"(^(?:\^(?:font|fontid|Font|fontMus|fontTxt|fontNum|size|nfx)\([^)]*\))*\^page\([^)]*\)$)");
    return std::regex_match(value, pattern);
}

std::map<std::string, ReferentComparison>
compareTextBlockReferents(const musx::dom::DocumentPtr& sourceDocument,
                          const musx::dom::DocumentPtr& companionDocument)
{
    using TextBlock = musx::dom::others::TextBlock;
    using TextBlockKey = std::pair<musx::dom::Cmper, musx::dom::Cmper>;
    std::map<std::string, ReferentComparison> result;
    std::map<TextBlockKey, std::shared_ptr<const TextBlock>> companionByKey;
    for (const auto& item : sourceInstances<TextBlock>(companionDocument)) {
        companionByKey.emplace(TextBlockKey{item->getSourcePartId(), item->getCmper()}, item);
    }
    const auto rawText =
        [](const musx::dom::DocumentPtr& document,
           const std::shared_ptr<const TextBlock>& block) -> std::optional<std::string> {
        if (block->textId == 0) return std::nullopt;
        if (block->textType == TextBlock::TextType::Expression) {
            const auto text =
                document->getTexts()->get<musx::dom::texts::ExpressionText>(block->textId);
            return text ? std::optional(text->text) : std::nullopt;
        }
        const auto text = document->getTexts()->get<musx::dom::texts::BlockText>(block->textId);
        return text ? std::optional(text->text) : std::nullopt;
    };
    for (const auto& sourceBlock : sourceInstances<TextBlock>(sourceDocument)) {
        const auto key = TextBlockKey{sourceBlock->getSourcePartId(), sourceBlock->getCmper()};
        const auto companion = companionByKey.find(key);
        if (companion == companionByKey.end()) continue;
        const auto prefix = "text_blocks[" + partIdentityPrefix(sourceBlock->getSourcePartId()) +
                            "cmper=" + std::to_string(sourceBlock->getCmper()) + ']';
        const auto sourceText = rawText(sourceDocument, sourceBlock);
        const auto companionText = rawText(companionDocument, companion->second);
        if (!sourceText && !companionText) continue;
        if (!sourceText || !companionText) {
            if (sourceBlock->textId == 0 || companion->second->textId == 0) {
                result[prefix] = ReferentComparison::Renumbered;
            } else if (sourceBlock->textId != companion->second->textId &&
                       sourceBlock->textType != companion->second->textType) {
                result[prefix] = ReferentComparison::Renumbered;
            }
            continue;
        }
        const auto comparison =
            compareText("block_texts", {}, *sourceText, *companionText, sourceDocument,
                        companionDocument, false);
        if (comparison.differences.contains(TextDifferenceClassification::Other) ||
            comparison.differences.contains(TextDifferenceClassification::MissingRun)) {
            result[prefix] = ReferentComparison::Renumbered;
        } else {
            result[prefix] = enigmaTextIsPageInsertOnly(*sourceText) &&
                                     enigmaTextIsPageInsertOnly(*companionText)
                                 ? ReferentComparison::MatchingPageOnly
                                 : ReferentComparison::Matching;
        }
    }
    return result;
}

std::pair<std::int64_t, std::set<std::int64_t>> partNameTextIds(const SurveySnapshot& snapshot)
{
    const auto relationships = snapshot.find("relationships");
    if (relationships == snapshot.end()) return {};
    const auto* partNames = relationships->second.find("part_names");
    if (!partNames || !partNames->isObject()) return {};
    const auto* totalParts = partNames->find("total_parts");
    const auto* textIds = partNames->find("text_ids");
    std::set<std::int64_t> ids;
    if (textIds && textIds->isArray()) {
        for (const auto& id : textIds->asArray()) {
            if (id.isInteger()) ids.insert(id.asInteger());
        }
    }
    return {totalParts && totalParts->isInteger() ? totalParts->asInteger() : 0, std::move(ids)};
}

bool isPartNameText(const std::string& className, const std::string& path,
                    const SurveySnapshot& source, const SurveySnapshot& companion)
{
    if (className != "block_texts") return false;
    static const std::regex blockTextPattern(R"(^block_texts\[number=(\d+)\]\.text$)");
    std::smatch match;
    if (!std::regex_match(path, match, blockTextPattern)) return false;
    const auto textId = std::stoll(match[1].str());
    const auto [sourceParts, sourceIds] = partNameTextIds(source);
    const auto [companionParts, companionIds] = partNameTextIds(companion);
    if (sourceParts && companionParts) {
        return sourceIds.contains(textId) && companionIds.contains(textId);
    }
    if (sourceParts) return sourceIds.contains(textId);
    return companionIds.contains(textId);
}

TextComparison compareText(const std::string& className, const std::string& path,
                           const std::string& source, const std::string& companion,
                           const musx::dom::DocumentPtr& sourceDocument,
                           const musx::dom::DocumentPtr& companionDocument,
                           bool partNameText)
{
    const auto normalizeWhitespaceControls = [](std::string value) {
        std::erase_if(
            value, [](unsigned char character) { return character >= 0x01 && character <= 0x07; });
        return value;
    };
    const auto normalizedSource = normalizeWhitespaceControls(source);
    const auto normalizedCompanion = normalizeWhitespaceControls(companion);
    const bool removedWhitespaceControl =
        normalizedSource != source || normalizedCompanion != companion;
    const auto classClassifier = textDifferenceClassifier(className);
    if (classClassifier) {
        if (const auto classified =
                classClassifier({path, normalizedSource, normalizedCompanion, std::nullopt,
                                 std::nullopt, removedWhitespaceControl, partNameText})) {
            return *classified;
        }
    }
    if (normalizedSource == normalizedCompanion) {
        if (removedWhitespaceControl) {
            return {false, {TextDifferenceClassification::Whitespace}, {}};
        }
        return {true, {}, ComparisonTransformation::EquivalentEnigmaFontState};
    }
    const auto sourceChunks = tryEnigmaChunks(sourceDocument, normalizedSource, false);
    const auto companionChunks = tryEnigmaChunks(companionDocument, normalizedCompanion, false);
    if (!sourceChunks || !companionChunks) {
        std::set<TextDifferenceClassification> differences{
            TextDifferenceClassification::UnresolvedFont};
        if (removedWhitespaceControl) {
            differences.insert(TextDifferenceClassification::Whitespace);
        }
        return {false, std::move(differences), {}};
    }
    if (chunksEqual(*sourceChunks, *companionChunks)) {
        if (removedWhitespaceControl) {
            return {false, {TextDifferenceClassification::Whitespace}, {}};
        }
        return {true, {}, ComparisonTransformation::EquivalentEnigmaFontState};
    }
    const auto sourceNoTime = tryEnigmaChunks(sourceDocument, normalizedSource, true);
    const auto companionNoTime = tryEnigmaChunks(companionDocument, normalizedCompanion, true);
    if (sourceNoTime && companionNoTime && chunksEqual(*sourceNoTime, *companionNoTime)) {
        if (removedWhitespaceControl) {
            return {false, {TextDifferenceClassification::Whitespace}, {}};
        }
        return {true, {}, ComparisonTransformation::FinaleDroppedTimeInsert};
    }
    const auto sourcePlain = plainText(*sourceChunks);
    const auto companionPlain = plainText(*companionChunks);
    if (classClassifier) {
        if (const auto classified =
                classClassifier({path, normalizedSource, normalizedCompanion, sourcePlain,
                                 companionPlain, removedWhitespaceControl, partNameText})) {
            return *classified;
        }
    }
    if (differsOnlyByWhitespace(normalizedSource, normalizedCompanion, *sourceChunks,
                                *companionChunks)) {
        return {false, {TextDifferenceClassification::Whitespace}, {}};
    }
    static const std::regex explicitSymbolCharset(
        R"(\^(?:font|fontid|Font|fontMus|fontTxt|fontNum)\([^,)]*,(?:8191|8192)\))");
    if (wrongPlatformEncodingGlitch(*sourceChunks, *companionChunks) ||
        symbolFontEncodingGlitch(*sourceChunks, *companionChunks,
                                 std::regex_search(normalizedCompanion, explicitSymbolCharset))) {
        std::set<TextDifferenceClassification> differences{
            TextDifferenceClassification::KnownEncodingGlitch};
        if (removedWhitespaceControl) {
            differences.insert(TextDifferenceClassification::Whitespace);
        }
        return {false, std::move(differences), {}};
    }
    if (utf16BytePairGlitch(*sourceChunks, *companionChunks)) {
        std::set<TextDifferenceClassification> differences{
            TextDifferenceClassification::KnownEncodingGlitch};
        if (removedWhitespaceControl) {
            differences.insert(TextDifferenceClassification::Whitespace);
        }
        return {false, std::move(differences), {}};
    }
    std::set<TextDifferenceClassification> differences;
    if (removedWhitespaceControl) differences.insert(TextDifferenceClassification::Whitespace);
    if (plainText(*sourceChunks) != plainText(*companionChunks)) {
        differences.insert(TextDifferenceClassification::Other);
    }
    const auto common = (std::min)(sourceChunks->size(), companionChunks->size());
    for (std::size_t index = 0; index < common; ++index) {
        if (!sameFont((*sourceChunks)[index].font, (*companionChunks)[index].font)) {
            differences.insert(TextDifferenceClassification::Font);
        }
        if ((*sourceChunks)[index].font && (*companionChunks)[index].font) {
            if ((*sourceChunks)[index].font->fontSize != (*companionChunks)[index].font->fontSize) {
                differences.insert(TextDifferenceClassification::Size);
            }
            const auto left = (*sourceChunks)[index].font;
            const auto right = (*companionChunks)[index].font;
            if (std::tie(left->bold, left->italic, left->underline, left->strikeout, left->absolute,
                         left->hidden) != std::tie(right->bold, right->italic, right->underline,
                                                   right->strikeout, right->absolute,
                                                   right->hidden)) {
                differences.insert(TextDifferenceClassification::Effects);
            }
        }
    }
    if (sourceChunks->size() != companionChunks->size() &&
        differences.contains(TextDifferenceClassification::Other)) {
        differences.erase(TextDifferenceClassification::Other);
        differences.insert(TextDifferenceClassification::MissingRun);
    }
    static const std::regex effectsCommand(R"(\^(?:nfx|efx)\([^)]*\))");
    if (std::regex_search(normalizedSource, effectsCommand) !=
        std::regex_search(normalizedCompanion, effectsCommand)) {
        differences.insert(TextDifferenceClassification::Effects);
    }
    if (differences.empty()) differences.insert(TextDifferenceClassification::Other);
    return {false, std::move(differences), {}};
}

bool hasSynthesizedTextState(const SurveySnapshot& source, const std::string& className,
                             const std::string& path)
{
    static const std::regex numberPattern(R"(\[number=(\d+)\]\.text$)");
    std::smatch match;
    if (!std::regex_search(path, match, numberPattern)) return false;
    const auto classFound = source.find(className);
    if (classFound == source.end() || !classFound->second.isArray()) return false;
    const auto number = std::stoll(match[1].str());
    for (const auto& item : classFound->second.asArray()) {
        const auto* itemNumber = item.find("number");
        if (!itemNumber || !itemNumber->isInteger() || itemNumber->asInteger() != number) continue;
        for (const auto field : {"font_synthesized", "size_synthesized", "effects_synthesized"}) {
            if (const auto* value = item.find(field); value && value->isBool() && value->asBool()) {
                return true;
            }
        }
        return false;
    }
    return false;
}

} // namespace comparison_text
} // namespace coverage
} // namespace finale_mus_reader
