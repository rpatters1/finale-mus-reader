// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/support/source_gate.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyAugmentationDotDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    const bool predatesFinale35 =
        context.epoch == finale_mus_reader::FormatEpoch::UncompressedLegacy &&
        sourcePredatesVersion(context.epoch, context.sourceVersion,
            finale_mus_reader::FormatEpoch::UncompressedLegacy,
            finale_mus_reader::versions::finale3_5);
    if (context.category == Differs && predatesFinale35 &&
        context.origin == "finale27-default" &&
        context.path == "augmentation_dot_options.adj_multiple_voices" &&
        context.sourceValue.isBool() && context.sourceValue.asBool() &&
        context.companionValue.isBool() && !context.companionValue.asBool()) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

Value observeAugmentationDotOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::AugmentationDotOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) {
        return {};
    }
    return observe(
        *options, ctx, field("dot_up_flag_offset", &Target::dotUpFlagOffset),
        field("dot_offset", &Target::dotOffset), field("adj_multiple_voices", &Target::adjMultipleVoices),
        field("dot_note_offset", &Target::dotNoteOffset), field("dot_lift", &Target::dotLift),
        field("use_legacy_flipped_stem_positioning", &Target::useLegacyFlippedStemPositioning),
        originField<Target>("origin_dotUpFlagOffset", "dotUpFlagOffset"),
        originField<Target>("origin_dotOffset", "dotOffset"),
        originField<Target>("origin_adjMultipleVoices", "adjMultipleVoices"),
        originField<Target>("origin_dotNoteOffset", "dotNoteOffset"),
        originField<Target>("origin_dotLift", "dotLift"),
        originField<Target>("origin_useLegacyFlippedStemPositioning",
            "useLegacyFlippedStemPositioning"));
}

COVERAGE_CLASS("options", "augmentation_dot_options", observeAugmentationDotOptions,
    classifyAugmentationDotDifference);

} // namespace
