// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The page and shape graphic assignments, whose recovered values are the tuple the source
// stores rather than a named field list. Both carry the same sixteen words plus the graphic
// they name, so one template serves both -- registered as two surveyors, one per class.

#include <ostream>

#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

template <typename Target>
void writeGraphicAssignments(std::ostream& out, const SurveyContext& ctx)
{
    out << '[';
    bool first = true;
    for (const auto& assign : ctx.document->getOthers()->getArray<Target>(musx::dom::SCORE_PARTID)) {
        if (!first) out << ',';
        first = false;
        out << "{\"cmper\":" << assign->getCmper()
            << ",\"inci\":" << assign->getInci().value_or(0)
            << ",\"left\":" << assign->left
            << ",\"bottom\":" << assign->bottom
            << ",\"width\":" << assign->width
            << ",\"height\":" << assign->height
            << ",\"f_desc_id\":" << assign->fDescId
            << '}';
    }
    out << ']';
}

void writePageGraphicAssigns(std::ostream& out, const SurveyContext& ctx)
{
    writeGraphicAssignments<musx::dom::others::PageGraphicAssign>(out, ctx);
}

void writeShapeGraphicAssigns(std::ostream& out, const SurveyContext& ctx)
{
    writeGraphicAssignments<musx::dom::others::ShapeGraphicAssign>(out, ctx);
}

COVERAGE_SURVEYOR("page_graphic_assigns", writePageGraphicAssigns);
COVERAGE_SURVEYOR("shape_graphic_assigns", writeShapeGraphicAssigns);

} // namespace
