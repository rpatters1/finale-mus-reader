// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/support/legacy_mapping.h"

namespace finale_mus_reader {
namespace details {

/// @brief Recovers per-clef flat octave placements for custom key signatures.
void importClefOctaveFlats(const ImportContext& context);

/// @brief Recovers per-clef sharp octave placements for custom key signatures.
void importClefOctaveSharps(const ImportContext& context);

/// @brief Recovers source fretboard diagrams.
void importFretboardDiagrams(const ImportContext& context);

/// @brief Recovers accidental strings for custom key symbol lists.
void importKeySymbolListElements(const ImportContext& context);

/// @brief Recovers graphics anchored to a staff and measure.
void importMeasureGraphicAssignments(const ImportContext& context);

} // namespace details
} // namespace finale_mus_reader
