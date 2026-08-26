// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeFretInstruments(const SurveyContext& ctx)
{
    using Target = musx::dom::others::FretInstrument;
    Value::Array result;
    for (const auto& instrument : ctx.document->getOthers()
            ->getArray<Target>(musx::dom::SCORE_PARTID)) {
        Value::Array strings;
        for (std::size_t index = 0; index < instrument->strings.size(); ++index) {
            const auto prefix = "strings[" + std::to_string(index) + "].";
            strings.emplace_back(Value::Object{
                {"pitch", instrument->strings[index]->pitch},
                {"nut_offset", instrument->strings[index]->nutOffset},
                {"origin_pitch", fieldOrigin<Target>(ctx,
                    prefix + "pitch", instrument->getCmper())},
                {"origin_nutOffset", fieldOrigin<Target>(ctx,
                    prefix + "nutOffset", instrument->getCmper())}});
        }
        Value::Array fretSteps;
        for (std::size_t index = 0; index < instrument->fretSteps.size(); ++index) {
            fretSteps.emplace_back(Value::Object{
                {"fret", instrument->fretSteps[index]},
                {"origin", fieldOrigin<Target>(ctx,
                    "fretSteps[" + std::to_string(index) + "]", instrument->getCmper())}});
        }
        result.emplace_back(observe(*instrument, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("num_frets", &Target::numFrets),
            field("num_strings", &Target::numStrings), field("name", &Target::name),
            field("strings", [strings = std::move(strings)](const Target&) {
                return Value(strings);
            }),
            field("fret_steps", [fretSteps = std::move(fretSteps)](const Target&) {
                return Value(fretSteps);
            }),
            field("speedy_clef", &Target::speedyClef),
            field("origin_num_frets", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "numFrets", value.getCmper());
            }),
            field("origin_num_strings", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "numStrings", value.getCmper());
            }),
            field("origin_speedy_clef", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "speedyClef", value.getCmper());
            })));
    }
    return result;
}

COVERAGE_SURVEYOR("others", "fret_instruments", observeFretInstruments);

} // namespace
