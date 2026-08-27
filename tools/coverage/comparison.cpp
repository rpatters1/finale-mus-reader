// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/comparison.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string_view>
#include <tuple>
#include <unordered_set>

#include "coverage/json.h"
#include "coverage/support/source_gate.h"
#include "import/support/text_encoding.h"
#include "musx/dom/CommonClasses.h"
#include "musx/musx.h"
#include "musx/util/EnigmaString.h"

namespace finale_mus_reader {
namespace coverage {
namespace {

ClassComparison& classComparison(ComparisonResult& result, std::string_view className)
{
    return result.classes[std::string(surveyorPool(className))][std::string(className)];
}

std::string_view surveyorClass(std::string_view path)
{
    const auto separator = path.find_first_of(".[");
    return path.substr(0, separator);
}

using Leaves = std::map<std::string, std::pair<Value, std::string>>;
constexpr std::size_t maximumExamplesPerRow = 20;
constexpr double chordScalingComparisonTolerance = 0.010001;
constexpr std::string_view finaleUpgradeLossRule = "finale-upgrade-loss";
constexpr std::string_view readerCompletedConnectionArrayRule =
    "reader-completed-connection-array";
constexpr std::string_view slurConnectionStylePath =
    "smart_shape_options.slur_connect_styles[type=";
constexpr std::size_t completeSlurConnectionStyleCount =
    static_cast<std::size_t>(
        musx::dom::options::SmartShapeOptions::SlurConnectStyleType::UnderTabNumEnd) + 1;

const std::unordered_set<std::string> metadataKeys = {
    "corpus_id", "status", "epoch", "saving_product", "source_version", "header",
    "warning_count", "diagnostics", "duration_ms", "timings", "companion", "finder_type",
    "error"};

const std::unordered_set<std::string> excludedClasses = {
    "header", "layer_atts", "relationships", "spacing_options"};

bool equalChordScalingPercent(
    std::string_view path, const Value& source, const Value& companion)
{
    if (path != "chord_options.fret_percent"
            && path != "chord_options.chord_percent") return false;
    return source.isDouble() && companion.isDouble()
        && std::abs(source.asDouble() - companion.asDouble())
            <= chordScalingComparisonTolerance;
}

bool startsWith(std::string_view value, std::string_view prefix)
{
    return value.substr(0, prefix.size()) == prefix;
}

bool endsWith(std::string_view value, std::string_view suffix)
{
    return value.size() >= suffix.size()
        && value.substr(value.size() - suffix.size()) == suffix;
}

std::string snakeToCamel(std::string_view value)
{
    std::string result;
    bool uppercase = false;
    for (const char character : value) {
        if (character == '_') {
            uppercase = true;
        } else if (uppercase) {
            result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(character))));
            uppercase = false;
        } else {
            result.push_back(character);
        }
    }
    return result;
}

bool isNoncontentKey(std::string_view key)
{
    static const std::unordered_set<std::string> exact = {
        "origin", "index", "cmper", "_report_match_key", "instruction_count",
        "value_count", "external_graphic_count", "undocumented_instruction_count",
        "effects_synthesized", "font_synthesized", "size_synthesized"};
    return exact.contains(std::string(key)) || startsWith(key, "origin_")
        || endsWith(key, "_origin") || endsWith(key, "_block_offset")
        || endsWith(key, "_decoded_field_offset");
}

bool isExcludedPath(std::string_view path)
{
    static const std::vector<std::string> exact = {
        "font_options.tuples", "font_options.recovered_count",
        "font_options.legacy_behavior_count", "font_options.default_count",
        "font_options.unmapped_count", "font_options.musx_only_count",
        "font_definitions.duplicate_nonzero_name_count",
        "font_definitions.introduced_duplicate_nonzero_name_count",
        "shape_instruction_lists.instruction_types"};
    for (const auto& item : exact) {
        if (path == item || (startsWith(path, item) && path.size() > item.size()
                && (path[item.size()] == '.' || path[item.size()] == '['))) return true;
    }
    if (startsWith(path, "font_definitions.definitions[") && endsWith(path, ".name")) return true;
    if (startsWith(path, "shape_defs[")
            && (endsWith(path, ".instruction_list") || endsWith(path, ".data_list"))) return true;
    return false;
}

std::string canonicalFontName(std::string_view value)
{
    return musx::dom::normalizeFontName(std::string(value));
}

bool sameFontName(std::string_view left, std::string_view right)
{
    return canonicalFontName(left) == canonicalFontName(right);
}

std::optional<std::pair<std::string, std::string>> ordinaryListKey(const Value& item)
{
    if (!item.isObject()) return std::nullopt;
    const auto integer = [&](std::string_view key) -> const Value* {
        const auto* value = item.find(key);
        return value && value->isInteger() ? value : nullptr;
    };
    if (integer("cmper1") && integer("cmper2") && integer("inci")) {
        return std::pair{"identity", "cmper1=" + std::to_string(integer("cmper1")->asInteger())
            + ",cmper2=" + std::to_string(integer("cmper2")->asInteger())
            + ",inci=" + std::to_string(integer("inci")->asInteger())};
    }
    if (integer("cmper") && integer("inci")) {
        return std::pair{"identity", "cmper=" + std::to_string(integer("cmper")->asInteger())
            + ",inci=" + std::to_string(integer("inci")->asInteger())};
    }
    for (const auto key : {"cmper", "number", "index"}) {
        if (const auto* value = integer(key)) return std::pair{std::string(key), std::to_string(value->asInteger())};
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, std::string>> listKey(
    std::string_view path, const Value& item)
{
    if (startsWith(path, "smart_shape_options.")
            && endsWith(path, "_connect_styles") && item.isObject()) {
        if (const auto* type = item.find("type"); type && type->isInteger()) {
            return std::pair{"type", std::to_string(type->asInteger())};
        }
    }
    if (path == "font_definitions.definitions" && item.isObject()) {
        if (const auto* cmper = item.find("cmper"); cmper && cmper->isInteger()
                && cmper->asInteger() == 0) return std::pair{"cmper", "0"};
        if (const auto* name = item.find("normalized_name"); name && name->isString()) {
            return std::pair{"normalized_name", canonicalFontName(name->asString())};
        }
    }
    if (surveyorPool(surveyorClass(path)) == "texts" && item.isObject()) {
        if (const auto* reportKey = item.find("_report_match_key"); reportKey && reportKey->isString()) {
            return std::pair{"semantic", reportKey->asString()};
        }
    }
    return ordinaryListKey(item);
}

std::vector<std::string> listSegments(const Value::Array& items, std::string_view path)
{
    std::vector<std::optional<std::pair<std::string, std::string>>> keys;
    keys.reserve(items.size());
    bool allKeyed = !items.empty();
    for (const auto& item : items) {
        keys.push_back(listKey(path, item));
        allKeyed = allKeyed && keys.back().has_value();
    }
    std::vector<std::string> result;
    std::map<std::pair<std::string, std::string>, std::size_t> seen;
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (!allKeyed) {
            result.push_back("[" + std::to_string(index) + "]");
            continue;
        }
        const auto& [field, key] = *keys[index];
        const auto occurrence = ++seen[*keys[index]];
        const auto suffix = occurrence == 1 ? "" : "#" + std::to_string(occurrence);
        if (field == "index" || field == "identity") result.push_back("[" + key + suffix + "]");
        else result.push_back("[" + field + "=" + key + suffix + "]");
    }
    return result;
}

void collectLeaves(const Value& value, std::string path, std::string origin,
    bool includeOrigins, Leaves& result)
{
    if (value.isObject()) {
        if (includeOrigins) {
            if (const auto* objectOrigin = value.find("origin");
                    objectOrigin && objectOrigin->isString()) origin = objectOrigin->asString();
        }
        for (const auto& [key, child] : value.asObject()) {
            if (metadataKeys.contains(key) || isNoncontentKey(key)) continue;
            const auto childPath = path.empty() ? key : path + '.' + key;
            if (isExcludedPath(childPath)) continue;
            std::string childOrigin = origin;
            if (includeOrigins) {
                const auto camelOrigin = "origin_" + snakeToCamel(key);
                const auto suffixOrigin = key + "_origin";
                if (const auto* found = value.find(camelOrigin); found && found->isString()) {
                    childOrigin = found->asString();
                } else if (const auto* found = value.find(suffixOrigin);
                        found && found->isString()) {
                    childOrigin = found->asString();
                }
            }
            collectLeaves(child, childPath, childOrigin, includeOrigins, result);
        }
    } else if (value.isArray()) {
        const auto segments = listSegments(value.asArray(), path);
        for (std::size_t index = 0; index < value.asArray().size(); ++index) {
            collectLeaves(value.asArray()[index], path + segments[index], origin, includeOrigins, result);
        }
    } else {
        result.insert_or_assign(std::move(path), std::pair{value, std::move(origin)});
    }
}

std::string objectPrefix(std::string_view path)
{
    const auto end = path.find(']');
    return end == std::string_view::npos ? std::string{} : std::string(path.substr(0, end + 1));
}

std::optional<std::int64_t> integerLeaf(const Leaves& leaves, const std::string& path)
{
    const auto found = leaves.find(path);
    if (found == leaves.end() || !found->second.first.isInteger()) return std::nullopt;
    return found->second.first.asInteger();
}

std::set<std::int64_t> slurConnectionStyleTypes(const Leaves& leaves)
{
    std::set<std::int64_t> types;
    for (const auto& [path, value] : leaves) {
        if (!startsWith(path, slurConnectionStylePath) || !endsWith(path, "].type")
                || !value.first.isInteger()) continue;
        types.insert(value.first.asInteger());
    }
    return types;
}

std::optional<std::string> omittedSlurConnectionStyleDifference(
    const std::string& path, const std::string& category,
    const Leaves& source, const Leaves& companion)
{
    if (category != "reader_only" || !startsWith(path, slurConnectionStylePath)) {
        return std::nullopt;
    }
    const auto close = path.find(']');
    if (close == std::string::npos) return std::nullopt;
    const auto prefix = path.substr(0, close + 1);
    const auto type = integerLeaf(source, prefix + ".type");
    const auto sourceTypes = slurConnectionStyleTypes(source);
    const auto companionTypes = slurConnectionStyleTypes(companion);
    bool sourceIsComplete = sourceTypes.size() == completeSlurConnectionStyleCount;
    for (std::size_t index = 0;
            sourceIsComplete && index < completeSlurConnectionStyleCount; ++index) {
        sourceIsComplete = sourceTypes.contains(static_cast<std::int64_t>(index));
    }
    if (!type || !sourceIsComplete
            || (companionTypes.size() != 4 && companionTypes.size() != 25)
            || companionTypes.contains(*type)) {
        return std::nullopt;
    }

    bool hasNonzeroLegacyValue = false;
    for (const auto suffix : {".connect_index", ".x", ".y"}) {
        const auto found = source.find(prefix + suffix);
        if (found == source.end() || !found->second.first.isInteger()
                || (found->second.second != "legacy-mus"
                    && found->second.second != "finale27-default")) {
            return std::nullopt;
        }
        if (found->second.second == "legacy-mus" && found->second.first.asInteger() != 0) {
            hasNonzeroLegacyValue = true;
        }
    }
    return std::string(hasNonzeroLegacyValue
        ? finaleUpgradeLossRule : readerCompletedConnectionArrayRule);
}

bool equalSurrounding(const Leaves& source, const Leaves& companion,
    std::string_view prefix, std::string_view excluded)
{
    std::set<std::string> paths;
    for (const auto& [path, unused] : source) if (startsWith(path, prefix) && path != excluded) paths.insert(path);
    for (const auto& [path, unused] : companion) if (startsWith(path, prefix) && path != excluded) paths.insert(path);
    for (const auto& path : paths) {
        const auto sourceFound = source.find(path);
        const auto companionFound = companion.find(path);
        if (sourceFound == source.end() || companionFound == companion.end()
                || sourceFound->second.first != companionFound->second.first) return false;
    }
    return true;
}

bool equalOmittedZeroInsertFont(const std::string& path, const Value& sourceValue,
    const Value& companionValue, const Leaves& source, const Leaves& companion)
{
    if (!startsWith(path, "text_options.inserts[")) return false;
    const auto separator = path.rfind('.');
    if (separator == std::string::npos) return false;
    const auto prefix = path.substr(0, separator);
    if (endsWith(path, ".has_font")) {
        if (!sourceValue.isBool() || !companionValue.isBool()
                || !sourceValue.asBool() || companionValue.asBool()) return false;
    } else if (endsWith(path, ".font_name") || endsWith(path, ".normalized_font_name")) {
        if (!sourceValue.isString() || !companionValue.isString()
                || sourceValue.asString().empty() || !companionValue.asString().empty()) return false;
    } else {
        return false;
    }
    for (const auto* field : {".font_id", ".font_size", ".font_effects"}) {
        if (integerLeaf(source, prefix + field) != 0
                || integerLeaf(companion, prefix + field) != 0) return false;
    }
    for (const auto* field : {".present", ".tracking_before", ".tracking_after",
             ".baseline_shift_perc", ".sym_char", ".dangling_font"}) {
        const auto left = source.find(prefix + field);
        const auto right = companion.find(prefix + field);
        if (left == source.end() || right == companion.end()
                || left->second.first != right->second.first) return false;
    }
    return true;
}

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
    auto insert = [dropTime](const std::vector<std::string>& components)
        -> std::optional<std::string> {
        if (components.empty()) return std::string{};
        if (dropTime && components.front() == "time") return std::string{};
        std::string rebuilt = '^' + components.front() + '(';
        for (std::size_t index = 1; index < components.size(); ++index) {
            if (index != 1) rebuilt += ',';
            rebuilt += components[index];
        }
        return rebuilt + ')';
    };
    musx::util::EnigmaString::parseEnigmaText(document, musx::dom::SCORE_PARTID, rawText,
        [&](const std::string& text, const musx::util::EnigmaStyles& styles) {
            std::optional<std::pair<TextChunk::CharsetBank, int>> charset;
            if (styles.font) {
                const auto definition = document->getOthers()
                    ->get<musx::dom::others::FontDefinition>(
                        musx::dom::SCORE_PARTID, styles.font->fontId);
                if (definition) charset.emplace(definition->charsetBank, definition->charsetVal);
            }
            result.push_back({text, styles.font, styles.categoryFont, styles.baseline,
                styles.superscript, styles.tracking, charset});
            return true;
        }, insert);
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
        return sameFontName(left->getName(), right->getName())
            && left->fontSize == right->fontSize
            && left->getSizeIsPercent() == right->getSizeIsPercent()
            && left->getEnigmaStyles() == right->getEnigmaStyles();
    } catch (...) {
        return false;
    }
}

bool sameChunkState(const TextChunk& left, const TextChunk& right)
{
    return sameFont(left.font, right.font) && left.category == right.category
        && left.baseline == right.baseline && left.superscript == right.superscript
        && left.tracking == right.tracking;
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
    return text::toUtf8(bytes,
        musx::dom::others::FontDefinition::CharacterSetBank::MacOS, 0);
}

bool symbolFontEncodingGlitch(const std::vector<TextChunk>& source,
    const std::vector<TextChunk>& companion, bool companionHasExplicitSymbolCharset,
    const text::SymbolFontNames* symbolFontNames)
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
                if (symbolFontNames) {
                    symbolFont = symbolFont || symbolFontNames->contains(
                        canonicalFontName(font->getName()));
                }
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
            const auto encoded = text::toUtf8(
                std::string_view(&raw, 1), sourceBank, sourceValue);
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
        if (bank == Bank::MacOS && value == 0) target.emplace(Bank::Windows, 0);
        else if (bank == Bank::MacOS && value == 29) target.emplace(Bank::MacOS, 0);
        else if (bank == Bank::Windows && (value == 0 || value == 1)) {
            target.emplace(Bank::MacOS, 0);
        } else if (bank == Bank::Windows && value == 238) {
            target.emplace(Bank::MacOS, 0);
        }
        if (!target) continue;
        const auto changed = reinterpretEncoding(source[index].text, bank, value,
            target->first, target->second);
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
        const std::size_t length = lead < 0x80 ? 1 : (lead & 0xe0) == 0xc0 ? 2
            : (lead & 0xf0) == 0xe0 ? 3 : (lead & 0xf8) == 0xf0 ? 4 : 0;
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
        const auto meaningful = std::count_if(rebuiltPlain.begin(), rebuiltPlain.end(),
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
        if (left.text != right.text || !sameFont(left.font, right.font)
                || left.baseline != right.baseline || left.superscript != right.superscript
                || left.tracking != right.tracking) return false;
    }
    return true;
}

std::vector<TextChunk> nonWhitespaceChunks(const std::vector<TextChunk>& chunks)
{
    std::vector<TextChunk> result;
    for (const auto& chunk : chunks) {
        std::size_t start = 0;
        while (start < chunk.text.size()) {
            while (start < chunk.text.size()
                    && std::isspace(static_cast<unsigned char>(chunk.text[start]))) ++start;
            auto end = start;
            while (end < chunk.text.size()
                    && !std::isspace(static_cast<unsigned char>(chunk.text[end]))) ++end;
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
    const std::vector<TextChunk>& sourceChunks, const std::vector<TextChunk>& companionChunks)
{
    if (source == companion) return false;
    const auto containsWhitespace = [](const std::string& value) {
        return std::any_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isspace(character);
        });
    };
    return (containsWhitespace(source) || containsWhitespace(companion))
        && chunksEqual(nonWhitespaceChunks(sourceChunks), nonWhitespaceChunks(companionChunks));
}

std::string plainText(const std::vector<TextChunk>& chunks)
{
    std::string result;
    for (const auto& chunk : chunks) result += chunk.text;
    return result;
}

struct TextComparison
{
    bool equivalent{};
    std::set<std::string> differences;
    std::string transformation;
};

TextComparison compareText(const std::string& className, const std::string& source,
    const std::string& companion, const musx::dom::DocumentPtr& sourceDocument,
    const musx::dom::DocumentPtr& companionDocument,
    const text::SymbolFontNames* symbolFontNames, bool partNameText);

void realignCodaBlockTexts(SurveySnapshot& source, SurveySnapshot& companion,
    const musx::dom::DocumentPtr& sourceDocument,
    const musx::dom::DocumentPtr& companionDocument, ComparisonResult& result)
{
    auto sourceFound = source.find("block_texts");
    auto companionFound = companion.find("block_texts");
    if (sourceFound == source.end() || companionFound == companion.end()
            || !sourceFound->second.isArray() || !companionFound->second.isArray()) return;
    auto& sourceItems = sourceFound->second.asArray();
    auto& companionItems = companionFound->second.asArray();
    const auto parseTexts = [](const Value::Array& items,
                                const musx::dom::DocumentPtr& document) {
        std::vector<std::optional<std::vector<TextChunk>>> result;
        result.reserve(items.size());
        for (const auto& item : items) {
            const auto* value = item.find("text");
            result.push_back(value && value->isString()
                ? tryEnigmaChunks(document, value->asString(), false) : std::nullopt);
        }
        return result;
    };
    const auto sourceChunks = parseTexts(sourceItems, sourceDocument);
    const auto companionChunks = parseTexts(companionItems, companionDocument);
    std::set<std::size_t> matchedSource;
    std::set<std::size_t> matchedCompanion;
    for (std::size_t sourceIndex = 0; sourceIndex < sourceItems.size(); ++sourceIndex) {
        if (!sourceChunks[sourceIndex]) continue;
        for (std::size_t companionIndex = 0; companionIndex < companionItems.size(); ++companionIndex) {
            if (matchedCompanion.contains(companionIndex)) continue;
            if (companionChunks[companionIndex]
                    && chunksEqual(*sourceChunks[sourceIndex], *companionChunks[companionIndex])) {
                matchedSource.insert(sourceIndex);
                matchedCompanion.insert(companionIndex);
                ++result.transformations["Semantically paired Coda block text"];
                ++classComparison(result, "block_texts").same;
                break;
            }
        }
    }
    Value::Array remainingSource;
    Value::Array remainingCompanion;
    for (std::size_t index = 0; index < sourceItems.size(); ++index) {
        if (!matchedSource.contains(index)) {
            auto item = sourceItems[index];
            item.asObject().insert_or_assign("_report_match_key", Value("source-" + std::to_string(index)));
            remainingSource.push_back(std::move(item));
        }
    }
    for (std::size_t index = 0; index < companionItems.size(); ++index) {
        if (!matchedCompanion.contains(index)) {
            auto item = companionItems[index];
            item.asObject().insert_or_assign("_report_match_key", Value("companion-" + std::to_string(index)));
            remainingCompanion.push_back(std::move(item));
        }
    }
    sourceItems = std::move(remainingSource);
    companionItems = std::move(remainingCompanion);
}

enum class ReferentComparison
{
    None,
    Matching,
    MatchingPageOnly,
    Renumbered
};

bool enigmaTextIsPageInsertOnly(const std::string& value)
{
    static const std::regex pattern(
        R"(^(?:\^(?:font|fontid|Font|fontMus|fontTxt|fontNum|size|nfx)\([^)]*\))*\^page\([^)]*\)$)");
    return std::regex_match(value, pattern);
}

std::map<std::string, ReferentComparison> compareTextBlockReferents(
    const musx::dom::DocumentPtr& sourceDocument,
    const musx::dom::DocumentPtr& companionDocument,
    const text::SymbolFontNames* symbolFontNames)
{
    using TextBlock = musx::dom::others::TextBlock;
    std::map<std::string, ReferentComparison> result;
    std::map<musx::dom::Cmper, std::shared_ptr<const TextBlock>> companionByCmper;
    for (const auto& item : companionDocument->getOthers()->getArray<TextBlock>(musx::dom::SCORE_PARTID)) {
        companionByCmper.emplace(item->getCmper(), item);
    }
    const auto rawText = [](const musx::dom::DocumentPtr& document,
                             const std::shared_ptr<const TextBlock>& block) -> std::optional<std::string> {
        if (block->textId == 0) return std::nullopt;
        if (block->textType == TextBlock::TextType::Expression) {
            const auto text = document->getTexts()->get<musx::dom::texts::ExpressionText>(block->textId);
            return text ? std::optional(text->text) : std::nullopt;
        }
        const auto text = document->getTexts()->get<musx::dom::texts::BlockText>(block->textId);
        return text ? std::optional(text->text) : std::nullopt;
    };
    for (const auto& sourceBlock :
         sourceDocument->getOthers()->getArray<TextBlock>(musx::dom::SCORE_PARTID)) {
        const auto companion = companionByCmper.find(sourceBlock->getCmper());
        if (companion == companionByCmper.end()) continue;
        const auto prefix = "text_blocks[cmper=" + std::to_string(sourceBlock->getCmper()) + ']';
        const auto sourceText = rawText(sourceDocument, sourceBlock);
        const auto companionText = rawText(companionDocument, companion->second);
        if (!sourceText && !companionText) continue;
        if (!sourceText || !companionText) {
            if (sourceBlock->textId == 0 || companion->second->textId == 0) {
                result[prefix] = ReferentComparison::Renumbered;
            } else if (sourceBlock->textId != companion->second->textId
                    && sourceBlock->textType != companion->second->textType) {
                result[prefix] = ReferentComparison::Renumbered;
            }
            continue;
        }
        const auto comparison = compareText("block_texts", *sourceText, *companionText,
            sourceDocument, companionDocument, symbolFontNames, false);
        if (comparison.differences.contains("other")
                || comparison.differences.contains("missing run")) {
            result[prefix] = ReferentComparison::Renumbered;
        } else {
            result[prefix] = enigmaTextIsPageInsertOnly(*sourceText)
                    && enigmaTextIsPageInsertOnly(*companionText)
                ? ReferentComparison::MatchingPageOnly : ReferentComparison::Matching;
        }
    }
    return result;
}

std::int64_t integerMember(const Value& object, std::string_view key, std::int64_t fallback = 0)
{
    const auto* value = object.find(key);
    return value && value->isInteger() ? value->asInteger() : fallback;
}

std::string instructionSignature(const Value& list, std::size_t skip = 0)
{
    const auto* instructions = list.find("instructions");
    if (!instructions || !instructions->isArray()) return {};
    std::string result;
    for (std::size_t index = skip; index < instructions->asArray().size(); ++index) {
        const auto& instruction = instructions->asArray()[index];
        result += std::to_string(integerMember(instruction, "type")) + ':'
            + std::to_string(integerMember(instruction, "num_data")) + ';';
    }
    return result;
}

std::map<std::int64_t, Value*> objectsByCmper(Value::Array& items)
{
    std::map<std::int64_t, Value*> result;
    for (auto& item : items) result.emplace(integerMember(item, "cmper"), &item);
    return result;
}

std::map<std::int64_t, const Value*> objectsByCmper(const Value::Array& items)
{
    std::map<std::int64_t, const Value*> result;
    for (const auto& item : items) result.emplace(integerMember(item, "cmper"), &item);
    return result;
}

Value::Array* nestedArray(SurveySnapshot& snapshot, std::string_view object,
    std::string_view array)
{
    auto found = snapshot.find(object);
    if (found == snapshot.end()) return nullptr;
    auto* value = found->second.find(array);
    return value && value->isArray() ? &value->asArray() : nullptr;
}

Value::Array* topArray(SurveySnapshot& snapshot, std::string_view key)
{
    auto found = snapshot.find(key);
    return found != snapshot.end() && found->second.isArray() ? &found->second.asArray() : nullptr;
}

std::map<std::int64_t, std::size_t> consumedLengths(
    const Value::Array& shapes, const std::map<std::int64_t, const Value*>& lists)
{
    std::map<std::int64_t, std::size_t> result;
    for (const auto& shape : shapes) {
        const auto listFound = lists.find(integerMember(shape, "instruction_list"));
        if (listFound == lists.end()) continue;
        std::size_t consumed = 0;
        if (const auto* instructions = listFound->second->find("instructions");
                instructions && instructions->isArray()) {
            for (const auto& instruction : instructions->asArray()) {
                consumed += static_cast<std::size_t>(integerMember(instruction, "num_data"));
            }
        }
        result.try_emplace(integerMember(shape, "data_list"), consumed);
    }
    return result;
}

std::map<std::int64_t, std::set<std::size_t>> setFontPositions(
    const Value::Array& shapes, const std::map<std::int64_t, const Value*>& lists)
{
    std::map<std::int64_t, std::set<std::size_t>> result;
    for (const auto& shape : shapes) {
        const auto listFound = lists.find(integerMember(shape, "instruction_list"));
        if (listFound == lists.end()) continue;
        std::size_t offset = 0;
        const auto* instructions = listFound->second->find("instructions");
        if (!instructions || !instructions->isArray()) continue;
        for (const auto& instruction : instructions->asArray()) {
            if (integerMember(instruction, "type") == 20) {
                result[integerMember(shape, "data_list")].insert(offset);
            }
            offset += static_cast<std::size_t>(integerMember(instruction, "num_data"));
        }
    }
    return result;
}

std::string dataSignature(const Value& buffer, const std::set<std::size_t>& masked,
    std::optional<std::size_t> consumed, std::size_t skip = 0)
{
    const auto* values = buffer.find("values");
    if (!values || !values->isArray()) return {};
    const auto limit = consumed ? (std::min)(*consumed, values->asArray().size())
                                : values->asArray().size();
    std::string result;
    for (std::size_t index = skip; index < limit; ++index) {
        result += masked.contains(index) ? "*;"
            : std::to_string(integerMember(values->asArray()[index], "value")) + ';';
    }
    return result;
}

template <typename Signature>
std::map<std::int64_t, std::int64_t> matchBySignature(
    const Value::Array& source, const Value::Array& companion, Signature signature)
{
    std::map<std::int64_t, std::int64_t> result;
    std::set<std::int64_t> consumedSource;
    const auto sourceByCmper = objectsByCmper(source);
    for (const auto& item : companion) {
        const auto cmper = integerMember(item, "cmper");
        const auto found = sourceByCmper.find(cmper);
        if (found != sourceByCmper.end() && signature(*found->second, true) == signature(item, false)) {
            result[cmper] = cmper;
            consumedSource.insert(cmper);
        }
    }
    std::map<std::string, std::vector<std::int64_t>> available;
    for (const auto& item : source) {
        const auto cmper = integerMember(item, "cmper");
        if (!consumedSource.contains(cmper)) available[signature(item, true)].push_back(cmper);
    }
    for (const auto& item : companion) {
        const auto cmper = integerMember(item, "cmper");
        if (result.contains(cmper)) continue;
        auto& candidates = available[signature(item, false)];
        if (!candidates.empty()) {
            result[cmper] = candidates.front();
            candidates.erase(candidates.begin());
        }
    }
    return result;
}

std::map<std::int64_t, std::int64_t> safeRenumbering(const Value::Array& source,
    const Value::Array& companion, const std::map<std::int64_t, std::int64_t>& matches)
{
    std::set<std::int64_t> sourceCmpers;
    std::set<std::int64_t> allCmpers;
    for (const auto& item : source) sourceCmpers.insert(integerMember(item, "cmper"));
    for (const auto& item : companion) allCmpers.insert(integerMember(item, "cmper"));
    allCmpers.insert(sourceCmpers.begin(), sourceCmpers.end());
    auto next = allCmpers.empty() ? 1 : *allCmpers.rbegin() + 1;
    std::map<std::int64_t, std::int64_t> result;
    for (const auto& item : companion) {
        const auto cmper = integerMember(item, "cmper");
        const auto match = matches.find(cmper);
        if (match != matches.end()) result[cmper] = match->second;
        else if (sourceCmpers.contains(cmper)) result[cmper] = next++;
        else result[cmper] = cmper;
    }
    return result;
}

std::size_t realignShapes(SurveySnapshot& source, SurveySnapshot& companion)
{
    auto* sourceShapes = topArray(source, "shape_defs");
    auto* companionShapes = topArray(companion, "shape_defs");
    auto* sourceLists = nestedArray(source, "shape_instruction_lists", "lists");
    auto* companionLists = nestedArray(companion, "shape_instruction_lists", "lists");
    auto* sourceBuffers = nestedArray(source, "shape_data", "buffers");
    auto* companionBuffers = nestedArray(companion, "shape_data", "buffers");
    if (!sourceShapes || !companionShapes || !sourceLists || !companionLists
            || !sourceBuffers || !companionBuffers) return 0;

    auto instructionMatches = matchBySignature(*sourceLists, *companionLists,
        [](const Value& item, bool) { return instructionSignature(item); });
    const auto sourceListsByCmper = objectsByCmper(std::as_const(*sourceLists));
    const auto companionListsByCmper = objectsByCmper(std::as_const(*companionLists));
    const auto sourceConsumed = consumedLengths(*sourceShapes, sourceListsByCmper);
    auto companionConsumed = consumedLengths(*companionShapes, companionListsByCmper);
    const auto sourceFontPositions = setFontPositions(*sourceShapes, sourceListsByCmper);
    const auto companionFontPositions = setFontPositions(*companionShapes, companionListsByCmper);
    const auto dataSignatureFor = [&](const Value& item, bool isSource) {
        const auto cmper = integerMember(item, "cmper");
        const auto& positions = isSource ? sourceFontPositions : companionFontPositions;
        const auto& consumed = isSource ? sourceConsumed : companionConsumed;
        const auto positionFound = positions.find(cmper);
        const auto consumedFound = consumed.find(cmper);
        return dataSignature(item,
            positionFound == positions.end() ? std::set<std::size_t>{} : positionFound->second,
            consumedFound == consumed.end() ? std::nullopt
                : std::optional<std::size_t>(consumedFound->second));
    };
    auto dataMatches = matchBySignature(*sourceBuffers, *companionBuffers, dataSignatureFor);
    const auto sourceBuffersByCmper = objectsByCmper(std::as_const(*sourceBuffers));
    const auto companionBuffersConstByCmper = objectsByCmper(std::as_const(*companionBuffers));
    auto companionBuffersByCmper = objectsByCmper(*companionBuffers);

    const auto shapeSignature = [&](const Value& shape, bool isSource) {
        const auto& lists = isSource ? sourceListsByCmper : companionListsByCmper;
        const auto& buffers = isSource ? sourceBuffersByCmper : companionBuffersConstByCmper;
        const auto list = lists.find(integerMember(shape, "instruction_list"));
        const auto buffer = buffers.find(integerMember(shape, "data_list"));
        return (list == lists.end() ? std::string{} : instructionSignature(*list->second)) + '|'
            + (buffer == buffers.end() ? std::string{} : dataSignatureFor(*buffer->second, isSource));
    };
    auto shapeMatches = matchBySignature(*sourceShapes, *companionShapes, shapeSignature);

    std::map<std::string, std::vector<std::int64_t>> sourceShapesBySignature;
    for (const auto& shape : *sourceShapes) {
        const auto cmper = integerMember(shape, "cmper");
        bool already = false;
        for (const auto& [unused, target] : shapeMatches) if (target == cmper) already = true;
        if (!already) sourceShapesBySignature[shapeSignature(shape, true)].push_back(cmper);
    }
    const auto sourceShapesByCmper = objectsByCmper(std::as_const(*sourceShapes));
    std::size_t wrapperCount = 0;
    for (auto& shape : *companionShapes) {
        const auto cmper = integerMember(shape, "cmper");
        if (shapeMatches.contains(cmper)) continue;
        const auto listCmper = integerMember(shape, "instruction_list");
        const auto dataCmper = integerMember(shape, "data_list");
        const auto listFound = companionListsByCmper.find(listCmper);
        const auto bufferFound = companionBuffersByCmper.find(dataCmper);
        if (listFound == companionListsByCmper.end() || bufferFound == companionBuffersByCmper.end()) continue;
        const auto* instructions = listFound->second->find("instructions");
        if (!instructions || !instructions->isArray() || instructions->asArray().empty()
                || integerMember(instructions->asArray().front(), "type") != 25) continue;
        const auto numData = static_cast<std::size_t>(
            integerMember(instructions->asArray().front(), "num_data"));
        const auto stripped = instructionSignature(*listFound->second, 1) + '|'
            + dataSignature(*bufferFound->second,
                companionFontPositions.contains(dataCmper)
                    ? companionFontPositions.at(dataCmper) : std::set<std::size_t>{},
                companionConsumed.contains(dataCmper)
                    ? std::optional<std::size_t>(companionConsumed.at(dataCmper)) : std::nullopt,
                numData);
        auto& candidates = sourceShapesBySignature[stripped];
        if (candidates.empty()) continue;
        const auto sourceCmper = candidates.front();
        candidates.erase(candidates.begin());
        shapeMatches[cmper] = sourceCmper;
        const auto* sourceShape = sourceShapesByCmper.at(sourceCmper);
        instructionMatches[listCmper] = integerMember(*sourceShape, "instruction_list");
        dataMatches[dataCmper] = integerMember(*sourceShape, "data_list");
        auto listMutable = objectsByCmper(*companionLists).at(listCmper);
        listMutable->find("instructions")->asArray().erase(
            listMutable->find("instructions")->asArray().begin());
        auto& values = bufferFound->second->find("values")->asArray();
        values.erase(values.begin(), values.begin() + (std::min)(numData, values.size()));
        ++wrapperCount;
    }

    const auto shapeFinal = safeRenumbering(*sourceShapes, *companionShapes, shapeMatches);
    const auto instructionFinal = safeRenumbering(*sourceLists, *companionLists, instructionMatches);
    const auto dataFinal = safeRenumbering(*sourceBuffers, *companionBuffers, dataMatches);
    companionConsumed = consumedLengths(*companionShapes, companionListsByCmper);
    for (auto& buffer : *sourceBuffers) {
        const auto cmper = integerMember(buffer, "cmper");
        if (auto* values = buffer.find("values"); values && sourceConsumed.contains(cmper)
                && values->asArray().size() > sourceConsumed.at(cmper)) {
            values->asArray().resize(sourceConsumed.at(cmper));
        }
    }
    for (auto& shape : *companionShapes) {
        const auto cmper = integerMember(shape, "cmper");
        const auto list = integerMember(shape, "instruction_list");
        const auto data = integerMember(shape, "data_list");
        shape.asObject()["cmper"] = Value(shapeFinal.at(cmper));
        if (instructionFinal.contains(list)) shape.asObject()["instruction_list"] = Value(instructionFinal.at(list));
        if (dataFinal.contains(data)) shape.asObject()["data_list"] = Value(dataFinal.at(data));
    }
    for (auto& list : *companionLists) {
        const auto cmper = integerMember(list, "cmper");
        list.asObject()["cmper"] = Value(instructionFinal.at(cmper));
        if (auto* instructions = list.find("instructions")) {
            for (std::size_t index = 0; index < instructions->asArray().size(); ++index) {
                instructions->asArray()[index].asObject()["index"] = Value(static_cast<std::int64_t>(index));
            }
        }
    }
    for (auto& buffer : *companionBuffers) {
        const auto cmper = integerMember(buffer, "cmper");
        buffer.asObject()["cmper"] = Value(dataFinal.at(cmper));
        if (auto* values = buffer.find("values")) {
            const auto consumed = companionConsumed.find(cmper);
            if (consumed != companionConsumed.end() && values->asArray().size() > consumed->second) {
                values->asArray().resize(consumed->second);
            }
            for (std::size_t index = 0; index < values->asArray().size(); ++index) {
                values->asArray()[index].asObject()["index"] = Value(static_cast<std::int64_t>(index));
            }
        }
    }
    if (auto found = companion.find("clef_options"); found != companion.end()) {
        if (auto* clefs = found->second.find("clef_defs"); clefs && clefs->isArray()) {
            for (auto& clef : clefs->asArray()) {
                const auto id = integerMember(clef, "shape_id");
                if (shapeFinal.contains(id)) clef.asObject()["shape_id"] = Value(shapeFinal.at(id));
            }
        }
    }
    if (auto found = companion.find("mmrest_options"); found != companion.end()) {
        const auto id = integerMember(found->second, "shape_def");
        if (shapeFinal.contains(id)) found->second.asObject()["shape_def"] = Value(shapeFinal.at(id));
    }
    return wrapperCount;
}

std::set<std::string> shapeFontPaths(const SurveySnapshot& snapshot)
{
    std::set<std::string> result;
    const auto shapesFound = snapshot.find("shape_defs");
    const auto listsFound = snapshot.find("shape_instruction_lists");
    if (shapesFound == snapshot.end() || !shapesFound->second.isArray()
            || listsFound == snapshot.end()) return result;
    const auto* listsValue = listsFound->second.find("lists");
    if (!listsValue || !listsValue->isArray()) return result;
    const auto lists = objectsByCmper(listsValue->asArray());
    for (const auto& shape : shapesFound->second.asArray()) {
        const auto list = lists.find(integerMember(shape, "instruction_list"));
        if (list == lists.end()) continue;
        const auto* instructions = list->second->find("instructions");
        if (!instructions || !instructions->isArray()) continue;
        std::size_t offset = 0;
        for (const auto& instruction : instructions->asArray()) {
            if (integerMember(instruction, "type") == 20) {
                result.insert("shape_data.buffers[cmper="
                    + std::to_string(integerMember(shape, "data_list")) + "].values["
                    + std::to_string(offset) + "].value");
            }
            offset += static_cast<std::size_t>(integerMember(instruction, "num_data"));
        }
    }
    return result;
}

std::string fontIdentity(const SurveySnapshot& snapshot, std::int64_t id)
{
    const auto found = snapshot.find("font_definitions");
    if (found == snapshot.end()) return {};
    const auto* definitions = found->second.find("definitions");
    if (!definitions || !definitions->isArray()) return {};
    for (const auto& definition : definitions->asArray()) {
        if (integerMember(definition, "cmper", -1) != id) continue;
        if (const auto* name = definition.find("normalized_name"); name && name->isString()) {
            return canonicalFontName(name->asString());
        }
    }
    return {};
}

bool isFontReferencePath(std::string_view path,
    const std::set<std::string>& shapeFontPaths)
{
    return endsWith(path, "_font_id") || shapeFontPaths.contains(std::string(path));
}

std::pair<std::int64_t, std::set<std::int64_t>> partNameTextIds(
    const SurveySnapshot& snapshot)
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
    return {totalParts && totalParts->isInteger() ? totalParts->asInteger() : 0,
        std::move(ids)};
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

TextComparison compareText(const std::string& className, const std::string& source,
    const std::string& companion, const musx::dom::DocumentPtr& sourceDocument,
    const musx::dom::DocumentPtr& companionDocument,
    const text::SymbolFontNames* symbolFontNames, bool partNameText)
{
    const auto normalizeWhitespaceControls = [](std::string value) {
        std::erase_if(value, [](unsigned char character) {
            return character >= 0x01 && character <= 0x07;
        });
        return value;
    };
    const auto normalizedSource = normalizeWhitespaceControls(source);
    const auto normalizedCompanion = normalizeWhitespaceControls(companion);
    const bool removedWhitespaceControl = normalizedSource != source
        || normalizedCompanion != companion;
    if (normalizedSource == normalizedCompanion) {
        if (removedWhitespaceControl) return {false, {"whitespace"}, {}};
        return {true, {}, "Equivalent Enigma font-state serialization"};
    }
    if (className == "block_texts" && normalizedCompanion.empty()) {
        static const std::regex emptyPartNameTemplate(
            R"(^(?:\^(?:font|fontid|Font|fontMus|fontTxt|fontNum|size|nfx)\([^)]*\))*\^partname\(\)$)");
        if (std::regex_match(normalizedSource, emptyPartNameTemplate)) {
            return {false, {"empty part-name template"}, {}};
        }
    }
    const auto sourceChunks = tryEnigmaChunks(sourceDocument, normalizedSource, false);
    const auto companionChunks = tryEnigmaChunks(companionDocument, normalizedCompanion, false);
    if (!sourceChunks || !companionChunks) {
        std::set<std::string> differences{"unresolved font"};
        if (removedWhitespaceControl) differences.insert("whitespace");
        return {false, std::move(differences), {}};
    }
    if (chunksEqual(*sourceChunks, *companionChunks)) {
        if (removedWhitespaceControl) return {false, {"whitespace"}, {}};
        return {true, {}, "Equivalent Enigma font-state serialization"};
    }
    const auto sourceNoTime = tryEnigmaChunks(sourceDocument, normalizedSource, true);
    const auto companionNoTime = tryEnigmaChunks(companionDocument, normalizedCompanion, true);
    if (sourceNoTime && companionNoTime && chunksEqual(*sourceNoTime, *companionNoTime)) {
        if (removedWhitespaceControl) return {false, {"whitespace"}, {}};
        return {true, {}, "Finale-dropped ^time insert"};
    }
    if (partNameText && plainText(*sourceChunks) == plainText(*companionChunks)) {
        if (removedWhitespaceControl) return {false, {"whitespace"}, {}};
        return {true, {}, "Finale-reformatted part-name text"};
    }
    if (partNameText && plainText(*sourceChunks).empty()
            && plainText(*companionChunks) == "Score") {
        return {false, {"empty part-name template"}, {}};
    }
    if (differsOnlyByWhitespace(
            normalizedSource, normalizedCompanion, *sourceChunks, *companionChunks)) {
        return {false, {"whitespace"}, {}};
    }
    if (className == "file_info_texts"
            && plainText(*sourceChunks) == plainText(*companionChunks)) {
        std::set<std::string> differences{"added font info"};
        if (removedWhitespaceControl) differences.insert("whitespace");
        return {false, std::move(differences), {}};
    }
    static const std::regex explicitSymbolCharset(
        R"(\^(?:font|fontid|Font|fontMus|fontTxt|fontNum)\([^,)]*,(?:8191|8192)\))");
    if (wrongPlatformEncodingGlitch(*sourceChunks, *companionChunks)
            || symbolFontEncodingGlitch(*sourceChunks, *companionChunks,
            std::regex_search(normalizedCompanion, explicitSymbolCharset), symbolFontNames)) {
        std::set<std::string> differences{"known encoding glitch"};
        if (removedWhitespaceControl) differences.insert("whitespace");
        return {false, std::move(differences), {}};
    }
    if (utf16BytePairGlitch(*sourceChunks, *companionChunks)) {
        std::set<std::string> differences{"known encoding glitch"};
        if (removedWhitespaceControl) differences.insert("whitespace");
        return {false, std::move(differences), {}};
    }
    std::set<std::string> differences;
    if (removedWhitespaceControl) differences.insert("whitespace");
    if (plainText(*sourceChunks) != plainText(*companionChunks)) differences.insert("other");
    const auto common = (std::min)(sourceChunks->size(), companionChunks->size());
    for (std::size_t index = 0; index < common; ++index) {
        if (!sameFont((*sourceChunks)[index].font, (*companionChunks)[index].font)) differences.insert("font");
        if ((*sourceChunks)[index].font && (*companionChunks)[index].font) {
            if ((*sourceChunks)[index].font->fontSize != (*companionChunks)[index].font->fontSize) differences.insert("size");
            const auto left = (*sourceChunks)[index].font;
            const auto right = (*companionChunks)[index].font;
            if (std::tie(left->bold, left->italic, left->underline, left->strikeout,
                    left->absolute, left->hidden) != std::tie(right->bold, right->italic,
                    right->underline, right->strikeout, right->absolute, right->hidden)) {
                differences.insert("effects");
            }
        }
    }
    if (sourceChunks->size() != companionChunks->size() && differences.contains("other")) {
        differences.erase("other");
        differences.insert("missing run");
    }
    static const std::regex effectsCommand(R"(\^(?:nfx|efx)\([^)]*\))");
    if (std::regex_search(normalizedSource, effectsCommand)
            != std::regex_search(normalizedCompanion, effectsCommand)) {
        differences.insert("effects");
    }
    if (differences.empty()) differences.insert("other");
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

bool isDifferentDefault(const std::string& path, const std::string& category,
    const std::string& origin, const Value& sourceValue, const Value& companionValue,
    FormatEpoch epoch, const SourceVersion* sourceVersion)
{
    if (category != "differs" || origin != "finale27-default") return false;
    static const std::set<std::string_view> smartShapePaths{
        "smart_shape_options.cresc_horizontal",
        "smart_shape_options.cresc_line_width",
        "smart_shape_options.slur_avoid_staff_lines",
        "smart_shape_options.slur_left_break_horz_adj",
        "smart_shape_options.smart_line_width",
        "smart_shape_options.use_engraver_slurs"};
    if (smartShapePaths.contains(path)) return true;
    if (path == "chord_options.use_fretboard_font"
            && sourceValue.isBool() && !sourceValue.asBool()
            && companionValue.isBool() && companionValue.asBool()) {
        return true;
    }
    if (path == "chord_options.use_simple_chord_spelling"
            && sourceIsVersion(epoch, sourceVersion,
                FormatEpoch::UncompressedLegacy, versions::finale3_0)
            && sourceValue.isBool() && sourceValue.asBool()
            && companionValue.isBool() && !companionValue.asBool()) {
        return true;
    }
    if (startsWith(path, "smart_shape_options.")
            && path.find("_connect_styles[type=") != std::string::npos
            && (endsWith(path, ".x") || endsWith(path, ".y"))) {
        return true;
    }
    if (startsWith(path, "ss_line_styles[")
            && (endsWith(path, ".solid_width")
                || endsWith(path, ".char_font_size"))) {
        return true;
    }
    if (path == "repeat_options.bracket_height" && sourceValue.isInteger()
            && sourceValue.asInteger() == 96 && companionValue.isInteger()
            && std::set<std::int64_t>{72, 144}.contains(companionValue.asInteger())) {
        return true;
    }
    return false;
}

bool isBaselineFontCharsetNormalization(const std::string& path,
    const Value& sourceValue, const Value& companionValue)
{
    if (!sourceValue.isInteger() || !companionValue.isInteger()) return false;
    const auto values = std::pair{sourceValue.asInteger(), companionValue.asInteger()};
    if (endsWith(path, "].charset_bank")) {
        static const std::set<std::pair<std::int64_t, std::int64_t>> bankValues{
            {0, 1}, {1, 0}};
        return bankValues.contains(values);
    }
    if (endsWith(path, "].charset_value")) {
        static const std::set<std::pair<std::int64_t, std::int64_t>> charsetValues{
            {0, 2}, {0, 4095}, {1, 2}, {1, 4095}, {2, 4095}, {6, 4095}};
        return charsetValues.contains(values);
    }
    return false;
}

bool hasByteSwappedCStringSuffix(std::string_view source, std::string_view companion)
{
    std::size_t firstDifference = 0;
    while (firstDifference < source.size() && firstDifference < companion.size()
            && source[firstDifference] == companion[firstDifference]) {
        ++firstDifference;
    }
    std::string swapped(source.substr(firstDifference));
    swapped.push_back('\0');
    if (swapped.size() % 2 != 0) {
        // When the terminator is the first byte of its word, swapping that word exposes the
        // byte after it as the companion's one additional trailing byte.
        if (companion.size() != source.size() + 1) return false;
        swapped.push_back(companion.back());
    }
    for (std::size_t i = 0; i < swapped.size(); i += 2) {
        std::swap(swapped[i], swapped[i + 1]);
    }
    swapped.resize(swapped.find('\0'));
    return companion == std::string(source.substr(0, firstDifference)) + swapped;
}

bool isFretInstrumentStringByteSplit(const std::string& path,
    const std::string& category, const Leaves& source, const Leaves& companion)
{
    if (category != "differs") return false;
    static const std::regex stringMember(
        R"(^(fret_instruments\[cmper=\d+\]\.strings\[\d+\])\.(pitch|nut_offset)$)");
    std::smatch match;
    if (!std::regex_match(path, match, stringMember)) return false;
    const auto prefix = match[1].str();
    const auto sourcePitch = source.find(prefix + ".pitch");
    const auto sourceNutOffset = source.find(prefix + ".nut_offset");
    const auto companionPitch = companion.find(prefix + ".pitch");
    const auto companionNutOffset = companion.find(prefix + ".nut_offset");
    if (sourcePitch == source.end() || sourceNutOffset == source.end()
            || companionPitch == companion.end() || companionNutOffset == companion.end()
            || sourcePitch->second.second != "legacy-mus"
            || sourceNutOffset->second.second != "legacy-behavior"
            || !sourcePitch->second.first.isInteger()
            || !sourceNutOffset->second.first.isInteger()
            || !companionPitch->second.first.isInteger()
            || !companionNutOffset->second.first.isInteger()) {
        return false;
    }
    const auto oldPitch = sourcePitch->second.first.asInteger();
    const auto oldNutOffset = sourceNutOffset->second.first.asInteger();
    const auto splitPitch = companionPitch->second.first.asInteger();
    const auto splitNutOffset = companionNutOffset->second.first.asInteger();
    if (oldPitch < (std::numeric_limits<std::int16_t>::min)()
            || oldPitch > (std::numeric_limits<std::int16_t>::max)()
            || oldNutOffset != 0 || splitPitch < 0
            || splitPitch > (std::numeric_limits<std::uint8_t>::max)()
            || splitNutOffset < 0
            || splitNutOffset > (std::numeric_limits<std::uint8_t>::max)()) {
        return false;
    }
    const auto oldWord = static_cast<std::uint16_t>(oldPitch);
    const auto splitWord = static_cast<std::uint16_t>(splitPitch
        | (splitNutOffset << 8U));
    return oldWord == splitWord;
}

bool isFinaleUpgradeLoss(const std::string& path, const std::string& category,
    const std::string& origin, const Value& sourceValue, const Value& companionValue,
    const Leaves& source, const Leaves& companion, FormatEpoch epoch,
    ByteOrder byteOrder, const SourceVersion* sourceVersion)
{
    if (isFretInstrumentStringByteSplit(path, category, source, companion)) return true;
    if (category != "differs" || origin != "legacy-mus") return false;
    static const std::regex fretGroupInstrumentReference(
        R"(^fretboard_groups\[cmper=\d+,inci=\d+\]\.fret_inst_id$)");
    if (byteOrder == ByteOrder::BigEndian
            && std::regex_match(path, fretGroupInstrumentReference)
            && sourceValue.isInteger() && companionValue.isInteger()) {
        const auto sourceInstrument = sourceValue.asInteger();
        const auto companionInstrument = companionValue.asInteger();
        if (sourceInstrument >= 0 && sourceInstrument <= 0xffff
                && companionInstrument >= 0 && companionInstrument <= 0xffff) {
            const auto byteSwappedInstrument =
                ((sourceInstrument & 0x00ff) << 8) | ((sourceInstrument & 0xff00) >> 8);
            if (companionInstrument == byteSwappedInstrument) return true;
        }
    }
    static const std::regex fretGroupName(
        R"(^fretboard_groups\[cmper=\d+,inci=\d+\]\.name$)");
    if (byteOrder == ByteOrder::BigEndian && std::regex_match(path, fretGroupName)
            && sourceValue.isString() && companionValue.isString()
            && hasByteSwappedCStringSuffix(
                sourceValue.asString(), companionValue.asString())) {
        return true;
    }
    if (epoch == FormatEpoch::CodaBanner
            && std::set<std::string_view>{
                "smart_shape_options.slur_thickness_cp1_x",
                "smart_shape_options.slur_thickness_cp2_x",
                "smart_shape_options.slur_thickness_cp2_y"}.contains(path)) {
        return true;
    }
    return sourceIsVersion(epoch, sourceVersion,
        FormatEpoch::DclLegacy, versions::finale2003)
        && startsWith(path, "smart_shape_options.bend_curve_connect_styles[type=")
        && (endsWith(path, ".x") || endsWith(path, ".y"));
}

std::optional<std::string> expectedDifference(const std::string& path,
    const std::string& category, const std::string& origin, const Value& sourceValue,
    const Value& companionValue, const Leaves& source, const Leaves& companion,
    FormatEpoch epoch, ByteOrder byteOrder, const SourceVersion* sourceVersion)
{
    if (category == "companion_only" && startsWith(path, "stem_options.stem_connections[")) {
        return "stem-connection-past-terminator";
    }
    if (path == "stem_options.stem_connections[0].up_stem_horz" && category == "differs"
            && origin == "legacy-mus" && sourceValue.isInteger() && sourceValue.asInteger() == 0
            && companionValue.isInteger()
            && std::set<std::int64_t>{199, 221, 589, 6969}.contains(companionValue.asInteger())
            && equalSurrounding(source, companion,
                "stem_options.stem_connections[0].", path)) {
        return "stem-horizontal-correction";
    }
    if (path == "stem_options.stem_width" && category == "differs"
            && origin == "finale27-default" && epoch == FormatEpoch::CodaBanner) {
        return "coda-stem-width";
    }
    if (path == "stem_options.stem_offset" && category == "differs"
            && origin == "finale27-default" && epoch == FormatEpoch::CodaBanner
            && sourceValue.isInteger() && sourceValue.asInteger() == 256
            && companionValue.isInteger() && companionValue.asInteger() == 128) {
        return "coda-stem-offset";
    }
    if (isFinaleUpgradeLoss(path, category, origin, sourceValue, companionValue,
            source, companion, epoch, byteOrder, sourceVersion)) {
        return std::string(finaleUpgradeLossRule);
    }
    if (const auto omitted = omittedSlurConnectionStyleDifference(
            path, category, source, companion)) {
        return omitted;
    }
    if (isDifferentDefault(path, category, origin, sourceValue, companionValue,
            epoch, sourceVersion)) {
        return "different_defaults";
    }
    if (startsWith(path, "text_blocks[cmper=") && category == "differs"
            && (endsWith(path, ".new_pos_36") || endsWith(path, ".no_expand_single_word"))
            && origin == "legacy-behavior" && sourceValue.isBool() && companionValue.isBool()
            && !sourceValue.asBool() && companionValue.asBool()) {
        return "coda-text-block-upgrade";
    }
    if (startsWith(path, "text_blocks[cmper=") && category == "differs"
            && epoch == FormatEpoch::CodaBanner
            && (endsWith(path, ".shape_id") || endsWith(path, ".show_shape"))) {
        if (endsWith(path, ".shape_id") && sourceValue.isInteger() && companionValue.isInteger()
                && sourceValue.asInteger() == 0 && companionValue.asInteger() != 0) {
            return "coda-text-block-upgrade";
        }
        if (sourceValue.isBool() && companionValue.isBool()
                && !sourceValue.asBool() && companionValue.asBool()) return "coda-text-block-upgrade";
    }
    if (startsWith(path, "shape_defs[cmper=") && endsWith(path, ".shape_type")
            && category == "differs" && origin == "legacy-mus" && sourceValue.isInteger()
            && companionValue.isInteger() && sourceValue.asInteger() != 0
            && companionValue.asInteger() == 0) return "shape-reclassified-other";
    if (endsWith(path, "shape_id") && category == "differs" && origin == "finale27-default") {
        return "default-shape-id";
    }
    if (startsWith(path, "font_definitions.definitions[") && category == "differs"
            && (origin != "legacy-mus"
                || isBaselineFontCharsetNormalization(
                    path, sourceValue, companionValue))) return "baseline-font";
    if (startsWith(path, "lyric_options.") && category == "differs"
            && (endsWith(path, "use_smart_hyphens") || endsWith(path, "use_smart_word_extensions"))
            && origin == "legacy-behavior" && sourceValue.isBool() && companionValue.isBool()
            && !sourceValue.asBool() && companionValue.asBool()) return "smart-lyrics-enabled";
    if (category == "differs" && origin == "finale27-default"
            && ((path == "text_options.inserts[1].tracking_before"
                    && sourceValue.isInteger() && companionValue.isInteger()
                    && sourceValue.asInteger() == 60 && companionValue.asInteger() == 50)
                || (path == "text_options.inserts[2].tracking_before"
                    && sourceValue.isInteger() && companionValue.isInteger()
                    && sourceValue.asInteger() == 50 && companionValue.asInteger() == 0))) {
        return "missing-accidental-insert-default";
    }
    if (category == "differs" && startsWith(path, "text_options.inserts[")
            && integerLeaf(source, "text_options.inserts[0].tracking_before") == 35
            && integerLeaf(source, "text_options.inserts[1].tracking_before") == 50
            && integerLeaf(source, "text_options.inserts[2].tracking_before") == 0
            && integerLeaf(source, "text_options.inserts[3].tracking_before") == 40
            && integerLeaf(source, "text_options.inserts[4].tracking_before") == 60
            && integerLeaf(companion, "text_options.inserts[0].tracking_before") == 2293760
            && integerLeaf(companion, "text_options.inserts[1].tracking_before") == 587202560
            && integerLeaf(companion, "text_options.inserts[2].tracking_before") == 0
            && integerLeaf(companion, "text_options.inserts[3].tracking_before") == 1845493760
            && integerLeaf(companion, "text_options.inserts[4].tracking_before") == 3932160) {
        return "17-byte-accidental-insert";
    }
    if (path == "lyric_options.word_ext_connect_styles.oneEntryEnd.x"
            && category == "differs" && sourceValue.isInteger() && companionValue.isInteger()
            && sourceValue.asInteger() == 42 && companionValue.asInteger() == 44
            && (epoch == FormatEpoch::CodaBanner
                || sourcePredatesVersion(sourceVersion, versions::finale2004))
            && equalSurrounding(source, companion,
                "lyric_options.word_ext_connect_styles.", path)) {
        return "pre-connection-endpoint";
    }
    return std::nullopt;
}

void writeCounts(std::ostream& out, const std::map<std::string, std::uint64_t>& counts)
{
    out << '{';
    bool first = true;
    for (const auto& [name, count] : counts) {
        out << (first ? "" : ",") << jsonString(name) << ':' << count;
        first = false;
    }
    out << '}';
}

} // namespace

ComparisonResult compareSnapshots(SurveySnapshot source, SurveySnapshot companion,
    const musx::dom::DocumentPtr& sourceDocument,
    const musx::dom::DocumentPtr& companionDocument,
    FormatEpoch sourceEpoch, ByteOrder sourceByteOrder, const SourceVersion* sourceVersion,
    const text::SymbolFontNames* symbolFontNames)
{
    ComparisonResult result;
    const auto wrappers = realignShapes(source, companion);
    if (wrappers) result.transformations["Finale-added StartObject wrapper"] += wrappers;
    if (sourceEpoch == FormatEpoch::CodaBanner) {
        realignCodaBlockTexts(source, companion, sourceDocument, companionDocument, result);
    }
    const auto textBlockReferents = compareTextBlockReferents(
        sourceDocument, companionDocument, symbolFontNames);
    auto shapeSetFontPaths = shapeFontPaths(source);
    const auto companionShapeFontPaths = shapeFontPaths(companion);
    shapeSetFontPaths.insert(companionShapeFontPaths.begin(), companionShapeFontPaths.end());
    std::set<std::string> classes;
    for (const auto& [name, unused] : source) if (!excludedClasses.contains(name)) classes.insert(name);
    for (const auto& [name, unused] : companion) if (!excludedClasses.contains(name)) classes.insert(name);
    for (const auto& className : classes) {
        const auto sourceClass = source.find(className);
        const auto companionClass = companion.find(className);
        Leaves sourceLeaves;
        Leaves companionLeaves;
        if (sourceClass != source.end()) collectLeaves(sourceClass->second, className, {}, true, sourceLeaves);
        if (companionClass != companion.end()) collectLeaves(companionClass->second, className, {}, false, companionLeaves);
        std::set<std::string> paths;
        for (const auto& [path, unused] : sourceLeaves) paths.insert(path);
        for (const auto& [path, unused] : companionLeaves) paths.insert(path);
        auto& stats = classComparison(result, className);
        for (const auto& path : paths) {
            const auto sourceFound = sourceLeaves.find(path);
            const auto companionFound = companionLeaves.find(path);
            const bool inSource = sourceFound != sourceLeaves.end();
            const bool inCompanion = companionFound != companionLeaves.end();
            const bool fontReference = isFontReferencePath(path, shapeSetFontPaths);
            if (inSource && inCompanion && fontReference
                    && sourceFound->second.first.isInteger()
                    && companionFound->second.first.isInteger()) {
                const auto sourceName = fontIdentity(
                    source, sourceFound->second.first.asInteger());
                const auto companionName = fontIdentity(
                    companion, companionFound->second.first.asInteger());
                if (!sourceName.empty() && sameFontName(sourceName, companionName)) {
                    ++stats.same;
                    continue;
                }
                if (shapeSetFontPaths.contains(path)) {
                    ++stats.expected;
                    ++result.expected["setfont-font-substitution"];
                    ++result.fontSubstitutions[(sourceName.empty() ? "?" : sourceName) + '\t'
                        + (companionName.empty() ? "?" : companionName)];
                    continue;
                }
            }
            if (inSource && inCompanion && !fontReference
                    && sourceFound->second.first == companionFound->second.first) {
                ++stats.same;
                continue;
            }
            if (inSource && inCompanion && !fontReference
                    && equalChordScalingPercent(path, sourceFound->second.first,
                        companionFound->second.first)) {
                ++stats.same;
                continue;
            }
            if (inSource && inCompanion && endsWith(path, "font_name")
                    && sourceFound->second.first.isString()
                    && companionFound->second.first.isString()
                    && sameFontName(sourceFound->second.first.asString(),
                        companionFound->second.first.asString())) {
                ++stats.same;
                continue;
            }
            if (inSource && inCompanion && equalOmittedZeroInsertFont(path,
                    sourceFound->second.first, companionFound->second.first,
                    sourceLeaves, companionLeaves)) {
                ++stats.same;
                continue;
            }
            const auto prefix = objectPrefix(path);
            const auto referent = textBlockReferents.find(prefix);
            if (inSource && inCompanion && endsWith(path, ".text_id")
                    && referent != textBlockReferents.end()
                    && (referent->second == ReferentComparison::Matching
                        || referent->second == ReferentComparison::MatchingPageOnly)) {
                ++stats.same;
                ++result.transformations["Equivalent TextBlock raw-text referent"];
                continue;
            }
            if (inSource && inCompanion && surveyorPool(className) == "texts"
                    && endsWith(path, ".text")
                    && sourceFound->second.first.isString() && companionFound->second.first.isString()) {
                const auto comparison = compareText(className, sourceFound->second.first.asString(),
                    companionFound->second.first.asString(), sourceDocument, companionDocument,
                    symbolFontNames, isPartNameText(className, path, source, companion));
                if (comparison.equivalent && hasSynthesizedTextState(source, className, path)
                        && sourceFound->second.first != companionFound->second.first) {
                    ++stats.expected;
                    ++result.expected["enigma-text-difference"];
                    ++result.textDifferences[className]["added font info"];
                    if (result.textExamples.size() < maximumExamplesPerRow) {
                        result.textExamples.push_back({path, sourceFound->second.first,
                            companionFound->second.first, "added font info", {}});
                    }
                } else if (comparison.equivalent) {
                    ++stats.same;
                    if (!comparison.transformation.empty()) ++result.transformations[comparison.transformation];
                } else {
                    for (const auto& kind : comparison.differences) {
                        if (kind == "other") {
                            ++stats.unexpected;
                        } else {
                            ++stats.expected;
                            ++result.expected["enigma-text-difference"];
                        }
                        ++result.textDifferences[className][kind];
                        if (result.textExamples.size() < maximumExamplesPerRow) {
                            result.textExamples.push_back({path, sourceFound->second.first,
                                companionFound->second.first, kind, {}});
                        }
                    }
                }
                continue;
            }
            const std::string category = !inSource ? "companion_only"
                : !inCompanion ? "reader_only" : "differs";
            const Value sourceValue = inSource ? sourceFound->second.first : Value{};
            const Value companionValue = inCompanion ? companionFound->second.first : Value{};
            const auto origin = inSource ? sourceFound->second.second : std::string{};
            if (const auto expected = expectedDifference(path, category, origin, sourceValue,
                    companionValue, sourceLeaves, companionLeaves, sourceEpoch,
                    sourceByteOrder, sourceVersion)) {
                ++stats.expected;
                ++result.expected[*expected];
            } else if (category == "differs" && referent != textBlockReferents.end()
                    && referent->second == ReferentComparison::MatchingPageOnly
                    && endsWith(path, ".justify") && sourceValue.isInteger()
                    && companionValue.isInteger()
                    && std::set<std::int64_t>{sourceValue.asInteger(), companionValue.asInteger()}
                        == std::set<std::int64_t>{0, 2}) {
                ++stats.expected;
                ++result.expected["legacy-page-parity-text"];
            } else if (category == "differs" && referent != textBlockReferents.end()
                    && referent->second == ReferentComparison::Renumbered) {
                ++stats.expected;
                ++result.expected["finale-text-block-renumbering"];
            } else if (category == "differs" && className == "text_blocks" && !prefix.empty()) {
                const auto equals = prefix.find("cmper=");
                const auto close = prefix.find(']', equals);
                const auto cmper = equals == std::string::npos ? 0
                    : std::stoll(prefix.substr(equals + 6, close - equals - 6));
                if (cmper > 65000) {
                    ++stats.expected;
                    ++result.expected["transient-text-block"];
                    continue;
                }
                ++stats.unexpected;
                if (result.unexpectedExamples.size() < maximumExamplesPerRow) {
                    result.unexpectedExamples.push_back(
                        {path, sourceValue, companionValue, {}, origin});
                }
            } else if (category == "differs") {
                ++stats.unexpected;
                if (result.unexpectedExamples.size() < maximumExamplesPerRow) {
                    result.unexpectedExamples.push_back(
                        {path, sourceValue, companionValue, {}, origin});
                }
            } else if (category == "reader_only") {
                ++stats.sourceOnly;
            } else {
                ++stats.companionOnly;
            }
        }
    }
    return result;
}

void writeCompactComparison(std::ostream& out, const ComparisonResult& comparison)
{
    out << "\"classes\":{";
    bool firstPool = true;
    for (const auto& [pool, classes] : comparison.classes) {
        out << (firstPool ? "" : ",") << jsonString(pool) << ":{";
        bool firstClass = true;
        for (const auto& [name, counts] : classes) {
            out << (firstClass ? "" : ",") << jsonString(name) << ":[" << counts.same << ','
                << counts.expected << ',' << counts.unexpected << ',' << counts.sourceOnly << ','
                << counts.companionOnly << ']';
            firstClass = false;
        }
        out << '}';
        firstPool = false;
    }
    out << "},\"expected\":";
    writeCounts(out, comparison.expected);
    out << ",\"transformations\":";
    writeCounts(out, comparison.transformations);
    out << ",\"font_substitutions\":";
    writeCounts(out, comparison.fontSubstitutions);
    out << ",\"text\":{";
    bool first = true;
    for (const auto& [className, counts] : comparison.textDifferences) {
        out << (first ? "" : ",") << jsonString(className) << ':';
        writeCounts(out, counts);
        first = false;
    }
    out << '}';
    const auto writeExamples = [&](std::string_view key,
                                   const std::vector<DifferenceExample>& examples,
                                   bool writeOrigin) {
        out << ",\"" << key << "\":[";
        bool firstExample = true;
        for (const auto& example : examples) {
            out << (firstExample ? "" : ",") << '[' << jsonString(example.path) << ','
                << example.source.toJson() << ',' << example.companion.toJson();
            const auto& annotation = writeOrigin ? example.origin : example.kind;
            if (!annotation.empty()) out << ',' << jsonString(annotation);
            out << ']';
            firstExample = false;
        }
        out << ']';
    };
    writeExamples("unexpected", comparison.unexpectedExamples, true);
    writeExamples("text_examples", comparison.textExamples, false);
}

} // namespace coverage
} // namespace finale_mus_reader
