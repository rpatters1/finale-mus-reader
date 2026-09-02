// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>

#include "coverage/classification.h"
#include "coverage/support/source_gate.h"

namespace finale_mus_reader
{
namespace coverage
{

inline std::optional<DifferenceClassification>
classifyTieOptionsDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    const auto isThickness = context.path == "tie_options.thickness_left" ||
                             context.path == "tie_options.thickness_right";
    if (context.category == Differs && context.epoch == FormatEpoch::CodaBanner && isThickness &&
        context.origin.starts_with("legacy-mus"))
    {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    if (context.category == Differs && context.path == "tie_options.mixed_stem_direction" &&
        context.origin.starts_with("legacy-mus") && context.sourceValue.isInteger() &&
        context.sourceValue.asInteger() == 2 && context.companionValue.isInteger() &&
        context.companionValue.asInteger() == 0 &&
        sourceIsVersion(context.epoch, context.sourceVersion, FormatEpoch::DclLegacy,
                        versions::finale2006))
    {
        return DifferenceClassification::FinaleUpgradeLoss;
    }

    const auto* scatteredLayout =
        context.sourceReport.findField<musx::dom::options::TieOptions>("breakForTimeSigs");
    const auto preSelector84Uncompressed = context.epoch == FormatEpoch::UncompressedLegacy &&
                                           scatteredLayout &&
                                           scatteredLayout->origin == ValueOrigin::LegacyBehavior;
    if (context.category == Differs && context.origin == "finale27-default" &&
        (context.epoch == FormatEpoch::CodaBanner || preSelector84Uncompressed))
    {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

} // namespace coverage
} // namespace finale_mus_reader
