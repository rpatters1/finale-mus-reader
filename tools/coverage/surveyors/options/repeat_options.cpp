// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeRepeatOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::RepeatOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("max_passes", &Target::maxPasses), field("add_period", &Target::addPeriod),
        field("thick_line_width", &Target::thickLineWidth), field("thin_line_width", &Target::thinLineWidth),
        field("line_space", &Target::lineSpace), field("back_to_back_style", &Target::backToBackStyle),
        field("forward_dot_h_pos", &Target::forwardDotHPos), field("backward_dot_h_pos", &Target::backwardDotHPos),
        field("upper_dot_v_pos", &Target::upperDotVPos), field("lower_dot_v_pos", &Target::lowerDotVPos),
        field("wing_style", &Target::wingStyle), field("after_clef_space", &Target::afterClefSpace),
        field("after_key_space", &Target::afterKeySpace), field("after_time_space", &Target::afterTimeSpace),
        field("bracket_height", &Target::bracketHeight), field("bracket_hook_len", &Target::bracketHookLen),
        field("bracket_line_width", &Target::bracketLineWidth), field("bracket_start_inset", &Target::bracketStartInset),
        field("bracket_end_inset", &Target::bracketEndInset), field("bracket_text_h_pos", &Target::bracketTextHPos),
        field("bracket_text_v_pos", &Target::bracketTextVPos), field("bracket_end_hook_len", &Target::bracketEndHookLen),
        field("bracket_end_anchor_thin_line", &Target::bracketEndAnchorThinLine),
        field("show_on_top_staff_only", &Target::showOnTopStaffOnly),
        field("show_on_staff_list_number", &Target::showOnStaffListNumber));
    for (const auto* member : {"maxPasses", "addPeriod", "thickLineWidth", "thinLineWidth",
             "lineSpace", "backToBackStyle", "forwardDotHPos", "backwardDotHPos",
             "upperDotVPos", "lowerDotVPos", "wingStyle", "afterClefSpace",
             "afterKeySpace", "afterTimeSpace", "bracketHeight", "bracketHookLen",
             "bracketLineWidth", "bracketStartInset", "bracketEndInset", "bracketTextHPos",
             "bracketTextVPos", "bracketEndHookLen", "bracketEndAnchorThinLine",
             "showOnTopStaffOnly", "showOnStaffListNumber"}) {
        result.asObject().emplace(std::string("origin_") + member,
            fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_SURVEYOR("options", "repeat_options", observeRepeatOptions);

} // namespace
