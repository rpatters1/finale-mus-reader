// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/legacy_mapping.h"

namespace finale_mus_reader {
namespace options {

[[nodiscard]] const MappingTable& musicSpacingOptionsTable();

/// @brief Recovers source FontOptions and completes the modern 45-type collection.
/// @details Physical tuples are interpreted through versioned semantic mappings. Types absent
/// from the source are copied from the separate reference document with safe font-id remapping.
void captureFontOptions(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument, ImportReport& report);

/// @brief Reconciles recovered nonzero FontOptions ids whose MUS FontDefinition is absent.
/// @details Selects the reference face by semantic type, reuses an equal nonzero target
/// face by normalized name, or clones it at the next target comparator when necessary.
void repairMissingRecoveredFontDefinitions(const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument,
    const std::shared_ptr<musx::dom::options::FontOptions>& target, ImportReport& report);

} // namespace options
} // namespace finale_mus_reader
