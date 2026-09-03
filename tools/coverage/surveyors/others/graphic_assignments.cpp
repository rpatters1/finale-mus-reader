// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The page and shape graphic assignments, whose recovered values are the tuple the source
// stores rather than a named field list. Both carry the same sixteen words plus the graphic
// they name, so one template serves both -- registered as two surveyors, one per class.

#include <type_traits>
#include <utility>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

template <typename Target>
Value observeGraphicAssignments(const SurveyContext& ctx)
{
    Value::Array result;
    for (const auto& assign : sourceInstances<Target>(ctx)) {
        auto observed = observe(*assign, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("inci", [](const Target& value) { return value.getInci().value_or(0); }),
            field("version", &Target::version), field("left", &Target::left),
            field("bottom", &Target::bottom),
            field("width", &Target::width), field("height", &Target::height),
            field("f_desc_id", &Target::fDescId), field("hidden", &Target::hidden),
            field("h_align", &Target::hAlign), field("v_align", &Target::vAlign),
            field("fixed_perc", &Target::fixedPerc), field("saved_record", &Target::savedRecord),
            field("orig_width", &Target::origWidth), field("orig_height", &Target::origHeight),
            field("graphic_cmper", &Target::graphicCmper));
        if constexpr (std::is_same_v<Target, musx::dom::others::PageGraphicAssign>) {
            observed.asObject().insert({
                {"display_type", static_cast<std::int64_t>(assign->displayType)},
                {"pos_from", static_cast<std::int64_t>(assign->posFrom)},
                {"start_page", assign->startPage}, {"end_page", assign->endPage},
                {"right_pg_h_align", static_cast<std::int64_t>(assign->rightPgHAlign)},
                {"right_pg_v_align", static_cast<std::int64_t>(assign->rightPgVAlign)},
                {"right_pg_pos_from", static_cast<std::int64_t>(assign->rightPgPosFrom)},
                {"right_pg_fixed_perc", assign->rightPgFixedPerc},
                {"right_pg_left", assign->rightPgLeft},
                {"right_pg_bottom", assign->rightPgBottom}});
        }
        result.push_back(std::move(observed));
    }
    return Value(std::move(result));
}

Value observePageGraphicAssigns(const SurveyContext& ctx)
{
    return observeGraphicAssignments<musx::dom::others::PageGraphicAssign>(ctx);
}

Value observeShapeGraphicAssigns(const SurveyContext& ctx)
{
    return observeGraphicAssignments<musx::dom::others::ShapeGraphicAssign>(ctx);
}

COVERAGE_SURVEYOR("others", "page_graphic_assigns", observePageGraphicAssigns);
COVERAGE_SURVEYOR("others", "shape_graphic_assigns", observeShapeGraphicAssigns);

} // namespace
