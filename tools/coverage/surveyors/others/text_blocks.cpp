// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeTextBlocks(const SurveyContext& ctx)
{
    using TextBlock = musx::dom::others::TextBlock;

    Value::Array result;
    for (const auto& block :
         ctx.document->getOthers()->getArray<TextBlock>(musx::dom::SCORE_PARTID)) {
        result.push_back(observe(*block, ctx,
            field("cmper", [](const TextBlock& value) { return value.getCmper(); }),
            field("text_id", &TextBlock::textId), field("shape_id", &TextBlock::shapeId),
            field("width", &TextBlock::width), field("height", &TextBlock::height),
            field("line_spacing_percent", &TextBlock::lineSpacingPercentage),
            field("line_spacing_evpu", &TextBlock::lineSpacingEvpu),
            field("x_add", &TextBlock::xAdd), field("y_add", &TextBlock::yAdd),
            field("justify", &TextBlock::justify), field("new_pos_36", &TextBlock::newPos36),
            field("show_shape", &TextBlock::showShape),
            field("no_expand_single_word", &TextBlock::noExpandSingleWord),
            field("word_wrap", &TextBlock::wordWrap), field("inset", &TextBlock::inset),
            field("standard_line", &TextBlock::stdLineThickness),
            field("round_corners", &TextBlock::roundCorners),
            field("corner_radius", &TextBlock::cornerRadius),
            field("text_type", &TextBlock::textType),
            field("origin_text_id", [](const TextBlock& value, const SurveyContext& context) {
                return fieldOrigin<TextBlock>(context, "textId", value.getCmper());
            }),
            field("origin_justify", [](const TextBlock& value, const SurveyContext& context) {
                return fieldOrigin<TextBlock>(context, "justify", value.getCmper());
            }),
            field("origin_newPos36",
                [](const TextBlock& value, const SurveyContext& context) {
                    return fieldOrigin<TextBlock>(context, "newPos36", value.getCmper());
                }),
            field("origin_noExpandSingleWord",
                [](const TextBlock& value, const SurveyContext& context) {
                    return fieldOrigin<TextBlock>(
                        context, "noExpandSingleWord", value.getCmper());
                }),
            field("origin_round_corners",
                [](const TextBlock& value, const SurveyContext& context) {
                    return fieldOrigin<TextBlock>(context, "roundCorners", value.getCmper());
                }),
            field("origin_corner_radius",
                [](const TextBlock& value, const SurveyContext& context) {
                    return fieldOrigin<TextBlock>(context, "cornerRadius", value.getCmper());
                })));
    }
    return Value(std::move(result));
}

COVERAGE_SURVEYOR("others", "text_blocks", observeTextBlocks);

} // namespace
