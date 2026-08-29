// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

#include <array>
#include <string_view>

namespace {

using namespace finale_mus_reader::coverage;

constexpr std::string_view flagOptionsUpHAdjLeaf = "up_h_adj";
constexpr std::string_view flagOptionsUpHAdj2Leaf = "up_h_adj2";
constexpr std::string_view flagOptionsUpHAdj16Leaf = "up_h_adj16";
constexpr std::string_view flagOptionsEighthFlagHoistLeaf = "eighth_flag_hoist";
constexpr std::string_view flagOptionsUpVAdjLeaf = "up_v_adj";
constexpr std::string_view flagOptionsDownVAdjLeaf = "down_v_adj";
constexpr std::string_view flagOptionsUpVAdj2Leaf = "up_v_adj2";
constexpr std::string_view flagOptionsDownVAdj2Leaf = "down_v_adj2";
constexpr std::string_view flagOptionsUpVAdj16Leaf = "up_v_adj16";
constexpr std::string_view flagOptionsDownVAdj16Leaf = "down_v_adj16";
constexpr std::string_view flagOptionsStDownVAdjLeaf = "st_down_v_adj";
constexpr std::string_view flagOptionsFlagSpacingLeaf = "flag_spacing";
constexpr std::string_view flagOptionsSecondaryGroupAdjLeaf = "secondary_group_adj";

constexpr std::array flagOptionsDefaultDifferenceLeaves{
    flagOptionsUpHAdjLeaf,
    flagOptionsUpHAdj2Leaf,
    flagOptionsUpHAdj16Leaf,
    flagOptionsEighthFlagHoistLeaf,
    flagOptionsUpVAdjLeaf,
    flagOptionsDownVAdjLeaf,
    flagOptionsUpVAdj2Leaf,
    flagOptionsDownVAdj2Leaf,
    flagOptionsUpVAdj16Leaf,
    flagOptionsDownVAdj16Leaf,
    flagOptionsStDownVAdjLeaf,
    flagOptionsFlagSpacingLeaf,
    flagOptionsSecondaryGroupAdjLeaf,
};

bool isFlagOptionsLeaf(std::string_view path, std::string_view leaf)
{
    constexpr std::string_view prefix = "flag_options.";
    return path.size() == prefix.size() + leaf.size() &&
        path.starts_with(prefix) && path.ends_with(leaf);
}

std::optional<DifferenceClassification>
classifyFlagOptionsDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category == Differs && context.origin == "finale27-default") {
        for (const auto leaf : flagOptionsDefaultDifferenceLeaves) {
            if (isFlagOptionsLeaf(context.path, leaf)) {
                return DifferenceClassification::DifferentDefaults;
            }
        }
    }
    return std::nullopt;
}

Value observeFlagOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::FlagOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("straight_flags", &Target::straightFlags),
        field(flagOptionsUpHAdjLeaf, &Target::upHAdj),
        field("down_h_adj", &Target::downHAdj),
        field(flagOptionsUpHAdj2Leaf, &Target::upHAdj2),
        field("down_h_adj2", &Target::downHAdj2),
        field(flagOptionsUpHAdj16Leaf, &Target::upHAdj16),
        field("down_h_adj16", &Target::downHAdj16),
        field(flagOptionsEighthFlagHoistLeaf, &Target::eighthFlagHoist),
        field("st_up_h_adj", &Target::stUpHAdj),
        field("st_down_h_adj", &Target::stDownHAdj),
        field(flagOptionsUpVAdjLeaf, &Target::upVAdj),
        field(flagOptionsDownVAdjLeaf, &Target::downVAdj),
        field(flagOptionsUpVAdj2Leaf, &Target::upVAdj2),
        field(flagOptionsDownVAdj2Leaf, &Target::downVAdj2),
        field(flagOptionsUpVAdj16Leaf, &Target::upVAdj16),
        field(flagOptionsDownVAdj16Leaf, &Target::downVAdj16),
        field("st_up_v_adj", &Target::stUpVAdj),
        field(flagOptionsStDownVAdjLeaf, &Target::stDownVAdj),
        field(flagOptionsFlagSpacingLeaf, &Target::flagSpacing),
        field(flagOptionsSecondaryGroupAdjLeaf, &Target::secondaryGroupAdj));
    for (const auto* member : {"straightFlags", "upHAdj", "downHAdj", "upHAdj2",
             "downHAdj2", "upHAdj16", "downHAdj16", "eighthFlagHoist", "stUpHAdj",
             "stDownHAdj", "upVAdj", "downVAdj", "upVAdj2", "downVAdj2", "upVAdj16",
             "downVAdj16", "stUpVAdj", "stDownVAdj", "flagSpacing", "secondaryGroupAdj"}) {
        result.asObject().emplace(std::string("origin_") + member,
            fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_CLASS("options", "flag_options", observeFlagOptions,
    classifyFlagOptionsDifference);

} // namespace
