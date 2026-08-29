// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeBeamOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::BeamOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("beam_stub_length", &Target::beamStubLength),
        field("max_slope", &Target::maxSlope),
        field("beam_separ", &Target::beamSepar),
        field("max_from_middle", &Target::maxFromMiddle),
        field("beaming_style", &Target::beamingStyle),
        field("extend_beams_over_rests", &Target::extendBeamsOverRests),
        field("inc_rests_in_four_groups", &Target::incRestsInFourGroups),
        field("beam_four_eighths_in_common_time",
            &Target::beamFourEighthsInCommonTime),
        field("beam_three_eighths_in_common_time",
            &Target::beamThreeEighthsInCommonTime),
        field("disp_half_stems_on_rests", &Target::dispHalfStemsOnRests),
        field("old_finale_rest_beams", &Target::oldFinaleRestBeams),
        field("span_space", &Target::spanSpace),
        field("extend_sec_beams_over_rests", &Target::extendSecBeamsOverRests),
        field("beam_width", &Target::beamWidth));
    for (const auto* member : {"beamStubLength",
                               "maxSlope",
                               "beamSepar",
                               "maxFromMiddle",
                               "beamingStyle",
                               "extendBeamsOverRests",
                               "incRestsInFourGroups",
                               "beamFourEighthsInCommonTime",
                               "beamThreeEighthsInCommonTime",
                               "dispHalfStemsOnRests",
                               "oldFinaleRestBeams",
                               "spanSpace",
                               "extendSecBeamsOverRests",
                               "beamWidth"}) {
        result.asObject().emplace(std::string("origin_") + member,
            fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_SURVEYOR("options", "beam_options", observeBeamOptions);

} // namespace
