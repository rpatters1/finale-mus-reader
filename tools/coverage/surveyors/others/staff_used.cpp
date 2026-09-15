// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/surveyors/shared/staff_fields.h"
#include "musx/musx.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace finale_mus_reader::coverage;
using StaffUsedSurveyTarget = musx::dom::others::StaffUsed;
using staff_fields::hasStaffSystem;
using staff_fields::isUniformCompanionRespacing;
using staff_fields::StaffUsedItem;
using staff_fields::staffUsedList;

std::string partGlobalsObjectPath(musx::dom::Cmper partId)
{
    const auto identity = partId == musx::dom::SCORE_PARTID
                              ? "cmper=" + std::to_string(musx::dom::MUSX_GLOBALS_CMPER)
                              : "part_id=" + std::to_string(partId) + ",cmper=" + std::to_string(musx::dom::MUSX_GLOBALS_CMPER);
    return "part_globals[" + identity + ']';
}

bool isOrderedSubsequence(const std::vector<StaffUsedItem>& subset, const std::vector<StaffUsedItem>& sequence)
{
    auto position = sequence.begin();
    for (const auto& item : subset) {
        position = std::find_if(position, sequence.end(), [&](const auto& candidate) { return candidate.staffId == item.staffId; });
        if (position == sequence.end()) {
            return false;
        }
        ++position;
    }
    return true;
}

std::optional<musx::dom::Cmper> specialPartExtractionCmper(const DifferenceContext& context, musx::dom::Cmper partId)
{
    constexpr std::string_view fieldSuffix = ".special_part_extraction_i_u_list";
    if (!context.sourceDocumentLeaves) {
        return std::nullopt;
    }
    const auto found = context.sourceDocumentLeaves->find(partGlobalsObjectPath(partId) + std::string(fieldSuffix));
    if (found != context.sourceDocumentLeaves->end() && found->second.first.isInteger() && found->second.first.asInteger() != 0) {
        return static_cast<musx::dom::Cmper>(found->second.first.asInteger());
    }
    return std::nullopt;
}

bool isSpecialPartExtractionReduction(const DifferenceContext& context, musx::dom::Cmper partId, musx::dom::Cmper cmper)
{
    if (!hasStaffSystem(context.sourceDocumentLeaves, partId, cmper) || !hasStaffSystem(context.companionDocumentLeaves, partId, cmper)) {
        return false;
    }
    const auto extractionCmper = specialPartExtractionCmper(context, partId);
    if (!extractionCmper) {
        return false;
    }
    const auto extraction = staffUsedList(context.source, partId, *extractionCmper);
    const auto source = staffUsedList(context.source, partId, cmper);
    const auto companion = staffUsedList(context.companion, partId, cmper);
    if (extraction.empty() || source.empty() || companion.empty() || companion.size() >= source.size()
        || !std::ranges::all_of(source, [](const auto& item) { return item.origin == "legacy-mus"; })) {
        return false;
    }

    std::vector<StaffUsedItem> selected;
    std::ranges::copy_if(source, std::back_inserter(selected),
        [&](const auto& item) { return std::ranges::find(extraction, item.staffId, &StaffUsedItem::staffId) != extraction.end(); });
    return selected.size() == companion.size() && std::ranges::equal(selected, companion, {}, &StaffUsedItem::staffId, &StaffUsedItem::staffId);
}

std::optional<std::vector<std::int64_t>> layoutInsertedStaffs(const DifferenceContext& context, musx::dom::Cmper partId, musx::dom::Cmper cmper)
{
    if (cmper == musx::dom::BASE_SYSTEM_ID || !hasStaffSystem(context.sourceDocumentLeaves, partId, cmper)
        || !hasStaffSystem(context.companionDocumentLeaves, partId, cmper)) {
        return std::nullopt;
    }
    const auto source = staffUsedList(context.source, partId, cmper);
    const auto companion = staffUsedList(context.companion, partId, cmper);
    const auto base = staffUsedList(context.source, partId, musx::dom::BASE_SYSTEM_ID);
    if (source.empty() || companion.size() <= source.size() || base.empty()
        || !std::ranges::all_of(source, [](const auto& item) { return item.origin == "legacy-mus"; }) || !isOrderedSubsequence(source, companion)
        || !isOrderedSubsequence(companion, base)) {
        return std::nullopt;
    }

    std::vector<std::int64_t> inserted;
    std::optional<std::int64_t> segmentDelta;
    for (const auto& companionItem : companion) {
        const auto sourceItem = std::ranges::find(source, companionItem.staffId, &StaffUsedItem::staffId);
        if (sourceItem == source.end()) {
            inserted.push_back(companionItem.staffId);
            segmentDelta.reset();
            continue;
        }
        const auto delta = companionItem.distFromTop - sourceItem->distFromTop;
        if (segmentDelta && *segmentDelta != delta) {
            return std::nullopt;
        }
        segmentDelta = delta;
    }
    return inserted.empty() ? std::nullopt : std::optional{std::move(inserted)};
}

std::optional<DifferenceClassification> classifyStaffUsedDifference(const DifferenceContext& context)
{
    const auto partId = staff_fields::partIdFromComparisonPath(context.path);
    const auto cmper = staff_fields::staffLikeCmperFromComparisonPath(context.path);
    const auto objectEnd = context.path.find("].");
    if (!partId || !cmper || objectEnd == std::string_view::npos) {
        return std::nullopt;
    }
    if (context.category == DifferenceCategory::CompanionOnly && *cmper == musx::dom::STUDIO_VIEW_SYSTEM_ID
        && context.epoch != finale_mus_reader::FormatEpoch::ZlibLegacy) {
        return DifferenceClassification::FinaleUpgradeSynthesis;
    }
    if (context.category == DifferenceCategory::Differs && context.corpusId == staff_fields::finale27DamagedPageLayoutCorpusId) {
        return DifferenceClassification::FinaleLayoutRecalculation;
    }
    const auto sourceHasSystem = hasStaffSystem(context.sourceDocumentLeaves, *partId, *cmper);
    const auto companionHasSystem = hasStaffSystem(context.companionDocumentLeaves, *partId, *cmper);
    if (context.category == DifferenceCategory::CompanionOnly && !sourceHasSystem && companionHasSystem) {
        return DifferenceClassification::FinaleLayoutRecalculation;
    }
    if ((context.category == DifferenceCategory::ReaderOnly || context.category == DifferenceCategory::CompanionOnly)
        && *cmper != musx::dom::BASE_SYSTEM_ID && *cmper < musx::dom::STUDIO_VIEW_SYSTEM_ID && !sourceHasSystem && !companionHasSystem) {
        return DifferenceClassification::FinaleLayoutRecalculation;
    }
    const auto object = context.path.substr(0, objectEnd + 1);
    if (context.category == DifferenceCategory::Differs && context.origin == "legacy-mus-adjusted" && context.path.ends_with(".dist_from_top")
        && isUniformCompanionRespacing(context, *partId, *cmper)) {
        return DifferenceClassification::FinaleLayoutRecalculation;
    }
    if ((context.category == DifferenceCategory::ReaderOnly
            || (context.category == DifferenceCategory::Differs && context.path.ends_with(".dist_from_top")))
        && isSpecialPartExtractionReduction(context, *partId, *cmper)) {
        return DifferenceClassification::FinaleLayoutRecalculation;
    }
    const auto staffId = comparisonIntegerLeaf(context.companion, std::string(object) + ".staff_id");
    if (!staffId) {
        return std::nullopt;
    }
    const auto inserted = layoutInsertedStaffs(context, *partId, *cmper);
    if (!inserted) {
        return std::nullopt;
    }
    if (context.category == DifferenceCategory::CompanionOnly && std::ranges::find(*inserted, *staffId) != inserted->end()) {
        return DifferenceClassification::FinaleLayoutRecalculation;
    }
    if (context.category == DifferenceCategory::Differs && context.path.ends_with(".dist_from_top")) {
        return DifferenceClassification::FinaleLayoutRecalculation;
    }
    return std::nullopt;
}

std::string staffUsedMatchKey(const StaffUsedSurveyTarget& value)
{
    return "part=" + std::to_string(value.getSourcePartId()) + ",cmper=" + std::to_string(value.getCmper())
           + ",staff=" + std::to_string(value.staffId);
}

std::string staffUsedOrigin(const StaffUsedSurveyTarget& value, const SurveyContext& context, const char* member)
{
    return fieldOrigin<StaffUsedSurveyTarget>(context, member, value);
}

Value observeStaffUsedRange(const StaffUsedSurveyTarget& value, const SurveyContext& context)
{
    if (!value.range) {
        return {};
    }
    return Value::Object{{"start_meas", value.range->startMeas}, {"start_edu", value.range->startEdu}, {"end_meas", value.range->endMeas},
        {"end_edu", value.range->endEdu}, {"origin_startMeas", staffUsedOrigin(value, context, "range.startMeas")},
        {"origin_startEdu", staffUsedOrigin(value, context, "range.startEdu")}, {"origin_endMeas", staffUsedOrigin(value, context, "range.endMeas")},
        {"origin_endEdu", staffUsedOrigin(value, context, "range.endEdu")}};
}

Value observeStaffUsed(const SurveyContext& context)
{
    using Target = StaffUsedSurveyTarget;
    Value::Array result;
    for (const auto& value : sourceInstances<Target>(context)) {
        result.push_back(observe(*value, context, field("cmper", [](const Target& item) { return item.getCmper(); }),
            field("_report_match_key", &staffUsedMatchKey), field("_classifier_inci", [](const Target& item) { return item.getInci(); }),
            field("staff_id", &Target::staffId), field("dist_from_top", &Target::distFromTop), field("range", &observeStaffUsedRange),
            field("origin_staffId", [](const Target& item, const SurveyContext& ctx) { return staffUsedOrigin(item, ctx, "staffId"); }),
            field("origin_distFromTop", [](const Target& item, const SurveyContext& ctx) { return staffUsedOrigin(item, ctx, "distFromTop"); })));
    }
    return Value(std::move(result));
}

COVERAGE_CLASS("others", "staff_used", observeStaffUsed, classifyStaffUsedDifference);

} // namespace
