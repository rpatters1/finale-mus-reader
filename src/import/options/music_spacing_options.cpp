// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options/options.h"

#include <iterator>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace options {
namespace {

using MusicSpacingTarget = musx::dom::options::MusicSpacingOptions;

// Verified against the controlled Finale 2002-2005 MUS/ETF pairs: changing one spacing
// value at a time moves exactly these words.
//
// The same logical options object also draws fields from selector 41, and the remaining
// words of selector 94 carry the spacing flags and the reference duration/width pairs.
// Those are distilled in research/data/legacy_option_mappings.csv but are not yet
// verified against a fixture, so they are not promoted here.
const FieldMapping spacingFields[] = {
    MUS_WORD(MusicSpacingTarget, "94", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 1, minWidth),
    MUS_WORD(MusicSpacingTarget, "94", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 2, maxWidth),
    MUS_WORD(MusicSpacingTarget, "94", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 3, minDistance),
    MUS_WORD(MusicSpacingTarget, "94", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 4, minDistTiedNotes),
};

const MappingTable& musicSpacingOptionsTable()
{
    static const MappingTable table{
        .reportPrefix = "options.musicSpacing",
        .epochs = EpochMask::FixedRow,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<MusicSpacingTarget>,
        .fields = spacingFields,
        .fieldCount = std::size(spacingFields)};
    return table;
}

} // namespace

void importMusicSpacingOptions(const ImportContext& context)
{
    applyMappingTables({&musicSpacingOptionsTable()},
        context.index, context.profile, context.document, context.report);
}

} // namespace options
} // namespace finale_mus_reader
