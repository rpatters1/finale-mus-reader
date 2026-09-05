// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/support/source_gate.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<std::size_t> chordSuffixPlaybackValueIndex(std::string_view path)
{
    constexpr std::string_view prefix = "chord_suffix_playback[cmper=";
    constexpr std::string_view marker = "].values[";
    constexpr std::string_view suffix = "].value";
    if (!comparisonPathStartsWith(path, prefix) || !comparisonPathEndsWith(path, suffix)) {
        return std::nullopt;
    }
    const auto begin = path.find(marker, prefix.size());
    if (begin == std::string_view::npos) return std::nullopt;
    const auto digitsBegin = begin + marker.size();
    const auto digitsEnd = path.size() - suffix.size();
    std::size_t index{};
    const auto [end, error] =
        std::from_chars(path.data() + digitsBegin, path.data() + digitsEnd, index);
    if (error != std::errc{} || end != path.data() + digitsEnd) return std::nullopt;
    return index;
}

std::optional<DifferenceClassification>
classifyChordSuffixPlaybackDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    constexpr std::size_t logicalValueLimit = 64;
    constexpr std::size_t paddedValueCount = 66;
    if (context.origin != "legacy-mus" || !sourceIsBeta(context.sourceVersion) ||
        !sourceIsVersion(context.epoch, context.sourceVersion,
                         finale_mus_reader::FormatEpoch::ZlibLegacy,
                         finale_mus_reader::versions::finale2008)) {
        return std::nullopt;
    }
    const auto index = chordSuffixPlaybackValueIndex(context.path);
    if (!index) return std::nullopt;
    if (context.category == Differs && *index >= logicalValueLimit &&
        *index < paddedValueCount &&
        context.companionValue.isInteger() && context.companionValue.asInteger() == 0) {
        return DifferenceClassification::BetaDiscrepancy;
    }
    if (context.category == ReaderOnly && *index >= paddedValueCount) {
        return DifferenceClassification::BetaDiscrepancy;
    }
    return std::nullopt;
}

Value observeChordSuffixPlayback(const SurveyContext& ctx)
{
    using Target = musx::dom::others::ChordSuffixPlayback;
    Value::Array result;
    for (const auto& playback : sourceInstances<Target>(ctx)) {
        Value::Array values;
        for (std::size_t index = 0; index < playback->values.size(); ++index) {
            const auto member = "values[" + std::to_string(index) + "]";
            values.emplace_back(Value::Object{
                {"value", playback->values[index]},
                {"origin", fieldOrigin<Target>(ctx, member, *playback)},
            });
        }
        result.emplace_back(observe(
            *playback, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("values", [values = std::move(values)](const Target&) {
                return Value(values);
            })));
    }
    return result;
}

COVERAGE_CLASS("others", "chord_suffix_playback", observeChordSuffixPlayback,
               classifyChordSuffixPlaybackDifference);

} // namespace
