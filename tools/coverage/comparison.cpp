// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/comparison.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string_view>
#include <tuple>
#include <unordered_set>

#include "coverage/classification_rules.h"
#include "coverage/common/font_info.h"
#include "coverage/comparison_text.h"
#include "coverage/identity.h"
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

using Leaves = ComparisonLeaves;
constexpr std::size_t maximumExamplesPerRow = 20;

const std::unordered_set<std::string> metadataKeys = {
    "corpus_id", "status",        "epoch",       "saving_product", "source_version",
    "header",    "warning_count", "diagnostics", "duration_ms",    "timings",
    "companion", "finder_type",   "error"};

const std::unordered_set<std::string> excludedClasses = {"header", "relationships"};

bool startsWith(std::string_view value, std::string_view prefix)
{
    return value.substr(0, prefix.size()) == prefix;
}

bool endsWith(std::string_view value, std::string_view suffix)
{
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

std::string snakeToCamel(std::string_view value)
{
    std::string result;
    bool uppercase = false;
    for (const char character : value) {
        if (character == '_') {
            uppercase = true;
        } else if (uppercase) {
            result.push_back(
                static_cast<char>(std::toupper(static_cast<unsigned char>(character))));
            uppercase = false;
        } else {
            result.push_back(character);
        }
    }
    return result;
}

bool isNoncontentKey(std::string_view key)
{
    static const std::unordered_set<std::string> exact = {"origin",
                                                          "index",
                                                          "cmper",
                                                          "part_id",
                                                          "_report_match_key",
                                                          "instruction_count",
                                                          "value_count",
                                                          "external_graphic_count",
                                                          "undocumented_instruction_count",
                                                          "effects_synthesized",
                                                          "font_synthesized",
                                                          "size_synthesized"};
    return exact.contains(std::string(key)) || startsWith(key, "origin_") ||
           endsWith(key, "_origin") || endsWith(key, "_block_offset") ||
           endsWith(key, "_decoded_field_offset");
}

bool isExcludedPath(std::string_view path)
{
    static const std::vector<std::string> exact = {
        "font_options.tuples",
        "font_options.recovered_count",
        "font_options.legacy_behavior_count",
        "font_options.default_count",
        "font_options.unmapped_count",
        "font_options.musx_only_count",
        "font_definitions.duplicate_nonzero_name_count",
        "font_definitions.introduced_duplicate_nonzero_name_count",
        "shape_instruction_lists.instruction_types"};
    for (const auto& item : exact) {
        if (path == item || (startsWith(path, item) && path.size() > item.size() &&
                             (path[item.size()] == '.' || path[item.size()] == '[')))
            return true;
    }
    if (startsWith(path, "font_definitions.definitions[") && endsWith(path, ".name")) return true;
    if (startsWith(path, "shape_defs[") &&
        (endsWith(path, ".instruction_list") || endsWith(path, ".data_list")))
        return true;
    return false;
}

std::string listPartPrefix(const Value& item)
{
    const auto* partId = item.find("part_id");
    return partId && partId->isInteger() ? partIdentityPrefix(partId->asInteger())
                                         : std::string{};
}

std::optional<std::pair<std::string, std::string>> ordinaryListKey(const Value& item)
{
    if (!item.isObject()) return std::nullopt;
    const auto integer = [&](std::string_view key) -> const Value* {
        const auto* value = item.find(key);
        return value && value->isInteger() ? value : nullptr;
    };
    const auto partPrefix = listPartPrefix(item);
    if (integer("cmper1") && integer("cmper2") && integer("inci")) {
        return std::pair{"identity",
                         partPrefix + "cmper1=" +
                             std::to_string(integer("cmper1")->asInteger()) +
                             ",cmper2=" + std::to_string(integer("cmper2")->asInteger()) +
                             ",inci=" + std::to_string(integer("inci")->asInteger())};
    }
    if (integer("cmper") && integer("inci")) {
        return std::pair{"identity", partPrefix + "cmper=" +
                                         std::to_string(integer("cmper")->asInteger()) +
                                         ",inci=" +
                                         std::to_string(integer("inci")->asInteger())};
    }
    for (const auto key : {"cmper", "number", "index"}) {
        if (const auto* value = integer(key)) {
            if (partPrefix.empty()) {
                return std::pair{std::string(key), std::to_string(value->asInteger())};
            }
            return std::pair{"identity",
                partPrefix + key + "=" + std::to_string(value->asInteger())};
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, std::string>> listKey(std::string_view path, const Value& item)
{
    if (startsWith(path, "smart_shape_options.") && endsWith(path, "_connect_styles") &&
        item.isObject()) {
        if (const auto* type = item.find("type"); type && type->isInteger()) {
            return std::pair{"type", std::to_string(type->asInteger())};
        }
    }
    if (path == "font_definitions.definitions" && item.isObject()) {
        const auto partPrefix = listPartPrefix(item);
        if (const auto* cmper = item.find("cmper");
            cmper && cmper->isInteger() && cmper->asInteger() == 0) {
            if (partPrefix.empty()) return std::pair{"cmper", "0"};
            return std::pair{"identity", partPrefix + "cmper=0"};
        }
        if (const auto* name = item.find("normalized_name"); name && name->isString()) {
            const auto normalized = canonicalFontName(name->asString());
            if (partPrefix.empty()) return std::pair{"normalized_name", normalized};
            return std::pair{"identity", partPrefix + "normalized_name=" + normalized};
        }
    }
    if (surveyorPool(surveyorClass(path)) == "texts" && item.isObject()) {
        if (const auto* reportKey = item.find("_report_match_key");
            reportKey && reportKey->isString()) {
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
        if (field == "index" || field == "identity")
            result.push_back("[" + key + suffix + "]");
        else
            result.push_back("[" + field + "=" + key + suffix + "]");
    }
    return result;
}

void collectLeaves(const Value& value, std::string path, std::string origin, bool includeOrigins,
                   bool partObject, Leaves& result, std::set<std::string>* partLeaves)
{
    if (value.isObject()) {
        if (const auto* partId = value.find("part_id"); partId && partId->isInteger()) {
            partObject = partId->asInteger() != musx::dom::SCORE_PARTID;
        }
        if (includeOrigins) {
            if (const auto* objectOrigin = value.find("origin");
                objectOrigin && objectOrigin->isString())
                origin = objectOrigin->asString();
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
            collectLeaves(child, childPath, childOrigin, includeOrigins, partObject, result,
                          partLeaves);
        }
    } else if (value.isArray()) {
        const auto segments = listSegments(value.asArray(), path);
        for (std::size_t index = 0; index < value.asArray().size(); ++index) {
            collectLeaves(value.asArray()[index], path + segments[index], origin, includeOrigins,
                          partObject, result, partLeaves);
        }
    } else {
        if (partObject && partLeaves) partLeaves->insert(path);
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

bool equalSurrounding(const Leaves& source, const Leaves& companion, std::string_view prefix,
                      std::string_view excluded)
{
    std::set<std::string> paths;
    for (const auto& [path, unused] : source)
        if (startsWith(path, prefix) && path != excluded) paths.insert(path);
    for (const auto& [path, unused] : companion)
        if (startsWith(path, prefix) && path != excluded) paths.insert(path);
    for (const auto& path : paths) {
        const auto sourceFound = source.find(path);
        const auto companionFound = companion.find(path);
        if (sourceFound == source.end() || companionFound == companion.end() ||
            sourceFound->second.first != companionFound->second.first)
            return false;
    }
    return true;
}

} // namespace

bool comparisonPathStartsWith(std::string_view path, std::string_view prefix)
{
    return startsWith(path, prefix);
}

bool comparisonPathEndsWith(std::string_view path, std::string_view suffix)
{
    return endsWith(path, suffix);
}

bool comparisonEqualSurrounding(const ComparisonLeaves& source, const ComparisonLeaves& companion,
                                std::string_view prefix, std::string_view excluded)
{
    return equalSurrounding(source, companion, prefix, excluded);
}

ComparisonResult compareSnapshots(SurveySnapshot source, SurveySnapshot companion,
                                  const musx::dom::DocumentPtr& sourceDocument,
                                  const musx::dom::DocumentPtr& companionDocument,
                                  FormatEpoch sourceEpoch, ByteOrder sourceByteOrder,
                                  const SourceVersion* sourceVersion,
                                  const ImportReport& sourceReport)
{
    ComparisonResult result;
    ComparisonPreparationContext preparation{source, companion, result.transformations};
    runComparisonPreparers(preparation);
    if (sourceEpoch == FormatEpoch::CodaBanner) {
        comparison_text::realignCodaBlockTexts(source, companion, sourceDocument,
                                               companionDocument, result);
    }
    const auto textBlockReferents =
        comparison_text::compareTextBlockReferents(sourceDocument, companionDocument);
    auto shapeSetFontPaths = comparisonFontReferencePaths(source);
    const auto companionShapeFontPaths = comparisonFontReferencePaths(companion);
    shapeSetFontPaths.insert(companionShapeFontPaths.begin(), companionShapeFontPaths.end());
    std::set<std::string> classes;
    for (const auto& [name, unused] : source)
        if (!excludedClasses.contains(name)) classes.insert(name);
    for (const auto& [name, unused] : companion)
        if (!excludedClasses.contains(name)) classes.insert(name);
    for (const auto& className : classes) {
        const auto sourceClass = source.find(className);
        const auto companionClass = companion.find(className);
        Leaves sourceLeaves;
        Leaves companionLeaves;
        std::set<std::string> sourcePartLeaves;
        if (sourceClass != source.end())
            collectLeaves(sourceClass->second, className, {}, true, false, sourceLeaves,
                          &sourcePartLeaves);
        if (companionClass != companion.end())
            collectLeaves(companionClass->second, className, {}, false, false, companionLeaves,
                          nullptr);
        std::set<std::string> paths;
        for (const auto& [path, unused] : sourceLeaves)
            if (!isClassifierMetadataPath(path)) paths.insert(path);
        for (const auto& [path, unused] : companionLeaves)
            if (!isClassifierMetadataPath(path)) paths.insert(path);
        auto& stats = classComparison(result, className);
        for (const auto& path : paths) {
            const auto sourceFound = sourceLeaves.find(path);
            const auto companionFound = companionLeaves.find(path);
            const bool inSource = sourceFound != sourceLeaves.end();
            const bool inCompanion = companionFound != companionLeaves.end();
            const bool fontReference = isComparisonFontReference(path, shapeSetFontPaths);
            if (inSource && inCompanion && fontReference && sourceFound->second.first.isInteger() &&
                companionFound->second.first.isInteger()) {
                const auto sourceName =
                    comparisonFontIdentity(source, sourceFound->second.first.asInteger());
                const auto companionName =
                    comparisonFontIdentity(companion, companionFound->second.first.asInteger());
                if (!sourceName.empty() && sameFontName(sourceName, companionName)) {
                    ++stats.same;
                    continue;
                }
                if (shapeSetFontPaths.contains(path)) {
                    ++stats.expected;
                    ++result.expected[DifferenceClassification::SetFontSubstitution];
                    ++result.fontSubstitutions[(sourceName.empty() ? "?" : sourceName) + '\t' +
                                               (companionName.empty() ? "?" : companionName)];
                    continue;
                }
            }
            if (inSource && inCompanion && !fontReference &&
                sourceFound->second.first == companionFound->second.first) {
                ++stats.same;
                continue;
            }
            if (inSource && inCompanion && endsWith(path, "font_name") &&
                sourceFound->second.first.isString() && companionFound->second.first.isString() &&
                sameFontName(sourceFound->second.first.asString(),
                             companionFound->second.first.asString())) {
                ++stats.same;
                continue;
            }
            const auto prefix = objectPrefix(path);
            const auto referent = textBlockReferents.find(prefix);
            if (inSource && inCompanion && endsWith(path, ".text_id") &&
                referent != textBlockReferents.end() &&
                (referent->second == comparison_text::ReferentComparison::Matching ||
                 referent->second == comparison_text::ReferentComparison::MatchingPageOnly)) {
                ++stats.same;
                ++result.transformations[ComparisonTransformation::EquivalentTextBlockReferent];
                continue;
            }
            if (inSource && inCompanion && surveyorPool(className) == "texts" &&
                endsWith(path, ".text") && sourceFound->second.first.isString() &&
                companionFound->second.first.isString()) {
                const auto comparison = comparison_text::compareText(
                    className, path, sourceFound->second.first.asString(),
                    companionFound->second.first.asString(), sourceDocument, companionDocument,
                    comparison_text::isPartNameText(className, path, source, companion),
                    comparison_text::isSynthesizedScoreNameText(className, path, source,
                                                                companion));
                if (comparison.equivalent &&
                    comparison_text::hasSynthesizedTextState(source, className, path) &&
                    sourceFound->second.first != companionFound->second.first) {
                    ++stats.expected;
                    ++result.expected[DifferenceClassification::EnigmaTextDifference];
                    ++result
                          .textDifferences[className][TextDifferenceClassification::AddedFontInfo];
                    if (result.textExamples.size() < maximumExamplesPerRow) {
                        result.textExamples.push_back(
                            {path,
                             sourceFound->second.first,
                             companionFound->second.first,
                             DifferenceClassification::EnigmaTextDifference,
                             TextDifferenceClassification::AddedFontInfo,
                             {}});
                    }
                } else if (comparison.equivalent) {
                    ++stats.same;
                    if (comparison.transformation) {
                        ++result.transformations[*comparison.transformation];
                    }
                } else {
                    for (const auto& kind : comparison.differences) {
                        if (kind == TextDifferenceClassification::Other) {
                            ++stats.unexpected;
                        } else {
                            ++stats.expected;
                            ++result.expected[DifferenceClassification::EnigmaTextDifference];
                        }
                        ++result.textDifferences[className][kind];
                        if (result.textExamples.size() < maximumExamplesPerRow) {
                            result.textExamples.push_back(
                                {path,
                                 sourceFound->second.first,
                                 companionFound->second.first,
                                 kind == TextDifferenceClassification::Other
                                     ? DifferenceClassification::Unexpected
                                     : DifferenceClassification::EnigmaTextDifference,
                                 kind,
                                 {}});
                        }
                    }
                }
                continue;
            }
            const auto category = !inSource      ? DifferenceCategory::CompanionOnly
                                  : !inCompanion ? DifferenceCategory::ReaderOnly
                                                 : DifferenceCategory::Differs;
            const Value sourceValue = inSource ? sourceFound->second.first : Value{};
            const Value companionValue = inCompanion ? companionFound->second.first : Value{};
            const auto origin = inSource ? sourceFound->second.second : std::string{};
            const auto relatedDifference =
                referent == textBlockReferents.end()
                    ? RelatedDifference::None
                    : referent->second == comparison_text::ReferentComparison::Matching
                          ? RelatedDifference::MatchingTextBlockReferent
                      : referent->second == comparison_text::ReferentComparison::MatchingPageOnly
                          ? RelatedDifference::MatchingPageOnlyTextBlockReferent
                          : RelatedDifference::RenumberedTextBlockReferent;
            const DifferenceContext differenceContext{
                path,         category,        origin,      sourceValue,     companionValue,
                sourceLeaves, companionLeaves, sourceEpoch, sourceByteOrder, sourceVersion,
                sourceReport, relatedDifference};
            const auto equivalence = differenceEquivalence(className);
            if (equivalence && equivalence(differenceContext)) {
                ++stats.same;
                continue;
            }
            const auto classifier = differenceClassifier(className);
            const auto classExpected = classifier ? classifier(differenceContext) : std::nullopt;
            if (classExpected) {
                ++stats.expected;
                ++result.expected[*classExpected];
            } else if (category == DifferenceCategory::Differs) {
                ++stats.unexpected;
                if (result.unexpectedExamples.size() < maximumExamplesPerRow) {
                    result.unexpectedExamples.push_back({path,
                                                         sourceValue,
                                                         companionValue,
                                                         DifferenceClassification::Unexpected,
                                                         {},
                                                         origin});
                }
            } else if (category == DifferenceCategory::ReaderOnly) {
                if (sourcePartLeaves.contains(path)) {
                    ++stats.sourceOnlyPart;
                } else {
                    ++stats.sourceOnly;
                }
            } else {
                ++stats.companionOnly;
            }
        }
    }
    return result;
}

} // namespace coverage
} // namespace finale_mus_reader
