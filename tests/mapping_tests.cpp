// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// White-box tests for the table-driven mapping framework. These drive the engine with
// synthetic record streams and purpose-built tables so that each mechanism can be
// exercised on its own, including the ones no promoted mapping uses yet.

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "container/mus_container.h"
#include "import/legacy_mapping.h"
#include "records/legacy_record_index.h"

#include "musx/musx.h"

#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#define FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML
#endif

#include "musx/xml/PugiXmlImpl.h"

#ifdef FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML
#undef MUSX_USE_PUGIXML
#undef FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML
#endif

namespace {

using finale_mus_reader::ByteOrder;
using finale_mus_reader::FormatEpoch;
using finale_mus_reader::ImportReport;
using finale_mus_reader::SourceVersion;
using finale_mus_reader::ValueOrigin;
using finale_mus_reader::mapping::EpochMask;
using finale_mus_reader::mapping::FieldMapping;
using finale_mus_reader::mapping::GLOBALS_CMPER;
using finale_mus_reader::mapping::LongWordOrder;
using finale_mus_reader::mapping::MappingTable;
using finale_mus_reader::mapping::SourceProfile;
using finale_mus_reader::mapping::TargetKind;
using finale_mus_reader::mapping::VersionRange;
using finale_mus_reader::records::LegacyRecordIndex;
using Spacing = musx::dom::options::MusicSpacingOptions;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

/// @brief One synthetic 16-byte legacy row.
struct SyntheticRow
{
    std::uint16_t cmper{};
    const char* tag{};
    std::array<std::int16_t, 6> words{};
};

/// @brief Builds a parsed container holding the given rows as an uncompressed other pool.
finale_mus_reader::container::ParsedContainer makeContainer(
    const std::vector<SyntheticRow>& rows)
{
    finale_mus_reader::container::ParsedContainer parsed;
    parsed.formatEpoch = FormatEpoch::UncompressedLegacy;
    parsed.byteOrder = ByteOrder::BigEndian;

    finale_mus_reader::container::DecodedBlock block;
    block.info.type = 0x0001;
    for (const auto& row : rows) {
        block.data.push_back(static_cast<std::uint8_t>(row.cmper >> 8U));
        block.data.push_back(static_cast<std::uint8_t>(row.cmper));
        block.data.push_back(static_cast<std::uint8_t>(row.tag[0]));
        block.data.push_back(static_cast<std::uint8_t>(row.tag[1]));
        for (const auto word : row.words) {
            const auto raw = static_cast<std::uint16_t>(word);
            block.data.push_back(static_cast<std::uint8_t>(raw >> 8U));
            block.data.push_back(static_cast<std::uint8_t>(raw));
        }
    }
    block.info.decodedSize = block.data.size();
    parsed.blocks.push_back(std::move(block));
    return parsed;
}

/// @brief A bare document carrying one music spacing options instance.
musx::dom::DocumentPtr makeDocument(Spacing** instanceOut)
{
    auto session = musx::factory::DocumentFactory::begin();
    auto document = session.getDocument();
    auto spacing = std::make_shared<Spacing>(
        document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All);
    spacing->minWidth = 111;
    spacing->maxWidth = 222;
    spacing->referenceDuration = 333;
    spacing->avoidColNotes = false;
    *instanceOut = spacing.get();
    document->getOptions()->add(Spacing::XmlNodeName, spacing);
    return std::move(session).finish();
}

SourceProfile profileFor(std::uint8_t major, std::uint8_t minor = 0)
{
    SourceProfile profile;
    profile.epoch = FormatEpoch::UncompressedLegacy;
    profile.byteOrder = ByteOrder::BigEndian;
    SourceVersion version;
    version.major = major;
    version.minor = minor;
    profile.version = version;
    return profile;
}

const finale_mus_reader::FieldInfo& field(const ImportReport& report, const std::string& target)
{
    const auto found = std::find_if(report.fields.begin(), report.fields.end(),
        [&](const finale_mus_reader::FieldInfo& info) { return info.target == target; });
    expect(found != report.fields.end(), "Missing mapping report for " + target);
    return *found;
}

MappingTable makeTable(const char* prefix, const FieldMapping* fields, std::size_t count,
    VersionRange versions = {}, EpochMask epochs = EpochMask::FixedRow)
{
    return MappingTable{prefix, epochs, versions, TargetKind::OptionsSingleton,
        &finale_mus_reader::mapping::enumerateOptionsTarget<Spacing>, fields, count};
}

// A four-byte value whose words straddle an incidence boundary: the low word is the last
// slot of incidence 1 and the high word is the first slot of incidence 2. This is the
// shape the distilled framework mapping records for MusicSpacingPrefs.scalingFactor.
void testFourByteStraddlesIncidence()
{
    const FieldMapping fields[] = {
        MUS_LONG(Spacing, "94", GLOBALS_CMPER, /*incidence*/ 1, /*slot*/ 5,
            LongWordOrder::LowFirst, referenceDuration),
    };
    const auto table = makeTable("options.spacing", fields, std::size(fields));
    const std::vector<const MappingTable*> tables{&table};

    const auto parsed = makeContainer({
        {GLOBALS_CMPER, "94", {0, 0, 0, 0, 0, 0}},
        {GLOBALS_CMPER, "94", {0, 0, 0, 0, 0, static_cast<std::int16_t>(0x5678)}},
        {GLOBALS_CMPER, "94", {0x1234, 0, 0, 0, 0, 0}},
    });

    Spacing* spacing = nullptr;
    auto document = makeDocument(&spacing);
    ImportReport report;
    finale_mus_reader::mapping::applyMappingTables(
        tables, LegacyRecordIndex::build(parsed), profileFor(7), document, report);

    expect(spacing->referenceDuration == 0x12345678,
        "Four-byte value straddling an incidence boundary was not assembled");
    expect(field(report, "options.spacing.referenceDuration").origin == ValueOrigin::LegacyMus,
        "Straddling four-byte value was not reported as recovered");
}

// The same two words, read with each word order, must disagree.
void testLongWordOrder()
{
    const FieldMapping highFirst[] = {
        MUS_LONG(Spacing, "94", GLOBALS_CMPER, 0, 0, LongWordOrder::HighFirst, referenceDuration),
    };
    const FieldMapping lowFirst[] = {
        MUS_LONG(Spacing, "94", GLOBALS_CMPER, 0, 0, LongWordOrder::LowFirst, referenceDuration),
    };
    const auto parsed = makeContainer({
        {GLOBALS_CMPER, "94", {0x0102, 0x0304, 0, 0, 0, 0}},
    });

    const auto readWith = [&](const FieldMapping* fields) {
        const auto table = makeTable("options.spacing", fields, 1);
        const std::vector<const MappingTable*> tables{&table};
        Spacing* spacing = nullptr;
        auto document = makeDocument(&spacing);
        ImportReport report;
        finale_mus_reader::mapping::applyMappingTables(
            tables, LegacyRecordIndex::build(parsed), profileFor(7), document, report);
        return spacing->referenceDuration;
    };

    expect(readWith(highFirst) == 0x01020304, "High-first four-byte order is wrong");
    expect(readWith(lowFirst) == 0x03040102, "Low-first four-byte order is wrong");
}

// Booleans live as bits of a flag word, so bit addressing has to work before any such
// mapping can be promoted.
void testBitExtraction()
{
    const FieldMapping fields[] = {
        MUS_BIT(Spacing, "94", GLOBALS_CMPER, 0, 5, /*bit*/ 3, avoidColNotes),
    };
    const auto table = makeTable("options.spacing", fields, std::size(fields));
    const std::vector<const MappingTable*> tables{&table};

    const auto readFlag = [&](std::int16_t flagWord) {
        const auto parsed = makeContainer({{GLOBALS_CMPER, "94", {0, 0, 0, 0, 0, flagWord}}});
        Spacing* spacing = nullptr;
        auto document = makeDocument(&spacing);
        ImportReport report;
        finale_mus_reader::mapping::applyMappingTables(
            tables, LegacyRecordIndex::build(parsed), profileFor(7), document, report);
        return spacing->avoidColNotes;
    };

    expect(readFlag(0x0008), "Bit 3 set was not extracted as true");
    expect(!readFlag(0x0004), "An unrelated bit was reported as the mapped bit");
    expect(readFlag(static_cast<std::int16_t>(0xfff8)), "Bit 3 was lost among other bits");
}

// A file with no matching record keeps the seeded value and still reports the field.
void testAbsentRecordKeepsSeededDefault()
{
    const FieldMapping fields[] = {
        MUS_WORD(Spacing, "94", GLOBALS_CMPER, 0, 1, minWidth),
    };
    const auto table = makeTable("options.spacing", fields, std::size(fields));
    const std::vector<const MappingTable*> tables{&table};

    const auto parsed = makeContainer({{7, "ZZ", {0, 0, 0, 0, 0, 0}}});
    Spacing* spacing = nullptr;
    auto document = makeDocument(&spacing);
    ImportReport report;
    finale_mus_reader::mapping::applyMappingTables(
        tables, LegacyRecordIndex::build(parsed), profileFor(7), document, report);

    expect(spacing->minWidth == 111, "An absent record overwrote the seeded default");
    const auto& info = field(report, "options.spacing.minWidth");
    expect(info.origin == ValueOrigin::Finale27Default,
        "An absent record was not reported as a synthesized default");
    expect(info.rawValue == 111, "The reported default did not carry the seeded value");
}

// A row gated to a later version must not be applied to an earlier file, and a file whose
// version could not be recovered matches only ungated rows.
void testVersionGating()
{
    const FieldMapping fields[] = {
        MUS_WORD_V(Spacing, "94", GLOBALS_CMPER, 0, 1,
            finale_mus_reader::mapping::versions::from(12), minWidth),
    };
    const auto table = makeTable("options.spacing", fields, std::size(fields));
    const std::vector<const MappingTable*> tables{&table};
    const auto parsed = makeContainer({{GLOBALS_CMPER, "94", {0, 777, 0, 0, 0, 0}}});

    const auto readWithProfile = [&](const SourceProfile& profile) {
        Spacing* spacing = nullptr;
        auto document = makeDocument(&spacing);
        ImportReport report;
        finale_mus_reader::mapping::applyMappingTables(
            tables, LegacyRecordIndex::build(parsed), profile, document, report);
        return spacing->minWidth;
    };

    expect(readWithProfile(profileFor(7)) == 111,
        "A row gated to a later version was applied to an earlier file");
    expect(readWithProfile(profileFor(12)) == 777,
        "A row gated to this version was not applied");

    SourceProfile unknownVersion;
    unknownVersion.epoch = FormatEpoch::UncompressedLegacy;
    expect(readWithProfile(unknownVersion) == 111,
        "A gated row was applied to a file with no recovered version");
}

// Minor participates in ordering, because major alone does not separate Finale 97 from
// the Finale 3.x line.
void testMinorVersionOrdering()
{
    const FieldMapping fields[] = {
        MUS_WORD_V(Spacing, "94", GLOBALS_CMPER, 0, 1,
            finale_mus_reader::mapping::versions::from(3, 5), minWidth),
    };
    const auto table = makeTable("options.spacing", fields, std::size(fields));
    const std::vector<const MappingTable*> tables{&table};
    const auto parsed = makeContainer({{GLOBALS_CMPER, "94", {0, 888, 0, 0, 0, 0}}});

    const auto readWith = [&](std::uint8_t major, std::uint8_t minor) {
        Spacing* spacing = nullptr;
        auto document = makeDocument(&spacing);
        ImportReport report;
        finale_mus_reader::mapping::applyMappingTables(
            tables, LegacyRecordIndex::build(parsed), profileFor(major, minor), document, report);
        return spacing->minWidth;
    };

    expect(readWith(3, 2) == 111, "Version 3.2 matched a gate starting at 3.5");
    expect(readWith(3, 7) == 888, "Version 3.7 did not match a gate starting at 3.5");
}

// A later table supersedes an earlier one for the same field, so a field that moves costs
// one override row rather than a restatement of the whole table.
void testTableLayering()
{
    const FieldMapping baseFields[] = {
        MUS_WORD(Spacing, "94", GLOBALS_CMPER, 0, 1, minWidth),
        MUS_WORD(Spacing, "94", GLOBALS_CMPER, 0, 2, maxWidth),
    };
    const FieldMapping overrideFields[] = {
        MUS_WORD(Spacing, "94", GLOBALS_CMPER, 0, 3, minWidth),
    };
    const auto base = makeTable("options.spacing", baseFields, std::size(baseFields));
    const auto override = makeTable("options.spacing", overrideFields,
        std::size(overrideFields), finale_mus_reader::mapping::versions::from(12));
    const std::vector<const MappingTable*> tables{&base, &override};

    const auto parsed = makeContainer({{GLOBALS_CMPER, "94", {0, 11, 22, 33, 0, 0}}});

    const auto readWith = [&](std::uint8_t major) {
        Spacing* spacing = nullptr;
        auto document = makeDocument(&spacing);
        ImportReport report;
        finale_mus_reader::mapping::applyMappingTables(
            tables, LegacyRecordIndex::build(parsed), profileFor(major), document, report);
        return std::pair<int, int>{spacing->minWidth, spacing->maxWidth};
    };

    const auto early = readWith(7);
    expect(early.first == 11, "The base location was not used below the override version");
    expect(early.second == 22, "The base table lost an unrelated field");

    const auto late = readWith(12);
    expect(late.first == 33, "The override location did not supersede the base location");
    expect(late.second == 22, "The override table dropped a field it does not restate");
}

// An epoch the tables do not cover still reports its supported fields as defaults.
void testUncoveredEpochStillReports()
{
    const FieldMapping fields[] = {
        MUS_WORD(Spacing, "94", GLOBALS_CMPER, 0, 1, minWidth),
    };
    const auto table = makeTable("options.spacing", fields, std::size(fields));
    const std::vector<const MappingTable*> tables{&table};
    const auto parsed = makeContainer({{GLOBALS_CMPER, "94", {0, 777, 0, 0, 0, 0}}});

    SourceProfile profile;
    profile.epoch = FormatEpoch::ZlibLegacy;
    Spacing* spacing = nullptr;
    auto document = makeDocument(&spacing);
    ImportReport report;
    finale_mus_reader::mapping::applyMappingTables(
        tables, LegacyRecordIndex::build(parsed), profile, document, report);

    expect(spacing->minWidth == 111, "A table was applied outside its epoch");
    expect(field(report, "options.spacing.minWidth").origin == ValueOrigin::Finale27Default,
        "An uncovered epoch did not report its supported field as a default");
}

} // namespace

namespace finale_mus_reader_tests {

void runMappingTests()
{
    testFourByteStraddlesIncidence();
    testLongWordOrder();
    testBitExtraction();
    testAbsentRecordKeepsSeededDefault();
    testVersionGating();
    testMinorVersionOrdering();
    testTableLayering();
    testUncoveredEpochStillReports();
}

} // namespace finale_mus_reader_tests
