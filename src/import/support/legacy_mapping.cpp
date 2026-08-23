// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/support/legacy_mapping.h"

#include <algorithm>
#include <stdexcept>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "import/details.h"
#include "import/entries.h"
#include "import/options.h"
#include "import/others.h"
#include "import/texts.h"
#include "reader/timing.h"

namespace finale_mus_reader {

namespace {

/// @brief Every musxdom class this reader recovers, one entry each, grouped by pool.
/// @details The registry says what is imported and in what order, and nothing else. How many
/// physical layouts a class has, which epochs each covers, and whether it needs a capture
/// pass before its tables or a check after them are decisions its own translation unit owns.
///
/// Order is a dependency statement wherever one class reads what another has already built.
/// FontDefinitions and FontOptions form a bootstrap stage: option and text classes name font
/// comparators, and text conversion also needs its class default before decoding literal bytes.
/// The remaining importers follow Finale's pool order. Within the others pool, a custom line
/// style can therefore decode its stored character through the charset of its named font.
///
/// The list is written out rather than assembled by self-registration, so that a static
/// archive cannot discard an importer nothing else references.
struct RegisteredImporter
{
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    timing::Phase phase;
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    ClassImporter importer;
};

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
#define FINALE_MUS_READER_IMPORTER(phase, importer) {timing::Phase::phase, importer}
#else
#define FINALE_MUS_READER_IMPORTER(phase, importer) {importer}
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

const std::vector<RegisteredImporter>& registeredImporters()
{
    // Keep one line per importer so the complete execution order stays directly scannable.
    // clang-format off
    static const std::vector<RegisteredImporter> result = {
        // bootstrap
        FINALE_MUS_READER_IMPORTER(ImportFontDefinitions, &others::importFontDefinitions),
        FINALE_MUS_READER_IMPORTER(ImportFontOptions, &options::importFontOptions),
        // options
        FINALE_MUS_READER_IMPORTER(ImportClefOptions, &options::importClefOptions),
        FINALE_MUS_READER_IMPORTER(ImportLyricOptions, &options::importLyricOptions),
        FINALE_MUS_READER_IMPORTER(ImportMultimeasureRestOptions, &options::importMultimeasureRestOptions),
        FINALE_MUS_READER_IMPORTER(ImportMusicSpacingOptions, &options::importMusicSpacingOptions),
        FINALE_MUS_READER_IMPORTER(ImportStemOptions, &options::importStemOptions),
        FINALE_MUS_READER_IMPORTER(ImportTextOptions, &options::importTextOptions),
        // others
        FINALE_MUS_READER_IMPORTER(ImportLayerAttributes, &others::importLayerAttributes),
        FINALE_MUS_READER_IMPORTER(ImportPageGraphicAssignments, &others::importPageGraphicAssignments),
        FINALE_MUS_READER_IMPORTER(ImportShapeGraphicAssignments, &others::importShapeGraphicAssignments),
        FINALE_MUS_READER_IMPORTER(ImportShapeDefinitions, &others::importShapeDefinitions),
        FINALE_MUS_READER_IMPORTER(ImportSmartShapeCustomLines, &others::importSmartShapeCustomLines),
        // details
        FINALE_MUS_READER_IMPORTER(ImportMeasureGraphicAssignments, &details::importMeasureGraphicAssignments),
        // entries (none recovered yet)
        // texts
        FINALE_MUS_READER_IMPORTER(ImportTexts, &texts::importTexts),
        FINALE_MUS_READER_IMPORTER(ImportTextBlocks, &others::importTextBlocks),
    };
    // clang-format on
    return result;
}

#undef FINALE_MUS_READER_IMPORTER

// Reads a numeric value from a class-identified record, addressed by byte offset inside a
// single record's payload rather than by word slot across an incidence stream.
std::optional<ResolvedValue> readClassValue(const records::LegacyRecordIndex& index,
    std::uint16_t cmper, const SourceLocation& source, ByteOrder byteOrder)
{
    const auto* row = index.getClassOthers().get(
        source.identity, cmper, 0, source.incidence);
    if (!row) {
        return std::nullopt;
    }
    const auto payload = index.getClassOthers().payloadOf(*row);
    const std::size_t width = source.width == ValueWidth::Long ? 4
        : source.width == ValueWidth::Byte ? 1 : 2;
    if (source.wordSlot + width > payload.size()) {
        return std::nullopt;
    }
    // Payload numbers follow the file's byte order, as everything numeric does. Finale 2007
    // in particular is mostly big-endian. Each width is read through its own signed type,
    // because the fixed-row path reads one through a signed word and the two encodings must
    // not disagree about the same logical option: an Evpu of -12 read unsigned arrives as
    // 65524 and is assigned as such. A bit range is exempt, being a magnitude rather than a
    // number with a sign, and takes its value from the unsigned payload below.
    const auto readWord = [&](std::size_t at) {
        return byteOrder == ByteOrder::BigEndian
            ? static_cast<std::uint16_t>(
                  (static_cast<std::uint16_t>(payload[at]) << 8U) | payload[at + 1])
            : static_cast<std::uint16_t>(
                  payload[at] | (static_cast<std::uint16_t>(payload[at + 1]) << 8U));
    };
    std::int64_t value = 0;
    if (width == 4) {
        // Not a plain four-byte read: the zlib serialization kept the two payload words the
        // fixed rows carried, so their order is the mapping's own and the same
        // MACFOURBYTE/WINFOURBYTE rule decides it. On a little-endian file the two differ,
        // and the stem offset is the field that shows it.
        const auto first = readWord(source.wordSlot);
        const auto second = readWord(source.wordSlot + 2);
        const std::uint32_t combined = source.longOrder == LongWordOrder::HighFirst
            ? (static_cast<std::uint32_t>(first) << 16U) | second
            : (static_cast<std::uint32_t>(second) << 16U) | first;
        value = static_cast<std::int32_t>(combined);
    } else if (width == 2) {
        value = static_cast<std::int16_t>(readWord(source.wordSlot));
    } else {
        value = static_cast<std::int8_t>(payload[source.wordSlot]);
    }
    if (source.bits.bitCount != 0) {
        const auto mask = (std::uint64_t{1} << source.bits.bitCount) - 1U;
        value = static_cast<std::int64_t>(
            (static_cast<std::uint64_t>(value) >> source.bits.firstBit) & mask);
    }
    return ResolvedValue{value, row->blockOffset, row->decodedOffset};
}

// Reads text from a class-identified record: the payload runs to its own end, so the name
// simply occupies whatever remains after the fields before it.
std::optional<std::string> readClassText(const records::LegacyRecordIndex& index,
    std::uint16_t cmper, const SourceLocation& source)
{
    const auto* row = index.getClassOthers().get(source.identity, cmper, 0, 0);
    if (!row) {
        return std::nullopt;
    }
    const auto payload = index.getClassOthers().payloadOf(*row);
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
        // **Unverified: which half it keeps.** No one-byte mapping is promoted until a
        // document settles it.
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
    const std::vector<const MappingTable*>& tables,
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    std::vector<EffectiveTable> result;
    for (const auto* table : tables) {
        const bool tableApplies = epochMatches(table->epochs, profile.epoch)
            && table->versions.includes(profile.version)
            && (!table->applies || table->applies(index, profile));
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

std::string reportMember(const FieldMapping& field)
{
    std::string result;
    // A row names its destination once, as the C++ path that reaches it, so a field inside a
    // contained object arrives here spelled with `->`. Every report target names a document
    // path instead, whose separator is a dot, which is also how the capture passes spell the
    // nested targets they build by hand.
    for (const char* at = field.fieldName; *at != '\0'; ++at) {
        if (at[0] == '-' && at[1] == '>') {
            result += '.';
            ++at;
            continue;
        }
        result += *at;
    }
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

records::LegacyTag numericGlobalTag(std::uint16_t selector)
{
    const char text[2] = {static_cast<char>('0' + (selector / 10) % 10),
        static_cast<char>('0' + selector % 10)};
    return records::packTag(std::string_view(text, std::size(text)));
}

GlobalSelectorWords readNumericGlobalWords(
    const records::LegacyRecordIndex& index, std::uint16_t selector)
{
    GlobalSelectorWords result;
    const auto rows = index.getOthers().getArray(numericGlobalTag(selector), GLOBALS_CMPER);
    result.present = !rows.empty();
    if (result.present) {
        result.blockOffset = rows.front().blockOffset;
        result.decodedOffset = rows.front().decodedOffset;
    }
    for (const auto& row : rows) {
        for (std::uint8_t slot = 0; slot < row.wordCount; ++slot) {
            result.words.push_back(row.words[slot]);
        }
    }
    return result;
}

GlobalSelectorWords readGlobalWords(const records::LegacyRecordIndex& index,
    const SourceProfile& profile, std::uint16_t selector)
{
    if (profile.epoch != FormatEpoch::ZlibLegacy) {
        return readNumericGlobalWords(index, selector);
    }
    GlobalSelectorWords result;
    const auto* row = index.getClassOthers().get(
        numericGlobalClass(selector), GLOBALS_CMPER, 0, 0);
    if (!row) {
        return result;
    }
    result.words = payloadWords(index.getClassOthers().payloadOf(*row), profile.byteOrder);
    result.present = true;
    result.blockOffset = row->blockOffset;
    result.decodedOffset = row->decodedOffset;
    return result;
}

std::int16_t wordAt(const std::vector<std::int16_t>& words, std::size_t index)
{
    return index < words.size() ? words[index] : static_cast<std::int16_t>(0);
}

std::vector<std::int16_t> payloadWords(
    std::span<const std::uint8_t> payload, ByteOrder byteOrder)
{
    std::vector<std::int16_t> words;
    words.reserve(payload.size() / 2);
    for (std::size_t offset = 0; offset + 2 <= payload.size(); offset += 2) {
        words.push_back(byteOrder == ByteOrder::BigEndian
            ? static_cast<std::int16_t>(
                  (static_cast<std::uint16_t>(payload[offset]) << 8U) | payload[offset + 1])
            : static_cast<std::int16_t>(
                  payload[offset] | (static_cast<std::uint16_t>(payload[offset + 1]) << 8U)));
    }
    return words;
}

std::uint32_t narrowCodepoint(std::int16_t stored)
{
    return static_cast<std::uint32_t>(static_cast<std::uint8_t>(stored));
}

std::uint32_t wideCodepoint(std::int16_t low, std::int16_t high)
{
    return static_cast<std::uint32_t>(static_cast<std::uint16_t>(low))
        | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(high)) << 16U);
}

namespace {

/// @brief Copies every requested reference object into the document and fills in its comparator.
/// @details The one phase that belongs to no single class: it drains what every importer asked
/// for. File-local because @ref applyLegacyMappings is its only caller and the ordering rule --
/// that this runs after every pool is filled, and that nothing may allocate an `others`
/// comparator afterwards -- is enforced there rather than by any caller of this header.
void resolveDeferredReferences(const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument,
    PendingReferences& pending, ImportReport& report)
{
    // Keyed by the reference comparator, and scoped to this one import. Two clefs naming the same
    // reference shape share a copy; this is not a search of the target for something equivalent,
    // which is never correct for a shape.
    std::map<musx::dom::Cmper, musx::dom::Cmper> copied;
    bool reportedFailure = false;
    for (auto& request : pending) {
        musx::dom::Cmper resolved = 0;
        if (const auto found = copied.find(request.referenceShapeId); found != copied.end()) {
            resolved = found->second;
        } else if (const auto source = referenceDocument->getOthers()
                ->get<musx::dom::others::ShapeDef>(
                    musx::dom::SCORE_PARTID, request.referenceShapeId)) {
            if (const auto imported = musx::dom::others::importShapeDefInto(document, source)) {
                resolved = *imported;
                copied.emplace(request.referenceShapeId, resolved);
            }
        }
        if (resolved == 0) {
            if (!reportedFailure) {
                report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
                    "A Finale 27 shape could not be copied into this document; the clefs that "
                    "use it read as blank."});
                reportedFailure = true;
            }
            continue;
        }
        request.assign(resolved);
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        if (auto* info = report.findField(request.reportInstance, request.reportMember)) {
            info->rawValue = resolved;
        }
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    }
    if (!pending.empty()) {
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Verbose,
            "Copied " + std::to_string(copied.size()) + " Finale 27 shape(s) for "
            + std::to_string(pending.size()) + " clef definition(s)."});
    }
}

} // namespace

void applyLegacyMappings(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    std::span<const std::uint8_t> source, const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument, ImportReport& report,
    musx::factory::ConstructionContext& construction)
{
    if (!referenceDocument || referenceDocument == document) {
        throw std::logic_error(
            "Legacy mappings require a separate, fully formed reference document");
    }
    PendingReferences pending;
    const ImportContext context{
        index, profile, source, document, referenceDocument, report, pending, construction};
    for (const auto& entry : registeredImporters()) {
        FINALE_MUS_READER_TIMED_SCOPE(entry.phase);
        entry.importer(context);
    }
    // Last, and it must stay last. Copying a reference object allocates comparators, so this
    // cannot run while any pool is still being filled. It is the one phase that belongs to no
    // single class: it drains what every importer asked for.
    {
        FINALE_MUS_READER_TIMED_SCOPE(timing::Phase::DeferredReferences);
        resolveDeferredReferences(document, referenceDocument, pending, report);
    }
}


void applyMappingTables(const std::vector<const MappingTable*>& tables,
    const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document, ImportReport& report)
{
    for (const auto& effective : buildEffectiveTables(tables, index, profile)) {
        const auto& table = *effective.table;
        std::vector<MappingTarget> targets;
        if (table.targetKind == TargetKind::OthersFromRecords) {
            // The legacy file is the only source of these objects, so they are created from
            // the comparators the records themselves carry rather than found in the pool.
            // Nothing is created when no table applies to this file: with no seeded object to
            // overlay there is no default to report either, and an object built from a record
            // no table can read would be an empty fabrication.
            const auto& pool = table.encoding == RecordEncoding::ClassRecord
                ? index.getClassOthers() : index.getOthers();
            if (effective.applicable) {
                for (const auto cmper : pool.cmpersForTag(table.recordIdentity)) {
                    targets.push_back(table.createTarget(document, cmper));
                }
            }
        } else {
            targets = table.enumerateTargets(document);
        }

        for (const auto& target : targets) {
            for (const auto& field : effective.fields) {
                // A record that states its own layout decides which of its mutually exclusive
                // fields exist. Such a field is neither read nor reported for a record that
                // does not select it, because the destination itself is absent there. The test
                // belongs to the destination rather than to any one era, so it is taken from
                // the reporting row, and it sees every field declared before it already
                // applied.
                if (field.reporting->targetApplies
                    && !field.reporting->targetApplies(target.instance)) {
                    continue;
                }
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
                const InstanceKey reportInstance{target.classType, musx::dom::SCORE_PARTID,
                    table.targetKind == TargetKind::OptionsSingleton
                        ? std::optional<musx::dom::Cmper>{}
                        : std::optional<musx::dom::Cmper>{target.cmper},
                    std::nullopt, std::nullopt};
                const auto member = reportMember(*field.reporting);
                FieldInfo info;
                info.origin = ValueOrigin::Finale27Default;
                // A capture pass runs before the tables and may already have established
                // this field, most often as era behavior that no record stores. Claiming it
                // as a synthesized default afterwards would both duplicate the entry and
                // downgrade what is known about it, so the earlier claim stands.
                if (!field.readable && report.findField(reportInstance, member)) {
                    continue;
                }
                // A text field has no numeric default to report, and an object created from
                // records has no seeded value at all, so both leave the raw value at zero.
                if (field.reporting->read) {
                    info.rawValue = field.reporting->read(target.instance);
                }
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

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
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
                            info.origin = ValueOrigin::LegacyMus;
                            info.rawValue = static_cast<std::int64_t>(text->size());
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
                        }
                        FINALE_MUS_READER_REPORT_FIELD(
                            report, reportInstance, member, std::move(info));
                        continue;
                    }
                    const auto resolved = readSourceValue(index, table.encoding, selector,
                        field.readable->source, profile.byteOrder);
                    if (resolved) {
                        field.readable->apply(target.instance, resolved->value);
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
                        info.origin = ValueOrigin::LegacyMus;
                        info.blockOffset = resolved->blockOffset;
                        info.decodedOffset = resolved->decodedOffset;
                        info.rawValue = resolved->value;
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
                    }
                }
                FINALE_MUS_READER_REPORT_FIELD(
                    report, reportInstance, member, std::move(info));
            }
            if (table.finalizeTarget) {
                table.finalizeTarget(target.instance, profile, document);
            }
        }
    }
}


} // namespace finale_mus_reader
