// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

using LayerAttributesSurveyTarget = musx::dom::others::LayerAttributes;

// Every origin leaf names its C++ member exactly, so the comparison model can pair the two
// halves by spelling. Building them through one helper keeps that pairing from drifting a
// member at a time.
auto layerAttributeOrigin(const char* member)
{
    return [member](const LayerAttributesSurveyTarget& value, const SurveyContext& context) {
        // Keyed by the instance's own identity rather than by comparator alone: the source may
        // own layer objects the baseline never seeded, including part-scoped ones, and a lookup
        // that assumed the score part would report those as unmapped.
        return fieldOrigin<LayerAttributesSurveyTarget>(context, member, value);
    };
}

Value observeLayerAttributes(const SurveyContext& ctx)
{
    using Target = LayerAttributesSurveyTarget;
    Value::Array result;
    for (const auto& layer : sourceInstances<Target>(ctx)) {
        result.push_back(observe(*layer, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("rest_offset", &Target::restOffset),
            field("freez_ties_to_stems", &Target::freezTiesToStems),
            field("only_if_other_layers_have_notes", &Target::onlyIfOtherLayersHaveNotes),
            field("use_rest_offset", &Target::useRestOffset),
            field("freeze_stems_up", &Target::freezeStemsUp),
            field("freeze_layer", &Target::freezeLayer),
            field("playback", &Target::playback),
            field("affect_spacing", &Target::affectSpacing),
            field("ignore_hidden_notes_only", &Target::ignoreHiddenNotesOnly),
            field("ignore_hidden_layers", &Target::ignoreHiddenLayers),
            field("hide_layer", &Target::hideLayer),
            field("origin_restOffset", layerAttributeOrigin("restOffset")),
            field("origin_freezTiesToStems", layerAttributeOrigin("freezTiesToStems")),
            field("origin_onlyIfOtherLayersHaveNotes",
                layerAttributeOrigin("onlyIfOtherLayersHaveNotes")),
            field("origin_useRestOffset", layerAttributeOrigin("useRestOffset")),
            field("origin_freezeStemsUp", layerAttributeOrigin("freezeStemsUp")),
            field("origin_freezeLayer", layerAttributeOrigin("freezeLayer")),
            field("origin_playback", layerAttributeOrigin("playback")),
            field("origin_affectSpacing", layerAttributeOrigin("affectSpacing")),
            field("origin_ignoreHiddenNotesOnly",
                layerAttributeOrigin("ignoreHiddenNotesOnly")),
            field("origin_ignoreHiddenLayers", layerAttributeOrigin("ignoreHiddenLayers")),
            field("origin_hideLayer", layerAttributeOrigin("hideLayer"))));
    }
    return Value(std::move(result));
}

COVERAGE_SURVEYOR("others", "layer_atts", observeLayerAttributes);

} // namespace
