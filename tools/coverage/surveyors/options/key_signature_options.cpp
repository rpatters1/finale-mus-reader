// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyKeySignatureOptionsDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category == Differs && context.origin == "legacy-behavior" &&
        context.path ==
            "key_signature_options.do_key_cancel_between_sharps_flats" &&
        context.sourceValue.isBool() && context.sourceValue.asBool() &&
        context.companionValue.isBool() && !context.companionValue.asBool()) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

Value observeKeySignatureOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::KeySignatureOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("do_key_cancel", &Target::doKeyCancel),
        field("do_c_start", &Target::doCStart),
        field("redisplay_on_mode_change", &Target::redisplayOnModeChange),
        field("key_front", &Target::keyFront),
        field("key_mid", &Target::keyMid),
        field("key_back", &Target::keyBack),
        field("acci_add", &Target::acciAdd),
        field("show_key_first_system_only", &Target::showKeyFirstSystemOnly),
        field("key_time_separ", &Target::keyTimeSepar),
        field("simplify_key_hold_octave", &Target::simplifyKeyHoldOctave),
        field("cautionary_key_changes", &Target::cautionaryKeyChanges),
        field("do_key_cancel_between_sharps_flats",
            &Target::doKeyCancelBetweenSharpsFlats));
    for (const auto* member : {"doKeyCancel", "doCStart", "redisplayOnModeChange",
             "keyFront", "keyMid", "keyBack", "acciAdd", "showKeyFirstSystemOnly",
             "keyTimeSepar", "simplifyKeyHoldOctave", "cautionaryKeyChanges",
             "doKeyCancelBetweenSharpsFlats"}) {
        result.asObject().emplace(
            std::string("origin_") + member, fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_CLASS("options", "key_signature_options", observeKeySignatureOptions,
    classifyKeySignatureOptionsDifference);

} // namespace
