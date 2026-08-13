// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others/others.h"

#include <iterator>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using LayerAttributesTarget = musx::dom::others::LayerAttributes;

// Layer attributes are ordinary other records rather than synthetic preferences: the
// record comparator is the layer number, so the table binds each record to the seeded
// object of the same comparator and the row's own selector is unused.
//
// Verified against the controlled fixtures, where each layer's rest offset moves
// independently.
const FieldMapping layerFields[] = {
    MUS_WORD(LayerAttributesTarget, "LA", CMPER_FROM_TARGET, /*incidence*/ 0, /*slot*/ 0, restOffset),
};

const MappingTable& layerAttributesTable()
{
    static const MappingTable table{
        .reportPrefix = "others.layerAtts",
        .epochs = EpochMask::FixedRow,
        .targetKind = TargetKind::OthersByCmper,
        .enumerateTargets = &enumerateOthersTargets<LayerAttributesTarget>,
        .fields = layerFields,
        .fieldCount = std::size(layerFields)};
    return table;
}

} // namespace

void importLayerAttributes(const ImportContext& context)
{
    applyMappingTables({&layerAttributesTable()},
        context.index, context.profile, context.document, context.report);
}

} // namespace others
} // namespace finale_mus_reader
