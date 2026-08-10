// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/legacy_mapping.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "import/mappings/tables.h"

namespace finale_mus_reader {
namespace mapping {
namespace {

/// @brief Every table, in the order they are layered.
const std::vector<const MappingTable*>& registeredTables()
{
    static const std::vector<const MappingTable*> result = {
        &fontDefinitionsTable(),
        &earlyFontDefinitionsTable(),
        &musicSpacingOptionsTable(),
        &layerAttributesTable()};
    return result;
}

/// @brief A value read out of the record stream, with the provenance of its first word.
struct ResolvedValue
{
    std::int64_t value{};
    std::size_t blockOffset{};
    std::size_t decodedOffset{};
};

std::optional<ResolvedValue> readValue(const records::LegacyRecordIndex& index,
    std::uint16_t cmper, const SourceLocation& source)
{
    const auto tag = records::packTag(std::string_view(source.tag, sizeof(source.tag)));
    const std::size_t wordIndex = source.incidence * records::otherWordCount + source.wordSlot;
    const auto first = index.word(tag, cmper, wordIndex);
    if (!first) {
        return std::nullopt;
    }

    std::int64_t value = 0;
    if (source.width == ValueWidth::Long) {
        // The word index is absolute, so a four-byte value whose words fall in different
        // incidences resolves without special handling.
        const auto second = index.word(tag, cmper, wordIndex + 1);
        if (!second) {
            return std::nullopt;
        }
        const auto firstWord = static_cast<std::uint16_t>(first->value);
        const auto secondWord = static_cast<std::uint16_t>(second->value);
        const std::uint32_t combined = source.longOrder == LongWordOrder::HighFirst
            ? (static_cast<std::uint32_t>(firstWord) << 16U) | secondWord
            : (static_cast<std::uint32_t>(secondWord) << 16U) | firstWord;
        value = static_cast<std::int32_t>(combined);
    } else if (source.width == ValueWidth::Byte) {
        // The framework selects one-byte fields through a 16-bit slot and then narrows.
        // Which half it keeps is still a binary-validation target, so no one-byte mapping
        // is promoted until a fixture settles it.
        value = static_cast<std::int8_t>(static_cast<std::uint16_t>(first->value) & 0xffU);
    } else {
        value = first->value;
    }

    if (source.bits.bitCount != 0) {
        const auto mask = (std::uint64_t{1} << source.bits.bitCount) - 1U;
        value = static_cast<std::int64_t>(
            (static_cast<std::uint64_t>(value) >> source.bits.firstBit) & mask);
    }
    return ResolvedValue{value, first->blockOffset, first->decodedOffset};
}

// Character payloads are not byte-order sensitive, so text is assembled from the raw bytes
// rather than from the decoded words. A little-endian file stores a font name as plain text,
// and reading it through the words would transpose every character pair.
std::optional<std::string> readText(const records::LegacyRecordIndex& index,
    std::uint16_t cmper, const SourceLocation& source)
{
    const auto tag = records::packTag(std::string_view(source.tag, sizeof(source.tag)));
    const auto family = index.getOthers().getArray(tag, cmper);
    std::string text;
    bool found = false;
    for (const auto& row : family) {
        if (row.inci < source.incidence) {
            continue;
        }
        found = true;
        text.append(reinterpret_cast<const char*>(row.bytes.data()), row.byteCount);
    }
    if (!found) {
        return std::nullopt;
    }
    // Rows are fixed width, so the last one is padded. The name ends at the first NUL.
    if (const auto end = text.find('\0'); end != std::string::npos) {
        text.resize(end);
    }
    return text;
}

/// @brief One destination field: what to report, and where to read it if this file can.
struct EffectiveField
{
    /// @brief Supplies the field name and the accessor that reads the seeded default.
    /// The destination is a property of the musxdom class, so any table variant will do.
    const FieldMapping* reporting{};
    /// @brief The source location applicable to this file, or null when no table applies.
    const FieldMapping* readable{};
};

/// @brief The tables for one destination class, with their fields layered.
struct EffectiveTable
{
    const MappingTable* table{};
    std::vector<EffectiveField> fields;
};

/// @details Every registered field is reported, whether or not this file can supply it,
/// so a document from an era the tables do not cover still shows its supported fields
/// sitting at their synthesized defaults. The gates decide only what is readable.
std::vector<EffectiveTable> buildEffectiveTables(
    const std::vector<const MappingTable*>& tables, const SourceProfile& profile)
{
    std::vector<EffectiveTable> result;
    for (const auto* table : tables) {
        const bool tableApplies = epochMatches(table->epochs, profile.epoch)
            && table->versions.includes(profile.version);
        const auto found = std::find_if(result.begin(), result.end(),
            [&](const EffectiveTable& candidate) {
                return std::string_view(candidate.table->reportPrefix) == table->reportPrefix;
            });
        auto& effective = found != result.end()
            ? *found
            : result.emplace_back(EffectiveTable{table, {}});
        for (std::size_t i = 0; i < table->fieldCount; ++i) {
            const auto* field = table->fields + i;
            const bool readable = tableApplies && field->versions.includes(profile.version);
            const auto existing = std::find_if(effective.fields.begin(), effective.fields.end(),
                [&](const EffectiveField& candidate) {
                    return std::string_view(candidate.reporting->fieldName) == field->fieldName;
                });
            if (existing == effective.fields.end()) {
                effective.fields.push_back({field, readable ? field : nullptr});
            } else if (readable) {
                // A later table supersedes an earlier one for the same field, so a field
                // that moves in a later version costs one override row rather than a new
                // table.
                existing->readable = field;
            }
        }
    }
    return result;
}

std::string reportTarget(const MappingTable& table, const MappingTarget& target,
    const FieldMapping& field)
{
    std::string result = table.reportPrefix;
    if (table.targetKind == TargetKind::OthersByCmper) {
        result += '[' + std::to_string(target.cmper) + ']';
    }
    result += '.';
    result += field.fieldName;
    return result;
}

} // namespace

bool epochMatches(EpochMask mask, FormatEpoch epoch)
{
    auto bit = EpochMask::None;
    switch (epoch) {
    case FormatEpoch::CodaBanner:
        bit = EpochMask::CodaBanner;
        break;
    case FormatEpoch::UncompressedLegacy:
        bit = EpochMask::Uncompressed;
        break;
    case FormatEpoch::DclLegacy:
        bit = EpochMask::Dcl;
        break;
    case FormatEpoch::ZlibLegacy:
        bit = EpochMask::Zlib;
        break;
    case FormatEpoch::Unknown:
        return false;
    }
    return (static_cast<std::uint8_t>(mask) & static_cast<std::uint8_t>(bit)) != 0;
}

void applyLegacyMappings(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document, ImportReport& report)
{
    applyMappingTables(registeredTables(), index, profile, document, report);
}

void applyMappingTables(const std::vector<const MappingTable*>& tables,
    const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document, ImportReport& report)
{
    for (const auto& effective : buildEffectiveTables(tables, profile)) {
        const auto& table = *effective.table;
        std::vector<MappingTarget> targets;
        if (table.targetKind == TargetKind::OthersFromRecords) {
            // The legacy file is the only source of these objects, so they are created from
            // the comparators the records themselves carry rather than found in the pool.
            const auto tag = records::packTag(table.recordTag);
            for (const auto cmper : index.getOthers().cmpersForTag(tag)) {
                targets.push_back({cmper, table.createTarget(document, cmper)});
            }
        } else {
            targets = table.enumerateTargets(document);
        }

        for (const auto& target : targets) {
            for (const auto& field : effective.fields) {
                FieldInfo info;
                info.target = reportTarget(table, target, *field.reporting);
                info.origin = ValueOrigin::Finale27Default;
                // A text field has no numeric default to report, and an object created from
                // records has no seeded value at all, so both leave the raw value at zero.
                if (field.reporting->read) {
                    info.rawValue = field.reporting->read(target.instance);
                }

                if (field.readable) {
                    const auto selector =
                        table.targetKind == TargetKind::OptionsSingleton
                            ? field.readable->source.selector : target.cmper;
                    if (field.readable->kind == FieldKind::Text) {
                        if (const auto text = readText(index, selector, field.readable->source)) {
                            field.readable->applyText(target.instance, *text);
                            info.origin = ValueOrigin::LegacyMus;
                            info.rawValue = static_cast<std::int64_t>(text->size());
                        }
                        report.fields.push_back(std::move(info));
                        continue;
                    }
                    if (const auto resolved = readValue(index, selector, field.readable->source)) {
                        field.readable->apply(target.instance, resolved->value);
                        info.origin = ValueOrigin::LegacyMus;
                        info.blockOffset = resolved->blockOffset;
                        info.decodedOffset = resolved->decodedOffset;
                        info.rawValue = resolved->value;
                    }
                }
                report.fields.push_back(std::move(info));
            }
        }
    }
}

} // namespace mapping
} // namespace finale_mus_reader
