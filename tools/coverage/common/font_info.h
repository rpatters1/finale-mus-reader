// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <string_view>

#include "coverage/classification.h"
#include "import/support/text_encoding.h"

namespace finale_mus_reader {
namespace coverage {

/// @brief Classifier metadata populated through FontDefinition::calcIsSymbolFont().
inline constexpr std::string_view fontDefinitionIsSymbolField = "is_symbol";

inline bool isClassifierMetadataPath(std::string_view path)
{
    return path.size() > fontDefinitionIsSymbolField.size()
        && path[path.size() - fontDefinitionIsSymbolField.size() - 1] == '.'
        && path.ends_with(fontDefinitionIsSymbolField);
}

inline std::optional<DifferenceClassification>
classifySymbolFontEquivalence(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs) return std::nullopt;
    constexpr std::string_view bankField = "charset_bank";
    constexpr std::string_view valueField = "charset_val";
    const auto field = context.path.ends_with(bankField) ? bankField
        : context.path.ends_with(valueField) ? valueField : std::string_view{};
    if (field.empty()) return std::nullopt;
    auto symbolPath = std::string(context.path.substr(0, context.path.size() - field.size()));
    symbolPath.append(fontDefinitionIsSymbolField);
    const auto isSymbol = [&symbolPath](const ComparisonLeaves& leaves) {
        const auto found = leaves.find(symbolPath);
        return found != leaves.end() && found->second.first.isBool()
            && found->second.first.asBool();
    };
    return isSymbol(context.source) && isSymbol(context.companion)
        ? std::optional{DifferenceClassification::SymbolFontEquivalence}
        : std::nullopt;
}

inline std::optional<DifferenceClassification>
classifyFontDefinitionDifference(const DifferenceContext& context)
{
    if (const auto symbolFont = classifySymbolFontEquivalence(context)) return symbolFont;
    constexpr std::string_view charsetBankField = "charset_bank";
    constexpr std::string_view charsetValueField = "charset_val";
    if (context.category == DifferenceCategory::Differs
        && context.path.ends_with(charsetBankField)) {
        auto valuePath = std::string(
            context.path.substr(0, context.path.size() - charsetBankField.size()));
        valuePath.append(charsetValueField);
        const auto sourceCharset = comparisonIntegerLeaf(context.source, valuePath);
        const auto companionCharset = comparisonIntegerLeaf(context.companion, valuePath);
        if (sourceCharset == 0 && companionCharset == 0) {
            return DifferenceClassification::FontPlatformShift;
        }
    }
    if (context.category == DifferenceCategory::Differs &&
        context.origin == "finale27-default" && context.path.ends_with(charsetValueField) &&
        context.sourceValue.isInteger() && context.companionValue.isInteger()) {
        const auto sourceValue = context.sourceValue.asInteger();
        const auto companionValue = context.companionValue.asInteger();
        const bool equivalentValues =
            (sourceValue == text::windowsAnsiCharset &&
                companionValue == text::windowsDefaultCharset) ||
            (sourceValue == text::windowsDefaultCharset &&
                companionValue == text::windowsAnsiCharset);
        auto bankPath = std::string(
            context.path.substr(0, context.path.size() - charsetValueField.size()));
        bankPath.append("charset_bank");
        constexpr auto windowsBank = static_cast<std::int64_t>(
            musx::dom::others::FontDefinition::CharacterSetBank::Windows);
        const auto isWindowsBank = [&bankPath](const ComparisonLeaves& leaves) {
            const auto found = leaves.find(bankPath);
            return found != leaves.end() && found->second.first.isInteger() &&
                found->second.first.asInteger() == windowsBank;
        };
        if (equivalentValues && isWindowsBank(context.source) &&
            isWindowsBank(context.companion)) {
            return DifferenceClassification::CharsetEquivalence;
        }
    }
    if (context.category == DifferenceCategory::Differs &&
        context.origin == "finale27-default" && context.path.ends_with(".pitch")) {
        return DifferenceClassification::CharsetPitchDifference;
    }
    return std::nullopt;
}

std::string canonicalFontName(std::string_view value);
bool sameFontName(std::string_view left, std::string_view right);
std::set<std::string> comparisonFontReferencePaths(const SurveySnapshot& snapshot);
std::string comparisonFontIdentity(const SurveySnapshot& snapshot, std::int64_t id);
bool isComparisonFontReference(std::string_view path,
                               const std::set<std::string>& shapeFontPaths);

} // namespace coverage
} // namespace finale_mus_reader
