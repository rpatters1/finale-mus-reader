// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <regex>
#include <string>
#include <utility>

#include "coverage/common/byte_swap.h"
#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

bool hasByteSwappedFretboardSuffix(std::string_view source, std::string_view companion)
{
    std::size_t firstDifference = 0;
    while (firstDifference < source.size() && firstDifference < companion.size() &&
           source[firstDifference] == companion[firstDifference]) {
        ++firstDifference;
    }
    std::string swapped(source.substr(firstDifference));
    swapped.push_back('\0');
    if (swapped.size() % 2 != 0) {
        if (companion.size() != source.size() + 1) return false;
        swapped.push_back(companion.back());
    }
    for (std::size_t index = 0; index < swapped.size(); index += 2) {
        std::swap(swapped[index], swapped[index + 1]);
    }
    swapped.resize(swapped.find('\0'));
    return companion == std::string(source.substr(0, firstDifference)) + swapped;
}

std::optional<DifferenceClassification>
classifyFretboardGroupDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category != Differs || context.origin != "legacy-mus" ||
        context.byteOrder != finale_mus_reader::ByteOrder::BigEndian) {
        return std::nullopt;
    }
    static const std::regex reference(
        R"(^fretboard_groups\[(?:part_id=\d+,)?cmper=\d+,inci=\d+\]\.fret_inst_id$)");
    if (std::regex_match(context.path.begin(), context.path.end(), reference) &&
        context.sourceValue.isInteger() && context.companionValue.isInteger()) {
        const auto source = context.sourceValue.asInteger();
        const auto companion = context.companionValue.asInteger();
        if (source >= 0 && source <= 0xffff && companion >= 0 && companion <= 0xffff &&
            companion == (((source & 0x00ff) << 8) | ((source & 0xff00) >> 8))) {
            return DifferenceClassification::FinaleUpgradeLoss;
        }
    }
    static const std::regex name(
        R"(^fretboard_groups\[(?:part_id=\d+,)?cmper=\d+,inci=\d+\]\.name$)");
    if (std::regex_match(context.path.begin(), context.path.end(), name) &&
        context.sourceValue.isString() && context.companionValue.isString() &&
        (hasContiguousAdjacentByteSwap(context.sourceValue.asString(),
             context.companionValue.asString()) ||
            hasByteSwappedFretboardSuffix(context.sourceValue.asString(),
                context.companionValue.asString()))) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    return std::nullopt;
}

Value observeFretboardGroups(const SurveyContext& ctx)
{
    using Target = musx::dom::others::FretboardGroup;
    Value::Array result;
    for (const auto& group : sourceInstances<Target>(ctx)) {
        result.emplace_back(observe(
            *group, ctx, field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("inci", [](const Target& value) { return value.getInci().value_or(0); }),
            field("fret_inst_id", &Target::fretInstId), field("name", &Target::name),
            field("origin_fret_inst_id", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "fretInstId", value);
            }),
            field("origin_name", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "name", value);
            })));
    }
    return result;
}

COVERAGE_CLASS("others", "fretboard_groups", observeFretboardGroups,
               classifyFretboardGroupDifference);

} // namespace
