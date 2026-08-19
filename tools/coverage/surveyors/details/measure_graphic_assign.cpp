// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The measure graphic assignments, which are details rather than others and so carry a second
// comparator. The fields are the ones both sides spell as plain numbers.

#include <ostream>

#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeMeasureGraphicAssignments(std::ostream& out, const SurveyContext& ctx)
{
    out << '[';
    bool first = true;
    for (const auto& assign : ctx.document->getDetails()
            ->getArray<musx::dom::details::MeasureGraphicAssign>(musx::dom::SCORE_PARTID)) {
        if (!first) out << ',';
        first = false;
        out << "{\"cmper1\":" << assign->getCmper1()
            << ",\"cmper2\":" << assign->getCmper2()
            << ",\"inci\":" << assign->getInci().value_or(0)
            << ",\"left\":" << assign->left
            << ",\"bottom\":" << assign->bottom
            << ",\"width\":" << assign->width
            << ",\"height\":" << assign->height
            << ",\"f_desc_id\":" << assign->fDescId
            << ",\"orig_width\":" << assign->origWidth
            << ",\"orig_height\":" << assign->origHeight
            << '}';
    }
    out << ']';
}

COVERAGE_SURVEYOR("meas_graphic_assigns", writeMeasureGraphicAssignments);

} // namespace
