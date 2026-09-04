// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>
#include <string_view>

#include "coverage/classification.h"

namespace finale_mus_reader {
namespace coverage {

/// @brief Classifier metadata populated through FontDefinition::calcIsSymbolFont().
inline constexpr std::string_view fontDefinitionIsSymbolField = "is_symbol";

bool isClassifierMetadataPath(std::string_view path);
std::optional<DifferenceClassification>
classifyFontDefinitionDifference(const DifferenceContext& context);

std::optional<DifferenceClassification>
classifyDoubleWholeSlashConversionLoss(const DifferenceContext& context);
std::optional<DifferenceClassification>
classifyVersionlessCodaSlashDefault(const DifferenceContext& context);

inline constexpr std::string_view noteRestDrop8thLeaf = "drop8th_rest";
inline constexpr std::string_view noteRestDrop16thLeaf = "drop16th_rest";
inline constexpr std::string_view noteRestDrop32ndLeaf = "drop32nd_rest";
inline constexpr std::string_view noteRestDrop64thLeaf = "drop64th_rest";
inline constexpr std::string_view noteRestDrop128thLeaf = "drop128th_rest";

std::optional<DifferenceClassification>
classifyNoteRestOptionsDifference(const DifferenceContext& context);
std::optional<DifferenceClassification>
classifyPageFormatOptionsDifference(const DifferenceContext& context);

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
std::optional<DifferenceClassification>
classifyPartDefinitionDifference(const DifferenceContext& context);
std::optional<DifferenceClassification>
classifyTieOptionsDifference(const DifferenceContext& context);

} // namespace coverage
} // namespace finale_mus_reader
