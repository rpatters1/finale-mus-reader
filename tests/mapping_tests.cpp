// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// White-box tests for the table-driven mapping framework. These drive the engine with
// synthetic record streams and purpose-built tables so that each mechanism can be
// exercised on its own, including the ones no promoted mapping uses yet.

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "container/mus_container.h"
#include "import/legacy_mapping.h"
#include "import/options/options.h"
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
using finale_mus_reader::EpochMask;
using finale_mus_reader::FieldMapping;
using finale_mus_reader::GLOBALS_CMPER;
using finale_mus_reader::LongWordOrder;
using finale_mus_reader::MappingTable;
using finale_mus_reader::SourceProfile;
using finale_mus_reader::TargetKind;
using finale_mus_reader::VersionRange;
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

void testMissingRecoveredFontDefinitionFallback()
{
    using FontDefinition = musx::dom::others::FontDefinition;
    using FontOptions = musx::dom::options::FontOptions;
    using FontType = FontOptions::FontType;

    auto targetSession = musx::factory::DocumentFactory::begin();
    const auto targetDocument = targetSession.getDocument();
    auto targetOptions = std::make_shared<FontOptions>(targetDocument);
    targetDocument->getOptions()->add(FontOptions::XmlNodeName, targetOptions);
    const auto addTargetFont = [&](musx::dom::Cmper cmper, const char* name) {
        auto font = std::make_shared<FontDefinition>(targetDocument, musx::dom::SCORE_PARTID,
            musx::dom::EnigmaBase::ShareMode::All, cmper);
        font->name = name;
        targetDocument->getOthers()->add(FontDefinition::XmlNodeName, font);
    };
    addTargetFont(0, "Seville");
    addTargetFont(5, "Arial");
    const auto addMissingOption = [&](FontType type, int size, std::uint16_t effects) {
        auto font = std::make_shared<musx::dom::FontInfo>(targetDocument);
        font->fontId = 99;
        font->fontSize = size;
        font->setEnigmaStyles(effects);
        targetOptions->fontOptions.emplace(type, font);
    };
    addMissingOption(FontType::Fretboard, 36, 1);
    addMissingOption(FontType::Tablature, 12, 2);

    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto referenceDocument = referenceSession.getDocument();
    auto referenceOptions = std::make_shared<FontOptions>(referenceDocument);
    referenceDocument->getOptions()->add(FontOptions::XmlNodeName, referenceOptions);
    const auto addReference = [&](FontType type, musx::dom::Cmper cmper, const char* name) {
        auto definition = std::make_shared<FontDefinition>(referenceDocument,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
        definition->name = name;
        referenceDocument->getOthers()->add(FontDefinition::XmlNodeName, definition);
        auto font = std::make_shared<musx::dom::FontInfo>(referenceDocument);
        font->fontId = cmper;
        referenceOptions->fontOptions.emplace(type, font);
    };
    addReference(FontType::Fretboard, 3, "Seville");
    addReference(FontType::Tablature, 4, " arIAL ");

    ImportReport report;
    finale_mus_reader::options::repairMissingRecoveredFontDefinitions(
        targetDocument, referenceDocument, targetOptions, report);

    const auto fretboard = targetOptions->getFontInfo(FontType::Fretboard);
    expect(fretboard->fontId == 6 && fretboard->fontSize == 36
            && fretboard->getEnigmaStyles() == 1,
        "A same-type reference face was not cloned after the highest target comparator");
    expect(targetDocument->getOthers()->get<FontDefinition>(
            musx::dom::SCORE_PARTID, 6)->name == "Seville",
        "The cloned same-type reference face did not retain its reference spelling");
    const auto tablature = targetOptions->getFontInfo(FontType::Tablature);
    expect(tablature->fontId == 5 && tablature->fontSize == 12
            && tablature->getEnigmaStyles() == 2,
        "A normalized nonzero target face was not reused by the fallback");
    expect(targetDocument->getOthers()->getArray<FontDefinition>(
            musx::dom::SCORE_PARTID).size() == 3,
        "The fallback introduced a duplicate nonzero font name");
    expect(report.warnings.size() == 2,
        "Missing recovered font definitions were not reported");
}

// The target is taken by value rather than by const reference so that a literal call site
// does not create a temporary bound to a reference parameter. GCC cannot prove the returned
// reference does not alias such a temporary and rejects the binding under
// -Wdangling-reference.
const finale_mus_reader::FieldInfo& field(const ImportReport& report, std::string_view target)
{
    const auto found = std::find_if(report.fields.begin(), report.fields.end(),
        [&](const finale_mus_reader::FieldInfo& info) {
            return std::string_view(info.target) == target;
        });
    expect(found != report.fields.end(),
        std::string("Missing mapping report for ").append(target));
    return *found;
}

MappingTable makeTable(const char* prefix, const FieldMapping* fields, std::size_t count,
    VersionRange versions = {}, EpochMask epochs = EpochMask::FixedRow)
{
    return MappingTable{
        .reportPrefix = prefix,
        .epochs = epochs,
        .versions = versions,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &finale_mus_reader::enumerateOptionsTarget<Spacing>,
        .fields = fields,
        .fieldCount = count};
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
    finale_mus_reader::applyMappingTables(
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
        finale_mus_reader::applyMappingTables(
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
        finale_mus_reader::applyMappingTables(
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
    finale_mus_reader::applyMappingTables(
        tables, LegacyRecordIndex::build(parsed), profileFor(7), document, report);

    expect(spacing->minWidth == 111, "An absent record overwrote the seeded default");
    const auto info = field(report, "options.spacing.minWidth");
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
            finale_mus_reader::versions::from(12), minWidth),
    };
    const auto table = makeTable("options.spacing", fields, std::size(fields));
    const std::vector<const MappingTable*> tables{&table};
    const auto parsed = makeContainer({{GLOBALS_CMPER, "94", {0, 777, 0, 0, 0, 0}}});

    const auto readWithProfile = [&](const SourceProfile& profile) {
        Spacing* spacing = nullptr;
        auto document = makeDocument(&spacing);
        ImportReport report;
        finale_mus_reader::applyMappingTables(
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
            finale_mus_reader::versions::from(3, 5), minWidth),
    };
    const auto table = makeTable("options.spacing", fields, std::size(fields));
    const std::vector<const MappingTable*> tables{&table};
    const auto parsed = makeContainer({{GLOBALS_CMPER, "94", {0, 888, 0, 0, 0, 0}}});

    const auto readWith = [&](std::uint8_t major, std::uint8_t minor) {
        Spacing* spacing = nullptr;
        auto document = makeDocument(&spacing);
        ImportReport report;
        finale_mus_reader::applyMappingTables(
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
        std::size(overrideFields), finale_mus_reader::versions::from(12));
    const std::vector<const MappingTable*> tables{&base, &override};

    const auto parsed = makeContainer({{GLOBALS_CMPER, "94", {0, 11, 22, 33, 0, 0}}});

    const auto readWith = [&](std::uint8_t major) {
        Spacing* spacing = nullptr;
        auto document = makeDocument(&spacing);
        ImportReport report;
        finale_mus_reader::applyMappingTables(
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
    finale_mus_reader::applyMappingTables(
        tables, LegacyRecordIndex::build(parsed), profile, document, report);

    expect(spacing->minWidth == 111, "A table was applied outside its epoch");
    expect(field(report, "options.spacing.minWidth").origin == ValueOrigin::Finale27Default,
        "An uncovered epoch did not report its supported field as a default");
}

// Details carry a second comparator, which displaces the tag two bytes and leaves five
// payload words instead of six. The index normalizes both shapes into one row type.
void testDetailRowShape()
{
    finale_mus_reader::container::ParsedContainer parsed;
    parsed.formatEpoch = FormatEpoch::UncompressedLegacy;
    parsed.byteOrder = ByteOrder::BigEndian;

    finale_mus_reader::container::DecodedBlock block;
    block.info.type = 0x0002;
    const std::vector<std::array<std::int16_t, 8>> detailRows{
        // cmper1, cmper2, packed tag halves, then five payload words
        {7, 9, 'C', 'L', 11, 22, 33, 44},
        {7, 9, 'C', 'L', 55, 66, 77, 88},
        {7, 8, 'C', 'L', 99, 0, 0, 0},
    };
    for (const auto& row : detailRows) {
        const auto push16 = [&](std::uint16_t v) {
            block.data.push_back(static_cast<std::uint8_t>(v >> 8U));
            block.data.push_back(static_cast<std::uint8_t>(v));
        };
        push16(static_cast<std::uint16_t>(row[0]));
        push16(static_cast<std::uint16_t>(row[1]));
        block.data.push_back(static_cast<std::uint8_t>(row[2]));
        block.data.push_back(static_cast<std::uint8_t>(row[3]));
        for (int i = 4; i < 8; ++i) push16(static_cast<std::uint16_t>(row[i]));
        push16(0);
    }
    parsed.blocks.push_back(std::move(block));

    const auto index = LegacyRecordIndex::build(parsed);
    const auto tag = finale_mus_reader::records::packTag("CL");
    const auto family = index.getDetails().getArray(tag, 7, 9);
    expect(family.size() == 2, "Detail family did not group by both comparators");
    expect(family[0].inci == 0 && family[1].inci == 1,
        "Detail incidences were not assigned in encounter order");
    expect(family[0].wordCount == finale_mus_reader::records::detailWordCount,
        "Detail rows should carry five payload words");
    expect(family[0].words[0] == 11 && family[1].words[0] == 55,
        "Detail payload was read from the wrong offset");

    const auto other = index.getDetails().getArray(tag, 7, 8);
    expect(other.size() == 1 && other[0].words[0] == 99,
        "A different second comparator was not treated as a separate family");
    expect(index.getDetails().get(tag, 7, 9, 1) != nullptr
        && index.getDetails().get(tag, 7, 9, 2) == nullptr,
        "Detail incidence lookup did not bound correctly");
    expect(index.getDetails().getArray(tag, 1, 1).empty(),
        "An absent detail family returned rows");
    expect(index.getOthers().empty(), "A details block produced others rows");
}

// The others pool keeps working through the same normalized index, and the word stream is
// still addressed across incidences.
void testOtherRowsRemainSearchable()
{
    const auto parsed = makeContainer({
        {GLOBALS_CMPER, "94", {1, 2, 3, 4, 5, 6}},
        {GLOBALS_CMPER, "94", {7, 8, 9, 10, 11, 12}},
        {3, "LA", {-14, 0, 0, 0, 0, 0}},
    });
    const auto index = LegacyRecordIndex::build(parsed);
    const auto spacing = finale_mus_reader::records::packTag("94");
    expect(index.getOthers().getArray(spacing, GLOBALS_CMPER).size() == 2,
        "Others family did not group by comparator");
    expect(index.getOthers().cmpersForTag(finale_mus_reader::records::packTag("LA"))
        == std::vector<std::uint16_t>{3}, "cmpersForTag did not report the layer comparator");
    const auto straddle = index.word(spacing, GLOBALS_CMPER, 6);
    expect(straddle && straddle->value == 7,
        "Word addressing did not continue into the next incidence");
    expect(!index.word(spacing, GLOBALS_CMPER, 12),
        "Word addressing ran past the last incidence");
}

} // namespace

namespace finale_mus_reader_tests {

TEST_CASE("Detail row shape", "[mapping]") { testDetailRowShape(); }
TEST_CASE("Other rows remain searchable", "[mapping]") { testOtherRowsRemainSearchable(); }
TEST_CASE("Four-byte incidence straddling", "[mapping]")
{
    testFourByteStraddlesIncidence();
}
TEST_CASE("Long word order", "[mapping]") { testLongWordOrder(); }
TEST_CASE("Bit extraction", "[mapping]") { testBitExtraction(); }
TEST_CASE("Absent record keeps seeded default", "[mapping]")
{
    testAbsentRecordKeepsSeededDefault();
}
TEST_CASE("Missing recovered font definition fallback", "[mapping]")
{
    testMissingRecoveredFontDefinitionFallback();
}
TEST_CASE("Version gating", "[mapping]") { testVersionGating(); }
TEST_CASE("Minor version ordering", "[mapping]") { testMinorVersionOrdering(); }
TEST_CASE("Table layering", "[mapping]") { testTableLayering(); }
TEST_CASE("Uncovered epoch still reports", "[mapping]")
{
    testUncoveredEpochStillReports();
}

} // namespace finale_mus_reader_tests
