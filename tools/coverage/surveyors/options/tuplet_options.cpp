// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "import/support/legacy_mapping.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeTupletOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::TupletOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("display_number", &Target::displayNumber),
        field("display_duration", &Target::displayDuration),
        field("reference_number", &Target::referenceNumber),
        field("reference_duration", &Target::referenceDuration),
        field("always_flat", &Target::alwaysFlat), field("full_dura", &Target::fullDura),
        field("metric_center", &Target::metricCenter),
        field("avoid_staff", &Target::avoidStaff),
        field("auto_bracket_style", &Target::autoBracketStyle),
        field("tup_off_x", &Target::tupOffX), field("tup_off_y", &Target::tupOffY),
        field("brack_off_x", &Target::brackOffX), field("brack_off_y", &Target::brackOffY),
        field("num_style", &Target::numStyle), field("pos_style", &Target::posStyle),
        field("allow_horz", &Target::allowHorz),
        field("ignore_horz_num_offset", &Target::ignoreHorzNumOffset),
        field("break_bracket", &Target::breakBracket),
        field("match_hooks", &Target::matchHooks),
        field("use_bottom_note", &Target::useBottomNote),
        field("brack_style", &Target::brackStyle),
        field("smart_tuplet", &Target::smartTuplet),
        field("left_hook_len", &Target::leftHookLen),
        field("left_hook_ext", &Target::leftHookExt),
        field("right_hook_len", &Target::rightHookLen),
        field("right_hook_ext", &Target::rightHookExt),
        field("manual_slope_adj", &Target::manualSlopeAdj),
        field("tup_max_slope", &Target::tupMaxSlope),
        field("tup_line_width", &Target::tupLineWidth),
        field("tup_n_upstem_offset", &Target::tupNUpstemOffset),
        field("tup_n_downstem_offset", &Target::tupNDownstemOffset));
    for (const auto* member : {"displayNumber", "displayDuration", "referenceNumber",
             "referenceDuration", "alwaysFlat", "fullDura", "metricCenter", "avoidStaff",
             "autoBracketStyle", "tupOffX", "tupOffY", "brackOffX", "brackOffY", "numStyle",
             "posStyle", "allowHorz", "ignoreHorzNumOffset", "breakBracket", "matchHooks",
             "useBottomNote", "brackStyle", "smartTuplet", "leftHookLen", "leftHookExt",
             "rightHookLen", "rightHookExt", "manualSlopeAdj", "tupMaxSlope", "tupLineWidth",
             "tupNUpstemOffset", "tupNDownstemOffset"}) {
        result.asObject().emplace(
            std::string("origin_") + member, fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_CLASS("options", "tuplet_options", observeTupletOptions, nullptr);

} // namespace
