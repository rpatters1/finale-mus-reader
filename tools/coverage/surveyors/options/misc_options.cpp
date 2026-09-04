// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyMiscOptionsDifference(const DifferenceContext &context) {
  using enum DifferenceCategory;
  if (context.category == Differs && context.origin == "finale27-default" &&
      (context.path == "misc_options.show_repeats_for_parts" ||
       context.path == "misc_options.show_active_layer_only" ||
       context.path == "misc_options.shape_designer_dash_length" ||
       context.path == "misc_options.shape_designer_dash_space" ||
       context.path == "misc_options.rest_width_adjust" ||
       context.path == "misc_options.dbl_whole_vert_adjust")) {
    return DifferenceClassification::DifferentDefaults;
  }
  if (context.category == Differs &&
      context.path == "misc_options.consolidate_rests_across_layers" &&
      context.origin == "legacy-behavior" && context.sourceValue.isBool() &&
      !context.sourceValue.asBool() && context.companionValue.isBool() &&
      context.companionValue.asBool() && context.sourceVersion &&
      sourceIsBeta(context.sourceVersion)) {
    return DifferenceClassification::BetaDiscrepancy;
  }
  return std::nullopt;
}

Value observeMiscOptions(const SurveyContext &ctx) {
  using Target = musx::dom::options::MiscOptions;
  const auto options = ctx.document->getOptions()->get<Target>();
  if (!options)
    return {};
  auto result = observe(
      *options, ctx,
      field("show_repeats_for_parts", &Target::showRepeatsForParts),
      field("pickup_value", &Target::pickupValue),
      field("keep_written_octave_in_concert_pitch",
            &Target::keepWrittenOctaveInConcertPitch),
      field("show_active_layer_only", &Target::showActiveLayerOnly),
      field("consolidate_rests_across_layers",
            &Target::consolidateRestsAcrossLayers),
      field("shape_designer_dash_length", &Target::shapeDesignerDashLength),
      field("shape_designer_dash_space", &Target::shapeDesignerDashSpace),
      field("rest_width_adjust", &Target::restWidthAdjust),
      field("dbl_whole_vert_adjust", &Target::dblWholeVertAdjust),
      field("align_measure_numbers_with_barlines",
            &Target::alignMeasureNumbersWithBarlines));
  for (const auto *member :
       {"showRepeatsForParts", "pickupValue", "keepWrittenOctaveInConcertPitch",
        "showActiveLayerOnly", "consolidateRestsAcrossLayers",
        "shapeDesignerDashLength", "shapeDesignerDashSpace", "restWidthAdjust",
        "dblWholeVertAdjust", "alignMeasureNumbersWithBarlines"}) {
    result.asObject().emplace(std::string("origin_") + member,
                              fieldOrigin<Target>(ctx, member));
  }
  return result;
}

COVERAGE_CLASS("options", "misc_options", observeMiscOptions,
               classifyMiscOptionsDifference);

} // namespace
