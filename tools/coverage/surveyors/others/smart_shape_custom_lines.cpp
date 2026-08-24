// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The custom line styles. Which parameter block a record carries is the record's own
// statement, so the block is named rather than flattened: a comparison that flattened it could
// not tell a solid line's width from a dashed line's. The character is a code point on both
// sides by the time it is written here.

#include <cstdint>
#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeSmartShapeCustomLines(const SurveyContext& ctx)
{
    using CustomLine = musx::dom::others::SmartShapeCustomLine;
    Value::Array result;
    for (const auto& line :
        ctx.document->getOthers()->getArray<CustomLine>(musx::dom::SCORE_PARTID)) {
        result.emplace_back(observe(*line, ctx,
            field("cmper", [](const CustomLine& value) { return value.getCmper(); }),
            field("line_style", &CustomLine::lineStyle),
            field("cap_start_type", &CustomLine::lineCapStartType),
            field("cap_end_type", &CustomLine::lineCapEndType),
            field("cap_start_arrow_id", &CustomLine::lineCapStartArrowId),
            field("cap_end_arrow_id", &CustomLine::lineCapEndArrowId),
            field("cap_start_hook_length", &CustomLine::lineCapStartHookLength),
            field("cap_end_hook_length", &CustomLine::lineCapEndHookLength),
            field("left_start_x", &CustomLine::leftStartX), field("left_start_y", &CustomLine::leftStartY),
            field("line_start_x", &CustomLine::lineStartX), field("line_end_x", &CustomLine::lineEndX),
            field("line_cont_x", &CustomLine::lineContX),
            field("line_char", [](const CustomLine& value) { return value.charParams ? static_cast<std::uint32_t>(value.charParams->lineChar) : 0; }),
            field("char_font_id", [](const CustomLine& value) { return value.charParams ? value.charParams->font->fontId : 0; }),
            field("char_font_size", [](const CustomLine& value) { return value.charParams ? value.charParams->font->fontSize : 0; }),
            field("solid_width", [](const CustomLine& value) { return value.solidParams ? value.solidParams->lineWidth : 0; }),
            field("dash_on", [](const CustomLine& value) { return value.dashedParams ? value.dashedParams->dashOn : 0; }),
            field("dash_off", [](const CustomLine& value) { return value.dashedParams ? value.dashedParams->dashOff : 0; })));
    }
    return result;
}

COVERAGE_SURVEYOR("ss_line_styles", observeSmartShapeCustomLines);

} // namespace
