// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "import/support/legacy_mapping.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeTimeSignatureOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::TimeSignatureOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options)
        return {};
    auto result = observe(*options, ctx, field("time_upper_lift", &Target::timeUpperLift),
        field("time_front", &Target::timeFront), field("time_back", &Target::timeBack),
        field("time_front_parts", &Target::timeFrontParts),
        field("time_back_parts", &Target::timeBackParts),
        field("time_upper_lift_parts", &Target::timeUpperLiftParts),
        field("time_lower_lift_parts", &Target::timeLowerLiftParts),
        field("time_abrv_lift_parts", &Target::timeAbrvLiftParts),
        field("time_sig_do_abrv_common", &Target::timeSigDoAbrvCommon),
        field("time_sig_do_abrv_cut", &Target::timeSigDoAbrvCut),
        field("num_composite_decimal_places", &Target::numCompositeDecimalPlaces),
        field("cautionary_time_changes", &Target::cautionaryTimeChanges),
        field("time_lower_lift", &Target::timeLowerLift),
        field("time_abrv_lift", &Target::timeAbrvLift));
    for (const auto* member : {"timeUpperLift", "timeFront", "timeBack", "timeFrontParts",
             "timeBackParts", "timeUpperLiftParts", "timeLowerLiftParts", "timeAbrvLiftParts",
             "timeSigDoAbrvCommon", "timeSigDoAbrvCut", "numCompositeDecimalPlaces",
             "cautionaryTimeChanges", "timeLowerLift", "timeAbrvLift"}) {
        result.asObject().emplace(
            std::string("origin_") + member, fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_CLASS("options", "time_signature_options", observeTimeSignatureOptions, nullptr);

} // namespace
