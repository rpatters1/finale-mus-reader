// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <cstdint>
#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification> classifyClefDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category == Differs && context.origin == "finale27-default" &&
        comparisonPathEndsWith(context.path, "shape_id")) {
        return DifferenceClassification::DefaultShapeId;
    }
    return std::nullopt;
}

Value observeClefOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::ClefOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result =
        observe(*options, ctx, field("default_clef", &Target::defaultClef),
                field("clef_change_percent", &Target::clefChangePercent),
                field("clef_change_offset", &Target::clefChangeOffset),
                field("clef_front_separ", &Target::clefFrontSepar),
                field("clef_back_separ", &Target::clefBackSepar),
                field("clef_key_separ", &Target::clefKeySepar),
                field("clef_time_separ", &Target::clefTimeSepar),
                field("show_clef_first_system_only", &Target::showClefFirstSystemOnly),
                field("cautionary_clef_changes", &Target::cautionaryClefChanges),
                originField<Target>("origin_defaultClef", "defaultClef"),
                originField<Target>("origin_clefChangePercent", "clefChangePercent"),
                originField<Target>("origin_clefChangeOffset", "clefChangeOffset"),
                originField<Target>("origin_clefFrontSepar", "clefFrontSepar"),
                originField<Target>("origin_clefBackSepar", "clefBackSepar"),
                originField<Target>("origin_clefKeySepar", "clefKeySepar"),
                originField<Target>("origin_clefTimeSepar", "clefTimeSepar"),
                originField<Target>("origin_showClefFirstSystemOnly", "showClefFirstSystemOnly"),
                originField<Target>("origin_cautionaryClefChanges", "cautionaryClefChanges"));
    Value::Array definitions;
    for (std::size_t index = 0; index < options->clefDefs.size(); ++index) {
        const auto& def = options->clefDefs[index];
        const auto prefix = "clefDefs[" + std::to_string(index) + "].";
        std::string fontName;
        if (def->useOwnFont && def->font) {
            if (const auto font = ctx.document->getOthers()->get<musx::dom::others::FontDefinition>(
                    musx::dom::SCORE_PARTID, def->font->fontId)) {
                fontName = font->name;
            }
        }
        // A shape comparator that names no shape leaves the definition
        // unrenderable.
        bool danglingShape = false;
        if (def->isShape && def->shapeId != 0) {
            danglingShape = !ctx.document->getOthers()->get<musx::dom::others::ShapeDef>(
                musx::dom::SCORE_PARTID, def->shapeId);
        }
        definitions.emplace_back(Value::Object{
            {"index", index},
            {"middle_c_pos", def->middleCPos},
            {"clef_char", static_cast<std::uint32_t>(def->clefChar)},
            {"staff_position", def->staffPosition},
            {"baseline_adjust", def->baselineAdjust},
            {"shape_id", def->shapeId},
            {"is_shape", def->isShape},
            {"scale_to_staff_height", def->scaleToStaffHeight},
            {"use_own_font", def->useOwnFont},
            {"font_id", def->font ? def->font->fontId : 0},
            {"font_size", def->font ? def->font->fontSize : 0},
            {"font_bold", def->font && def->font->bold},
            {"font_italic", def->font && def->font->italic},
            {"font_underline", def->font && def->font->underline},
            {"font_strikeout", def->font && def->font->strikeout},
            {"font_absolute", def->font && def->font->absolute},
            {"font_hidden", def->font && def->font->hidden},
            {"origin_isShape", fieldOrigin<Target>(ctx, prefix + "flags")},
            {"origin_scaleToStaffHeight", fieldOrigin<Target>(ctx, prefix + "flags")},
            {"origin_useOwnFont", fieldOrigin<Target>(ctx, prefix + "flags")},
            {"origin_fontId", fieldOrigin<Target>(ctx, prefix + "font.fontId")},
            {"origin_fontSize", fieldOrigin<Target>(ctx, prefix + "font.fontSize")},
            {"origin_fontBold", fieldOrigin<Target>(ctx, prefix + "font.effects")},
            {"origin_fontItalic", fieldOrigin<Target>(ctx, prefix + "font.effects")},
            {"origin_fontUnderline", fieldOrigin<Target>(ctx, prefix + "font.effects")},
            {"origin_fontStrikeout", fieldOrigin<Target>(ctx, prefix + "font.effects")},
            {"origin_fontAbsolute", fieldOrigin<Target>(ctx, prefix + "font.effects")},
            {"origin_fontHidden", fieldOrigin<Target>(ctx, prefix + "font.effects")},
            {"font_name", fontName},
            {"dangling_shape", danglingShape},
            {"origin_middleCPos", fieldOrigin<Target>(ctx, prefix + "middleCPos")},
            {"origin_clefChar", fieldOrigin<Target>(ctx, prefix + "clefChar")},
            {"origin_staffPosition", fieldOrigin<Target>(ctx, prefix + "staffPosition")},
            {"origin_baselineAdjust", fieldOrigin<Target>(ctx, prefix + "baselineAdjust")},
            {"origin_shapeId", fieldOrigin<Target>(ctx, prefix + "shapeId")}});
    }
    result.asObject().emplace("clef_defs", std::move(definitions));
    return result;
}

COVERAGE_CLASS("options", "clef_options", observeClefOptions, classifyClefDifference);

} // namespace
