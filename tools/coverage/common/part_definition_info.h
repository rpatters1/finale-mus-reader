// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>

#include "coverage/classification.h"

namespace finale_mus_reader {
namespace coverage {

/// @brief Classifies the two companion differences this class is expected to produce.
/// @details The part definition record does not carry `unlinkInsts`, so the source side leaves it
/// default with @ref ValueOrigin::Unmapped while a companion may set it. Another record may carry
/// it, which is why the classification says *possibly* unrecoverable rather than asserting that
/// none does.
///
/// The score's name is the other. A score had no name object before linked parts existed, so the
/// source side holds a null reference reported as the era's behavior while the companion points at
/// a text block a later Finale synthesized during its upgrade. Every condition below is
/// load-bearing -- the score part alone, a null source value against a supplied companion one,
/// era's-behavior provenance, and never the epoch that stores the member -- so that a recovered
/// `nameId` that disagrees cannot pass as expected.
inline std::optional<DifferenceClassification>
classifyPartDefinitionDifference(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs) return std::nullopt;
    if (context.path.ends_with(".unlink_insts") && context.origin == "unmapped"
        && context.sourceValue.isBool() && context.companionValue.isBool()
        && !context.sourceValue.asBool() && context.companionValue.asBool()) {
        return DifferenceClassification::PossiblyUnrecoverable;
    }
    if (context.epoch == FormatEpoch::ZlibLegacy) return std::nullopt;
    if (!context.path.ends_with("[cmper=0].name_id")) return std::nullopt;
    if (context.origin != "legacy-behavior") return std::nullopt;
    if (!context.sourceValue.isInteger() || !context.companionValue.isInteger()) {
        return std::nullopt;
    }
    if (context.sourceValue.asInteger() != 0 || context.companionValue.asInteger() == 0) {
        return std::nullopt;
    }
    return DifferenceClassification::SynthesizedScoreName;
}

} // namespace coverage
} // namespace finale_mus_reader
