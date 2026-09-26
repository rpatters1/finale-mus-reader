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

/// @brief Recovers flat accidental amounts for custom key signatures.
void importAcciAmountFlats(const ImportContext& context);

/// @brief Recovers sharp accidental amounts for custom key signatures.
void importAcciAmountSharps(const ImportContext& context);

/// @brief Recovers flat accidental ordering for custom key signatures.
void importAcciOrderFlats(const ImportContext& context);

/// @brief Recovers sharp accidental ordering for custom key signatures.
void importAcciOrderSharps(const ImportContext& context);

/// @brief Recovers the control header and positioned elements of every stored measure beat chart.
void importBeatChartElements(const ImportContext& context);

/// @brief Recovers the text of the document's bookmarks.
/// @details musxdom has no class for the bookmark object itself, so only its name is recovered,
/// as a `texts::BookmarkText`. The eras that pool that text are read by the text-pool importer.
void importBookmarks(const ImportContext& context);

/// @brief Recovers the positioned elements that form chord suffix definitions.
void importChordSuffixElements(const ImportContext& context);

/// @brief Recovers chord-suffix playback intervals.
void importChordSuffixPlayback(const ImportContext& context);

/// @brief Recovers the clef lists that frames name for their barline and mid-measure clefs.
void importClefLists(const ImportContext& context);

/// @brief Recovers the percussion-map association for each percussion staff.
void importDrumStaff(const ImportContext& context);

/// @brief Imports graphic file locator records.
void importFilePath(const ImportContext& context);

/// @brief Recovers the others::FontDefinition pool, whose four layouts span every epoch.
void importFontDefinitions(const ImportContext& context);

/// @brief Recovers source fret instrument definitions.
void importFretInstruments(const ImportContext& context);

/// @brief Recovers source fretboard groups.
void importFretboardGroups(const ImportContext& context);

/// @brief Recovers source fretboard styles.
void importFretboardStyles(const ImportContext& context);

/// @brief Recovers custom key attributes.
void importKeyAttributes(const ImportContext& context);

/// @brief Recovers custom key formats.
void importKeyFormats(const ImportContext& context);

/// @brief Recovers custom key step maps.
void importKeyMapArrays(const ImportContext& context);

/// @brief Recovers every others::LayerAttributes object the source stores, and supplies the
/// era's own behavior for the layers it does not.
void importLayerAttributes(const ImportContext& context);

/// @brief Recovers marking categories and their names, supplying the canned categories only
/// when the source predates Finale 2009.
void importMarkingCategories(const ImportContext& context);

/// @brief Recovers every measure the source stores, score and unlinked part alike.
void importMeasures(const ImportContext& context);

/// @brief Recovers the part-specific staff-group reference for each multi-staff instrument.
void importMultiStaffGroupIds(const ImportContext& context);

/// @brief Recovers the staff membership of each multi-staff instrument.
void importMultiStaffInstrumentGroups(const ImportContext& context);

/// @brief Recovers abbreviated-name positioning overrides for staves.
void importNamePositionAbbreviated(const ImportContext& context);

/// @brief Recovers full-name positioning overrides for staves.
void importNamePositionFull(const ImportContext& context);

/// @brief Recovers abbreviated-name positioning overrides for staff styles.
void importNamePositionStyleAbbreviated(const ImportContext& context);

/// @brief Recovers full-name positioning overrides for staff styles.
void importNamePositionStyleFull(const ImportContext& context);

/// @brief Recovers page graphic assignments and their embedded-graphic references.
void importPageGraphicAssignments(const ImportContext& context);

/// @brief Recovers page-layout objects and their first-system references.
void importPages(const ImportContext& context);

/// @brief Recovers PartDefinition objects, and supplies the score part every era has.
void importPartDefinitions(const ImportContext& context);

/// @brief Recovers the score- and part-specific global view settings.
void importPartGlobals(const ImportContext& context);
void importPartVoicing(const ImportContext& context);

/// @brief Recovers percussion-map note identities, staff positions, and noteheads.
void importPercussionNoteInfo(const ImportContext& context);

/// @brief Recovers ShapeDef objects and their owned instruction and data lists.
void importShapeDefinitions(const ImportContext& context);

/// @brief Recovers graphic assignments referenced by ShapeDef instructions.
void importShapeGraphicAssignments(const ImportContext& context);

/// @brief Recovers SmartShapeCustomLine objects.
void importSmartShapeCustomLines(const ImportContext& context);

/// @brief Recovers stored split positions for measures.
void importSplitMeasures(const ImportContext& context);

/// @brief Recovers backward repeat assignments attached to measures.
void importRepeatBacks(const ImportContext& context);

/// @brief Recovers source Staff objects and the parallel names used before Finale 3.7.
void importStaff(const ImportContext& context);

/// @brief Recovers category and repeat staff lists, supplying absent canned
/// category lists.
void importStaffLists(const ImportContext& context);

/// @brief Recovers source StaffStyleAssign objects.
void importStaffStyleAssignments(const ImportContext& context);

/// @brief Recovers source StaffStyle objects.
void importStaffStyles(const ImportContext& context);

/// @brief Recovers source StaffSystem objects from legacy system-layout records.
void importStaffSystems(const ImportContext& context);

/// @brief Recovers, normalizes, and completes the StaffUsed lists used by layout.
void importStaffUsed(const ImportContext& context);

/// @brief Recovers locked measure spans from fixed-row other records.
void importSystemLocks(const ImportContext& context);

/// @brief Recovers measure tempo tool changes in incidence order.
void importTempoChanges(const ImportContext& context);

/// @brief Recovers composite time signature lower lists.
void importTimeCompositeLowers(const ImportContext& context);

/// @brief Recovers composite time signature upper lists.
void importTimeCompositeUppers(const ImportContext& context);

/// @brief Recovers TextBlock objects and Coda-banner block-text structure.
void importTextBlocks(const ImportContext& context);

/// @brief Recovers text expression definitions.
void importTextExpressionDefs(const ImportContext& context);

/// @brief Recovers flat tonal-center tables for custom key signatures.
void importTonalCenterFlats(const ImportContext& context);

/// @brief Recovers sharp tonal-center tables for custom key signatures.
void importTonalCenterSharps(const ImportContext& context);

} // namespace others
} // namespace finale_mus_reader
