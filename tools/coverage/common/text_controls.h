// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

namespace finale_mus_reader {
namespace coverage {

[[nodiscard]] constexpr bool isFinaleWhitespaceControl(unsigned char value)
{
    return value >= 0x01 && value <= 0x07;
}

} // namespace coverage
} // namespace finale_mus_reader
