// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/support/percussion_mappings.h"

#include <algorithm>
#include <array>
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

std::string_view trimMappingField(std::string_view value)
{
    while (!value.empty() && text::isSpace(value.front())) {
        value.remove_prefix(1);
    }
    while (!value.empty() && text::isSpace(value.back())) {
        value.remove_suffix(1);
    }
    return value;
}

std::string mappingFileName(std::string_view path)
{
    const auto slash = path.find_last_of("/\\");
    return std::string(slash == std::string_view::npos ? path : path.substr(slash + 1));
}

std::array<std::string, 3> parseConversionRow(std::string_view line)
{
    std::array<std::string, 3> fields;
    std::size_t column = 0;
    bool escaped = false;
    for (const char value : line) {
        if (escaped) {
            if (value != ',' && value != '\\') {
                fields[column].push_back('\\');
            }
            fields[column].push_back(value);
            escaped = false;
        } else if (value == '\\') {
            escaped = true;
        } else if (value == ',') {
            if (++column == fields.size()) {
                throw std::invalid_argument("Percussion map conversion row has too many columns");
            }
        } else {
            fields[column].push_back(value);
        }
    }
    if (escaped) {
        fields[column].push_back('\\');
    }
    if (column + 1 != fields.size()) {
        throw std::invalid_argument("Percussion map conversion row must have three columns");
    }
    for (auto& field : fields) {
        field = std::string(trimMappingField(field));
        if (field.empty()) {
            throw std::invalid_argument("Percussion map conversion row has an empty column");
        }
    }
    return fields;
}

std::optional<std::string> attributeValue(const musx::xml::XmlElementPtr& element, const std::string& name)
{
    if (const auto attribute = element->findAttribute(name)) {
        return attribute->getValueTrimmed();
    }
    return std::nullopt;
}

bool isValidNoteType(NoteType value)
{
    constexpr NoteType baseTypeMask = 0x0fff;
    const auto baseType = NoteType(value & baseTypeMask);
    return musx::dom::percussion::getPercussionNoteTypeFromId(baseType).instrumentId == baseType;
}

MappingTable parseNoteMap(const musx::xml::XmlElementPtr& list)
{
    MappingTable result;
    for (auto note = list->getFirstChildElement("Note"); note; note = note->getNextSibling("Note")) {
        const auto numberText = attributeValue(note, "Number");
        const auto typeText = attributeValue(note, "PercNoteType");
        if (!numberText || !typeText) {
            continue;
        }
        const auto number = std::stoul(*numberText);
        const auto type = std::stoul(*typeText);
        if (number > (std::numeric_limits<std::uint16_t>::max)() || type > (std::numeric_limits<NoteType>::max)()) {
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

void collectNoteNameLists(const musx::xml::XmlElementPtr& element, std::vector<std::pair<std::string, MappingTable>>& result)
{
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
    for (auto child = element->getFirstChildElement(); child; child = child->getNextSibling()) {
        collectNoteNameLists(child, result);
    }
}

} // namespace

std::optional<NoteType> MappingTables::find(std::string_view mapName, std::uint16_t inputKey, std::uint16_t playbackMidiNote) const
{
    if (const auto conversions = m_conversions.find(std::string(trimMappingField(mapName))); conversions != m_conversions.end()) {
        for (const auto& target : conversions->second) {
            const auto namedTable = m_namedTables.find(target);
            if (namedTable != m_namedTables.end()) {
                if (const auto note = namedTable->second.find(inputKey); note != namedTable->second.end()) {
                    return note->second;
                }
            }
        }
    }
    const auto table = m_tables.find(normalizeMappingName(mapName));
    if (table == m_tables.end()) {
        return std::nullopt;
    }
    const auto note = table->second.find(playbackMidiNote);
    return note == table->second.end() ? std::nullopt : std::optional(note->second);
}

MappingTables parseMappingTables(std::span<const PercussionMappingXml> documents, std::span<const std::uint8_t> conversionTable, XmlParser parseXml)
{
    MappingTables result;
    for (const auto& document : documents) {
        if (document.bytes.empty()) {
            continue;
        }
        const auto xml = parseXml(reinterpret_cast<const char*>(document.bytes.data()), document.bytes.size());
        std::vector<std::pair<std::string, MappingTable>> parsedTables;
        collectNoteNameLists(xml->getRootElement(), parsedTables);
        for (auto& [name, table] : parsedTables) {
            if (!document.fileName.empty()) {
                result.m_namedTables.try_emplace({mappingFileName(document.fileName), name}, table);
            }
            result.m_tables.try_emplace(std::move(name), std::move(table));
        }
    }
    if (!conversionTable.empty()) {
        std::string_view contents(reinterpret_cast<const char*>(conversionTable.data()), conversionTable.size());
        if (contents.starts_with("\xef\xbb\xbf")) {
            contents.remove_prefix(3);
        }
        while (!contents.empty()) {
            const auto end = contents.find('\n');
            const auto line = trimMappingField(contents.substr(0, end));
            if (!line.empty() && !line.starts_with("//")) {
                const auto [legacyName, filename, listName] = parseConversionRow(line);
                result.m_conversions[legacyName].emplace_back(filename, normalizeMappingName(listName));
            }
            if (end == std::string_view::npos) {
                break;
            }
            contents.remove_prefix(end + 1);
        }
    }
    return result;
}

std::string normalizeMappingName(std::string_view name)
{
    std::string result(trimMappingField(name));
    for (const std::string_view suffix : {" GPO Finale Edition", " Finale Edition"}) {
        if (result.ends_with(suffix)) {
            result.resize(result.size() - suffix.size());
            break;
        }
    }
    return result;
}

} // namespace percussion
} // namespace finale_mus_reader
