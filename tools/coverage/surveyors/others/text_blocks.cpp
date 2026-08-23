// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <ostream>
#include <string>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeTextBlocks(std::ostream& out, const SurveyContext& ctx)
{
    using TextBlock = musx::dom::others::TextBlock;

    out << '[';
    bool first = true;
    for (const auto& block :
         ctx.document->getOthers()->getArray<TextBlock>(musx::dom::SCORE_PARTID)) {
        if (!first)
            out << ',';
        first = false;
        const auto prefix = "others.textBlock[" + std::to_string(block->getCmper()) + "].";
        out << "{\"cmper\":" << block->getCmper() << ",\"text_id\":" << block->textId
            << ",\"shape_id\":" << block->shapeId << ",\"width\":" << block->width
            << ",\"height\":" << block->height
            << ",\"line_spacing_percent\":";
        if (block->lineSpacingPercentage)
            out << *block->lineSpacingPercentage;
        else
            out << "null";
        out << ",\"line_spacing_evpu\":";
        if (block->lineSpacingEvpu)
            out << *block->lineSpacingEvpu;
        else
            out << "null";
        out << ",\"x_add\":" << block->xAdd << ",\"y_add\":" << block->yAdd
            << ",\"justify\":" << static_cast<int>(block->justify)
            << ",\"new_pos_36\":" << (block->newPos36 ? "true" : "false")
            << ",\"show_shape\":" << (block->showShape ? "true" : "false")
            << ",\"no_expand_single_word\":" << (block->noExpandSingleWord ? "true" : "false")
            << ",\"word_wrap\":" << (block->wordWrap ? "true" : "false")
            << ",\"inset\":" << block->inset << ",\"standard_line\":" << block->stdLineThickness
            << ",\"round_corners\":" << (block->roundCorners ? "true" : "false")
            << ",\"corner_radius\":" << block->cornerRadius
            << ",\"text_type\":" << static_cast<int>(block->textType)
            << ",\"origin_text_id\":" << jsonString(ctx.fields.originOf(prefix + "textId"))
            << ",\"origin_justify\":" << jsonString(ctx.fields.originOf(prefix + "justify"))
            << ",\"origin_round_corners\":"
            << jsonString(ctx.fields.originOf(prefix + "roundCorners"))
            << ",\"origin_corner_radius\":"
            << jsonString(ctx.fields.originOf(prefix + "cornerRadius")) << '}';
    }
    out << ']';
}

COVERAGE_SURVEYOR("text_blocks", writeTextBlocks);

} // namespace
