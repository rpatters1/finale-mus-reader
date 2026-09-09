// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

#include "finale_mus_reader/reader.h"
#include "musx/dom/Fundamentals.h"

namespace finale_mus_reader {
namespace percussion {

using MappingTable =
    std::unordered_map<std::uint16_t, musx::dom::PercussionNoteTypeId>;

/// @brief Map-specific MIDI-to-percussion-type tables supplied by the caller.
class MappingTables {
public:
    /// @brief Returns the mapped percussion note type for a legacy map name and
    /// MIDI note.
    [[nodiscard]] std::optional<musx::dom::PercussionNoteTypeId>
    find(std::string_view mapName, std::uint16_t midiNote) const;

private:
    std::unordered_map<std::string, MappingTable> m_tables;

    friend MappingTables
        parseMappingTables(std::span<const std::span<const std::uint8_t>>,
                           XmlParser);
};

/// @brief Parses Finale MIDI Device Annotation XML buffers through the caller's
/// XML backend.
/// @details The first table with each normalized name is retained. Within a
/// table, the first note for a repeated MIDI number is retained because the
/// legacy map carries no discriminator.
[[nodiscard]] MappingTables
parseMappingTables(std::span<const std::span<const std::uint8_t>> documents,
                   XmlParser parseXml);

/// @brief Normalizes a legacy percussion-map name for annotation-table lookup.
[[nodiscard]] std::string normalizeMappingName(std::string_view name);

} // namespace percussion
} // namespace finale_mus_reader
