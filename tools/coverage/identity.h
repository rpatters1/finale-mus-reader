// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <string>

#include "musx/dom/Fundamentals.h"

namespace finale_mus_reader {
namespace coverage {

inline std::string partIdentityPrefix(std::int64_t partId)
{
    return partId == musx::dom::SCORE_PARTID
        ? std::string{}
        : "part_id=" + std::to_string(partId) + ',';
}

} // namespace coverage
} // namespace finale_mus_reader
