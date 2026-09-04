// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

using PartGlobalsSurveyTarget = musx::dom::others::PartGlobals;

auto partGlobalsOrigin(const char* member)
{
    return [member](const PartGlobalsSurveyTarget& value, const SurveyContext& context) {
        return fieldOrigin<PartGlobalsSurveyTarget>(context, member, value);
    };
}

Value observePartGlobals(const SurveyContext& ctx)
{
    using Target = PartGlobalsSurveyTarget;
    Value::Array result;
    for (const auto& globals : sourceInstances<Target>(ctx)) {
        result.push_back(observe(*globals, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("show_transposed", &Target::showTransposed),
            field("scroll_view_i_ulist", &Target::scrollViewIUlist),
            field("studio_view_i_ulist", &Target::studioViewIUlist),
            field("special_part_extraction_i_u_list", &Target::specialPartExtractionIUList),
            field("origin_showTransposed", partGlobalsOrigin("showTransposed")),
            field("origin_scrollViewIUlist", partGlobalsOrigin("scrollViewIUlist")),
            field("origin_studioViewIUlist", partGlobalsOrigin("studioViewIUlist")),
            field("origin_specialPartExtractionIUList",
                partGlobalsOrigin("specialPartExtractionIUList"))));
    }
    return Value(std::move(result));
}

COVERAGE_SURVEYOR("others", "part_globals", observePartGlobals);

} // namespace
