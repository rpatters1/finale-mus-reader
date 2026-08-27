// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The custom line styles. Which parameter block a record carries is the
// record's own statement, so the block is named rather than flattened: a
// comparison that flattened it could not tell a solid line's width from a
// dashed line's. The character is a code point on both sides by the time it is
// written here.

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"
#include <cstdint>

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyCustomLineDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category == Differs && context.origin == "finale27-default" &&
        (comparisonPathEndsWith(context.path, ".solid_width") ||
         comparisonPathEndsWith(context.path, ".char_font_size"))) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

Value observeSmartShapeCustomLines(const SurveyContext& ctx)
{
    using CustomLine = musx::dom::others::SmartShapeCustomLine;
    Value::Array result;
    for (const auto& line :
         ctx.document->getOthers()->getArray<CustomLine>(musx::dom::SCORE_PARTID)) {
        result.emplace_back(observe(
            *line, ctx, field("cmper", [](const CustomLine& value) { return value.getCmper(); }),
            field("line_style", &CustomLine::lineStyle),
            field("cap_start_type", &CustomLine::lineCapStartType),
            field("cap_end_type", &CustomLine::lineCapEndType),
            field("cap_start_arrow_id", &CustomLine::lineCapStartArrowId),
            field("cap_end_arrow_id", &CustomLine::lineCapEndArrowId),
            field("cap_start_hook_length", &CustomLine::lineCapStartHookLength),
            field("cap_end_hook_length", &CustomLine::lineCapEndHookLength),
            field("make_horz", &CustomLine::makeHorz),
            field("line_after_left_start_text", &CustomLine::lineAfterLeftStartText),
            field("line_before_right_end_text", &CustomLine::lineBeforeRightEndText),
            field("line_after_left_cont_text", &CustomLine::lineAfterLeftContText),
            field("left_start_raw_text_id", &CustomLine::leftStartRawTextId),
            field("left_cont_raw_text_id", &CustomLine::leftContRawTextId),
            field("right_end_raw_text_id", &CustomLine::rightEndRawTextId),
            field("center_full_raw_text_id", &CustomLine::centerFullRawTextId),
            field("center_abbr_raw_text_id", &CustomLine::centerAbbrRawTextId),
            field("left_start_x", &CustomLine::leftStartX),
            field("left_start_y", &CustomLine::leftStartY),
            field("left_cont_x", &CustomLine::leftContX),
            field("left_cont_y", &CustomLine::leftContY),
            field("right_end_x", &CustomLine::rightEndX),
            field("right_end_y", &CustomLine::rightEndY),
            field("center_full_x", &CustomLine::centerFullX),
            field("center_full_y", &CustomLine::centerFullY),
            field("center_abbr_x", &CustomLine::centerAbbrX),
            field("center_abbr_y", &CustomLine::centerAbbrY),
            field("line_start_x", &CustomLine::lineStartX),
            field("line_start_y", &CustomLine::lineStartY),
            field("line_end_x", &CustomLine::lineEndX), field("line_end_y", &CustomLine::lineEndY),
            field("line_cont_x", &CustomLine::lineContX),
            field("line_char",
                  [](const CustomLine& value) {
                      return value.charParams
                                 ? static_cast<std::uint32_t>(value.charParams->lineChar)
                                 : 0;
                  }),
            field("char_font_id",
                  [](const CustomLine& value) {
                      return value.charParams ? value.charParams->font->fontId : 0;
                  }),
            field("char_font_size",
                  [](const CustomLine& value) {
                      return value.charParams ? value.charParams->font->fontSize : 0;
                  }),
            field("char_font_bold",
                  [](const CustomLine& value) {
                      return value.charParams && value.charParams->font->bold;
                  }),
            field("char_font_italic",
                  [](const CustomLine& value) {
                      return value.charParams && value.charParams->font->italic;
                  }),
            field("char_font_underline",
                  [](const CustomLine& value) {
                      return value.charParams && value.charParams->font->underline;
                  }),
            field("char_font_strikeout",
                  [](const CustomLine& value) {
                      return value.charParams && value.charParams->font->strikeout;
                  }),
            field("char_font_absolute",
                  [](const CustomLine& value) {
                      return value.charParams && value.charParams->font->absolute;
                  }),
            field("char_font_hidden",
                  [](const CustomLine& value) {
                      return value.charParams && value.charParams->font->hidden;
                  }),
            field("char_baseline_shift_ems",
                  [](const CustomLine& value) {
                      return value.charParams ? value.charParams->baselineShiftEms : 0;
                  }),
            field("solid_width",
                  [](const CustomLine& value) {
                      return value.solidParams ? value.solidParams->lineWidth : 0;
                  }),
            field("dashed_width",
                  [](const CustomLine& value) {
                      return value.dashedParams ? value.dashedParams->lineWidth : 0;
                  }),
            field("dash_on",
                  [](const CustomLine& value) {
                      return value.dashedParams ? value.dashedParams->dashOn : 0;
                  }),
            field("dash_off", [](const CustomLine& value) {
                return value.dashedParams ? value.dashedParams->dashOff : 0;
            })));
    }
    return result;
}

COVERAGE_CLASS("others", "ss_line_styles", observeSmartShapeCustomLines,
               classifyCustomLineDifference);

} // namespace
