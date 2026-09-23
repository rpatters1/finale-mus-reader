// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "finale_mus_reader/reader.h"
#include "musx/dom/Fundamentals.h"

namespace finale_mus_reader {
namespace percussion {

using MappingTable = std::unordered_map<std::uint16_t, musx::dom::PercussionNoteTypeId>;

/// @brief Map-specific percussion note-type tables supplied by the caller.
class MappingTables
{
public:
    /// @brief Resolves a legacy map name through a conversion entry, then falls back to
    /// direct name lookup when the entry or its note is unavailable.
    [[nodiscard]] std::optional<musx::dom::PercussionNoteTypeId> find(
        std::string_view mapName, std::uint16_t inputKey, std::uint16_t playbackMidiNote) const;

private:
    std::unordered_map<std::string, MappingTable> m_tables;
    std::map<std::pair<std::string, std::string>, MappingTable> m_namedTables;
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> m_conversions;

    friend MappingTables parseMappingTables(std::span<const PercussionMappingXml>, std::span<const std::uint8_t>, XmlParser);
};

/// @brief Parses Finale MIDI Device Annotation XML buffers through the caller's
/// XML backend.
/// @details The first table with each normalized name is retained. Within a
/// table, the first note for a repeated MIDI number is retained because the
/// legacy map carries no discriminator.
[[nodiscard]] MappingTables parseMappingTables(
    std::span<const PercussionMappingXml> documents, std::span<const std::uint8_t> conversionTable, XmlParser parseXml);

/// @brief Normalizes a legacy percussion-map name for annotation-table lookup.
[[nodiscard]] std::string normalizeMappingName(std::string_view name);

} // namespace percussion
} // namespace finale_mus_reader
