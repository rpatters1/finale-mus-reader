// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <set>

namespace finale_mus_reader {
namespace coverage {

/// @brief Whether one block text is a part name, given the text ids each side's parts name.
/// @details The source must name it. Where both sides name a text they must agree on which, so an
/// unrelated block is not swept in; where only the source names one, the source decides.
///
/// A text only the companion names is deliberately **not** a part name here. That is a synthesized
/// score name rather than a name the source stored, and @ref synthesizedScoreNameText identifies it
/// so it can be classified as what it is.
[[nodiscard]] inline bool partNameTextMatches(const std::set<std::int64_t>& sourceIds,
    const std::set<std::int64_t>& companionIds, std::int64_t textId)
{
    if (!sourceIds.contains(textId)) return false;
    return companionIds.empty() || companionIds.contains(textId);
}

/// @brief Whether one block text is a name Finale synthesized for a score the source did not name.
/// @details Every imported document has a score part, including the eras that store no part
/// definition, and that part carries no name: the text block holding it is written by a later
/// Finale during its upgrade and is not in the source. Usually that block is a fresh one the source
/// side does not have at all; where Finale instead reuses an empty block the source already
/// carries, the synthesis appears as a text difference on a block both sides have.
[[nodiscard]] inline bool synthesizedScoreNameText(const std::set<std::int64_t>& sourceIds,
    const std::set<std::int64_t>& companionIds, std::int64_t textId)
{
    return sourceIds.empty() && companionIds.contains(textId);
}

} // namespace coverage
} // namespace finale_mus_reader
