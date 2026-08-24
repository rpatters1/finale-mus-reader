// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The extension point: a surveyor answers one question about one class by returning its
// structured observation. The harness owns per-surveyor failure isolation, so a surveyor
// file never repeats that boilerplate.
//
// To add coverage for a class: write a `Value observeX(const SurveyContext&)`
// in the surveyors/<pool>/ file for that class's pool (options, others, details, texts),
// matching the layout of src/import/, and register it with COVERAGE_SURVEYOR at namespace
// scope. Nothing else changes: registry.h, runAllSurveyors, and the probe's main() are not
// touched, and CMake picks the new file up on its own (see the surveyors/ glob in
// CMakeLists.txt).

#pragma once

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "coverage/context.h"
#include "coverage/value.h"

namespace finale_mus_reader {
namespace coverage {

using SurveyorFn = Value (*)(const SurveyContext& ctx);

struct SurveyTimings
{
    double durationMs{};
    std::vector<std::pair<std::string, double>> surveyors;
};

using SurveySnapshot = Value::Object;

struct SurveyResult
{
    SurveySnapshot snapshot;
    SurveyTimings timings;
    std::map<std::string, std::string> errors;
};

/// @brief Registers `fn` to run under observation member name `key`. Called only by
/// @ref COVERAGE_SURVEYOR; a duplicate key throws at static-init time.
void registerSurveyor(std::string_view key, SurveyorFn fn);

/// @brief Runs every registered surveyor over one imported document in registration order.
/// @details A surveyor that throws does not stop the others: its snapshot value is null and
/// the loop continues. The return value carries the structured snapshot plus whole-pass and
/// per-surveyor wall-clock durations.
SurveyResult runAllSurveyors(const SurveyContext& ctx);

} // namespace coverage
} // namespace finale_mus_reader

/// @brief Registers `fn` under `key` when this translation unit's static initializers
/// run. Put exactly one of these at namespace scope per surveyor a file defines.
#define COVERAGE_SURVEYOR(key, fn) \
    namespace { \
    const int registration_##fn = \
        (::finale_mus_reader::coverage::registerSurveyor(key, fn), 0); \
    }
