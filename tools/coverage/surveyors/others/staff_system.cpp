// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <limits>
#include <optional>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/surveyors/shared/staff_fields.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;
using StaffSystemSurveyTarget = musx::dom::others::StaffSystem;

struct StaffSystemBounds {
    std::optional<std::int64_t> firstCmper;
    std::optional<std::int64_t> firstStart;
    std::optional<std::int64_t> lastCmper;
    std::optional<std::int64_t> lastEnd;
};

std::optional<StaffSystemBounds> staffSystemBounds(
    const ComparisonLeaves& leaves, musx::dom::Cmper partId)
{
    StaffSystemBounds result;
    for (const auto& [path, valueAndOrigin] : leaves) {
        if (!path.starts_with("staff_systems[") ||
            !valueAndOrigin.first.isInteger() ||
            staff_fields::partIdFromComparisonPath(path) != partId) {
            continue;
        }
        const auto cmper = staff_fields::staffLikeCmperFromComparisonPath(path);
        if (!cmper) continue;
        if (path.ends_with(".start_meas") &&
            (!result.firstCmper || *cmper < *result.firstCmper)) {
            result.firstCmper = *cmper;
            result.firstStart = valueAndOrigin.first.asInteger();
        } else if (path.ends_with(".end_meas") &&
                   (!result.lastCmper || *cmper > *result.lastCmper)) {
            result.lastCmper = *cmper;
            result.lastEnd = valueAndOrigin.first.asInteger();
        }
    }
    return result.firstStart && result.lastEnd ? std::optional{result}
                                                : std::nullopt;
}

std::optional<std::int64_t> lastScoreMeasure(
    const ComparisonLeaves& leaves)
{
    std::optional<std::int64_t> result;
    for (const auto& [path, unused] : leaves) {
        if (!path.starts_with("measures[") ||
            staff_fields::partIdFromComparisonPath(path) !=
                musx::dom::SCORE_PARTID) {
            continue;
        }
        const auto cmper = staff_fields::staffLikeCmperFromComparisonPath(path);
        if (cmper && (!result || *cmper > *result)) result = *cmper;
    }
    return result;
}

std::optional<DifferenceClassification> classifyStaffSystemDifference(
    const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (deferredRecoveryClassified() && context.category == Differs &&
        context.path.starts_with("staff_systems[")) {
        if (context.path.ends_with(".distance_to_prev") ||
            (context.origin == "unmapped" &&
             (context.path.ends_with(".top") ||
              context.path.ends_with(".has_staff_scaling")))) {
            return DifferenceClassification::AwaitsDependentRecovery;
        }
    }
    if (context.category != Differs ||
        !context.path.starts_with("staff_systems[")) {
        return std::nullopt;
    }
    if (context.path.ends_with(".horz_percent")) {
        return DifferenceClassification::FinaleLayoutRecalculation;
    }
    if ((!context.path.ends_with(".start_meas") &&
         !context.path.ends_with(".end_meas")) ||
        !context.sourceValue.isInteger() || !context.companionValue.isInteger()) {
        return std::nullopt;
    }
    if (!context.sourceDocumentLeaves) return std::nullopt;
    const auto partId = staff_fields::partIdFromComparisonPath(context.path);
    const auto sourceSystem =
        staff_fields::staffLikeCmperFromComparisonPath(context.path);
    const auto lastMeasure = lastScoreMeasure(*context.sourceDocumentLeaves);
    if (!partId || !sourceSystem || !lastMeasure ||
        *lastMeasure == (std::numeric_limits<std::int64_t>::max)()) {
        return std::nullopt;
    }
    const auto sourceBounds = staffSystemBounds(context.source, *partId);
    const auto companionBounds = staffSystemBounds(context.companion, *partId);
    const auto sourceHasValidBounds = sourceBounds &&
                                      sourceBounds->firstStart == 1 &&
                                      sourceBounds->lastEnd == *lastMeasure + 1;
    if (context.path.ends_with(".end_meas") && sourceHasValidBounds &&
        sourceBounds->lastCmper == *sourceSystem &&
        sourceBounds->lastEnd == context.sourceValue.asInteger()) {
        return DifferenceClassification::FinaleLayoutRecalculation;
    }
    return sourceHasValidBounds && companionBounds &&
                   companionBounds->firstStart == 1 &&
                   companionBounds->lastEnd >= sourceBounds->lastEnd
               ? std::optional{DifferenceClassification::
                                   FinaleLayoutRecalculation}
               : std::nullopt;
}

auto staffSystemOrigin(const char* member)
{
    return [member](const StaffSystemSurveyTarget& value,
                    const SurveyContext& context) {
        return fieldOrigin<StaffSystemSurveyTarget>(context, member, value);
    };
}

Value observeStaffSystems(const SurveyContext& ctx)
{
    using Target = StaffSystemSurveyTarget;
    Value::Array result;
    for (const auto& system : sourceInstances<Target>(ctx)) {
        result.push_back(observe(
            *system, ctx,
            field("cmper",
                  [](const Target& value) { return value.getCmper(); }),
            field("start_meas", &Target::startMeas),
            field("end_meas", &Target::endMeas),
            field("horz_percent", &Target::horzPercent),
            field("ssys_percent", &Target::ssysPercent),
            field("staff_height", &Target::staffHeight),
            field("top", &Target::top), field("left", &Target::left),
            field("right", &Target::right), field("bottom", &Target::bottom),
            field("no_names", &Target::noNames),
            field("has_staff_scaling", &Target::hasStaffScaling),
            field("place_end_space_before_barline",
                  &Target::placeEndSpaceBeforeBarline),
            field("scale_vert", &Target::scaleVert),
            field("hold_margins", &Target::holdMargins),
            field("distance_to_prev", &Target::distanceToPrev),
            field("extra_start_system_space", &Target::extraStartSystemSpace),
            field("extra_end_system_space", &Target::extraEndSystemSpace),
            field("origin_startMeas", staffSystemOrigin("startMeas")),
            field("origin_endMeas", staffSystemOrigin("endMeas")),
            field("origin_horzPercent", staffSystemOrigin("horzPercent")),
            field("origin_ssysPercent", staffSystemOrigin("ssysPercent")),
            field("origin_staffHeight", staffSystemOrigin("staffHeight")),
            field("origin_top", staffSystemOrigin("top")),
            field("origin_left", staffSystemOrigin("left")),
            field("origin_right", staffSystemOrigin("right")),
            field("origin_bottom", staffSystemOrigin("bottom")),
            field("origin_noNames", staffSystemOrigin("noNames")),
            field("origin_hasStaffScaling",
                  staffSystemOrigin("hasStaffScaling")),
            field("origin_placeEndSpaceBeforeBarline",
                  staffSystemOrigin("placeEndSpaceBeforeBarline")),
            field("origin_scaleVert", staffSystemOrigin("scaleVert")),
            field("origin_holdMargins", staffSystemOrigin("holdMargins")),
            field("origin_distanceToPrev", staffSystemOrigin("distanceToPrev")),
            field("origin_extraStartSystemSpace",
                  staffSystemOrigin("extraStartSystemSpace")),
            field("origin_extraEndSystemSpace",
                  staffSystemOrigin("extraEndSystemSpace"))));
    }
    return Value(std::move(result));
}

COVERAGE_CLASS("others", "staff_systems", observeStaffSystems,
               classifyStaffSystemDifference);

}  // namespace
