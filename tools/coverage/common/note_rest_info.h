// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <optional>
#include <string_view>

#include "coverage/classification.h"

namespace finale_mus_reader {
namespace coverage {

inline constexpr std::string_view noteRestDrop8thLeaf = "drop8th_rest";
inline constexpr std::string_view noteRestDrop16thLeaf = "drop16th_rest";
inline constexpr std::string_view noteRestDrop32ndLeaf = "drop32nd_rest";
inline constexpr std::string_view noteRestDrop64thLeaf = "drop64th_rest";
inline constexpr std::string_view noteRestDrop128thLeaf = "drop128th_rest";

inline std::optional<DifferenceClassification>
classifyNoteRestOptionsDifference(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs ||
        context.origin != "finale27-default") {
        return std::nullopt;
    }

    constexpr std::string_view prefix = "note_rest_options.";
    constexpr std::array restDropLeaves{
        noteRestDrop8thLeaf,
        noteRestDrop16thLeaf,
        noteRestDrop32ndLeaf,
        noteRestDrop64thLeaf,
        noteRestDrop128thLeaf,
    };
    for (const auto leaf : restDropLeaves) {
        if (context.path.size() == prefix.size() + leaf.size() &&
            context.path.starts_with(prefix) && context.path.ends_with(leaf)) {
            return DifferenceClassification::DifferentDefaults;
        }
    }
    return std::nullopt;
}

} // namespace coverage
} // namespace finale_mus_reader
