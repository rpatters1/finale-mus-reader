// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <ostream>
#include <string>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeMusicSpacingOptions(std::ostream& out, const SurveyContext& ctx)
{
    const auto options = ctx.document->getOptions()->get<musx::dom::options::MusicSpacingOptions>();
    if (!options) {
        out << "null";
        return;
    }
    out << '{'
        << "\"min_width\":" << options->minWidth
        << ",\"max_width\":" << options->maxWidth
        << ",\"min_distance\":" << options->minDistance
        << ",\"min_dist_tied_notes\":" << options->minDistTiedNotes;
    for (const auto* member : {"minWidth", "maxWidth", "minDistance", "minDistTiedNotes"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(ctx.fields.originOf(std::string("options.musicSpacing.") + member));
    }
    out << '}';
}

COVERAGE_SURVEYOR("spacing_options", writeMusicSpacingOptions);

} // namespace
