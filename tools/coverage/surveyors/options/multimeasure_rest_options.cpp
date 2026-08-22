// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <ostream>
#include <string>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeMultimeasureRestOptions(std::ostream& out, const SurveyContext& ctx)
{
    const auto options =
        ctx.document->getOptions()->get<musx::dom::options::MultimeasureRestOptions>();
    if (!options) {
        out << "null";
        return;
    }
    // A shape comparator that names no shape leaves the H-bar undrawable, and comparator zero
    // means no shape rather than a missing one.
    const bool danglingShape = options->shapeDef != 0
        && !ctx.document->getOthers()->get<musx::dom::others::ShapeDef>(
            musx::dom::SCORE_PARTID, options->shapeDef);
    out << '{'
        << "\"meas_width\":" << options->measWidth
        << ",\"num_adj_y\":" << options->numAdjY
        << ",\"shape_def\":" << options->shapeDef
        << ",\"num_start\":" << options->numStart
        << ",\"use_syms_threshold\":" << options->useSymsThreshold
        << ",\"sym_spacing\":" << options->symSpacing
        << ",\"num_adj_x\":" << options->numAdjX
        << ",\"start_adjust\":" << options->startAdjust
        << ",\"end_adjust\":" << options->endAdjust
        << ",\"use_symbols\":" << jsonBool(options->useSymbols)
        << ",\"no_horizontal_stretch\":" << jsonBool(options->noHorizontalStretch)
        << ",\"auto_update_mm_rests\":" << jsonBool(options->autoUpdateMmRests)
        << ",\"dangling_shape\":" << jsonBool(danglingShape);
    for (const auto* member : {"measWidth", "numAdjY", "shapeDef", "numStart",
             "useSymsThreshold", "symSpacing", "numAdjX", "startAdjust", "endAdjust",
             "useSymbols", "noHorizontalStretch", "autoUpdateMmRests"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(ctx.fields.originOf(std::string("options.multimeasureRestOptions.") + member));
    }
    out << '}';
}

COVERAGE_SURVEYOR("mmrest_options", writeMultimeasureRestOptions);

} // namespace
