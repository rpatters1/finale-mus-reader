// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/mappings/tables.h"

#include <iterator>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace mapping {
namespace {

using Target = musx::dom::others::LayerAttributes;

// Layer attributes are ordinary other records rather than synthetic preferences: the
// record comparator is the layer number, so the table binds each record to the seeded
// object of the same comparator and the row's own selector is unused.
//
// Verified against the controlled fixtures, where each layer's rest offset moves
// independently.
const FieldMapping layerFields[] = {
    MUS_WORD(Target, "LA", CMPER_FROM_TARGET, /*incidence*/ 0, /*slot*/ 0, restOffset),
};

} // namespace

const MappingTable& layerAttributesTable()
{
    static const MappingTable table{
        "others.layerAtts",
        EpochMask::FixedRow,
        VersionRange{},
        TargetKind::OthersByCmper,
        &enumerateOthersTargets<Target>,
        layerFields,
        std::size(layerFields)};
    return table;
}

} // namespace mapping
} // namespace finale_mus_reader
