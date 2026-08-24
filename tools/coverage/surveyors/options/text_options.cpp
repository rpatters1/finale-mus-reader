// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// TextOptions. The two line-spacing members are optional and mutually exclusive, so both are
// emitted and a null means the document did not state that spelling. Each symbol insert emits
// its font by comparator and by resolved name: comparators are renumbered between the legacy
// pool and the companion, so only the name is comparable across the two.

#include <cstdint>
#include <string>
#include <utility>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeTextOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::TextOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("line_spacing_percent", &Target::textLineSpacingPercent), field("line_spacing_evpu", &Target::textLineSpacingEvpu),
        field("show_time_seconds", &Target::showTimeSeconds), field("date_format", &Target::dateFormat),
        field("tab_spaces", &Target::tabSpaces), field("text_tracking", &Target::textTracking),
        field("text_baseline_shift", &Target::textBaselineShift), field("text_superscript", &Target::textSuperscript),
        field("text_word_wrap", &Target::textWordWrap), field("text_page_offset", &Target::textPageOffset),
        field("text_justify", &Target::textJustify), field("text_expand_single_word", &Target::textExpandSingleWord),
        field("text_horz_align", &Target::textHorzAlign), field("text_vert_align", &Target::textVertAlign),
        field("text_is_edge_aligned", &Target::textIsEdgeAligned));
    for (const auto* member : {"textLineSpacingPercent", "textLineSpacingEvpu", "showTimeSeconds", "dateFormat",
             "tabSpaces", "textTracking", "textBaselineShift", "textSuperscript", "textWordWrap", "textPageOffset",
             "textJustify", "textExpandSingleWord", "textHorzAlign", "textVertAlign", "textIsEdgeAligned"}) {
        result.asObject().emplace(std::string("origin_") + member,
            fieldOrigin<Target>(ctx, member));
    }

    using Insert = musx::dom::options::AccidentalInsertSymbolType;
    static const std::pair<Insert, const char*> insertOrder[] = {
        {Insert::Sharp, "sharp"}, {Insert::Flat, "flat"}, {Insert::Natural, "natural"},
        {Insert::DblSharp, "dblSharp"}, {Insert::DblFlat, "dblFlat"}};
    Value::Array inserts;
    for (const auto& [type, name] : insertOrder) {
        const auto found = options->symbolInserts.find(type);
        Value::Object observed{{"type", name}};
        if (found == options->symbolInserts.end() || !found->second) {
            observed.emplace("present", false);
            inserts.emplace_back(std::move(observed));
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
        observed.insert({{"present", true}, {"tracking_before", insert.trackingBefore},
            {"tracking_after", insert.trackingAfter}, {"baseline_shift_perc", insert.baselineShiftPerc},
            {"sym_char", static_cast<std::uint32_t>(insert.symChar)}, {"has_font", bool(insert.symFont)},
            {"font_id", insert.symFont ? int(insert.symFont->fontId) : 0},
            {"font_size", insert.symFont ? insert.symFont->fontSize : 0},
            {"font_effects", insert.symFont ? int(insert.symFont->getEnigmaStyles()) : 0},
            {"font_name", fontName}, {"normalized_font_name", musx::dom::normalizeFontName(fontName)},
            {"dangling_font", dangling}});
        const auto prefix = std::string("symbolInserts[") + name + "].";
        for (const auto* member : {"trackingBefore", "trackingAfter", "baselineShiftPerc",
                 "symChar", "symFont.fontId", "symFont.fontSize", "symFont.effects"}) {
            observed.emplace(std::string("origin_") + member,
                fieldOrigin<Target>(ctx, prefix + member));
        }
        inserts.emplace_back(std::move(observed));
    }
    result.asObject().emplace("inserts", std::move(inserts));
    return result;
}

COVERAGE_SURVEYOR("options", "text_options", observeTextOptions);

} // namespace
