// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/classification_rules.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "coverage/common/text_controls.h"
#include "coverage/support/source_gate.h"
#include "import/support/text_encoding.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace coverage {
namespace {

std::optional<bool> fontDefinitionSymbolStatus(const ComparisonLeaves& leaves,
    std::string_view path, std::string_view field)
{
    auto symbolPath = std::string(path.substr(0, path.size() - field.size()));
    symbolPath.append(fontDefinitionIsSymbolField);
    const auto found = leaves.find(symbolPath);
    if (found == leaves.end() || !found->second.first.isBool()) return std::nullopt;
    return found->second.first.asBool();
}

std::optional<DifferenceClassification>
classifySymbolFontEquivalence(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs) return std::nullopt;
    constexpr std::string_view bankField = "charset_bank";
    constexpr std::string_view valueField = "charset_val";
    const auto field = context.path.ends_with(bankField) ? bankField
        : context.path.ends_with(valueField) ? valueField : std::string_view{};
    if (field.empty()) return std::nullopt;
    return fontDefinitionSymbolStatus(context.source, context.path, field) == true &&
            fontDefinitionSymbolStatus(context.companion, context.path, field) == true
        ? std::optional{DifferenceClassification::SymbolFontEquivalence}
        : std::nullopt;
}

std::optional<DifferenceClassification>
classifySymbolFontConversionLoss(const DifferenceContext& context)
{
    constexpr std::string_view valueField = "charset_val";
    if (context.category != DifferenceCategory::Differs ||
        context.origin != "legacy-mus-adjusted" || !context.path.ends_with(valueField)) {
        return std::nullopt;
    }
    return fontDefinitionSymbolStatus(context.source, context.path, valueField) == true &&
            fontDefinitionSymbolStatus(context.companion, context.path, valueField) == false
        ? std::optional{DifferenceClassification::FinaleUpgradeLoss}
        : std::nullopt;
}

} // namespace

bool isClassifierMetadataPath(std::string_view path)
{
    return path.size() > fontDefinitionIsSymbolField.size() &&
        path[path.size() - fontDefinitionIsSymbolField.size() - 1] == '.' &&
        path.ends_with(fontDefinitionIsSymbolField);
}

std::optional<DifferenceClassification>
classifyFontDefinitionDifference(const DifferenceContext& context)
{
    if (const auto symbolFont = classifySymbolFontEquivalence(context)) return symbolFont;
    if (const auto symbolFontLoss = classifySymbolFontConversionLoss(context)) {
        return symbolFontLoss;
    }
    constexpr std::string_view charsetBankField = "charset_bank";
    constexpr std::string_view charsetValueField = "charset_val";
    if (context.category == DifferenceCategory::Differs &&
        context.path.ends_with(charsetBankField)) {
        auto valuePath =
            std::string(context.path.substr(0, context.path.size() - charsetBankField.size()));
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
        auto bankPath =
            std::string(context.path.substr(0, context.path.size() - charsetValueField.size()));
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

std::optional<DifferenceClassification>
classifyKeySymbolListDifference(const DifferenceContext& context)
{
    constexpr std::string_view pathSuffix = "].accidental_string";
    const bool keySymbolListPath = context.path.starts_with(keySymbolListElementsCoverageKey) &&
        context.path.size() > keySymbolListElementsCoverageKey.size() &&
        context.path[keySymbolListElementsCoverageKey.size()] == '[';
    if (context.category == DifferenceCategory::ReaderOnly &&
        context.epoch == FormatEpoch::CodaBanner && keySymbolListPath) {
        const auto bracket = context.path.find(']');
        if (bracket != std::string_view::npos) {
            const auto objectPrefix = std::string(context.path.substr(0, bracket + 1));
            const auto cmper2 = comparisonIntegerLeaf(
                context.source, objectPrefix + ".cmper2");
            constexpr std::array<std::int64_t, 6> elementalSlots{
                1, 2, 4, 65532, 65534, 65535};
            const auto accidental = context.source.find(objectPrefix + ".accidental_string");
            const auto companionObject = context.companion.find(objectPrefix + ".cmper1");
            if (cmper2 && std::find(elementalSlots.begin(), elementalSlots.end(), *cmper2) !=
                    elementalSlots.end() &&
                accidental != context.source.end() && accidental->second.second == "legacy-mus" &&
                companionObject == context.companion.end()) {
                return DifferenceClassification::FinaleUpgradeLoss;
            }
        }
    }
    if (context.category == DifferenceCategory::Differs &&
        context.epoch == FormatEpoch::CodaBanner && context.origin == "legacy-mus" &&
        keySymbolListPath && context.path.ends_with(pathSuffix) &&
        context.sourceValue.isString() && context.companionValue.isString()) {
        auto source = std::string_view(context.sourceValue.asString());
        const auto storedSize = source.size();
        while (!source.empty() &&
            isFinaleWhitespaceControl(static_cast<unsigned char>(source.back()))) {
            source.remove_suffix(1);
        }
        if (source.size() != storedSize && source == context.companionValue.asString()) {
            return DifferenceClassification::WhitespaceControl;
        }
    }
    if (context.category != DifferenceCategory::Differs ||
        context.epoch == FormatEpoch::CodaBanner || context.origin != "legacy-mus" ||
        !keySymbolListPath || !context.path.ends_with(pathSuffix) ||
        !context.sourceValue.isString() || !context.companionValue.isString()) {
        return std::nullopt;
    }

    // Finale's upgrade is treated as corrupt only when its result differs solely by changing
    // every affected Petrucci double-sharp code from U+00DC to U+008B.
    constexpr std::string_view doubleSharp = "\xC3\x9C";
    constexpr std::string_view mojibake = "\xC2\x8B";
    auto upgraded = context.sourceValue.asString();
    bool replaced = false;
    std::size_t offset = 0;
    while ((offset = upgraded.find(doubleSharp, offset)) != std::string::npos) {
        upgraded.replace(offset, doubleSharp.size(), mojibake);
        offset += mojibake.size();
        replaced = true;
    }
    return replaced && upgraded == context.companionValue.asString()
        ? std::optional{DifferenceClassification::TextEncodingError}
        : std::nullopt;
}

std::optional<DifferenceClassification>
classifyMultimeasureRestOptionsDifference(const DifferenceContext& context)
{
    if (context.category == DifferenceCategory::Differs &&
        context.path == "mmrest_options.num_start" && context.origin == "legacy-mus" &&
        context.sourceValue.isInteger() && context.sourceValue.asInteger() == 0 &&
        context.companionValue.isInteger() && context.companionValue.asInteger() == 1) {
        return DifferenceClassification::FinaleUpgradeNormalization;
    }
    return std::nullopt;
}

std::optional<DifferenceClassification>
classifyDoubleWholeSlashConversionLoss(const DifferenceContext& context)
{
    constexpr std::int64_t legacyDoubleWholeSlash = 218;
    constexpr std::int64_t filledNoteheadSlash = 213;
    if (context.category == DifferenceCategory::Differs &&
        context.path == "music_symbol_options.dbl_whole_slash" &&
        context.origin == "finale27-default" && context.sourceValue.isInteger() &&
        context.sourceValue.asInteger() == legacyDoubleWholeSlash &&
        context.companionValue.isInteger() &&
        context.companionValue.asInteger() == filledNoteheadSlash) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    return std::nullopt;
}

std::optional<DifferenceClassification>
classifyVersionlessCodaSlashDefault(const DifferenceContext& context)
{
    const bool lacksHeaderVersion = !context.sourceVersion || context.sourceVersion->raw == 0;
    if (context.category != DifferenceCategory::Differs ||
        context.epoch != finale_mus_reader::FormatEpoch::CodaBanner) {
        return std::nullopt;
    }
    const bool sourceRecovered = context.path == "music_symbol_options.half_slash" ||
        context.path == "music_symbol_options.whole_slash";
    const bool retainedCodaDefault = context.origin == "finale27-default" &&
        (context.path == "music_symbol_options.quarter_slash" ||
            context.path == "music_symbol_options.slash_bar");
    const bool retainedHeaderlessDefault = lacksHeaderVersion &&
        context.origin == "finale27-default" &&
        context.path == "music_symbol_options.dbl_whole_slash";
    if ((lacksHeaderVersion && sourceRecovered) || retainedCodaDefault ||
        retainedHeaderlessDefault) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

std::optional<DifferenceClassification>
classifyNoteRestOptionsDifference(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs ||
        context.origin != "finale27-default") {
        return std::nullopt;
    }

    constexpr std::string_view prefix = "note_rest_options.";
    constexpr std::array restDropLeaves{
        noteRestDrop8thLeaf,
        noteRestDrop16thLeaf,
        noteRestDrop32ndLeaf,
        noteRestDrop64thLeaf,
        noteRestDrop128thLeaf,
    };
    for (const auto leaf : restDropLeaves) {
        if (context.path.size() == prefix.size() + leaf.size() &&
            context.path.starts_with(prefix) && context.path.ends_with(leaf)) {
            return DifferenceClassification::DifferentDefaults;
        }
    }
    return std::nullopt;
}

std::optional<DifferenceClassification>
classifyStemConnectionEncodingError(const DifferenceContext& context)
{
    constexpr std::string_view symbolSuffix = ".symbol";
    if (context.category != DifferenceCategory::Differs ||
        context.epoch != FormatEpoch::UncompressedLegacy || context.origin != "legacy-mus" ||
        context.sourceReport.sourcePlatform != SourcePlatform::MacOS ||
        !context.path.starts_with("stem_options.stem_connections[") ||
        !context.path.ends_with(symbolSuffix) || !context.sourceValue.isInteger() ||
        context.sourceValue.asInteger() != 0x00c0 || !context.companionValue.isInteger() ||
        context.companionValue.asInteger() != 0x00bf) {
        return std::nullopt;
    }
    const auto prefix =
        std::string(context.path.substr(0, context.path.size() - symbolSuffix.size()));
    const auto sourceFont = context.source.find(prefix + ".font_name");
    const auto companionFont = context.companion.find(prefix + ".font_name");
    return sourceFont != context.source.end() && companionFont != context.companion.end() &&
            sourceFont->second.first.isString() && companionFont->second.first.isString() &&
            sourceFont->second.first.asString() == "Times" &&
            companionFont->second.first.asString() == "Times"
        ? std::optional{DifferenceClassification::TextEncodingError}
        : std::nullopt;
}

std::optional<DifferenceClassification>
classifyPageFormatOptionsDifference(const DifferenceContext& context)
{
    if (context.category == DifferenceCategory::Differs &&
        context.path == "page_format_options.adjust_page_scope" &&
        context.origin == "finale27-default") {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

std::optional<DifferenceClassification>
classifyPartDefinitionDifference(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs) return std::nullopt;
    if (context.path.ends_with(".unlink_insts") && context.origin == "unmapped" &&
        context.sourceValue.isBool() && context.companionValue.isBool() &&
        !context.sourceValue.asBool() && context.companionValue.asBool()) {
        return DifferenceClassification::PossiblyUnrecoverable;
    }
    if (context.epoch == FormatEpoch::ZlibLegacy) return std::nullopt;
    if (!context.path.ends_with("[cmper=0].name_id")) return std::nullopt;
    if (context.origin != "legacy-behavior") return std::nullopt;
    if (!context.sourceValue.isInteger() || !context.companionValue.isInteger()) {
        return std::nullopt;
    }
    if (context.sourceValue.asInteger() != 0 || context.companionValue.asInteger() == 0) {
        return std::nullopt;
    }
    return DifferenceClassification::SynthesizedScoreName;
}

std::optional<DifferenceClassification>
classifyTieOptionsDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    const auto isThickness = context.path == "tie_options.thickness_left" ||
        context.path == "tie_options.thickness_right";
    if (context.category == Differs && context.epoch == FormatEpoch::CodaBanner && isThickness &&
        context.origin.starts_with("legacy-mus")) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    if (context.category == Differs && context.path == "tie_options.mixed_stem_direction" &&
        context.origin.starts_with("legacy-mus") && context.sourceValue.isInteger() &&
        context.sourceValue.asInteger() == 2 && context.companionValue.isInteger() &&
        context.companionValue.asInteger() == 0 &&
        sourceIsVersion(context.epoch, context.sourceVersion, FormatEpoch::DclLegacy,
            versions::finale2006)) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }

    const auto* scatteredLayout =
        context.sourceReport.findField<musx::dom::options::TieOptions>("breakForTimeSigs");
    const auto preSelector84Uncompressed =
        context.epoch == FormatEpoch::UncompressedLegacy && scatteredLayout &&
        scatteredLayout->origin == ValueOrigin::LegacyBehavior;
    if (context.category == Differs && context.origin == "finale27-default" &&
        (context.epoch == FormatEpoch::CodaBanner || preSelector84Uncompressed)) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

} // namespace coverage
} // namespace finale_mus_reader
