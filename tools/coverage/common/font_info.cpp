// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/common/font_info.h"

#include <map>

#include "coverage/identity.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace coverage {
namespace {

std::int64_t fontInfoIntegerMember(const Value& object, std::string_view key,
                                   std::int64_t fallback = 0)
{
    const auto* value = object.find(key);
    return value && value->isInteger() ? value->asInteger() : fallback;
}

using FontInfoObjectKey = std::pair<std::int64_t, std::int64_t>;

std::map<FontInfoObjectKey, const Value*> fontInfoObjectsByPartAndCmper(const Value::Array& items)
{
    std::map<FontInfoObjectKey, const Value*> result;
    for (const auto& item : items) {
        result.emplace(FontInfoObjectKey{fontInfoIntegerMember(item, "part_id"),
                           fontInfoIntegerMember(item, "cmper")},
            &item);
    }
    return result;
}

const Value* fontInfoObjectForPart(
    const std::map<FontInfoObjectKey, const Value*>& objects, std::int64_t partId,
    std::int64_t cmper)
{
    if (const auto found = objects.find({partId, cmper}); found != objects.end()) {
        return found->second;
    }
    const auto score = objects.find({musx::dom::SCORE_PARTID, cmper});
    return score == objects.end() ? nullptr : score->second;
}

} // namespace

std::string canonicalFontName(std::string_view value)
{
    return musx::dom::normalizeFontName(std::string(value));
}

bool sameFontName(std::string_view left, std::string_view right)
{
    return canonicalFontName(left) == canonicalFontName(right);
}

std::set<std::string> comparisonFontReferencePaths(const SurveySnapshot& snapshot)
{
    std::set<std::string> result;
    const auto shapesFound = snapshot.find("shape_defs");
    const auto listsFound = snapshot.find("shape_instruction_lists");
    if (shapesFound == snapshot.end() || !shapesFound->second.isArray() ||
        listsFound == snapshot.end()) {
        return result;
    }
    const auto* listsValue = listsFound->second.find("lists");
    if (!listsValue || !listsValue->isArray()) return result;
    const auto lists = fontInfoObjectsByPartAndCmper(listsValue->asArray());
    std::map<FontInfoObjectKey, const Value*> buffers;
    if (const auto buffersFound = snapshot.find("shape_data"); buffersFound != snapshot.end()) {
        if (const auto* values = buffersFound->second.find("buffers"); values && values->isArray()) {
            buffers = fontInfoObjectsByPartAndCmper(values->asArray());
        }
    }
    for (const auto& shape : shapesFound->second.asArray()) {
        const auto partId = fontInfoIntegerMember(shape, "part_id");
        const auto* list = fontInfoObjectForPart(
            lists, partId, fontInfoIntegerMember(shape, "instruction_list"));
        if (!list) continue;
        const auto* instructions = list->find("instructions");
        if (!instructions || !instructions->isArray()) continue;
        const auto dataCmper = fontInfoIntegerMember(shape, "data_list");
        const auto* buffer = fontInfoObjectForPart(buffers, partId, dataCmper);
        const auto bufferPartId = buffer ? fontInfoIntegerMember(*buffer, "part_id") : partId;
        std::size_t offset = 0;
        for (const auto& instruction : instructions->asArray()) {
            if (fontInfoIntegerMember(instruction, "type") == 20) {
                result.insert("shape_data.buffers[" + partIdentityPrefix(bufferPartId) +
                              "cmper=" + std::to_string(dataCmper) +
                              "].values[" + std::to_string(offset) + "].value");
            }
            offset += static_cast<std::size_t>(fontInfoIntegerMember(instruction, "num_data"));
        }
    }
    return result;
}

std::string comparisonFontIdentity(const SurveySnapshot& snapshot, std::int64_t id)
{
    const auto found = snapshot.find("font_definitions");
    if (found == snapshot.end()) return {};
    const auto* definitions = found->second.find("definitions");
    if (!definitions || !definitions->isArray()) return {};
    for (const auto& definition : definitions->asArray()) {
        if (fontInfoIntegerMember(definition, "cmper", -1) != id) continue;
        if (const auto* name = definition.find("normalized_name"); name && name->isString()) {
            return canonicalFontName(name->asString());
        }
    }
    return {};
}

bool isComparisonFontReference(std::string_view path,
                               const std::set<std::string>& shapeFontPaths)
{
    constexpr std::string_view suffix = "_font_id";
    return (path.size() >= suffix.size() && path.substr(path.size() - suffix.size()) == suffix) ||
           shapeFontPaths.contains(std::string(path));
}

} // namespace coverage
} // namespace finale_mus_reader
