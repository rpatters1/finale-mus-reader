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
classifyAlternateNotationDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    const bool predatesFinale97 = sourcePredatesVersion(context.epoch, context.sourceVersion,
        finale_mus_reader::FormatEpoch::UncompressedLegacy,
        finale_mus_reader::versions::finale97);
    if (context.category == Differs && predatesFinale97 &&
        context.origin == "legacy-mus-adjusted" &&
        comparisonPathStartsWith(context.path, "alternate_notation_options.")) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

Value observeAlternateNotationOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::AlternateNotationOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("half_slash_lift", &Target::halfSlashLift),
        field("whole_slash_lift", &Target::wholeSlashLift),
        field("double_whole_slash_lift", &Target::dWholeSlashLift),
        field("half_slash_stem_lift", &Target::halfSlashStemLift),
        field("quarter_slash_stem_lift", &Target::quartSlashStemLift),
        field("quarter_slash_lift", &Target::quartSlashLift),
        field("two_measure_number_lift", &Target::twoMeasNumLift));
    result.asObject().emplace(
        "origin_halfSlashLift", fieldOrigin<Target>(ctx, "halfSlashLift"));
    result.asObject().emplace(
        "origin_wholeSlashLift", fieldOrigin<Target>(ctx, "wholeSlashLift"));
    result.asObject().emplace(
        "origin_doubleWholeSlashLift", fieldOrigin<Target>(ctx, "dWholeSlashLift"));
    result.asObject().emplace(
        "origin_halfSlashStemLift", fieldOrigin<Target>(ctx, "halfSlashStemLift"));
    result.asObject().emplace(
        "origin_quarterSlashStemLift", fieldOrigin<Target>(ctx, "quartSlashStemLift"));
    result.asObject().emplace(
        "origin_quarterSlashLift", fieldOrigin<Target>(ctx, "quartSlashLift"));
    result.asObject().emplace(
        "origin_twoMeasureNumberLift", fieldOrigin<Target>(ctx, "twoMeasNumLift"));
    return result;
}

COVERAGE_CLASS("options", "alternate_notation_options", observeAlternateNotationOptions,
               classifyAlternateNotationDifference);

} // namespace
