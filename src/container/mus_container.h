// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "finale_mus_reader/reader.h"

namespace finale_mus_reader {
namespace container {

struct DecodedBlock
{
    BlockInfo info;
    std::vector<std::uint8_t> data;
};

struct ParsedContainer
{
    explicit ParsedContainer(FormatEpoch epoch) : formatEpoch(epoch) {}

    FormatEpoch formatEpoch;
    ByteOrder byteOrder = ByteOrder::Unknown;
    std::vector<DecodedBlock> blocks;
    std::size_t trailingByteCount{};
    /// @brief The Coda-banner text region: whatever follows the last record pool.
    /// @details That era keeps its text outside the pool chain, as length-prefixed chunks
    /// rather than in a block of its own, so it has nowhere else to be reported. Empty for
    /// every other epoch, which carries text in an ordinary block.
    std::vector<std::uint8_t> textRegion;
};

[[nodiscard]] ParsedContainer parse(const std::uint8_t* data, std::size_t size);

} // namespace container
} // namespace finale_mus_reader
