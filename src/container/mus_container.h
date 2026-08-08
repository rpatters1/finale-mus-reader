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
    FormatEpoch formatEpoch = FormatEpoch::Unknown;
    ByteOrder byteOrder = ByteOrder::Unknown;
    std::vector<DecodedBlock> blocks;
    std::size_t trailingByteCount{};
};

[[nodiscard]] ParsedContainer parse(const std::uint8_t* data, std::size_t size);

} // namespace container
} // namespace finale_mus_reader
