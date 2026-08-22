// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeClefOptions(std::ostream& out, const SurveyContext& ctx)
{
    const auto options = ctx.document->getOptions()->get<musx::dom::options::ClefOptions>();
    if (!options) {
        out << "null";
        return;
    }
    out << '{'
        << "\"default_clef\":" << options->defaultClef
        << ",\"clef_change_percent\":" << options->clefChangePercent
        << ",\"clef_change_offset\":" << options->clefChangeOffset
        << ",\"clef_front_separ\":" << options->clefFrontSepar
        << ",\"clef_back_separ\":" << options->clefBackSepar
        << ",\"clef_key_separ\":" << options->clefKeySepar
        << ",\"clef_time_separ\":" << options->clefTimeSepar
        << ",\"show_clef_first_system_only\":" << jsonBool(options->showClefFirstSystemOnly)
        << ",\"cautionary_clef_changes\":" << jsonBool(options->cautionaryClefChanges);

    for (const auto* member : {"defaultClef", "clefChangePercent", "clefChangeOffset",
             "clefFrontSepar", "clefBackSepar", "clefKeySepar", "clefTimeSepar",
             "showClefFirstSystemOnly", "cautionaryClefChanges"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(ctx.fields.originOf(std::string("options.clefOptions.") + member));
    }

    out << ",\"clef_defs\":[";
    for (std::size_t index = 0; index < options->clefDefs.size(); ++index) {
        const auto& def = options->clefDefs[index];
        const auto prefix = "options.clefOptions.clefDefs[" + std::to_string(index) + "].";
        std::string fontName;
        if (def->useOwnFont && def->font) {
            if (const auto font = ctx.document->getOthers()
                    ->get<musx::dom::others::FontDefinition>(
                        musx::dom::SCORE_PARTID, def->font->fontId)) {
                fontName = font->name;
            }
        }
        // A shape comparator that names no shape leaves the definition unrenderable.
        bool danglingShape = false;
        if (def->isShape && def->shapeId != 0) {
            danglingShape = !ctx.document->getOthers()->get<musx::dom::others::ShapeDef>(
                musx::dom::SCORE_PARTID, def->shapeId);
        }
        out << (index ? "," : "") << '{'
            << "\"index\":" << index
            << ",\"middle_c_pos\":" << def->middleCPos
            << ",\"clef_char\":" << static_cast<std::uint32_t>(def->clefChar)
            << ",\"staff_position\":" << def->staffPosition
            << ",\"baseline_adjust\":" << def->baselineAdjust
            << ",\"shape_id\":" << def->shapeId
            << ",\"is_shape\":" << jsonBool(def->isShape)
            << ",\"scale_to_staff_height\":" << jsonBool(def->scaleToStaffHeight)
            << ",\"use_own_font\":" << jsonBool(def->useOwnFont)
            << ",\"font_id\":" << (def->font ? def->font->fontId : 0)
            << ",\"font_size\":" << (def->font ? def->font->fontSize : 0)
            << ",\"font_name\":" << jsonString(fontName)
            << ",\"dangling_shape\":" << jsonBool(danglingShape)
            << ",\"origin_middleCPos\":" << jsonString(ctx.fields.originOf(prefix + "middleCPos"))
            << ",\"origin_clefChar\":" << jsonString(ctx.fields.originOf(prefix + "clefChar"))
            << ",\"origin_staffPosition\":"
            << jsonString(ctx.fields.originOf(prefix + "staffPosition"))
            << ",\"origin_baselineAdjust\":"
            << jsonString(ctx.fields.originOf(prefix + "baselineAdjust"))
            << ",\"origin_shapeId\":" << jsonString(ctx.fields.originOf(prefix + "shapeId"))
            << '}';
    }
    out << "]}";
}

COVERAGE_SURVEYOR("clef_options", writeClefOptions);

} // namespace
