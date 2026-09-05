// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/classification_rules.h"
#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeMultimeasureRestOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::MultimeasureRestOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    // A shape comparator that names no shape leaves the H-bar undrawable, and comparator zero
    // means no shape rather than a missing one.
    const bool danglingShape = options->shapeDef != 0
        && !ctx.document->getOthers()->get<musx::dom::others::ShapeDef>(
            musx::dom::SCORE_PARTID, options->shapeDef);
    return observe(*options, ctx,
        field("meas_width", &Target::measWidth), field("num_adj_y", &Target::numAdjY),
        field("shape_def", &Target::shapeDef), field("num_start", &Target::numStart),
        field("use_syms_threshold", &Target::useSymsThreshold),
        field("sym_spacing", &Target::symSpacing), field("num_adj_x", &Target::numAdjX),
        field("start_adjust", &Target::startAdjust), field("end_adjust", &Target::endAdjust),
        field("use_symbols", &Target::useSymbols),
        field("no_horizontal_stretch", &Target::noHorizontalStretch),
        field("auto_update_mm_rests", &Target::autoUpdateMmRests),
        field("dangling_shape", [danglingShape](const Target&) { return danglingShape; }),
        originField<Target>("origin_measWidth", "measWidth"),
        originField<Target>("origin_numAdjY", "numAdjY"),
        originField<Target>("origin_shapeDef", "shapeDef"),
        originField<Target>("origin_numStart", "numStart"),
        originField<Target>("origin_useSymsThreshold", "useSymsThreshold"),
        originField<Target>("origin_symSpacing", "symSpacing"),
        originField<Target>("origin_numAdjX", "numAdjX"),
        originField<Target>("origin_startAdjust", "startAdjust"),
        originField<Target>("origin_endAdjust", "endAdjust"),
        originField<Target>("origin_useSymbols", "useSymbols"),
        originField<Target>("origin_noHorizontalStretch", "noHorizontalStretch"),
        originField<Target>("origin_autoUpdateMmRests", "autoUpdateMmRests"));
}

COVERAGE_CLASS("options", "mmrest_options", observeMultimeasureRestOptions,
    classifyMultimeasureRestOptionsDifference);

} // namespace
