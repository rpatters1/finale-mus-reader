// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/support/legacy_mapping.h"

namespace finale_mus_reader {
namespace options {

// One importer per musxdom options class, and the registry calls nothing else. Everything a
// class needs -- how many physical layouts it has, which epochs each covers, whether a
// collection must be captured before its scalars are overlaid, and what must be checked once
// they have run -- is decided inside that class's own translation unit.
//
// Individual stages that a test drives on their own are declared in options/test_access.h,
// which no library code includes. A stage no test drives is file-local to its class.

/// @brief Recovers options::ClefOptions: its definition collection and its scalars.
void importClefOptions(const ImportContext& context);

/// @brief Recovers options::FontOptions and completes the modern type set.
void importFontOptions(const ImportContext& context);

/// @brief Recovers options::MultimeasureRestOptions across all four epochs.
/// @details Its two stages are file-local: no test drives either one alone, because the
/// class has no collection to build and the reader's own fixtures exercise both through
/// this entry point.
void importMultimeasureRestOptions(const ImportContext& context);

/// @brief Recovers options::MusicSpacingOptions.
void importMusicSpacingOptions(const ImportContext& context);

/// @brief Recovers options::StemOptions, whose connections are a source-owned collection.
void importStemOptions(const ImportContext& context);

} // namespace options
} // namespace finale_mus_reader
