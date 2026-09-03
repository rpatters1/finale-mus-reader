// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeLayerAttributes(const SurveyContext& ctx)
{
    using Target = musx::dom::others::LayerAttributes;
    Value::Array result;
    for (const auto& layer : sourceInstances<Target>(ctx)) {
        result.push_back(observe(*layer, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("rest_offset", &Target::restOffset),
            field("origin_restOffset", [](const Target& value, const SurveyContext& context) {
                return fieldOrigin<Target>(context, "restOffset", value.getCmper());
            })));
    }
    return Value(std::move(result));
}

COVERAGE_SURVEYOR("others", "layer_atts", observeLayerAttributes);

} // namespace
