// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/legacy_mapping.h"

namespace finale_mus_reader {
namespace mapping {

// Mapping tables are reached through accessors rather than registering themselves from
// static initializers. This library is a static archive, and a linker is free to drop an
// object file whose only contribution is a static initializer.
//
// Each table corresponds to one musxdom class. Add a new table by writing its file,
// declaring its accessor here, and listing it in registeredTables() in
// src/import/legacy_mapping.cpp.

[[nodiscard]] const MappingTable& musicSpacingOptionsTable();
[[nodiscard]] const MappingTable& layerAttributesTable();
[[nodiscard]] const MappingTable& fontDefinitionsTable();
[[nodiscard]] const MappingTable& earlyFontDefinitionsTable();
[[nodiscard]] const MappingTable& classFontDefinitionsTable();

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

} // namespace mapping
} // namespace finale_mus_reader
