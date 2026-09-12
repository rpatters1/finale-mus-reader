// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/surveyors/shared/staff_style_semantics.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

using StaffStyleAssignSurveyTarget = musx::dom::others::StaffStyleAssign;

constexpr std::string_view staffStyleAssignCoverageKey = "staff_style_assigns";
constexpr std::string_view staffStyleAssignPatchKey = "_staff_style_patch";

std::optional<std::int64_t> staffStyleAssignIntegerMember(const Value& value,
                                                          std::string_view member)
{
    if (!value.isObject())
        return std::nullopt;
    const auto* found = value.find(member);
    return found && found->isInteger() ? std::optional{found->asInteger()} : std::nullopt;
}

std::optional<std::int64_t> staffStyleAssignIntegerMember(const Value::Object& value,
                                                          std::string_view member)
{
    const auto found = value.find(member);
    return found != value.end() && found->second.isInteger()
               ? std::optional{found->second.asInteger()}
               : std::nullopt;
}

using StaffStyleAssignRange =
    std::tuple<std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t>;

std::optional<StaffStyleAssignRange> staffStyleAssignRange(const Value& value)
{
    std::array<std::int64_t, 6> fields{};
    std::size_t index{};
    for (const auto member :
         {"part_id", "cmper", "start_meas", "start_edu", "end_meas", "end_edu"}) {
        const auto field = staffStyleAssignIntegerMember(value, member);
        if (!field)
            return std::nullopt;
        fields[index++] = *field;
    }
    return StaffStyleAssignRange{fields[0], fields[1], fields[2], fields[3], fields[4], fields[5]};
}

std::string staffStyleAssignRangeKey(const StaffStyleAssignRange& range)
{
    const auto& [part, staff, startMeasure, startEdu, endMeasure, endEdu] = range;
    return "part=" + std::to_string(part) + ",staff=" + std::to_string(staff) +
           ",range=" + std::to_string(startMeasure) + ':' + std::to_string(startEdu) + '-' +
           std::to_string(endMeasure) + ':' + std::to_string(endEdu);
}

void copyStaffStyleAssignMember(const Value::Object& source, Value::Object& destination,
                                std::string_view member)
{
    const auto found = source.find(member);
    if (found != source.end())
        destination.insert_or_assign(found->first, found->second);
}

void applyStaffStyleAssignPatch(Value::Object& effective, const Value& assignment)
{
    const auto* patch = assignment.find(staffStyleAssignPatchKey);
    if (!patch || !patch->isObject())
        return;
    for (const auto& [field, value] : patch->asObject())
        effective.insert_or_assign(field, value);
}

bool staffStyleAssignSemanticPatchMember(std::string_view member)
{
    return !member.starts_with("_classifier_") && !member.ends_with("_origin");
}

Value::Object* staffStyleAssignBaseStaff(SurveySnapshot& snapshot, std::int64_t partId,
                                        std::int64_t staffId, bool scoreFallback = true)
{
    const auto found = snapshot.find("staff");
    if (found == snapshot.end() || !found->second.isArray())
        return nullptr;
    const auto findPart = [&](std::int64_t candidatePart) -> Value::Object* {
        for (auto& staff : found->second.asArray()) {
            if (staffStyleAssignIntegerMember(staff, "part_id") == candidatePart &&
                staffStyleAssignIntegerMember(staff, "cmper") == staffId) {
                return &staff.asObject();
            }
        }
        return nullptr;
    };
    if (auto* exact = findPart(partId))
        return exact;
    return scoreFallback && partId != musx::dom::SCORE_PARTID
        ? findPart(musx::dom::SCORE_PARTID)
        : nullptr;
}

struct StaffStyleAssignBaseLeaf
{
    const Value* value{};
    const Value* origin{};
};

StaffStyleAssignBaseLeaf staffStyleAssignBaseLeaf(const Value::Object& base, std::string_view path)
{
    const Value::Object* object = &base;
    std::size_t begin{};
    while (begin < path.size()) {
        const auto end = path.find('.', begin);
        const auto leaf =
            path.substr(begin, end == std::string_view::npos ? path.size() - begin : end - begin);
        const auto found = object->find(leaf);
        if (found == object->end())
            return {};
        if (end == std::string_view::npos) {
            const auto origin = object->find(originKeyForLeaf(leaf));
            return {&found->second, origin != object->end() && origin->second.isString()
                                        ? &origin->second
                                        : nullptr};
        }
        if (!found->second.isObject())
            return {};
        object = &found->second.asObject();
        begin = end + 1;
    }
    return {};
}

void staffStyleAssignCompleteFromBase(Value::Object& effective, const Value::Object* base,
                                      std::string_view field)
{
    if (!base || effective.contains(field))
        return;
    const auto found = staffStyleAssignBaseLeaf(*base, field);
    if (!found.value)
        return;
    effective.emplace(std::string(field), *found.value);
    if (found.origin)
        effective.emplace(std::string(field) + "_origin", *found.origin);
}

std::map<std::string, Value::Object*> staffStyleAssignGroupsByKey(SurveySnapshot& snapshot)
{
    std::map<std::string, Value::Object*> result;
    const auto found = snapshot.find(staffStyleAssignCoverageKey);
    if (found == snapshot.end() || !found->second.isArray())
        return result;
    for (auto& group : found->second.asArray()) {
        const auto* key = group.find("_report_match_key");
        if (key && key->isString())
            result.emplace(key->asString(), &group.asObject());
    }
    return result;
}

void completeStaffStyleAssignGroupsFromBase(SurveySnapshot& source, SurveySnapshot& companion)
{
    const auto sourceGroups = staffStyleAssignGroupsByKey(source);
    const auto companionGroups = staffStyleAssignGroupsByKey(companion);
    for (const auto& [key, sourceGroup] : sourceGroups) {
        const auto companionFound = companionGroups.find(key);
        if (companionFound == companionGroups.end())
            continue;
        const auto sourceApplied = sourceGroup->find("applied_style");
        const auto companionApplied = companionFound->second->find("applied_style");
        if (sourceApplied == sourceGroup->end() ||
            companionApplied == companionFound->second->end() ||
            !sourceApplied->second.isObject() || !companionApplied->second.isObject()) {
            continue;
        }
        const auto partId = staffStyleAssignIntegerMember(*sourceGroup, "part_id");
        const auto staffId = staffStyleAssignIntegerMember(*sourceGroup, "cmper");
        if (!partId || !staffId)
            continue;
        const auto* sourceBase = staffStyleAssignBaseStaff(source, *partId, *staffId);
        const auto* companionBase = staffStyleAssignBaseStaff(companion, *partId, *staffId);
        auto& sourceEffective = sourceApplied->second.asObject();
        auto& companionEffective = companionApplied->second.asObject();
        std::set<std::string> fields;
        for (const auto& [field, unused] : sourceEffective)
            if (staffStyleAssignSemanticPatchMember(field))
                fields.insert(field);
        for (const auto& [field, unused] : companionEffective)
            if (staffStyleAssignSemanticPatchMember(field))
                fields.insert(field);
        for (const auto& field : fields) {
            staffStyleAssignCompleteFromBase(sourceEffective, sourceBase, field);
            staffStyleAssignCompleteFromBase(companionEffective, companionBase, field);
        }
    }
}

void prepareStaffStyleAssignComparison(ComparisonPreparationContext& context)
{
    const auto groupAssignments = [](SurveySnapshot& snapshot) {
        const auto found = snapshot.find(staffStyleAssignCoverageKey);
        if (found == snapshot.end() || !found->second.isArray())
            return;

        std::map<StaffStyleAssignRange, std::vector<const Value*>> groups;
        Value::Array ungrouped;
        for (const auto& assignment : found->second.asArray()) {
            if (const auto range = staffStyleAssignRange(assignment)) {
                groups[*range].push_back(&assignment);
            } else {
                ungrouped.push_back(assignment);
            }
        }

        Value::Array result;
        result.reserve(groups.size() + ungrouped.size());
        for (auto& [range, assignments] : groups) {
            // Staff styles on the same range are applied in incidence order. Reducing
            // them to the resulting masked overrides makes a combined style
            // comparable with split styles.
            std::ranges::stable_sort(assignments, [](const Value* left, const Value* right) {
                return staffStyleAssignIntegerMember(*left, "inci").value_or(0) <
                       staffStyleAssignIntegerMember(*right, "inci").value_or(0);
            });

            const auto& first = assignments.front()->asObject();
            Value::Object group;
            for (const auto member : {"part_id", "share_mode", "origin", "cmper", "start_meas",
                                      "start_edu", "end_meas", "end_edu", "origin_startMeas",
                                      "origin_startEdu", "origin_endMeas", "origin_endEdu"}) {
                copyStaffStyleAssignMember(first, group, member);
            }
            group.emplace("_report_match_key", staffStyleAssignRangeKey(range));

            Value::Object effective;
            for (const auto* assignment : assignments)
                applyStaffStyleAssignPatch(effective, *assignment);
            staff_style_semantics::removeInactiveTablatureValues(effective);
            effective.emplace(std::string(staff_style_semantics::classifierAssignmentCount),
                              static_cast<std::int64_t>(assignments.size()));
            group.emplace("applied_style", std::move(effective));
            result.emplace_back(std::move(group));
        }
        result.insert(result.end(), ungrouped.begin(), ungrouped.end());
        found->second = Value(std::move(result));
    };

    groupAssignments(context.source);
    groupAssignments(context.companion);
    completeStaffStyleAssignGroupsFromBase(context.source, context.companion);
}

std::string_view staffStyleAssignAppliedStylePrefix(std::string_view path)
{
    constexpr std::string_view marker = "].applied_style";
    if (!path.starts_with("staff_style_assigns["))
        return {};
    const auto end = path.find(marker);
    return end == std::string_view::npos ? std::string_view{} : path.substr(0, end + marker.size());
}

std::optional<DifferenceClassification>
classifyStaffStyleAssignAttachedItemsUpgrade(const DifferenceContext& context)
{
    constexpr std::string_view classPrefix = "staff_style_assigns[";
    if (const auto expression =
            staff_fields::classifyNoteAttachedItemsExpressionUpgradeLoss(context, classPrefix)) {
        return expression;
    }
    const auto prefix = staffStyleAssignAppliedStylePrefix(context.path);
    if (prefix.empty())
        return std::nullopt;
    const auto aggregateOther = context.source.find(
        std::string(prefix) + "." +
        std::string(staff_style_semantics::classifierAggregateOtherAttachedItems));
    const auto usesAggregateOther = aggregateOther != context.source.end() &&
                                    aggregateOther->second.first.isBool() &&
                                    aggregateOther->second.first.asBool();
    if (const auto smartShapes = staff_fields::classifyAggregateOtherSmartShapeUpgradeLoss(
            context, classPrefix, usesAggregateOther)) {
        return smartShapes;
    }
    return staff_fields::classifyNoteAttachedItemsAggregateHideUpgradeLoss(
        context, classPrefix, "." + std::string(staff_style_semantics::classifierMaskPrefix));
}

std::optional<DifferenceClassification>
classifyStaffStyleAssignInstrumentPromotion(const DifferenceContext& context)
{
    const auto prefix = staffStyleAssignAppliedStylePrefix(context.path);
    if (prefix.empty() || context.category != DifferenceCategory::Differs ||
        !sourcePredatesVersion(context.epoch, context.sourceVersion,
                               finale_mus_reader::FormatEpoch::ZlibLegacy,
                               finale_mus_reader::versions::finale2012)) {
        return std::nullopt;
    }
    const auto sourceCount = comparisonIntegerLeaf(
        context.source,
        std::string(prefix) + "." + std::string(staff_style_semantics::classifierAssignmentCount));
    const auto companionCount = comparisonIntegerLeaf(
        context.companion,
        std::string(prefix) + "." + std::string(staff_style_semantics::classifierAssignmentCount));
    if (!sourceCount || !companionCount)
        return std::nullopt;
    const auto relativePath = context.path.substr(prefix.size());
    const auto mask = staff_style_semantics::maskForValue(relativePath);
    const auto sourceNotation =
        comparisonIntegerLeaf(context.source, std::string(prefix) + ".notation_style");
    const auto companionNotation =
        comparisonIntegerLeaf(context.companion, std::string(prefix) + ".notation_style");
    const auto percussion =
        static_cast<std::int64_t>(staff_style_semantics::SurveyTarget::NotationStyle::Percussion);
    const auto sourceAndCompanionArePercussion =
        sourceNotation && *sourceNotation == percussion && companionNotation &&
        *companionNotation == percussion;
    const auto sourceMaskActiveAndCompanionMaskInactive = [&](std::string_view maskSuffix) {
        return staff_style_semantics::hasActiveInstrumentMask(
                   context.source, prefix, maskSuffix,
                   staff_style_semantics::classifierMaskPrefix) &&
               !staff_style_semantics::hasActiveInstrumentMask(
                   context.companion, prefix, maskSuffix,
                   staff_style_semantics::classifierMaskPrefix);
    };
    const auto companionIsCompletePercussionInstrument =
        sourceAndCompanionArePercussion &&
        staff_style_semantics::hasAnyInstrumentOnlyMask(
            context.companion, prefix, staff_style_semantics::classifierMaskPrefix) &&
        staff_style_semantics::hasAllRequiredInstrumentMasks(
            context.companion, prefix, staff_style_semantics::classifierMaskPrefix);
    if (context.origin == "legacy-mus" && companionIsCompletePercussionInstrument && mask &&
        sourceMaskActiveAndCompanionMaskInactive(*mask)) {
        if (*mask == ".masks.float_notehead_font")
            return DifferenceClassification::FinaleUpgradeNormalization;
        if (*mask == ".masks.no_key" && context.sourceValue.isBool() &&
            context.sourceValue.asBool() && context.companionValue.isBool() &&
            !context.companionValue.asBool()) {
            return DifferenceClassification::FinaleUpgradeNormalization;
        }
    }
    const auto instrumentPromotion =
        staff_style_semantics::hasAnyInstrumentOnlyMask(
            context.source, prefix, staff_style_semantics::classifierMaskPrefix) &&
        !staff_style_semantics::hasAllRequiredInstrumentMasks(
            context.source, prefix, staff_style_semantics::classifierMaskPrefix) &&
        staff_style_semantics::hasAnyInstrumentOnlyMask(
            context.companion, prefix, staff_style_semantics::classifierMaskPrefix) &&
        staff_style_semantics::hasAllRequiredInstrumentMasks(
            context.companion, prefix, staff_style_semantics::classifierMaskPrefix);
    const auto expandedSplit = *companionCount > *sourceCount;
    const auto oneForOnePromotion = context.origin == "legacy-mus" && *sourceCount == 1 &&
                                    *companionCount == 1 && instrumentPromotion;
    if (!expandedSplit && !oneForOnePromotion)
        return std::nullopt;
    if (mask && staff_style_semantics::isInstrumentMask(*mask))
        return DifferenceClassification::FinaleUpgradeNormalization;
    if (context.origin == "legacy-mus" && expandedSplit && instrumentPromotion &&
        relativePath == ".no_key" && context.sourceValue.isBool() &&
        context.sourceValue.asBool() && context.companionValue.isBool() &&
        !context.companionValue.asBool() &&
        staff_style_semantics::hasActiveMask(
            context.source, prefix,
            "." + std::string(staff_style_semantics::classifierMaskPrefix) + "no_key") &&
        !staff_style_semantics::hasActiveMask(
            context.companion, prefix,
            "." + std::string(staff_style_semantics::classifierMaskPrefix) + "no_key")) {
        if (sourceAndCompanionArePercussion) {
            return DifferenceClassification::FinaleUpgradeNormalization;
        }
    }
    if (!mask || *mask != ".masks.float_notehead_font")
        return std::nullopt;
    return companionNotation && *companionNotation == percussion
               ? std::optional{DifferenceClassification::FinaleUpgradeNormalization}
               : std::nullopt;
}

std::optional<DifferenceClassification>
classifyStaffStyleAssignDifference(const DifferenceContext& context)
{
    if (const auto attachedItems = classifyStaffStyleAssignAttachedItemsUpgrade(context))
        return attachedItems;
    return classifyStaffStyleAssignInstrumentPromotion(context);
}

auto staffStyleAssignOrigin(const char* member)
{
    return [member](const StaffStyleAssignSurveyTarget& value, const SurveyContext& context) {
        return fieldOrigin<StaffStyleAssignSurveyTarget>(context, member, value);
    };
}

Value observeStaffStyleAssignments(const SurveyContext& context)
{
    using Target = StaffStyleAssignSurveyTarget;
    Value::Array result;
    for (const auto& assign : sourceInstances<Target>(context)) {
        auto object = observe(
            *assign, context, field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("inci", [](const Target& value) { return value.getInci().value_or(0); }),
            field("style_id", &Target::styleId), field("start_meas", &Target::startMeas),
            field("start_edu", &Target::startEdu), field("end_meas", &Target::endMeas),
            field("end_edu", &Target::endEdu),
            field("origin_styleId", staffStyleAssignOrigin("styleId")),
            field("origin_startMeas", staffStyleAssignOrigin("startMeas")),
            field("origin_startEdu", staffStyleAssignOrigin("startEdu")),
            field("origin_endMeas", staffStyleAssignOrigin("endMeas")),
            field("origin_endEdu", staffStyleAssignOrigin("endEdu")));
        if (const auto style = assign->getStaffStyle()) {
            object.asObject().emplace(staffStyleAssignPatchKey,
                                      staff_style_semantics::canonicalPatch(
                                          staff_style_semantics::observe(*style, context),
                                          staff_style_semantics::usesAggregateOtherAttachedItems(
                                              context.report, style->getCmper())));
        } else {
            object.asObject().emplace(
                staffStyleAssignPatchKey,
                Value::Object{{"unresolved_style_id", Value(assign->styleId)}});
        }
        result.emplace_back(std::move(object));
    }
    return Value(std::move(result));
}

COVERAGE_CLASS_WITH_PREPARATION("others", staffStyleAssignCoverageKey, observeStaffStyleAssignments,
                                classifyStaffStyleAssignDifference,
                                prepareStaffStyleAssignComparison);

} // namespace
