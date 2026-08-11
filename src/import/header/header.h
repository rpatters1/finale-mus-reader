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

void describeSourceIdentity(const std::uint8_t* data, std::size_t size, ImportReport& report);

[[nodiscard]] musx::dom::header::HeaderPtr recover(
    const std::uint8_t* data, std::size_t size, const ImportReport& report);

} // namespace header
} // namespace finale_mus_reader
