// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/support/source_gate.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyLineCurveOptionsDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category == Differs && context.origin == "legacy-mus" &&
        sourceIsVersion(context.epoch, context.sourceVersion,
            finale_mus_reader::FormatEpoch::CodaBanner,
            finale_mus_reader::versions::finale1_0) &&
        (context.path == "line_curve_options.ps_ul_depth" ||
         context.path == "line_curve_options.ps_ul_width")) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    const bool legacyEnclosureWidth =
        context.path == "line_curve_options.enclosure_width" &&
        (context.epoch == finale_mus_reader::FormatEpoch::CodaBanner ||
         context.epoch == finale_mus_reader::FormatEpoch::UncompressedLegacy);
    const bool codaStaffWidth =
        context.epoch == finale_mus_reader::FormatEpoch::CodaBanner &&
        (context.path == "line_curve_options.staff_line_width" ||
         context.path == "line_curve_options.leger_line_width");
    if (context.category == Differs &&
        context.origin == "legacy-behavior" && context.sourceValue.isInteger() &&
        context.sourceValue.asInteger() == 118 && context.companionValue.isInteger() &&
        context.companionValue.asInteger() == 224 &&
        (legacyEnclosureWidth || codaStaffWidth)) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

Value observeLineCurveOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::LineCurveOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("bezier_step", &Target::bezierStep),
        field("enclosure_width", &Target::enclosureWidth),
        field("enclosure_round_corners", &Target::enclosureRoundCorners),
        field("enclosure_corner_radius", &Target::enclosureCornerRadius),
        field("staff_line_width", &Target::staffLineWidth),
        field("leger_line_width", &Target::legerLineWidth),
        field("leger_front_length", &Target::legerFrontLength),
        field("leger_back_length", &Target::legerBackLength),
        field("rest_leger_front_length", &Target::restLegerFrontLength),
        field("rest_leger_back_length", &Target::restLegerBackLength),
        field("ps_ul_depth", &Target::psUlDepth),
        field("ps_ul_width", &Target::psUlWidth),
        field("path_slur_tip_width", &Target::pathSlurTipWidth));
    for (const auto* member : {"bezierStep", "enclosureWidth", "enclosureRoundCorners",
             "enclosureCornerRadius", "staffLineWidth", "legerLineWidth",
             "legerFrontLength", "legerBackLength", "restLegerFrontLength",
             "restLegerBackLength", "psUlDepth", "psUlWidth", "pathSlurTipWidth"}) {
        result.asObject().emplace(std::string("origin_") + member,
            fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_CLASS("options", "line_curve_options", observeLineCurveOptions,
    classifyLineCurveOptionsDifference);

} // namespace
