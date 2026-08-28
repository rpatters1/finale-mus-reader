// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeAccidentalOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::AccidentalOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("minimum_overlap", &Target::minOverlap),
        field("multi_character_space", &Target::multiCharSpace),
        field("cross_layer_positioning", &Target::crossLayerPositioning),
        field("start_measure_separation", &Target::startMeasureSepar),
        field("accidental_note_space", &Target::acciNoteSpace),
        field("accidental_accidental_space", &Target::acciAcciSpace));
    result.asObject().emplace(
        "origin_minimumOverlap", fieldOrigin<Target>(ctx, "minOverlap"));
    result.asObject().emplace(
        "origin_multiCharacterSpace", fieldOrigin<Target>(ctx, "multiCharSpace"));
    result.asObject().emplace("origin_crossLayerPositioning",
        fieldOrigin<Target>(ctx, "crossLayerPositioning"));
    result.asObject().emplace("origin_startMeasureSeparation",
        fieldOrigin<Target>(ctx, "startMeasureSepar"));
    result.asObject().emplace(
        "origin_accidentalNoteSpace", fieldOrigin<Target>(ctx, "acciNoteSpace"));
    result.asObject().emplace("origin_accidentalAccidentalSpace",
        fieldOrigin<Target>(ctx, "acciAcciSpace"));
    return result;
}

COVERAGE_SURVEYOR("options", "accidental_options", observeAccidentalOptions);

} // namespace
