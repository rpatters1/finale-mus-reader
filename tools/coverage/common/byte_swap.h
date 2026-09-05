// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace finale_mus_reader {
namespace coverage {

[[nodiscard]] inline bool hasContiguousAdjacentByteSwap(
    std::string_view source, std::string_view companion)
{
    if (source.size() != companion.size()) return false;
    for (std::size_t start = 0; start + 1 < source.size(); start += 2) {
        std::string swapped(source);
        for (std::size_t end = start + 2; end <= swapped.size(); end += 2) {
            std::swap(swapped[end - 2], swapped[end - 1]);
            if (swapped == companion) return true;
        }
    }
    return false;
}

} // namespace coverage
} // namespace finale_mus_reader
