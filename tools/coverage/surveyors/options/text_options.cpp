// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// TextOptions. The two line-spacing members are optional and mutually
// exclusive, so both are emitted and a null means the document did not state
// that spelling. Each symbol insert emits its font by comparator and by
// resolved name: comparators are renumbered between the legacy pool and the
// companion, so only the name is comparable across the two.

#include <cstdint>
#include <string>
#include <utility>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

bool equivalentTextOptionsDifference(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs ||
        !comparisonPathStartsWith(context.path, "text_options.inserts[")) {
        return false;
    }
    const auto suffix = context.path.substr(context.path.find_last_of('.'));
    const auto prefix = std::string(context.path.substr(0, context.path.find_last_of('.')));
    if (suffix == ".has_font") {
        if (!context.sourceValue.isBool() || !context.companionValue.isBool() ||
            !context.sourceValue.asBool() || context.companionValue.asBool()) {
            return false;
        }
    } else if (suffix == ".font_name" || suffix == ".normalized_font_name") {
        if (!context.sourceValue.isString() || !context.companionValue.isString() ||
            context.sourceValue.asString().empty() ||
            !context.companionValue.asString().empty()) {
            return false;
        }
    } else {
        return false;
    }
    for (const auto* field : {".font_id", ".font_size", ".font_effects"}) {
        if (comparisonIntegerLeaf(context.source, prefix + field) != 0 ||
            comparisonIntegerLeaf(context.companion, prefix + field) != 0) {
            return false;
        }
    }
    for (const auto* field : {".present", ".tracking_before", ".tracking_after",
                              ".baseline_shift_perc", ".sym_char", ".dangling_font"}) {
        const auto left = context.source.find(prefix + field);
        const auto right = context.companion.find(prefix + field);
        if (left == context.source.end() || right == context.companion.end() ||
            left->second.first != right->second.first) {
            return false;
        }
    }
    return true;
}

std::optional<DifferenceClassification>
classifyTextOptionsDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category != Differs) return std::nullopt;
    if (context.origin == "finale27-default" &&
        ((context.path == "text_options.inserts[1].tracking_before" &&
          context.sourceValue.isInteger() && context.sourceValue.asInteger() == 60 &&
          context.companionValue.isInteger() && context.companionValue.asInteger() == 50) ||
         (context.path == "text_options.inserts[2].tracking_before" &&
          context.sourceValue.isInteger() && context.sourceValue.asInteger() == 50 &&
          context.companionValue.isInteger() && context.companionValue.asInteger() == 0))) {
        return DifferenceClassification::MissingAccidentalInsertDefault;
    }
    if (comparisonPathStartsWith(context.path, "text_options.inserts[") &&
        comparisonIntegerLeaf(context.source, "text_options.inserts[0].tracking_before") == 35 &&
        comparisonIntegerLeaf(context.source, "text_options.inserts[1].tracking_before") == 50 &&
        comparisonIntegerLeaf(context.source, "text_options.inserts[2].tracking_before") == 0 &&
        comparisonIntegerLeaf(context.source, "text_options.inserts[3].tracking_before") == 40 &&
        comparisonIntegerLeaf(context.source, "text_options.inserts[4].tracking_before") == 60 &&
        comparisonIntegerLeaf(context.companion, "text_options.inserts[0].tracking_before") ==
            2293760 &&
        comparisonIntegerLeaf(context.companion, "text_options.inserts[1].tracking_before") ==
            587202560 &&
        comparisonIntegerLeaf(context.companion, "text_options.inserts[2].tracking_before") == 0 &&
        comparisonIntegerLeaf(context.companion, "text_options.inserts[3].tracking_before") ==
            1845493760 &&
        comparisonIntegerLeaf(context.companion, "text_options.inserts[4].tracking_before") ==
            3932160) {
        return DifferenceClassification::AccidentalInsert17Byte;
    }
    return std::nullopt;
}

Value observeTextOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::TextOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result =
        observe(*options, ctx, field("line_spacing_percent", &Target::textLineSpacingPercent),
                field("line_spacing_evpu", &Target::textLineSpacingEvpu),
                field("show_time_seconds", &Target::showTimeSeconds),
                field("date_format", &Target::dateFormat), field("tab_spaces", &Target::tabSpaces),
                field("text_tracking", &Target::textTracking),
                field("text_baseline_shift", &Target::textBaselineShift),
                field("text_superscript", &Target::textSuperscript),
                field("text_word_wrap", &Target::textWordWrap),
                field("text_page_offset", &Target::textPageOffset),
                field("text_justify", &Target::textJustify),
                field("text_expand_single_word", &Target::textExpandSingleWord),
                field("text_horz_align", &Target::textHorzAlign),
                field("text_vert_align", &Target::textVertAlign),
                field("text_is_edge_aligned", &Target::textIsEdgeAligned));
    for (const auto* member :
         {"textLineSpacingPercent", "textLineSpacingEvpu", "showTimeSeconds", "dateFormat",
          "tabSpaces", "textTracking", "textBaselineShift", "textSuperscript", "textWordWrap",
          "textPageOffset", "textJustify", "textExpandSingleWord", "textHorzAlign", "textVertAlign",
          "textIsEdgeAligned"}) {
        result.asObject().emplace(std::string("origin_") + member,
                                  fieldOrigin<Target>(ctx, member));
    }

    using Insert = musx::dom::options::AccidentalInsertSymbolType;
    static const std::pair<Insert, const char*> insertOrder[] = {{Insert::Sharp, "sharp"},
                                                                 {Insert::Flat, "flat"},
                                                                 {Insert::Natural, "natural"},
                                                                 {Insert::DblSharp, "dblSharp"},
                                                                 {Insert::DblFlat, "dblFlat"}};
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
            if (const auto definition =
                    ctx.document->getOthers()->get<musx::dom::others::FontDefinition>(
                        musx::dom::SCORE_PARTID, insert.symFont->fontId)) {
                fontName = definition->name;
            } else {
                dangling = insert.symFont->fontId != 0;
            }
        }
        observed.insert(
            {{"present", true},
             {"tracking_before", insert.trackingBefore},
             {"tracking_after", insert.trackingAfter},
             {"baseline_shift_perc", insert.baselineShiftPerc},
             {"sym_char", static_cast<std::uint32_t>(insert.symChar)},
             {"has_font", bool(insert.symFont)},
             {"font_id", insert.symFont ? int(insert.symFont->fontId) : 0},
             {"font_size", insert.symFont ? insert.symFont->fontSize : 0},
             {"font_effects", insert.symFont ? int(insert.symFont->getEnigmaStyles()) : 0},
             {"font_bold", insert.symFont && insert.symFont->bold},
             {"font_italic", insert.symFont && insert.symFont->italic},
             {"font_underline", insert.symFont && insert.symFont->underline},
             {"font_strikeout", insert.symFont && insert.symFont->strikeout},
             {"font_absolute", insert.symFont && insert.symFont->absolute},
             {"font_hidden", insert.symFont && insert.symFont->hidden},
             {"font_name", fontName},
             {"normalized_font_name", musx::dom::normalizeFontName(fontName)},
             {"dangling_font", dangling}});
        const auto prefix = std::string("symbolInserts[") + name + "].";
        for (const auto* member :
             {"trackingBefore", "trackingAfter", "baselineShiftPerc", "symChar", "symFont.fontId",
              "symFont.fontSize", "symFont.effects"}) {
            observed.emplace(std::string("origin_") + member,
                             fieldOrigin<Target>(ctx, prefix + member));
        }
        for (const auto* member :
             {"Bold", "Italic", "Underline", "Strikeout", "Absolute", "Hidden"}) {
            observed.emplace(std::string("origin_font") + member,
                             fieldOrigin<Target>(ctx, prefix + "symFont.effects"));
        }
        inserts.emplace_back(std::move(observed));
    }
    result.asObject().emplace("inserts", std::move(inserts));
    return result;
}

COVERAGE_CLASS_WITH_EQUIVALENCE("options", "text_options", observeTextOptions,
                                classifyTextOptionsDifference, equivalentTextOptionsDifference);

} // namespace
