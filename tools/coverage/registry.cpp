// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace finale_mus_reader {
namespace coverage {

namespace {

// A function-local static avoids the static-initialization-order fiasco: every
// registerSurveyor() call happens during some other translation unit's static
// initialization, and this guarantees the vector exists before the first of
// them runs.
struct RegisteredSurveyor
{
    std::string pool;
    std::string key;
    SurveyorFn fn;
    DifferenceClassifierFn classifyDifference{};
    TextDifferenceClassifierFn classifyTextDifference{};
    DifferenceEquivalenceFn equivalentDifference{};
    ComparisonPreparationFn prepareComparison{};
};

std::vector<RegisteredSurveyor>& registry()
{
    static std::vector<RegisteredSurveyor> instance;
    return instance;
}

} // namespace

void registerSurveyor(const CoverageClassDescriptor& descriptor)
{
    for (const auto& registered : registry()) {
        if (registered.key == descriptor.key) {
            throw std::logic_error("duplicate coverage surveyor key: " +
                                   std::string(descriptor.key));
        }
    }
    registry().push_back({std::string(descriptor.pool), std::string(descriptor.key),
                          descriptor.observe, descriptor.classifyDifference,
                          descriptor.classifyTextDifference, descriptor.equivalentDifference});
    registry().back().prepareComparison = descriptor.prepareComparison;
}

DifferenceEquivalenceFn differenceEquivalence(std::string_view key)
{
    for (const auto& registered : registry()) {
        if (registered.key == key) return registered.equivalentDifference;
    }
    throw std::logic_error("unregistered coverage surveyor key: " + std::string(key));
}

void runComparisonPreparers(ComparisonPreparationContext& context)
{
    for (const auto& registered : registry()) {
        if (registered.prepareComparison) registered.prepareComparison(context);
    }
}

TextDifferenceClassifierFn textDifferenceClassifier(std::string_view key)
{
    for (const auto& registered : registry()) {
        if (registered.key == key) return registered.classifyTextDifference;
    }
    throw std::logic_error("unregistered coverage surveyor key: " + std::string(key));
}

DifferenceClassifierFn differenceClassifier(std::string_view key)
{
    for (const auto& registered : registry()) {
        if (registered.key == key) return registered.classifyDifference;
    }
    throw std::logic_error("unregistered coverage surveyor key: " + std::string(key));
}

std::string_view surveyorPool(std::string_view key)
{
    for (const auto& registered : registry()) {
        if (registered.key == key) return registered.pool;
    }
    throw std::logic_error("unregistered coverage surveyor key: " + std::string(key));
}

SurveyResult runAllSurveyors(const SurveyContext& ctx)
{
    SurveyResult result;
    const auto allStarted = std::chrono::steady_clock::now();
    for (const auto& registered : registry()) {
        const auto& key = registered.key;
        const auto fn = registered.fn;
        const auto surveyorStarted = std::chrono::steady_clock::now();
        try {
            result.snapshot.insert_or_assign(key, fn(ctx));
        } catch (const std::exception& error) {
            result.snapshot.insert_or_assign(key, Value{});
            result.errors.emplace(key, error.what());
        }
        const std::chrono::duration<double, std::milli> elapsed =
            std::chrono::steady_clock::now() - surveyorStarted;
        result.timings.surveyors.emplace_back(key, elapsed.count());
    }
    const std::chrono::duration<double, std::milli> elapsed =
        std::chrono::steady_clock::now() - allStarted;
    result.timings.durationMs = elapsed.count();
    return result;
}

} // namespace coverage
} // namespace finale_mus_reader
