// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Category and repeat staff-list components share a cmper within their family. All
// membership and forced components have the same collection shape, while names are
// scalar strings.

#include <cstddef>
#include <set>
#include <string>
#include <typeindex>
#include <utility>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/support/source_gate.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

template <typename Target>
bool hasOriginalFourLists(const finale_mus_reader::ImportReport& report)
{
    std::set<musx::dom::Cmper> cmpers;
    for (const auto& [key, origin] : report.instanceOrigins) {
        if (key.classType == std::type_index(typeid(Target))
            && key.partId == musx::dom::SCORE_PARTID && key.cmper1
            && origin == finale_mus_reader::ValueOrigin::LegacyMus) {
            cmpers.insert(*key.cmper1);
        }
    }
    static const std::set<musx::dom::Cmper> expected{1, 2, 3, 4};
    return cmpers == expected;
}

std::optional<DifferenceClassification>
classifyCategoryStaffListNameDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category != ReaderOnly || context.origin != "legacy-mus"
        || !comparisonPathStartsWith(context.path, "staff_list_category_names[cmper=")
        || !sourceIsVersion(context.epoch, context.sourceVersion,
            finale_mus_reader::FormatEpoch::ZlibLegacy,
            finale_mus_reader::versions::finale2009)
        || !sourceIsBeta(context.sourceVersion)
        || !hasOriginalFourLists<musx::dom::others::StaffListCategoryParts>(context.sourceReport)
        || !hasOriginalFourLists<musx::dom::others::StaffListCategoryScore>(context.sourceReport)) {
        return std::nullopt;
    }
    return DifferenceClassification::BetaDiscrepancy;
}

template <typename Target>
Value observeStaffLists(const SurveyContext& ctx)
{
    Value::Array result;
    for (const auto& list : sourceInstances<Target>(ctx)) {
        Value::Array values;
        for (std::size_t index = 0; index < list->values.size(); ++index) {
            const auto member = "values[" + std::to_string(index) + "]";
            values.emplace_back(Value::Object{
                {"value", list->values[index]},
                {"origin", fieldOrigin<Target>(ctx, member, *list)},
            });
        }
        result.emplace_back(observe(
            *list, ctx, field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("values", [values = std::move(values)](const Target&) {
                return Value(values);
            })));
    }
    return result;
}

template <typename Target>
Value observeStaffListNames(const SurveyContext& ctx)
{
    Value::Array result;
    for (const auto& name : sourceInstances<Target>(ctx)) {
        result.emplace_back(observe(
            *name, ctx, field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("name", &Target::name),
            field("origin_name",
                [&ctx](const Target& value) {
                    return fieldOrigin<Target>(ctx, "name", value);
                })));
    }
    return result;
}

Value observeCategoryStaffListParts(const SurveyContext& ctx)
{
    return observeStaffLists<musx::dom::others::StaffListCategoryParts>(ctx);
}

Value observeCategoryStaffListScore(const SurveyContext& ctx)
{
    return observeStaffLists<musx::dom::others::StaffListCategoryScore>(ctx);
}

Value observeCategoryStaffListNames(const SurveyContext& ctx)
{
    return observeStaffListNames<musx::dom::others::StaffListCategoryName>(ctx);
}

Value observeRepeatStaffListNames(const SurveyContext& ctx)
{
    return observeStaffListNames<musx::dom::others::StaffListRepeatName>(ctx);
}

Value observeRepeatStaffListParts(const SurveyContext& ctx)
{
    return observeStaffLists<musx::dom::others::StaffListRepeatParts>(ctx);
}

Value observeRepeatStaffListPartsForced(const SurveyContext& ctx)
{
    return observeStaffLists<musx::dom::others::StaffListRepeatPartsForced>(ctx);
}

Value observeRepeatStaffListScore(const SurveyContext& ctx)
{
    return observeStaffLists<musx::dom::others::StaffListRepeatScore>(ctx);
}

Value observeRepeatStaffListScoreForced(const SurveyContext& ctx)
{
    return observeStaffLists<musx::dom::others::StaffListRepeatScoreForced>(ctx);
}

COVERAGE_CLASS("others", "staff_list_category_names", observeCategoryStaffListNames,
               classifyCategoryStaffListNameDifference);
COVERAGE_SURVEYOR("others", "staff_list_category_parts", observeCategoryStaffListParts);
COVERAGE_SURVEYOR("others", "staff_list_category_score", observeCategoryStaffListScore);
COVERAGE_SURVEYOR("others", "staff_list_repeat_names", observeRepeatStaffListNames);
COVERAGE_SURVEYOR("others", "staff_list_repeat_parts", observeRepeatStaffListParts);
COVERAGE_SURVEYOR(
    "others", "staff_list_repeat_parts_forced", observeRepeatStaffListPartsForced);
COVERAGE_SURVEYOR("others", "staff_list_repeat_score", observeRepeatStaffListScore);
COVERAGE_SURVEYOR(
    "others", "staff_list_repeat_score_forced", observeRepeatStaffListScoreForced);

} // namespace
