// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "coverage/common/tie_options_info.h"
#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace
{

using namespace finale_mus_reader::coverage;

Value observeTieOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::TieOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(
        *options, ctx, field("front_tie_separ", &Target::frontTieSepar),
        field("thickness_right", &Target::thicknessRight),
        field("thickness_left", &Target::thicknessLeft),
        field("break_for_time_sigs", &Target::breakForTimeSigs),
        field("break_for_key_sigs", &Target::breakForKeySigs),
        field("break_time_sig_left_h_offset", &Target::breakTimeSigLeftHOffset),
        field("break_time_sig_right_h_offset", &Target::breakTimeSigRightHOffset),
        field("break_key_sig_left_h_offset", &Target::breakKeySigLeftHOffset),
        field("break_key_sig_right_h_offset", &Target::breakKeySigRightHOffset),
        field("sys_break_left_h_adj", &Target::sysBreakLeftHAdj),
        field("sys_break_right_h_adj", &Target::sysBreakRightHAdj),
        field("use_outer_placement", &Target::useOuterPlacement),
        field("seconds_placement", &Target::secondsPlacement),
        field("chord_tie_dir_type", &Target::chordTieDirType),
        field("chord_tie_dir_opposing_seconds", &Target::chordTieDirOpposingSeconds),
        field("mixed_stem_direction", &Target::mixedStemDirection),
        field("after_single_dot", &Target::afterSingleDot),
        field("after_multiple_dots", &Target::afterMultipleDots),
        field("before_acci_single_note", &Target::beforeAcciSingleNote),
        field("special_pos_mode", &Target::specialPosMode),
        field("avoid_staff_lines_distance", &Target::avoidStaffLinesDistance),
        field("inset_style", &Target::insetStyle),
        field("use_interpolation", &Target::useInterpolation),
        field("use_tie_end_ctl_style", &Target::useTieEndCtlStyle),
        field("avoid_staff_lines_only", &Target::avoidStaffLinesOnly),
        field("tie_tip_width", &Target::tieTipWidth),
        originField<Target>("origin_frontTieSepar", "frontTieSepar"),
        originField<Target>("origin_thicknessRight", "thicknessRight"),
        originField<Target>("origin_thicknessLeft", "thicknessLeft"),
        originField<Target>("origin_breakForTimeSigs", "breakForTimeSigs"),
        originField<Target>("origin_breakForKeySigs", "breakForKeySigs"),
        originField<Target>("origin_breakTimeSigLeftHOffset", "breakTimeSigLeftHOffset"),
        originField<Target>("origin_breakTimeSigRightHOffset", "breakTimeSigRightHOffset"),
        originField<Target>("origin_breakKeySigLeftHOffset", "breakKeySigLeftHOffset"),
        originField<Target>("origin_breakKeySigRightHOffset", "breakKeySigRightHOffset"),
        originField<Target>("origin_sysBreakLeftHAdj", "sysBreakLeftHAdj"),
        originField<Target>("origin_sysBreakRightHAdj", "sysBreakRightHAdj"),
        originField<Target>("origin_useOuterPlacement", "useOuterPlacement"),
        originField<Target>("origin_secondsPlacement", "secondsPlacement"),
        originField<Target>("origin_chordTieDirType", "chordTieDirType"),
        originField<Target>("origin_chordTieDirOpposingSeconds", "chordTieDirOpposingSeconds"),
        originField<Target>("origin_mixedStemDirection", "mixedStemDirection"),
        originField<Target>("origin_afterSingleDot", "afterSingleDot"),
        originField<Target>("origin_afterMultipleDots", "afterMultipleDots"),
        originField<Target>("origin_beforeAcciSingleNote", "beforeAcciSingleNote"),
        originField<Target>("origin_specialPosMode", "specialPosMode"),
        originField<Target>("origin_avoidStaffLinesDistance", "avoidStaffLinesDistance"),
        originField<Target>("origin_insetStyle", "insetStyle"),
        originField<Target>("origin_useInterpolation", "useInterpolation"),
        originField<Target>("origin_useTieEndCtlStyle", "useTieEndCtlStyle"),
        originField<Target>("origin_avoidStaffLinesOnly", "avoidStaffLinesOnly"),
        originField<Target>("origin_tieTipWidth", "tieTipWidth"));

    std::vector<std::pair<musx::dom::TieConnectStyleType, std::shared_ptr<Target::ConnectStyle>>>
        connections(options->tieConnectStyles.begin(), options->tieConnectStyles.end());
    std::ranges::sort(connections, {}, [](const auto& item) { return item.first; });
    Value::Array observedConnections;
    for (std::size_t index = 0; index < connections.size(); ++index)
    {
        const auto& [type, style] = connections[index];
        const auto prefix = "tieConnectStyles[" + std::to_string(index) + "].";
        observedConnections.emplace_back(
            Value::Object{{"type", static_cast<std::int64_t>(type)},
                          {"offset_x", style->offsetX},
                          {"offset_y", style->offsetY},
                          {"origin_offsetX", fieldOrigin<Target>(ctx, prefix + "offsetX")},
                          {"origin_offsetY", fieldOrigin<Target>(ctx, prefix + "offsetY")}});
    }
    result.asObject().emplace("tie_connect_styles", std::move(observedConnections));

    std::vector<std::pair<Target::ControlStyleType, std::shared_ptr<Target::ControlStyle>>>
        controls(options->tieControlStyles.begin(), options->tieControlStyles.end());
    std::ranges::sort(controls, {}, [](const auto& item) { return item.first; });
    Value::Array observedControls;
    for (std::size_t index = 0; index < controls.size(); ++index)
    {
        const auto& [type, style] = controls[index];
        const auto prefix = "tieControlStyles[" + std::to_string(index) + "].";
        Value::Object observed{{"type", static_cast<std::int64_t>(type)},
                               {"span", style->span},
                               {"origin_span", fieldOrigin<Target>(ctx, prefix + "span")}};
        const auto addPoint =
            [&](std::string_view name, const std::shared_ptr<Target::ControlPoint>& point)
        {
            if (!point) return;
            const auto reportPrefix = prefix + std::string(name) + ".";
            observed.emplace(
                std::string(name),
                Value::Object{
                    {"inset_ratio", point->insetRatio},
                    {"height", point->height},
                    {"inset_fixed", point->insetFixed},
                    {"origin_insetRatio", fieldOrigin<Target>(ctx, reportPrefix + "insetRatio")},
                    {"origin_height", fieldOrigin<Target>(ctx, reportPrefix + "height")},
                    {"origin_insetFixed", fieldOrigin<Target>(ctx, reportPrefix + "insetFixed")}});
        };
        addPoint("cp1", style->cp1);
        addPoint("cp2", style->cp2);
        observedControls.emplace_back(std::move(observed));
    }
    result.asObject().emplace("tie_control_styles", std::move(observedControls));
    return result;
}

COVERAGE_CLASS("options", "tie_options", observeTieOptions, classifyTieOptionsDifference);

} // namespace
