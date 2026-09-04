// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <string>

#include "coverage/classification_rules.h"
#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeNoteRestOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::NoteRestOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};

    auto result = observe(
        *options, ctx, field("do_shape_notes", &Target::doShapeNotes),
        field("do_cross_staff_notes", &Target::doCrossStaffNotes),
        field(noteRestDrop8thLeaf, &Target::drop8thRest),
        field(noteRestDrop16thLeaf, &Target::drop16thRest),
        field(noteRestDrop32ndLeaf, &Target::drop32ndRest),
        field(noteRestDrop64thLeaf, &Target::drop64thRest),
        field(noteRestDrop128thLeaf, &Target::drop128thRest),
        field("scale_manual_positioning", &Target::scaleManualPositioning),
        field("draw_outline", &Target::drawOutline),
        originField<Target>("origin_doShapeNotes", "doShapeNotes"),
        originField<Target>("origin_doCrossStaffNotes", "doCrossStaffNotes"),
        originField<Target>("origin_drop8thRest", "drop8thRest"),
        originField<Target>("origin_drop16thRest", "drop16thRest"),
        originField<Target>("origin_drop32ndRest", "drop32ndRest"),
        originField<Target>("origin_drop64thRest", "drop64thRest"),
        originField<Target>("origin_drop128thRest", "drop128thRest"),
        originField<Target>("origin_scaleManualPositioning", "scaleManualPositioning"),
        originField<Target>("origin_drawOutline", "drawOutline"));

    Value::Array colors;
    for (std::size_t index = 0; index < options->noteColors.size(); ++index) {
        const auto& color = options->noteColors[index];
        if (!color) {
            colors.emplace_back();
            continue;
        }
        const auto prefix = "noteColors[" + std::to_string(index) + "].";
        colors.emplace_back(Value::Object{
            {"red", color->red},
            {"green", color->green},
            {"blue", color->blue},
            {"origin_red", fieldOrigin<Target>(ctx, prefix + "red")},
            {"origin_green", fieldOrigin<Target>(ctx, prefix + "green")},
            {"origin_blue", fieldOrigin<Target>(ctx, prefix + "blue")},
        });
    }
    result.asObject().emplace("note_colors", std::move(colors));
    return result;
}

COVERAGE_CLASS("options", "note_rest_options", observeNoteRestOptions,
    classifyNoteRestOptionsDifference);

} // namespace
