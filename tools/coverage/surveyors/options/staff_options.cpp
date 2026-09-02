// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace
{

using namespace finale_mus_reader::coverage;
using StaffOptionsTarget = musx::dom::options::StaffOptions;
using StaffNamePositioning = musx::dom::others::NamePositioning;

std::optional<DifferenceClassification>
classifyStaffOptionsDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    const bool isVertical = context.path == "staff_options.name_pos.vert_off" ||
                            context.path == "staff_options.name_pos_abbrv.vert_off";
    if (context.category == Differs &&
        context.epoch == finale_mus_reader::FormatEpoch::UncompressedLegacy && isVertical &&
        context.origin == "legacy-mus-adjusted")
    {
        return DifferenceClassification::FontMetricApproximation;
    }

    if (context.category != Differs ||
        context.epoch != finale_mus_reader::FormatEpoch::CodaBanner ||
        context.origin != "legacy-behavior" || !context.sourceValue.isInteger() ||
        !context.companionValue.isInteger())
    {
        return std::nullopt;
    }

    const auto sourceValue = context.sourceValue.asInteger();
    const bool isHorizontal = context.path == "staff_options.name_pos.horz_off" ||
                              context.path == "staff_options.name_pos_abbrv.horz_off";
    if ((isVertical && sourceValue == -27) || (isHorizontal && sourceValue == -192))
    {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

Value observeStaffNamePosition(const StaffNamePositioning& position, const SurveyContext& context,
                               std::string_view memberPrefix)
{
    auto result = observe(position, context, field("horz_off", &StaffNamePositioning::horzOff),
                          field("vert_off", &StaffNamePositioning::vertOff),
                          field("justify", &StaffNamePositioning::justify),
                          field("indiv_pos", &StaffNamePositioning::indivPos),
                          field("h_align", &StaffNamePositioning::hAlign),
                          field("expand", &StaffNamePositioning::expand),
                          field("hidden", &StaffNamePositioning::hidden));
    auto& fields = result.asObject();
    for (const auto& [output, member] :
         {std::pair{"origin_horzOff", "horzOff"}, std::pair{"origin_vertOff", "vertOff"},
          std::pair{"origin_justify", "justify"}, std::pair{"origin_indivPos", "indivPos"},
          std::pair{"origin_hAlign", "hAlign"}, std::pair{"origin_expand", "expand"},
          std::pair{"origin_hidden", "hidden"}})
    {
        fields.emplace(output, fieldOrigin<StaffOptionsTarget>(
                                   context, std::string(memberPrefix).append(member)));
    }
    return result;
}

Value observeStaffOptions(const SurveyContext& context)
{
    const auto options = context.document->getOptions()->get<StaffOptionsTarget>();
    if (!options || !options->namePos || !options->namePosAbbrv || !options->groupNameFullPos ||
        !options->groupNameAbbrvPos)
    {
        return {};
    }

    auto result = observe(
        *options, context, field("staff_separation", &StaffOptionsTarget::staffSeparation),
        field("staff_separ_incr", &StaffOptionsTarget::staffSeparIncr),
        field("auto_adjust_staff_separ", &StaffOptionsTarget::autoAdjustStaffSepar),
        originField<StaffOptionsTarget>("origin_staffSeparation", "staffSeparation"),
        originField<StaffOptionsTarget>("origin_staffSeparIncr", "staffSeparIncr"),
        originField<StaffOptionsTarget>("origin_autoAdjustStaffSepar", "autoAdjustStaffSepar"));
    auto& fields = result.asObject();
    fields.emplace("name_pos", observeStaffNamePosition(*options->namePos, context, "namePos."));
    fields.emplace("name_pos_abbrv",
                   observeStaffNamePosition(*options->namePosAbbrv, context, "namePosAbbrv."));
    fields.emplace("group_name_full_pos", observeStaffNamePosition(*options->groupNameFullPos,
                                                                   context, "groupNameFullPos."));
    fields.emplace("group_name_abbrv_pos", observeStaffNamePosition(*options->groupNameAbbrvPos,
                                                                    context, "groupNameAbbrvPos."));
    return result;
}

COVERAGE_CLASS("options", "staff_options", observeStaffOptions, classifyStaffOptionsDifference);

} // namespace
