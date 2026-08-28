// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeBarlineOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::BarlineOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("draw_barlines", &Target::drawBarlines),
        field("draw_close_system_barline", &Target::drawCloseSystemBarline),
        field("draw_close_final_barline", &Target::drawCloseFinalBarline),
        field("draw_final_barline_on_last_meas", &Target::drawFinalBarlineOnLastMeas),
        field("draw_double_barline_before_key_changes",
            &Target::drawDoubleBarlineBeforeKeyChanges),
        field("draw_left_barline_single_staff", &Target::drawLeftBarlineSingleStaff),
        field("draw_left_barline_multiple_staves", &Target::drawLeftBarlineMultipleStaves),
        field("left_barline_use_prev_style", &Target::leftBarlineUsePrevStyle),
        field("thick_barline_width", &Target::thickBarlineWidth),
        field("barline_width", &Target::barlineWidth),
        field("double_barline_space", &Target::doubleBarlineSpace),
        field("final_barline_space", &Target::finalBarlineSpace),
        field("barline_dash_on", &Target::barlineDashOn),
        field("barline_dash_off", &Target::barlineDashOff));
    for (const auto* member : {"drawBarlines",
                               "drawCloseSystemBarline",
                               "drawCloseFinalBarline",
                               "drawFinalBarlineOnLastMeas",
                               "drawDoubleBarlineBeforeKeyChanges",
                               "drawLeftBarlineSingleStaff",
                               "drawLeftBarlineMultipleStaves",
                               "leftBarlineUsePrevStyle",
                               "thickBarlineWidth",
                               "barlineWidth",
                               "doubleBarlineSpace",
                               "finalBarlineSpace",
                               "barlineDashOn",
                               "barlineDashOff"}) {
        result.asObject().emplace(std::string("origin_") + member,
            fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_SURVEYOR("options", "barline_options", observeBarlineOptions);

} // namespace
