// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The extension point: one descriptor owns a class's structured observation and
// any comparison rules specific to that class. The harness owns per-surveyor
// failure isolation and invokes the registered classifiers during comparison.
//
// To add coverage for a class: write a `Value observeX(const SurveyContext&)`
// in the surveyors/<pool>/ file for that class's pool (options, others,
// details, texts), matching the layout of src/import/, then register its
// descriptor at namespace scope. Nothing else changes: registry.h,
// comparison.cpp, runAllSurveyors, and the probe's main() are not touched, and
// CMake picks the new file up on its own (see the surveyors/ glob in
// CMakeLists.txt).

#pragma once

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "coverage/classification.h"
#include "coverage/context.h"
#include "coverage/value.h"

namespace finale_mus_reader {
namespace coverage {

using SurveyorFn = Value (*)(const SurveyContext& ctx);

struct CoverageClassDescriptor
{
    std::string_view pool;
    std::string_view key;
    SurveyorFn observe;
    DifferenceClassifierFn classifyDifference{};
    TextDifferenceClassifierFn classifyTextDifference{};
    DifferenceEquivalenceFn equivalentDifference{};
    ComparisonPreparationFn prepareComparison{};
};

struct SurveyTimings
{
    double durationMs{};
    std::vector<std::pair<std::string, double>> surveyors;
};

struct SurveyResult
{
    SurveySnapshot snapshot;
    SurveyTimings timings;
    std::map<std::string, std::string> errors;
};

/// @brief Registers one coverage class. A duplicate key throws at static-init
/// time.
void registerSurveyor(const CoverageClassDescriptor& descriptor);

/// @brief Returns the registered comparison classifier, or null when the class
/// has no rules.
DifferenceClassifierFn differenceClassifier(std::string_view key);
TextDifferenceClassifierFn textDifferenceClassifier(std::string_view key);
DifferenceEquivalenceFn differenceEquivalence(std::string_view key);
void runComparisonPreparers(ComparisonPreparationContext& context);

/// @brief Returns the registered musxdom pool for an observation member.
std::string_view surveyorPool(std::string_view key);

/// @brief Runs every registered surveyor over one imported document in
/// registration order.
/// @details A surveyor that throws does not stop the others: its snapshot value
/// is null and the loop continues. The return value carries the structured
/// snapshot plus whole-pass and per-surveyor wall-clock durations.
SurveyResult runAllSurveyors(const SurveyContext& ctx);

} // namespace coverage
} // namespace finale_mus_reader

/// @brief Registers one class descriptor when this translation unit
/// initializes.
#define COVERAGE_CLASS(pool, key, fn, classifier)                                                  \
    namespace {                                                                                    \
    const int registration_##fn =                                                                  \
        (::finale_mus_reader::coverage::registerSurveyor({pool, key, fn, classifier, nullptr}),    \
         0);                                                                                       \
    }

#define COVERAGE_TEXT_CLASS(pool, key, fn, textClassifier)                                         \
    namespace {                                                                                    \
    const int registration_##fn = (::finale_mus_reader::coverage::registerSurveyor(                \
                                       {pool, key, fn, nullptr, textClassifier, nullptr, nullptr}),\
                                   0);                                                             \
    }

#define COVERAGE_CLASS_WITH_EQUIVALENCE(pool, key, fn, classifier, equivalence)                    \
    namespace {                                                                                    \
    const int registration_##fn = (::finale_mus_reader::coverage::registerSurveyor(                \
                                       {pool, key, fn, classifier, nullptr, equivalence, nullptr}),\
                                   0);                                                             \
    }

#define COVERAGE_CLASS_WITH_PREPARATION(pool, key, fn, classifier, preparation)                   \
    namespace {                                                                                    \
    const int registration_##fn = (::finale_mus_reader::coverage::registerSurveyor(                \
                                       {pool, key, fn, classifier, nullptr, nullptr, preparation}),\
                                   0);                                                             \
    }

#define COVERAGE_SURVEYOR(pool, key, fn) COVERAGE_CLASS(pool, key, fn, nullptr)
