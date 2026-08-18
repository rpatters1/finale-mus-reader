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

/// @brief Recovers texts::ExpressionText from the era that embeds it in its own record.
/// @details Separate from @ref importTextPool because the source is a record family rather
/// than the text stream, and because the two never apply to the same document.
void importExpressionTexts(const ImportContext& context);

/// @brief Recovers texts::FileInfoText from the fixed header strings.
void importFileInfoTexts(const ImportContext& context);

} // namespace texts
} // namespace finale_mus_reader
