// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

using PercussionMapKey = std::pair<std::int64_t, std::int64_t>;
using PercussionRowKey = std::tuple<std::int64_t, std::int64_t, std::int64_t>;

std::optional<DifferenceClassification>
classifyPercussionNoteInfoDifference(const DifferenceContext &context) {
    if (context.category == DifferenceCategory::Differs &&
        context.path.ends_with(".perc_note_type") && context.origin == "legacy-behavior") {
        return DifferenceClassification::LegacyPercussionGeneralMidiFallback;
    }
    return std::nullopt;
}

std::optional<std::int64_t> percussionIntegerMember(const Value &value, std::string_view member) {
    if (!value.isObject())
        return std::nullopt;
    const auto *found = value.find(member);
    if (!found || !found->isInteger())
        return std::nullopt;
    return found->asInteger();
}

void collectPercussionReferencedMapIds(const SurveySnapshot &snapshot,
                                       std::set<std::int64_t> &result) {
    const auto found = snapshot.find("drum_staff");
    if (found == snapshot.end() || !found->second.isArray())
        return;
    for (const auto &staff : found->second.asArray()) {
        if (const auto mapId = percussionIntegerMember(staff, "which_drum_lib"))
            result.insert(*mapId);
    }
}

std::int64_t percussionPartId(const Value &value) {
    return percussionIntegerMember(value, "part_id").value_or(0);
}

std::optional<PercussionMapKey> percussionMapKey(const Value &value) {
    const auto mapId = percussionIntegerMember(value, "cmper");
    if (!mapId)
        return std::nullopt;
    return PercussionMapKey{percussionPartId(value), *mapId};
}

bool isLegacyPercussionMapNote(const Value &value) {
    if (!value.isObject())
        return false;
    const auto *origin = value.find("origin_percNoteType");
    return origin && origin->isString() && origin->asString() == "legacy-behavior";
}

void collectNativePercussionMapIds(const SurveySnapshot &snapshot, std::set<std::int64_t> &result) {
    const auto found = snapshot.find("percussion_note_info");
    if (found == snapshot.end() || !found->second.isArray())
        return;
    for (const auto &note : found->second.asArray()) {
        if (isLegacyPercussionMapNote(note))
            continue;
        if (const auto mapId = percussionIntegerMember(note, "cmper"))
            result.insert(*mapId);
    }
}

std::size_t dropUnreferencedCompanionMapNotes(const SurveySnapshot &source,
                                              SurveySnapshot &companion) {
    std::set<std::int64_t> retainedMaps;
    collectPercussionReferencedMapIds(companion, retainedMaps);
    collectNativePercussionMapIds(source, retainedMaps);

    const auto found = companion.find("percussion_note_info");
    if (found == companion.end() || !found->second.isArray())
        return 0;
    auto &notes = found->second.asArray();
    const auto before = notes.size();
    std::erase_if(notes, [&retainedMaps](const Value &note) {
        const auto mapId = percussionIntegerMember(note, "cmper");
        return mapId && !retainedMaps.contains(*mapId);
    });
    return before - notes.size();
}

std::map<PercussionMapKey, std::set<std::int64_t>>
percussionMapTargets(const SurveySnapshot &source, const SurveySnapshot &companion) {
    using StaffKey = std::pair<std::int64_t, std::int64_t>;
    const auto staffMaps = [](const SurveySnapshot &snapshot) {
        std::map<StaffKey, std::int64_t> result;
        const auto found = snapshot.find("drum_staff");
        if (found == snapshot.end() || !found->second.isArray())
            return result;
        for (const auto &staff : found->second.asArray()) {
            const auto staffId = percussionIntegerMember(staff, "cmper");
            const auto mapId = percussionIntegerMember(staff, "which_drum_lib");
            if (staffId && mapId)
                result.insert_or_assign({percussionPartId(staff), *staffId}, *mapId);
        }
        return result;
    };

    const auto sourceMaps = staffMaps(source);
    const auto companionMaps = staffMaps(companion);
    std::map<PercussionMapKey, std::set<std::int64_t>> result;
    for (const auto &[staff, sourceMap] : sourceMaps) {
        const auto companionMap = companionMaps.find(staff);
        if (companionMap != companionMaps.end())
            result[{staff.first, sourceMap}].insert(companionMap->second);
    }
    return result;
}

void realignLegacyPercussionMapNotes(SurveySnapshot &source, const SurveySnapshot &companion) {
    const auto sourceFound = source.find("percussion_note_info");
    const auto companionFound = companion.find("percussion_note_info");
    if (sourceFound == source.end() || !sourceFound->second.isArray() ||
        companionFound == companion.end() || !companionFound->second.isArray()) {
        return;
    }

    const auto targets = percussionMapTargets(source, companion);
    std::map<PercussionMapKey, std::vector<std::int64_t>> sourceIncis;
    for (const auto &note : sourceFound->second.asArray()) {
        const auto map = percussionMapKey(note);
        const auto inci = percussionIntegerMember(note, "inci");
        if (isLegacyPercussionMapNote(note) && map && inci)
            sourceIncis[*map].push_back(*inci);
    }
    for (auto &[map, incis] : sourceIncis)
        std::ranges::sort(incis);

    std::map<PercussionMapKey, std::vector<std::int64_t>> companionMapIncis;
    std::map<PercussionRowKey, std::vector<std::int64_t>> companionIncis;
    for (const auto &note : companionFound->second.asArray()) {
        const auto map = percussionMapKey(note);
        const auto noteType = percussionIntegerMember(note, "perc_note_type");
        const auto inci = percussionIntegerMember(note, "inci");
        if (map && inci)
            companionMapIncis[*map].push_back(*inci);
        if (map && noteType && inci)
            companionIncis[{map->first, map->second, *noteType}].push_back(*inci);
    }
    for (auto &[map, incis] : companionMapIncis)
        std::ranges::sort(incis);

    std::map<PercussionRowKey, std::size_t> sourceTypeCounts;
    for (const auto &note : sourceFound->second.asArray()) {
        const auto map = percussionMapKey(note);
        const auto noteType = percussionIntegerMember(note, "perc_note_type");
        if (isLegacyPercussionMapNote(note) && map && noteType)
            ++sourceTypeCounts[{map->first, map->second, *noteType}];
    }

    Value::Array realigned;
    for (const auto &note : sourceFound->second.asArray()) {
        const auto map = percussionMapKey(note);
        const auto noteType = percussionIntegerMember(note, "perc_note_type");
        if (!isLegacyPercussionMapNote(note) || !map || !noteType) {
            realigned.push_back(note);
            continue;
        }

        const auto foundTargets = targets.find(*map);
        if (foundTargets == targets.end() || foundTargets->second.empty()) {
            auto sourceOnly = note;
            sourceOnly.asObject()["cmper"] = Value(-1 - map->second);
            realigned.push_back(std::move(sourceOnly));
            continue;
        }

        for (const auto targetMap : foundTargets->second) {
            auto aligned = note;
            aligned.asObject()["cmper"] = Value(targetMap);
            const auto sourceMapIncis = sourceIncis.find(*map);
            const auto targetMapIncis = companionMapIncis.find({map->first, targetMap});
            const auto sourceInci = percussionIntegerMember(note, "inci");
            if (foundTargets->second.size() == 1 && sourceInci &&
                sourceMapIncis != sourceIncis.end() && targetMapIncis != companionMapIncis.end() &&
                sourceMapIncis->second.size() == targetMapIncis->second.size()) {
                const auto position = std::ranges::lower_bound(sourceMapIncis->second, *sourceInci);
                aligned.asObject()["inci"] = Value(targetMapIncis->second.at(
                    static_cast<std::size_t>(position - sourceMapIncis->second.begin())));
                realigned.push_back(std::move(aligned));
                continue;
            }
            const auto sourceRow = PercussionRowKey{map->first, map->second, *noteType};
            const auto companionRow = PercussionRowKey{map->first, targetMap, *noteType};
            const auto companionIncidence = companionIncis.find(companionRow);
            if (sourceTypeCounts[sourceRow] == 1 && companionIncidence != companionIncis.end() &&
                companionIncidence->second.size() == 1) {
                aligned.asObject()["inci"] = Value(companionIncidence->second.front());
            } else if (const auto inci = percussionIntegerMember(note, "inci")) {
                aligned.asObject()["inci"] = Value(-1 - *inci);
            }
            realigned.push_back(std::move(aligned));
        }
    }
    sourceFound->second = Value(std::move(realigned));
}

void preparePercussionNoteInfoComparison(ComparisonPreparationContext &context) {
    realignLegacyPercussionMapNotes(context.source, context.companion);
    const auto dropped = dropUnreferencedCompanionMapNotes(context.source, context.companion);
    if (dropped) {
        context.transformations[ComparisonTransformation::FinaleSynthesizedPercussionMapNote] +=
            dropped;
    }
}

Value observePercussionNoteInfo(const SurveyContext &ctx) {
    using Target = musx::dom::others::PercussionNoteInfo;
    Value::Array result;
    for (const auto &note : sourceInstances<Target>(ctx)) {
        const auto origin = [&ctx, &note](std::string_view member) {
            return fieldOrigin<Target>(ctx, member, *note);
        };
        result.emplace_back(
            observe(*note, ctx,
                    field("closed_notehead",
                          [](const Target &value) {
                              return static_cast<std::uint32_t>(value.closedNotehead);
                          }),
                    field("cmper", [](const Target &value) { return value.getCmper(); }),
                    field("dwhole_notehead",
                          [](const Target &value) {
                              return static_cast<std::uint32_t>(value.dwholeNotehead);
                          }),
                    field("half_notehead",
                          [](const Target &value) {
                              return static_cast<std::uint32_t>(value.halfNotehead);
                          }),
                    field("inci", [](const Target &value) { return value.getInci().value_or(0); }),
                    field("origin_closedNotehead",
                          [&origin](const Target &) { return origin("closedNotehead"); }),
                    field("origin_dwholeNotehead",
                          [&origin](const Target &) { return origin("dwholeNotehead"); }),
                    field("origin_halfNotehead",
                          [&origin](const Target &) { return origin("halfNotehead"); }),
                    field("origin_percNoteType",
                          [&origin](const Target &) { return origin("percNoteType"); }),
                    field("origin_staffPosition",
                          [&origin](const Target &) { return origin("staffPosition"); }),
                    field("origin_wholeNotehead",
                          [&origin](const Target &) { return origin("wholeNotehead"); }),
                    field("perc_note_type", &Target::percNoteType),
                    field("staff_position", &Target::staffPosition),
                    field("whole_notehead", [](const Target &value) {
                        return static_cast<std::uint32_t>(value.wholeNotehead);
                    })));
    }
    return result;
}

COVERAGE_CLASS_WITH_PREPARATION("others", "percussion_note_info", observePercussionNoteInfo,
                                classifyPercussionNoteInfoDifference,
                                preparePercussionNoteInfoComparison);

} // namespace
