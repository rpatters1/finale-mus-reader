// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/support/legacy_mapping.h"

namespace finale_mus_reader {
namespace details {

/// @brief Recovers staff and system baseline displacements.
void importBaselines(const ImportContext& context);

/// @brief Recovers primary downstem beam adjustments attached to entries.
void importBeamAlterationsDownStem(const ImportContext& context);

/// @brief Recovers primary upstem beam adjustments attached to entries.
void importBeamAlterationsUpStem(const ImportContext& context);

/// @brief Recovers downstem beam extensions attached to entries.
void importBeamExtensionDownStem(const ImportContext& context);

/// @brief Recovers upstem beam extensions attached to entries.
void importBeamExtensionUpStem(const ImportContext& context);
void importBeamStubDirection(const ImportContext& context);

/// @brief Recovers center adjustments for smart shapes spanning measures.
void importCenterShapes(const ImportContext& context);

/// @brief Recovers per-clef flat octave placements for custom key signatures.
void importClefOctaveFlats(const ImportContext& context);

/// @brief Recovers per-clef sharp octave placements for custom key signatures.
void importClefOctaveSharps(const ImportContext& context);

/// @brief Recovers source fretboard diagrams.
void importFretboardDiagrams(const ImportContext& context);

/// @brief Recovers GFrameHold objects and the earlier alternate-notation behavior.
void importGFrameHolds(const ImportContext& context);

/// @brief Recovers per-staff independent key and time signatures.
void importIndependentStaffDetails(const ImportContext& context);

/// @brief Recovers accidental strings for custom key symbol lists.
void importKeySymbolListElements(const ImportContext& context);

/// @brief Recovers graphics anchored to a staff and measure.
void importMeasureGraphicAssignments(const ImportContext& context);

/// @brief Recovers text blocks anchored to a staff and measure.
void importMeasureTextAssigns(const ImportContext& context);

/// @brief Recovers percussion-note type assignments for individual notes.
void importPercussionNoteCodes(const ImportContext& context);

/// @brief Recovers secondary downstem beam adjustments attached to entries.
void importSecondaryBeamAlterationsDownStem(const ImportContext& context);

/// @brief Recovers secondary upstem beam adjustments attached to entries.
void importSecondaryBeamAlterationsUpStem(const ImportContext& context);
void importSecondaryBeamBreak(const ImportContext& context);

/// @brief Recovers SmartShape assignments attached to entries.
void importSmartShapeEntryAssigns(const ImportContext& context);

/// @brief Recovers staff groups and their measure ranges.
void importStaffGroups(const ImportContext& context);

/// @brief Recovers per-staff size overrides and updates retained staff systems.
void importStaffSizes(const ImportContext& context);

/// @brief Recovers unbeamed entry stem adjustments.
void importStemAlterations(const ImportContext& context);

/// @brief Recovers entry stem adjustments under beams.
void importStemAlterationsUnderBeam(const ImportContext& context);

} // namespace details
} // namespace finale_mus_reader
