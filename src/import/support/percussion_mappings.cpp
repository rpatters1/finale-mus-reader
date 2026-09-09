// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/support/percussion_mappings.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "import/support/text_encoding.h"
#include "musx/dom/PercussionNoteType.h"
#include "musx/xml/XmlInterface.h"

namespace finale_mus_reader {
namespace percussion {
namespace {

using NoteType = musx::dom::PercussionNoteTypeId;

std::optional<std::string>
attributeValue(const musx::xml::XmlElementPtr &element,
               const std::string &name) {
    if (const auto attribute = element->findAttribute(name)) {
        return attribute->getValueTrimmed();
    }
    return std::nullopt;
}

bool isValidNoteType(NoteType value) {
    constexpr NoteType baseTypeMask = 0x0fff;
    const auto baseType = NoteType(value & baseTypeMask);
    return musx::dom::percussion::getPercussionNoteTypeFromId(baseType)
               .instrumentId == baseType;
}

MappingTable parseNoteMap(const musx::xml::XmlElementPtr &list) {
    MappingTable result;
    for (auto note = list->getFirstChildElement("Note"); note;
         note = note->getNextSibling("Note")) {
        const auto numberText = attributeValue(note, "Number");
        const auto typeText = attributeValue(note, "PercNoteType");
        if (!numberText || !typeText) {
            continue;
        }
        const auto number = std::stoul(*numberText);
        const auto type = std::stoul(*typeText);
        if (number > (std::numeric_limits<std::uint16_t>::max)() ||
            type > (std::numeric_limits<NoteType>::max)()) {
            continue;
        }
        const auto noteType = NoteType(type);
        if (!isValidNoteType(noteType)) {
            continue;
        }
        result.try_emplace(std::uint16_t(number), noteType);
    }
    return result;
}

void collectNoteNameLists(
    const musx::xml::XmlElementPtr &element,
    std::vector<std::pair<std::string, MappingTable>> &result) {
    if (!element) {
        return;
    }
    if (element->getTagName() == "NoteNameList") {
        if (const auto name = attributeValue(element, "Name")) {
            const auto normalized = normalizeMappingName(*name);
            if (!normalized.empty()) {
                result.emplace_back(normalized, parseNoteMap(element));
            }
        }
        return;
    }
    for (auto child = element->getFirstChildElement(); child;
         child = child->getNextSibling()) {
        collectNoteNameLists(child, result);
    }
}

} // namespace

std::optional<NoteType> MappingTables::find(std::string_view mapName,
                                            std::uint16_t midiNote) const {
    const auto table = m_tables.find(normalizeMappingName(mapName));
    if (table == m_tables.end()) {
        return std::nullopt;
    }
    const auto note = table->second.find(midiNote);
    return note == table->second.end() ? std::nullopt
                                       : std::optional(note->second);
}

MappingTables
parseMappingTables(std::span<const std::span<const std::uint8_t>> documents,
                   XmlParser parseXml) {
    MappingTables result;
    for (const auto document : documents) {
        if (document.empty()) {
            continue;
        }
        const auto xml = parseXml(
            reinterpret_cast<const char *>(document.data()), document.size());
        std::vector<std::pair<std::string, MappingTable>> parsedTables;
        collectNoteNameLists(xml->getRootElement(), parsedTables);
        for (auto &[name, table] : parsedTables) {
            result.m_tables.try_emplace(std::move(name), std::move(table));
        }
    }
    return result;
}

std::string normalizeMappingName(std::string_view name) {
    while (!name.empty() && text::isSpace(name.front())) {
        name.remove_prefix(1);
    }
    while (!name.empty() && text::isSpace(name.back())) {
        name.remove_suffix(1);
    }
    std::string result(name);
    for (const std::string_view suffix :
         {" GPO Finale Edition", " Finale Edition"}) {
        if (result.ends_with(suffix)) {
            result.resize(result.size() - suffix.size());
            break;
        }
    }
    return result;
}

} // namespace percussion
} // namespace finale_mus_reader
