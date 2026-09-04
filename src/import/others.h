// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/support/legacy_mapping.h"

namespace finale_mus_reader {
namespace others {

// One importer per musxdom others class, and the registry calls nothing else. The tables a
// class needs, and which epoch each of them covers, stay inside that class's own
// translation unit. Accessors are used instead of static registration so a static archive
// cannot discard an importer nothing else references.

/// @brief Recovers the positioned elements that form chord suffix definitions.
void importChordSuffixElements(const ImportContext& context);

/// @brief Recovers chord-suffix playback intervals.
void importChordSuffixPlayback(const ImportContext& context);

/// @brief Recovers the others::FontDefinition pool, whose four layouts span every epoch.
void importFontDefinitions(const ImportContext& context);

/// @brief Recovers source fret instrument definitions.
void importFretInstruments(const ImportContext& context);

/// @brief Recovers source fretboard groups.
void importFretboardGroups(const ImportContext& context);

/// @brief Recovers source fretboard styles.
void importFretboardStyles(const ImportContext& context);

/// @brief Recovers every others::LayerAttributes object the source stores, and supplies the
/// era's own behavior for the layers it does not.
void importLayerAttributes(const ImportContext& context);

/// @brief Recovers page graphic assignments and their embedded-graphic references.
void importPageGraphicAssignments(const ImportContext& context);

/// @brief Recovers PartDefinition objects, and supplies the score part every era has.
void importPartDefinitions(const ImportContext& context);

/// @brief Recovers the score- and part-specific global view settings.
void importPartGlobals(const ImportContext& context);

/// @brief Recovers ShapeDef objects and their owned instruction and data lists.
void importShapeDefinitions(const ImportContext& context);

/// @brief Recovers graphic assignments referenced by ShapeDef instructions.
void importShapeGraphicAssignments(const ImportContext& context);

/// @brief Recovers SmartShapeCustomLine objects.
void importSmartShapeCustomLines(const ImportContext& context);

/// @brief Recovers category and repeat staff lists, supplying absent canned
/// category lists.
void importStaffLists(const ImportContext& context);

/// @brief Recovers TextBlock objects and Coda-banner block-text structure.
void importTextBlocks(const ImportContext& context);

} // namespace others
} // namespace finale_mus_reader
