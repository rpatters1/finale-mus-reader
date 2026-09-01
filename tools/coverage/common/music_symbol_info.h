// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <optional>

#include "coverage/classification.h"

namespace finale_mus_reader {
namespace coverage {

inline std::optional<DifferenceClassification>
classifyDoubleWholeSlashConversionLoss(const DifferenceContext& context)
{
    constexpr std::int64_t legacyDoubleWholeSlash = 218;
    constexpr std::int64_t filledNoteheadSlash = 213;
    if (context.category == DifferenceCategory::Differs &&
        context.path == "music_symbol_options.dbl_whole_slash" &&
        context.origin == "finale27-default" && context.sourceValue.isInteger() &&
        context.sourceValue.asInteger() == legacyDoubleWholeSlash &&
        context.companionValue.isInteger() &&
        context.companionValue.asInteger() == filledNoteheadSlash) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    return std::nullopt;
}

inline std::optional<DifferenceClassification>
classifyVersionlessCodaSlashDefault(const DifferenceContext& context)
{
    const bool lacksHeaderVersion = !context.sourceVersion
        || context.sourceVersion->raw == 0;
    if (context.category != DifferenceCategory::Differs
        || context.epoch != finale_mus_reader::FormatEpoch::CodaBanner) {
        return std::nullopt;
    }
    const bool sourceRecovered = context.path == "music_symbol_options.half_slash"
        || context.path == "music_symbol_options.whole_slash";
    const bool retainedCodaDefault = context.origin == "finale27-default"
        && (context.path == "music_symbol_options.quarter_slash"
            || context.path == "music_symbol_options.slash_bar");
    const bool retainedHeaderlessDefault = lacksHeaderVersion
        && context.origin == "finale27-default"
        && context.path == "music_symbol_options.dbl_whole_slash";
    if ((lacksHeaderVersion && sourceRecovered) || retainedCodaDefault
        || retainedHeaderlessDefault) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

} // namespace coverage
} // namespace finale_mus_reader
