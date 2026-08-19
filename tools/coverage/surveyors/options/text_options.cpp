// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// TextOptions. The two line-spacing members are optional and mutually exclusive, so both are
// emitted and a null means the document did not state that spelling. Each symbol insert emits
// its font by comparator and by resolved name: comparators are renumbered between the legacy
// pool and the companion, so only the name is comparable across the two.

#include <cstdint>
#include <ostream>
#include <string>
#include <utility>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeTextOptions(std::ostream& out, const SurveyContext& ctx)
{
    const auto options = ctx.document->getOptions()->get<musx::dom::options::TextOptions>();
    if (!options) {
        out << "null";
        return;
    }
    out << '{';
    if (options->textLineSpacingPercent) {
        out << "\"line_spacing_percent\":" << *options->textLineSpacingPercent;
    } else {
        out << "\"line_spacing_percent\":null";
    }
    if (options->textLineSpacingEvpu) {
        out << ",\"line_spacing_evpu\":" << *options->textLineSpacingEvpu;
    } else {
        out << ",\"line_spacing_evpu\":null";
    }
    out << ",\"show_time_seconds\":" << jsonBool(options->showTimeSeconds)
        << ",\"date_format\":" << static_cast<int>(options->dateFormat)
        << ",\"tab_spaces\":" << options->tabSpaces
        << ",\"text_tracking\":" << options->textTracking
        << ",\"text_baseline_shift\":" << options->textBaselineShift
        << ",\"text_superscript\":" << options->textSuperscript
        << ",\"text_word_wrap\":" << jsonBool(options->textWordWrap)
        << ",\"text_page_offset\":" << options->textPageOffset
        << ",\"text_justify\":" << static_cast<int>(options->textJustify)
        << ",\"text_expand_single_word\":" << jsonBool(options->textExpandSingleWord)
        << ",\"text_horz_align\":" << static_cast<int>(options->textHorzAlign)
        << ",\"text_vert_align\":" << static_cast<int>(options->textVertAlign)
        << ",\"text_is_edge_aligned\":" << jsonBool(options->textIsEdgeAligned);

    for (const auto* member : {"textLineSpacingPercent", "textLineSpacingEvpu",
             "showTimeSeconds", "dateFormat", "tabSpaces", "textTracking",
             "textBaselineShift", "textSuperscript", "textWordWrap", "textPageOffset",
             "textJustify", "textExpandSingleWord", "textHorzAlign", "textVertAlign",
             "textIsEdgeAligned"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(ctx.fields.originOf(std::string("options.textOptions.") + member));
    }

    using Insert = musx::dom::options::AccidentalInsertSymbolType;
    static const std::pair<Insert, const char*> insertOrder[] = {
        {Insert::Sharp, "sharp"}, {Insert::Flat, "flat"}, {Insert::Natural, "natural"},
        {Insert::DblSharp, "dblSharp"}, {Insert::DblFlat, "dblFlat"}};
    out << ",\"inserts\":[";
    bool first = true;
    for (const auto& [type, name] : insertOrder) {
        const auto found = options->symbolInserts.find(type);
        out << (first ? "" : ",") << "{\"type\":" << jsonString(name);
        first = false;
        if (found == options->symbolInserts.end() || !found->second) {
            out << ",\"present\":false}";
            continue;
        }
        const auto& insert = *found->second;
        std::string fontName;
        bool dangling = false;
        if (insert.symFont) {
            if (const auto definition = ctx.document->getOthers()
                    ->get<musx::dom::others::FontDefinition>(
                        musx::dom::SCORE_PARTID, insert.symFont->fontId)) {
                fontName = definition->name;
            } else {
                dangling = insert.symFont->fontId != 0;
            }
        }
        out << ",\"present\":true"
            << ",\"tracking_before\":" << insert.trackingBefore
            << ",\"tracking_after\":" << insert.trackingAfter
            << ",\"baseline_shift_perc\":" << insert.baselineShiftPerc
            << ",\"sym_char\":" << static_cast<std::uint32_t>(insert.symChar)
            << ",\"has_font\":" << jsonBool(bool(insert.symFont))
            << ",\"font_id\":" << (insert.symFont ? int(insert.symFont->fontId) : 0)
            << ",\"font_size\":" << (insert.symFont ? insert.symFont->fontSize : 0)
            << ",\"font_effects\":"
            << (insert.symFont ? int(insert.symFont->getEnigmaStyles()) : 0)
            << ",\"font_name\":" << jsonString(fontName)
            << ",\"normalized_font_name\":" << jsonString(musx::dom::normalizeFontName(fontName))
            << ",\"dangling_font\":" << jsonBool(dangling);
        const auto prefix = std::string("options.textOptions.symbolInserts[") + name + "].";
        for (const auto* member : {"trackingBefore", "trackingAfter", "baselineShiftPerc",
                 "symChar", "symFont.fontId", "symFont.fontSize", "symFont.effects"}) {
            out << ",\"origin_" << member << "\":"
                << jsonString(ctx.fields.originOf(prefix + member));
        }
        out << '}';
    }
    out << "]}";
}

COVERAGE_SURVEYOR("text_options", writeTextOptions);

} // namespace
