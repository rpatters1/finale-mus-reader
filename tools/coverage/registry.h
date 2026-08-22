// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The extension point: a surveyor answers one question about one class -- given an
// imported document, write the single JSON value that is this class's coverage. The
// harness owns the JSON key, the comma, and per-surveyor failure isolation, so a
// surveyor file never repeats that boilerplate.
//
// To add coverage for a class: write a `void writeX(std::ostream&, const SurveyContext&)`
// in the surveyors/<pool>/ file for that class's pool (options, others, details, texts),
// matching the layout of src/import/, and register it with COVERAGE_SURVEYOR at namespace
// scope. Nothing else changes: registry.h, runAllSurveyors, and the probe's main() are not
// touched, and CMake picks the new file up on its own (see the surveyors/ glob in
// CMakeLists.txt).

#pragma once

#include <iosfwd>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "coverage/context.h"

namespace finale_mus_reader::coverage {

using SurveyorFn = void (*)(std::ostream& out, const SurveyContext& ctx);

struct SurveyTimings
{
    double durationMs{};
    std::vector<std::pair<std::string, double>> surveyors;
};

/// @brief Registers `fn` to run under JSON member name `key`. Called only by
/// @ref COVERAGE_SURVEYOR; a duplicate key throws at static-init time.
void registerSurveyor(std::string_view key, SurveyorFn fn);

/// @brief Runs every registered surveyor over one imported document, in registration
/// order, writing `,"<key>":<value>` for each.
/// @details A surveyor that throws does not stop the others: it contributes
/// `,"<key>":null,"<key>_error":"<message>"` and the loop continues, so one class's bug
/// or a newly added, still-broken surveyor can never blank the rest of the document's
/// coverage. The return value reports the whole pass and each registered surveyor's
/// wall-clock duration.
SurveyTimings runAllSurveyors(std::ostream& out, const SurveyContext& ctx);

} // namespace finale_mus_reader::coverage

/// @brief Registers `fn` under `key` when this translation unit's static initializers
/// run. Put exactly one of these at namespace scope per surveyor a file defines.
#define COVERAGE_SURVEYOR(key, fn) \
    namespace { \
    const int registration_##fn = \
        (::finale_mus_reader::coverage::registerSurveyor(key, fn), 0); \
    }
