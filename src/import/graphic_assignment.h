// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "musx/musx.h"

namespace finale_mus_reader {

inline constexpr std::size_t graphicAssignmentWordCount = 18;

template <typename Target>
void populateGraphicAssignmentCommon(Target& target, std::span<const std::int16_t> words)
{
    target.version = static_cast<std::uint16_t>(words[0]);
    target.left = words[1];
    target.bottom = words[2];
    target.width = words[3];
    target.height = words[4];
    target.fDescId = static_cast<musx::dom::Cmper>(words[5]);
    target.hidden = words[6] != 0;
    target.savedRecord = words[11] != 0;
    target.origWidth = words[12];
    target.origHeight = words[13];
    target.graphicCmper = static_cast<musx::dom::Cmper>(words[17]);
}

} // namespace finale_mus_reader
