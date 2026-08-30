// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/support/legacy_mapping.h"

#include <algorithm>
#include <bit>
#include <cmath>
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

double legacySinglePrecision(std::int64_t value)
{
    return static_cast<double>(
        std::bit_cast<float>(static_cast<std::uint32_t>(value)));
}

musx::dom::Efix legacyPointsToEfix(double value)
{
    return static_cast<musx::dom::Efix>(std::llround(
        value * musx::dom::EVPU_PER_POINT * musx::dom::EFIX_PER_EVPU));
}

musx::dom::Efix legacyTenThousandthsPointToEfix(std::int64_t value)
{
    constexpr double storedUnitsPerPoint = 10000.0;
    return legacyPointsToEfix(static_cast<double>(value) / storedUnitsPerPoint);
}

namespace {

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
std::optional<InstanceKey> importedInstanceKey(const musx::dom::EnigmaBase& object)
{
    if (const auto* other = dynamic_cast<const musx::dom::OthersBase*>(&object)) {
        return InstanceKey{typeid(object), other->getSourcePartId(), other->getCmper(),
            other->getInci(), std::nullopt};
    }
    if (const auto* detail = dynamic_cast<const musx::dom::DetailsBase*>(&object)) {
        return InstanceKey{typeid(object), detail->getSourcePartId(), detail->getCmper1(),
            detail->getInci(), detail->getCmper2()};
    }
    if (const auto* text = dynamic_cast<const musx::dom::TextsBase*>(&object)) {
        return InstanceKey{typeid(object), musx::dom::SCORE_PARTID,
            text->getTextNumber(), std::nullopt, std::nullopt};
    }
    if (dynamic_cast<const musx::dom::OptionsBase*>(&object)) {
        return InstanceKey{typeid(object), musx::dom::SCORE_PARTID, std::nullopt,
            std::nullopt, std::nullopt};
    }
    return std::nullopt;
}
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

} // namespace

std::optional<RecordFamilySource> selectRecordFamilySource(const ImportContext& context,
    const records::LegacyRowPool& fixedPool, const records::LegacyRowPool& classPool,
    records::LegacyTag fixedTag, records::LegacyTag classId)
{
    switch (context.profile.epoch) {
    case FormatEpoch::CodaBanner:
    case FormatEpoch::UncompressedLegacy:
    case FormatEpoch::DclLegacy:
        return RecordFamilySource{&fixedPool, fixedTag, false};
    case FormatEpoch::ZlibLegacy:
        return RecordFamilySource{&classPool, classId, true};
    }
    return std::nullopt;
}

std::vector<std::uint8_t> collectRecordPayload(
    const RecordFamilySource& source, std::span<const records::LegacyRow> rows)
{
    std::vector<std::uint8_t> result;
    for (const auto& row : rows) {
        const auto payload = source.pool->payloadOf(row);
        result.insert(result.end(), payload.begin(), payload.end());
    }
    return result;
}

std::vector<std::int16_t> collectRecordWords(
    const RecordFamilySource& source, std::span<const records::LegacyRow> rows,
    ByteOrder byteOrder)
{
    return payloadWords(collectRecordPayload(source, rows), byteOrder);
}

std::uint16_t payloadWord(
    std::span<const std::uint8_t> payload, std::size_t offset, ByteOrder byteOrder)
{
    return byteOrder == ByteOrder::BigEndian
        ? static_cast<std::uint16_t>((static_cast<std::uint16_t>(payload[offset]) << 8U)
            | payload[offset + 1])
        : static_cast<std::uint16_t>(payload[offset]
            | (static_cast<std::uint16_t>(payload[offset + 1]) << 8U));
}

std::int32_t payloadLong(std::span<const std::uint8_t> payload,
    std::size_t offset, ByteOrder byteOrder, LongWordOrder wordOrder)
{
    const auto first = payloadWord(payload, offset, byteOrder);
    const auto second = payloadWord(payload, offset + 2, byteOrder);
    const auto high = wordOrder == LongWordOrder::HighFirst ? first : second;
    const auto low = wordOrder == LongWordOrder::HighFirst ? second : first;
    return static_cast<std::int32_t>(
        (static_cast<std::uint32_t>(high) << 16U) | low);
}

std::string payloadString(std::span<const std::uint8_t> payload,
    std::size_t offset, std::size_t capacity)
{
    const auto available = (std::min)(capacity, payload.size() - offset);
    std::string result(
        reinterpret_cast<const char*>(payload.data() + offset), available);
    if (const auto end = result.find('\0'); end != std::string::npos) {
        result.resize(end);
    }
    return result;
}

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
        FINALE_MUS_READER_IMPORTER(ImportAccidentalOptions, &options::importAccidentalOptions),
        FINALE_MUS_READER_IMPORTER(ImportAlternateNotationOptions, &options::importAlternateNotationOptions),
        FINALE_MUS_READER_IMPORTER(ImportAugmentationDotOptions, &options::importAugmentationDotOptions),
        FINALE_MUS_READER_IMPORTER(ImportBarlineOptions, &options::importBarlineOptions),
        FINALE_MUS_READER_IMPORTER(ImportBeamOptions, &options::importBeamOptions),
        FINALE_MUS_READER_IMPORTER(ImportClefOptions, &options::importClefOptions),
        FINALE_MUS_READER_IMPORTER(ImportFlagOptions, &options::importFlagOptions),
        FINALE_MUS_READER_IMPORTER(ImportGraceNoteOptions, &options::importGraceNoteOptions),
        FINALE_MUS_READER_IMPORTER(ImportKeySignatureOptions, &options::importKeySignatureOptions),
        FINALE_MUS_READER_IMPORTER(ImportLineCurveOptions, &options::importLineCurveOptions),
        FINALE_MUS_READER_IMPORTER(ImportLyricOptions, &options::importLyricOptions),
        FINALE_MUS_READER_IMPORTER(ImportMiscOptions, &options::importMiscOptions),
        FINALE_MUS_READER_IMPORTER(ImportMultimeasureRestOptions, &options::importMultimeasureRestOptions),
        FINALE_MUS_READER_IMPORTER(ImportMusicSpacingOptions, &options::importMusicSpacingOptions),
        FINALE_MUS_READER_IMPORTER(ImportPianoBraceBracketOptions, &options::importPianoBraceBracketOptions),
        FINALE_MUS_READER_IMPORTER(ImportRepeatOptions, &options::importRepeatOptions),
        FINALE_MUS_READER_IMPORTER(ImportSmartShapeOptions, &options::importSmartShapeOptions),
        FINALE_MUS_READER_IMPORTER(ImportStemOptions, &options::importStemOptions),
        FINALE_MUS_READER_IMPORTER(ImportTextOptions, &options::importTextOptions),
        // others
        FINALE_MUS_READER_IMPORTER(ImportFretInstruments, &others::importFretInstruments),
        FINALE_MUS_READER_IMPORTER(ImportFretboardGroups, &others::importFretboardGroups),
        FINALE_MUS_READER_IMPORTER(ImportFretboardStyles, &others::importFretboardStyles),
        // ChordOptions follows its source-owned fret definitions so its default references
        // can be accepted only when their targets exist.
        FINALE_MUS_READER_IMPORTER(ImportChordOptions, &options::importChordOptions),
        FINALE_MUS_READER_IMPORTER(ImportLayerAttributes, &others::importLayerAttributes),
        FINALE_MUS_READER_IMPORTER(ImportPageGraphicAssignments, &others::importPageGraphicAssignments),
        FINALE_MUS_READER_IMPORTER(ImportShapeGraphicAssignments, &others::importShapeGraphicAssignments),
        FINALE_MUS_READER_IMPORTER(ImportShapeDefinitions, &others::importShapeDefinitions),
        FINALE_MUS_READER_IMPORTER(ImportSmartShapeCustomLines, &others::importSmartShapeCustomLines),
        // details
        FINALE_MUS_READER_IMPORTER(ImportFretboardDiagrams, &details::importFretboardDiagrams),
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
        const bool tableApplies = sourceMatches(profile, table->epochs)
            && (!table->sourceApplies || table->sourceApplies(profile))
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
            const bool readable = tableApplies
                && (!field->sourceApplies || field->sourceApplies(profile));
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

musx::dom::ImportObjectCallback baselineObjectReporter(ImportReport& report)
{
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    return [&report](const musx::dom::EnigmaBase& object) {
        if (const auto instance = importedInstanceKey(object)) {
            report.setInstanceOrigin(*instance, ValueOrigin::Finale27Default);
        }
    };
#else
    static_cast<void>(report);
    return {};
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

std::optional<ResolvedValue> readSourceValue(
    const records::LegacyRecordIndex& index, RecordEncoding encoding,
    std::uint16_t cmper, const SourceLocation& source, ByteOrder byteOrder)
{
    return encoding == RecordEncoding::ClassRecord
        ? readClassValue(index, cmper, source, byteOrder)
        : readValue(index, cmper, source);
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

bool storesCodaMigratedPointSizes(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    return profile.epoch == FormatEpoch::CodaBanner
        && readGlobalWords(index, profile, codaMigratedPointSizeSelector).present;
}

bool storesCodaFloatPointSizes(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    return profile.epoch == FormatEpoch::CodaBanner
        && !storesCodaMigratedPointSizes(index, profile);
}

bool storesLoneStemFlagLayout(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    if (profile.epoch != FormatEpoch::CodaBanner
        && profile.epoch != FormatEpoch::UncompressedLegacy) {
        return false;
    }
    constexpr std::uint16_t beamFlagsSelector = 41;
    constexpr std::size_t beamFlagsSlot = 1;
    const auto words = readNumericGlobalWords(index, beamFlagsSelector);
    if (!words.present || words.words.size() <= beamFlagsSlot) return true;
    constexpr std::uint16_t loneStemFlag = 0x0001;
    return (static_cast<std::uint16_t>(words.words[beamFlagsSlot])
        & ~loneStemFlag) == 0;
}

bool storesPackedBeamFlagLayout(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    return !storesLoneStemFlagLayout(index, profile);
}

bool storesPreFinale35StemAndBeamUnits(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    if (profile.epoch == FormatEpoch::ZlibLegacy) return false;
    constexpr std::uint16_t stemConnectionSelector = 40;
    constexpr std::size_t earlyConnectionCount = 32;
    constexpr std::size_t connectionWords = 6;
    const auto family = readNumericGlobalWords(index, stemConnectionSelector);
    return family.present
        && family.words.size() <= earlyConnectionCount * connectionWords;
}

bool storesFinale35StemAndBeamUnits(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    return !storesPreFinale35StemAndBeamUnits(index, profile);
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
    const auto reportImported = baselineObjectReporter(report);
    // Keyed by the reference comparator, and scoped to this one import. Two clefs naming the same
    // reference shape share a copy; this is not a search of the target for something equivalent,
    // which is never correct for a shape.
    std::map<musx::dom::Cmper, musx::dom::Cmper> copiedShapes;
    bool reportedFailure = false;
    for (auto& request : pending.shapes) {
        musx::dom::Cmper resolved = 0;
        if (const auto found = copiedShapes.find(request.referenceShapeId);
            found != copiedShapes.end()) {
            resolved = found->second;
        } else if (const auto source = referenceDocument->getOthers()
                ->get<musx::dom::others::ShapeDef>(
                    musx::dom::SCORE_PARTID, request.referenceShapeId)) {
            if (const auto imported = musx::dom::others::importShapeDefInto(
                    document, source, reportImported)) {
                resolved = *imported;
                copiedShapes.emplace(request.referenceShapeId, resolved);
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
    if (!pending.shapes.empty()) {
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Verbose,
            "Copied " + std::to_string(copiedShapes.size()) + " Finale 27 shape(s) for "
            + std::to_string(pending.shapes.size()) + " clef definition(s)."});
    }

    std::map<musx::dom::Cmper, musx::dom::Cmper> copiedCustomLines;
    reportedFailure = false;
    for (auto& request : pending.customLines) {
        musx::dom::Cmper resolved = 0;
        if (const auto found = copiedCustomLines.find(request.referenceLineId);
            found != copiedCustomLines.end()) {
            resolved = found->second;
        } else if (const auto source = referenceDocument->getOthers()
                ->get<musx::dom::others::SmartShapeCustomLine>(
                    musx::dom::SCORE_PARTID, request.referenceLineId)) {
            if (const auto imported =
                    musx::dom::others::importSmartShapeCustomLineInto(
                        document, source, reportImported)) {
                resolved = *imported;
                copiedCustomLines.emplace(request.referenceLineId, resolved);
            }
        }
        if (resolved == 0) {
            if (!reportedFailure) {
                report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
                    "A Finale 27 custom line could not be copied into this document; the "
                    "Smart Shape tools that use it have no line style."});
                reportedFailure = true;
            }
            continue;
        }
        request.assign(resolved);
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        if (auto* info = report.findField(request.reportInstance, request.reportMember)) {
            info->origin = ValueOrigin::Finale27Default;
            info->rawValue = resolved;
        }
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    }
    if (!pending.customLines.empty()) {
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Verbose,
            "Copied " + std::to_string(copiedCustomLines.size())
            + " Finale 27 custom line(s) for "
            + std::to_string(pending.customLines.size()) + " Smart Shape option field(s)."});
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
                        const auto adjusted = field.readable->sourceAdjustment
                            ? field.readable->sourceAdjustment(
                                  resolved->value, index, profile)
                            : std::nullopt;
                        field.readable->apply(target.instance,
                            adjusted.value_or(resolved->value));
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
                        info.origin = adjusted ? ValueOrigin::LegacyMusAdjusted
                                               : ValueOrigin::LegacyMus;
                        info.blockOffset = resolved->blockOffset;
                        info.decodedOffset = resolved->decodedOffset;
                        info.rawValue = resolved->value;
                        info.sourceIdentity = field.readable->source.identity;
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
