// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/support/legacy_mapping.h"

namespace finale_mus_reader {
namespace details {

/// @brief Recovers staff and system baseline displacements.
void importBaselines(const ImportContext& context);

/// @brief Recovers per-clef flat octave placements for custom key signatures.
void importClefOctaveFlats(const ImportContext& context);

/// @brief Recovers per-clef sharp octave placements for custom key signatures.
void importClefOctaveSharps(const ImportContext& context);

/// @brief Recovers source fretboard diagrams.
void importFretboardDiagrams(const ImportContext& context);

/// @brief Recovers GFrameHold behavior represented elsewhere by modern Finale.
/// @details This entry point does not construct GFrameHold objects. It currently reads only the
/// alternate-notation bits from supported pre-Finale-2000 record shapes.
void importGFrameHolds(const ImportContext& context);

/// @brief Recovers accidental strings for custom key symbol lists.
void importKeySymbolListElements(const ImportContext& context);

/// @brief Recovers graphics anchored to a staff and measure.
void importMeasureGraphicAssignments(const ImportContext& context);

/// @brief Recovers percussion-note type assignments for individual notes.
void importPercussionNoteCodes(const ImportContext& context);

/// @brief Recovers staff groups and their measure ranges.
void importStaffGroups(const ImportContext& context);

/// @brief Recovers per-staff size overrides and updates retained staff systems.
void importStaffSizes(const ImportContext& context);

} // namespace details
} // namespace finale_mus_reader
