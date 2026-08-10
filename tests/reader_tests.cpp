// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <zlib.h>

#include "finale_mus_reader/reader.h"
#include "musx/musx.h"

#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#define FINALE_MUS_READER_TEST_UNDEFINE_MUSX_USE_PUGIXML
#endif

#include "musx/xml/PugiXmlImpl.h"

#ifdef FINALE_MUS_READER_TEST_UNDEFINE_MUSX_USE_PUGIXML
#undef MUSX_USE_PUGIXML
#undef FINALE_MUS_READER_TEST_UNDEFINE_MUSX_USE_PUGIXML
#endif

namespace {

using finale_mus_reader::BlockInfo;
using finale_mus_reader::ByteOrder;
using finale_mus_reader::FieldInfo;
using finale_mus_reader::FormatEpoch;
using finale_mus_reader::ImportResult;
using finale_mus_reader::Reader;
using finale_mus_reader::SourcePlatform;
using finale_mus_reader::ValueOrigin;
using TestXmlDocument = musx::xml::pugi::Document;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void write16(std::vector<std::uint8_t>& output, std::uint16_t value, ByteOrder byteOrder)
{
    if (byteOrder == ByteOrder::BigEndian) {
        output.push_back(static_cast<std::uint8_t>(value >> 8U));
        output.push_back(static_cast<std::uint8_t>(value));
    } else {
        output.push_back(static_cast<std::uint8_t>(value));
        output.push_back(static_cast<std::uint8_t>(value >> 8U));
    }
}

void write32(std::vector<std::uint8_t>& output, std::uint32_t value, ByteOrder byteOrder)
{
    if (byteOrder == ByteOrder::BigEndian) {
        output.push_back(static_cast<std::uint8_t>(value >> 24U));
        output.push_back(static_cast<std::uint8_t>(value >> 16U));
        output.push_back(static_cast<std::uint8_t>(value >> 8U));
        output.push_back(static_cast<std::uint8_t>(value));
    } else {
        output.push_back(static_cast<std::uint8_t>(value));
        output.push_back(static_cast<std::uint8_t>(value >> 8U));
        output.push_back(static_cast<std::uint8_t>(value >> 16U));
        output.push_back(static_cast<std::uint8_t>(value >> 24U));
    }
}

void writeFixed(std::vector<std::uint8_t>& output, std::size_t offset,
    std::string_view value, std::size_t capacity)
{
    expect(offset + capacity <= output.size(), "Synthetic fixed field exceeds its buffer");
    const auto count = (std::min)(value.size(), capacity);
    std::copy_n(value.begin(), static_cast<std::ptrdiff_t>(count),
        output.begin() + static_cast<std::ptrdiff_t>(offset));
}

// The synthetic Enigma version: major 12, minor 3, maintenance 4, build 5. It is
// written in the file's own byte order, as Finale writes it.
constexpr std::uint32_t syntheticVersion = (12U << 24U) | (3U << 20U) | (4U << 16U) | 5U;

void writeVersion(std::vector<std::uint8_t>& output, std::size_t offset,
    std::uint32_t value, ByteOrder byteOrder)
{
    for (std::size_t i = 0; i < 4; ++i) {
        const auto shift = byteOrder == ByteOrder::BigEndian ? (3 - i) * 8 : i * 8;
        output[offset + i] = static_cast<std::uint8_t>(value >> shift);
    }
}

std::vector<std::uint8_t> makeBanner(std::string_view product,
    std::string_view platform, ByteOrder byteOrder)
{
    std::vector<std::uint8_t> result(0x200);
    writeFixed(result, 0, "ENIGMA BINARY FILE", 19);
    writeFixed(result, 0x20, "Finale(R) " + std::string(product) + " Copyright synthetic", 0x40);
    result[0x66] = 126;
    result[0x67] = 8;
    result[0x68] = 8;
    result[0x8c] = 126;
    result[0x8d] = 8;
    result[0x8e] = 8;
    for (const auto tupleOffset : {std::size_t{0x6c}, std::size_t{0x92}}) {
        writeVersion(result, tupleOffset, syntheticVersion, byteOrder);
        writeFixed(result, tupleOffset + 4, "FIN", 4);
        writeFixed(result, tupleOffset + 8, platform, 4);
        writeVersion(result, tupleOffset + 12, syntheticVersion, byteOrder);
        writeVersion(result, tupleOffset + 16, syntheticVersion, byteOrder);
    }
    return result;
}

std::array<std::int16_t, 6> words(
    std::int16_t first, std::int16_t second, std::int16_t third,
    std::int16_t fourth, std::int16_t fifth, std::int16_t sixth)
{
    return {first, second, third, fourth, fifth, sixth};
}

void appendOther(std::vector<std::uint8_t>& output, std::uint16_t cmper,
    std::string_view tag, const std::array<std::int16_t, 6>& payload,
    ByteOrder byteOrder)
{
    expect(tag.size() == 2, "Synthetic other tag must be two bytes");
    write16(output, cmper, byteOrder);
    // The tag is a 16-bit value, so a little-endian file stores "LA" as the bytes "AL".
    // Writing the characters raw would encode a file Finale never produces.
    write16(output, static_cast<std::uint16_t>(
        (static_cast<unsigned char>(tag[0]) << 8U) | static_cast<unsigned char>(tag[1])),
        byteOrder);
    for (const auto value : payload) {
        write16(output, static_cast<std::uint16_t>(value), byteOrder);
    }
}

void appendUncompressedBlock(std::vector<std::uint8_t>& output,
    std::uint16_t type, const std::vector<std::uint8_t>& payload,
    ByteOrder byteOrder)
{
    write16(output, type, byteOrder);
    write32(output, static_cast<std::uint32_t>(payload.size() + 6), byteOrder);
    output.insert(output.end(), payload.begin(), payload.end());
}

std::vector<std::uint8_t> makeUncompressedMus()
{
    constexpr auto byteOrder = ByteOrder::LittleEndian;
    auto result = makeBanner("2000", "WIN", byteOrder);
    std::vector<std::uint8_t> others;
    appendOther(others, 0, "LA", words(11, 0, 0, 0, 0, 0), byteOrder);
    appendOther(others, 1, "LA", words(-12, 0, 0, 0, 0, 0), byteOrder);
    appendOther(others, 2, "LA", words(13, 0, 0, 0, 0, 0), byteOrder);
    appendOther(others, 3, "LA", words(-14, 0, 0, 0, 0, 0), byteOrder);
    appendOther(others, 0xfffe, "94", words(2, 361, 1801, 13, 49, 0), byteOrder);
    appendUncompressedBlock(result, 1, others, byteOrder);
    appendUncompressedBlock(result, 2, {}, byteOrder);
    appendUncompressedBlock(result, 3, {}, byteOrder);
    appendUncompressedBlock(result, 4, {}, byteOrder);
    return result;
}

std::vector<std::uint8_t> compressZlib(const std::vector<std::uint8_t>& input)
{
    uLongf compressedSize = compressBound(static_cast<uLong>(input.size()));
    std::vector<std::uint8_t> result(compressedSize);
    const auto code = compress2(result.data(), &compressedSize, input.data(),
        static_cast<uLong>(input.size()), Z_BEST_COMPRESSION);
    expect(code == Z_OK, "Unable to create synthetic zlib member");
    result.resize(compressedSize);
    return result;
}

void appendZlibBlock(std::vector<std::uint8_t>& output, std::uint16_t type,
    const std::vector<std::uint8_t>& payload, ByteOrder byteOrder)
{
    const auto compressed = compressZlib(payload);
    write16(output, type, byteOrder);
    write32(output, static_cast<std::uint32_t>(compressed.size() + 10), byteOrder);
    const auto checksum = static_cast<std::uint32_t>(
        crc32(crc32(0L, Z_NULL, 0), payload.data(), static_cast<uInt>(payload.size())));
    write32(output, checksum, byteOrder);
    output.insert(output.end(), compressed.begin(), compressed.end());
}

std::vector<std::uint8_t> makeZlibMus()
{
    constexpr auto byteOrder = ByteOrder::LittleEndian;
    auto result = makeBanner("2012", "MAC", byteOrder);
    appendZlibBlock(result, 0x001a, {1, 2, 3}, byteOrder);
    appendZlibBlock(result, 0x001b, {4, 5}, byteOrder);
    appendZlibBlock(result, 0x0016, {6}, byteOrder);
    appendZlibBlock(result, 0x0017, {7, 8, 9}, byteOrder);
    write16(result, 0x001d, byteOrder);
    write32(result, 6, byteOrder);
    return result;
}

// Taken by value rather than by const reference: see the note on the equivalent helper in
// mapping_tests.cpp about -Wdangling-reference.
const FieldInfo& field(const ImportResult& result, std::string_view target)
{
    const auto found = std::find_if(result.report.fields.begin(), result.report.fields.end(),
        [&](const FieldInfo& value) { return std::string_view(value.target) == target; });
    expect(found != result.report.fields.end(),
        std::string("Missing field report for ").append(target));
    return *found;
}

template <typename T>
void expectOption(const ImportResult& result)
{
    expect(static_cast<bool>(result.document->getOptions()->get<T>()),
        "Pinned default omitted an expected options instance");
}

void expectCompleteOptions(const ImportResult& result)
{
    using namespace musx::dom::options;
    expectOption<AccidentalOptions>(result);
    expectOption<AlternateNotationOptions>(result);
    expectOption<AugmentationDotOptions>(result);
    expectOption<BarlineOptions>(result);
    expectOption<BeamOptions>(result);
    expectOption<ChordOptions>(result);
    expectOption<ClefOptions>(result);
    expectOption<FlagOptions>(result);
    expectOption<FontOptions>(result);
    expectOption<GraceNoteOptions>(result);
    expectOption<KeySignatureOptions>(result);
    expectOption<LineCurveOptions>(result);
    expectOption<LyricOptions>(result);
    expectOption<MiscOptions>(result);
    expectOption<MultimeasureRestOptions>(result);
    expectOption<MusicSpacingOptions>(result);
    expectOption<MusicSymbolOptions>(result);
    expectOption<NoteRestOptions>(result);
    expectOption<PageFormatOptions>(result);
    expectOption<PianoBraceBracketOptions>(result);
    expectOption<RepeatOptions>(result);
    expectOption<SmartShapeOptions>(result);
    expectOption<StaffOptions>(result);
    expectOption<StemOptions>(result);
    expectOption<TextOptions>(result);
    expectOption<TieOptions>(result);
    expectOption<TimeSignatureOptions>(result);
    expectOption<TupletOptions>(result);
}

void expectNoScoreContent(const ImportResult& result)
{
    using namespace musx::dom;
    expect(result.document->getOthers()->getArray<others::Measure>(SCORE_PARTID).empty(),
        "Output contains fallback measures");
    expect(result.document->getOthers()->getArray<others::Staff>(SCORE_PARTID).empty(),
        "Output contains fallback staves");
    expect(result.document->getOthers()->getArray<others::StaffSystem>(SCORE_PARTID).empty(),
        "Output contains fallback systems");
    expect(result.document->getOthers()->getArray<others::Page>(SCORE_PARTID).empty(),
        "Output contains fallback pages");
    expect(result.document->getOthers()->getArray<others::PartDefinition>(SCORE_PARTID).empty(),
        "Output contains fallback part definitions");
    // The pinned <others> element has 127 direct children; only the four layerAtts are
    // allowlisted. These are the most numerous of the rest.
    // Font definitions are recovered from the file, so the test is not that they are absent
    // but that none of them came from the pinned baseline. These names appear in the Finale
    // 27 default and in no legacy fixture.
    for (const auto& font : result.document->getOthers()
            ->getArray<others::FontDefinition>(SCORE_PARTID)) {
        expect(font->name != "Broadway Copyist" && font->name != "Engraver Text T"
                && font->name != "Broadway Copyist Text",
            "A pinned default font leaked into the imported document");
    }
    expect(result.document->getOthers()->getArray<others::MarkingCategory>(SCORE_PARTID).empty(),
        "Output contains fallback marking categories");
    expect(result.document->getOthers()->getArray<others::ShapeDef>(SCORE_PARTID).empty(),
        "Output contains fallback shape definitions");
    expect(result.document->getOthers()->getArray<others::TextBlock>(SCORE_PARTID).empty(),
        "Output contains fallback text blocks");
    expect(result.document->getOthers()->getArray<others::MeasureNumberRegion>(SCORE_PARTID).empty(),
        "Output contains fallback measure number regions");
    expect(!result.document->getEntries()->get(1), "Output contains fallback entries");
    expect(result.document->getInstruments().empty(), "Output contains fallback instruments");
    expect(result.document->getOthers()->getArray<others::LayerAttributes>(SCORE_PARTID).size() == 4,
        "Output does not contain the four option-like layer attributes");
}

void testControlledDclFile()
{
    const auto path = std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
        / "evidence/F2002/F2002-baseline.mus";
    const auto result = Reader::read<TestXmlDocument>(path);
    expect(result.report.formatEpoch == FormatEpoch::DclLegacy,
        "F2002 fixture was not classified as DCL");
    expect(result.report.byteOrder == ByteOrder::BigEndian,
        "F2002 fixture byte order was not recovered");
    expect(result.report.sourcePlatform == SourcePlatform::MacOS,
        "F2002 fixture platform was not recovered");
    expect(result.report.defaultsPlatform == SourcePlatform::MacOS,
        "F2002 fixture was not seeded from the macOS baseline");
    expect(result.report.savingProduct == "2002", "F2002 product was not recovered");
    expect(result.report.blocks.size() == 4, "F2002 block count is incorrect");
    expect(std::all_of(result.report.blocks.begin(), result.report.blocks.begin() + 3,
        [](const BlockInfo& block) { return block.checksumPresent && block.checksumValid; }),
        "F2002 compressed block checksum validation failed");

    const auto& header = *result.document->getHeader();
    expect(header.created.year == 2026 && header.created.month == 8 && header.created.day == 5,
        "F2002 creation date was not recovered");
    expect(header.created.application == "FIN", "F2002 creator application was not recovered");
    expect(header.created.finaleVersion.major == 7 && header.created.finaleVersion.minor == 0
            && header.created.finaleVersion.maint == 1,
        "F2002 internal creator version was not recovered");
    expect(result.document->getSourcePath() == path, "Source path was not retained");

    const auto spacing = result.document->getOptions()
        ->get<musx::dom::options::MusicSpacingOptions>();
    expect(spacing->minWidth == 360 && spacing->maxWidth == 1800,
        "F2002 music spacing width overlay failed");
    expect(spacing->minDistance == 12 && spacing->minDistTiedNotes == 48,
        "F2002 music spacing distance overlay failed");
    expect(field(result, "options.musicSpacing.minWidth").origin == ValueOrigin::LegacyMus,
        "F2002 music spacing overlay was not reported as recovered");
    expectCompleteOptions(result);
    expectNoScoreContent(result);
}

void testIndependentImportedDocuments()
{
    // The reader builds each document with its own construction session, so no pinned
    // fallback document can remain the owner of options placed in an imported document.
    const auto path = std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
        / "evidence/F2002/F2002-baseline.mus";
    const auto first = Reader::read<TestXmlDocument>(path);
    const auto second = Reader::read<TestXmlDocument>(path);
    expect(first.document != second.document, "Both reads returned the same document");

    const auto firstSpacing = first.document->getOptions()
        ->get<musx::dom::options::MusicSpacingOptions>();
    const auto secondSpacing = second.document->getOptions()
        ->get<musx::dom::options::MusicSpacingOptions>();
    expect(firstSpacing && secondSpacing, "Music spacing options were not seeded");
    expect(firstSpacing.get() != secondSpacing.get(),
        "Imported documents share a music spacing options instance");

    const auto firstLayer = first.document->getOthers()
        ->get<musx::dom::others::LayerAttributes>(musx::dom::SCORE_PARTID, 0);
    const auto secondLayer = second.document->getOthers()
        ->get<musx::dom::others::LayerAttributes>(musx::dom::SCORE_PARTID, 0);
    expect(firstLayer && secondLayer, "Layer attributes were not seeded");
    expect(firstLayer.get() != secondLayer.get(),
        "Imported documents share a layer attributes instance");
}

// Font definitions come from the file, never from the pinned baseline. The controlled
// fixture's ETF prints the same table, so the names and character sets are ground truth:
// ^FN(0) 8191 0 0 0 0 0 with ^FN(0) "Maestro", where 8191 is 0x1fff, a Mac symbol font.
void testFontDefinitions()
{
    const auto path = std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
        / "evidence/F2002/F2002-baseline.mus";
    const auto result = Reader::read<TestXmlDocument>(path);
    using musx::dom::others::FontDefinition;
    const auto fonts = result.document->getOthers()
        ->getArray<FontDefinition>(musx::dom::SCORE_PARTID);
    expect(fonts.size() == 9, "F2002 font table size is incorrect");

    const auto fontAt = [&](musx::dom::Cmper cmper) {
        const auto font = result.document->getOthers()->get<FontDefinition>(
            musx::dom::SCORE_PARTID, cmper);
        expect(static_cast<bool>(font), "Missing font definition " + std::to_string(cmper));
        return font;
    };

    const auto maestro = fontAt(0);
    expect(maestro->name == "Maestro", "Font 0 name was not recovered");
    expect(maestro->charsetBank == FontDefinition::CharacterSetBank::MacOS,
        "Font 0 character set bank was not recovered");
    expect(maestro->charsetVal == 0xfff, "Font 0 character set value was not recovered");
    expect(maestro->calcIsSymbolFont(), "Font 0 should be a symbol font");

    const auto times = fontAt(1);
    expect(times->name == "Times", "Font 1 name was not recovered");
    expect(times->charsetVal == 0 && !times->calcIsSymbolFont(),
        "A text font was reported as a symbol font");

    // A name longer than one row continues into the following incidences.
    expect(fontAt(5)->name == "Maestro Percussion",
        "A font name spanning incidences was not assembled");

    // The pinned baseline must not contribute fonts of its own: every one of the Finale 27
    // defaults would otherwise appear here alongside the recovered table.
    for (const auto& font : fonts) {
        expect(font->name != "Broadway Copyist" && font->name != "Engraver Text T",
            "A pinned default font leaked into the imported document");
    }
}

// The uncompressed era had no tracked fixture until these: every result for it was
// previously verified against an unpublished corpus or against synthetic files the tests
// wrote themselves.
void testUncompressedFixtures()
{
    const auto read = [](const char* relative) {
        return Reader::read<TestXmlDocument>(
            std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / relative);
    };
    using musx::dom::others::FontDefinition;

    const auto f2000 = read("evidence/F2000/F2000-multilayer.mus");
    expect(f2000.report.formatEpoch == FormatEpoch::UncompressedLegacy,
        "Finale 2000 fixture was not classified as uncompressed");
    expect(f2000.report.savingProduct == "2000", "Finale 2000 product was not recovered");
    expect(f2000.report.sourceVersion && f2000.report.sourceVersion->major == 5,
        "Finale 2000 should record internal major version 5");
    expect(field(f2000, "options.musicSpacing.minWidth").origin == ValueOrigin::LegacyMus,
        "Finale 2000 music spacing was not recovered");
    expect(!f2000.document->getOthers()->getArray<FontDefinition>(
        musx::dom::SCORE_PARTID).empty(), "Finale 2000 fonts were not recovered");

    // These two differ only in the length of their file-info text. The long one overruns the
    // customary 0x200 body boundary, so it frames only when the body offset is read from the
    // header field at 0x60 rather than assumed.
    const auto shortInfo = read("evidence/F97/F97-fileinfo-short.mus");
    const auto longInfo = read("evidence/F97/F97-fileinfo-long.mus");
    for (const auto* result : {&shortInfo, &longInfo}) {
        expect(result->report.formatEpoch == FormatEpoch::UncompressedLegacy,
            "A Finale 97 fixture was not classified as uncompressed");
        expect(result->report.sourceVersion && result->report.sourceVersion->major == 3
                && result->report.sourceVersion->minor == 8,
            "Finale 97 should record internal version 3.8");
    }
    expect(longInfo.report.blocks.size() == shortInfo.report.blocks.size(),
        "The long file-info variant did not frame like its short counterpart");

    const auto shortFonts = shortInfo.document->getOthers()
        ->getArray<FontDefinition>(musx::dom::SCORE_PARTID);
    const auto longFonts = longInfo.document->getOthers()
        ->getArray<FontDefinition>(musx::dom::SCORE_PARTID);
    expect(!shortFonts.empty() && shortFonts.size() == longFonts.size(),
        "The two file-info variants disagree about their font table");
    // Finale 97 is internal 3.8, so its font records carry the header incidence.
    expect(shortFonts[0]->charsetBank == FontDefinition::CharacterSetBank::MacOS,
        "Finale 97 font character set bank was not recovered");
}

// The 2007 encoding is a separate pathway: class-identified, length-governed records rather
// than fixed 16-byte rows. Nothing else in the suite exercises it.
void testClassRecordEra()
{
    const auto result = Reader::read<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2012/F2012-upstem-flags.mus");
    expect(result.report.formatEpoch == FormatEpoch::ZlibLegacy,
        "The Finale 2012 fixture was not classified as zlib era");
    expect(result.report.sourceVersion && result.report.sourceVersion->major == 17,
        "Finale 2012 should record internal major version 17");

    using musx::dom::others::FontDefinition;
    const auto fonts = result.document->getOthers()
        ->getArray<FontDefinition>(musx::dom::SCORE_PARTID);
    expect(fonts.size() == 16, "The zlib-era font table size is incorrect");
    const auto maestro = result.document->getOthers()->get<FontDefinition>(
        musx::dom::SCORE_PARTID, 0);
    expect(maestro && maestro->name == "Maestro",
        "A zlib-era font name was not recovered");
    // The character set encoding survived the 2007 serialization change unchanged.
    expect(maestro->charsetBank == FontDefinition::CharacterSetBank::MacOS
            && maestro->charsetVal == 0xfff && maestro->calcIsSymbolFont(),
        "The zlib-era character set was not decoded like the earlier eras");
}

// Most Finale 2007 documents are big-endian, and every numeric field of the class-record
// encoding follows that byte order, including the payload length in the record header.
void testBigEndianClassRecords()
{
    const auto result = Reader::read<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2007/F2007-lyric-hyphens.mus");
    expect(result.report.formatEpoch == FormatEpoch::ZlibLegacy,
        "The Finale 2007 fixture was not classified as zlib era");
    expect(result.report.byteOrder == ByteOrder::BigEndian,
        "The Finale 2007 fixture should be big-endian");
    expect(result.report.sourceVersion && result.report.sourceVersion->major == 12,
        "Finale 2007 should record internal major version 12");

    using musx::dom::others::FontDefinition;
    const auto fonts = result.document->getOthers()
        ->getArray<FontDefinition>(musx::dom::SCORE_PARTID);
    expect(fonts.size() == 9, "The big-endian font table size is incorrect");
    const auto maestro = result.document->getOthers()->get<FontDefinition>(
        musx::dom::SCORE_PARTID, 0);
    expect(maestro && maestro->name == "Maestro" && maestro->charsetVal == 0xfff,
        "A big-endian class record was not decoded");
    expect(result.document->getOthers()->get<FontDefinition>(
        musx::dom::SCORE_PARTID, 5)->name == "Maestro Percussion",
        "A long name in a big-endian class record was not decoded");
}

// Finale 2006 is the last release to write ETF, but it did not begin mixing the later
// class-identified records into its blocks: the fixed-row model is unchanged.
void testFinale2006RemainsFixedRow()
{
    const auto result = Reader::read<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2006/F2006-single-title.mus");
    expect(result.report.formatEpoch == FormatEpoch::DclLegacy,
        "The Finale 2006 fixture was not classified as DCL");
    expect(field(result, "options.musicSpacing.minWidth").origin == ValueOrigin::LegacyMus,
        "Finale 2006 music spacing was not recovered through the fixed-row path");
    expect(result.document->getOthers()
        ->getArray<musx::dom::others::FontDefinition>(musx::dom::SCORE_PARTID).size() == 9,
        "The Finale 2006 font table size is incorrect");
    // The fifth block is present and empty, which is what distinguishes this era.
    expect(result.report.blocks.size() == 5,
        "Finale 2006 should carry a fifth block");
    expect(result.report.blocks.back().decodedSize == 0,
        "The Finale 2006 fifth block should be empty");
}

void testControlledDclVersions()
{
    // The embedded Enigma version, decoded from the last-saver tuple. Finale's internal
    // majors run 3 for the 3.x line and Finale 97, then 5 for Finale 2000 onward.
    struct Expected
    {
        std::string_view version;
        std::string_view savingProduct;
        std::uint8_t major;
        std::uint8_t minor;
        std::uint8_t maint;
        std::uint8_t build;
    };
    const std::array<Expected, 4> versions{{
        {"F2002", "2002", 7, 0, 1, 2},
        {"F2003", "2003", 8, 0, 0, 5},
        {"F2004", "2004b", 9, 0, 0, 58},
        {"F2005", "2005", 10, 0, 0, 10}}};
    for (const auto& [version, savingProduct, major, minor, maint, build] : versions) {
        const auto path = std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence" / version / (std::string(version) + "-baseline.mus");
        const auto result = Reader::read<TestXmlDocument>(path);
        expect(result.report.formatEpoch == FormatEpoch::DclLegacy,
            std::string(version) + " fixture was not classified as DCL");
        expect(result.report.byteOrder == ByteOrder::BigEndian,
            std::string(version) + " fixture byte order was not recovered");
        expect(result.report.sourcePlatform == SourcePlatform::MacOS,
            std::string(version) + " fixture platform was not recovered");
        expect(result.report.savingProduct == savingProduct,
            std::string(version) + " saving product was not recovered");
        expect(result.report.sourceVersion
            && result.report.sourceVersion->major == major
            && result.report.sourceVersion->minor == minor
            && result.report.sourceVersion->maint == maint
            && result.report.sourceVersion->build == build,
            std::string(version) + " embedded Finale version was not recovered");
        expect(field(result, "options.musicSpacing.minWidth").origin == ValueOrigin::LegacyMus,
            std::string(version) + " music spacing options were not recovered");
        expectNoScoreContent(result);
    }
}

void testUncompressedEpochAndOverlays()
{
    const auto result = Reader::read<TestXmlDocument>(makeUncompressedMus());
    expect(result.report.formatEpoch == FormatEpoch::UncompressedLegacy,
        "Synthetic Finale 2000 file was not classified as uncompressed legacy");
    expect(result.report.byteOrder == ByteOrder::LittleEndian,
        "Synthetic Windows byte order was not detected");
    expect(result.report.sourcePlatform == SourcePlatform::Windows,
        "Synthetic Windows platform was not recovered");
    expect(result.report.defaultsPlatform == SourcePlatform::Windows,
        "Windows-sourced file was not seeded from the Windows baseline");
    // The tuple is stored in the file's byte order, so a little-endian file holds it
    // reversed. Reading it in file order would report the build as the major version.
    expect(result.report.sourceVersion
        && result.report.sourceVersion->major == 12
        && result.report.sourceVersion->minor == 3
        && result.report.sourceVersion->maint == 4
        && result.report.sourceVersion->build == 5,
        "Little-endian header version was not decoded in the file's byte order");
    const auto layer = result.document->getOthers()
        ->get<musx::dom::others::LayerAttributes>(musx::dom::SCORE_PARTID, 3);
    expect(layer && layer->restOffset == -14, "Legacy layer attribute overlay failed");
    const auto spacing = result.document->getOptions()
        ->get<musx::dom::options::MusicSpacingOptions>();
    expect(spacing->minWidth == 361 && spacing->maxWidth == 1801,
        "Uncompressed music spacing overlay failed");
    expect(field(result, "others.layerAtts[3].restOffset").origin == ValueOrigin::LegacyMus,
        "Layer overlay was not reported as recovered");
    expectCompleteOptions(result);
    expectNoScoreContent(result);
}

// Files older than the ENIGMA signature open with a plain-text product banner and
// reserve the same 0x200 header; the body's second word repeats the body offset.
std::vector<std::uint8_t> makeCodaBannerMus()
{
    std::vector<std::uint8_t> result(0x220);
    constexpr std::string_view banner = "Finale(TM) 2.6 Copyright 1987 by Coda. All rights reserved.";
    writeFixed(result, 0, banner, banner.size());
    // Every surveyed file of this era carries this constant pair and nothing else in
    // the 0x60-0x200 region; there is no version tuple to recover.
    result[0x80] = 0x01;
    result[0x81] = 0x03;
    result[0x203] = 0x2b;
    result[0x206] = 0x02;
    return result;
}

void testCodaBannerEpoch()
{
    const auto result = Reader::read<TestXmlDocument>(makeCodaBannerMus());
    expect(result.report.formatEpoch == FormatEpoch::CodaBanner,
        "Synthetic Coda-banner file was not classified");
    expect(result.report.savingProduct == "2.6",
        "Coda-banner product was not recovered from the banner text");
    expect(result.report.sourceVersion
        && result.report.sourceVersion->major == 2 && result.report.sourceVersion->minor == 6,
        "Coda-banner version was not recovered from the banner text");
    expect(result.report.sourceVersion->raw == 0,
        "A Coda-banner version was reported as though it came from a header tuple");
    expect(result.report.byteOrder == ByteOrder::BigEndian,
        "Coda-banner byte order was not classified");
    // This era records its version only in the product banner, and that names the
    // application that wrote the file, so it belongs to the last saver. Nothing identifies
    // the creator, so that block stays empty rather than repeating a version it never held.
    const auto& header = *result.document->getHeader();
    expect(header.modified.finaleVersion.major == 2 && header.modified.finaleVersion.minor == 6,
        "The Coda-banner version did not reach the document header");
    expect(header.created.finaleVersion.major == 0,
        "A creator version was invented for a file that records none");
    expect(result.report.sourcePlatform == SourcePlatform::Unknown
        && result.report.defaultsPlatform == SourcePlatform::MacOS,
        "An unknown source platform did not fall back to the macOS baseline");
    expect(!result.report.warnings.empty(), "Coda-banner limitation was not reported");
    expect(field(result, "options.musicSpacing.minWidth").origin
        == ValueOrigin::Finale27Default,
        "Unsupported Coda-banner option was not retained as a default");
    expectCompleteOptions(result);
    expectNoScoreContent(result);
}

void testZlibEpoch()
{
    const auto result = Reader::read<TestXmlDocument>(makeZlibMus());
    expect(result.report.formatEpoch == FormatEpoch::ZlibLegacy,
        "Synthetic Finale 2012 file was not classified as zlib legacy");
    expect(result.report.blocks.size() == 5, "Synthetic zlib block count is incorrect");
    expect(result.report.blocks.front().decodedSize == 3,
        "Synthetic zlib decoded size is incorrect");
    expect(field(result, "options.musicSpacing.minWidth").origin
        == ValueOrigin::Finale27Default,
        "Unsupported zlib-era option was not retained as a default");
    expect(!result.report.warnings.empty(), "Zlib-era overlay limitation was not reported");
    expectNoScoreContent(result);
}

void testMalformedInput()
{
    bool threw = false;
    try {
        static_cast<void>(
            Reader::read<TestXmlDocument>(std::vector<std::uint8_t>{1, 2, 3, 4}));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expect(threw, "Arbitrary input was accepted as a MUS file");
}

} // namespace

TEST_CASE("Controlled DCL file", "[reader]") { testControlledDclFile(); }
TEST_CASE("Font definitions", "[reader]") { testFontDefinitions(); }
TEST_CASE("Uncompressed fixtures", "[reader]") { testUncompressedFixtures(); }
TEST_CASE("Class record era", "[reader]") { testClassRecordEra(); }
TEST_CASE("Big-endian class records", "[reader]") { testBigEndianClassRecords(); }
TEST_CASE("Finale 2006 remains fixed-row", "[reader]")
{
    testFinale2006RemainsFixedRow();
}
TEST_CASE("Independent imported documents", "[reader]")
{
    testIndependentImportedDocuments();
}
TEST_CASE("Controlled DCL versions", "[reader]") { testControlledDclVersions(); }
TEST_CASE("Uncompressed epoch and overlays", "[reader]")
{
    testUncompressedEpochAndOverlays();
}
TEST_CASE("Coda banner epoch", "[reader]") { testCodaBannerEpoch(); }
TEST_CASE("Zlib epoch", "[reader]") { testZlibEpoch(); }
TEST_CASE("Malformed input", "[reader]") { testMalformedInput(); }
