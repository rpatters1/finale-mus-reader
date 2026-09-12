// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/comparison_text.h"
#include "coverage/registry.h"
#include "coverage/surveyors/shared/staff_fields.h"
#include "coverage/surveyors/shared/staff_style_semantics.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;
using namespace finale_mus_reader::coverage::staff_style_semantics;
using StaffStyleSurveyTarget = musx::dom::others::StaffStyle;

constexpr std::string_view staffStyleComparisonKey = "staff_style";

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
    const auto maskSuffix = maskForValue(context.path.substr(objectPath.size()));
    if (!maskSuffix)
        return std::nullopt;
    const auto maskPath = std::string(objectPath) + std::string(*maskSuffix);
    return staffStyleBoolLeaf(context.source, maskPath, false) &&
                   staffStyleBoolLeaf(context.companion, maskPath, false)
               ? std::optional{DifferenceClassification::DifferentDefaults}
               : std::nullopt;
}

bool staffStyleUsesAggregateOtherAttachedItems(const DifferenceContext& context)
{
    const auto cmper = staff_fields::staffLikeCmperFromComparisonPath(context.path);
    return cmper && usesAggregateOtherAttachedItems(context.sourceReport, *cmper);
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
               usesAggregateOtherAttachedItems(report, *instance.cmper1);
    });
}

void prepareStaffStyleComparison(ComparisonPreparationContext& context)
{
    const auto removeInactiveTablature = [](SurveySnapshot& snapshot) {
        const auto found = snapshot.find(staffStyleComparisonKey);
        if (found == snapshot.end() || !found->second.isArray())
            return;
        for (auto& style : found->second.asArray()) {
            if (style.isObject())
                removeInactiveTablatureValues(style.asObject());
        }
    };
    removeInactiveTablature(context.source);
    removeInactiveTablature(context.companion);

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
    return staff_fields::classifyAggregateOtherSmartShapeUpgradeLoss(
        context, "staff_style[", staffStyleUsesAggregateOtherAttachedItems(context));
}

bool staffStyleEqualLeaf(const DifferenceContext& context, std::string_view prefix,
                         std::string_view suffix)
{
    const auto path = std::string(prefix) + std::string(suffix);
    const auto* source = staffStyleLeaf(context.source, path);
    const auto* companion = staffStyleLeaf(context.companion, path);
    return source && companion && *source == *companion;
}

bool staffStyleIsPre2012InstrumentPromotion(const DifferenceContext& context,
                                            std::string_view companionPrefix)
{
    return sourcePredatesVersion(context.epoch, context.sourceVersion,
                                 finale_mus_reader::FormatEpoch::ZlibLegacy,
                                 finale_mus_reader::versions::finale2012) &&
           hasAllRequiredInstrumentMasks(context.companion, companionPrefix) &&
           hasAnyInstrumentOnlyMask(context.companion, companionPrefix);
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
    // TODO: Use semantic assignment ranges to distinguish migrated instrument
    // payload from discarded unassigned definitions.
    return sourcePredatesVersion(context.epoch, context.sourceVersion,
                                 finale_mus_reader::FormatEpoch::ZlibLegacy,
                                 finale_mus_reader::versions::finale2012) &&
           hasAnyInstrumentOnlyMask(context.source, prefix) &&
           !hasAnyInstrumentOnlyMask(context.companion, prefix) &&
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
    if (isInstrumentMask(relativePath)) {
        return context.sourceValue.isBool() && context.sourceValue.asBool() &&
                       context.companionValue.isBool() && !context.companionValue.asBool()
                   ? std::optional{DifferenceClassification::FinaleUpgradeNormalization}
                   : std::nullopt;
    }

    const auto maskSuffix = maskForValue(relativePath);
    if (!maskSuffix || !isInstrumentMask(*maskSuffix))
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
            isInstrumentMask(path.substr(prefix.size())) || !valueAndOrigin.first.isBool() ||
            !valueAndOrigin.first.asBool()) {
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

    const auto maskSuffix = maskForValue(relativePath);
    if (!isInstrumentMask(relativePath) && (!maskSuffix || !isInstrumentMask(*maskSuffix))) {
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
    if (const auto aggregateHide = staff_fields::classifyNoteAttachedItemsAggregateHideUpgradeLoss(
            context, "staff_style[", ".masks.", "legacy-mus")) {
        return aggregateHide;
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
    for (const auto& style : sourceInstances<StaffStyleSurveyTarget>(context))
        result.emplace_back(staff_style_semantics::observe(*style, context));
    return Value(std::move(result));
}

COVERAGE_CLASS_WITH_PREPARATION("others", staffStyleComparisonKey, observeStaffStyles,
                                classifyStaffStyleDifference, prepareStaffStyleComparison);

} // namespace
