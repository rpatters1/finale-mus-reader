// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "records/legacy_record_index.h"

#include <algorithm>
#include <utility>

namespace finale_mus_reader {
namespace records {

LegacyRecordIndex LegacyRecordIndex::build(const container::ParsedContainer& parsed)
{
    LegacyRecordIndex result;
    result.m_byteOrder = parsed.byteOrder;
    for (auto& record : decodeLegacyOthers(parsed)) {
        auto& family = result.m_families[FamilyKey{record.tag, record.cmper}];
        family.push_back(std::move(record));
    }
    for (auto& [key, family] : result.m_families) {
        (void)key;
        std::sort(family.begin(), family.end(),
            [](const LegacyOther& left, const LegacyOther& right) {
                return left.incident < right.incident;
            });
    }
    return result;
}

std::optional<RecordWord> LegacyRecordIndex::word(
    std::string_view tag, std::uint16_t cmper, std::size_t wordIndex) const
{
    const auto found = m_families.find(FamilyKey{std::string(tag), cmper});
    if (found == m_families.end()) {
        return std::nullopt;
    }
    const auto incidence = wordIndex / wordsPerIncidence;
    const auto slot = wordIndex % wordsPerIncidence;
    if (incidence >= found->second.size()) {
        return std::nullopt;
    }
    const auto& record = found->second[incidence];
    // The decoder assigns incidences in encounter order, so position and incident agree.
    if (record.incident != incidence) {
        return std::nullopt;
    }
    RecordWord result;
    result.value = readPayloadWord(record, slot, m_byteOrder);
    result.blockOffset = record.blockOffset;
    result.decodedOffset = record.decodedOffset;
    return result;
}

std::vector<std::uint16_t> LegacyRecordIndex::cmpersForTag(std::string_view tag) const
{
    std::vector<std::uint16_t> result;
    for (const auto& [key, family] : m_families) {
        if (!family.empty() && key.first == tag) {
            result.push_back(key.second);
        }
    }
    // The family map is already ordered by tag then cmper.
    return result;
}

} // namespace records
} // namespace finale_mus_reader
