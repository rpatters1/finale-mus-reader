// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeMusicSpacingOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::MusicSpacingOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    return observe(*options, ctx,
        field("min_width", &Target::minWidth), field("max_width", &Target::maxWidth),
        field("min_distance", &Target::minDistance),
        field("min_dist_tied_notes", &Target::minDistTiedNotes),
        originField<Target>("origin_minWidth", "minWidth"),
        originField<Target>("origin_maxWidth", "maxWidth"),
        originField<Target>("origin_minDistance", "minDistance"),
        originField<Target>("origin_minDistTiedNotes", "minDistTiedNotes"));
}

COVERAGE_SURVEYOR("spacing_options", observeMusicSpacingOptions);

} // namespace
