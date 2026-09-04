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

COVERAGE_SURVEYOR("others", "chord_suffix_playback", observeChordSuffixPlayback);

} // namespace
