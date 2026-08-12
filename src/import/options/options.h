// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/legacy_mapping.h"

namespace finale_mus_reader {
namespace options {

[[nodiscard]] const MappingTable& musicSpacingOptionsTable();

/// @brief The scalar ClefOptions fields, overlaid onto the object @ref captureClefOptions made.
/// @details Three tables because the eras disagree about what these selectors hold, not
/// merely about how they are framed. They share a report prefix and layer as one group.
[[nodiscard]] const MappingTable& clefOptionsTable();

/// @brief The Coda-banner subset: the five scalars that era stores where later ones do.
[[nodiscard]] const MappingTable& earlyClefOptionsTable();

/// @brief The Finale 2007 and later scalars, as class records addressed by byte offset.
[[nodiscard]] const MappingTable& classClefOptionsTable();

/// @brief Checks recovered ClefOptions values that only make sense once the tables have run.
/// @details Runs after @ref clefOptionsTable, because the default clef index is a mapped
/// scalar and the collection it indexes is built by @ref captureClefOptions before that.
void validateClefOptions(const musx::dom::DocumentPtr& document, ImportReport& report);

/// @brief Recovers source clef definitions and completes the modern 18-definition collection.
/// @details The collection is rebuilt rather than seeded: its shape and font comparators
/// belong to the baseline's own tables. Definitions the source does not store are copied
/// from the reference document, which is what Finale's own upgrade does. Runs before the
/// mapping tables so that @ref clefOptionsTable has an object to overlay.
void captureClefOptions(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument, ImportReport& report);

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
