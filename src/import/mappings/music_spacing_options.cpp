// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/mappings/tables.h"

#include <iterator>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace mapping {
namespace {

using Target = musx::dom::options::MusicSpacingOptions;

// Verified against the controlled Finale 2002-2005 MUS/ETF pairs: changing one spacing
// value at a time moves exactly these words.
//
// The same logical options object also draws fields from selector 41, and the remaining
// words of selector 94 carry the spacing flags and the reference duration/width pairs.
// Those are distilled in research/data/legacy_option_mappings.csv but are not yet
// verified against a fixture, so they are not promoted here.
const FieldMapping spacingFields[] = {
    MUS_WORD(Target, "94", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 1, minWidth),
    MUS_WORD(Target, "94", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 2, maxWidth),
    MUS_WORD(Target, "94", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 3, minDistance),
    MUS_WORD(Target, "94", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 4, minDistTiedNotes),
};

} // namespace

const MappingTable& musicSpacingOptionsTable()
{
    static const MappingTable table{
        "options.musicSpacing",
        EpochMask::FixedRow,
        VersionRange{},
        TargetKind::OptionsSingleton,
        &enumerateOptionsTarget<Target>,
        spacingFields,
        std::size(spacingFields)};
    return table;
}

} // namespace mapping
} // namespace finale_mus_reader
