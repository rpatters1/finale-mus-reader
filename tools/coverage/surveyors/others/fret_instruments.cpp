// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <limits>
#include <regex>
#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyFretInstrumentDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category != Differs) return std::nullopt;
    static const std::regex member(
        R"(^(fret_instruments\[(?:part_id=\d+,)?cmper=\d+\]\.strings\[\d+\])\.(pitch|nut_offset)$)");
    std::match_results<std::string_view::const_iterator> match;
    if (!std::regex_match(context.path.begin(), context.path.end(), match, member)) {
        return std::nullopt;
    }
    const std::string prefix(match[1].first, match[1].second);
    const auto sourcePitch = context.source.find(prefix + ".pitch");
    const auto sourceOffset = context.source.find(prefix + ".nut_offset");
    const auto companionPitch = context.companion.find(prefix + ".pitch");
    const auto companionOffset = context.companion.find(prefix + ".nut_offset");
    if (sourcePitch == context.source.end() || sourceOffset == context.source.end() ||
        companionPitch == context.companion.end() || companionOffset == context.companion.end() ||
        sourcePitch->second.second != "legacy-mus" ||
        sourceOffset->second.second != "legacy-behavior" ||
        !sourcePitch->second.first.isInteger() || !sourceOffset->second.first.isInteger() ||
        !companionPitch->second.first.isInteger() || !companionOffset->second.first.isInteger()) {
        return std::nullopt;
    }
    const auto oldPitch = sourcePitch->second.first.asInteger();
    const auto splitPitch = companionPitch->second.first.asInteger();
    const auto splitOffset = companionOffset->second.first.asInteger();
    if (oldPitch < (std::numeric_limits<std::int16_t>::min)() ||
        oldPitch > (std::numeric_limits<std::int16_t>::max)() ||
        sourceOffset->second.first.asInteger() != 0 || splitPitch < 0 || splitPitch > 0xff ||
        splitOffset < 0 || splitOffset > 0xff) {
        return std::nullopt;
    }
    const auto oldWord = static_cast<std::uint16_t>(oldPitch);
    const auto splitWord = static_cast<std::uint16_t>(splitPitch | (splitOffset << 8U));
    if (oldWord == splitWord) return DifferenceClassification::FinaleUpgradeLoss;
    return std::nullopt;
}

Value observeFretInstruments(const SurveyContext& ctx)
{
    using Target = musx::dom::others::FretInstrument;
    Value::Array result;
    for (const auto& instrument : sourceInstances<Target>(ctx)) {
        Value::Array strings;
        for (std::size_t index = 0; index < instrument->strings.size(); ++index) {
            const auto prefix = "strings[" + std::to_string(index) + "].";
            strings.emplace_back(
                Value::Object{{"pitch", instrument->strings[index]->pitch},
                              {"nut_offset", instrument->strings[index]->nutOffset},
                              {"origin_pitch",
                               fieldOrigin<Target>(ctx, prefix + "pitch", instrument->getCmper())},
                              {"origin_nutOffset", fieldOrigin<Target>(ctx, prefix + "nutOffset",
                                                                       instrument->getCmper())}});
        }
        Value::Array fretSteps;
        for (std::size_t index = 0; index < instrument->fretSteps.size(); ++index) {
            fretSteps.emplace_back(Value::Object{
                {"fret", instrument->fretSteps[index]},
                {"origin", fieldOrigin<Target>(ctx, "fretSteps[" + std::to_string(index) + "]",
                                               instrument->getCmper())}});
        }
        result.emplace_back(observe(
            *instrument, ctx, field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("num_frets", &Target::numFrets), field("num_strings", &Target::numStrings),
            field("name", &Target::name),
            field("strings",
                  [strings = std::move(strings)](const Target&) { return Value(strings); }),
            field("fret_steps",
                  [fretSteps = std::move(fretSteps)](const Target&) { return Value(fretSteps); }),
            field("speedy_clef", &Target::speedyClef),
            field("origin_num_frets",
                  [&ctx](const Target& value) {
                      return fieldOrigin<Target>(ctx, "numFrets", value.getCmper());
                  }),
            field("origin_num_strings",
                  [&ctx](const Target& value) {
                      return fieldOrigin<Target>(ctx, "numStrings", value.getCmper());
                  }),
            field("origin_speedy_clef", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "speedyClef", value.getCmper());
            })));
    }
    return result;
}

COVERAGE_CLASS("others", "fret_instruments", observeFretInstruments,
               classifyFretInstrumentDifference);

} // namespace
