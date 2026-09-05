// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/classification_rules.h"
#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeKeySymbolListElements(const SurveyContext& ctx)
{
    using Target = musx::dom::details::KeySymbolListElement;
    Value::Array result;
    for (const auto& value : sourceInstances<Target>(ctx)) {
        result.emplace_back(observe(*value, ctx,
            field("cmper1", [](const Target& item) { return item.getCmper1(); }),
            field("cmper2", [](const Target& item) { return item.getCmper2(); }),
            field("accidental_string", &Target::accidentalString),
            field("origin_accidentalString", [&ctx](const Target& item) {
                return fieldOrigin<Target>(ctx, "accidentalString", item);
            })));
    }
    return result;
}

COVERAGE_CLASS("details", keySymbolListElementsCoverageKey, observeKeySymbolListElements,
    classifyKeySymbolListDifference);

} // namespace
