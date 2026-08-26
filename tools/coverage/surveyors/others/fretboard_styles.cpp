// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeFretboardStyles(const SurveyContext& ctx)
{
    using Target = musx::dom::others::FretboardStyle;
    Value::Array result;
    for (const auto& style : ctx.document->getOthers()
            ->getArray<Target>(musx::dom::SCORE_PARTID)) {
        result.emplace_back(observe(*style, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("show_last_fret", &Target::showLastFret),
            field("rotate", &Target::rotate), field("fing_num_white", &Target::fingNumWhite),
            field("fing_str_shape_id", &Target::fingStrShapeId),
            field("open_str_shape_id", &Target::openStrShapeId),
            field("mute_str_shape_id", &Target::muteStrShapeId),
            field("barre_shape_id", &Target::barreShapeId),
            field("custom_shape_id", &Target::customShapeId),
            field("def_num_frets", &Target::defNumFrets),
            field("string_gap", &Target::stringGap), field("fret_gap", &Target::fretGap),
            field("string_width", &Target::stringWidth), field("fret_width", &Target::fretWidth),
            field("nut_width", &Target::nutWidth), field("vert_text_off", &Target::vertTextOff),
            field("horz_text_off", &Target::horzTextOff),
            field("horz_handle_off", &Target::horzHandleOff),
            field("vert_handle_off", &Target::vertHandleOff), field("whiteout", &Target::whiteout),
            field("fret_num_font_id", [](const Target& value) { return value.fretNumFont->fontId; }),
            field("fret_num_font_size", [](const Target& value) { return value.fretNumFont->fontSize; }),
            field("fret_num_font_efx", [](const Target& value) { return value.fretNumFont->getEnigmaStyles(); }),
            field("fing_num_font_id", [](const Target& value) { return value.fingNumFont->fontId; }),
            field("fing_num_font_size", [](const Target& value) { return value.fingNumFont->fontSize; }),
            field("fing_num_font_efx", [](const Target& value) { return value.fingNumFont->getEnigmaStyles(); }),
            field("horz_fing_num_off", &Target::horzFingNumOff),
            field("vert_fing_num_off", &Target::vertFingNumOff),
            field("name", &Target::name), field("fret_num_text", &Target::fretNumText)));
    }
    return result;
}

COVERAGE_SURVEYOR("others", "fretboard_styles", observeFretboardStyles);

} // namespace
