// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The measure graphic assignments, which are details rather than others and so carry a second
// comparator. The fields are the ones both sides spell as plain numbers.

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeMeasureGraphicAssignments(const SurveyContext& ctx)
{
    using Target = musx::dom::details::MeasureGraphicAssign;
    Value::Array result;
    for (const auto& assign : ctx.document->getDetails()
            ->getArray<Target>(musx::dom::SCORE_PARTID)) {
        result.push_back(observe(*assign, ctx,
            field("cmper1", [](const Target& value) { return value.getCmper1(); }),
            field("cmper2", [](const Target& value) { return value.getCmper2(); }),
            field("inci", [](const Target& value) { return value.getInci().value_or(0); }),
            field("left", &Target::left), field("bottom", &Target::bottom),
            field("width", &Target::width), field("height", &Target::height),
            field("f_desc_id", &Target::fDescId), field("orig_width", &Target::origWidth),
            field("orig_height", &Target::origHeight)));
    }
    return Value(std::move(result));
}

COVERAGE_SURVEYOR("details", "meas_graphic_assigns", observeMeasureGraphicAssignments);

} // namespace
