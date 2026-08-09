// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
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

std::vector<std::uint8_t> makeBanner(std::string_view product,
    std::string_view platform)
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
        result[tupleOffset] = 12;
        result[tupleOffset + 1] = 3;
        result[tupleOffset + 2] = 4;
        result[tupleOffset + 3] = 5;
        writeFixed(result, tupleOffset + 4, "FIN", 4);
        writeFixed(result, tupleOffset + 8, platform, 4);
        result[tupleOffset + 12] = 12;
        result[tupleOffset + 13] = 4;
        result[tupleOffset + 16] = 12;
        result[tupleOffset + 17] = 5;
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
    output.push_back(static_cast<std::uint8_t>(tag[0]));
    output.push_back(static_cast<std::uint8_t>(tag[1]));
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
    auto result = makeBanner("2000", "WIN");
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
    auto result = makeBanner("2012", "MAC");
    appendZlibBlock(result, 0x001a, {1, 2, 3}, byteOrder);
    appendZlibBlock(result, 0x001b, {4, 5}, byteOrder);
    appendZlibBlock(result, 0x0016, {6}, byteOrder);
    appendZlibBlock(result, 0x0017, {7, 8, 9}, byteOrder);
    write16(result, 0x001d, byteOrder);
    write32(result, 6, byteOrder);
    return result;
}

const FieldInfo& field(const ImportResult& result, const std::string& target)
{
    const auto found = std::find_if(result.report.fields.begin(), result.report.fields.end(),
        [&](const FieldInfo& value) { return value.target == target; });
    expect(found != result.report.fields.end(), "Missing field report for " + target);
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
    expect(result.document->getOthers()->getArray<others::FontDefinition>(SCORE_PARTID).empty(),
        "Output contains fallback font definitions");
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
    expect(header.created.finaleVersion.major == 7 && header.created.finaleVersion.minor == 1,
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

void testControlledDclVersions()
{
    const std::array<std::pair<std::string_view, std::string_view>, 4> versions{{
        {"F2002", "2002"}, {"F2003", "2003"}, {"F2004", "2004b"}, {"F2005", "2005"}}};
    for (const auto& [version, savingProduct] : versions) {
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

void testPreBannerEpoch()
{
    std::vector<std::uint8_t> data(0x220);
    data[3] = 0x2e;
    data[6] = 0x02;
    const auto result = Reader::read<TestXmlDocument>(data);
    expect(result.report.formatEpoch == FormatEpoch::PreBanner,
        "Synthetic pre-banner file was not classified");
    expect(result.report.byteOrder == ByteOrder::Unknown,
        "Unresolved pre-banner byte order was overstated");
    expect(result.report.sourcePlatform == SourcePlatform::Unknown
        && result.report.defaultsPlatform == SourcePlatform::MacOS,
        "An unknown source platform did not fall back to the macOS baseline");
    expect(!result.report.warnings.empty(), "Pre-banner limitation was not reported");
    expect(field(result, "options.musicSpacing.minWidth").origin
        == ValueOrigin::Finale27Default,
        "Unsupported pre-banner option was not retained as a default");
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

namespace finale_mus_reader_tests {
void runMappingTests();
} // namespace finale_mus_reader_tests

int main()
{
    try {
        finale_mus_reader_tests::runMappingTests();
        testControlledDclFile();
        testIndependentImportedDocuments();
        testControlledDclVersions();
        testUncompressedEpochAndOverlays();
        testPreBannerEpoch();
        testZlibEpoch();
        testMalformedInput();
        std::cout << "All Legacy Finale MUS Reader tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
