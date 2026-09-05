// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <string>
#include <utility>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

template <typename Target> Value observeClefOctaveArrays(const SurveyContext& ctx)
{
    Value::Array result;
    for (const auto& item : sourceInstances<Target>(ctx)) {
        Value::Array values;
        for (std::size_t index = 0; index < item->values.size(); ++index) {
            const auto member = "values[" + std::to_string(index) + "]";
            values.emplace_back(Value::Object{
                {"value", item->values[index]},
                {"origin", fieldOrigin<Target>(ctx, member, *item)},
            });
        }
        result.emplace_back(observe(*item, ctx,
            field("cmper1", [](const Target& value) { return value.getCmper1(); }),
            field("cmper2", [](const Target& value) { return value.getCmper2(); }),
            field(
                "values", [values = std::move(values)](const Target&) { return Value(values); })));
    }
    return result;
}

Value observeClefOctaveFlats(const SurveyContext& ctx)
{
    return observeClefOctaveArrays<musx::dom::details::ClefOctaveFlats>(ctx);
}

Value observeClefOctaveSharps(const SurveyContext& ctx)
{
    return observeClefOctaveArrays<musx::dom::details::ClefOctaveSharps>(ctx);
}

COVERAGE_SURVEYOR("details", "clef_octave_flats", observeClefOctaveFlats);
COVERAGE_SURVEYOR("details", "clef_octave_sharps", observeClefOctaveSharps);

} // namespace
