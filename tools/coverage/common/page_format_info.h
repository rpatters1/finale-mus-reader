// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "coverage/classification.h"

namespace finale_mus_reader {
namespace coverage {

inline std::optional<DifferenceClassification>
classifyPageFormatOptionsDifference(const DifferenceContext& context)
{
    if (context.category == DifferenceCategory::Differs
        && context.path == "page_format_options.adjust_page_scope"
        && context.origin == "finale27-default") {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

} // namespace coverage
} // namespace finale_mus_reader
