// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The custom line styles. Which parameter block a record carries is the record's own
// statement, so the block is named rather than flattened: a comparison that flattened it could
// not tell a solid line's width from a dashed line's. The character is a code point on both
// sides by the time it is written here.

#include <cstdint>
#include <ostream>

#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeSmartShapeCustomLines(std::ostream& out, const SurveyContext& ctx)
{
    using CustomLine = musx::dom::others::SmartShapeCustomLine;
    out << '[';
    bool first = true;
    for (const auto& line :
        ctx.document->getOthers()->getArray<CustomLine>(musx::dom::SCORE_PARTID)) {
        if (!first) out << ',';
        first = false;
        out << "{\"cmper\":" << line->getCmper()
            << ",\"line_style\":" << static_cast<int>(line->lineStyle)
            << ",\"cap_start_type\":" << static_cast<int>(line->lineCapStartType)
            << ",\"cap_end_type\":" << static_cast<int>(line->lineCapEndType)
            << ",\"cap_start_arrow_id\":" << line->lineCapStartArrowId
            << ",\"cap_end_arrow_id\":" << line->lineCapEndArrowId
            << ",\"cap_start_hook_length\":" << line->lineCapStartHookLength
            << ",\"cap_end_hook_length\":" << line->lineCapEndHookLength
            << ",\"left_start_x\":" << line->leftStartX
            << ",\"left_start_y\":" << line->leftStartY
            << ",\"line_start_x\":" << line->lineStartX
            << ",\"line_end_x\":" << line->lineEndX
            << ",\"line_cont_x\":" << line->lineContX
            << ",\"line_char\":"
            << (line->charParams ? static_cast<std::uint32_t>(line->charParams->lineChar) : 0)
            << ",\"char_font_id\":"
            << (line->charParams ? line->charParams->font->fontId : 0)
            << ",\"char_font_size\":"
            << (line->charParams ? line->charParams->font->fontSize : 0)
            << ",\"solid_width\":" << (line->solidParams ? line->solidParams->lineWidth : 0)
            << ",\"dash_on\":" << (line->dashedParams ? line->dashedParams->dashOn : 0)
            << ",\"dash_off\":" << (line->dashedParams ? line->dashedParams->dashOff : 0)
            << '}';
    }
    out << ']';
}

COVERAGE_SURVEYOR("ss_line_styles", writeSmartShapeCustomLines);

} // namespace
