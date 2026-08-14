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
#include "import/support/embedded_graphics.h"
#include "import/details.h"
#include "import/support/legacy_mapping.h"
#include "import/others.h"
#include "import/options.h"
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

void expectMapping(bool condition, const std::string& message)
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
    const std::vector<SyntheticRow>& rows,
    FormatEpoch epoch = FormatEpoch::UncompressedLegacy)
{
    finale_mus_reader::container::ParsedContainer parsed;
    parsed.formatEpoch = epoch;
    parsed.byteOrder = ByteOrder::BigEndian;

    finale_mus_reader::container::DecodedBlock block;
    block.info.type = epoch == FormatEpoch::DclLegacy ? 0x000f : 0x0001;
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

/// @brief Builds a parsed container holding one class-identified record, the 2007+ framing.
finale_mus_reader::container::ParsedContainer makeClassContainer(
    std::uint16_t classId, const std::vector<std::int16_t>& words, ByteOrder byteOrder,
    std::uint16_t cmper = GLOBALS_CMPER)
{
    finale_mus_reader::container::ParsedContainer parsed;
    parsed.formatEpoch = FormatEpoch::ZlibLegacy;
    parsed.byteOrder = byteOrder;

    finale_mus_reader::container::DecodedBlock block;
    block.info.type = 0x001a;
    const auto push16 = [&](std::uint16_t value) {
        if (byteOrder == ByteOrder::BigEndian) {
            block.data.push_back(static_cast<std::uint8_t>(value >> 8U));
            block.data.push_back(static_cast<std::uint8_t>(value));
        } else {
            block.data.push_back(static_cast<std::uint8_t>(value));
            block.data.push_back(static_cast<std::uint8_t>(value >> 8U));
        }
    };
    push16(classId);
    push16(cmper);
    push16(0);
    const auto length = static_cast<std::uint32_t>(words.size() * 2);
    if (byteOrder == ByteOrder::BigEndian) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            block.data.push_back(static_cast<std::uint8_t>(length >> shift));
        }
    } else {
        for (int shift = 0; shift <= 24; shift += 8) {
            block.data.push_back(static_cast<std::uint8_t>(length >> shift));
        }
    }
    for (const auto word : words) {
        push16(static_cast<std::uint16_t>(word));
    }
    block.data.insert(block.data.end(), 4, 0);
    block.info.decodedSize = block.data.size();
    parsed.blocks.push_back(std::move(block));
    return parsed;
}

finale_mus_reader::container::ParsedContainer makeDetailContainer(
    FormatEpoch epoch, std::uint16_t staffId, std::uint16_t meas,
    const std::vector<std::int16_t>& words)
{
    finale_mus_reader::container::ParsedContainer parsed;
    parsed.formatEpoch = epoch;
    parsed.byteOrder = ByteOrder::BigEndian;
    finale_mus_reader::container::DecodedBlock block;
    block.info.type = epoch == FormatEpoch::DclLegacy ? 0x0010 : 0x0002;
    for (std::size_t at = 0; at < words.size(); at += finale_mus_reader::records::detailWordCount) {
        const auto push16 = [&](std::uint16_t value) {
            block.data.push_back(static_cast<std::uint8_t>(value >> 8U));
            block.data.push_back(static_cast<std::uint8_t>(value));
        };
        push16(staffId);
        push16(meas);
        push16(finale_mus_reader::records::packTag("mg"));
        for (std::size_t slot = 0; slot < finale_mus_reader::records::detailWordCount; ++slot)
            push16(static_cast<std::uint16_t>(words[at + slot]));
    }
    block.info.decodedSize = block.data.size();
    parsed.blocks.push_back(std::move(block));
    return parsed;
}

finale_mus_reader::container::ParsedContainer makeDetailClassContainer(
    std::uint16_t staffId, std::uint16_t meas, std::uint16_t inci,
    const std::vector<std::int16_t>& words, ByteOrder byteOrder)
{
    auto parsed = makeClassContainer(0x041d, {}, byteOrder, staffId);
    auto& block = parsed.blocks.front();
    block.info.type = 0x001b;
    block.data.clear();
    const auto push16 = [&](std::uint16_t value) {
        if (byteOrder == ByteOrder::BigEndian) {
            block.data.push_back(static_cast<std::uint8_t>(value >> 8U));
            block.data.push_back(static_cast<std::uint8_t>(value));
        } else {
            block.data.push_back(static_cast<std::uint8_t>(value));
            block.data.push_back(static_cast<std::uint8_t>(value >> 8U));
        }
    };
    push16(0x041d);
    push16(staffId);
    push16(meas);
    push16(inci);
    const auto length = static_cast<std::uint32_t>(words.size() * 2);
    if (byteOrder == ByteOrder::BigEndian) {
        push16(static_cast<std::uint16_t>(length));
    } else {
        for (int shift = 0; shift <= 24; shift += 8)
            block.data.push_back(static_cast<std::uint8_t>(length >> shift));
    }
    for (const auto word : words) push16(static_cast<std::uint16_t>(word));
    block.data.insert(block.data.end(), 4, 0);
    block.info.decodedSize = block.data.size();
    return parsed;
}

/// @brief A reference document whose ClefOptions has the modern collection and no fonts.
musx::dom::DocumentPtr makeClefReferenceDocument()
{
    using ClefOptions = musx::dom::options::ClefOptions;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto options = std::make_shared<ClefOptions>(document);
    options->clefFrontSepar = 24;
    options->cautionaryClefChanges = true;
    for (int index = 0; index < 18; ++index) {
        auto def = std::make_shared<ClefOptions::ClefDef>(options);
        def->middleCPos = -1;
        def->clefChar = static_cast<char32_t>(900 + index);
        options->clefDefs.push_back(std::move(def));
    }
    document->getOptions()->add(ClefOptions::XmlNodeName, options);
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
    // The reference carries a size and effects distinct from the source's, so the
    // assertions below can tell which document each part of the tuple came from.
    const auto addReference = [&](FontType type, musx::dom::Cmper cmper, const char* name,
                                  int size, std::uint16_t effects) {
        auto definition = std::make_shared<FontDefinition>(referenceDocument,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
        definition->name = name;
        referenceDocument->getOthers()->add(FontDefinition::XmlNodeName, definition);
        auto font = std::make_shared<musx::dom::FontInfo>(referenceDocument);
        font->fontId = cmper;
        font->fontSize = size;
        font->setEnigmaStyles(effects);
        referenceOptions->fontOptions.emplace(type, font);
    };
    addReference(FontType::Fretboard, 3, "Seville", 24, 4);
    addReference(FontType::Tablature, 4, " arIAL ", 18, 5);

    ImportReport report;
    finale_mus_reader::options::repairMissingRecoveredFontDefinitions(
        targetDocument, referenceDocument, targetOptions, report);

    // The whole tuple comes from the reference, not just the face. A point size is not
    // independent of the face it was chosen for, so pairing a substituted face with the
    // source's size would produce a combination present in neither document. The source
    // values here are 36/1 and 12/2; both must be gone.
    const auto fretboard = targetOptions->getFontInfo(FontType::Fretboard);
    expectMapping(fretboard->fontId == 6 && fretboard->fontSize == 24
            && fretboard->getEnigmaStyles() == 4,
        "A same-type reference face was not cloned after the highest target comparator");
    expectMapping(targetDocument->getOthers()->get<FontDefinition>(
            musx::dom::SCORE_PARTID, 6)->name == "Seville",
        "The cloned same-type reference face did not retain its reference spelling");
    const auto tablature = targetOptions->getFontInfo(FontType::Tablature);
    expectMapping(tablature->fontId == 5 && tablature->fontSize == 18
            && tablature->getEnigmaStyles() == 5,
        "A normalized nonzero target face was not reused by the fallback");
    expectMapping(targetDocument->getOthers()->getArray<FontDefinition>(
            musx::dom::SCORE_PARTID).size() == 3,
        "The fallback introduced a duplicate nonzero font name");
    // The fallback is silent by design: it is a considered substitution that leaves the
    // document usable, and a warning would surface it in user interfaces as though
    // something had gone wrong. Callers distinguish substituted values from recovered ones
    // through the reported ValueOrigin, not through a message.
    expectMapping(std::none_of(report.diagnostics.begin(), report.diagnostics.end(),
                      [](const finale_mus_reader::Diagnostic& entry) {
                          return entry.level == musx::util::Logger::LogLevel::Warning;
                      }),
        "The designed-in font substitution emitted a user-facing warning");
    expectMapping(report.diagnostics.size() == 2,
        "The font substitution was not recorded at verbose level");
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
    expectMapping(found != report.fields.end(),
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

    expectMapping(spacing->referenceDuration == 0x12345678,
        "Four-byte value straddling an incidence boundary was not assembled");
    expectMapping(field(report, "options.spacing.referenceDuration").origin == ValueOrigin::LegacyMus,
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

    expectMapping(readWith(highFirst) == 0x01020304, "High-first four-byte order is wrong");
    expectMapping(readWith(lowFirst) == 0x03040102, "Low-first four-byte order is wrong");
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

    expectMapping(readFlag(0x0008), "Bit 3 set was not extracted as true");
    expectMapping(!readFlag(0x0004), "An unrelated bit was reported as the mapped bit");
    expectMapping(readFlag(static_cast<std::int16_t>(0xfff8)), "Bit 3 was lost among other bits");
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

    expectMapping(spacing->minWidth == 111, "An absent record overwrote the seeded default");
    const auto info = field(report, "options.spacing.minWidth");
    expectMapping(info.origin == ValueOrigin::Finale27Default,
        "An absent record was not reported as a synthesized default");
    expectMapping(info.rawValue == 111, "The reported default did not carry the seeded value");
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

    expectMapping(readWithProfile(profileFor(7)) == 111,
        "A row gated to a later version was applied to an earlier file");
    expectMapping(readWithProfile(profileFor(12)) == 777,
        "A row gated to this version was not applied");

    SourceProfile unknownVersion;
    unknownVersion.epoch = FormatEpoch::UncompressedLegacy;
    expectMapping(readWithProfile(unknownVersion) == 111,
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

    expectMapping(readWith(3, 2) == 111, "Version 3.2 matched a gate starting at 3.5");
    expectMapping(readWith(3, 7) == 888, "Version 3.7 did not match a gate starting at 3.5");
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
    expectMapping(early.first == 11, "The base location was not used below the override version");
    expectMapping(early.second == 22, "The base table lost an unrelated field");

    const auto late = readWith(12);
    expectMapping(late.first == 33, "The override location did not supersede the base location");
    expectMapping(late.second == 22, "The override table dropped a field it does not restate");
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

    expectMapping(spacing->minWidth == 111, "A table was applied outside its epoch");
    expectMapping(field(report, "options.spacing.minWidth").origin == ValueOrigin::Finale27Default,
        "An uncovered epoch did not report its supported field as a default");
}

// Details carry a second comparator, which displaces the tag two bytes and leaves five
// payload words instead of six. The index normalizes both shapes into one row type.
// The two clef-tuple rules that no tracked fixture exercises: a sign-extended clef
// character, which the corpus has but the published fixtures do not, and the payload size
// that both tuple widths divide evenly.
void testClefTupleDecoding()
{
    using ClefOptions = musx::dom::options::ClefOptions;
    const auto captured = [](const finale_mus_reader::container::ParsedContainer& parsed,
                              const SourceProfile& profile) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        ImportReport report;
        finale_mus_reader::PendingReferences pending;
        finale_mus_reader::options::captureClefOptions(LegacyRecordIndex::build(parsed),
            profile, document, makeClefReferenceDocument(), report, pending);
        return document->getOptions()->get<ClefOptions>();
    };

    // One nine-word tuple whose character is the sign-extended spelling of 139, and whose
    // flags claim an own font. Padded out to a whole sixteen-definition table.
    std::vector<SyntheticRow> rows;
    std::vector<std::int16_t> stream{-10, static_cast<std::int16_t>(0xff8b), -6, 7, 0,
        11, 24, 1, 0x0002};
    stream.resize(16 * 9, 0);
    for (std::size_t first = 0; first < stream.size(); first += 6) {
        SyntheticRow row;
        row.cmper = GLOBALS_CMPER;
        row.tag = "95";
        for (std::size_t slot = 0; slot < 6; ++slot) {
            row.words[slot] = stream[first + slot];
        }
        rows.push_back(row);
    }
    const auto narrow = captured(makeContainer(rows), profileFor(7));
    expectMapping(narrow && narrow->clefDefs.size() == 18,
        "The synthetic sixteen-clef table was not completed from the reference");
    const auto first = narrow->getClefDef(0);
    expectMapping(first->clefChar == 139,
        "A sign-extended clef character was not narrowed to its stored byte");
    expectMapping(first->useOwnFont && first->font && first->font->fontId == 11
            && first->font->fontSize == 24 && first->font->bold,
        "The own-font flag bit did not bring across the tuple's font triple");
    expectMapping(!first->isShape && !first->scaleToStaffHeight,
        "Unset clef flag bits were treated as set");
    expectMapping(first->baselineAdjust == 7,
        "The 2001-and-later baseline word was not assigned as Efix");
    expectMapping(narrow->clefFrontSepar == 24,
        "The reference scalars were not copied onto the rebuilt clef options");

    // A DCL file with no clef table must not fall back on the pre-2001 selectors. Those
    // selectors still exist in that era and hold unrelated option words, so reading them
    // would fabricate eight clef definitions and report them as recovered from the file.
    {
        std::vector<SyntheticRow> dclRows;
        std::array<std::array<char, 3>, 8> dclTags{};
        // The values a real Finale 2002 file holds at selectors 28 through 35.
        const std::array<std::array<std::int16_t, 6>, 8> notClefs{{{0, 0, 0, 0, 0, 24},
            {0, 0, 0, 0, 0, 1}, {0, 0, 0, 0, 0, 4}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 128}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}}};
        for (int selector = 28; selector <= 35; ++selector) {
            auto& tag = dclTags[static_cast<std::size_t>(selector - 28)];
            tag = {static_cast<char>('0' + selector / 10),
                static_cast<char>('0' + selector % 10), '\0'};
            SyntheticRow row;
            row.cmper = GLOBALS_CMPER;
            row.tag = tag.data();
            row.words = notClefs[static_cast<std::size_t>(selector - 28)];
            dclRows.push_back(row);
        }
        auto profile = profileFor(7);
        profile.epoch = FormatEpoch::DclLegacy;
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        ImportReport report;
        finale_mus_reader::PendingReferences pending;
        finale_mus_reader::options::captureClefOptions(
            LegacyRecordIndex::build(makeContainer(dclRows)), profile, document,
            makeClefReferenceDocument(), report, pending);
        const auto recovered = std::count_if(report.fields.begin(), report.fields.end(),
            [](const finale_mus_reader::FieldInfo& f) {
                return f.origin == ValueOrigin::LegacyMus
                    && f.target.find("clefDefs") != std::string::npos;
            });
        expectMapping(recovered == 0,
            "A DCL file with no clef table read the pre-2001 selectors as clefs");
        expectMapping(document->getOptions()->get<ClefOptions>()->clefDefs.size() == 18,
            "A DCL file with no clef table was not completed from the reference");
    }

    // An out-of-range default clef index is warned about and left alone. Clamping it would
    // turn a damaged file into a plausible document and hide the fact worth knowing.
    {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto options = std::make_shared<ClefOptions>(document);
        options->defaultClef = 99;
        for (int i = 0; i < 18; ++i) {
            options->clefDefs.push_back(std::make_shared<ClefOptions::ClefDef>(options));
        }
        document->getOptions()->add(ClefOptions::XmlNodeName, options);
        ImportReport report;
        finale_mus_reader::options::validateClefOptions(document, report);
        expectMapping(options->defaultClef == 99,
            "An out-of-range default clef index was silently corrected");
        expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                          [](const finale_mus_reader::Diagnostic& entry) {
                              return entry.message.find("default clef index 99") != std::string::npos;
                          }),
            "An out-of-range default clef index was accepted without a warning");
        ImportReport clean;
        options->defaultClef = 17;
        finale_mus_reader::options::validateClefOptions(document, clean);
        expectMapping(clean.diagnostics.empty(), "A valid default clef index warned");
    }

    // Pre-2001 clefs are eight separate globals whose baseline word is in harmonic levels,
    // so it is scaled into Efix on the way in while the report keeps the stored number.
    std::vector<SyntheticRow> earlyRows;
    // Fixed storage: SyntheticRow keeps a pointer, so the tag text must outlive the vector
    // and must not move when rows are appended.
    std::array<std::array<char, 3>, 8> earlyTags{};
    for (int selector = 28; selector <= 35; ++selector) {
        auto& tag = earlyTags[static_cast<std::size_t>(selector - 28)];
        tag = {static_cast<char>('0' + selector / 10),
            static_cast<char>('0' + selector % 10), '\0'};
        SyntheticRow row;
        row.cmper = GLOBALS_CMPER;
        row.tag = tag.data();
        // Slot 4 is the baseline adjustment in this era; slot 1 holds something else.
        row.words = {static_cast<std::int16_t>(-selector), 7,
            static_cast<std::int16_t>(0xff8b), -6, 2, 0};
        earlyRows.push_back(row);
    }
    const auto early = captured(makeContainer(earlyRows), profileFor(5));
    expectMapping(early && early->clefDefs.size() == 18,
        "The eight pre-2001 clef globals were not completed from the reference");
    expectMapping(early->getClefDef(0)->middleCPos == -28
            && early->getClefDef(0)->clefChar == 139
            && early->getClefDef(0)->staffPosition == -6,
        "The pre-2001 per-clef record slots were misread");
    expectMapping(early->getClefDef(0)->baselineAdjust == 2 * 768,
        "The pre-2001 harmonic-level baseline was not converted to Efix");
    expectMapping(early->getClefDef(7)->middleCPos == -35
            && early->getClefDef(8)->middleCPos == -1,
        "The boundary between eight stored and ten synthesized clefs moved");

    // 360 bytes divides by both tuple widths. Only the version separates them, and only
    // the wide reading yields the eighteen definitions Finale actually stores.
    std::vector<std::int16_t> wide(18 * 10, 0);
    wide[0] = -10;
    wide[1] = 38;   // low half of the long character
    wide[2] = 0;    // high half
    wide[3] = -6;
    wide[9] = 0x0005;
    const auto unicodeEra = captured(
        makeClassContainer(0x006d, wide, ByteOrder::LittleEndian), [] {
            auto profile = profileFor(17);
            profile.epoch = FormatEpoch::ZlibLegacy;
            profile.byteOrder = ByteOrder::LittleEndian;
            return profile;
        }());
    expectMapping(unicodeEra && unicodeEra->clefDefs.size() == 18,
        "A 360-byte payload was not read as eighteen wide clef definitions");
    expectMapping(unicodeEra->getClefDef(0)->clefChar == 38
            && unicodeEra->getClefDef(0)->staffPosition == -6,
        "The wide tuple's long character or shifted slots were misread");
    expectMapping(unicodeEra->getClefDef(0)->isShape
            && unicodeEra->getClefDef(0)->scaleToStaffHeight,
        "The wide tuple's flags word was not read at its shifted slot");

    // A whole class-record field is signed, like the fixed-row word that carries the same
    // logical option. Read unsigned, this Evpu of -12 arrives as 65524.
    {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto profile = profileFor(13);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::BigEndian;
        // Class 0x1b is globals selector 13; word 2 is the percent and word 3 the offset.
        auto options = std::make_shared<ClefOptions>(document);
        document->getOptions()->add(ClefOptions::XmlNodeName, options);
        ImportReport report;
        finale_mus_reader::applyMappingTables(
            {&finale_mus_reader::options::classClefOptionsTable()},
            LegacyRecordIndex::build(makeClassContainer(
                0x001b, {4, 1024, 75, -12, 0, 1}, ByteOrder::BigEndian)),
            profile, document, report);
        expectMapping(options->clefChangeOffset == -12,
            "A negative class-record word was read as an unsigned value");
        expectMapping(options->clefChangePercent == 75,
            "A positive class-record word was not read");
    }

    // A big-endian file using the Finale 2012 layout announces itself, because no surveyed
    // file is one and the long character's word order has never been checked against a
    // specimen. The values still decode as the symmetric counterpart of the verified case.
    {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto profile = profileFor(17);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::BigEndian;
        ImportReport report;
        finale_mus_reader::PendingReferences pending;
        finale_mus_reader::options::captureClefOptions(
            LegacyRecordIndex::build(makeClassContainer(0x006d, wide, ByteOrder::BigEndian)),
            profile, document, makeClefReferenceDocument(), report, pending);
        expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                          [](const finale_mus_reader::Diagnostic& entry) {
                              return entry.message.find("unverified") != std::string::npos;
                          }),
            "A big-endian Finale 2012 clef layout was decoded without saying it is unverified");
        expectMapping(document->getOptions()->get<ClefOptions>()->getClefDef(0)->clefChar == 38,
            "The big-endian wide tuple did not decode symmetrically");
    }

    // The same payload from a pre-Unicode file is the narrow tuple, and twenty definitions
    // is more than Finale stores, so the reader must say so rather than pass it off.
    ImportReport report;
    {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto profile = profileFor(13);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::LittleEndian;
        finale_mus_reader::PendingReferences pending;
        finale_mus_reader::options::captureClefOptions(
            LegacyRecordIndex::build(makeClassContainer(0x006d, wide, ByteOrder::LittleEndian)),
            profile, document, makeClefReferenceDocument(), report, pending);
        expectMapping(document->getOptions()->get<ClefOptions>()->clefDefs.size() == 20,
            "The pre-Unicode reading of an ambiguous payload did not use the narrow tuple");
    }
    expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                      [](const finale_mus_reader::Diagnostic& entry) {
                          return entry.message.find("more than the 18") != std::string::npos;
                      }),
        "An over-long clef collection was accepted without a warning");
}

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
    expectMapping(family.size() == 2, "Detail family did not group by both comparators");
    expectMapping(family[0].inci == 0 && family[1].inci == 1,
        "Detail incidences were not assigned in encounter order");
    expectMapping(family[0].wordCount == finale_mus_reader::records::detailWordCount,
        "Detail rows should carry five payload words");
    expectMapping(family[0].words[0] == 11 && family[1].words[0] == 55,
        "Detail payload was read from the wrong offset");

    const auto other = index.getDetails().getArray(tag, 7, 8);
    expectMapping(other.size() == 1 && other[0].words[0] == 99,
        "A different second comparator was not treated as a separate family");
    expectMapping(index.getDetails().get(tag, 7, 9, 1) != nullptr
        && index.getDetails().get(tag, 7, 9, 2) == nullptr,
        "Detail incidence lookup did not bound correctly");
    expectMapping(index.getDetails().getArray(tag, 1, 1).empty(),
        "An absent detail family returned rows");
    expectMapping(index.getOthers().empty(), "A details block produced others rows");
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
    expectMapping(index.getOthers().getArray(spacing, GLOBALS_CMPER).size() == 2,
        "Others family did not group by comparator");
    expectMapping(index.getOthers().cmpersForTag(finale_mus_reader::records::packTag("LA"))
        == std::vector<std::uint16_t>{3}, "cmpersForTag did not report the layer comparator");
    const auto straddle = index.word(spacing, GLOBALS_CMPER, 6);
    expectMapping(straddle && straddle->value == 7,
        "Word addressing did not continue into the next incidence");
    expectMapping(!index.word(spacing, GLOBALS_CMPER, 12),
        "Word addressing ran past the last incidence");
}

/// @brief A document whose StemOptions already carries connections from somewhere else.
/// @details Seeded deliberately, because the capture pass must drop them: stem connections
/// belong to the document that stated them and name that document's fonts.
musx::dom::DocumentPtr makeStemDocument()
{
    using StemOptions = musx::dom::options::StemOptions;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto options = std::make_shared<StemOptions>(document);
    options->stemLength = 84;
    for (char32_t symbol : {U'΄', U'΅', U'Ά'}) {
        auto connection = std::make_shared<StemOptions::StemConnection>();
        connection->symbol = symbol;
        connection->fontId = 99;
        options->stemConnections.push_back(std::move(connection));
    }
    document->getOptions()->add(StemOptions::XmlNodeName, options);
    return std::move(session).finish();
}

std::shared_ptr<musx::dom::options::StemOptions> captureStems(
    const finale_mus_reader::container::ParsedContainer& parsed, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document, ImportReport& report)
{
    finale_mus_reader::options::captureStemOptions(
        LegacyRecordIndex::build(parsed), profile, document, report);
    return std::const_pointer_cast<musx::dom::options::StemOptions>(
        document->getOptions()->get<musx::dom::options::StemOptions>());
}

// Finale 3.5 changed every unit in the stem family at once, and the reader decides that
// boundary from the size of the connection collection rather than from the version. The
// versions below therefore include one outside the era entirely: whatever a file claims to
// be, the collection it carries is what dates it. No tracked fixture can exercise this --
// the corpus has no publishable Finale 3.0 through 3.4 document -- so both shapes are built
// here.
void testStemPreFinale35Units()
{
    const auto tableWith = [](std::size_t slots) {
        std::vector<SyntheticRow> rows{
            {GLOBALS_CMPER, "20", {0, 0, 0, 0, 7, 5}},   // stem length and shortened length
            {GLOBALS_CMPER, "21", {0, 0, 18, 0, 0, 0}},  // reverse stem adjustment
        };
        // The connection family, whose size is the era marker. Only the first carries data.
        rows.push_back({GLOBALS_CMPER, "40", {0, 192, 12, -12, 0, 0}});
        for (std::size_t i = 1; i < slots; ++i) {
            rows.push_back({GLOBALS_CMPER, "40", {0, 0, 0, 0, 0, 0}});
        }
        return makeContainer(rows);
    };
    const auto runImport = [](const finale_mus_reader::container::ParsedContainer& parsed,
                               const SourceProfile& profile, ImportReport& report) {
        const auto document = makeStemDocument();
        const auto reference = makeStemDocument();
        const auto index = LegacyRecordIndex::build(parsed);
        finale_mus_reader::PendingReferences pending;
        const finale_mus_reader::ImportContext context{
            index, profile, document, reference, report, pending};
        finale_mus_reader::options::importStemOptions(context);
        return document->getOptions()->get<musx::dom::options::StemOptions>();
    };

    constexpr std::size_t earlySlots = 32;
    constexpr std::size_t modernSlots = 128;
    // Staff positions become Evpu and Evpu becomes Efix, whatever the version says. The third
    // profile is a version from well outside the era, which the marker must override.
    auto coda = profileFor(0, 0);
    coda.epoch = FormatEpoch::CodaBanner;
    coda.version.reset();
    for (const auto& profile : {profileFor(3, 0), profileFor(3, 2), profileFor(15, 0), coda}) {
        ImportReport report;
        const auto options = runImport(tableWith(earlySlots), profile, report);
        expectMapping(options->stemLength == 7 * 12 && options->shortStemLength == 5 * 12
                && options->revStemAdj == 18 * 12,
            "A pre-Finale-3.5 stem length was not converted from staff positions");
        expectMapping(options->stemConnections.size() == 1
                && options->stemConnections[0]->upStemVert == 12 * 64,
            "A pre-Finale-3.5 stem adjustment was not converted from Evpu");
        // The half-stem length is not stored in that era, so it keeps the seeded default.
        expectMapping(field(report, "options.stemOptions.halfStemLength").origin
                == ValueOrigin::Finale27Default,
            "A pre-Finale-3.5 document claimed a half-stem length it does not store");
    }
    // The same words in the later shape are already in the modern units, and the version that
    // would have said otherwise is ignored.
    for (const auto& profile : {profileFor(3, 0), profileFor(5, 0), profileFor(15, 0)}) {
        ImportReport report;
        const auto options = runImport(tableWith(modernSlots), profile, report);
        expectMapping(options->stemLength == 7 && options->shortStemLength == 5
                && options->revStemAdj == 18,
            "A Finale 3.5 stem length was scaled as though it were staff positions");
        expectMapping(options->stemConnections[0]->upStemVert == 12,
            "A Finale 3.5 stem adjustment was scaled as though it were Evpu");
    }
}

// Nothing about the collection may come from the pinned baseline, and a source that states
// no connections must produce none rather than inheriting three.
void testStemConnectionsAreSourceOwned()
{
    ImportReport report;
    const auto document = makeStemDocument();
    const auto options = captureStems(
        makeContainer({{GLOBALS_CMPER, "94", {1, 2, 3, 4, 5, 6}}}),
        profileFor(5, 0), document, report);
    expectMapping(options->stemConnections.empty(),
        "A document with no stem-connection record kept the seeded connections");
    expectMapping(options->stemLength == 84,
        "Clearing the collection disturbed the seeded scalars");
}

// A Finale 2012 record frequently holds a stale copy of the pre-Unicode default table, which
// through the widened element reads as one implausible symbol. Recovering nothing is what
// Finale sees; recovering the old table would assert a layout the era does not use.
void testStemStaleUnicodeRecord()
{
    using finale_mus_reader::numericGlobalClass;
    // The bytes an untouched Finale 2012 document carries: the pre-Unicode table, whose
    // first two words the widened element reads as the single symbol 0x030000c0.
    const auto parsed = makeClassContainer(numericGlobalClass(40),
        {0, 192, 768, -768, 0, 0, 0, 131, -2304, 2304, -1024, 1024, 0, 0},
        ByteOrder::LittleEndian);
    SourceProfile profile;
    profile.epoch = FormatEpoch::ZlibLegacy;
    profile.byteOrder = ByteOrder::LittleEndian;
    SourceVersion version;
    version.major = 17;
    profile.version = version;

    ImportReport report;
    const auto document = makeStemDocument();
    const auto options = captureStems(parsed, profile, document, report);
    expectMapping(options->stemConnections.empty(),
        "A stale pre-Unicode stem record was read as though it were the widened layout");
    expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                      [](const auto& entry) {
                          return entry.message.find("not a Unicode codepoint")
                              != std::string::npos;
                      }),
        "The stale stem-connection record produced no diagnostic");
    expectMapping(std::any_of(report.fields.begin(), report.fields.end(),
                      [](const finale_mus_reader::FieldInfo& value) {
                          return value.target == "options.stemOptions.stemConnections[0].symbol"
                              && value.rawValue == 0x030000c0;
                      }),
        "The out-of-range symbol was not reported as raw evidence");

    // The same words in a pre-Unicode file are the table they always were.
    SourceProfile earlier = profile;
    SourceVersion narrow;
    narrow.major = 13;
    earlier.version = narrow;
    ImportReport narrowReport;
    const auto narrowDocument = makeStemDocument();
    const auto narrowOptions = captureStems(parsed, earlier, narrowDocument, narrowReport);
    expectMapping(narrowOptions->stemConnections.size() == 2
            && narrowOptions->stemConnections[0]->symbol == 192
            && narrowOptions->stemConnections[1]->symbol == 131,
        "The twelve-byte zlib element stopped decoding before Finale 2012");
}

// A connection names its font by comparator. A dangling one is preserved and reported rather
// than replaced, because a default would invent a typeface the source never named.
void testStemFontReferenceValidation()
{
    ImportReport report;
    const auto document = makeStemDocument();
    const auto options = captureStems(
        makeContainer({{GLOBALS_CMPER, "40", {7, 192, 768, -768, 0, 0}},
            {GLOBALS_CMPER, "40", {0, 0, 0, 0, 0, 0}}}),
        profileFor(5, 0), document, report);
    finale_mus_reader::options::validateStemOptions(document, report);
    expectMapping(options->stemConnections.size() == 1
            && options->stemConnections[0]->fontId == 7,
        "A stem connection did not keep the font comparator the source stated");
    expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                      [](const auto& entry) {
                          return entry.message.find("does not define") != std::string::npos;
                      }),
        "A dangling stem-connection font reference was not reported");
}

void testGraphicAssignmentsAcrossEpochs()
{
    using PageGraphicAssign = musx::dom::others::PageGraphicAssign;
    const std::vector<std::int16_t> tuple{
        0x100, 120, -48, 640, 320, 7, 0, 1, 0x014c,
        4, 4, 1, 1280, 640, 144, -24, 0x0192, 2};
    const std::vector<SyntheticRow> fixedRows{
        {4, "pg", {tuple[0], tuple[1], tuple[2], tuple[3], tuple[4], tuple[5]}},
        {4, "pg", {tuple[6], tuple[7], tuple[8], tuple[9], tuple[10], tuple[11]}},
        {4, "pg", {tuple[12], tuple[13], tuple[14], tuple[15], tuple[16], tuple[17]}}};

    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        const auto parsed = epoch == FormatEpoch::ZlibLegacy
            ? makeClassContainer(0x00bc, tuple, ByteOrder::BigEndian, 4)
            : makeContainer(fixedRows, epoch);
        const auto index = LegacyRecordIndex::build(parsed);
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto referenceSession = musx::factory::DocumentFactory::begin();
        const auto reference = std::move(referenceSession).finish();
        ImportReport report;
        finale_mus_reader::PendingReferences pending;
        SourceProfile profile;
        profile.epoch = epoch;
        profile.byteOrder = parsed.byteOrder;
        const finale_mus_reader::ImportContext context{
            index, profile, document, reference, report, pending};
        finale_mus_reader::others::importPageGraphicAssignments(context);
        const auto assignment = document->getOthers()
            ->get<PageGraphicAssign>(musx::dom::SCORE_PARTID, 4, musx::dom::Inci(0));
        expectMapping(assignment && assignment->version == 0x100
                && assignment->left == 120 && assignment->bottom == -48
                && assignment->width == 640 && assignment->height == 320
                && assignment->fDescId == 7 && !assignment->hidden
                && assignment->displayType == PageGraphicAssign::PageAssignType::One
                && assignment->hAlign == PageGraphicAssign::HorizontalAlignment::Center
                && assignment->vAlign == PageGraphicAssign::VerticalAlignment::Top
                && assignment->posFrom == PageGraphicAssign::PositionFrom::Margins
                && assignment->fixedPerc && assignment->startPage == 4
                && assignment->endPage == 4 && assignment->savedRecord
                && assignment->origWidth == 1280 && assignment->origHeight == 640
                && assignment->rightPgLeft == 144 && assignment->rightPgBottom == -24
                && assignment->rightPgHAlign == PageGraphicAssign::HorizontalAlignment::Right
                && assignment->rightPgVAlign == PageGraphicAssign::VerticalAlignment::Bottom
                && assignment->rightPgPosFrom == PageGraphicAssign::PositionFrom::PageEdge
                && assignment->rightPgFixedPerc && assignment->graphicCmper == 2,
            "A PageGraphicAssign field failed in epoch "
                + std::to_string(static_cast<int>(epoch)));
        expectMapping(report.fields.size() == 18,
            "A PageGraphicAssign did not report all source words");
    }
}

void testEmbeddedGraphicFraming()
{
    finale_mus_reader::container::ParsedContainer parsed;
    parsed.formatEpoch = FormatEpoch::ZlibLegacy;
    parsed.byteOrder = ByteOrder::LittleEndian;
    finale_mus_reader::container::DecodedBlock block;
    block.info.type = 0x0013;
    block.info.stored = true;
    const auto appendItem = [&](std::span<const std::uint8_t> payload) {
        block.data.push_back(9);
        block.data.push_back(0);
        const auto size = static_cast<std::uint32_t>(payload.size());
        for (int shift = 0; shift <= 24; shift += 8) {
            block.data.push_back(static_cast<std::uint8_t>(size >> shift));
        }
        block.data.insert(block.data.end(), payload.begin(), payload.end());
        block.data.insert(block.data.end(), {1, 0, 0, 0, 0});
    };
    const std::array<std::uint8_t, 8> png{0x89, 'P', 'N', 'G', 13, 10, 0x1a, 10};
    const std::array<std::uint8_t, 14> eps{'%', '!', 'P', 'S', '-', 'A', 'd', 'o', 'b', 'e',
        '-', '3', '.', '0'};
    const std::array<std::uint8_t, 4> unknown{'N', 'O', 'P', 'E'};
    appendItem(png);
    appendItem(eps);
    appendItem(unknown);
    parsed.blocks.push_back(std::move(block));
    ImportReport report;
    const auto graphics = finale_mus_reader::recoverEmbeddedGraphics(parsed, report);
    expectMapping(graphics.size() == 2 && graphics.at(1).extension == "png"
            && graphics.at(1).bytes == std::vector<std::uint8_t>(png.begin(), png.end())
            && graphics.at(2).extension == "eps"
            && graphics.at(2).bytes == std::vector<std::uint8_t>(eps.begin(), eps.end())
            && std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                [](const auto& diagnostic) {
                    return diagnostic.level == musx::util::Logger::LogLevel::Info
                        && diagnostic.message.find(
                               "Embedded graphic 3 has an unrecognized file signature")
                            != std::string::npos;
                }),
        "The stored graphics block did not map encounter order to embedded cmper ids");

    auto unsupportedFooter = parsed;
    unsupportedFooter.blocks.front().data[6 + png.size()] = 2;
    ImportReport unsupportedFooterReport;
    const auto withoutFirst = finale_mus_reader::recoverEmbeddedGraphics(
        unsupportedFooter, unsupportedFooterReport);
    expectMapping(withoutFirst.size() == 1
            && std::any_of(unsupportedFooterReport.diagnostics.begin(),
                unsupportedFooterReport.diagnostics.end(), [](const auto& diagnostic) {
                    return diagnostic.level == musx::util::Logger::LogLevel::Info
                        && diagnostic.message.find(
                               "Embedded graphic 1 has an unsupported footer version")
                            != std::string::npos;
                }),
        "An unsupported graphic footer did not report its embedded graphic comparator");

    parsed.blocks.front().data.pop_back();
    ImportReport truncatedReport;
    const auto truncated = finale_mus_reader::recoverEmbeddedGraphics(parsed, truncatedReport);
    expectMapping(truncated.size() == 2
            && std::any_of(truncatedReport.diagnostics.begin(), truncatedReport.diagnostics.end(),
                [](const auto& diagnostic) {
                    return diagnostic.level == musx::util::Logger::LogLevel::Warning
                        && diagnostic.message.find("truncated") != std::string::npos;
                }),
        "A truncated embedded-graphic footer was not bounded and reported");
}

void testShapeGraphicAssignmentsAcrossEpochs()
{
    using ShapeGraphicAssign = musx::dom::others::ShapeGraphicAssign;
    const std::vector<std::int16_t> tuple{
        0x100, 800, -520, 64, 20, 1, 0, 1, 0x0189,
        0, 0, 1, 64, 20, 0, 0, 0, 3};
    const std::vector<SyntheticRow> fixedRows{
        {1, "sg", {tuple[0], tuple[1], tuple[2], tuple[3], tuple[4], tuple[5]}},
        {1, "sg", {tuple[6], tuple[7], tuple[8], tuple[9], tuple[10], tuple[11]}},
        {1, "sg", {tuple[12], tuple[13], tuple[14], tuple[15], tuple[16], tuple[17]}}};
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        const auto parsed = epoch == FormatEpoch::ZlibLegacy
            ? makeClassContainer(0x00d8, tuple, ByteOrder::BigEndian, 1)
            : makeContainer(fixedRows, epoch);
        const auto index = LegacyRecordIndex::build(parsed);
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto referenceSession = musx::factory::DocumentFactory::begin();
        const auto reference = std::move(referenceSession).finish();
        ImportReport report;
        finale_mus_reader::PendingReferences pending;
        SourceProfile profile;
        profile.epoch = epoch;
        profile.byteOrder = parsed.byteOrder;
        const finale_mus_reader::ImportContext context{
            index, profile, document, reference, report, pending};
        finale_mus_reader::others::importShapeGraphicAssignments(context);
        const auto assignment = ShapeGraphicAssign::findForGraphic(
            document, musx::dom::SCORE_PARTID, 3);
        expectMapping(assignment && assignment->getCmper() == 1
                && assignment->left == 800 && assignment->bottom == -520
                && assignment->width == 64 && assignment->height == 20
                && assignment->fDescId == 1 && !assignment->hidden
                && assignment->hAlign == ShapeGraphicAssign::HorizontalAlignment::Left
                && assignment->vAlign == ShapeGraphicAssign::VerticalAlignment::Top
                && assignment->fixedPerc && assignment->savedRecord
                && assignment->origWidth == 64 && assignment->origHeight == 20
                && assignment->graphicCmper == 3,
            "A ShapeGraphicAssign field failed in epoch "
                + std::to_string(static_cast<int>(epoch)));
    }
}

void testMeasureGraphicAssignmentsAcrossEpochs()
{
    using Target = musx::dom::details::MeasureGraphicAssign;
    const std::vector<std::int16_t> tuple{
        0x100, 120, -324, 336, 168, 1, 0, 1, 393, 0,
        0, 1, 336, 168, 0, 0, 0, 1, 0, 0};
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        const auto parsed = epoch == FormatEpoch::ZlibLegacy
            ? makeDetailClassContainer(1, 2, 0, tuple, ByteOrder::LittleEndian)
            : makeDetailContainer(epoch, 1, 2, tuple);
        const auto index = LegacyRecordIndex::build(parsed);
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto referenceSession = musx::factory::DocumentFactory::begin();
        const auto reference = std::move(referenceSession).finish();
        ImportReport report;
        finale_mus_reader::PendingReferences pending;
        SourceProfile profile;
        profile.epoch = epoch;
        profile.byteOrder = parsed.byteOrder;
        const finale_mus_reader::ImportContext context{
            index, profile, document, reference, report, pending};
        finale_mus_reader::details::importMeasureGraphicAssignments(context);
        const auto assignment = document->getDetails()->get<Target>(
            musx::dom::SCORE_PARTID, 1, 2, musx::dom::Inci(0));
        expectMapping(assignment && assignment->version == 0x100
                && assignment->left == 120 && assignment->bottom == -324
                && assignment->width == 336 && assignment->height == 168
                && assignment->fDescId == 1 && !assignment->hidden
                && assignment->savedRecord && assignment->origWidth == 336
                && assignment->origHeight == 168 && assignment->graphicCmper == 1,
            "A MeasureGraphicAssign field or comparator failed in epoch "
                + std::to_string(static_cast<int>(epoch)));
        expectMapping(report.fields.size() == 11,
            "A MeasureGraphicAssign did not report every imported field");
    }
    const auto bigEndian = makeDetailClassContainer(7, 12, 2, tuple, ByteOrder::BigEndian);
    const auto bigEndianIndex = LegacyRecordIndex::build(bigEndian);
    const auto bigEndianRows = bigEndianIndex.getClassDetails().getArray(0x041d, 7, 12);
    expectMapping(bigEndianRows.size() == 1 && bigEndianRows.front().inci == 2
            && bigEndianIndex.getClassDetails().get(0x041d, 7, 12, 0) == nullptr
            && bigEndianIndex.getClassDetails().get(0x041d, 7, 12, 2)
                == &bigEndianRows.front()
            && bigEndianIndex.getClassDetails().payloadOf(bigEndianRows.front()).size() == 40,
        "A big-endian zlib detail did not preserve cmper2, incidence, and payload length");
}

} // namespace

namespace finale_mus_reader_tests {

TEST_CASE("Clef tuple decoding", "[mapping]") { testClefTupleDecoding(); }
TEST_CASE("Stem pre-Finale-3.5 units", "[mapping]") { testStemPreFinale35Units(); }
TEST_CASE("Stem connections are source owned", "[mapping]")
{
    testStemConnectionsAreSourceOwned();
}
TEST_CASE("Stale Unicode stem record", "[mapping]") { testStemStaleUnicodeRecord(); }
TEST_CASE("Stem font reference validation", "[mapping]")
{
    testStemFontReferenceValidation();
}
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
TEST_CASE("Graphic assignments span four epochs", "[mapping]")
{
    testGraphicAssignmentsAcrossEpochs();
}
TEST_CASE("Embedded graphic framing", "[mapping]") { testEmbeddedGraphicFraming(); }
TEST_CASE("Shape graphic assignments span four epochs", "[mapping]")
{
    testShapeGraphicAssignmentsAcrossEpochs();
}
TEST_CASE("Measure graphic assignments span four epochs", "[mapping]")
{
    testMeasureGraphicAssignmentsAcrossEpochs();
}

} // namespace finale_mus_reader_tests
