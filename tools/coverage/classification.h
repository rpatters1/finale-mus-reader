// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "coverage/value.h"
#include "finale_mus_reader/reader.h"

namespace finale_mus_reader {
namespace coverage {

enum class DifferenceCategory
{
    Differs,
    ReaderOnly,
    CompanionOnly
};

enum class RelatedDifference
{
    None,
    MatchingTextBlockReferent,
    MatchingPageOnlyTextBlockReferent,
    RenumberedTextBlockReferent
};

enum class DifferenceClassification
{
    Unexpected,
    AccidentalInsert17Byte,
    AwaitsDependentRecovery,
    CharsetEquivalence,
    CharsetPitchDifference,
    FontPlatformShift,
    SymbolFontEquivalence,
    BetaDiscrepancy,
    CodaTextBlockUpgrade,
    DefaultShapeId,
    DifferentDefaults,
    EnigmaTextDifference,
    FinaleTextBlockRenumbering,
    FinaleUpgradeLoss,
    FinaleUpgradeNormalization,
    FontMetricApproximation,
    TextEncodingError,
    LegacyPageParityText,
    MissingAccidentalInsertDefault,
    MissingSelector,
    PossiblyUnrecoverable,
    PreConnectionEndpoint,
    ReaderCompletedConnectionArray,
    SetFontSubstitution,
    ShapeReclassifiedOther,
    SmartLyricsEnabled,
    StemConnectionPastTerminator,
    StemHorizontalCorrection,
    SynthesizedScoreName,
    TransientTextBlock,
    WhitespaceControl
};

enum class TextDifferenceClassification
{
    AddedFontInfo,
    Effects,
    EmptyPartNameTemplate,
    Font,
    KnownEncodingGlitch,
    MissingRun,
    Other,
    Size,
    SynthesizedScoreName,
    UnresolvedFont,
    Whitespace
};

enum class ComparisonTransformation
{
    EquivalentEnigmaFontState,
    EquivalentTextBlockReferent,
    FinaleAddedChordSuffixFiller,
    FinaleAddedStartObjectWrapper,
    FinaleMaterializedPartMeasure,
    FinaleDroppedTimeInsert,
    FinaleReformattedPartName,
    SemanticallyPairedCodaBlockText
};

using SurveySnapshot = Value::Object;

struct ComparisonPreparationContext
{
    SurveySnapshot& source;
    SurveySnapshot& companion;
    std::map<ComparisonTransformation, std::uint64_t>& transformations;
    FormatEpoch sourceEpoch;
};

using ComparisonPreparationFn = void (*)(ComparisonPreparationContext& context);

struct TextClassificationResult
{
    bool equivalent{};
    std::set<TextDifferenceClassification> differences;
    std::optional<ComparisonTransformation> transformation;
};

struct TextDifferenceContext
{
    std::string_view path;
    std::string_view normalizedSource;
    std::string_view normalizedCompanion;
    std::optional<std::string_view> sourcePlain;
    std::optional<std::string_view> companionPlain;
    bool removedWhitespaceControl{};
    bool partNameText{};
    /// @brief The companion's score part names this text and the reader's names none.
    bool synthesizedScoreName{};
};

using TextDifferenceClassifierFn =
    std::optional<TextClassificationResult> (*)(const TextDifferenceContext& context);

using ComparisonLeaves = std::map<std::string, std::pair<Value, std::string>>;

struct DifferenceContext
{
    std::string_view path;
    DifferenceCategory category;
    std::string_view origin;
    const Value& sourceValue;
    const Value& companionValue;
    const ComparisonLeaves& source;
    const ComparisonLeaves& companion;
    FormatEpoch epoch;
    ByteOrder byteOrder;
    const SourceVersion* sourceVersion;
    const ImportReport& sourceReport;
    RelatedDifference relatedDifference{};
    std::string_view companionFontIdentity{};
};

using DifferenceClassifierFn =
    std::optional<DifferenceClassification> (*)(const DifferenceContext& context);
using DifferenceEquivalenceFn = bool (*)(const DifferenceContext& context);

/// @brief Whether deferred-recovery rules classify their differences at all.
/// @details A deferred-recovery difference is one that a musxdom class the reader does not yet
/// recover would settle: the companion states a value the source cannot yet be asked for. Such a
/// difference is expected only in the sense that it is understood, and every one of them is work
/// still owed, so the whole set can be switched back to @ref DifferenceClassification::Unexpected
/// to see what is actually outstanding. `--strict-deferred` on the probe does exactly that.
///
/// Process-wide because a classifier is a plain function pointer with no configuration of its own,
/// and the probe surveys one document at a time in one thread.
void setDeferredRecoveryClassified(bool enabled);
[[nodiscard]] bool deferredRecoveryClassified();

bool comparisonPathStartsWith(std::string_view path, std::string_view prefix);
bool comparisonPathEndsWith(std::string_view path, std::string_view suffix);
inline std::optional<std::int64_t> comparisonIntegerLeaf(
    const ComparisonLeaves& leaves, std::string_view path)
{
    const auto found = leaves.find(std::string(path));
    if (found == leaves.end() || !found->second.first.isInteger()) return std::nullopt;
    return found->second.first.asInteger();
}
bool comparisonEqualSurrounding(const ComparisonLeaves& source, const ComparisonLeaves& companion,
                                std::string_view prefix, std::string_view excluded);

} // namespace coverage
} // namespace finale_mus_reader
