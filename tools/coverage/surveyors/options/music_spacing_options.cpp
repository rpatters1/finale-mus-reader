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
        field("avoid_col_notes", &Target::avoidColNotes),
        field("avoid_col_lyrics", &Target::avoidColLyrics),
        field("avoid_col_chords", &Target::avoidColChords),
        field("avoid_col_artics", &Target::avoidColArtics),
        field("avoid_col_clefs", &Target::avoidColClefs),
        field("avoid_col_seconds", &Target::avoidColSeconds),
        field("avoid_col_stems", &Target::avoidColStems),
        field("avoid_col_unisons", &Target::avoidColUnisons),
        field("avoid_col_ledgers", &Target::avoidColLedgers),
        field("manual_positioning", &Target::manualPositioning),
        field("ignore_hidden", &Target::ignoreHidden),
        field("interpolate_allotments", &Target::interpolateAllotments),
        field("use_printer", &Target::usePrinter),
        field("use_allottment_tables", &Target::useAllottmentTables),
        field("reference_duration", &Target::referenceDuration),
        field("reference_width", &Target::referenceWidth),
        field("scaling_factor", &Target::scalingFactor),
        field("default_allotment", &Target::defaultAllotment),
        field("min_dist_grace", &Target::minDistGrace),
        field("grace_note_spacing", &Target::graceNoteSpacing),
        field("mus_front", &Target::musFront), field("mus_back", &Target::musBack),
        originField<Target>("origin_minWidth", "minWidth"),
        originField<Target>("origin_maxWidth", "maxWidth"),
        originField<Target>("origin_minDistance", "minDistance"),
        originField<Target>("origin_minDistTiedNotes", "minDistTiedNotes"),
        originField<Target>("origin_avoidColNotes", "avoidColNotes"),
        originField<Target>("origin_avoidColLyrics", "avoidColLyrics"),
        originField<Target>("origin_avoidColChords", "avoidColChords"),
        originField<Target>("origin_avoidColArtics", "avoidColArtics"),
        originField<Target>("origin_avoidColClefs", "avoidColClefs"),
        originField<Target>("origin_avoidColSeconds", "avoidColSeconds"),
        originField<Target>("origin_avoidColStems", "avoidColStems"),
        originField<Target>("origin_avoidColUnisons", "avoidColUnisons"),
        originField<Target>("origin_avoidColLedgers", "avoidColLedgers"),
        originField<Target>("origin_manualPositioning", "manualPositioning"),
        originField<Target>("origin_ignoreHidden", "ignoreHidden"),
        originField<Target>("origin_interpolateAllotments", "interpolateAllotments"),
        originField<Target>("origin_usePrinter", "usePrinter"),
        originField<Target>("origin_useAllottmentTables", "useAllottmentTables"),
        originField<Target>("origin_referenceDuration", "referenceDuration"),
        originField<Target>("origin_referenceWidth", "referenceWidth"),
        originField<Target>("origin_scalingFactor", "scalingFactor"),
        originField<Target>("origin_defaultAllotment", "defaultAllotment"),
        originField<Target>("origin_minDistGrace", "minDistGrace"),
        originField<Target>("origin_graceNoteSpacing", "graceNoteSpacing"),
        originField<Target>("origin_musFront", "musFront"),
        originField<Target>("origin_musBack", "musBack"));
}

COVERAGE_SURVEYOR("options", "music_spacing_options", observeMusicSpacingOptions);

} // namespace
