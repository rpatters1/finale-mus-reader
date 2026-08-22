// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"

#include <chrono>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "coverage/json.h"

namespace finale_mus_reader::coverage {

namespace {

// A function-local static avoids the static-initialization-order fiasco: every
// registerSurveyor() call happens during some other translation unit's static
// initialization, and this guarantees the vector exists before the first of them runs.
std::vector<std::pair<std::string, SurveyorFn>>& registry()
{
    static std::vector<std::pair<std::string, SurveyorFn>> instance;
    return instance;
}

} // namespace

void registerSurveyor(std::string_view key, SurveyorFn fn)
{
    for (const auto& [existingKey, existingFn] : registry()) {
        if (existingKey == key) {
            throw std::logic_error("duplicate coverage surveyor key: " + std::string(key));
        }
    }
    registry().emplace_back(std::string(key), fn);
}

SurveyTimings runAllSurveyors(std::ostream& out, const SurveyContext& ctx)
{
    SurveyTimings timings;
    const auto allStarted = std::chrono::steady_clock::now();
    for (const auto& [key, fn] : registry()) {
        const auto surveyorStarted = std::chrono::steady_clock::now();
        out << ",\"" << key << "\":";
        std::ostringstream value;
        try {
            fn(value, ctx);
            out << value.str();
        } catch (const std::exception& error) {
            out << "null,\"" << key << "_error\":" << jsonString(error.what());
        }
        const std::chrono::duration<double, std::milli> elapsed =
            std::chrono::steady_clock::now() - surveyorStarted;
        timings.surveyors.emplace_back(key, elapsed.count());
    }
    const std::chrono::duration<double, std::milli> elapsed =
        std::chrono::steady_clock::now() - allStarted;
    timings.durationMs = elapsed.count();
    return timings;
}

} // namespace finale_mus_reader::coverage
