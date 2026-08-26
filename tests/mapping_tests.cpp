// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// White-box tests for the table-driven mapping framework. These drive the engine with
// synthetic record streams and purpose-built tables so that each mechanism can be
// exercised on its own, including the ones no promoted mapping uses yet.

#include <algorithm>
#include <array>
#include <span>
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
#include "import/options/test_access.h"
#include "records/legacy_record_index.h"

#include "musx/musx.h"

#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#define FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML
#endif // !defined(MUSX_USE_PUGIXML)

#include "musx/xml/PugiXmlImpl.h"

#ifdef FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML
#undef MUSX_USE_PUGIXML
#undef FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML
#endif // defined(FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML)

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

// These tests drive one importer at a time against a synthesized record set, so none of them
// has a source file for the importer to read the header out of.
constexpr std::span<const std::uint8_t> noSource{};

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

struct SyntheticClassRow
{
    std::uint16_t classId{};
    std::vector<std::int16_t> words;
    std::uint16_t cmper{GLOBALS_CMPER};
};

/// @brief Builds a parsed container holding class-identified records, the 2007+ framing.
finale_mus_reader::container::ParsedContainer makeClassContainer(
    const std::vector<SyntheticClassRow>& rows, ByteOrder byteOrder)
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
    for (const auto& row : rows) {
        push16(row.classId);
        push16(row.cmper);
        push16(0);
        const auto length = static_cast<std::uint32_t>(row.words.size() * 2);
        if (byteOrder == ByteOrder::BigEndian) {
            for (int shift = 24; shift >= 0; shift -= 8) {
                block.data.push_back(static_cast<std::uint8_t>(length >> shift));
            }
        } else {
            for (int shift = 0; shift <= 24; shift += 8) {
                block.data.push_back(static_cast<std::uint8_t>(length >> shift));
            }
        }
        for (const auto word : row.words) {
            push16(static_cast<std::uint16_t>(word));
        }
        block.data.insert(block.data.end(), 4, 0);
    }
    block.info.decodedSize = block.data.size();
    parsed.blocks.push_back(std::move(block));
    return parsed;
}

/// @brief Builds a parsed container holding one class-identified record.
finale_mus_reader::container::ParsedContainer makeClassContainer(
    std::uint16_t classId, const std::vector<std::int16_t>& words, ByteOrder byteOrder,
    std::uint16_t cmper = GLOBALS_CMPER)
{
    return makeClassContainer({SyntheticClassRow{classId, words, cmper}}, byteOrder);
}

void testClassRecordContinuationSegment()
{
    const auto verify = [](ByteOrder byteOrder) {
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
        const auto push32 = [&](std::uint32_t value) {
            if (byteOrder == ByteOrder::BigEndian) {
                push16(static_cast<std::uint16_t>(value >> 16U));
                push16(static_cast<std::uint16_t>(value));
            } else {
                push16(static_cast<std::uint16_t>(value));
                push16(static_cast<std::uint16_t>(value >> 16U));
            }
        };
        const auto appendHeader = [&](std::uint16_t classId, std::uint16_t cmper,
                                      std::uint16_t partId, std::uint32_t length) {
            push16(classId);
            push16(cmper);
            push16(partId);
            push32(length);
        };

        appendHeader(0x00b1, 1, 1, 12);
        block.data.insert(block.data.end(), {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
        // The continuation occupies the declared byte count including its repeated-size
        // prefix. It is framing, not another searchable incidence.
        push32(12);
        block.data.insert(block.data.end(), {21, 22, 23, 24, 25, 26, 27, 28});
        push16(0);
        push16(0x1234);

        appendHeader(0x00d6, 3, 0, 4);
        block.data.insert(block.data.end(), {31, 32, 33, 34});
        block.data.insert(block.data.end(), 4, 0);
        block.info.decodedSize = block.data.size();
        parsed.blocks.push_back(std::move(block));

        const auto index = LegacyRecordIndex::build(parsed);
        expectMapping(index.getClassOthers().cmpersForTag(0x00b1).empty()
                && index.getClassOthers().cmpersForTag(0x00b1, 1)
                    == std::vector<std::uint16_t>{1},
            "A part-owned class record leaked into the score comparator list");
        const auto first = index.getClassOthers().get(0x00b1, 1, 0, 0, 1);
        const auto firstPayload = first
            ? index.getClassOthers().payloadOf(*first) : std::span<const std::uint8_t>{};
        expectMapping(firstPayload.size() == 12 && firstPayload.front() == 1
                && firstPayload.back() == 12,
            "A class-record continuation replaced the primary payload");
        const auto following = index.getClassOthers().get(0x00d6, 3, 0, 0);
        const auto followingPayload = following
            ? index.getClassOthers().payloadOf(*following) : std::span<const std::uint8_t>{};
        expectMapping(followingPayload.size() == 4 && followingPayload.front() == 31
                && followingPayload.back() == 34,
            "A class-record continuation stopped the remaining pool");
    };

    verify(ByteOrder::BigEndian);
    verify(ByteOrder::LittleEndian);
}

finale_mus_reader::container::ParsedContainer makeDetailContainer(
    FormatEpoch epoch, std::uint16_t staffId, std::uint16_t meas,
    const std::vector<std::int16_t>& words, const char* tag = "mg")
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
        push16(finale_mus_reader::records::packTag(tag));
        for (std::size_t slot = 0; slot < finale_mus_reader::records::detailWordCount; ++slot)
            push16(static_cast<std::uint16_t>(words[at + slot]));
    }
    block.info.decodedSize = block.data.size();
    parsed.blocks.push_back(std::move(block));
    return parsed;
}

finale_mus_reader::container::ParsedContainer makeDetailClassContainer(
    std::uint16_t staffId, std::uint16_t meas, std::uint16_t partId,
    const std::vector<std::int16_t>& words, ByteOrder byteOrder,
    std::uint16_t classId = 0x041d)
{
    auto parsed = makeClassContainer(classId, {}, byteOrder, staffId);
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
    push16(classId);
    push16(staffId);
    push16(meas);
    push16(partId);
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
struct ExpectedReportField
{
    std::string member;
    std::optional<musx::dom::Cmper> cmper;
};

ExpectedReportField expectedReportField(std::string_view target)
{
    ExpectedReportField result{std::string(target), std::nullopt};
    if (target.starts_with("options.fontOptions[")) {
        result.member = "fonts["
            + std::string(target.substr(std::string_view("options.fontOptions[").size()));
    } else if (target.starts_with("options.fontOptionsPhysical[")) {
        result.member = "physical[" + std::string(
            target.substr(std::string_view("options.fontOptionsPhysical[").size()));
    } else if (target.starts_with("options.")) {
        const auto dot = target.find('.', std::string_view("options.").size());
        if (dot != std::string_view::npos) result.member = target.substr(dot + 1);
    } else if (const auto open = target.find('['); open != std::string_view::npos) {
        const auto close = target.find(']', open);
        const auto dot = close == std::string_view::npos ? close : target.find('.', close);
        if (close != std::string_view::npos && dot != std::string_view::npos) {
            result.cmper = static_cast<musx::dom::Cmper>(std::stoul(
                std::string(target.substr(open + 1, close - open - 1))));
            result.member = target.substr(dot + 1);
        }
    }
    return result;
}

const finale_mus_reader::FieldInfo& field(const ImportReport& report, std::string_view target)
{
    const auto expected = expectedReportField(target);
    for (const auto& [instance, fields] : report.fields) {
        if (expected.cmper && instance.cmper1 != expected.cmper) continue;
        for (const auto& [member, info] : fields) {
            if (member == expected.member) return info;
        }
    }
    std::string message = std::string("Missing mapping report for ").append(target)
        .append("; available members:");
    for (const auto& [instance, fields] : report.fields) {
        static_cast<void>(instance);
        for (const auto& [member, info] : fields) {
            static_cast<void>(info);
            message.append(" ").append(member);
        }
    }
    throw std::runtime_error(std::move(message));
}

/// @brief Whether the report names a target at all, for a field a record is not expected to have.
bool fieldPresent(const ImportReport& report, std::string_view target)
{
    const auto expected = expectedReportField(target);
    for (const auto& [instance, fields] : report.fields) {
        if (expected.cmper && instance.cmper1 != expected.cmper) continue;
        for (const auto& [member, info] : fields) {
            (void)info;
            if (member == expected.member) return true;
        }
    }
    return false;
}

template <typename Predicate>
bool anyMappingReportedField(const ImportReport& report, Predicate predicate)
{
    for (const auto& [instance, fields] : report.fields) {
        (void)instance;
        for (const auto& [member, info] : fields) {
            if (predicate(member, info)) return true;
        }
    }
    return false;
}

std::size_t reportedFieldCount(const ImportReport& report)
{
    std::size_t result = 0;
    for (const auto& [instance, fields] : report.fields) {
        (void)instance;
        result += fields.size();
    }
    return result;
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
        std::size_t recovered = 0;
        anyMappingReportedField(report, [&](const auto& member, const auto& info) {
            if (info.origin == ValueOrigin::LegacyMus
                    && member.find("clefDefs") != std::string::npos) ++recovered;
            return false;
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
        musx::factory::ConstructionContext construction;
        finale_mus_reader::options::validateClefOptions(document, report, construction);
        expectMapping(options->defaultClef == 99,
            "An out-of-range default clef index was silently corrected");
        expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                          [](const finale_mus_reader::Diagnostic& entry) {
                              return entry.message.find("default clef index 99") != std::string::npos;
                          }),
            "An out-of-range default clef index was accepted without a warning");
        ImportReport clean;
        options->defaultClef = 17;
        finale_mus_reader::options::validateClefOptions(document, clean, construction);
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

/// @brief A document whose MultimeasureRestOptions carries the pinned baseline's values.
/// @details The three the early era cannot state are what matter: the baseline starts the
/// H-bar 30 Evpu in, ends it 30 Evpu out, and switches automatic updating on.
///
/// `noHorizontalStretch` is seeded **true**, which the pinned baseline is not. That option
/// arrived with Finale 27, so no legacy document can state it and the reader must assert it
/// false rather than take the baseline's word; seeding the baseline's own false would let an
/// implementation that merely inherited pass this test.
musx::dom::DocumentPtr makeMmRestDocument()
{
    using MmRest = musx::dom::options::MultimeasureRestOptions;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto options = std::make_shared<MmRest>(document);
    options->measWidth = 360;
    options->numAdjY = -32;
    options->shapeDef = 3;
    options->numStart = 2;
    options->useSymsThreshold = 9;
    options->symSpacing = 48;
    options->startAdjust = 30;
    options->endAdjust = -30;
    options->autoUpdateMmRests = true;
    options->noHorizontalStretch = true;
    document->getOptions()->add(MmRest::XmlNodeName, options);
    return std::move(session).finish();
}

// Finale 3.5 rewrote the multimeasure-rest record, and the reader decides that boundary from
// the size of the selector-25 family rather than from the version. The boundary falls inside
// the uncompressed epoch, so no epoch gate can express it, and no tracked fixture can exercise
// the uncompressed half of the early layout -- the corpus has no publishable Finale 3.0 or 3.2
// document -- so both shapes are built here, and each is read under versions that would
// contradict the marker if the marker were not what decides.
void testMmRestEarlyLayoutMarker()
{
    using MmRest = musx::dom::options::MultimeasureRestOptions;
    const auto runImport = [](const std::vector<SyntheticRow>& rows,
                               const SourceProfile& profile, ImportReport& report) {
        const auto document = makeMmRestDocument();
        const auto reference = makeMmRestDocument();
        const auto index = LegacyRecordIndex::build(makeContainer(rows));
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importMultimeasureRestOptions(context);
        return document->getOptions()->get<MmRest>();
    };

    // The early record: one incidence, the adjustment and shape in slots 4 and 5. Read through
    // the later table these same words would give a shape of 7 and a "start number at" of -20.
    const std::vector<SyntheticRow> early{{GLOBALS_CMPER, "25", {320, 3, 24, 7, -20, 5}}};
    // The later record: two incidences, everything in the framework's places.
    const std::vector<SyntheticRow> later{
        {GLOBALS_CMPER, "25", {216, 0, -20, 5, 3, 11}},
        {GLOBALS_CMPER, "25", {60, -4, 12, -12, 0, 1}},
    };

    auto coda = profileFor(2, 6);
    coda.epoch = FormatEpoch::CodaBanner;
    coda.version.reset();
    // Finale 3.0 and 3.2 by version, one version from well past the boundary, and a
    // Coda-banner file that states no version at all.
    for (const auto& profile : {profileFor(3, 0), profileFor(3, 2), profileFor(15, 0), coda}) {
        ImportReport report;
        const auto options = runImport(early, profile, report);
        expectMapping(options->measWidth == 320 && options->numAdjY == -20
                && options->shapeDef == 5,
            "The early multimeasure-rest layout was not read from its own slots");
        expectMapping(options->numStart == 2 && options->useSymsThreshold == 9
                && options->symSpacing == 48,
            "The early layout disturbed a field the baseline supplies");
        // The three the baseline gets wrong for this era, plus the Finale 27 option no legacy
        // era has at all.
        expectMapping(options->startAdjust == 0 && options->endAdjust == 0
                && !options->autoUpdateMmRests && !options->noHorizontalStretch,
            "An early document inherited a value its era cannot have stated");
        expectMapping(field(report, "options.multimeasureRestOptions.startAdjust").origin
                == ValueOrigin::LegacyBehavior,
            "An asserted early value was not reported as era behavior");
        expectMapping(field(report, "options.multimeasureRestOptions.symSpacing").origin
                == ValueOrigin::Finale27Default,
            "An early field the baseline supplies was claimed as read");
        // These documents recover shape 5 and define no shapes at all, which is the ordinary
        // Finale case rather than a fault: hundreds of corpus documents name an H-bar shape
        // their own file never carries. It must be noted, and noted at Info, so that a host
        // filtering for real problems does not see several hundred false ones.
        expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                          [](const finale_mus_reader::Diagnostic& entry) {
                              return entry.level == musx::util::Logger::LogLevel::Info
                                  && entry.message.find("H-bar") != std::string::npos;
                          }),
            "A multimeasure-rest H-bar naming an undefined shape was not noted at Info");
    }

    // The same reader on the two-incidence record, including under versions that predate the
    // boundary and under the Coda-banner epoch. Whatever a file claims to be, the record it
    // carries is what decides: the epoch mask says only that these are 16-byte rows.
    for (const auto& profile :
            {profileFor(3, 0), profileFor(3, 5), profileFor(15, 0), coda}) {
        ImportReport report;
        const auto options = runImport(later, profile, report);
        expectMapping(options->measWidth == 216 && options->numAdjY == -20
                && options->shapeDef == 5 && options->numStart == 3
                && options->useSymsThreshold == 11,
            "The later multimeasure-rest layout was not read from its own slots");
        expectMapping(options->symSpacing == 60 && options->numAdjX == -4
                && options->startAdjust == 12 && options->endAdjust == -12
                && options->useSymbols,
            "The later layout's second incidence was not read");
        expectMapping(field(report, "options.multimeasureRestOptions.startAdjust").origin
                == ValueOrigin::LegacyMus,
            "A recovered H-bar adjustment was reported as era behavior");
        // Asserted in every era, not only the early one, because no legacy format has it.
        expectMapping(!options->noHorizontalStretch
                && field(report, "options.multimeasureRestOptions.noHorizontalStretch").origin
                    == ValueOrigin::LegacyBehavior,
            "A later-layout document inherited the baseline's horizontal-stretch setting");
        // Selector 83 is still absent, which is what a Finale 3.5 or 3.7 document looks like.
        expectMapping(!options->autoUpdateMmRests
                && field(report, "options.multimeasureRestOptions.autoUpdateMmRests").origin
                    == ValueOrigin::LegacyBehavior,
            "A document without selector 83 kept the baseline's automatic-update setting");
    }

    // With the selector present, the flag is read rather than asserted, and from word 4.
    ImportReport report;
    auto withUpdate = later;
    withUpdate.push_back({GLOBALS_CMPER, "83", {0, 0, 1, 0, 1, 0}});
    const auto options = runImport(withUpdate, profileFor(5, 0), report);
    expectMapping(options->autoUpdateMmRests
            && field(report, "options.multimeasureRestOptions.autoUpdateMmRests").origin
                == ValueOrigin::LegacyMus,
        "The automatic-update word was not read from selector 83");

    // An absent selector 83 means off with no further qualification, including for a document
    // whose epoch could not be classified. That is the document most in need of it: nothing was
    // read from it, and the baseline would otherwise leave it claiming a Finale 27 setting.
    ImportReport unclassified;
    SourceProfile unknown;
    unknown.epoch = FormatEpoch::Unknown;
    const auto unknownOptions = runImport(later, unknown, unclassified);
    expectMapping(!unknownOptions->autoUpdateMmRests
            && field(unclassified, "options.multimeasureRestOptions.autoUpdateMmRests").origin
                == ValueOrigin::LegacyBehavior,
        "An unclassified document inherited the baseline's automatic-update setting");
}

/// @brief A LyricOptions seeded with the opposite of everything the reader asserts.
/// @details Every value here contradicts what a legacy document means, so an implementation
/// that inherited the seed rather than asserting over it would fail. `hyphenChar` is the
/// deliberate exception: the reader must leave that one exactly as seeded.
musx::dom::DocumentPtr makeLyricDocument()
{
    using Lyrics = musx::dom::options::LyricOptions;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto options = std::make_shared<Lyrics>(document);
    options->hyphenChar = U'~';
    options->useAltHyphenFont = true;
    options->wordExtLineWidth = 115;
    options->lyricUseEdgePunctuation = false;
    for (const auto type : {Lyrics::SyllablePosStyleType::Default,
             Lyrics::SyllablePosStyleType::WordExt, Lyrics::SyllablePosStyleType::First,
             Lyrics::SyllablePosStyleType::SystemStart}) {
        auto style = std::make_shared<Lyrics::SyllablePosStyle>();
        style->on = true;
        options->syllablePosStyles[type] = std::move(style);
    }
    auto zeroOffset = std::make_shared<Lyrics::WordExtConnectStyle>();
    zeroOffset->connectIndex = Lyrics::WordExtConnectIndex::SystemRight;
    zeroOffset->xOffset = 91;
    zeroOffset->yOffset = 92;
    options->wordExtConnectStyles[Lyrics::WordExtConnectStyleType::ZeroOffset] =
        std::move(zeroOffset);
    document->getOptions()->add(Lyrics::XmlNodeName, options);
    return std::move(session).finish();
}

void testLyricWordExtConnectionLayouts()
{
    using Lyrics = musx::dom::options::LyricOptions;
    using ConnectType = Lyrics::WordExtConnectStyleType;
    const auto runImport = [](std::vector<SyntheticRow> rows) {
        const auto document = makeLyricDocument();
        const auto reference = makeLyricDocument();
        auto profile = profileFor(9);
        profile.epoch = FormatEpoch::DclLegacy;
        ImportReport report;
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            LegacyRecordIndex::build(makeContainer(rows, FormatEpoch::DclLegacy)), profile,
            noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importLyricOptions(context);
        return document->getOptions()->get<Lyrics>();
    };

    const std::vector<SyntheticRow> earlyRows{
        {GLOBALS_CMPER, "55", {16, 4, 1, 17, 0, 0}},
        {GLOBALS_CMPER, "55", {19, 0, 0, 20, 0, 0}},
        {GLOBALS_CMPER, "55", {17, 8, 0, 18, -8, 0}},
        {GLOBALS_CMPER, "55", {16, 42, 0, 0, 0, 0}},
    };
    const auto early = runImport(earlyRows);
    expectMapping(early->wordExtConnectStyles.at(ConnectType::SystemStart)->connectIndex
            == Lyrics::WordExtConnectIndex::DurationLyrBaseline,
        "The eight-style word-extension table did not retain its third element");
    expectMapping(early->wordExtConnectStyles.at(ConnectType::DottedEnd)->xOffset == 8
            && early->wordExtConnectStyles.at(ConnectType::DurationEnd)->xOffset == -8,
        "The eight-style word-extension table was shifted into the later layout");
    expectMapping(early->wordExtConnectStyles.at(ConnectType::ZeroOffset)->xOffset == 91
            && early->wordExtConnectStyles.at(ConnectType::ZeroOffset)->yOffset == 92,
        "The absent ninth word-extension style overwrote the seeded value");

    auto laterRows = earlyRows;
    laterRows.push_back({GLOBALS_CMPER, "55", {17, 7, 8, 0, 0, 0}});
    const auto later = runImport(std::move(laterRows));
    expectMapping(later->wordExtConnectStyles.at(ConnectType::ZeroOffset)->connectIndex
            == Lyrics::WordExtConnectIndex::HeadRightLyrBaseline
            && later->wordExtConnectStyles.at(ConnectType::ZeroOffset)->xOffset == 7
            && later->wordExtConnectStyles.at(ConnectType::ZeroOffset)->yOffset == 8,
        "The ninth word-extension style was not read from the longer layout");
}

// "Ignore Syllable Edge Punctuation" arrives with Finale 2012 and is the one lyric field whose
// word exists before its meaning does: selector 57 word 4 is clear in all 487 companion-backed
// Finale 2004-2010 documents of the reference corpus, every one of which converts with the
// punctuation *not* ignored. Reading it on such a document would invert all 487, so the gate is
// what this test pins -- one record, read under two versions, must give opposite answers.
void testLyricEdgePunctuationVersionGate()
{
    using Lyrics = musx::dom::options::LyricOptions;
    // Selector 57 with word 4 set: "use edge punctuation", the Finale 2012 spelling.
    const auto parsed = makeClassContainer(
        0x0047, {1, 0, 38, 1, 1, 0}, ByteOrder::LittleEndian);
    const auto runAt = [&](std::uint8_t major) {
        const auto document = makeLyricDocument();
        const auto reference = makeLyricDocument();
        auto profile = profileFor(major);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::LittleEndian;
        ImportReport report;
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{LegacyRecordIndex::build(parsed),
            profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importLyricOptions(context);
        return std::make_pair(document->getOptions()->get<Lyrics>(),
            field(report, "options.lyricOptions.lyricUseEdgePunctuation").origin);
    };

    // Finale 2011 is the first release that stores it, and Finale 2012 keeps doing so.
    for (const std::uint8_t major : {std::uint8_t{16}, std::uint8_t{17}}) {
        const auto [live, origin] = runAt(major);
        expectMapping(live->lyricUseEdgePunctuation && origin == ValueOrigin::LegacyMus,
            "A Finale 2011-or-later document did not read edge punctuation from selector 57"
            " word 4");
    }
    // Finale 2010 by version, on a record carrying the very same word. The era has no such
    // setting, so the word means nothing and the answer must come from era behavior instead.
    // All 22 companion-backed Finale 2010 documents of the installs corpus carry 0 here while
    // all 597 Finale 2011 ones carry 1, which is where the boundary comes from.
    const auto [ten, tenOrigin] = runAt(15);
    expectMapping(ten->lyricUseEdgePunctuation && tenOrigin == ValueOrigin::LegacyBehavior,
        "A pre-Finale-2011 document read a word its era does not use");
    // And a zlib document whose version could not be recovered falls to the same era behavior,
    // which is the right answer for every release but one.
    const auto document = makeLyricDocument();
    const auto reference = makeLyricDocument();
    SourceProfile unknown;
    unknown.epoch = FormatEpoch::ZlibLegacy;
    unknown.byteOrder = ByteOrder::LittleEndian;
    ImportReport report;
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{LegacyRecordIndex::build(parsed), unknown,
        noSource, document, reference, report, pending, construction};
    finale_mus_reader::options::importLyricOptions(context);
    expectMapping(document->getOptions()->get<Lyrics>()->lyricUseEdgePunctuation
            && field(report, "options.lyricOptions.lyricUseEdgePunctuation").origin
                == ValueOrigin::LegacyBehavior,
        "A zlib document with no recoverable version did not fall back to era behavior");
}

// The punctuation tail is readable only where its encoding is verified. Finale 2011 stores the
// switch but predates Unicode, and no document of that release carries a tail for anyone to
// check against, so a tail found there is reported and left rather than guessed at.
void testLyricPunctuationTailEncodingGate()
{
    using Lyrics = musx::dom::options::LyricOptions;
    // Selector 57 with a four-character tail, the shape the Finale 2012 fixture carries.
    // Word 6 onward differs by era, so each version is given the layout its release writes:
    // 16-bit code units for Finale 2012, packed bytes for Finale 2011. Little-endian in both,
    // which is what makes the transposition hazard real rather than theoretical.
    const std::vector<std::int16_t> unicodeTail{1, 0, 38, 1, 1, 0, 0x23, 0x40, 0x25, 0x26, 0, 0};
    const std::vector<std::int16_t> byteTail{
        1, 0, 38, 1, 1, 0, static_cast<std::int16_t>(0x4023),
        static_cast<std::int16_t>(0x2625), 0, 0, 0, 0};
    const auto runAt = [&](std::uint8_t major) {
        const auto parsed = makeClassContainer(0x0047,
            major >= 17 ? unicodeTail : byteTail, ByteOrder::LittleEndian);
        const auto document = makeLyricDocument();
        const auto reference = makeLyricDocument();
        auto profile = profileFor(major);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::LittleEndian;
        ImportReport report;
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{LegacyRecordIndex::build(parsed),
            profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importLyricOptions(context);
        return std::make_pair(document->getOptions()->get<Lyrics>(), report);
    };

    const auto [twelve, twelveReport] = runAt(17);
    expectMapping(twelve->lyricPunctuationToIgnore == "#@%&",
        "A Finale 2012 tail was not decoded as UTF-16");
    expectMapping(field(twelveReport, "options.lyricOptions.lyricPunctuationToIgnore").origin
            == ValueOrigin::LegacyMus,
        "A decoded Finale 2012 tail was not reported as read");

    // Finale 2011 reads the switch from the same record but stores the tail as packed 8-bit
    // bytes. Read through the word path these same bytes would come back transposed, which is
    // what this half of the test pins: the words above hold 0x2340, 0x2526 and so on, so a
    // reader that treated them as code units would produce the pairs in the wrong order.
    const auto [eleven, elevenReport] = runAt(16);
    expectMapping(eleven->lyricUseEdgePunctuation,
        "A Finale 2011 document did not read the edge punctuation switch");
    expectMapping(eleven->lyricPunctuationToIgnore == "#@%&",
        "A Finale 2011 tail was not decoded as packed bytes");
    expectMapping(field(elevenReport, "options.lyricOptions.lyricPunctuationToIgnore").origin
            == ValueOrigin::LegacyMus,
        "A decoded Finale 2011 tail was not reported as read");
}

// Two lyric settings postdate Finale 2012, the last release this reader opens, so no legacy
// document can state either one. They are treated differently on purpose, and the difference is
// the whole point of this test: the switch is known false and is asserted over whatever the
// baseline says, while the hyphen character is left exactly as seeded because restating U+002D
// in code would be a second copy of a fact the pinned resource already carries.
void testLyricPostFormatAssertions()
{
    using Lyrics = musx::dom::options::LyricOptions;
    // A source with none of the six lyric selectors, which is what makes every value below
    // either an assertion or the seed.
    const auto index = LegacyRecordIndex::build(makeContainer({
        {GLOBALS_CMPER, "94", {0, 0, 0, 0, 0, 0}},
    }));
    const auto document = makeLyricDocument();
    const auto reference = makeLyricDocument();
    ImportReport report;
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{
        index, profileFor(9), noSource, document, reference, report, pending, construction};
    finale_mus_reader::options::importLyricOptions(context);

    const auto lyrics = document->getOptions()->get<Lyrics>();
    expectMapping(!lyrics->useAltHyphenFont,
        "The alternate hyphen font switch was inherited rather than asserted");
    expectMapping(field(report, "options.lyricOptions.useAltHyphenFont").origin
            == ValueOrigin::LegacyBehavior,
        "The alternate hyphen font switch was not reported as era behavior");
    // The one field the reader must not touch. A hard-coded U+002D would show up here as the
    // seeded tilde being overwritten.
    expectMapping(lyrics->hyphenChar == U'~',
        "The hyphen character was asserted in code instead of taken from the seed");
    expectMapping(field(report, "options.lyricOptions.hyphenChar").origin
            == ValueOrigin::MusxOnly,
        "The post-legacy hyphen character was not reported as MUSX-only");
    // musxdom populates altHyphenFont only from an <altHyphenFont> element and synthesizes one
    // in integrityCheck, so the field surface is reported without claiming a source.
    for (const auto* member : {"altHyphenFont.fontId", "altHyphenFont.fontSize",
             "altHyphenFont.effects"}) {
        expectMapping(field(report, std::string("options.lyricOptions.") + member).origin
                == ValueOrigin::MusxOnly,
            std::string("The alternate hyphen font field was not MUSX-only: ") + member);
    }

    // The three assertions that do contradict the seed, for contrast: without any of the six
    // selectors the reader must not leave a document claiming these.
    expectMapping(lyrics->wordExtLineWidth == 224,
        "The word extension line width was inherited rather than asserted");
    expectMapping(lyrics->lyricUseEdgePunctuation,
        "The syllable edge punctuation setting was inherited rather than asserted");
    for (const auto type : {Lyrics::SyllablePosStyleType::WordExt,
             Lyrics::SyllablePosStyleType::First, Lyrics::SyllablePosStyleType::SystemStart}) {
        expectMapping(!lyrics->syllablePosStyles.at(type)->on,
            "An optional syllable position was left switched on by an era that has no record"
            " for it");
    }
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
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
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
    expectMapping(anyMappingReportedField(report, [](const auto& member, const auto& value) {
                      return member == "stemConnections[0].symbol"
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

// A connection names its font by comparator. A dangling one is preserved rather than replaced,
// because a default would invent a typeface the source never named; registering it is what lets
// musxdom mint and log a placeholder for it at the end of construction instead of leaving the
// comparator unusable, which is what testDanglingFontComparatorRequiresRegistration verifies.
void testStemFontReferenceValidation()
{
    ImportReport report;
    const auto document = makeStemDocument();
    const auto options = captureStems(
        makeContainer({{GLOBALS_CMPER, "40", {7, 192, 768, -768, 0, 0}},
            {GLOBALS_CMPER, "40", {0, 0, 0, 0, 0, 0}}}),
        profileFor(5, 0), document, report);
    musx::factory::ConstructionContext construction;
    finale_mus_reader::options::validateStemOptions(document, construction);
    expectMapping(options->stemConnections.size() == 1
            && options->stemConnections[0]->fontId == 7,
        "A stem connection did not keep the font comparator the source stated");
}

// No real Finale save can stand in for a comparator that resolves to nothing: Finale always
// writes some definition for every comparator it uses, even a placeholder of its own. The
// state this reader must survive -- a hand-edited or otherwise malformed source naming a font
// id its own table never defines -- has to be built synthetically instead. This constructs it
// directly against musxdom's own construction session, twice: once exactly as an importer that
// forgot to register the comparator would leave it, and once as the reader actually does.
//
// Both documents give a TextOptions symbol insert a font id no FontDefinition in the document
// answers. The only difference is whether that id is registered with the session's own
// ConstructionContext before the session finishes -- which is what
// options::registerSymbolInsertFonts does in the real pipeline -- and that difference is the
// whole story: unregistered, FontInfo::getName throws exactly as it did before musxdom offered
// a placeholder; registered, the same call resolves to musxdom's "Missing Font (n)" spelling.
void testDanglingFontComparatorRequiresRegistration()
{
    using TextOptions = musx::dom::options::TextOptions;
    using Insert = musx::dom::options::AccidentalInsertSymbolType;
    constexpr musx::dom::Cmper danglingFontId = 909;

    const auto buildDocument = [](bool registerComparator) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto options = std::make_shared<TextOptions>(document);
        options->textLineSpacingPercent = 100;
        auto insert = std::make_shared<TextOptions::InsertSymbolInfo>(options);
        auto font = std::make_shared<musx::dom::FontInfo>(document, /*sizeIsPercent*/ true);
        font->fontId = danglingFontId;
        font->fontSize = 12;
        insert->symFont = std::move(font);
        options->symbolInserts[Insert::Sharp] = std::move(insert);
        document->getOptions()->add(TextOptions::XmlNodeName, options);
        if (registerComparator) {
            session.getConstructionContext().registerFontId(danglingFontId);
        }
        return std::move(session).finish();
    };

    const auto getSymFont = [](const musx::dom::DocumentPtr& document) {
        return document->getOptions()->get<TextOptions>()
            ->symbolInserts.at(Insert::Sharp)->symFont;
    };

    const auto unregistered = buildDocument(false);
    expectMapping(getSymFont(unregistered)->fontId == danglingFontId,
        "An unregistered dangling font comparator did not survive construction");
    bool threw = false;
    try {
        (void)getSymFont(unregistered)->getName();
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expectMapping(threw,
        "An unregistered dangling font comparator no longer throws out of FontInfo::getName; "
        "the contrast this test relies on is gone, not just the registered half of it");

    const auto registered = buildDocument(true);
    expectMapping(getSymFont(registered)->fontId == danglingFontId,
        "A registered dangling font comparator did not survive construction");
    std::string name;
    bool registeredThrew = false;
    try {
        name = getSymFont(registered)->getName();
    } catch (const std::exception&) {
        registeredThrew = true;
    }
    expectMapping(!registeredThrew,
        "A registered dangling font comparator threw out of FontInfo::getName instead of "
        "resolving to musxdom's placeholder");
    expectMapping(name == "Missing Font (" + std::to_string(danglingFontId) + ")",
        "A registered dangling font comparator did not resolve to the placeholder spelling "
        "Finale's own conversions use");
}

void testGraphicAssignmentsAcrossEpochs()
{
    using PageGraphicAssign = musx::dom::others::PageGraphicAssign;
    const std::vector<std::int16_t> tuple{
        0x100, 120, -48, 640, 320, 7, 0, 0x11, 0x014c,
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
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::others::importPageGraphicAssignments(context);
        const auto assignment = document->getOthers()
            ->get<PageGraphicAssign>(musx::dom::SCORE_PARTID, 4, musx::dom::Inci(0));
        expectMapping(assignment && assignment->version == 0x100
                && assignment->left == 120 && assignment->bottom == -48
                && assignment->width == 640 && assignment->height == 320
                && assignment->fDescId == 7 && assignment->hidden
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
        expectMapping(reportedFieldCount(report) == 24,
            "A PageGraphicAssign did not report every persisted field");
    }

    const std::array<std::pair<std::int16_t, PageGraphicAssign::PageAssignType>, 4>
        displayTypes{{
            {std::int16_t(0x0001), PageGraphicAssign::PageAssignType::One},
            {std::int16_t(0x0002), PageGraphicAssign::PageAssignType::AllPages},
            {std::int16_t(0x0004), PageGraphicAssign::PageAssignType::Odd},
            {std::int16_t(0x0008), PageGraphicAssign::PageAssignType::Even},
        }};
    std::vector<SyntheticRow> displayRows;
    for (const auto& [displayFlags, expected] : displayTypes) {
        (void)expected;
        auto displayTuple = tuple;
        displayTuple[7] = displayFlags;
        for (std::size_t at = 0; at < displayTuple.size();
                at += finale_mus_reader::records::otherWordCount) {
            SyntheticRow row{4, "pg", {}};
            for (std::size_t slot = 0;
                    slot < finale_mus_reader::records::otherWordCount; ++slot) {
                row.words[slot] = displayTuple[at + slot];
            }
            displayRows.push_back(row);
        }
    }
    const auto displayParsed = makeContainer(displayRows, FormatEpoch::UncompressedLegacy);
    const auto displayIndex = LegacyRecordIndex::build(displayParsed);
    auto displaySession = musx::factory::DocumentFactory::begin();
    const auto displayDocument = displaySession.getDocument();
    auto displayReferenceSession = musx::factory::DocumentFactory::begin();
    const auto displayReference = std::move(displayReferenceSession).finish();
    ImportReport displayReport;
    finale_mus_reader::PendingReferences displayPending;
    SourceProfile displayProfile;
    displayProfile.epoch = FormatEpoch::UncompressedLegacy;
    displayProfile.byteOrder = displayParsed.byteOrder;
    musx::factory::ConstructionContext displayConstruction;
    const finale_mus_reader::ImportContext displayContext{displayIndex, displayProfile, noSource,
        displayDocument, displayReference, displayReport, displayPending, displayConstruction};
    finale_mus_reader::others::importPageGraphicAssignments(displayContext);
    for (std::size_t index = 0; index < displayTypes.size(); ++index) {
        const auto assignment = displayDocument->getOthers()->get<PageGraphicAssign>(
            musx::dom::SCORE_PARTID, 4, musx::dom::Inci(static_cast<int>(index)));
        expectMapping(assignment && assignment->displayType == displayTypes[index].second
                && !assignment->hidden,
            "A page graphic display flag did not map to its page-selection type");
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
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
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
        expectMapping(reportedFieldCount(report) == 14,
            "A ShapeGraphicAssign did not report every persisted field");
    }
}

// The pre-Unicode word stream of one custom line style per line style, 36 words each.
// Spelled out rather than derived, so a change to the layout has to be restated here to pass.
const std::vector<std::int16_t> ssLineCharWords{
    2, 199, 6, 17, 3, -1, -83, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const std::vector<std::int16_t> ssLineDashedWords{
    1, 118, 192, 448, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const std::vector<std::int16_t> ssLineCappedWords{
    0, 118, 0, 0, 0, 0, 0, 3, 2, 1984, 0, 2,
    0, 5, 1, 2, 3, 4, 5, 11, 13, 17, 19, 23,
    -29, 31, 37, 41, 43, 47, 61, 59, 61, 53, 0, 0};

// The same three in the Finale 2012 layout. The character occupies words 1 and 2, so only the
// Char record's own later fields move; the Dashed record is untouched and everything from the
// old word 7 on moves for all three.
const std::vector<std::int16_t> ssLineWideCharWords{
    2, 171, 0, 6, 17, 3, -1, -83, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const std::vector<std::int16_t> ssLineWideDashedWords = ssLineDashedWords;
const std::vector<std::int16_t> ssLineWideCappedWords{
    0, 118, 0, 0, 0, 0, 0, 0, 3, 2, 1984, 0,
    2, 0, 5, 1, 2, 3, 4, 5, 11, 13, 17, 19,
    23, -29, 31, 37, 41, 43, 47, 61, 59, 61, 53, 0};

/// @brief Splits one logical word stream into the six fixed rows a family occupies.
std::vector<SyntheticRow> ssLineFixedRows(
    std::uint16_t cmper, const std::vector<std::int16_t>& words)
{
    std::vector<SyntheticRow> rows;
    for (std::size_t at = 0; at < words.size(); at += 6) {
        SyntheticRow row{cmper, "ls", {}};
        for (std::size_t slot = 0; slot < 6; ++slot) {
            row.words[slot] = words[at + slot];
        }
        rows.push_back(row);
    }
    return rows;
}

/// @brief A document carrying the two font definitions the character decoding needs.
/// @details Font 6 is an ordinary Mac text font, so a byte stored in it is Mac Roman. Font 0 is
/// the default music font, whose byte is a glyph number whatever its record claims.
musx::dom::DocumentPtr ssLineDocument(musx::factory::DocumentFactory::ConstructionSession& session)
{
    using FontDefinition = musx::dom::others::FontDefinition;
    const auto document = session.getDocument();
    const auto addFont = [&](musx::dom::Cmper cmper, int charsetVal) {
        auto font = std::make_shared<FontDefinition>(
            document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
        font->charsetBank = FontDefinition::CharacterSetBank::MacOS;
        font->charsetVal = charsetVal;
        document->getOthers()->add(FontDefinition::XmlNodeName, font);
    };
    addFont(0, 4095);
    addFont(6, 0);
    return document;
}

/// @brief Runs the custom-line importer over one synthesized container.
void ssLineImport(const finale_mus_reader::container::ParsedContainer& parsed,
    const SourceProfile& profile, const musx::dom::DocumentPtr& document, ImportReport& report)
{
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{
        index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importSmartShapeCustomLines(context);
}

/// @brief Every field of every line style, in each epoch that carries the record.
/// @details The Coda-banner epoch is in the sweep to assert the opposite: the record does not
/// exist before Finale 2000, so rows carrying its tag must build nothing at all rather than
/// empty objects.
void testSmartShapeCustomLinesAcrossEpochs()
{
    using CustomLine = musx::dom::others::SmartShapeCustomLine;
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        const auto era = " in epoch " + std::to_string(static_cast<int>(epoch));
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = ssLineDocument(session);
        ImportReport report;
        SourceProfile profile;
        profile.epoch = epoch;
        if (epoch == FormatEpoch::ZlibLegacy) {
            // One record per container, so each line style is imported into its own document
            // and then read back from a document of its own.
            for (const auto& record : {std::pair{std::uint16_t{1}, ssLineCharWords},
                     std::pair{std::uint16_t{2}, ssLineDashedWords},
                     std::pair{std::uint16_t{3}, ssLineCappedWords}}) {
                const auto parsed = makeClassContainer(
                    0x00de, record.second, ByteOrder::BigEndian, record.first);
                profile.byteOrder = parsed.byteOrder;
                ssLineImport(parsed, profile, document, report);
            }
        } else {
            std::vector<SyntheticRow> rows;
            for (const auto& record : {std::pair{std::uint16_t{1}, ssLineCharWords},
                     std::pair{std::uint16_t{2}, ssLineDashedWords},
                     std::pair{std::uint16_t{3}, ssLineCappedWords}}) {
                const auto family = ssLineFixedRows(record.first, record.second);
                rows.insert(rows.end(), family.begin(), family.end());
            }
            const auto parsed = makeContainer(rows, epoch);
            profile.byteOrder = parsed.byteOrder;
            ssLineImport(parsed, profile, document, report);
        }

        const auto lines = document->getOthers()->getArray<CustomLine>(musx::dom::SCORE_PARTID);
        if (epoch == FormatEpoch::CodaBanner) {
            expectMapping(lines.empty(),
                "A Coda-banner source built custom line styles from a record that era never"
                " wrote" + era);
            expectMapping(reportedFieldCount(report) == 0,
                "A Coda-banner source reported custom line style fields it cannot have" + era);
            continue;
        }
        expectMapping(lines.size() == 3, "Not every custom line style was built" + era);

        const auto charLine = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 1);
        expectMapping(charLine && charLine->lineStyle == CustomLine::LineStyle::Char,
            "The Char line style was not recovered" + era);
        expectMapping(charLine->charParams && !charLine->solidParams && !charLine->dashedParams,
            "A Char line kept a parameter block its line style does not select" + era);
        // 199 is Mac Roman in font 6, so the document must carry the code point rather than
        // the byte. The setter musxdom owns turns the stored mask into the two flags.
        expectMapping(charLine->charParams->lineChar == 0x00ab,
            "The Char line character was not decoded through its own font" + era);
        expectMapping(charLine->charParams->font->fontId == 6
                && charLine->charParams->font->fontSize == 17
                && charLine->charParams->font->bold && charLine->charParams->font->italic
                && !charLine->charParams->font->underline,
            "The Char line font tuple was not recovered" + era);
        expectMapping(charLine->charParams->baselineShiftEms == -83,
            "The Char line baseline shift was not recovered" + era);

        const auto dashed = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 2);
        expectMapping(dashed && dashed->dashedParams && !dashed->charParams
                && dashed->dashedParams->lineWidth == 118
                && dashed->dashedParams->dashOn == 192
                && dashed->dashedParams->dashOff == 448,
            "The Dashed line parameters were not recovered" + era);

        const auto capped = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 3);
        expectMapping(capped && capped->solidParams && capped->solidParams->lineWidth == 118,
            "The Solid line width was not recovered" + era);
        expectMapping(capped->lineCapStartType == CustomLine::LineCapType::Hook
                && capped->lineCapEndType == CustomLine::LineCapType::ArrowheadCustom
                && capped->lineCapStartHookLength == 1984 && capped->lineCapEndArrowId == 2
                && capped->lineCapStartArrowId == 0 && capped->lineCapEndHookLength == 0,
            "A line cap read its shared value slot as the wrong one of the two" + era);
        expectMapping(capped->makeHorz && !capped->lineAfterLeftStartText
                && capped->lineBeforeRightEndText && !capped->lineAfterLeftContText,
            "The line adjustment flags were not recovered" + era);
        expectMapping(capped->leftStartRawTextId == 1 && capped->leftContRawTextId == 2
                && capped->rightEndRawTextId == 3 && capped->centerFullRawTextId == 4
                && capped->centerAbbrRawTextId == 5,
            "The text anchor comparators were not recovered" + era);
        expectMapping(capped->leftStartX == 11 && capped->leftStartY == 13
                && capped->leftContX == 17 && capped->leftContY == 19
                && capped->rightEndX == 23 && capped->rightEndY == -29
                && capped->centerFullX == 31 && capped->centerFullY == 37
                && capped->centerAbbrX == 41 && capped->centerAbbrY == 43,
            "The anchor offsets did not interleave X with Y" + era);
        expectMapping(capped->lineStartX == 47 && capped->lineStartY == 61
                && capped->lineEndX == 59 && capped->lineEndY == 61 && capped->lineContX == 53,
            "The line adjustments were not recovered" + era);

        // A field the record does not carry is not reported at all, because the destination
        // does not exist for it. The nested destinations are named with dots, as every other
        // nested report target is.
        expectMapping(!fieldPresent(report, "others.smartShapeCustomLine[2].charParams.lineChar"),
            "A Dashed line reported a Char parameter it cannot have" + era);
        expectMapping(fieldPresent(report, "others.smartShapeCustomLine[3].lineCapStartHookLength")
                && !fieldPresent(report, "others.smartShapeCustomLine[3].lineCapEndHookLength"),
            "An arrowhead cap reported the hook length it does not use" + era);
        expectMapping(
            field(report, "others.smartShapeCustomLine[1].charParams.font.fontSize").origin
                == ValueOrigin::LegacyMus,
            "A nested destination was not reported under its dotted path" + era);
        // The report keeps the byte the source held, not the code point it decoded to.
        expectMapping(
            field(report, "others.smartShapeCustomLine[1].charParams.lineChar").rawValue == 199,
            "The report did not keep the stored character byte" + era);
    }
}

/// @brief The Finale 2012 layout, where only the Char parameter block moves with the character.
void testSmartShapeCustomLineUnicodeLayout()
{
    using CustomLine = musx::dom::others::SmartShapeCustomLine;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = ssLineDocument(session);
    ImportReport report;
    auto profile = profileFor(17);
    profile.epoch = FormatEpoch::ZlibLegacy;
    profile.byteOrder = ByteOrder::LittleEndian;
    for (const auto& record : {std::pair{std::uint16_t{1}, ssLineWideCharWords},
             std::pair{std::uint16_t{2}, ssLineWideDashedWords},
             std::pair{std::uint16_t{3}, ssLineWideCappedWords}}) {
        ssLineImport(makeClassContainer(0x00de, record.second, profile.byteOrder, record.first),
            profile, document, report);
    }

    const auto charLine = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 1);
    expectMapping(charLine && charLine->charParams,
        "The Unicode-era Char line was not recovered");
    // Already a code point in this era, so nothing is decoded and font 6 is not consulted.
    expectMapping(charLine->charParams->lineChar == 0x00ab,
        "The Unicode-era character was not read as a code point");
    expectMapping(charLine->charParams->font->fontId == 6
            && charLine->charParams->font->fontSize == 17
            && charLine->charParams->font->bold && charLine->charParams->font->italic
            && charLine->charParams->baselineShiftEms == -83,
        "The Char parameter block did not shift with the widened character");

    const auto dashed = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 2);
    expectMapping(dashed && dashed->dashedParams && dashed->dashedParams->dashOn == 192
            && dashed->dashedParams->dashOff == 448,
        "A Dashed record shifted with a character it does not have");

    const auto capped = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 3);
    expectMapping(capped && capped->lineCapStartType == CustomLine::LineCapType::Hook
            && capped->lineCapStartHookLength == 1984 && capped->lineCapEndArrowId == 2,
        "The common part of the record did not shift in the Unicode era");
    expectMapping(capped->leftStartX == 11 && capped->centerAbbrY == 43
            && capped->lineStartX == 47 && capped->lineContX == 53,
        "The positions did not shift in the Unicode era");
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
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::details::importMeasureGraphicAssignments(context);
        const auto assignment = document->getDetails()->get<Target>(
            musx::dom::SCORE_PARTID, 1, 2, musx::dom::Inci(0));
        expectMapping(assignment && assignment->version == 0x100
                && assignment->left == 120 && assignment->bottom == -324
                && assignment->width == 336 && assignment->height == 168
                && assignment->fDescId == 1 && !assignment->hidden
                && assignment->hAlign == Target::HorizontalAlignment::Left
                && assignment->vAlign == Target::VerticalAlignment::Top
                && assignment->posFrom == Target::PositionFrom::PageEdge
                && assignment->fixedPerc
                && assignment->savedRecord && assignment->origWidth == 336
                && assignment->origHeight == 168 && assignment->graphicCmper == 1,
            "A MeasureGraphicAssign field or comparator failed in epoch "
                + std::to_string(static_cast<int>(epoch)));
        expectMapping(reportedFieldCount(report) == 15,
            "A MeasureGraphicAssign did not report every persisted field");
        const auto instance = finale_mus_reader::instanceKey<Target>(
            musx::dom::SCORE_PARTID, musx::dom::Cmper(1), musx::dom::Inci(0),
            musx::dom::Cmper(2));
        for (const auto* member : {"hAlign", "vAlign", "posFrom", "fixedPerc"}) {
            const auto* info = report.findField(instance, member);
            expectMapping(info && info->origin == ValueOrigin::LegacyMus
                    && info->rawValue == 393,
                std::string("MeasureGraphicAssign did not recover ") + member
                    + " from the packed positioning word");
        }
    }
    const auto bigEndian = makeDetailClassContainer(7, 12, 2, tuple, ByteOrder::BigEndian);
    const auto bigEndianIndex = LegacyRecordIndex::build(bigEndian);
    const auto bigEndianRows = bigEndianIndex.getClassDetails().getArray(0x041d, 7, 12, 2);
    expectMapping(bigEndianRows.size() == 1 && bigEndianRows.front().partId == 2
            && bigEndianRows.front().inci == 0
            && bigEndianIndex.getClassDetails().get(0x041d, 7, 12, 0) == nullptr
            && bigEndianIndex.getClassDetails().get(0x041d, 7, 12, 0, 2)
                == &bigEndianRows.front()
            && bigEndianIndex.getClassDetails().payloadOf(bigEndianRows.front()).size() == 40,
        "A big-endian zlib detail did not preserve cmper2, part id, and payload length");

    auto partSession = musx::factory::DocumentFactory::begin();
    const auto partDocument = partSession.getDocument();
    auto partReferenceSession = musx::factory::DocumentFactory::begin();
    const auto partReference = std::move(partReferenceSession).finish();
    ImportReport partReport;
    finale_mus_reader::PendingReferences partPending;
    SourceProfile partProfile;
    partProfile.epoch = FormatEpoch::ZlibLegacy;
    partProfile.byteOrder = ByteOrder::BigEndian;
    musx::factory::ConstructionContext partConstruction;
    const finale_mus_reader::ImportContext partContext{bigEndianIndex, partProfile, noSource,
        partDocument, partReference, partReport, partPending, partConstruction};
    finale_mus_reader::details::importMeasureGraphicAssignments(partContext);
    expectMapping(!partDocument->getDetails()->get<Target>(
            musx::dom::SCORE_PARTID, 7, 12, musx::dom::Inci(0)),
        "A part-owned zlib detail was imported into the score pool");
}

std::vector<SyntheticRow> textBlockFixedRows(
    std::uint16_t cmper, const std::vector<std::int16_t>& words)
{
    std::vector<SyntheticRow> rows;
    for (std::size_t at = 0; at < words.size(); at += 6) {
        SyntheticRow row{cmper, "TX", {}};
        for (std::size_t slot = 0; slot < 6; ++slot) row.words[slot] = words[at + slot];
        rows.push_back(row);
    }
    return rows;
}

void textBlockImport(const finale_mus_reader::container::ParsedContainer& parsed,
    const SourceProfile& profile, const musx::dom::DocumentPtr& document, ImportReport& report)
{
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{
        index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importTextBlocks(context);
}

void testStoredTextBlocksAcrossEpochs()
{
    using Target = musx::dom::others::TextBlock;
    const std::vector<std::int16_t> words{
        9, 480, 240, 3, 125, -12, 34, 0x1e09, -1, -32, 0, 64,
        2004, 0, 0, 0, 0, 0};
    for (const auto epoch : {FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        ImportReport report;
        SourceProfile profile;
        profile.epoch = epoch;
        profile.byteOrder = ByteOrder::BigEndian;
        if (epoch == FormatEpoch::ZlibLegacy) {
            textBlockImport(makeClassContainer(0x00b7, words, profile.byteOrder, 7),
                profile, document, report);
        } else {
            textBlockImport(makeContainer(textBlockFixedRows(7, words), epoch),
                profile, document, report);
        }
        const auto block = document->getOthers()->get<Target>(musx::dom::SCORE_PARTID, 7);
        expectMapping(block && block->textId == 9 && block->width == 480
                && block->height == 240 && block->shapeId == 3
                && block->lineSpacingPercentage == 125 && !block->lineSpacingEvpu
                && block->xAdd == -12 && block->yAdd == 34,
            "The stored TextBlock scalar fields were not recovered");
        expectMapping(block->justify == Target::TextJustify::Right && block->newPos36
                && block->showShape && block->noExpandSingleWord && block->wordWrap,
            "The stored TextBlock flags were not recovered");
        expectMapping(block->inset == -32 && block->stdLineThickness == 64,
            "The stored TextBlock Efix values were not read high-word first");
        expectMapping(block->textType == Target::TextType::Block,
            "The stored TextBlock block-family discriminator was not recovered");
        expectMapping(!block->roundCorners && block->cornerRadius == 0
                && field(report, "others.textBlock[7].roundCorners").origin
                    == ValueOrigin::LegacyBehavior
                && field(report, "others.textBlock[7].cornerRadius").origin
                    == ValueOrigin::LegacyBehavior,
            "The legacy TextBlock corner behavior was not reported");
        expectMapping(field(report, "others.textBlock[7].justify").rawValue == 1
                && field(report, "others.textBlock[7].textType").rawValue == 2004
                && field(report, "others.textBlock[7].lineSpacingPercentage").rawValue == 125,
            "The TextBlock report did not retain its stored values");
    }

    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    ImportReport report;
    SourceProfile profile;
    profile.epoch = FormatEpoch::UncompressedLegacy;
    profile.byteOrder = ByteOrder::BigEndian;
    auto absolute = words;
    absolute[4] = 72;
    absolute[7] = 2;
    textBlockImport(makeContainer(textBlockFixedRows(8, absolute)),
        profile, document, report);
    const auto block = document->getOthers()->get<Target>(musx::dom::SCORE_PARTID, 8);
    expectMapping(block && block->lineSpacingEvpu == 72 && !block->lineSpacingPercentage
            && block->justify == Target::TextJustify::Center,
        "The absolute TextBlock spacing or center justification was not recovered");

    auto zeroSpacingSession = musx::factory::DocumentFactory::begin();
    const auto zeroSpacingDocument = zeroSpacingSession.getDocument();
    ImportReport zeroSpacingReport;
    auto zeroSpacing = absolute;
    zeroSpacing[4] = 0;
    textBlockImport(makeContainer(textBlockFixedRows(10, zeroSpacing)),
        profile, zeroSpacingDocument, zeroSpacingReport);
    const auto zeroSpacingBlock = zeroSpacingDocument->getOthers()->get<Target>(
        musx::dom::SCORE_PARTID, 10);
    expectMapping(zeroSpacingBlock && zeroSpacingBlock->lineSpacingEvpu == 0
            && !zeroSpacingBlock->lineSpacingPercentage
            && field(zeroSpacingReport,
                "others.textBlock[10].lineSpacingEvpu").rawValue == 0,
        "Zero-EVPU TextBlock spacing was not preserved as the stored setting");

    auto zeroPercentSession = musx::factory::DocumentFactory::begin();
    const auto zeroPercentDocument = zeroPercentSession.getDocument();
    ImportReport zeroPercentReport;
    auto zeroPercent = words;
    zeroPercent[4] = 0;
    textBlockImport(makeContainer(textBlockFixedRows(11, zeroPercent)),
        profile, zeroPercentDocument, zeroPercentReport);
    const auto zeroPercentBlock = zeroPercentDocument->getOthers()->get<Target>(
        musx::dom::SCORE_PARTID, 11);
    expectMapping(zeroPercentBlock && zeroPercentBlock->lineSpacingEvpu == 0
            && !zeroPercentBlock->lineSpacingPercentage
            && field(zeroPercentReport,
                "others.textBlock[11].lineSpacingPercentage").rawValue == 0
            && field(zeroPercentReport,
                "others.textBlock[11].lineSpacingEvpu").origin
                == ValueOrigin::LegacyBehavior,
        "Zero-percent TextBlock spacing was not upgraded to zero EVPU");

    for (const std::int16_t discriminator : {
             static_cast<std::int16_t>(finale_mus_reader::records::packTag("xp")),
             std::int16_t(2006)}) {
        auto expressionSession = musx::factory::DocumentFactory::begin();
        const auto expressionDocument = expressionSession.getDocument();
        ImportReport expressionReport;
        auto expression = words;
        expression[12] = discriminator;
        textBlockImport(makeContainer(textBlockFixedRows(9, expression), FormatEpoch::DclLegacy),
            profile, expressionDocument, expressionReport);
        const auto expressionBlock = expressionDocument->getOthers()->get<Target>(
            musx::dom::SCORE_PARTID, 9);
        expectMapping(expressionBlock
                && expressionBlock->textType == Target::TextType::Expression
                && field(expressionReport, "others.textBlock[9].textType").rawValue
                    == discriminator,
            "A stored TextBlock expression-family discriminator was not recovered");
    }

    auto taggedBlockSession = musx::factory::DocumentFactory::begin();
    const auto taggedBlockDocument = taggedBlockSession.getDocument();
    ImportReport taggedBlockReport;
    auto taggedBlockWords = words;
    taggedBlockWords[12] =
        static_cast<std::int16_t>(finale_mus_reader::records::packTag("bl"));
    textBlockImport(makeContainer(textBlockFixedRows(10, taggedBlockWords)),
        profile, taggedBlockDocument, taggedBlockReport);
    const auto taggedBlock = taggedBlockDocument->getOthers()->get<Target>(
        musx::dom::SCORE_PARTID, 10);
    expectMapping(taggedBlock && taggedBlock->textType == Target::TextType::Block
            && field(taggedBlockReport, "others.textBlock[10].textType").rawValue
                == taggedBlockWords[12],
        "The packed block-family TextBlock discriminator was not recovered");

    auto oldSession = musx::factory::DocumentFactory::begin();
    const auto oldDocument = oldSession.getDocument();
    ImportReport oldReport;
    auto oldWords = words;
    oldWords[12] = 0;
    textBlockImport(makeContainer(textBlockFixedRows(11, oldWords)),
        profile, oldDocument, oldReport);
    const auto oldBlock = oldDocument->getOthers()->get<Target>(musx::dom::SCORE_PARTID, 11);
    expectMapping(oldBlock && oldBlock->textType == Target::TextType::Block,
        "A TextBlock without a family discriminator did not retain the block default");
}

void testCodaTextBlockSynthesis()
{
    using BlockText = musx::dom::texts::BlockText;
    using Target = musx::dom::others::TextBlock;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    for (musx::dom::Cmper number : {musx::dom::Cmper{1}, musx::dom::Cmper{2}}) {
        auto text = std::make_shared<BlockText>(document, musx::dom::SCORE_PARTID,
            musx::dom::EnigmaBase::ShareMode::All, number);
        text->text = "block " + std::to_string(number);
        document->getTexts()->add(BlockText::XmlNodeName, std::move(text));
    }
    const auto parsed = makeContainer({{0, "HS", {0, 0, 1036, 0, 0, 0x0080}},
        {0, "HS", {0, 0, 1036, 0, 0, 0x0081}}}, FormatEpoch::CodaBanner);
    ImportReport report;
    SourceProfile profile;
    profile.epoch = FormatEpoch::CodaBanner;
    profile.byteOrder = ByteOrder::BigEndian;
    textBlockImport(parsed, profile, document, report);

    const auto first = document->getOthers()->get<Target>(musx::dom::SCORE_PARTID, 1);
    const auto second = document->getOthers()->get<Target>(musx::dom::SCORE_PARTID, 2);
    expectMapping(first && first->textId == 1 && first->justify == Target::TextJustify::Left
            && second && second->textId == 2 && second->justify == Target::TextJustify::Right,
        "Coda TextBlocks did not follow the HS/HT structural order");
    expectMapping(first->lineSpacingPercentage == 100 && !first->newPos36
            && first->shapeId == 0 && !first->showShape && first->wordWrap
            && !first->noExpandSingleWord && !second->noExpandSingleWord
            && !first->roundCorners && first->cornerRadius == 0,
        "Coda TextBlock behavior was not synthesized");
    expectMapping(field(report, "others.textBlock[1].textId").origin == ValueOrigin::LegacyMus
            && field(report, "others.textBlock[1].newPos36").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].shapeId").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].showShape").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].noExpandSingleWord").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].wordWrap").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].roundCorners").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].cornerRadius").origin
                == ValueOrigin::LegacyBehavior,
        "Coda stored identity and era behavior were not reported separately");
}

void testFretClassesAreSourceOwned()
{
    using FretInstrument = musx::dom::others::FretInstrument;
    using FretboardGroup = musx::dom::others::FretboardGroup;
    using FretboardStyle = musx::dom::others::FretboardStyle;
    using FretboardDiagram = musx::dom::details::FretboardDiagram;

    std::vector<std::int16_t> instrumentWords(36);
    instrumentWords[0] = 0x000a;
    instrumentWords[2] = 21;
    instrumentWords[3] = 2;
    instrumentWords[4] = 5;
    instrumentWords[6] = 0x4869;
    instrumentWords[30] = 0x4000;
    instrumentWords[31] = 0x3b00;
    std::vector<std::int16_t> groupWords(30);
    groupWords[0] = 7;
    groupWords[6] = 0x4772;
    std::vector<std::int16_t> styleWords(78);
    styleWords[0] = 1;
    styleWords[3] = 11;
    styleWords[4] = 12;
    styleWords[5] = 13;
    styleWords[6] = 14;
    styleWords[7] = 15;
    styleWords[8] = 4;
    styleWords[10] = 900;
    styleWords[12] = 1404;
    styleWords[29] = 2;
    styleWords[30] = 9;
    styleWords[32] = 3;
    styleWords[33] = 5;
    styleWords[38] = 80;
    styleWords[42] = 0x5374;
    styleWords[66] = 0x6672;
    styleWords[67] = 0x2e00;

    const auto parsed = makeClassContainer({{0x0094, groupWords, 9},
        {0x0095, instrumentWords, 7}, {0x0097, styleWords, 3}},
        ByteOrder::BigEndian);
    const auto index = LegacyRecordIndex::build(parsed);
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    ImportReport report;
    finale_mus_reader::PendingReferences pending;
    SourceProfile profile;
    profile.epoch = FormatEpoch::ZlibLegacy;
    profile.byteOrder = ByteOrder::BigEndian;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{
        index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importFretInstruments(context);
    finale_mus_reader::others::importFretboardGroups(context);
    finale_mus_reader::others::importFretboardStyles(context);

    const auto instrument = document->getOthers()->get<FretInstrument>(
        musx::dom::SCORE_PARTID, 7);
    const auto group = document->getOthers()->get<FretboardGroup>(
        musx::dom::SCORE_PARTID, 9, 0);
    const auto style = document->getOthers()->get<FretboardStyle>(
        musx::dom::SCORE_PARTID, 3);
    expectMapping(instrument && instrument->numFrets == 21 && instrument->numStrings == 2
            && instrument->speedyClef == 5 && instrument->name == "Hi"
            && instrument->strings.size() == 2 && instrument->strings[0]->pitch == 64
            && instrument->strings[1]->pitch == 59
            && instrument->fretSteps == std::vector<int>{2, 4},
        "A zlib FretInstrument field failed to decode");
    expectMapping(group && group->fretInstId == 7 && group->name == "Gr",
        "A zlib FretboardGroup tuple failed to decode");
    expectMapping(style && style->showLastFret && style->fingStrShapeId == 11
            && style->openStrShapeId == 12 && style->muteStrShapeId == 13
            && style->barreShapeId == 14 && style->customShapeId == 15
            && style->defNumFrets == 4 && style->stringGap == 900
            && style->fretGap == 1404 && style->fretNumFont->fontId == 2
            && style->fretNumFont->fontSize == 9 && style->fingNumFont->fontId == 3
            && style->fingNumFont->fontSize == 5 && style->vertFingNumOff == 80
            && style->name == "St" && style->fretNumText == "fr.",
        "A zlib FretboardStyle field failed to decode");

    const std::vector<std::int16_t> diagramWords{4, 2, 5, 3, 1,
        0x2002, 1, 0x1803, static_cast<std::int16_t>(0xa001), 0,
        0x2800, 2, 0, 0, 0, 1, 0x0105, 0, 0, 0};
    const auto detailParsed = makeDetailClassContainer(
        9, 2, 0, diagramWords, ByteOrder::BigEndian, 0x0413);
    const auto detailIndex = LegacyRecordIndex::build(detailParsed);
    ImportReport detailReport;
    finale_mus_reader::PendingReferences detailPending;
    musx::factory::ConstructionContext detailConstruction;
    const finale_mus_reader::ImportContext detailContext{detailIndex, profile, noSource,
        document, reference, detailReport, detailPending, detailConstruction};
    finale_mus_reader::details::importFretboardDiagrams(detailContext);
    const auto diagram = document->getDetails()->get<FretboardDiagram>(
        musx::dom::SCORE_PARTID, 9, 2);
    expectMapping(diagram && diagram->numFrets == 4 && diagram->fretboardNum == 2
            && diagram->lock && diagram->showNum && diagram->cells.size() == 3
            && diagram->cells[0]->string == 4 && diagram->cells[0]->fret == 2
            && diagram->cells[0]->shape == FretboardDiagram::Shape::Closed
            && diagram->cells[1]->fingerNum == 5
            && diagram->cells[2]->shape == FretboardDiagram::Shape::Open
            && diagram->barres.size() == 1 && diagram->barres[0]->fret == 1
            && diagram->barres[0]->startString == 1 && diagram->barres[0]->endString == 5,
        "A zlib FretboardDiagram field or padded item array failed to decode");

    std::vector<SyntheticRow> fixedRows;
    const auto appendFixedOther = [&](std::uint16_t cmper, const char* tag,
                                      const std::vector<std::int16_t>& words) {
        for (std::size_t at = 0; at < words.size(); at += 6) {
            std::array<std::int16_t, 6> incidence{};
            std::copy_n(words.begin() + static_cast<std::ptrdiff_t>(at), 6,
                incidence.begin());
            fixedRows.push_back({cmper, tag, incidence});
        }
    };
    appendFixedOther(9, "fg", groupWords);
    appendFixedOther(7, "fI", instrumentWords);
    appendFixedOther(3, "ft", styleWords);
    const auto fixedParsed = makeContainer(fixedRows, FormatEpoch::DclLegacy);
    const auto fixedIndex = LegacyRecordIndex::build(fixedParsed);
    auto fixedSession = musx::factory::DocumentFactory::begin();
    const auto fixedDocument = fixedSession.getDocument();
    ImportReport fixedReport;
    finale_mus_reader::PendingReferences fixedPending;
    SourceProfile fixedProfile;
    fixedProfile.epoch = FormatEpoch::DclLegacy;
    fixedProfile.byteOrder = ByteOrder::BigEndian;
    musx::factory::ConstructionContext fixedConstruction;
    const finale_mus_reader::ImportContext fixedContext{fixedIndex, fixedProfile, noSource,
        fixedDocument, reference, fixedReport, fixedPending, fixedConstruction};
    finale_mus_reader::others::importFretInstruments(fixedContext);
    finale_mus_reader::others::importFretboardGroups(fixedContext);
    finale_mus_reader::others::importFretboardStyles(fixedContext);
    const auto fixedInstrument = fixedDocument->getOthers()->get<FretInstrument>(
        musx::dom::SCORE_PARTID, 7);
    const auto fixedGroup = fixedDocument->getOthers()->get<FretboardGroup>(
        musx::dom::SCORE_PARTID, 9, 0);
    const auto fixedStyle = fixedDocument->getOthers()->get<FretboardStyle>(
        musx::dom::SCORE_PARTID, 3);
    expectMapping(fixedInstrument && fixedInstrument->numFrets == 21
            && fixedInstrument->strings.size() == 2 && fixedInstrument->name == "Hi"
            && fixedGroup && fixedGroup->fretInstId == 7 && fixedGroup->name == "Gr"
            && fixedStyle && fixedStyle->stringGap == 900 && fixedStyle->name == "St"
            && fixedStyle->fretNumText == "fr.",
        "A fixed-row fret others record failed to decode");

    const auto fixedDetailParsed = makeDetailContainer(
        FormatEpoch::DclLegacy, 9, 2, diagramWords, "fb");
    const auto fixedDetailIndex = LegacyRecordIndex::build(fixedDetailParsed);
    ImportReport fixedDetailReport;
    finale_mus_reader::PendingReferences fixedDetailPending;
    musx::factory::ConstructionContext fixedDetailConstruction;
    const finale_mus_reader::ImportContext fixedDetailContext{fixedDetailIndex, fixedProfile,
        noSource, fixedDocument, reference, fixedDetailReport, fixedDetailPending,
        fixedDetailConstruction};
    finale_mus_reader::details::importFretboardDiagrams(fixedDetailContext);
    const auto fixedDiagram = fixedDocument->getDetails()->get<FretboardDiagram>(
        musx::dom::SCORE_PARTID, 9, 2);
    expectMapping(fixedDiagram && fixedDiagram->cells.size() == 3
            && fixedDiagram->barres.size() == 1 && fixedDiagram->showNum,
        "A fixed-row fretboard diagram failed to decode");

    const auto emptyIndex = LegacyRecordIndex::build(
        makeClassContainer(std::vector<SyntheticClassRow>{}, ByteOrder::LittleEndian));
    auto emptySession = musx::factory::DocumentFactory::begin();
    const auto emptyDocument = emptySession.getDocument();
    ImportReport emptyReport;
    finale_mus_reader::PendingReferences emptyPending;
    musx::factory::ConstructionContext emptyConstruction;
    const finale_mus_reader::ImportContext emptyContext{emptyIndex, profile, noSource,
        emptyDocument, reference, emptyReport, emptyPending, emptyConstruction};
    finale_mus_reader::others::importFretInstruments(emptyContext);
    finale_mus_reader::others::importFretboardGroups(emptyContext);
    finale_mus_reader::others::importFretboardStyles(emptyContext);
    finale_mus_reader::details::importFretboardDiagrams(emptyContext);
    expectMapping(emptyDocument->getOthers()->getArray<FretInstrument>(
                      musx::dom::SCORE_PARTID).empty()
            && emptyDocument->getOthers()->getArray<FretboardGroup>(
                musx::dom::SCORE_PARTID).empty()
            && emptyDocument->getOthers()->getArray<FretboardStyle>(
                musx::dom::SCORE_PARTID).empty()
            && emptyDocument->getDetails()->getArray<FretboardDiagram>(
                musx::dom::SCORE_PARTID).empty(),
        "An absent source fret record synthesized an instance");

    for (const auto emptyEpoch : {FormatEpoch::CodaBanner,
             FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        const auto emptyFixedIndex = LegacyRecordIndex::build(
            makeContainer({}, emptyEpoch));
        auto emptyFixedSession = musx::factory::DocumentFactory::begin();
        const auto emptyFixedDocument = emptyFixedSession.getDocument();
        ImportReport emptyFixedReport;
        finale_mus_reader::PendingReferences emptyFixedPending;
        SourceProfile emptyFixedProfile;
        emptyFixedProfile.epoch = emptyEpoch;
        emptyFixedProfile.byteOrder = ByteOrder::BigEndian;
        musx::factory::ConstructionContext emptyFixedConstruction;
        const finale_mus_reader::ImportContext emptyFixedContext{emptyFixedIndex,
            emptyFixedProfile, noSource, emptyFixedDocument, reference, emptyFixedReport,
            emptyFixedPending, emptyFixedConstruction};
        finale_mus_reader::others::importFretInstruments(emptyFixedContext);
        finale_mus_reader::others::importFretboardGroups(emptyFixedContext);
        finale_mus_reader::others::importFretboardStyles(emptyFixedContext);
        finale_mus_reader::details::importFretboardDiagrams(emptyFixedContext);
        expectMapping(emptyFixedDocument->getOthers()->getArray<FretInstrument>(
                          musx::dom::SCORE_PARTID).empty()
                && emptyFixedDocument->getOthers()->getArray<FretboardGroup>(
                    musx::dom::SCORE_PARTID).empty()
                && emptyFixedDocument->getOthers()->getArray<FretboardStyle>(
                    musx::dom::SCORE_PARTID).empty()
                && emptyFixedDocument->getDetails()->getArray<FretboardDiagram>(
                    musx::dom::SCORE_PARTID).empty(),
            "An empty fixed-row epoch synthesized a fret instance");
    }
}

void testFretboardGroupUnicodeLayout()
{
    using FretboardGroup = musx::dom::others::FretboardGroup;
    constexpr std::size_t tupleWords = 102;
    std::vector<std::int16_t> words(tupleWords * 2);
    words[0] = 2;
    const std::u16string firstName = u"Simple Major Triad";
    std::copy(firstName.begin(), firstName.end(), words.begin() + 6);
    words[tupleWords] = 2;
    const std::u16string secondName = u"Major   (copy)";
    std::copy(secondName.begin(), secondName.end(), words.begin() + tupleWords + 6);

    const auto index = LegacyRecordIndex::build(
        makeClassContainer(0x0094, words, ByteOrder::LittleEndian, 1));
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    ImportReport report;
    finale_mus_reader::PendingReferences pending;
    auto profile = profileFor(17);
    profile.epoch = FormatEpoch::ZlibLegacy;
    profile.byteOrder = ByteOrder::LittleEndian;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{
        index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importFretboardGroups(context);

    const auto first = document->getOthers()->get<FretboardGroup>(
        musx::dom::SCORE_PARTID, 1, 0);
    const auto second = document->getOthers()->get<FretboardGroup>(
        musx::dom::SCORE_PARTID, 1, 1);
    const auto nonexistent = document->getOthers()->get<FretboardGroup>(
        musx::dom::SCORE_PARTID, 1, 2);
    expectMapping(first && first->fretInstId == 2 && first->name == "Simple Major Triad"
            && second && second->fretInstId == 2 && second->name == "Major   (copy)"
            && !nonexistent,
        "A Finale 2012 FretboardGroup did not use its 204-byte UTF-16LE tuple");
}

} // namespace

namespace finale_mus_reader_tests {

TEST_CASE("Clef tuple decoding", "[mapping]") { testClefTupleDecoding(); }
TEST_CASE("Stem pre-Finale-3.5 units", "[mapping]") { testStemPreFinale35Units(); }
TEST_CASE("Multimeasure rest layout marker", "[mapping]") { testMmRestEarlyLayoutMarker(); }
TEST_CASE("Stem connections are source owned", "[mapping]")
{
    testStemConnectionsAreSourceOwned();
}
TEST_CASE("Stale Unicode stem record", "[mapping]") { testStemStaleUnicodeRecord(); }
TEST_CASE("Stem font reference validation", "[mapping]")
{
    testStemFontReferenceValidation();
}
TEST_CASE("Dangling font comparator requires registration", "[mapping]")
{
    testDanglingFontComparatorRequiresRegistration();
}
TEST_CASE("Detail row shape", "[mapping]") { testDetailRowShape(); }
TEST_CASE("Other rows remain searchable", "[mapping]") { testOtherRowsRemainSearchable(); }
TEST_CASE("Four-byte incidence straddling", "[mapping]")
{
    testFourByteStraddlesIncidence();
}
TEST_CASE("Class-record continuation segment", "[mapping]")
{
    testClassRecordContinuationSegment();
}
TEST_CASE("Fret classes are source owned", "[mapping]")
{
    testFretClassesAreSourceOwned();
}
TEST_CASE("Fretboard group Unicode layout", "[mapping]")
{
    testFretboardGroupUnicodeLayout();
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
/// @brief A document whose TextOptions carries the pinned baseline's line spacing.
/// @details The baseline seeds the percent spelling, which is what a source stating the
/// absolute spelling has to displace. Seeding both would let an implementation that never
/// clears the baseline pass.
musx::dom::DocumentPtr makeTextOptionsDocument()
{
    using TextOptions = musx::dom::options::TextOptions;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto options = std::make_shared<TextOptions>(document);
    options->textLineSpacingPercent = 100;
    options->tabSpaces = 4;
    options->textWordWrap = true;
    options->textExpandSingleWord = true;
    document->getOptions()->add(TextOptions::XmlNodeName, options);
    // The pinned baseline always carries all five inserts, and a source with no block of its
    // own is completed from them, so the stand-in must carry them too.
    using Insert = musx::dom::options::AccidentalInsertSymbolType;
    const std::array<std::tuple<Insert, int, int, char32_t>, 5> seeds{
        {{Insert::Sharp, 35, 34, U'#'}, {Insert::Flat, 60, 19, U'b'},
            {Insert::Natural, 50, 34, U'n'}, {Insert::DblSharp, 40, 34, U'Ü'},
            {Insert::DblFlat, 60, 19, U'º'}}};
    for (const auto& [type, tracking, shift, character] : seeds) {
        auto insert = std::make_shared<TextOptions::InsertSymbolInfo>(options);
        insert->trackingBefore = tracking;
        insert->baselineShiftPerc = shift;
        insert->symChar = character;
        auto font = std::make_shared<musx::dom::FontInfo>(document, /*sizeIsPercent*/ true);
        font->fontSize = 100;
        insert->symFont = std::move(font);
        options->symbolInserts[type] = std::move(insert);
    }
    return std::move(session).finish();
}

// Selectors 5 and 13 are carried by every era; 81, 82 and 83 arrive with Finale 97. The reader
// decides that from the records present rather than from the version, so a document that
// predates them must recover the two old fields and leave the other eleven at the baseline.
// The two enums are checked here because Finale orders its alignment lists first, opposite,
// centre while musxdom puts centre second.
void testTextOptionsScalars()
{
    using TextOptions = musx::dom::options::TextOptions;
    const auto runImport = [](const std::vector<SyntheticRow>& rows,
                               const SourceProfile& profile, ImportReport& report) {
        const auto document = makeTextOptionsDocument();
        const auto reference = makeTextOptionsDocument();
        const auto index = LegacyRecordIndex::build(makeContainer(rows));
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importTextOptions(context);
        return document->getOptions()->get<TextOptions>();
    };

    // A Coda-banner document: the two old selectors and nothing else.
    {
        ImportReport report;
        auto coda = profileFor(2, 6);
        coda.epoch = FormatEpoch::CodaBanner;
        coda.version.reset();
        const auto options = runImport(
            {{GLOBALS_CMPER, "05", {0, 0, 0, 0, 1, 2}}, {GLOBALS_CMPER, "13", {7, 0, 0, 0, 0, 0}}},
            coda, report);
        expectMapping(options->showTimeSeconds && options->dateFormat == musx::dom::DateFormat::Abbrev
                && options->tabSpaces == 7,
            "The Coda-banner era did not recover the date stamp and tab spacing");
        expectMapping(options->textLineSpacingPercent.has_value()
                && options->textLineSpacingPercent.value() == 100
                && !options->textLineSpacingEvpu.has_value(),
            "A document with no selector 82 lost the baseline line spacing");
        expectMapping(options->textJustify == TextOptions::TextJustify::Left
                && options->textVertAlign == TextOptions::VerticalAlignment::Top,
            "A document with no selector 83 did not keep the baseline alignment");
    }

    // Finale 97 onward, with the full set. Selector 82 states a percent, so the percent member
    // is engaged and the absolute one stays empty.
    {
        ImportReport report;
        const auto options = runImport(
            {
                {GLOBALS_CMPER, "05", {0, 0, 0, 0, 1, 1}},
                {GLOBALS_CMPER, "13", {7, 0, 0, 0, 0, 0}},
                {GLOBALS_CMPER, "81", {-1, -6, 0, 2016, -1, -3168}},
                {GLOBALS_CMPER, "82", {89, 1, 0, 11, 4, 1}},
                {GLOBALS_CMPER, "83", {1, 1, 0, 1, 0, 0}},
            },
            profileFor(3, 8), report);
        expectMapping(options->dateFormat == musx::dom::DateFormat::Long && options->tabSpaces == 7,
            "The Finale 97 era did not recover the date stamp and tab spacing");
        // Three 32-bit values in the framework's high-word-first order, two of them negative.
        expectMapping(options->textTracking == -6 && options->textBaselineShift == 2016
                && options->textSuperscript == -3168,
            "The 32-bit text metrics were not assembled high word first");
        expectMapping(options->textLineSpacingPercent.has_value()
                && options->textLineSpacingPercent.value() == 89
                && !options->textLineSpacingEvpu.has_value(),
            "A stated percent line spacing did not engage the percent member alone");
        expectMapping(!options->textWordWrap && options->textPageOffset == 11
                && options->textExpandSingleWord,
            "Selector 82 was not read from its own slots");
        // Stored 4 is ForcedFull in both spellings; stored 1 is Right, which musxdom numbers 1
        // as well. The exchange is proved by the vertical alignment below.
        expectMapping(options->textJustify == TextOptions::TextJustify::ForcedFull
                && options->textHorzAlign == TextOptions::HorizontalAlignment::Right
                && options->textIsEdgeAligned,
            "Selector 83 or the justification was not read from its own slots");
        expectMapping(options->textVertAlign == TextOptions::VerticalAlignment::Bottom,
            "A stored vertical alignment of 1 did not become Bottom");
    }

    // The same record with the two exchanged enum values, which is what separates Finale's
    // order from musxdom's: a stored 2 is centre in both lists.
    {
        ImportReport report;
        const auto options = runImport(
            {
                {GLOBALS_CMPER, "82", {100, 1, 1, 0, 2, 1}},
                {GLOBALS_CMPER, "83", {0, 2, 0, 0, 0, 0}},
            },
            profileFor(3, 8), report);
        expectMapping(options->textJustify == TextOptions::TextJustify::Center,
            "A stored justification of 2 did not become Center");
        expectMapping(options->textVertAlign == TextOptions::VerticalAlignment::Center,
            "A stored vertical alignment of 2 did not become Center");
    }

    // Selector 82 word 1 clear: the same word 0 is an absolute distance, and the baseline's
    // percent must not survive beside it. musxdom's own integrity check rejects both engaged.
    {
        ImportReport report;
        const auto options = runImport(
            {{GLOBALS_CMPER, "82", {72, 0, 1, 0, 0, 1}}}, profileFor(3, 8), report);
        expectMapping(options->textLineSpacingEvpu.has_value()
                && options->textLineSpacingEvpu.value() == 72
                && !options->textLineSpacingPercent.has_value(),
            "A stated absolute line spacing did not displace the baseline percent");
        expectMapping(
            field(report, "options.textOptions.textLineSpacingEvpu").origin
                == ValueOrigin::LegacyMus,
            "The absolute line spacing was not reported as recovered");
    }

    // The zlib epoch reaches the same words through the numericGlobalClass rule, addressed by
    // byte offset. Both byte orders are exercised, because the 32-bit rule is one rule for
    // both: two 16-bit words with the high word first, each word in the container's order.
    const auto runClassImport = [](const finale_mus_reader::container::ParsedContainer& parsed,
                                    const SourceProfile& profile, ImportReport& report) {
        const auto document = makeTextOptionsDocument();
        const auto reference = makeTextOptionsDocument();
        const auto index = LegacyRecordIndex::build(parsed);
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importTextOptions(context);
        return document->getOptions()->get<TextOptions>();
    };

    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        auto profile = profileFor(16, 0);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = byteOrder;

        ImportReport layoutReport;
        const auto layout = runClassImport(
            makeClassContainer(finale_mus_reader::numericGlobalClass(82), {137, 0, 0, 77, 3, 0}, byteOrder),
            profile, layoutReport);
        expectMapping(layout->textLineSpacingEvpu.has_value()
                && layout->textLineSpacingEvpu.value() == 137
                && !layout->textLineSpacingPercent.has_value(),
            "The zlib epoch did not route line spacing by word 1");
        expectMapping(layout->textJustify == TextOptions::TextJustify::Full
                && layout->textPageOffset == 77 && !layout->textExpandSingleWord
                && !layout->textWordWrap,
            "The zlib epoch did not read selector 82 from its own offsets");

        ImportReport metricsReport;
        const auto metrics = runClassImport(
            makeClassContainer(finale_mus_reader::numericGlobalClass(81), {-1, -6, 0, 2016, -1, -3168}, byteOrder),
            profile, metricsReport);
        expectMapping(metrics->textTracking == -6 && metrics->textBaselineShift == 2016
                && metrics->textSuperscript == -3168,
            "The zlib epoch did not assemble the 32-bit metrics high word first");
    }

}

// The five accidental inserts are a direct block with three physical layouts. Each is built
// here from the bytes a real fixture of its era carries, so a layout that regressed would have
// to produce the same five characters from a different stride to pass.
void testTextOptionsSymbolInserts()
{
    using TextOptions = musx::dom::options::TextOptions;
    using Insert = musx::dom::options::AccidentalInsertSymbolType;
    const auto runImport = [](const finale_mus_reader::container::ParsedContainer& parsed,
                               const SourceProfile& profile, ImportReport& report) {
        const auto document = makeTextOptionsDocument();
        const auto reference = makeTextOptionsDocument();
        const auto index = LegacyRecordIndex::build(parsed);
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importTextOptions(context);
        return document->getOptions()->get<TextOptions>();
    };
    // Every era stores the same five characters, which is what makes a layout error visible.
    const auto expectDefaults = [](const auto& options, const std::string& what) {
        const std::array<std::pair<Insert, char32_t>, 5> chars{
            {{Insert::Sharp, U'#'}, {Insert::Flat, U'b'}, {Insert::Natural, U'n'},
                {Insert::DblSharp, U'Ü'}, {Insert::DblFlat, U'º'}}};
        for (const auto& [type, expected] : chars) {
            const auto found = options->symbolInserts.find(type);
            expectMapping(found != options->symbolInserts.end() && found->second
                    && found->second->symChar == expected,
                what + " did not recover the stored insert characters");
        }
        expectMapping(options->symbolInserts.at(Insert::Sharp)->trackingBefore == 35
                && options->symbolInserts.at(Insert::Flat)->trackingBefore == 50
                && options->symbolInserts.at(Insert::Natural)->trackingBefore == 0
                && options->symbolInserts.at(Insert::DblSharp)->trackingBefore == 40
                && options->symbolInserts.at(Insert::DblFlat)->trackingBefore == 60,
            what + " did not recover the stored tracking");
        expectMapping(options->symbolInserts.at(Insert::Sharp)->baselineShiftPerc == 34
                && options->symbolInserts.at(Insert::Flat)->baselineShiftPerc == 19,
            what + " did not recover the stored baseline shift");
        expectMapping(options->symbolInserts.at(Insert::Sharp)->symFont
                && options->symbolInserts.at(Insert::Sharp)->symFont->fontSize == 100,
            what + " did not recover the stored font size");
    };

    // Finale 3.7-2000: a 17-byte element with a one-byte character, read little-endian on a
    // big-endian file. These are the words a Finale 97 fixture carries.
    {
        ImportReport report;
        const std::vector<SyntheticRow> rows{
            {GLOBALS_CMPER, "78", {35, 0, 0, 0, 34, 0}},
            {GLOBALS_CMPER, "78", {100, 0, 12835, 0, 0, 0}},
            {GLOBALS_CMPER, "78", {4864, 0, 25600, 0, 25088, 0}},
            {GLOBALS_CMPER, "78", {0, 0, 0, 34, 0, 100}},
            {GLOBALS_CMPER, "78", {0, 10350, 0, 0, 0, 8704}},
            {GLOBALS_CMPER, "78", {0, 25600, 0, -9216, 60, 0}},
            {GLOBALS_CMPER, "78", {0, 0, 19, 0, 100, 0}},
            {GLOBALS_CMPER, "78", {186, 0, 0, 0, 0, 0}},
        };
        const auto options = runImport(makeContainer(rows), profileFor(3, 8), report);
        expectDefaults(options, "The 17-byte insert layout");
    }

    // Finale 2001-2010: an 18-byte element, container order, the character in a whole word of
    // which only the low byte counts. The double sharp and double flat are stored
    // sign-extended here, as four Finale 2006 fixtures do.
    {
        ImportReport report;
        const std::vector<SyntheticRow> rows{
            {GLOBALS_CMPER, "78", {0, 35, 0, 0, 34, 0}},
            {GLOBALS_CMPER, "78", {100, 0, 35, 0, 50, 0}},
            {GLOBALS_CMPER, "78", {0, 19, 0, 100, 0, 98}},
            {GLOBALS_CMPER, "78", {0, 0, 0, 0, 34, 0}},
            {GLOBALS_CMPER, "78", {100, 0, 110, 0, 40, 0}},
            {GLOBALS_CMPER, "78", {0, 34, 0, 100, 0, -36}},
            {GLOBALS_CMPER, "78", {0, 60, 0, 0, 19, 0}},
            {GLOBALS_CMPER, "78", {100, 0, -70, 0, 0, 0}},
        };
        auto profile = profileFor(12, 0);
        profile.epoch = FormatEpoch::DclLegacy;
        const auto options = runImport(makeContainer(rows, FormatEpoch::DclLegacy), profile, report);
        expectDefaults(options, "The 18-byte insert layout");
    }

    // Finale 2012: a 20-byte element with the character widened to a long. Each element is ten
    // words: the two trackings high word first, then the shift, font tuple and character.
    {
        std::vector<std::int16_t> words;
        const auto element = [&words](std::int16_t tb, std::int16_t bsp, std::int16_t chr) {
            for (const std::int16_t value : {std::int16_t(0), tb, std::int16_t(0),
                     std::int16_t(0), bsp, std::int16_t(0), std::int16_t(100),
                     std::int16_t(0), chr, std::int16_t(0)}) {
                words.push_back(value);
            }
        };
        element(35, 34, 35);
        element(50, 19, 98);
        element(0, 34, 110);
        element(40, 34, 220);
        element(60, 19, 186);
        words.insert(words.end(), 4, 0);

        ImportReport report;
        auto profile = profileFor(16, 0);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::LittleEndian;
        const auto options = runImport(
            makeClassContainer(finale_mus_reader::numericGlobalClass(78), words,
                ByteOrder::LittleEndian),
            profile, report);
        expectDefaults(options, "The 20-byte insert layout");

        // A character above the basic multilingual plane is the only value that can tell the
        // two candidate word orders apart, and it says the character is a plain little-endian
        // long: U+26469 is stored low word first, where the high-word-first order the two
        // trackings use would give 0x64690002 and no codepoint at all. Taken from
        // tests/evidence/F2012/F2012-dblsharp-insert-outside-BMP.
        auto astral = words;
        const std::size_t dblSharpChar = 3 * 10 + 8;
        astral[dblSharpChar] = static_cast<std::int16_t>(0x6469);
        astral[dblSharpChar + 1] = 2;
        ImportReport astralReport;
        const auto withAstral = runImport(
            makeClassContainer(finale_mus_reader::numericGlobalClass(78), astral,
                ByteOrder::LittleEndian),
            profile, astralReport);
        expectMapping(withAstral->symbolInserts.at(Insert::DblSharp)->symChar == U'\U00026469',
            "A symbol character above the basic multilingual plane was not read low word first");
    }

    // A source with no insert block at all -- every Coda-banner and Finale 3.0-3.5 document.
    // All five come from the pinned baseline and must say so, and the font must be cloned into
    // this document rather than shared with the reference.
    {
        ImportReport report;
        auto coda = profileFor(2, 6);
        coda.epoch = FormatEpoch::CodaBanner;
        coda.version.reset();
        const auto options = runImport(
            makeContainer({{GLOBALS_CMPER, "13", {7, 0, 0, 0, 0, 0}}}, FormatEpoch::CodaBanner),
            coda, report);
        expectMapping(options->symbolInserts.size() == 5,
            "A source with no insert block did not receive the baseline's five inserts");
        expectMapping(options->symbolInserts.at(Insert::Sharp)->symFont != nullptr,
            "A completed insert has no font");
        expectMapping(
            field(report, "options.textOptions.symbolInserts[sharp].symChar").origin
                == ValueOrigin::Finale27Default,
            "A completed insert was not reported as a synthesized default");
    }
}

musx::dom::DocumentPtr makeRepeatOptionsDocument()
{
    using Repeat = musx::dom::options::RepeatOptions;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto options = std::make_shared<Repeat>(document);
    options->maxPasses = 90;
    options->addPeriod = true;
    options->thickLineWidth = 91;
    options->thinLineWidth = 92;
    options->lineSpace = 93;
    options->backToBackStyle = Repeat::BackToBackStyle::Thin;
    options->forwardDotHPos = 94;
    options->backwardDotHPos = 95;
    options->upperDotVPos = 96;
    options->lowerDotVPos = 97;
    options->wingStyle = Repeat::WingStyle::None;
    options->afterClefSpace = 98;
    options->afterKeySpace = 99;
    options->afterTimeSpace = 100;
    options->bracketHeight = 101;
    options->bracketHookLen = 102;
    options->bracketLineWidth = 103;
    options->bracketStartInset = 104;
    options->bracketEndInset = 105;
    options->bracketTextHPos = 106;
    options->bracketTextVPos = 107;
    options->bracketEndHookLen = 108;
    options->bracketEndAnchorThinLine = true;
    options->showOnTopStaffOnly = true;
    options->showOnStaffListNumber = 9;
    document->getOptions()->add(Repeat::XmlNodeName, options);
    return std::move(session).finish();
}

void testRepeatOptionsAcrossEpochs()
{
    using Repeat = musx::dom::options::RepeatOptions;
    const std::vector<SyntheticRow> rows{
        {GLOBALS_CMPER, "05", {0, 0, 0, 14, 0, 0}},
        {GLOBALS_CMPER, "20", {0, 0, 0, 17, 0, 0}},
        {GLOBALS_CMPER, "69", {0, 1, 0, 0, 0, 0}},
        {GLOBALS_CMPER, "70", {111, 22, 333, 2, 44, 55}},
        {GLOBALS_CMPER, "71", {-6, -7, 3, 8, 9, 10}},
        {GLOBALS_CMPER, "72", {11, 12, 13, 14, 15, 16}},
        {GLOBALS_CMPER, "76", {0, 0, 18, 0, 0, 0}},
    };
    const auto runImport = [](const finale_mus_reader::container::ParsedContainer& parsed,
                               const SourceProfile& profile, ImportReport& report) {
        const auto document = makeRepeatOptionsDocument();
        const auto reference = makeRepeatOptionsDocument();
        const auto index = LegacyRecordIndex::build(parsed);
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importRepeatOptions(context);
        return document->getOptions()->get<Repeat>();
    };
    const auto expectRecovered = [](const auto& options, const ImportReport& report,
                                     const std::string& epoch) {
        expectMapping(options->maxPasses == 17 && options->addPeriod,
            epoch + " did not recover the repeat pass settings");
        expectMapping(options->thickLineWidth == 111 && options->thinLineWidth == 22
                && options->lineSpace == 333
                && options->backToBackStyle == Repeat::BackToBackStyle::Thick,
            epoch + " did not recover the repeat line settings");
        expectMapping(options->forwardDotHPos == 44 && options->backwardDotHPos == 55
                && options->upperDotVPos == -6 && options->lowerDotVPos == -7,
            epoch + " did not recover the repeat dot positions");
        expectMapping(options->wingStyle == Repeat::WingStyle::Curved
                && options->afterClefSpace == 8 && options->afterKeySpace == 9
                && options->afterTimeSpace == 10,
            epoch + " did not recover the repeat wing and spacing settings");
        expectMapping(options->bracketHeight == 14 && options->bracketHookLen == 11
                && options->bracketLineWidth == 12 && options->bracketStartInset == 13
                && options->bracketEndInset == 14 && options->bracketTextHPos == 15
                && options->bracketTextVPos == 16 && options->bracketEndHookLen == 18,
            epoch + " did not recover the ending bracket settings");
        expectMapping(!options->bracketEndAnchorThinLine && options->showOnTopStaffOnly
                && options->showOnStaffListNumber == 9,
            epoch + " did not assert the legacy anchor behavior or disturbed a staff-list option");
        expectMapping(field(report, "options.repeatOptions.maxPasses").origin
                    == ValueOrigin::LegacyMus
                && field(report, "options.repeatOptions.bracketEndAnchorThinLine").origin
                    == ValueOrigin::LegacyBehavior,
            epoch + " reported an incorrect repeat-option origin");
    };

    for (const auto epoch : {FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        auto profile = profileFor(epoch == FormatEpoch::DclLegacy ? 12 : 3, 0);
        profile.epoch = epoch;
        ImportReport report;
        expectRecovered(runImport(makeContainer(rows, epoch), profile, report), report,
            epoch == FormatEpoch::DclLegacy ? "The DCL epoch" : "The uncompressed epoch");
    }

    auto sentinelRows = rows;
    sentinelRows[1].words[3] = 0;
    ImportReport sentinelReport;
    expectMapping(runImport(makeContainer(sentinelRows), profileFor(3, 7), sentinelReport)
            ->maxPasses == 20,
        "The older zero maximum-pass sentinel did not retain its twenty-pass behavior");

    std::vector<SyntheticClassRow> classRows;
    for (const auto& row : rows) {
        const auto selector = static_cast<std::uint16_t>(
            (row.tag[0] - '0') * 10 + row.tag[1] - '0');
        classRows.push_back({finale_mus_reader::numericGlobalClass(selector),
            std::vector<std::int16_t>(row.words.begin(), row.words.end())});
    }
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        auto profile = profileFor(16, 0);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = byteOrder;
        ImportReport report;
        expectRecovered(runImport(makeClassContainer(classRows, byteOrder), profile, report),
            report, byteOrder == ByteOrder::BigEndian
                ? "The big-endian zlib epoch" : "The little-endian zlib epoch");
    }

    auto coda = profileFor(2, 6);
    coda.epoch = FormatEpoch::CodaBanner;
    coda.version.reset();
    ImportReport codaReport;
    const auto codaOptions = runImport(
        makeContainer(rows, FormatEpoch::CodaBanner), coda, codaReport);
    expectMapping(codaOptions->maxPasses == 90 && codaOptions->bracketHeight == 101
            && !codaOptions->addPeriod && codaOptions->thinLineWidth == 224
            && codaOptions->upperDotVPos == 0 && codaOptions->lowerDotVPos == 0
            && codaOptions->bracketLineWidth == 224
            && !codaOptions->bracketEndAnchorThinLine
            && codaOptions->showOnStaffListNumber == 9,
        "The Coda epoch did not apply the pre-layout RepeatOptions behavior");
    expectMapping(field(codaReport, "options.repeatOptions.maxPasses").origin
                == ValueOrigin::Finale27Default
            && field(codaReport, "options.repeatOptions.addPeriod").origin
                == ValueOrigin::LegacyBehavior
            && field(codaReport, "options.repeatOptions.bracketEndAnchorThinLine").origin
                == ValueOrigin::LegacyBehavior,
        "The Coda epoch reported an incorrect RepeatOptions origin");

    auto earlyUncompressed = profileFor(3, 0);
    earlyUncompressed.epoch = FormatEpoch::UncompressedLegacy;
    auto rowsWithoutLayoutMarker = rows;
    rowsWithoutLayoutMarker.erase(rowsWithoutLayoutMarker.begin() + 5);
    ImportReport earlyReport;
    const auto earlyOptions = runImport(
        makeContainer(rowsWithoutLayoutMarker, FormatEpoch::UncompressedLegacy),
        earlyUncompressed, earlyReport);
    expectMapping(earlyOptions->maxPasses == 90 && earlyOptions->bracketHeight == 101
            && earlyOptions->thickLineWidth == 91
            && !earlyOptions->addPeriod && earlyOptions->thinLineWidth == 224
            && earlyOptions->upperDotVPos == 0 && earlyOptions->lowerDotVPos == 0
            && earlyOptions->bracketLineWidth == 224
            && !earlyOptions->bracketEndAnchorThinLine,
        "An uncompressed file without the family did not apply the pre-layout behavior");
    expectMapping(field(earlyReport, "options.repeatOptions.maxPasses").origin
                == ValueOrigin::Finale27Default
            && field(earlyReport, "options.repeatOptions.thinLineWidth").origin
                == ValueOrigin::LegacyBehavior,
        "An uncompressed file without the family reported an incorrect RepeatOptions origin");
}

musx::dom::DocumentPtr makeSmartShapeOptionsDocument()
{
    using SmartShape = musx::dom::options::SmartShapeOptions;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto options = std::make_shared<SmartShape>(document);
    options->shortHairpinOpeningWidth = 901;
    options->crescHeight = 902;
    options->maximumShortHairpinLength = 927;
    options->crescLineWidth = 903;
    options->hookLength = 904;
    options->smartLineWidth = 905;
    options->showOctavaAsText = false;
    options->smartDashOn = 906;
    options->smartDashOff = 907;
    options->crescHorizontal = false;
    options->slurThicknessCp1X = 908;
    options->slurThicknessCp1Y = 909;
    options->slurThicknessCp2X = 910;
    options->slurThicknessCp2Y = 911;
    options->slurAvoidAccidentals = false;
    options->slurAvoidStaffLinesAmt = 912;
    options->maxSlurStretch = 913;
    options->maxSlurLift = 914;
    options->slurSymmetry = 915;
    options->useEngraverSlurs = false;
    options->slurLeftBreakHorzAdj = 916;
    options->slurRightBreakHorzAdj = 917;
    options->slurBreakVertAdj = 918;
    options->slurAvoidStaffLines = false;
    options->slurPadding = 919;
    options->maxSlurAngle = 919;
    options->slurAcciPadding = 920;
    options->slurDoStretchFirst = true;
    options->slurStretchByPercent = false;
    options->maxSlurStretchPercent = 921;
    options->articAvoidSlurAmt = 928;
    options->ssLineStyleCmpCustom = 922;
    options->ssLineStyleCmpGlissando = 923;
    options->ssLineStyleCmpTabSlide = 924;
    options->ssLineStyleCmpTabBendCurve = 925;
    options->smartSlurTipWidth = 9.26;
    options->guitarBendUseParens = false;
    options->guitarBendHideBendTo = true;
    options->guitarBendGenText = false;
    options->guitarBendUseFull = true;
    constexpr std::array controlTypes{
        SmartShape::SlurControlStyleType::ShortSpan,
        SmartShape::SlurControlStyleType::MediumSpan,
        SmartShape::SlurControlStyleType::LongSpan,
        SmartShape::SlurControlStyleType::ExtraLongSpan,
    };
    for (std::size_t index = 0; index < controlTypes.size(); ++index) {
        auto control = std::make_shared<SmartShape::ControlStyle>();
        control->span = 927 + static_cast<int>(index) * 3;
        control->inset = 928 + static_cast<int>(index) * 3;
        control->height = 929 + static_cast<int>(index) * 3;
        options->slurControlStyles.emplace(controlTypes[index], std::move(control));
    }
    const auto addConnections = []<typename Map>(Map& map, std::size_t count) {
        for (std::size_t index = 0; index < count; ++index) {
            auto connection = std::make_shared<SmartShape::ConnectionStyle>();
            connection->connectIndex = SmartShape::ConnectionIndex::HeadRightTop;
            connection->xOffset = 929 + static_cast<int>(index);
            connection->yOffset = 930 + static_cast<int>(index);
            map.emplace(static_cast<typename Map::key_type>(index), std::move(connection));
        }
    };
    addConnections(options->slurConnectStyles,
        static_cast<std::size_t>(SmartShape::SlurConnectStyleType::UnderTabNumEnd) + 1);
    addConnections(options->tabSlideConnectStyles,
        static_cast<std::size_t>(SmartShape::TabSlideConnectStyleType::SameLevelPitchSameEnd) + 1);
    addConnections(options->glissandoConnectStyles,
        static_cast<std::size_t>(SmartShape::GlissandoConnectStyleType::DefaultEnd) + 1);
    addConnections(options->bendCurveConnectStyles,
        static_cast<std::size_t>(SmartShape::BendCurveConnectStyleType::StaffFromTopEndOffset) + 1);
    document->getOptions()->add(SmartShape::XmlNodeName, options);
    return std::move(session).finish();
}

void testSmartShapeOptionsAcrossEpochs()
{
    using SmartShape = musx::dom::options::SmartShapeOptions;
    const auto connectionWords = [](std::size_t count, bool appendTerminal = false) {
        std::vector<std::int16_t> words;
        words.reserve((count + (appendTerminal ? 1 : 0)) * 3);
        for (std::size_t index = 0; index < count; ++index) {
            words.push_back(static_cast<std::int16_t>(index % 14));
            words.push_back(static_cast<std::int16_t>(1000 + index));
            words.push_back(static_cast<std::int16_t>(-1000 - index));
        }
        if (appendTerminal) {
            words.insert(words.end(), 3, 0);
        }
        return words;
    };
    const auto slurConnectionWords = connectionWords(29, true);
    const auto tabSlideConnectionWords = connectionWords(18);
    const auto glissandoConnectionWords = connectionWords(2);
    const auto bendCurveConnectionWords = connectionWords(8);

    std::vector<SyntheticRow> rows{
        {GLOBALS_CMPER, "50", {10, 11, 12, 13, 2, 9}},
        {GLOBALS_CMPER, "51", {0, 1536, 0, 2048, 8500, 1}},
        {GLOBALS_CMPER, "52", {36, 614, 16, 288, 512, 60}},
        {GLOBALS_CMPER, "52", {864, 410, 72, 1152, 369, 80}},
        {GLOBALS_CMPER, "53", {21, 22, 23, 1, 24, 4500}},
        {GLOBALS_CMPER, "53", {3, 1, 1, 1500, 0, 0}},
        {GLOBALS_CMPER, "92", {4, 5, 6, 7, 0, 0}},
        {GLOBALS_CMPER, "93", {0, 10000, 0, 0, 0, 0}},
        {GLOBALS_CMPER, "97", {0, 0, 0, 0, 0, 0}},
        {GLOBALS_CMPER, "97", {1, 0, 1, 0, 0, 0}},
        {11, "FI", {40, 224, 12, 0, 225, 1}},
        {12, "FI", {0, 18, 0, 19, 1, 0}},
    };
    const auto appendFixedFamily = [](std::vector<SyntheticRow>& destination,
                                      const char* tag,
                                      const std::vector<std::int16_t>& words) {
        expectMapping(words.size() % 6 == 0,
            "A synthetic fixed-row family did not end on an incidence boundary");
        for (std::size_t first = 0; first < words.size(); first += 6) {
            std::array<std::int16_t, 6> incidence{};
            std::ranges::copy(words.begin() + static_cast<std::ptrdiff_t>(first),
                words.begin() + static_cast<std::ptrdiff_t>(first + 6), incidence.begin());
            destination.push_back({GLOBALS_CMPER, tag, incidence});
        }
    };
    appendFixedFamily(rows, "26", slurConnectionWords);
    appendFixedFamily(rows, "90", tabSlideConnectionWords);
    appendFixedFamily(rows, "91", glissandoConnectionWords);
    appendFixedFamily(rows, "98", bendCurveConnectionWords);
    const auto runImport = [](const finale_mus_reader::container::ParsedContainer& parsed,
                               const SourceProfile& profile, ImportReport& report) {
        const auto document = makeSmartShapeOptionsDocument();
        const auto reference = makeSmartShapeOptionsDocument();
        const auto index = LegacyRecordIndex::build(parsed);
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importSmartShapeOptions(context);
        return document->getOptions()->get<SmartShape>();
    };
    const auto expectRecovered = [](const auto& options, const ImportReport& report,
                                     const std::string& epoch) {
        expectMapping(options->crescHeight == 40 && options->shortHairpinOpeningWidth == 40
                && options->crescLineWidth == 224 && options->hookLength == 12
                && options->smartLineWidth == 225 && options->showOctavaAsText
                && options->smartDashOn == 18 && options->smartDashOff == 19
                && options->crescHorizontal,
            epoch + " did not recover the Smart Shape line settings");
        expectMapping(options->slurThicknessCp1X == 10 && options->slurThicknessCp1Y == 11
                && options->slurThicknessCp2X == 12 && options->slurThicknessCp2Y == 13
                && options->slurAvoidAccidentals
                && options->slurAvoidStaffLinesAmt == 8,
            epoch + " did not recover the Smart Shape slur thickness settings");
        expectMapping(options->maxSlurStretch == 1536 && options->maxSlurLift == 2048
                && options->slurSymmetry == 8500 && options->useEngraverSlurs,
            epoch + " did not recover the engraver-slur settings");
        expectMapping(options->slurLeftBreakHorzAdj == 21
                && options->slurRightBreakHorzAdj == 22 && options->slurBreakVertAdj == 23
                && options->slurAvoidStaffLines && options->slurPadding == 24
                && options->maxSlurAngle == 4500
                && options->slurAcciPadding == 3 && options->slurDoStretchFirst
                && options->slurStretchByPercent
                && options->maxSlurStretchPercent == 1500,
            epoch + " did not recover the slur adjustment settings");
        expectMapping(options->ssLineStyleCmpCustom == 4
                && options->ssLineStyleCmpGlissando == 5
                && options->ssLineStyleCmpTabSlide == 6
                && options->ssLineStyleCmpTabBendCurve == 7
                && options->smartSlurTipWidth == 1.0,
            epoch + " did not recover the Smart Shape line references");
        expectMapping(options->guitarBendUseParens && !options->guitarBendHideBendTo
                && options->guitarBendGenText && !options->guitarBendUseFull,
            epoch + " did not recover the guitar-bend settings");
        expectMapping(options->slurControlStyles.size() == 4,
            epoch + " did not replace the seeded slur contours");
        const auto extraLong = options->slurControlStyles.at(
            SmartShape::SlurControlStyleType::ExtraLongSpan);
        expectMapping(extraLong->span == 1152 && extraLong->inset == 369
                && extraLong->height == 80,
            epoch + " did not recover the extra-long slur contour");
        const auto lastSlur = options->slurConnectStyles.at(
            SmartShape::SlurConnectStyleType::UnderTabNumEnd);
        const auto lastTabSlide = options->tabSlideConnectStyles.at(
            SmartShape::TabSlideConnectStyleType::SameLevelPitchSameEnd);
        const auto glissandoEnd = options->glissandoConnectStyles.at(
            SmartShape::GlissandoConnectStyleType::DefaultEnd);
        const auto lastBend = options->bendCurveConnectStyles.at(
            SmartShape::BendCurveConnectStyleType::StaffFromTopEndOffset);
        expectMapping(options->slurConnectStyles.size() == 29
                && lastSlur->connectIndex == SmartShape::ConnectionIndex::HeadLeftTop
                && lastSlur->xOffset == 1028 && lastSlur->yOffset == -1028
                && options->tabSlideConnectStyles.size() == 18
                && lastTabSlide->connectIndex == SmartShape::ConnectionIndex::HeadLeftBottom
                && lastTabSlide->xOffset == 1017
                && options->glissandoConnectStyles.size() == 2
                && glissandoEnd->connectIndex == SmartShape::ConnectionIndex::HeadRightTop
                && options->bendCurveConnectStyles.size() == 8
                && lastBend->connectIndex == SmartShape::ConnectionIndex::StemLeftBottom,
            epoch + " did not recover every Smart Shape connection style");
        expectMapping(field(report, "options.smartShapeOptions.crescHeight").origin
                    == ValueOrigin::LegacyMus
                && field(report,
                       "options.smartShapeOptions.shortHairpinOpeningWidth").origin
                    == ValueOrigin::LegacyBehavior
                && field(report,
                       "options.smartShapeOptions.slurControlStyles[3].height").rawValue
                    == 80,
            epoch + " reported incorrect Smart Shape origins");
        expectMapping(field(report,
                          "options.smartShapeOptions.maximumShortHairpinLength").origin
                    == ValueOrigin::MusxOnly
                && field(report,
                       "options.smartShapeOptions.articAvoidSlurAmt").origin
                    == ValueOrigin::MusxOnly,
            epoch + " reported incorrect unresolved Smart Shape scalar origins");
        expectMapping(field(report,
                          "options.smartShapeOptions.slurConnectStyles[28].xOffset").origin
                    == ValueOrigin::LegacyMus
                && field(report,
                       "options.smartShapeOptions.tabSlideConnectStyles[17].yOffset").origin
                    == ValueOrigin::LegacyMus
                && field(report,
                       "options.smartShapeOptions.glissandoConnectStyles[1].connectIndex").origin
                    == ValueOrigin::LegacyMus
                && field(report,
                       "options.smartShapeOptions.bendCurveConnectStyles[7].xOffset").origin
                    == ValueOrigin::LegacyMus,
            epoch + " reported incorrect Smart Shape connection-style origins");
    };

    for (const auto epoch : {FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        auto profile = profileFor(epoch == FormatEpoch::DclLegacy ? 12 : 4, 0);
        profile.epoch = epoch;
        ImportReport report;
        expectRecovered(runImport(makeContainer(rows, epoch), profile, report), report,
            epoch == FormatEpoch::DclLegacy ? "The DCL epoch" : "The uncompressed epoch");
    }

    struct FigureSemanticsCase
    {
        FormatEpoch epoch;
        std::optional<SourceVersion> version;
        int expectedCrescLineWidth;
        ValueOrigin crescLineWidthOrigin;
        std::int64_t crescLineWidthRawValue;
        int expectedHookLength;
        ValueOrigin hookLengthOrigin;
    };
    const std::array figureCases{
        FigureSemanticsCase{FormatEpoch::UncompressedLegacy,
            profileFor(3, 6).version, 225, ValueOrigin::LegacyBehavior, 225,
            904, ValueOrigin::Finale27Default},
        FigureSemanticsCase{FormatEpoch::UncompressedLegacy,
            profileFor(3, 7).version, 224, ValueOrigin::LegacyMus, 224,
            12, ValueOrigin::LegacyMus},
        FigureSemanticsCase{FormatEpoch::UncompressedLegacy,
            std::nullopt, 224, ValueOrigin::LegacyMus, 224,
            12, ValueOrigin::LegacyMus},
        FigureSemanticsCase{FormatEpoch::DclLegacy,
            profileFor(3, 6).version, 224, ValueOrigin::LegacyMus, 224,
            12, ValueOrigin::LegacyMus},
    };
    for (const auto& test : figureCases) {
        auto profile = profileFor(4, 0);
        profile.epoch = test.epoch;
        profile.version = test.version;
        ImportReport report;
        const auto options = runImport(makeContainer(rows, test.epoch), profile, report);
        const auto& recoveredWidth = field(
            report, "options.smartShapeOptions.crescLineWidth");
        const auto& recoveredHook = field(
            report, "options.smartShapeOptions.hookLength");
        expectMapping(options->crescLineWidth == test.expectedCrescLineWidth
                && recoveredWidth.origin == test.crescLineWidthOrigin
                && recoveredWidth.rawValue == test.crescLineWidthRawValue
                && options->hookLength == test.expectedHookLength
                && recoveredHook.origin == test.hookLengthOrigin,
            "The pre-Finale-3.7 figure gate crossed an epoch or version boundary");
    }

    auto zeroAvoidanceRows = rows;
    zeroAvoidanceRows.front().words[5] = 0;
    auto zeroAvoidanceProfile = profileFor(7, 0);
    zeroAvoidanceProfile.epoch = FormatEpoch::DclLegacy;
    ImportReport zeroAvoidanceReport;
    const auto zeroAvoidanceOptions = runImport(
        makeContainer(zeroAvoidanceRows, FormatEpoch::DclLegacy),
        zeroAvoidanceProfile, zeroAvoidanceReport);
    expectMapping(zeroAvoidanceOptions->slurAvoidStaffLinesAmt == 912
            && field(zeroAvoidanceReport,
                   "options.smartShapeOptions.slurAvoidStaffLinesAmt").origin
                == ValueOrigin::Finale27Default
            && field(zeroAvoidanceReport,
                   "options.smartShapeOptions.slurAvoidStaffLinesAmt").rawValue
                == 912,
        "A zero stored staff-line avoidance amount replaced the seeded default");

    auto singleAdjustmentRows = rows;
    singleAdjustmentRows.erase(singleAdjustmentRows.begin() + 5);
    auto singleAdjustmentProfile = profileFor(7, 0);
    singleAdjustmentProfile.epoch = FormatEpoch::DclLegacy;
    ImportReport singleAdjustmentReport;
    const auto singleAdjustmentOptions = runImport(
        makeContainer(singleAdjustmentRows, FormatEpoch::DclLegacy),
        singleAdjustmentProfile, singleAdjustmentReport);
    expectMapping(singleAdjustmentOptions->slurPadding == 24
            && singleAdjustmentOptions->slurAcciPadding == 24
            && !singleAdjustmentOptions->slurDoStretchFirst,
        "A single-incidence enhanced slur layout did not apply its shared padding behavior");
    expectMapping(field(singleAdjustmentReport,
                      "options.smartShapeOptions.slurAcciPadding").origin
                == ValueOrigin::LegacyBehavior
            && field(singleAdjustmentReport,
                   "options.smartShapeOptions.slurAcciPadding").rawValue
                == 24
            && field(singleAdjustmentReport,
                   "options.smartShapeOptions.slurDoStretchFirst").origin
                == ValueOrigin::LegacyBehavior,
        "A single-incidence enhanced slur layout reported incorrect behavior origins");

    std::vector<SyntheticClassRow> classRows{
        {finale_mus_reader::numericGlobalClass(50), {10, 11, 12, 13, 2, 9}},
        {finale_mus_reader::numericGlobalClass(51), {0, 1536, 0, 2048, 8500, 1}},
        {finale_mus_reader::numericGlobalClass(52),
            {36, 614, 16, 288, 512, 60, 864, 410, 72, 1152, 369, 80}},
        {finale_mus_reader::numericGlobalClass(53),
            {21, 22, 23, 1, 24, 4500, 3, 1, 1, 1500, 0, 0}},
        {finale_mus_reader::numericGlobalClass(92), {4, 5, 6, 7, 0, 0}},
        {finale_mus_reader::numericGlobalClass(93), {0, 10000, 0, 0, 0, 0}},
        {finale_mus_reader::numericGlobalClass(97),
            {0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0}},
        {0x008d, {40, 224, 12, 0, 225, 1}, 11},
        {0x008d, {0, 18, 0, 19, 1, 0}, 12},
        {0x0028, slurConnectionWords},
        {0x0068, tabSlideConnectionWords},
        {0x0069, glissandoConnectionWords},
        {0x0070, bendCurveConnectionWords},
    };
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        auto profile = profileFor(16, 0);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = byteOrder;
        ImportReport report;
        expectRecovered(runImport(makeClassContainer(classRows, byteOrder), profile, report),
            report, byteOrder == ByteOrder::BigEndian
                ? "The big-endian zlib epoch" : "The little-endian zlib epoch");
    }

    auto coda = profileFor(2, 6);
    coda.epoch = FormatEpoch::CodaBanner;
    coda.version.reset();
    ImportReport codaReport;
    const std::vector<SyntheticRow> codaRows{
        {GLOBALS_CMPER, "51", {-13, 17, -15, 19, 3, 5}},
        {GLOBALS_CMPER, "26", {13, 31, -32, 0, 33, -34}},
    };
    const auto codaOptions = runImport(makeContainer(codaRows, FormatEpoch::CodaBanner),
        coda, codaReport);
    expectMapping(codaOptions->crescHeight == 902
            && codaOptions->shortHairpinOpeningWidth == 902
            && codaOptions->slurThicknessCp1X == -13
            && codaOptions->slurThicknessCp1Y == -17
            && codaOptions->slurThicknessCp2X == -15
            && codaOptions->slurThicknessCp2Y == -19
            && codaOptions->slurControlStyles.size() == 4
            && codaOptions->slurConnectStyles.size() == 29
            && codaOptions->slurConnectStyles.at(
                   SmartShape::SlurConnectStyleType::OverNoteStart)->connectIndex
                == SmartShape::ConnectionIndex::NoteRightCenter
            && codaOptions->slurConnectStyles.at(
                   SmartShape::SlurConnectStyleType::OverNoteStart)->xOffset == 31
            && codaOptions->slurConnectStyles.at(
                   SmartShape::SlurConnectStyleType::OverNoteEnd)->yOffset == -34,
        "The Coda epoch did not recover its SmartShapeOptions control-point pairs");
    expectMapping(field(codaReport, "options.smartShapeOptions.crescHeight").origin
                == ValueOrigin::Finale27Default
            && field(codaReport,
                   "options.smartShapeOptions.slurThicknessCp1Y").origin
                == ValueOrigin::LegacyMus
            && field(codaReport,
                   "options.smartShapeOptions.slurThicknessCp2Y").rawValue
                == 19
            && field(codaReport,
                   "options.smartShapeOptions.shortHairpinOpeningWidth").origin
                == ValueOrigin::LegacyBehavior
            && field(codaReport,
                   "options.smartShapeOptions.slurConnectStyles[1].yOffset").origin
                == ValueOrigin::LegacyMus
            && field(codaReport,
                   "options.smartShapeOptions.slurConnectStyles[2].yOffset").origin
                == ValueOrigin::Finale27Default,
        "The Coda epoch reported incorrect SmartShapeOptions origins");

    ImportReport codaDefaultReport;
    const std::vector<SyntheticRow> codaDefaultRows{
        {GLOBALS_CMPER, "51", {0, -6, 0, -6, 0, 8}},
    };
    const auto codaDefaults = runImport(
        makeContainer(codaDefaultRows, FormatEpoch::CodaBanner), coda, codaDefaultReport);
    expectMapping(codaDefaults->slurThicknessCp1Y == 6
            && codaDefaults->slurThicknessCp2Y == 6
            && codaDefaults->slurThicknessCp1X == 0
            && codaDefaults->slurThicknessCp2X == 0,
        "The Coda defaults did not recover both thickness-control pairs");

    auto earlyRows = rows;
    earlyRows.erase(std::remove_if(earlyRows.begin(), earlyRows.end(), [](const auto& row) {
        return std::string_view(row.tag) == "97" || std::string_view(row.tag) == "FI";
    }), earlyRows.end());
    earlyRows.erase(earlyRows.begin() + 3);
    auto earlyProfile = profileFor(3, 7);
    earlyProfile.epoch = FormatEpoch::UncompressedLegacy;
    ImportReport earlyReport;
    const auto earlyOptions = runImport(
        makeContainer(earlyRows), earlyProfile, earlyReport);
    expectMapping(earlyOptions->slurThicknessCp1X == 908
            && earlyOptions->slurControlStyles.size() == 4,
        "An older six-word slur layout was interpreted as SmartShapeOptions");

    const std::vector<SyntheticRow> preEngraverRows{
        {GLOBALS_CMPER, "59", {118, 6, 6, 8, 8, 17}},
        {GLOBALS_CMPER, "52", {36, 532, 13, 288, 553, 43}},
        {GLOBALS_CMPER, "52", {864, 358, 73, 0, 0, 0}},
        {GLOBALS_CMPER, "53", {3, 5, 7, 0, 0, 0}},
    };
    ImportReport preEngraverReport;
    const auto preEngraverOptions = runImport(
        makeContainer(preEngraverRows), earlyProfile, preEngraverReport);
    const auto longStyle = preEngraverOptions->slurControlStyles.at(
        SmartShape::SlurControlStyleType::LongSpan);
    const auto extraLongStyle = preEngraverOptions->slurControlStyles.at(
        SmartShape::SlurControlStyleType::ExtraLongSpan);
    expectMapping(longStyle->span == 864 && longStyle->inset == 358
            && longStyle->height == 73 && extraLongStyle->span == 936
            && extraLongStyle->inset == 358 && extraLongStyle->height == 73
            && preEngraverOptions->slurLeftBreakHorzAdj == 3
            && preEngraverOptions->slurRightBreakHorzAdj == 5
            && preEngraverOptions->slurBreakVertAdj == 7
            && preEngraverOptions->slurThicknessCp1Y == 17
            && preEngraverOptions->slurThicknessCp2Y == 17
            && !preEngraverOptions->slurAvoidStaffLines
            && preEngraverOptions->slurAcciPadding == 920
            && preEngraverOptions->slurDoStretchFirst,
        "A pre-Engraver-Slur file did not preserve its three contours and seeded extra-long span");
    expectMapping(field(preEngraverReport,
                      "options.smartShapeOptions.slurControlStyles[2].height").origin
                == ValueOrigin::LegacyMus
            && field(preEngraverReport,
                   "options.smartShapeOptions.slurControlStyles[3].height").origin
                == ValueOrigin::LegacyBehavior
            && field(preEngraverReport,
                   "options.smartShapeOptions.slurControlStyles[3].height").rawValue
                == 73
            && field(preEngraverReport,
                   "options.smartShapeOptions.slurThicknessCp1Y").origin
                == ValueOrigin::LegacyMus
            && field(preEngraverReport,
                   "options.smartShapeOptions.slurThicknessCp2Y").rawValue
                == 17
            && field(preEngraverReport,
                   "options.smartShapeOptions.slurAvoidStaffLines").origin
                == ValueOrigin::Finale27Default,
        "A pre-Engraver-Slur file reported incorrect contour origins");
}

void testSmartShapeCustomLineFallbackGate()
{
    const auto queuedReferences = [](FormatEpoch epoch, std::uint8_t major) {
        const auto parsed = makeContainer(std::vector<SyntheticRow>{}, epoch);
        const auto index = LegacyRecordIndex::build(parsed);
        auto profile = profileFor(major);
        profile.epoch = epoch;
        const auto document = makeSmartShapeOptionsDocument();
        const auto reference = makeSmartShapeOptionsDocument();
        ImportReport report;
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importSmartShapeOptions(context);
        std::vector<musx::dom::Cmper> result;
        for (const auto& request : pending.customLines) {
            result.push_back(request.referenceLineId);
        }
        return result;
    };

    expectMapping(queuedReferences(FormatEpoch::CodaBanner, 99)
            == std::vector<musx::dom::Cmper>{923, 924, 925},
        "The Coda epoch did not request baseline lines independently of its version");
    expectMapping(queuedReferences(FormatEpoch::UncompressedLegacy, 4)
            == std::vector<musx::dom::Cmper>{923, 924, 925},
        "A pre-major-5 uncompressed file did not request baseline lines in tool order");
    expectMapping(queuedReferences(FormatEpoch::UncompressedLegacy, 5)
            == std::vector<musx::dom::Cmper>{925},
        "A Finale 2000 profile did not request only the unavailable bend curve");
    expectMapping(queuedReferences(FormatEpoch::DclLegacy, 7)
            == std::vector<musx::dom::Cmper>{925},
        "A pre-Finale-2003 DCL profile did not request the bend curve");
    expectMapping(queuedReferences(FormatEpoch::DclLegacy, 8).empty(),
        "A Finale 2003 DCL profile requested a baseline bend curve");
    expectMapping(queuedReferences(FormatEpoch::ZlibLegacy, 7).empty(),
        "A zlib file was accepted by the DCL bend-curve version gate");
}

void testSmartShapeDirectionGate()
{
    using Direction = musx::dom::ShapeDirection;
    using SmartShape = musx::dom::options::SmartShapeOptions;
    const auto importFixed = [](FormatEpoch epoch, std::uint8_t major,
                                 bool keepVersion, std::int16_t raw,
                                 ImportReport& report) {
        const std::vector<SyntheticRow> rows{
            {GLOBALS_CMPER, "10", {raw, 75, 239, 229, 0, 0}},
        };
        const auto parsed = makeContainer(rows, epoch);
        const auto index = LegacyRecordIndex::build(parsed);
        auto profile = profileFor(major);
        profile.epoch = epoch;
        if (!keepVersion) profile.version.reset();
        const auto document = makeSmartShapeOptionsDocument();
        const auto reference = makeSmartShapeOptionsDocument();
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importSmartShapeOptions(context);
        return document->getOptions()->get<SmartShape>()->direction;
    };

    ImportReport finale2001Report;
    expectMapping(importFixed(FormatEpoch::DclLegacy, 6, true, -1, finale2001Report)
                == Direction::Automatic
            && field(finale2001Report, "options.smartShapeOptions.direction").origin
                == ValueOrigin::Finale27Default,
        "A pre-Finale-2002 DCL word replaced the seeded Automatic direction");

    ImportReport finale2002Report;
    expectMapping(importFixed(FormatEpoch::DclLegacy, 7, true, -1, finale2002Report)
                == Direction::Under
            && field(finale2002Report, "options.smartShapeOptions.direction").origin
                == ValueOrigin::LegacyMus
            && field(finale2002Report, "options.smartShapeOptions.direction").rawValue == -1,
        "Finale 2002 did not recover its default slur direction");

    ImportReport finale2006Report;
    expectMapping(importFixed(FormatEpoch::DclLegacy, 11, true, 1, finale2006Report)
            == Direction::Over,
        "The Finale 2002 direction range ended before the DCL epoch");

    for (const auto invalid : std::array<std::int16_t, 2>{-2, 103}) {
        ImportReport report;
        expectMapping(importFixed(FormatEpoch::DclLegacy, 11, true, invalid, report)
                    == Direction::Automatic
                && field(report, "options.smartShapeOptions.direction").origin
                    == ValueOrigin::Finale27Default,
            "An invalid DCL direction replaced the seeded Automatic direction");
    }

    for (const auto& test : std::array{
             std::pair{FormatEpoch::UncompressedLegacy, true},
             std::pair{FormatEpoch::DclLegacy, false}}) {
        ImportReport report;
        expectMapping(importFixed(test.first, 7, test.second, -1, report)
                == Direction::Automatic,
            "The direction gate crossed an epoch or accepted a missing version");
    }

    const auto importClass = [](std::int16_t raw, ImportReport& report) {
        const std::vector<SyntheticClassRow> classRows{
            {finale_mus_reader::numericGlobalClass(10), {raw, 75, 239, 229, 0, 0}},
        };
        auto profile = profileFor(12);
        profile.epoch = FormatEpoch::ZlibLegacy;
        const auto parsed = makeClassContainer(classRows, ByteOrder::BigEndian);
        const auto index = LegacyRecordIndex::build(parsed);
        const auto document = makeSmartShapeOptionsDocument();
        const auto reference = makeSmartShapeOptionsDocument();
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importSmartShapeOptions(context);
        return document->getOptions()->get<SmartShape>()->direction;
    };

    ImportReport zlibReport;
    expectMapping(importClass(1, zlibReport) == Direction::Over
            && field(zlibReport, "options.smartShapeOptions.direction").origin
                == ValueOrigin::LegacyMus,
        "The zlib direction class did not recover the stored direction");

    ImportReport invalidZlibReport;
    expectMapping(importClass(103, invalidZlibReport) == Direction::Automatic
            && field(invalidZlibReport, "options.smartShapeOptions.direction").origin
                == ValueOrigin::Finale27Default,
        "An invalid zlib direction replaced the seeded Automatic direction");
}

void testSmartShapeHookLengthBehaviorGate()
{
    using SmartShape = musx::dom::options::SmartShapeOptions;
    const auto importHookLength = [](FormatEpoch epoch, std::uint8_t major,
                                      std::uint8_t minor, bool keepVersion,
                                      ImportReport& report) {
        const auto parsed = makeContainer(std::vector<SyntheticRow>{}, epoch);
        const auto index = LegacyRecordIndex::build(parsed);
        auto profile = profileFor(major, minor);
        profile.epoch = epoch;
        if (!keepVersion) {
            profile.version.reset();
        }
        const auto document = makeSmartShapeOptionsDocument();
        const auto reference = makeSmartShapeOptionsDocument();
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importSmartShapeOptions(context);
        return document->getOptions()->get<SmartShape>()->hookLength;
    };

    ImportReport finale26Report;
    expectMapping(importHookLength(FormatEpoch::CodaBanner, 2, 6, true, finale26Report) == 8
            && field(finale26Report, "options.smartShapeOptions.hookLength").origin
                == ValueOrigin::LegacyBehavior,
        "Finale 2.6 did not receive its fixed hook length as legacy behavior");

    struct ProfileCase
    {
        FormatEpoch epoch;
        std::uint8_t major;
        std::uint8_t minor;
        bool keepVersion;
    };
    for (const auto& profile : std::array{
             ProfileCase{FormatEpoch::CodaBanner, 1, 0, true},
             ProfileCase{FormatEpoch::CodaBanner, 2, 6, false},
             ProfileCase{FormatEpoch::UncompressedLegacy, 2, 6, true}}) {
        ImportReport report;
        expectMapping(importHookLength(profile.epoch, profile.major, profile.minor,
                          profile.keepVersion, report)
                == 904,
            "The Finale 2.6 hook-length behavior escaped its epoch and version gate");
    }
}

TEST_CASE("Lyric punctuation tail encoding gate", "[mapping]")
{
    testLyricPunctuationTailEncodingGate();
}
TEST_CASE("Lyric word extension connection layouts", "[mapping]")
{
    testLyricWordExtConnectionLayouts();
}
TEST_CASE("Lyric edge punctuation version gate", "[mapping]")
{
    testLyricEdgePunctuationVersionGate();
}
TEST_CASE("Lyric post-format assertions", "[mapping]")
{
    testLyricPostFormatAssertions();
}
TEST_CASE("Version gating", "[mapping]") { testVersionGating(); }
TEST_CASE("Minor version ordering", "[mapping]") { testMinorVersionOrdering(); }
TEST_CASE("Table layering", "[mapping]") { testTableLayering(); }
TEST_CASE("Text options scalars", "[mapping]") { testTextOptionsScalars(); }
TEST_CASE("Text options symbol inserts", "[mapping]")
{
    testTextOptionsSymbolInserts();
}
TEST_CASE("Repeat options span the located epochs", "[mapping]")
{
    testRepeatOptionsAcrossEpochs();
}
TEST_CASE("Smart Shape options span the located epochs", "[mapping]")
{
    testSmartShapeOptionsAcrossEpochs();
}
TEST_CASE("Smart Shape custom-line fallback gate", "[mapping]")
{
    testSmartShapeCustomLineFallbackGate();
}
TEST_CASE("Smart Shape direction gate", "[mapping]")
{
    testSmartShapeDirectionGate();
}
TEST_CASE("Smart Shape hook-length behavior gate", "[mapping]")
{
    testSmartShapeHookLengthBehaviorGate();
}
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
TEST_CASE("Smart shape custom lines span three epochs", "[mapping]")
{
    testSmartShapeCustomLinesAcrossEpochs();
}
TEST_CASE("Smart shape custom line Unicode layout", "[mapping]")
{
    testSmartShapeCustomLineUnicodeLayout();
}
TEST_CASE("Measure graphic assignments span four epochs", "[mapping]")
{
    testMeasureGraphicAssignmentsAcrossEpochs();
}
TEST_CASE("Stored text blocks span three epochs", "[mapping]")
{
    testStoredTextBlocksAcrossEpochs();
}
TEST_CASE("Coda text blocks are assembled from text structure", "[mapping]")
{
    testCodaTextBlockSynthesis();
}

} // namespace finale_mus_reader_tests
