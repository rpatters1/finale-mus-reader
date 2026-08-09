// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "container/mus_container.h"
#include "records/legacy_others.h"

namespace finale_mus_reader {
namespace records {

/// @brief One payload word together with the provenance of the row that held it.
struct RecordWord
{
    std::int16_t value{};
    std::size_t blockOffset{};
    std::size_t decodedOffset{};
};

/// @brief Presents the legacy other pool as a word stream per record family.
/// @details A record family is every incidence sharing a tag and cmper. Fields address
/// the family by absolute word index, `incidence * wordsPerIncidence + slot`, because a
/// four-byte value can straddle an incidence boundary: the distilled framework mapping
/// places `MusicSpacingPrefs.scalingFactor` low word at incidence 1 slot 5 and its high
/// word at incidence 2 slot 0. Treating incidence and slot as independent coordinates
/// cannot express that.
///
/// Mapping tables are applied against this interface rather than the decoded rows, so a
/// later record source for the 2007+ era can drive the same tables.
class LegacyRecordIndex
{
public:
    /// @brief Payload words carried by one 16-byte legacy row.
    static constexpr std::size_t wordsPerIncidence = 6;

    /// @brief Indexes every legacy other row in a parsed container.
    [[nodiscard]] static LegacyRecordIndex build(const container::ParsedContainer& parsed);

    /// @brief Reads one word of a record family.
    /// @param tag The two-character record tag.
    /// @param cmper The record comparator.
    /// @param wordIndex Absolute word index across incidences.
    /// @return The word, or `std::nullopt` when the family or that incidence is absent.
    [[nodiscard]] std::optional<RecordWord> word(
        std::string_view tag, std::uint16_t cmper, std::size_t wordIndex) const;

    /// @brief Returns every cmper present for a tag, in ascending order.
    [[nodiscard]] std::vector<std::uint16_t> cmpersForTag(std::string_view tag) const;

    /// @brief Returns whether any record was indexed.
    [[nodiscard]] bool empty() const { return m_families.empty(); }

private:
    using FamilyKey = std::pair<std::string, std::uint16_t>;

    std::map<FamilyKey, std::vector<LegacyOther>> m_families;
    ByteOrder m_byteOrder = ByteOrder::Unknown;
};

} // namespace records
} // namespace finale_mus_reader
