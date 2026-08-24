// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The page and shape graphic assignments, whose recovered values are the tuple the source
// stores rather than a named field list. Both carry the same sixteen words plus the graphic
// they name, so one template serves both -- registered as two surveyors, one per class.

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

template <typename Target>
Value observeGraphicAssignments(const SurveyContext& ctx)
{
    Value::Array result;
    for (const auto& assign : ctx.document->getOthers()->getArray<Target>(musx::dom::SCORE_PARTID)) {
        result.push_back(observe(*assign, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("inci", [](const Target& value) { return value.getInci().value_or(0); }),
            field("left", &Target::left), field("bottom", &Target::bottom),
            field("width", &Target::width), field("height", &Target::height),
            field("f_desc_id", &Target::fDescId)));
    }
    return Value(std::move(result));
}

Value observePageGraphicAssigns(const SurveyContext& ctx)
{
    return observeGraphicAssignments<musx::dom::others::PageGraphicAssign>(ctx);
}

Value observeShapeGraphicAssigns(const SurveyContext& ctx)
{
    return observeGraphicAssignments<musx::dom::others::ShapeGraphicAssign>(ctx);
}

COVERAGE_SURVEYOR("others", "page_graphic_assigns", observePageGraphicAssigns);
COVERAGE_SURVEYOR("others", "shape_graphic_assigns", observeShapeGraphicAssigns);

} // namespace
