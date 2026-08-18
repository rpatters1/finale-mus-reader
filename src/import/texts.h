// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/support/legacy_mapping.h"

namespace finale_mus_reader {
namespace texts {

// One importer per source of texts-pool objects, and the registry calls nothing else. The
// text pool is not a record pool, so neither of these is a mapping table: one walks a byte
// stream and the other reads fixed header offsets.

/// @brief Recovers every class the legacy text pool spells out for itself.
/// @details Block text, the three lyric kinds, smart shape text, and expression text all
/// live in one byte stream that names each record by keyword, so one importer covers them
/// and a keyword decides the class.
void importTextPool(const ImportContext& context);

/// @brief Recovers texts::FileInfoText from the fixed header strings.
void importFileInfoTexts(const ImportContext& context);

/// @brief Recovers the Coda-banner epoch's block and lyric text.
/// @details That era stores neither in the text pool the later ones use: block text is in the
/// `HT` and `HS` others families and lyric text is in the region behind the last record pool.
/// Does nothing for any other epoch.
void importCodaTexts(const ImportContext& context);

} // namespace texts
} // namespace finale_mus_reader
