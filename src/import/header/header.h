// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>

#include "container/mus_container.h"
#include "finale_mus_reader/reader.h"
#include "musx/dom/Document.h"

namespace finale_mus_reader {
namespace header {

[[nodiscard]] bool hasBanner(const std::uint8_t* data, std::size_t size);

/// @brief Fills in banner, product, platform, and version from the 0x200 header.
/// @param report **Set @ref ImportReport::byteOrder before calling.** The header version is a
/// 32-bit value in the file's own byte order, and with the order still `Unknown` this falls
/// back to trying big-endian and accepting the result whenever its major version lands in
/// Finale's 0-27 range. That guess is wrong often enough to matter: a Windows Finale 3.0
/// file stores `0f 03 01 03`, whose big-endian reading is major 15, which passes the range
/// test and hides the correct 3.0.1. The reader sets the order from the container first, so
/// only a standalone caller -- a probe, typically -- can trip over this.
void describeSourceIdentity(const std::uint8_t* data, std::size_t size, ImportReport& report);

[[nodiscard]] musx::dom::header::HeaderPtr recover(
    const std::uint8_t* data, std::size_t size, const ImportReport& report);

} // namespace header
} // namespace finale_mus_reader
