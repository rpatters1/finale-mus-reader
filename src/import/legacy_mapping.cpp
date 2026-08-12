// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/legacy_mapping.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "import/details/details.h"
#include "import/entries/entries.h"
#include "import/options/options.h"
#include "import/others/others.h"

namespace finale_mus_reader {

namespace {

/// @brief Every table, in the order they are layered.
const std::vector<const MappingTable*>& registeredTables()
{
    static const std::vector<const MappingTable*> result = {
        &others::fontDefinitionsTable(),
        &others::earlyFontDefinitionsTable(),
        &others::codaFontDefinitionsTable(),
        &others::classFontDefinitionsTable(),
        &options::musicSpacingOptionsTable(),
        &options::clefOptionsTable(),
        &options::earlyClefOptionsTable(),
        &options::classClefOptionsTable(),
        &others::layerAttributesTable()};
    return result;
}

// Reads a numeric value from a class-identified record, addressed by byte offset inside a
// single record's payload rather than by word slot across an incidence stream.
std::optional<ResolvedValue> readClassValue(const records::LegacyRecordIndex& index,
    std::uint16_t cmper, const SourceLocation& source, ByteOrder byteOrder)
{
    const auto* row = index.getClassRecords().get(
        source.identity, cmper, 0, source.incidence);
    if (!row) {
        return std::nullopt;
    }
    const auto payload = index.getClassRecords().payloadOf(*row);
    const std::size_t width = source.width == ValueWidth::Long ? 4
        : source.width == ValueWidth::Byte ? 1 : 2;
    if (source.wordSlot + width > payload.size()) {
        return std::nullopt;
    }
    // Payload numbers follow the file's byte order, as everything numeric does. Finale 2007
    // in particular is mostly big-endian.
    std::int64_t value = 0;
    for (std::size_t i = 0; i < width; ++i) {
        const auto shift = byteOrder == ByteOrder::BigEndian ? (width - 1 - i) : i;
        value |= static_cast<std::int64_t>(payload[source.wordSlot + i]) << (8U * shift);
    }
    if (source.bits.bitCount != 0) {
        const auto mask = (std::uint64_t{1} << source.bits.bitCount) - 1U;
        value = static_cast<std::int64_t>(
            (static_cast<std::uint64_t>(value) >> source.bits.firstBit) & mask);
    } else {
        // A whole field is signed, because the fixed-row path reads one through a signed
        // word and the two encodings must not disagree about the same logical option. A bit
        // range is exempt: extracted bits are a magnitude, not a number with a sign. Without
        // this an Evpu of -12 arrives as 65524 and is assigned as such.
        const auto bitCount = 8U * width;
        const auto signBit = std::uint64_t{1} << (bitCount - 1U);
        if ((static_cast<std::uint64_t>(value) & signBit) != 0) {
            value |= static_cast<std::int64_t>(~std::uint64_t{0} << bitCount);
        }
    }
    return ResolvedValue{value, row->blockOffset, row->decodedOffset};
}

// Reads text from a class-identified record: the payload runs to its own end, so the name
// simply occupies whatever remains after the fields before it.
std::optional<std::string> readClassText(const records::LegacyRecordIndex& index,
    std::uint16_t cmper, const SourceLocation& source)
{
    const auto* row = index.getClassRecords().get(source.identity, cmper, 0, 0);
    if (!row) {
        return std::nullopt;
    }
    const auto payload = index.getClassRecords().payloadOf(*row);
    if (source.wordSlot >= payload.size()) {
        return std::nullopt;
    }
    std::string text(reinterpret_cast<const char*>(payload.data() + source.wordSlot),
        payload.size() - source.wordSlot);
    if (const auto end = text.find('\0'); end != std::string::npos) {
        text.resize(end);
    }
    return text;
}

std::optional<ResolvedValue> readValue(const records::LegacyRecordIndex& index,
    std::uint16_t cmper, const SourceLocation& source)
{
    const auto tag = source.identity;
    const std::size_t wordIndex = static_cast<std::size_t>(source.incidence)
        * records::otherWordCount + source.wordSlot;
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
    const auto family = index.getOthers().getArray(source.identity, cmper);
    std::string text;
    bool found = false;
    for (const auto& row : family) {
        if (row.inci < source.incidence) {
            continue;
        }
        found = true;
        const auto bytes = index.getOthers().payloadOf(row);
        text.append(reinterpret_cast<const char*>(bytes.data()), bytes.size());
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
    /// @brief Whether @ref table is one that applies to this file, rather than merely the
    /// first registered for the prefix.
    bool applicable{};
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
            : result.emplace_back(EffectiveTable{table, {}, false});
        // Tables that share a prefix layer onto one destination, but they may differ in how
        // their records are encoded and enumerated. The group must be represented by a table
        // that actually applies to this file, otherwise a later era would be read through an
        // earlier era's encoding and find nothing.
        if (tableApplies && !effective.applicable) {
            effective.table = table;
            effective.applicable = true;
        }
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
    if (table.targetKind == TargetKind::OthersByCmper
            || table.targetKind == TargetKind::OthersFromRecords) {
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

std::optional<ResolvedValue> readSourceValue(
    const records::LegacyRecordIndex& index, RecordEncoding encoding,
    std::uint16_t cmper, const SourceLocation& source, ByteOrder byteOrder)
{
    return encoding == RecordEncoding::ClassRecord
        ? readClassValue(index, cmper, source, byteOrder)
        : readValue(index, cmper, source);
}

void applyLegacyMappings(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument, ImportReport& report)
{
    if (!referenceDocument || referenceDocument == document) {
        throw std::logic_error(
            "Legacy mappings require a separate, fully formed reference document");
    }
    // ClefOptions is rebuilt rather than seeded, so its object must exist before the tables
    // run: the clef tables overlay that object's scalars and report the ones this source
    // cannot supply as Finale 27 defaults.
    options::captureClefOptions(index, profile, document, referenceDocument, report);
    applyMappingTables(registeredTables(), index, profile, document, report);
    options::validateClefOptions(document, report);
    options::captureFontOptions(index, profile, document, referenceDocument, report);
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
            const auto& pool = table.encoding == RecordEncoding::ClassRecord
                ? index.getClassRecords() : index.getOthers();
            for (const auto cmper : pool.cmpersForTag(table.recordIdentity)) {
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
                // A capture pass runs before the tables and may already have established
                // this field, most often as era behavior that no record stores. Claiming it
                // as a synthesized default afterwards would both duplicate the entry and
                // downgrade what is known about it, so the earlier claim stands.
                if (!field.readable
                    && std::any_of(report.fields.begin(), report.fields.end(),
                        [&](const FieldInfo& existing) { return existing.target == info.target; })) {
                    continue;
                }
                // A text field has no numeric default to report, and an object created from
                // records has no seeded value at all, so both leave the raw value at zero.
                if (field.reporting->read) {
                    info.rawValue = field.reporting->read(target.instance);
                }

                if (field.readable) {
                    const auto selector =
                        table.targetKind == TargetKind::OptionsSingleton
                            ? field.readable->source.selector : target.cmper;
                    const bool classRecord = table.encoding == RecordEncoding::ClassRecord;
                    if (field.readable->kind == FieldKind::Text) {
                        const auto text = classRecord
                            ? readClassText(index, selector, field.readable->source)
                            : readText(index, selector, field.readable->source);
                        if (text) {
                            field.readable->applyText(target.instance, *text);
                            info.origin = ValueOrigin::LegacyMus;
                            info.rawValue = static_cast<std::int64_t>(text->size());
                        }
                        report.fields.push_back(std::move(info));
                        continue;
                    }
                    const auto resolved = readSourceValue(index, table.encoding, selector,
                        field.readable->source, profile.byteOrder);
                    if (resolved) {
                        field.readable->apply(target.instance, resolved->value);
                        info.origin = ValueOrigin::LegacyMus;
                        info.blockOffset = resolved->blockOffset;
                        info.decodedOffset = resolved->decodedOffset;
                        info.rawValue = resolved->value;
                    }
                }
                report.fields.push_back(std::move(info));
            }
            if (table.finalizeTarget) {
                table.finalizeTarget(target.instance, profile);
            }
        }
    }
}


} // namespace finale_mus_reader
