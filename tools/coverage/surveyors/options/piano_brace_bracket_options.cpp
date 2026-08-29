// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyPianoBraceBracketDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category == Differs &&
        context.epoch == finale_mus_reader::FormatEpoch::CodaBanner &&
        context.origin == "legacy-mus" &&
        (context.path == "piano_brace_bracket_options.inner_tip_h" ||
         context.path == "piano_brace_bracket_options.inner_body_h")) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    if (context.category == Differs &&
        context.path == "piano_brace_bracket_options.width" &&
        context.origin == "legacy-behavior") {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

Value observePianoBraceBracketOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::PianoBraceBracketOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(
        *options, ctx, field("def_bracket_pos", &Target::defBracketPos),
        field("center_thickness", &Target::centerThickness),
        field("tip_thickness", &Target::tipThickness), field("outer_body_v", &Target::outerBodyV),
        field("inner_tip_v", &Target::innerTipV), field("inner_body_v", &Target::innerBodyV),
        field("outer_tip_h", &Target::outerTipH), field("outer_tip_v", &Target::outerTipV),
        field("outer_body_h", &Target::outerBodyH), field("width", &Target::width),
        field("inner_tip_h", &Target::innerTipH), field("inner_body_h", &Target::innerBodyH));
    for (const auto* member : {"defBracketPos", "centerThickness", "tipThickness", "outerBodyV",
                               "innerTipV", "innerBodyV", "outerTipH", "outerTipV", "outerBodyH",
                               "width", "innerTipH", "innerBodyH"}) {
        result.asObject().emplace(std::string("origin_") + member,
                                  fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_CLASS("options", "piano_brace_bracket_options", observePianoBraceBracketOptions,
               classifyPianoBraceBracketDifference);

} // namespace
