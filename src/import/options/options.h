// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/legacy_mapping.h"

namespace finale_mus_reader {
namespace options {

// One importer per musxdom options class, and the registry calls nothing else. Everything a
// class needs -- how many physical layouts it has, which epochs each covers, whether a
// collection must be captured before its scalars are overlaid -- is decided inside that
// class's own translation unit. The entries below are what the registry lists; the
// declarations after them exist so tests can drive one stage of a class on its own.

/// @brief Recovers options::ClefOptions: its definition collection and its scalars.
void importClefOptions(const ImportContext& context);

/// @brief Recovers options::FontOptions and completes the modern type set.
void importFontOptions(const ImportContext& context);

/// @brief Recovers options::MusicSpacingOptions.
void importMusicSpacingOptions(const ImportContext& context);

/// @brief Recovers options::StemOptions, whose connections are a source-owned collection.
void importStemOptions(const ImportContext& context);

/// @brief The Finale 2007 and later clef scalars, as class records addressed by byte offset.
/// @details Exposed so a test can apply one era's table in isolation.
[[nodiscard]] const MappingTable& classClefOptionsTable();

/// @brief Checks recovered ClefOptions values that only make sense once the tables have run.
/// @details Runs after the clef scalar tables, because the default clef index is a mapped
/// scalar and the collection it indexes is built before them.
void validateClefOptions(const musx::dom::DocumentPtr& document, ImportReport& report);

/// @brief Recovers source clef definitions and completes the modern 18-definition collection.
/// @details The collection is rebuilt rather than seeded: its shape and font comparators
/// belong to the baseline's own tables. Definitions the source does not store are copied
/// from the reference document, which is what Finale's own upgrade does. Runs before the
/// clef scalar tables so that they have an object to overlay.
void captureClefOptions(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument, ImportReport& report,
    PendingReferences& pending);

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

/// @brief Recovers the source's stem connections into the seeded StemOptions object.
/// @details The collection is dropped and rebuilt rather than overlaid: stem connections
/// are a document's own table, naming that document's fonts, so the baseline's connections
/// are never a default for another document. A source that stores none leaves the
/// collection empty. The scalars around it stay seeded from the pinned baseline.
void captureStemOptions(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document, ImportReport& report);

/// @brief Checks the font references of recovered stem connections against the font pool.
/// @details Runs after the font definitions are decoded and after FontOptions has repaired
/// what it needed, because both may add to the pool a connection could be naming.
void validateStemOptions(const musx::dom::DocumentPtr& document, ImportReport& report);

} // namespace options
} // namespace finale_mus_reader
