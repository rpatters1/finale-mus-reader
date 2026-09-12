// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/comparison_text.h"
#include "coverage/registry.h"
#include "coverage/surveyors/others/staff_fields.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;
using StaffStyleSurveyTarget = musx::dom::others::StaffStyle;

constexpr std::string_view staffStyleComparisonKey = "staff_style";
constexpr std::string_view notationStyleMaskSuffix = ".masks.notation_style";
constexpr std::string_view defaultClefMaskSuffix = ".masks.default_clef";
constexpr std::string_view showNoteColorsMaskSuffix = ".masks.show_note_colors";
constexpr std::string_view hideKeySigsShowAccisMaskSuffix = ".masks.hide_key_sigs_show_accis";
constexpr std::string_view staffTypeMaskSuffix = ".masks.staff_type";
constexpr std::string_view transpositionMaskSuffix = ".masks.transposition";
constexpr std::string_view fullNameMaskSuffix = ".masks.full_name";
constexpr std::string_view abrvNameMaskSuffix = ".masks.abrv_name";
constexpr std::string_view staffStyleAggregateHideSuffixes[] = {".hide_chords", ".hide_fretboards"};

struct StaffStyleInstrumentMask
{
    std::string_view maskSuffix;
    std::string_view valueSuffix;
};

constexpr StaffStyleInstrumentMask staffStyleInstrumentOnlyMasks[] = {
    {notationStyleMaskSuffix, ".notation_style"},
    {defaultClefMaskSuffix, ".default_clef"},
    {showNoteColorsMaskSuffix, ".show_note_colors"},
    {hideKeySigsShowAccisMaskSuffix, ".hide_key_sigs_show_accis"}};

constexpr std::string_view staffStyleInstrumentRequiredSharedMasks[] = {
    staffTypeMaskSuffix, transpositionMaskSuffix, fullNameMaskSuffix, abrvNameMaskSuffix};

bool staffStyleIsInstrumentMask(std::string_view suffix)
{
    return std::ranges::any_of(staffStyleInstrumentOnlyMasks,
                               [&](const auto& field) { return suffix == field.maskSuffix; }) ||
           std::ranges::find(staffStyleInstrumentRequiredSharedMasks, suffix) !=
               std::ranges::end(staffStyleInstrumentRequiredSharedMasks);
}

struct StaffStyleValueMask
{
    std::string_view valueSuffix;
    std::string_view maskSuffix;
};

constexpr StaffStyleValueMask staffStyleValueMasks[] = {
    {".use_note_shapes", ".masks.use_note_shapes"},
    {".flat_beams", ".masks.flat_beams"},
    {".blank_measure", ".masks.blank_measure_rest"},
    {".no_optimize", ".masks.no_optimize"},
    {".default_clef", defaultClefMaskSuffix},
    {".bline_break", ".masks.bline_break"},
    {".rbar_break", ".masks.rbar_break"},
    {".hide_meas_nums", ".masks.neg_mnumb"},
    {".hide_repeats", ".masks.neg_repeat"},
    {".hide_name_in_score", ".masks.neg_name_score"},
    {".hide_barlines", ".masks.hide_barlines"},
    {".full_name_text_id", fullNameMaskSuffix},
    {".abbrv_name_text_id", abrvNameMaskSuffix},
    {".float_keys", ".masks.float_keys"},
    {".float_time", ".masks.float_time"},
    {".hide_rpt_bars", ".masks.hide_rpt_bars"},
    {".hide_key_sigs", ".masks.neg_key"},
    {".hide_time_sigs", ".masks.neg_time"},
    {".hide_clefs", ".masks.neg_clef"},
    {".hide_mode", ".masks.hide_staff"},
    {".no_key", ".masks.no_key"},
    {".hide_ties", ".masks.show_ties"},
    {".hide_dots", ".masks.show_dots"},
    {".hide_rests", ".masks.show_rests"},
    {".hide_chords", ".masks.hide_chords"},
    {".hide_fretboards", ".masks.hide_fretboards"},
    {".hide_lyrics", ".masks.hide_lyrics"},
    {".show_name_in_parts", ".masks.show_name_parts"},
    {".show_note_colors", showNoteColorsMaskSuffix},
    {".hide_staff_lines", ".masks.hide_staff_lines"},
    {".redisplay_layer_accis", ".masks.redisplay_layer_accis"},
    {".hide_time_sigs_in_parts", ".masks.neg_time_parts"},
    {".hide_key_sigs_show_accis", hideKeySigsShowAccisMaskSuffix},
};

std::optional<std::string_view> staffStyleMaskForValue(std::string_view relativePath)
{
    if (relativePath == ".use_note_font" || comparisonPathStartsWith(relativePath, ".note_font.")) {
        return ".masks.float_notehead_font";
    }
    if (relativePath == ".notation_style" || relativePath == ".capo_pos" ||
        relativePath == ".lowest_fret" || relativePath == ".vert_tab_num_off" ||
        relativePath == ".show_tab_clef_all_sys" || relativePath == ".use_tab_letters" ||
        relativePath == ".break_tab_lines_at_notes" || relativePath == ".hide_tuplets" ||
        relativePath == ".fret_inst_id") {
        return notationStyleMaskSuffix;
    }
    if (relativePath == ".staff_lines" || relativePath == ".custom_staff" ||
        comparisonPathStartsWith(relativePath, ".custom_staff[") || relativePath == ".line_space" ||
        relativePath == ".top_barline_offset" || relativePath == ".bot_barline_offset" ||
        relativePath == ".dw_rest_offset" || relativePath == ".w_rest_offset" ||
        relativePath == ".h_rest_offset" || relativePath == ".other_rest_offset" ||
        relativePath == ".bot_repeat_dot_off" || relativePath == ".top_repeat_dot_off" ||
        relativePath == ".stem_reversal" || relativePath == ".hide_repeat_bottom_dot" ||
        relativePath == ".hide_repeat_top_dot") {
        return staffTypeMaskSuffix;
    }
    if (relativePath == ".transposed_clef" ||
        comparisonPathStartsWith(relativePath, ".transposition.")) {
        return transpositionMaskSuffix;
    }
    if (relativePath == ".alt_notation" || relativePath == ".alt_layer" ||
        comparisonPathStartsWith(relativePath, ".alt_hide_") ||
        relativePath == ".alt_rhythm_stems_up" || relativePath == ".alt_slash_dots") {
        return ".masks.alt_notation";
    }
    if (relativePath == ".hide_stems" || relativePath == ".stem_direction" ||
        relativePath == ".hide_beams" || relativePath == ".stem_start_from_staff" ||
        relativePath == ".stems_fixed_end" || relativePath == ".stems_fixed_start" ||
        relativePath == ".horz_stem_off_up" || relativePath == ".horz_stem_off_down" ||
        relativePath == ".vert_stem_start_off_up" || relativePath == ".vert_stem_start_off_down" ||
        relativePath == ".vert_stem_end_off_up" || relativePath == ".vert_stem_end_off_down") {
        return ".masks.show_stems";
    }
    for (const auto& mapping : staffStyleValueMasks) {
        if (relativePath == mapping.valueSuffix)
            return mapping.maskSuffix;
    }
    return std::nullopt;
}

std::string_view staffStyleObjectPrefix(std::string_view path)
{
    if (!comparisonPathStartsWith(path, "staff_style["))
        return {};
    const auto close = path.find(']');
    return close == std::string_view::npos ? std::string_view{} : path.substr(0, close + 1);
}

const Value* staffStyleLeaf(const ComparisonLeaves& leaves, std::string_view path)
{
    const auto found = leaves.find(std::string(path));
    return found == leaves.end() ? nullptr : &found->second.first;
}

bool staffStyleBoolLeaf(const ComparisonLeaves& leaves, const std::string& path, bool expected)
{
    const auto* value = staffStyleLeaf(leaves, path);
    return value && value->isBool() && value->asBool() == expected;
}

std::optional<DifferenceClassification>
classifyMaskedOffStaffStyleValue(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs)
        return std::nullopt;
    const auto objectPath = staffStyleObjectPrefix(context.path);
    if (objectPath.empty())
        return std::nullopt;
    const auto maskSuffix = staffStyleMaskForValue(context.path.substr(objectPath.size()));
    if (!maskSuffix)
        return std::nullopt;
    const auto maskPath = std::string(objectPath) + std::string(*maskSuffix);
    return staffStyleBoolLeaf(context.source, maskPath, false) &&
                   staffStyleBoolLeaf(context.companion, maskPath, false)
               ? std::optional{DifferenceClassification::DifferentDefaults}
               : std::nullopt;
}

bool staffStyleUsesAggregateOtherAttachedItems(const finale_mus_reader::ImportReport& report,
                                               musx::dom::Cmper cmper)
{
    const auto instance =
        finale_mus_reader::instanceKey<StaffStyleSurveyTarget>(musx::dom::SCORE_PARTID, cmper);
    const auto fields = report.fields.find(instance);
    if (fields == report.fields.end())
        return false;
    const auto articulations = fields->second.find("altHideOtherArtics");
    const auto smartShapes = fields->second.find("altHideOtherSmartShapes");
    return articulations != fields->second.end() && smartShapes != fields->second.end() &&
           articulations->second.blockOffset == smartShapes->second.blockOffset &&
           articulations->second.decodedOffset == smartShapes->second.decodedOffset &&
           articulations->second.sourceIdentity == smartShapes->second.sourceIdentity;
}

bool staffStyleUsesAggregateOtherAttachedItems(const DifferenceContext& context)
{
    const auto cmper = staff_fields::staffLikeCmperFromComparisonPath(context.path);
    return cmper && staffStyleUsesAggregateOtherAttachedItems(context.sourceReport, *cmper);
}

std::optional<DifferenceClassification>
classifyTwoBarRepeatAggregateUpgradeLoss(const DifferenceContext& context)
{
    const auto artics = comparisonPathEndsWith(context.path, ".alt_hide_artics");
    const auto lyrics = comparisonPathEndsWith(context.path, ".alt_hide_lyrics");
    const auto chords = comparisonPathEndsWith(context.path, ".hide_chords");
    const auto fretboards = comparisonPathEndsWith(context.path, ".hide_fretboards");
    if (context.category != DifferenceCategory::Differs || context.origin != "legacy-mus" ||
        !comparisonPathStartsWith(context.path, "staff_style[") ||
        (!artics && !lyrics && !chords && !fretboards) || !context.sourceValue.isBool() ||
        context.sourceValue.asBool() || !context.companionValue.isBool() ||
        !context.companionValue.asBool() || !staffStyleUsesAggregateOtherAttachedItems(context)) {
        return std::nullopt;
    }
    const auto objectPath = staffStyleObjectPrefix(context.path);
    const auto* notation =
        staffStyleLeaf(context.source, std::string(objectPath) + ".alt_notation");
    if (!notation || !notation->isInteger() ||
        notation->asInteger() !=
            static_cast<std::int64_t>(StaffStyleSurveyTarget::AlternateNotation::TwoBarRepeat)) {
        return std::nullopt;
    }
    if ((chords || fretboards) &&
        !staffStyleBoolLeaf(context.source, std::string(objectPath) + ".alt_hide_other_artics",
                            true)) {
        return std::nullopt;
    }
    return DifferenceClassification::FinaleUpgradeLoss;
}

bool hasLegacyStaffStyleAlternateNotationLayout(const finale_mus_reader::ImportReport& report)
{
    return std::ranges::any_of(report.fields, [&](const auto& instanceAndFields) {
        const auto& [instance, unused] = instanceAndFields;
        return instance.classType == typeid(StaffStyleSurveyTarget) && instance.cmper1 &&
               staffStyleUsesAggregateOtherAttachedItems(report, *instance.cmper1);
    });
}

void prepareStaffStyleComparison(ComparisonPreparationContext& context)
{
    if (!sourceIsVersion(context.sourceEpoch, context.sourceVersion,
                         finale_mus_reader::FormatEpoch::ZlibLegacy,
                         finale_mus_reader::versions::finale2009) ||
        !sourceIsBeta(context.sourceVersion) || !context.sourceReport ||
        !hasLegacyStaffStyleAlternateNotationLayout(*context.sourceReport)) {
        return;
    }
    context.source.erase(std::string(staffStyleComparisonKey));
    context.companion.erase(std::string(staffStyleComparisonKey));
}

std::optional<DifferenceClassification>
classifyStaffStyleSmartShapeUpgradeLoss(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs ||
        !comparisonPathStartsWith(context.path, "staff_style[") || !context.sourceValue.isBool() ||
        !context.companionValue.isBool()) {
        return std::nullopt;
    }
    if (context.origin == "legacy-mus-adjusted" &&
        comparisonPathEndsWith(context.path, ".alt_hide_smart_shapes") &&
        context.sourceValue.asBool() && !context.companionValue.asBool() &&
        sourcePredatesVersion(context.epoch, context.sourceVersion,
                              finale_mus_reader::FormatEpoch::ZlibLegacy,
                              finale_mus_reader::versions::finale2009)) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    if (context.origin == "legacy-mus" &&
        comparisonPathEndsWith(context.path, ".alt_hide_other_smart_shapes") &&
        context.sourceValue.asBool() != context.companionValue.asBool() &&
        staffStyleUsesAggregateOtherAttachedItems(context)) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    return std::nullopt;
}

bool staffStyleEqualLeaf(const DifferenceContext& context, std::string_view prefix,
                         std::string_view suffix)
{
    const auto path = std::string(prefix) + std::string(suffix);
    const auto* source = staffStyleLeaf(context.source, path);
    const auto* companion = staffStyleLeaf(context.companion, path);
    return source && companion && *source == *companion;
}

bool staffStyleHasAnyInstrumentOnlyMask(const ComparisonLeaves& leaves, std::string_view prefix)
{
    return std::ranges::any_of(staffStyleInstrumentOnlyMasks, [&](const auto& field) {
        return staffStyleBoolLeaf(leaves, std::string(prefix) + std::string(field.maskSuffix),
                                  true);
    });
}

bool staffStyleHasAllRequiredInstrumentMasks(const ComparisonLeaves& leaves,
                                             std::string_view prefix)
{
    return std::ranges::all_of(staffStyleInstrumentRequiredSharedMasks, [&](const auto suffix) {
        return staffStyleBoolLeaf(leaves, std::string(prefix) + std::string(suffix), true);
    });
}

bool staffStyleIsPre2012InstrumentPromotion(const DifferenceContext& context,
                                            std::string_view companionPrefix)
{
    return sourcePredatesVersion(context.epoch, context.sourceVersion,
                                 finale_mus_reader::FormatEpoch::ZlibLegacy,
                                 finale_mus_reader::versions::finale2012) &&
           staffStyleHasAllRequiredInstrumentMasks(context.companion, companionPrefix) &&
           staffStyleHasAnyInstrumentOnlyMask(context.companion, companionPrefix);
}

std::optional<std::string> staffStyleNamedCompanionPrefix(const DifferenceContext& context,
                                                          std::string_view sourcePrefix)
{
    const auto sourceNamePath = std::string(sourcePrefix) + ".style_name";
    const auto* sourceName = staffStyleLeaf(context.companion, sourceNamePath);
    if (!sourceName || !sourceName->isString())
        return std::nullopt;

    constexpr std::string_view nameSuffix = ".style_name";
    for (const auto& [path, valueAndOrigin] : context.companion) {
        if (!comparisonPathEndsWith(path, nameSuffix) || !valueAndOrigin.first.isString() ||
            valueAndOrigin.first != *sourceName) {
            continue;
        }
        const auto candidate = path.substr(0, path.size() - nameSuffix.size());
        if (candidate != sourcePrefix)
            return std::string(candidate);
    }
    return std::nullopt;
}

bool staffStyleInstrumentPayloadLeavesDefinition(const DifferenceContext& context,
                                                 std::string_view prefix)
{
    // TODO: Once StaffStyleAssign is recovered, use assignment presence to
    // distinguish migrated instrument payload from discarded unassigned
    // definitions.
    return sourcePredatesVersion(context.epoch, context.sourceVersion,
                                 finale_mus_reader::FormatEpoch::ZlibLegacy,
                                 finale_mus_reader::versions::finale2012) &&
           staffStyleHasAnyInstrumentOnlyMask(context.source, prefix) &&
           !staffStyleHasAnyInstrumentOnlyMask(context.companion, prefix) &&
           !staffStyleNamedCompanionPrefix(context, prefix);
}

std::optional<DifferenceClassification>
classifyStaffStyleRemovedInstrumentPayload(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs || context.origin != "legacy-mus")
        return std::nullopt;
    const auto objectPath = staffStyleObjectPrefix(context.path);
    if (objectPath.empty() || !staffStyleInstrumentPayloadLeavesDefinition(context, objectPath)) {
        return std::nullopt;
    }

    const auto relativePath = context.path.substr(objectPath.size());
    if (staffStyleIsInstrumentMask(relativePath)) {
        return context.sourceValue.isBool() && context.sourceValue.asBool() &&
                       context.companionValue.isBool() && !context.companionValue.asBool()
                   ? std::optional{DifferenceClassification::FinaleUpgradeNormalization}
                   : std::nullopt;
    }

    const auto maskSuffix = staffStyleMaskForValue(relativePath);
    if (!maskSuffix || !staffStyleIsInstrumentMask(*maskSuffix))
        return std::nullopt;
    return staffStyleBoolLeaf(context.source, std::string(objectPath) + std::string(*maskSuffix),
                              true) &&
                   staffStyleBoolLeaf(context.companion,
                                      std::string(objectPath) + std::string(*maskSuffix), false)
               ? std::optional{DifferenceClassification::FinaleUpgradeNormalization}
               : std::nullopt;
}

bool staffStyleIsPercussionInstrumentPromotion(const DifferenceContext& context,
                                               std::string_view sourcePrefix,
                                               std::string_view companionPrefix)
{
    const auto* sourceNotation =
        staffStyleLeaf(context.source, std::string(sourcePrefix) + ".notation_style");
    const auto* companionNotation =
        staffStyleLeaf(context.companion, std::string(companionPrefix) + ".notation_style");
    const auto percussion =
        static_cast<std::int64_t>(StaffStyleSurveyTarget::NotationStyle::Percussion);
    return staffStyleIsPre2012InstrumentPromotion(context, companionPrefix) && sourceNotation &&
           sourceNotation->isInteger() && sourceNotation->asInteger() == percussion &&
           companionNotation && companionNotation->isInteger() &&
           companionNotation->asInteger() == percussion &&
           staffStyleBoolLeaf(context.source,
                              std::string(sourcePrefix) + std::string(notationStyleMaskSuffix),
                              true) &&
           staffStyleBoolLeaf(context.companion,
                              std::string(companionPrefix) + std::string(notationStyleMaskSuffix),
                              true);
}

bool staffStyleHasRetainedNonInstrumentMask(const DifferenceContext& context,
                                            std::string_view prefix)
{
    const auto maskPrefix = std::string(prefix) + ".masks.";
    for (const auto& [path, valueAndOrigin] : context.source) {
        if (!comparisonPathStartsWith(path, maskPrefix) ||
            staffStyleIsInstrumentMask(path.substr(prefix.size())) ||
            !valueAndOrigin.first.isBool() || !valueAndOrigin.first.asBool()) {
            continue;
        }
        if (staffStyleBoolLeaf(context.companion, path, true))
            return true;
    }
    return false;
}

std::optional<std::string> staffStyleSplitCompanionPrefix(const DifferenceContext& context,
                                                          std::string_view retainedPrefix)
{
    if (!staffStyleHasRetainedNonInstrumentMask(context, retainedPrefix)) {
        return std::nullopt;
    }
    const auto candidate = staffStyleNamedCompanionPrefix(context, retainedPrefix);
    return candidate && staffStyleIsPre2012InstrumentPromotion(context, *candidate) ? candidate
                                                                                    : std::nullopt;
}

std::optional<DifferenceClassification>
classifyStaffStyleInstrumentPromotion(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs || context.origin != "legacy-mus") {
        return std::nullopt;
    }
    const auto objectPath = staffStyleObjectPrefix(context.path);
    if (objectPath.empty())
        return std::nullopt;

    const auto relativePath = context.path.substr(objectPath.size());
    const auto sameStylePromotion = staffStyleIsPre2012InstrumentPromotion(context, objectPath);
    const auto splitPrefix = sameStylePromotion
                                 ? std::optional<std::string>{}
                                 : staffStyleSplitCompanionPrefix(context, objectPath);
    const auto promotionPrefix =
        sameStylePromotion ? std::optional{std::string(objectPath)} : splitPrefix;
    if (!promotionPrefix)
        return std::nullopt;

    if (sameStylePromotion && relativePath == ".style_name")
        return DifferenceClassification::FinaleUpgradeLoss;

    if (context.sourceValue.isBool() && context.companionValue.isBool()) {
        const auto maskChanged = [&](std::string_view suffix, bool source, bool companion,
                                     std::string_view valueSuffix) {
            return comparisonPathEndsWith(context.path, suffix) &&
                   context.sourceValue.asBool() == source &&
                   context.companionValue.asBool() == companion &&
                   staffStyleEqualLeaf(context, objectPath, valueSuffix);
        };
        if (staffStyleIsPercussionInstrumentPromotion(context, objectPath, *promotionPrefix) &&
            (maskChanged(".masks.float_notehead_font", true, false, ".use_note_font") ||
             maskChanged(".masks.no_key", true, false, ".no_key"))) {
            return DifferenceClassification::FinaleUpgradeNormalization;
        }
    }

    const auto maskSuffix = staffStyleMaskForValue(relativePath);
    if (!staffStyleIsInstrumentMask(relativePath) &&
        (!maskSuffix || !staffStyleIsInstrumentMask(*maskSuffix))) {
        return std::nullopt;
    }

    if (sameStylePromotion)
        return DifferenceClassification::FinaleUpgradeNormalization;

    const auto* splitValue =
        splitPrefix ? staffStyleLeaf(context.companion, *splitPrefix + std::string(relativePath))
                    : nullptr;
    return splitValue && *splitValue == context.sourceValue
               ? std::optional{DifferenceClassification::FinaleUpgradeNormalization}
               : std::nullopt;
}

std::optional<DifferenceClassification>
classifyStaffStyleDifference(const DifferenceContext& context)
{
    constexpr std::string_view styleNameSuffix = ".style_name";
    if (context.category == DifferenceCategory::Differs &&
        comparisonPathStartsWith(context.path, "staff_style[") &&
        (comparisonPathEndsWith(context.path, ".copyable") ||
         comparisonPathEndsWith(context.path, ".add_to_menu"))) {
        return DifferenceClassification::DifferentDefaults;
    }
    if (context.category == DifferenceCategory::Differs && context.origin == "legacy-mus" &&
        comparisonPathStartsWith(context.path, "staff_style[") &&
        comparisonPathEndsWith(context.path, styleNameSuffix) && context.sourceValue.isString() &&
        context.companionValue.isString() &&
        sourcePredatesVersion(context.epoch, context.sourceVersion,
                              finale_mus_reader::FormatEpoch::ZlibLegacy,
                              finale_mus_reader::versions::finale2012) &&
        comparison_text::isWindowsAnsiReinterpretedAsMacRoman(context.sourceValue.asString(),
                                                              context.companionValue.asString())) {
        return DifferenceClassification::TextEncodingError;
    }
    if (const auto removedInstrumentPayload = classifyStaffStyleRemovedInstrumentPayload(context)) {
        return removedInstrumentPayload;
    }
    if (const auto disabledNoteFont =
            staff_fields::classifyDisabledNoteFontSize(context, "staff_style[")) {
        return disabledNoteFont;
    }
    if (const auto maskedOff = classifyMaskedOffStaffStyleValue(context))
        return maskedOff;
    if (const auto smartShapes = classifyStaffStyleSmartShapeUpgradeLoss(context))
        return smartShapes;
    if (const auto attachedItems = classifyTwoBarRepeatAggregateUpgradeLoss(context))
        return attachedItems;
    if (context.category == DifferenceCategory::Differs && context.origin == "legacy-mus" &&
        comparisonPathStartsWith(context.path, "staff_style[") &&
        comparisonPathEndsWith(context.path, styleNameSuffix) && context.sourceValue.isString() &&
        context.companionValue.isString()) {
        const auto& source = context.sourceValue.asString();
        const auto& companion = context.companionValue.asString();
        if (!companion.empty() && companion.size() < source.size() &&
            source.starts_with(companion)) {
            return DifferenceClassification::FinaleUpgradeLoss;
        }
    }
    if (const auto promotion = classifyStaffStyleInstrumentPromotion(context))
        return promotion;
    if (const auto classification =
            staff_fields::classifyNoteAttachedItemsExpressionUpgradeLoss(context, "staff_style[")) {
        return classification;
    }
    const auto aggregateHideSuffix =
        std::ranges::find_if(staffStyleAggregateHideSuffixes, [&](const auto suffix) {
            return comparisonPathEndsWith(context.path, suffix);
        });
    if (aggregateHideSuffix != std::ranges::end(staffStyleAggregateHideSuffixes) &&
        context.category == DifferenceCategory::Differs && context.origin == "legacy-mus" &&
        comparisonPathStartsWith(context.path, "staff_style[") && context.sourceValue.isBool() &&
        context.sourceValue.asBool() && context.companionValue.isBool() &&
        !context.companionValue.asBool() &&
        sourceAtOrAfter(context.epoch, context.sourceVersion,
                        finale_mus_reader::FormatEpoch::UncompressedLegacy,
                        finale_mus_reader::versions::finale2000) &&
        sourcePredatesVersion(context.epoch, context.sourceVersion,
                              finale_mus_reader::FormatEpoch::ZlibLegacy,
                              finale_mus_reader::versions::finale2009)) {
        const auto objectPath =
            context.path.substr(0, context.path.size() - aggregateHideSuffix->size());
        const auto maskPath =
            std::string(objectPath) + ".masks" + std::string(*aggregateHideSuffix);
        const auto mask = context.source.find(maskPath);
        if (staff_fields::sourceHasNoteAttachedItemsExpansion(context, objectPath) &&
            mask != context.source.end() && mask->second.first.isBool() &&
            mask->second.first.asBool() && mask->second.second == "legacy-mus") {
            return DifferenceClassification::FinaleUpgradeLoss;
        }
    }
    if (context.category != DifferenceCategory::Differs || context.origin != "legacy-behavior" ||
        !comparisonPathStartsWith(context.path, "staff_style[") ||
        !comparisonPathEndsWith(context.path, ".inst_uuid") || !context.sourceValue.isString() ||
        !context.companionValue.isString() ||
        context.sourceValue.asString() != musx::dom::uuid::Unknown) {
        return std::nullopt;
    }
    const auto& companion = context.companionValue.asString();
    if (companion == musx::dom::uuid::BlankStaff || companion == musx::dom::uuid::BlankStaff2) {
        return DifferenceClassification::FinaleUpgradeNormalization;
    }
    const auto objectPath = staffStyleObjectPrefix(context.path);
    const auto meaningfulInstrument = !companion.empty() && companion != musx::dom::uuid::Unknown &&
                                      companion != musx::dom::uuid::BlankStaff &&
                                      companion != musx::dom::uuid::BlankStaff2;
    return meaningfulInstrument && staffStyleIsPre2012InstrumentPromotion(context, objectPath)
               ? std::optional{DifferenceClassification::FinaleUpgradeNormalization}
               : std::nullopt;
}

Value observeStaffStyles(const SurveyContext& context)
{
    Value::Array result;
    for (const auto& style : sourceInstances<StaffStyleSurveyTarget>(context)) {
        auto object = staff_fields::observeStaffLike(*style, context);
        staff_fields::addStaffLikeLeaf(object, *style, context, "styleName", "style_name",
                                       style->styleName);
        staff_fields::addStaffLikeLeaf(object, *style, context, "copyable", "copyable",
                                       style->copyable);
        staff_fields::addStaffLikeLeaf(object, *style, context, "addToMenu", "add_to_menu",
                                       style->addToMenu);

        Value::Object masks;
#define STAFF_STYLE_MASK_LEAF(member, leaf)                                                        \
    staff_fields::addStaffLikeLeaf(masks, *style, context, "masks." #member, #member, #leaf,       \
                                   style->masks ? style->masks->member : false)
        STAFF_STYLE_MASK_LEAF(floatNoteheadFont, float_notehead_font);
        STAFF_STYLE_MASK_LEAF(useNoteShapes, use_note_shapes);
        STAFF_STYLE_MASK_LEAF(flatBeams, flat_beams);
        STAFF_STYLE_MASK_LEAF(blankMeasureRest, blank_measure_rest);
        STAFF_STYLE_MASK_LEAF(noOptimize, no_optimize);
        STAFF_STYLE_MASK_LEAF(notationStyle, notation_style);
        STAFF_STYLE_MASK_LEAF(defaultClef, default_clef);
        STAFF_STYLE_MASK_LEAF(staffType, staff_type);
        STAFF_STYLE_MASK_LEAF(transposition, transposition);
        STAFF_STYLE_MASK_LEAF(blineBreak, bline_break);
        STAFF_STYLE_MASK_LEAF(rbarBreak, rbar_break);
        STAFF_STYLE_MASK_LEAF(negMnumb, neg_mnumb);
        STAFF_STYLE_MASK_LEAF(negRepeat, neg_repeat);
        STAFF_STYLE_MASK_LEAF(negNameScore, neg_name_score);
        STAFF_STYLE_MASK_LEAF(hideBarlines, hide_barlines);
        STAFF_STYLE_MASK_LEAF(fullName, full_name);
        STAFF_STYLE_MASK_LEAF(abrvName, abrv_name);
        STAFF_STYLE_MASK_LEAF(floatKeys, float_keys);
        STAFF_STYLE_MASK_LEAF(floatTime, float_time);
        STAFF_STYLE_MASK_LEAF(hideRptBars, hide_rpt_bars);
        STAFF_STYLE_MASK_LEAF(negKey, neg_key);
        STAFF_STYLE_MASK_LEAF(negTime, neg_time);
        STAFF_STYLE_MASK_LEAF(negClef, neg_clef);
        STAFF_STYLE_MASK_LEAF(hideStaff, hide_staff);
        STAFF_STYLE_MASK_LEAF(noKey, no_key);
        STAFF_STYLE_MASK_LEAF(fullNamePos, full_name_pos);
        STAFF_STYLE_MASK_LEAF(abrvNamePos, abrv_name_pos);
        STAFF_STYLE_MASK_LEAF(altNotation, alt_notation);
        STAFF_STYLE_MASK_LEAF(showTies, show_ties);
        STAFF_STYLE_MASK_LEAF(showDots, show_dots);
        STAFF_STYLE_MASK_LEAF(showRests, show_rests);
        STAFF_STYLE_MASK_LEAF(showStems, show_stems);
        STAFF_STYLE_MASK_LEAF(hideChords, hide_chords);
        STAFF_STYLE_MASK_LEAF(hideFretboards, hide_fretboards);
        STAFF_STYLE_MASK_LEAF(hideLyrics, hide_lyrics);
        STAFF_STYLE_MASK_LEAF(showNameParts, show_name_parts);
        STAFF_STYLE_MASK_LEAF(showNoteColors, show_note_colors);
        STAFF_STYLE_MASK_LEAF(hideStaffLines, hide_staff_lines);
        STAFF_STYLE_MASK_LEAF(redisplayLayerAccis, redisplay_layer_accis);
        STAFF_STYLE_MASK_LEAF(negTimeParts, neg_time_parts);
        STAFF_STYLE_MASK_LEAF(hideKeySigsShowAccis, hide_key_sigs_show_accis);
#undef STAFF_STYLE_MASK_LEAF
        object.emplace("masks", std::move(masks));
        result.emplace_back(std::move(object));
    }
    return Value(std::move(result));
}

COVERAGE_CLASS_WITH_PREPARATION("others", staffStyleComparisonKey, observeStaffStyles,
                                classifyStaffStyleDifference, prepareStaffStyleComparison);

} // namespace
