// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "finale_mus_reader/reader.h"

namespace finale_mus_reader {
namespace fret_records {

struct RecordSource
{
    const records::LegacyRowPool* pool{};
    records::LegacyTag identity{};
    bool variableLength{};
};

inline std::optional<RecordSource> selectSource(const ImportContext& context,
    const records::LegacyRowPool& fixedPool, const records::LegacyRowPool& classPool,
    records::LegacyTag fixedTag, records::LegacyTag classId)
{
    // The stored identity is the structural discriminator. A fixed-row epoch with no matching
    // rows produces no objects, so this selection needs no version gate.
    switch (context.profile.epoch) {
    case FormatEpoch::CodaBanner:
    case FormatEpoch::UncompressedLegacy:
    case FormatEpoch::DclLegacy:
        return RecordSource{&fixedPool, fixedTag, false};
    case FormatEpoch::ZlibLegacy:
        return RecordSource{&classPool, classId, true};
    case FormatEpoch::Unknown:
        return std::nullopt;
    }
    return std::nullopt;
}

inline std::vector<std::uint8_t> collectPayload(const RecordSource& source,
    std::span<const records::LegacyRow> rows)
{
    std::vector<std::uint8_t> result;
    if (source.variableLength && !rows.empty()) {
        const auto payload = source.pool->payloadOf(rows.front());
        result.assign(payload.begin(), payload.end());
        return result;
    }
    for (const auto& row : rows) {
        const auto payload = source.pool->payloadOf(row);
        result.insert(result.end(), payload.begin(), payload.end());
    }
    return result;
}

inline std::uint16_t readWord(std::span<const std::uint8_t> payload,
    std::size_t at, ByteOrder order)
{
    return order == ByteOrder::BigEndian
        ? static_cast<std::uint16_t>((static_cast<std::uint16_t>(payload[at]) << 8U)
            | payload[at + 1])
        : static_cast<std::uint16_t>(payload[at]
            | (static_cast<std::uint16_t>(payload[at + 1]) << 8U));
}

inline std::int32_t readLong(std::span<const std::uint8_t> payload,
    std::size_t at, ByteOrder order)
{
    const auto first = readWord(payload, at, order);
    const auto second = readWord(payload, at + 2, order);
    const auto value = order == ByteOrder::BigEndian
        ? (static_cast<std::uint32_t>(first) << 16U) | second
        : (static_cast<std::uint32_t>(second) << 16U) | first;
    return static_cast<std::int32_t>(value);
}

inline std::int32_t readHighFirstLong(std::span<const std::uint8_t> payload,
    std::size_t at, ByteOrder order)
{
    return static_cast<std::int32_t>(
        (static_cast<std::uint32_t>(readWord(payload, at, order)) << 16U)
        | readWord(payload, at + 2, order));
}

inline std::int32_t readLowFirstLong(std::span<const std::uint8_t> payload,
    std::size_t at, ByteOrder order)
{
    return static_cast<std::int32_t>(
        (static_cast<std::uint32_t>(readWord(payload, at + 2, order)) << 16U)
        | readWord(payload, at, order));
}

inline std::string readString(std::span<const std::uint8_t> payload,
    std::size_t at, std::size_t capacity)
{
    const auto available = (std::min)(capacity, payload.size() - at);
    std::string result(reinterpret_cast<const char*>(payload.data() + at), available);
    if (const auto end = result.find('\0'); end != std::string::npos) {
        result.resize(end);
    }
    return result;
}

} // namespace fret_records
} // namespace finale_mus_reader
