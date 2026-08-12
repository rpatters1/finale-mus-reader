// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "container/product_banner.h"
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <set>
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

// An uncompressed-era file carrying a selector 24 default-font array. Its banner version is
// major 12, deliberately outside the Finale 3.0 through 2002 range, because that is the shape
// of the three real Finale 3.0 documents whose header recovers a major version of 15: the
// epoch has to carry the semantic layout when the version cannot.
std::vector<std::uint8_t> makeUncompressedMusWithFontOptions()
{
    constexpr auto byteOrder = ByteOrder::LittleEndian;
    auto result = makeBanner("2000", "WIN", byteOrder);
    std::vector<std::uint8_t> others;
    // One font definition so the recovered ids resolve.
    appendOther(others, 0, "FN", words(0x1fff, 0, 0, 0, 0, 0), byteOrder);
    appendOther(others, 0, "FN", words(0x4d61, 0x6573, 0x7472, 0x6f00, 0, 0), byteOrder);
    // Two incidences of selector 24: four three-word tuples, music/key/clef/time.
    appendOther(others, 0xfffe, "24", words(0, 28, 0, 0, 26, 0), byteOrder);
    appendOther(others, 0xfffe, "24", words(0, 24, 0, 0, 22, 1), byteOrder);
    appendUncompressedBlock(result, 1, others, byteOrder);
    appendUncompressedBlock(result, 2, {}, byteOrder);
    appendUncompressedBlock(result, 3, {}, byteOrder);
    appendUncompressedBlock(result, 4, {}, byteOrder);
    return result;
}

// A Coda-banner document in either byte order. The era has no block framing to trial, so the
// container takes its order from the banner product: a `PC` product is a Windows document and
// little-endian, anything else is big-endian. Both are built here from one description so the
// two paths cannot drift apart, and so that neither needs a committed fixture -- the only real
// Windows documents of this era are Coda's own installer templates.
std::vector<std::uint8_t> makeCodaBannerMus(ByteOrder byteOrder, std::string_view product)
{
    std::vector<std::uint8_t> result(0x200, 0);
    const std::string banner = "Finale(TM) " + std::string(product)
        + " Copyright 1987 by Coda. All rights reserved.";
    writeFixed(result, 0, banner, banner.size());

    // The era's eight clefs are one single-incidence global each, selectors 28 through 35,
    // holding middle-C position, an unexplained word, the clef character and the staff
    // position. These are the values every surveyed document of the era carries.
    std::vector<std::uint8_t> pool;
    const std::array<std::array<std::int16_t, 4>, 8> clefs{{{-10, 6, 38, -6}, {-4, 0, 66, -4},
        {-2, -2, 66, -2}, {2, -6, 63, -2}, {-10, 6, 214, -4}, {-3, -1, 86, -6},
        {9, -13, 116, -2}, {0, -4, 63, -4}}};
    for (std::size_t i = 0; i < clefs.size(); ++i) {
        const char tag[2] = {static_cast<char>('0' + (28 + i) / 10),
            static_cast<char>('0' + (28 + i) % 10)};
        appendOther(pool, 0xfffe, std::string_view(tag, 2),
            words(clefs[i][0], clefs[i][1], clefs[i][2], clefs[i][3], 0, 0), byteOrder);
    }
    // Scalars the era does record: the default clef, the end-of-measure percent and offset,
    // and the spacing before and after a clef.
    appendOther(pool, 0xfffe, "01", words(0, 0, 0, 0, 0, 0), byteOrder);
    appendOther(pool, 0xfffe, "13", words(4, 24, 75, -12, 0, 0), byteOrder);
    appendOther(pool, 0xfffe, "19", words(24, 0, 0, 0, 0, 0), byteOrder);
    pool.resize(0x200, 0);

    // One pool: a page count and a page size, then that many 512-byte pages. The page size is
    // what confirms the era, and it reads 0x200 only in the file's own order.
    write32(result, 1, byteOrder);
    write32(result, 0x200, byteOrder);
    result.insert(result.end(), pool.begin(), pool.end());
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

// The same file, but its terminal block carries an embedded graphic instead of being an
// empty marker. A stored block has no checksum word and its payload starts right after the
// six-byte header, so writing one is not appendZlibBlock with compression turned off.
std::vector<std::uint8_t> makeZlibMusWithGraphic(
    std::uint16_t terminalType, std::string_view graphic)
{
    constexpr auto byteOrder = ByteOrder::LittleEndian;
    auto result = makeBanner("2012", "MAC", byteOrder);
    appendZlibBlock(result, 0x001a, {1, 2, 3}, byteOrder);
    appendZlibBlock(result, 0x001b, {4, 5}, byteOrder);
    appendZlibBlock(result, 0x0016, {6}, byteOrder);
    appendZlibBlock(result, 0x0017, {7, 8, 9}, byteOrder);
    write16(result, terminalType, byteOrder);
    write32(result, static_cast<std::uint32_t>(graphic.size() + 6), byteOrder);
    result.insert(result.end(), graphic.begin(), graphic.end());
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

// Whether the report carries any diagnostic at the given level. Tests assert the level a
// message was raised at, not merely that some message exists: the whole point of the level
// is that a routine fallback and an unreadable document must not look alike to a host.
bool hasDiagnostic(const finale_mus_reader::ImportReport& report,
    musx::util::Logger::LogLevel level)
{
    return std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
        [&](const finale_mus_reader::Diagnostic& entry) { return entry.level == level; });
}

template <typename T>
void expectOption(const ImportResult& result)
{
    expect(static_cast<bool>(result.document->getOptions()->get<T>()),
        "Pinned default omitted an expected options instance");
}

// The imported options pool must be structurally complete whatever era the source is.
// Most of these instances are seeded from the pinned baseline; FontOptions and ClefOptions
// are filtered out of that seeding and rebuilt, so their presence here is a check that the
// rebuild ran rather than that the baseline was copied.
void expectCompleteOptionsPool(const ImportResult& result)
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
    // allowlisted. Font definitions cloned individually to resolve synthesized FontOptions
    // are intentional and do not constitute leaked baseline score content.
    expect(result.document->getOthers()->getArray<others::MarkingCategory>(SCORE_PARTID).empty(),
        "Output contains fallback marking categories");
    // Shapes are no longer absent. The two tablature clef definitions completed from the
    // baseline are drawn as shapes, and those shapes are copied in so the clefs render. Nothing
    // else may ride along, so every shape present must be one a clef definition names.
    {
        std::set<musx::dom::Cmper> referencedShapes;
        if (const auto clefs = result.document->getOptions()
                ->get<musx::dom::options::ClefOptions>()) {
            for (const auto& def : clefs->clefDefs) {
                if (def->isShape && def->shapeId != 0) {
                    referencedShapes.insert(def->shapeId);
                }
            }
        }
        for (const auto& shape : result.document->getOthers()
                ->getArray<others::ShapeDef>(SCORE_PARTID)) {
            expect(referencedShapes.count(shape->getCmper()) != 0,
                "Output contains a baseline shape no clef definition references");
        }
    }
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
    expectCompleteOptionsPool(result);
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

// Font definitions come from the file except when a missing FontOptions type needs a
// baseline face that is not already present by normalized name. The controlled fixture's
// ETF prints the nine source definitions, so those names and character sets are ground truth:
// ^FN(0) 8191 0 0 0 0 0 with ^FN(0) "Maestro", where 8191 is 0x1fff, a Mac symbol font.
void testFontDefinitions()
{
    const auto path = std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
        / "evidence/F2002/F2002-baseline.mus";
    const auto result = Reader::read<TestXmlDocument>(path);
    using musx::dom::others::FontDefinition;
    const auto fonts = result.document->getOthers()
        ->getArray<FontDefinition>(musx::dom::SCORE_PARTID);
    // Nine source definitions, plus two introduced from the baseline: one for a FontOptions type
    // this source does not store, and one for the typeface the copied tablature clef shapes draw
    // their character in. A shape that names a font the target lacks brings that font with it.
    expect(fonts.size() == 11, "F2002 font table plus required fallback fonts is incorrect");

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

    expect(fontAt(9)->name == "Times New Roman",
        "The unmatched fallback face did not retain the reference spelling");
}

// FontOptions is a variable-length, versioned source collection. Recovered semantic tuples
// override a complete 45-type baseline whose nonzero font ids are remapped by name.
void testFontOptionsCapture()
{
    using FontOptions = musx::dom::options::FontOptions;
    using FontType = FontOptions::FontType;
    const auto read = [](const char* relative) {
        return Reader::read<TestXmlDocument>(
            std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / relative);
    };

    const auto f2002 = read("evidence/F2002/F2002-baseline.mus");
    const auto fixed = f2002.document->getOptions()->get<FontOptions>();
    expect(fixed && fixed->fontOptions.size() == 45,
        "Finale 2002 font options were not completed to the modern type set");
    const auto tuplet = fixed->getFontInfo(FontType::Tuplet);
    expect(tuplet->fontId == 1 && tuplet->fontSize == 10,
        "A fixed-row font-options tuple was not captured");
    expect(tuplet->bold && tuplet->italic && !tuplet->underline,
        "Font option effects were not expanded into musxdom booleans");
    expect(field(f2002, "others.fontName[0].name").origin == ValueOrigin::LegacyMus,
        "A record-created font definition did not retain its comparator in diagnostics");
    expect(field(f2002, "options.fontOptions[7].effects").rawValue == 3,
        "The raw fixed-row effects mask was not reported");
    expect(field(f2002, "options.fontOptionsPhysical[13].fontId").origin
            == ValueOrigin::LegacyMus,
        "The Finale 2002 drawing-time tablature slot was not retained as physical evidence");
    expect(field(f2002, "options.fontOptions[13].fontId").origin == ValueOrigin::LegacyMus,
        "Finale 2002 physical slot 28 was not mapped to semantic tablature");
    expect(field(f2002, "options.fontOptions[28].fontId").origin
            == ValueOrigin::Finale27Default,
        "Finale 2002 percussion was not supplied by the baseline");
    expect(field(f2002, "options.fontOptions[40].fontId").origin
            == ValueOrigin::Finale27Default,
        "A modern bend font absent from Finale 2002 was not synthesized");
    expect(fixed->getFontInfo(FontType::TimeParts)->fontId == 0,
        "A synthesized baseline font id 0 did not pass through unchanged");

    const auto f2005 = read("evidence/F2005/F2005-baseline.mus");
    const auto laterFixed = f2005.document->getOptions()->get<FontOptions>();
    expect(laterFixed && laterFixed->fontOptions.size() == 45,
        "Finale 2005 font options were not completed to the modern type set");
    expect(field(f2005, "options.fontOptionsPhysical[43].fontId").rawValue == 0
            && field(f2005, "options.fontOptionsPhysical[43].fontSize").rawValue == 0
            && field(f2005, "options.fontOptionsPhysical[43].effects").rawValue == 0,
        "The Finale 2005 structural-fill tuple was not retained as physical evidence");
    expect(field(f2005, "options.fontOptions[43].fontId").origin
            == ValueOrigin::Finale27Default
            && field(f2005, "options.fontOptions[44].fontId").origin
                == ValueOrigin::Finale27Default,
        "Finale 2005 time-parts fonts were not supplied by the baseline");

    const auto f2007 = read("evidence/F2007/F2007-lyric-hyphens.mus");
    const auto zlib = f2007.document->getOptions()->get<FontOptions>();
    expect(zlib && zlib->fontOptions.size() == 45,
        "The zlib font-options payload did not populate its live tuple range");
    const auto zlibTuplet = zlib->getFontInfo(FontType::Tuplet);
    expect(zlibTuplet->fontId == tuplet->fontId
            && zlibTuplet->fontSize == tuplet->fontSize
            && zlibTuplet->getEnigmaStyles() == tuplet->getEnigmaStyles(),
        "The zlib tuple layout disagrees with the fixed-row layout");
    expect(field(f2007, "options.fontOptionsPhysical[45].fontId").rawValue == 0
            && field(f2007, "options.fontOptionsPhysical[45].fontSize").rawValue == 0
            && field(f2007, "options.fontOptionsPhysical[45].effects").rawValue == 0,
        "The terminal physical zlib tuple was not captured in the report");

    // The 13/28 renumbering happens at Finale 2012, not Finale 2003 as the documentation
    // said. Finale 2007 is inside the zlib epoch but before the boundary, so it must still
    // take the earlier layout: tablature comes from physical 28, and percussion is not
    // stored at all. Pinning this side matters more than the modern side, because the
    // previous code got precisely this wrong for every 2003-2011 document.
    expect(field(f2007, "options.fontOptions[13].fontId").origin == ValueOrigin::LegacyMus,
        "Finale 2007 tablature was not recovered from physical slot 28");
    expect(field(f2007, "options.fontOptions[28].fontId").origin
            == ValueOrigin::Finale27Default,
        "Finale 2007 percussion was not left to the baseline; it is not stored before 2012");

    const auto f2012 = read("evidence/F2012/F2012-upstem-flags.mus");
    const auto littleEndianZlib = f2012.document->getOptions()->get<FontOptions>();
    expect(littleEndianZlib && littleEndianZlib->fontOptions.size() == 45,
        "The little-endian zlib font-options payload was not captured");
    // The far side of the same boundary: Finale 2012 stores both, at the modern ordinals.
    expect(field(f2012, "options.fontOptions[13].fontId").origin == ValueOrigin::LegacyMus
            && field(f2012, "options.fontOptions[28].fontId").origin
                == ValueOrigin::LegacyMus,
        "Finale 2012 did not recover both tablature and percussion from stored tuples");
    const auto music = littleEndianZlib->getFontInfo(FontType::Music);
    expect(music->fontId == 0 && music->fontSize == 24
            && field(f2012, "options.fontOptions[0].fontSize").rawValue == 24,
        "A little-endian zlib font-options word was byte-swapped incorrectly");

    const auto f100Baseline = read("evidence/F100/F100-baseline.mus");
    const auto earlyBaseline = f100Baseline.document->getOptions()->get<FontOptions>();
    expect(earlyBaseline && earlyBaseline->fontOptions.size() == 45,
        "Finale 1.0.0 font options were not completed");
    expect(earlyBaseline->getFontInfo(FontType::Music)->fontId == 0
            && earlyBaseline->getFontInfo(FontType::Music)->fontSize == 71,
        "Finale 1.0.0 music font tuple was not recovered");
    expect(earlyBaseline->getFontInfo(FontType::TextBlock)->fontId == 1
            && earlyBaseline->getFontInfo(FontType::TextBlock)->fontSize == 12,
        "Finale 1.0.0 text-block font tuple was not recovered");
    expect(earlyBaseline->getFontInfo(FontType::LyricVerse)->fontId == 1
            && earlyBaseline->getFontInfo(FontType::LyricVerse)->fontSize == 12,
        "Finale 1.0.0 lyric-verse font tuple was not recovered");

    const auto f100Music = read("evidence/F100/F100-music-font.mus");
    const auto earlyMusic = f100Music.document->getOptions()->get<FontOptions>()
        ->getFontInfo(FontType::Music);
    expect(earlyMusic->fontId == 12 && earlyMusic->fontSize == 60
            && earlyMusic->getEnigmaStyles() == 0,
        "The controlled Finale 1.0.0 music-font edit was not recovered");

    const auto f263Baseline = read("evidence/F263/F263-baseline.mus");
    const auto f263Music = read("evidence/F263/F263-music-font.mus");
    const auto f263BaselineOptions = f263Baseline.document->getOptions()->get<FontOptions>();
    const auto f263MusicOptions = f263Music.document->getOptions()->get<FontOptions>();
    expect(f263BaselineOptions && f263MusicOptions
            && f263BaselineOptions->fontOptions.size() == 45
            && f263MusicOptions->fontOptions.size() == 45,
        "Finale 2.6.3 font options were not completed");
    const auto f263ChangedMusic = f263MusicOptions->getFontInfo(FontType::Music);
    expect(f263ChangedMusic->fontId == 28 && f263ChangedMusic->fontSize == 24
            && f263ChangedMusic->italic,
        "The controlled Finale 2.6.3 music-font edit was not recovered");

    // Finale 27 derives a JazzPerc percussion preference when it upgrades the changed
    // fixture, but the MUS file contains no independently sourced percussion preference.
    // It therefore remains the selected platform reference value in both imports.
    for (const auto* result : {&f263Baseline, &f263Music}) {
        const auto percussion = result->document->getOptions()->get<FontOptions>()
            ->getFontInfo(FontType::Percussion);
        expect(percussion->fontId == 77 && percussion->fontSize == 24
                && percussion->getEnigmaStyles() == 0
                && percussion->getName() == "Maestro Percussion",
            "Pre-2003 percussion did not retain the reference FontOptions value");
        expect(field(*result, "options.fontOptions[28].fontId").origin
                == ValueOrigin::Finale27Default,
            "Pre-2003 percussion was reported as though it came from the MUS file");
    }

    const auto f100Text = read("evidence/F100/F100-text-font.mus");
    const auto earlyText = f100Text.document->getOptions()->get<FontOptions>()
        ->getFontInfo(FontType::TextBlock);
    expect(earlyText->fontId == 2 && earlyText->fontSize == 17
            && earlyText->bold && earlyText->italic,
        "The controlled Finale 1.0.0 text-font edit was not recovered");

    const auto f100Lyric = read("evidence/F100/F100-lyric-verse.mus");
    const auto earlyLyric = f100Lyric.document->getOptions()->get<FontOptions>()
        ->getFontInfo(FontType::LyricVerse);
    expect(earlyLyric->fontId == 3 && earlyLyric->fontSize == 13
            && earlyLyric->underline
            && field(f100Lyric, "options.fontOptions[9].effects").rawValue == 28,
        "The controlled Finale 1.0.0 lyric-font edit or effects mask was not recovered");

    const auto expectEarlyFont = [&](const char* path, FontType type,
                                     musx::dom::Cmper fontId, int size,
                                     std::uint16_t rawEffects) {
        auto result = read(path);
        const auto font = result.document->getOptions()->get<FontOptions>()->getFontInfo(type);
        expect(font->fontId == fontId && font->fontSize == size,
            std::string("Controlled early font was not recovered from ") + path);
        expect(field(result, "options.fontOptions["
                + std::to_string(static_cast<std::size_t>(type)) + "].effects").rawValue
                == rawEffects,
            std::string("Controlled early effects mask was not reported from ") + path);
        return result;
    };

    const auto f100Accis = expectEarlyFont(
        "evidence/F100/F100-accis.mus", FontType::ChordAcci, 2, 8, 0);
    const auto f100Chord = expectEarlyFont(
        "evidence/F100/F100-chord.mus", FontType::Chord, 3, 9, 8);
    const auto f100Chorus = expectEarlyFont(
        "evidence/F100/F100-chorus.mus", FontType::LyricChorus, 4, 11, 4);
    const auto f100Clef = expectEarlyFont(
        "evidence/F100/F100-clef.mus", FontType::Clef, 4, 33, 4);
    const auto f100Ending = expectEarlyFont(
        "evidence/F100/F100-ending.mus", FontType::Ending, 9, 19, 0);
    const auto f100Key = expectEarlyFont(
        "evidence/F100/F100-key-font.mus", FontType::Key, 4, 13, 2);
    const auto f100Name = expectEarlyFont(
        "evidence/F100/F100-name.mus", FontType::StaffNames, 7, 12, 1);
    // The Coda-banner era has one "Name" preference where Finale 3.0 and later store four
    // separate name tuples, so the single recovered value has to reach all four types.
    // Recovering StaffNames alone would split a document that was never split.
    for (const auto companion : {FontType::AbbrvStaffNames, FontType::GroupNames,
             FontType::AbbrvGroupNames}) {
        const auto font = f100Name.document->getOptions()
            ->get<FontOptions>()->getFontInfo(companion);
        expect(font->fontId == 7 && font->fontSize == 12,
            "The Finale 1.0.0 Name preference did not reach every modern name font type");
        expect(field(f100Name, "options.fontOptions["
                    + std::to_string(static_cast<std::size_t>(companion)) + "].fontId").origin
                == ValueOrigin::LegacyBehavior,
            "A propagated name font was not reported as restored era behavior");
    }
    expect(field(f100Name, "options.fontOptions["
                + std::to_string(static_cast<std::size_t>(FontType::StaffNames))
                + "].fontId").origin == ValueOrigin::LegacyMus,
        "The Name preference itself must still report as recovered from the source");

    // Finale 3.0 stores the four name types separately, so the fan-out must stop at the
    // Coda-banner epoch rather than overwriting three real recovered values.
    {
        const auto f97 = read("evidence/F372/F372-baseline.mus");
        for (const auto companion : {FontType::AbbrvStaffNames, FontType::GroupNames,
                 FontType::AbbrvGroupNames}) {
            expect(field(f97, "options.fontOptions["
                        + std::to_string(static_cast<std::size_t>(companion)) + "].fontId")
                    .origin == ValueOrigin::LegacyMus,
                "The Coda name fan-out leaked into an epoch that stores the types separately");
        }
    }

    const auto f100Section = expectEarlyFont(
        "evidence/F100/F100-section.mus", FontType::LyricSection, 3, 19, 16);
    const auto f100Time = expectEarlyFont(
        "evidence/F100/F100-time.mus", FontType::Time, 4, 17, 16);
    const auto f100Tuplet = expectEarlyFont(
        "evidence/F100/F100-tuplet.mus", FontType::Tuplet, 20, 17, 0);
    expect(f100Tuplet.document->getOptions()->get<FontOptions>()
                ->getFontInfo(FontType::ChordAcci)->fontId == 20,
        "The ChordAcci side effect in the controlled tuplet save was not recovered");

    for (const auto* result : {&f2002, &f2005, &f2007, &f2012,
             &f100Baseline, &f100Music, &f263Baseline, &f263Music,
             &f100Text, &f100Lyric,
             &f100Accis, &f100Chord, &f100Chorus, &f100Clef, &f100Ending,
             &f100Key, &f100Name, &f100Section, &f100Time, &f100Tuplet}) {
        const auto options = result->document->getOptions()->get<FontOptions>();
        for (const auto& [type, font] : options->fontOptions) {
            (void)type;
            expect(static_cast<bool>(result->document->getOthers()
                    ->get<musx::dom::others::FontDefinition>(
                        musx::dom::SCORE_PARTID, font->fontId)),
                "A completed font option has a dangling font id");
        }
    }
}

// Clef definitions are a numeric global like any other option, but their collection has
// changed size twice and their tuple once. Each fixture below is one of those layouts.
void testClefOptionsCapture()
{
    using ClefOptions = musx::dom::options::ClefOptions;
    const auto read = [](const char* relative) {
        return Reader::read<TestXmlDocument>(
            std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / relative);
    };
    const auto clefs = [](const ImportResult& result) {
        const auto options = result.document->getOptions()->get<ClefOptions>();
        expect(static_cast<bool>(options), "The imported document has no clef options");
        expect(options->clefDefs.size() == 18,
            "Clef definitions were not completed to the modern collection size");
        return options;
    };
    // Every era stores these three for its first clef, so agreement across all of them is
    // what shows the four physical layouts describe one logical table.
    const auto expectTreble = [](const auto& options, const char* era) {
        const auto treble = options->getClefDef(0);
        expect(treble->middleCPos == -10 && treble->clefChar == 38
                && treble->staffPosition == -6,
            std::string("The treble clef definition was not recovered from ") + era);
    };

    // Finale 2002: selector 95, 24 incidences, sixteen nine-word tuples. The last two
    // definitions did not exist yet and come from the baseline.
    const auto f2002 = read("evidence/F2002/F2002-baseline.mus");
    const auto f2002Clefs = clefs(f2002);
    expectTreble(f2002Clefs, "Finale 2002");
    expect(field(f2002, "options.clefOptions.clefDefs[0].middleCPos").origin
            == ValueOrigin::LegacyMus,
        "The Finale 2002 clef table was not reported as recovered");
    expect(f2002Clefs->getClefDef(15)->middleCPos == -10
            && f2002Clefs->getClefDef(15)->clefChar == 0
            && f2002Clefs->getClefDef(15)->staffPosition == -6,
        "The last stored Finale 2002 clef definition was not recovered");
    expect(field(f2002, "options.clefOptions.clefDefs[16].shapeId").origin
            == ValueOrigin::Finale27Default
            && field(f2002, "options.clefOptions.clefDefs[17].shapeId").origin
                == ValueOrigin::Finale27Default,
        "The two clef definitions Finale 2002 lacks were not reported as synthesized");
    expect(f2002Clefs->getClefDef(16)->isShape && f2002Clefs->getClefDef(16)->scaleToStaffHeight,
        "A synthesized shape clef lost its shape flags");
    // The scalars around the collection, verified against the controlled ETF and its
    // exact Finale 27 companion.
    expect(f2002Clefs->clefChangePercent == 75 && f2002Clefs->clefChangeOffset == -8
            && f2002Clefs->clefFrontSepar == 24 && f2002Clefs->clefBackSepar == 0
            && f2002Clefs->clefKeySepar == 0 && f2002Clefs->clefTimeSepar == 0
            && f2002Clefs->defaultClef == 0 && !f2002Clefs->showClefFirstSystemOnly,
        "The Finale 2002 clef scalars were not recovered");
    expect(field(f2002, "options.clefOptions.clefFrontSepar").origin == ValueOrigin::LegacyMus
            && field(f2002, "options.clefOptions.clefChangePercent").origin
                == ValueOrigin::LegacyMus,
        "Recovered clef scalars were reported as synthesized defaults");
    expect(f2002Clefs->cautionaryClefChanges
            && field(f2002, "options.clefOptions.cautionaryClefChanges").origin
                == ValueOrigin::LegacyMus,
        "cautionaryClefChanges was not recovered from the courtesy-flags word");

    // Finale 2005: the same tuple, but 27 incidences, so the collection is complete in
    // the source and nothing is synthesized.
    const auto f2005 = read("evidence/F2005/F2005-baseline.mus");
    const auto f2005Clefs = clefs(f2005);
    expectTreble(f2005Clefs, "Finale 2005");
    expect(field(f2005, "options.clefOptions.clefDefs[17].shapeId").origin
            == ValueOrigin::LegacyMus,
        "The Finale 2005 source stores eighteen clefs and should synthesize none");
    expect(f2005Clefs->getClefDef(16)->isShape && f2005Clefs->getClefDef(16)->shapeId == 2
            && f2005Clefs->getClefDef(16)->scaleToStaffHeight
            && !f2005Clefs->getClefDef(16)->useOwnFont,
        "The Finale 2005 shape-clef flags were not expanded from the packed word");

    // Finale 2007: the same nine-word tuple carried by a big-endian class record.
    const auto f2007 = read("evidence/F2007/F2007-lyric-hyphens.mus");
    const auto f2007Clefs = clefs(f2007);
    expectTreble(f2007Clefs, "Finale 2007");
    expect(f2007Clefs->getClefDef(13)->middleCPos == -17
            && f2007Clefs->getClefDef(13)->clefChar == 160,
        "A later big-endian class-record clef definition was not recovered");
    expect(f2007Clefs->getClefDef(17)->isShape && f2007Clefs->getClefDef(17)->shapeId == 3,
        "The big-endian class-record shape clef was not recovered");

    // Finale 2012: little-endian, and the clef character is a long because that release
    // introduced Unicode text. Reading it as the narrow tuple would shift every slot after
    // the character and yield twenty definitions instead of eighteen.
    const auto f2012 = read("evidence/F2012/F2012-upstem-flags.mus");
    const auto f2012Clefs = clefs(f2012);
    expectTreble(f2012Clefs, "Finale 2012");
    expect(f2012Clefs->getClefDef(12)->clefChar == 139
            && f2012Clefs->getClefDef(12)->staffPosition == -6,
        "The Finale 2012 long clef character was not assembled from its two words");
    expect(f2012Clefs->getClefDef(16)->isShape && f2012Clefs->getClefDef(16)->shapeId == 2
            && f2012Clefs->getClefDef(17)->shapeId == 3,
        "The wide-tuple shape clefs were not read at their shifted slots");

    // Finale 2000: eight separate globals, selectors 28 through 35. This fixture stores
    // clef character 32 for its fourth clef where every other era stores 214, and the
    // exact Finale 27 companion carries that 32 through, so it is a real stored value
    // rather than an absent one.
    const auto f2000 = read("evidence/F2000/F2000-multilayer.mus");
    const auto f2000Clefs = clefs(f2000);
    expectTreble(f2000Clefs, "Finale 2000");
    expect(f2000Clefs->getClefDef(4)->clefChar == 32
            && f2000Clefs->getClefDef(4)->middleCPos == -10
            && f2000Clefs->getClefDef(4)->staffPosition == -4,
        "The distinctive Finale 2000 alto clef character was not recovered");
    expect(field(f2000, "options.clefOptions.clefDefs[7].middleCPos").origin
            == ValueOrigin::LegacyMus
            && field(f2000, "options.clefOptions.clefDefs[8].middleCPos").origin
                == ValueOrigin::Finale27Default,
        "The pre-2001 boundary between eight stored and ten synthesized clefs moved");

    // Finale 1.0.0: the same eight selectors. Its seventh clef stores -5 where every later
    // era stores 9, and the exact Finale 27 companion preserves the -5, which is what
    // shows the value is read from the file rather than defaulted.
    const auto f100 = read("evidence/F100/F100-baseline.mus");
    const auto f100Clefs = clefs(f100);
    expectTreble(f100Clefs, "Finale 1.0.0");
    expect(f100Clefs->getClefDef(6)->middleCPos == -5
            && f100Clefs->getClefDef(6)->clefChar == 116,
        "The Finale 1.0.0 percussion clef definition was not recovered");
    expect(f100Clefs->getClefDef(4)->clefChar == 214,
        "The Finale 1.0.0 alto clef character was not recovered");

    // The clef baseline adjustment, from three controlled one-variable saves. It is the one
    // clef field the corpus could not exercise: every unedited document leaves it zero.
    //
    // Finale 2005 stores Efix directly. The fixture asked for one inch on the treble clef,
    // which is 18432 Efix, and its exact Finale 27 companion carries that number unchanged.
    // The second edit asked for minus two inches, which does not fit a signed word, so the
    // stored value saturated; recovering -32768 rather than -36864 is the file being read
    // correctly, not a decoding error.
    const auto f2005Baseline = read("evidence/F2005/F2005-clef-baseline.mus");
    const auto f2005BaselineClefs = clefs(f2005Baseline);
    expect(f2005BaselineClefs->getClefDef(0)->baselineAdjust == 18432,
        "The Finale 2005 clef baseline adjustment was not recovered as Efix");
    expect(f2005BaselineClefs->getClefDef(1)->baselineAdjust == -32768,
        "The saturated Finale 2005 baseline adjustment was not recovered verbatim");
    expect(f2005BaselineClefs->getClefDef(2)->baselineAdjust == 0,
        "An unedited clef did not keep a zero baseline adjustment");
    expect(field(f2005Baseline, "options.clefOptions.clefDefs[0].baselineAdjust").rawValue
            == 18432,
        "The stored baseline word was not reported for the Efix era");

    // The pre-2001 eras store the same setting as a small signed count of harmonic levels,
    // in word 4 of the clef's own selector, and scale it into Efix. Word 5 of the first
    // clef's selector is a document-wide switch: with it clear the stored counts are inert,
    // and Finale 27 discards them. Every assertion below matches the exact companion.
    constexpr int efixPerHarmonicLevel = 768;
    const auto expectEarlyBaseline = [&](const char* path, const std::vector<int>& expected,
                                         const char* era) {
        const auto result = read(path);
        const auto options = clefs(result);
        for (std::size_t i = 0; i < expected.size(); ++i) {
            expect(options->getClefDef(musx::dom::ClefIndex(i))->baselineAdjust == expected[i],
                std::string("The clef baseline adjustment was wrong for ") + era);
        }
        // The edit must not disturb the fields that share the record.
        expect(options->getClefDef(0)->middleCPos == -10
                && options->getClefDef(0)->clefChar == 38
                && options->getClefDef(0)->staffPosition == -6,
            std::string("A baseline edit changed neighbouring clef fields in ") + era);
        return result;
    };

    // Finale 3.7.2 with the switch on. All eight clefs convert, including ones this save
    // never touched, which is what shows the switch is per document rather than per clef.
    const auto f372Baseline = expectEarlyBaseline("evidence/F372/F372-clef-baseline.mus",
        {1 * efixPerHarmonicLevel, -2 * efixPerHarmonicLevel, -5 * efixPerHarmonicLevel},
        "Finale 3.7.2 with the switch on");
    expect(field(f372Baseline, "options.clefOptions.clefDefs[0].baselineAdjust").rawValue == 1,
        "The raw harmonic-level count was not reported for Finale 3.7.2");
    // The same document with the switch off. Its clefs still carry -2, -4 and -5, so a
    // reader that ignored the switch would produce three offsets Finale never applied.
    expectEarlyBaseline("evidence/F372/F372-baseline.mus", {0, 0, 0},
        "Finale 3.7.2 with the switch off");
    expect(field(read("evidence/F372/F372-baseline.mus"),
               "options.clefOptions.clefDefs[0].baselineAdjust").rawValue == -2,
        "A disabled baseline count was not still reported as stored evidence");

    // Finale 97 is internally 3.8 and dropped the checkbox, so it adjusts unconditionally.
    // Its word 5 is 30 here, which has bit 0 clear: a reader that tested the word for
    // non-zero, or that tested the bit without the version, would get this file wrong.
    const auto f97Baseline = expectEarlyBaseline("evidence/F97/Fin97-clef-baseline.mus",
        {1 * efixPerHarmonicLevel, -2 * efixPerHarmonicLevel, 0}, "Finale 97");

    // The Coda era is excluded outright, not merely left switched off: its word 4 is a
    // mid-measure-clef baseline rather than the general one, and Finale 27 discards it.
    const auto f100Baseline = expectEarlyBaseline("evidence/F100/F100-clef-baseline.mus",
        {0, 0, 0}, "Finale 1.0.0");
    const auto f263Baseline = expectEarlyBaseline("evidence/F263/F263-clef-baseline.mus",
        {0, 0, 0}, "Finale 2.6.3");
    expect(field(f100Baseline, "options.clefOptions.clefDefs[0].baselineAdjust").rawValue == -4,
        "The Finale 1.0.0 stored baseline count was not reported");

    // The courtesy flags pack clef, key and time in one word. Only a controlled pair can say
    // which bit is which: every companion in the corpus has all three set, and only the
    // values 5 and 7 occur, both of which leave the clef bit set.
    const auto clefCourtesyOff = read("evidence/F2005/F2005-courtesy-clef-off.mus");
    expect(!clefs(clefCourtesyOff)->cautionaryClefChanges,
        "Turning off the courtesy clef alone was not recovered");
    // Turning off the key signature's courtesy instead must leave the clef's alone. Without
    // this the test would pass for any bit that happens to be clear.
    const auto keyCourtesyOff = read("evidence/F2005/F2005-courtesy-key-off.mus");
    expect(clefs(keyCourtesyOff)->cautionaryClefChanges,
        "A courtesy key-signature edit was misread as the clef bit");

    // The Coda era has no courtesy-clef option and always shows one, so the reader asserts
    // that rather than reading selector 44, which is zero throughout the era and would say
    // the opposite. Both a plain Coda document and one whose key courtesy was turned off
    // must come out true.
    for (const char* path : {"evidence/F263/F263-baseline.mus",
             "evidence/F263/F263-courtesy-key-off.mus", "evidence/F100/F100-baseline.mus"}) {
        const auto coda = read(path);
        expect(clefs(coda)->cautionaryClefChanges,
            std::string("A Coda document did not always show a courtesy clef: ") + path);
        // Neither read from the file nor a baseline default: the era had no option, so the
        // behavior determines it. Reported once, and as behavior.
        const auto entries = std::count_if(coda.report.fields.begin(), coda.report.fields.end(),
            [](const FieldInfo& value) {
                return value.target == "options.clefOptions.cautionaryClefChanges";
            });
        expect(entries == 1,
            std::string("cautionaryClefChanges was reported more than once from ") + path);
        expect(field(coda, "options.clefOptions.cautionaryClefChanges").origin
                == ValueOrigin::LegacyBehavior,
            std::string("A Coda courtesy clef was not reported as legacy behavior: ") + path);
    }
    // Every other era reads it, so nothing else may claim behavior.
    for (const auto* result : {&f2002, &f2007, &f2000}) {
        expect(field(*result, "options.clefOptions.cautionaryClefChanges").origin
                == ValueOrigin::LegacyMus,
            "A recorded courtesy clef was reported as legacy behavior");
    }
    // Finale 3.0 through 3.5 predate the option too, but already carry the bit set, so the
    // epoch is the boundary and reading the bit gives the right answer there.
    expect(clefs(read("evidence/F372/F372-baseline.mus"))->cautionaryClefChanges,
        "A pre-3.6.2 uncompressed document lost its courtesy clef");

    for (const auto* result : {&f2002, &f2005, &f2007, &f2012, &f2000, &f100,
             &f2005Baseline, &f100Baseline, &f263Baseline, &f372Baseline, &f97Baseline}) {
        const auto options = result->document->getOptions()->get<ClefOptions>();
        for (std::size_t index = 0; index < options->clefDefs.size(); ++index) {
            const auto& def = options->clefDefs[index];
            // musxdom's own resolver rejects this combination, so it must never be built.
            expect(!def->useOwnFont || static_cast<bool>(def->font),
                "A clef claims its own font without carrying one");
            if (def->useOwnFont) {
                expect(static_cast<bool>(result->document->getOthers()
                        ->get<musx::dom::others::FontDefinition>(
                            musx::dom::SCORE_PARTID, def->font->fontId)),
                    "A recovered clef font has a dangling font id");
            }
        }
    }
}

// Selector 24 is the default-font array from well before the DCL era, but the reader used to
// gate that layout to DCL alone, so every Finale 3.0 through 2000 document reported all 45
// font options as Finale 27 defaults while its source held 40 of them.
void testUncompressedFontOptions()
{
    using FontOptions = musx::dom::options::FontOptions;
    using FontType = FontOptions::FontType;
    const auto read = [](const char* relative) {
        return Reader::read<TestXmlDocument>(
            std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / relative);
    };
    // Finale 3.7.2, Finale 97 and Finale 2000 span the epoch. All three store the same
    // sizes, and each agrees with its exact Finale 27 companion.
    const std::array<std::pair<const char*, std::array<int, 4>>, 3> eraFixtures{{
        // path, then the music, key, clef and time sizes its companion shows.
        {"evidence/F372/F372-baseline.mus", {24, 24, 24, 24}},
        {"evidence/F97/F97-fileinfo-short.mus", {28, 28, 24, 26}},
        {"evidence/F2000/F2000-multilayer.mus", {28, 28, 24, 26}}}};
    for (const auto& [path, sizes] : eraFixtures) {
        const auto result = read(path);
        expect(result.report.formatEpoch == FormatEpoch::UncompressedLegacy,
            "An uncompressed fixture was not classified as uncompressed");
        const auto options = result.document->getOptions()->get<FontOptions>();
        expect(options && options->fontOptions.size() == 45,
            "Uncompressed font options were not completed to the modern type set");
        expect(options->getFontInfo(FontType::Music)->fontSize == sizes[0]
                && options->getFontInfo(FontType::Key)->fontSize == sizes[1]
                && options->getFontInfo(FontType::Clef)->fontSize == sizes[2]
                && options->getFontInfo(FontType::Time)->fontSize == sizes[3],
            std::string("Uncompressed font options were not recovered from ") + path);
        expect(field(result, "options.fontOptions[0].fontSize").origin == ValueOrigin::LegacyMus,
            std::string("Uncompressed font options were reported as defaults from ") + path);
        // The era's physical slot 28 is tablature and slot 13 is not an independent value.
        expect(field(result, "options.fontOptions[13].fontId").origin == ValueOrigin::LegacyMus,
            "The uncompressed tablature slot was not mapped");
    }

    // The epoch alone must carry the semantic layout, because three real Finale 3.0
    // documents recover a major version far outside the era's own range.
    const auto synthetic = Reader::read<TestXmlDocument>(makeUncompressedMusWithFontOptions());
    const auto options = synthetic.document->getOptions()->get<FontOptions>();
    expect(options->getFontInfo(FontType::Music)->fontSize == 28
            && options->getFontInfo(FontType::Key)->fontSize == 26
            && options->getFontInfo(FontType::Clef)->fontSize == 24
            && options->getFontInfo(FontType::Time)->fontSize == 22,
        "An uncompressed file with an out-of-range major version recovered no font options");
    expect(options->getFontInfo(FontType::Time)->bold,
        "A recovered effects mask was not expanded for the uncompressed era");
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
    const auto fallbackText = result.document->getOptions()
        ->get<musx::dom::options::FontOptions>()
        ->getFontInfo(musx::dom::options::FontOptions::FontType::TextBlock);
    const auto fallbackDefinition = result.document->getOthers()
        ->get<musx::dom::others::FontDefinition>(
            musx::dom::SCORE_PARTID, fallbackText->fontId);
    // This target recovers no font definitions of its own, so every one below was introduced
    // from the Windows reference. Cmpers 0 and 1 both hold the music font: 0 is the
    // default-music-font sentinel, materialized so that anything storing 0 resolves, and 1 makes
    // the same typeface addressable concretely, since matching never selects 0.
    const auto zeroDefinition = result.document->getOthers()
        ->get<musx::dom::others::FontDefinition>(musx::dom::SCORE_PARTID, 0);
    expect(static_cast<bool>(zeroDefinition),
        "A document referencing font id 0 was left with no definition at 0");
    expect(fallbackText->fontId != 0
            && fallbackDefinition && fallbackDefinition->getCmper() == fallbackText->fontId
            && fallbackDefinition->name == "Times New Roman"
            && fallbackDefinition->charsetBank
                == musx::dom::others::FontDefinition::CharacterSetBank::Windows
            && fallbackDefinition->pitch == 2,
        "A cloned Windows fallback font retained its reference cmper or lost its definition");
    expectCompleteOptionsPool(result);
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
    expect(hasDiagnostic(result.report, musx::util::Logger::LogLevel::Warning),
        "Coda-banner limitation was not reported as a warning");
    expect(field(result, "options.musicSpacing.minWidth").origin
        == ValueOrigin::Finale27Default,
        "Unsupported Coda-banner option was not retained as a default");
    const auto fonts = result.document->getOptions()
        ->get<musx::dom::options::FontOptions>();
    expect(fonts && fonts->fontOptions.size() == 45,
        "Unsupported Coda-banner FontOptions were not safely completed from the baseline");
    expectCompleteOptionsPool(result);
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
    // Info, not a warning: this message fires for every zlib document ever read and
    // describes how far recovery reaches, so raising it to a user would report normal
    // operation as a fault on every file. Other diagnostics from this synthetic fixture
    // may legitimately be warnings, so the level is asserted on this message alone.
    expect(std::any_of(result.report.diagnostics.begin(), result.report.diagnostics.end(),
               [](const finale_mus_reader::Diagnostic& entry) {
                   return entry.message.find("variable logical records") != std::string::npos
                       && entry.level == musx::util::Logger::LogLevel::Info;
               }),
        "The zlib overlay limitation was not reported at info level");
    expectNoScoreContent(result);
}

// A document that embeds a graphic puts its bytes in one of the two terminal blocks, stored
// rather than deflated. Inflating it used to fail and abandon every block already decoded,
// so a score with a picture in it lost its entire options pool. Both terminal types are
// exercised because a file may use either.
void testEmbeddedGraphics()
{
    // A PNG signature and an EPS header: the two shapes the corpus actually contains.
    for (const auto& [terminalType, graphic] :
         {std::pair<std::uint16_t, std::string_view>{0x0013, "\x89PNG\r\n\x1a\n padding"},
          std::pair<std::uint16_t, std::string_view>{0x001d, "%!PS-Adobe-3.0 EPSF-3.0"}}) {
        const auto result = Reader::read<TestXmlDocument>(
            makeZlibMusWithGraphic(terminalType, graphic));
        expect(result.report.formatEpoch == FormatEpoch::ZlibLegacy,
            "A file with an embedded graphic was not classified as zlib legacy");
        // The point of the fix: the four real blocks survive rather than being discarded.
        expect(result.report.blocks.size() == 5,
            "A stored terminal block cost the file its already-decoded blocks");
        expect(result.report.blocks.front().decodedSize == 3,
            "The record block was not decoded alongside a stored terminal block");

        const auto& terminal = result.report.blocks.back();
        expect(terminal.type == terminalType && terminal.stored,
            "The graphics block was not reported as stored");
        expect(terminal.decodedSize == graphic.size(),
            "A stored block's payload begins after six header bytes, not ten");
        expect(!terminal.checksumPresent,
            "A stored block was reported as carrying a checksum");
        expect(std::any_of(result.report.diagnostics.begin(), result.report.diagnostics.end(),
                   [](const finale_mus_reader::Diagnostic& entry) {
                       return entry.level == musx::util::Logger::LogLevel::Info
                           && entry.message.find("does not import") != std::string::npos;
                   }),
            "An embedded graphic was preserved without being reported at info level");
        expectCompleteOptionsPool(result);
        expectNoScoreContent(result);
    }
}

// The Coda-banner era in both byte orders. Its Windows documents are little-endian, which the
// container learns from the banner product rather than by trialling framing, and the only real
// specimens are Coda's own installer templates, which cannot be committed here. A synthetic
// pair covers the path and keeps the two orders honest against each other.
void testCodaBannerByteOrder()
{
    using ClefOptions = musx::dom::options::ClefOptions;
    struct Case
    {
        ByteOrder byteOrder;
        const char* product;
        SourcePlatform platform;
        SourcePlatform baseline;
    };
    // A `PC` product states Windows, which is little-endian and seeds from the Windows
    // baseline. A numeric product says nothing about platform, so it stays Unknown and falls
    // back to macOS, exactly as before this era gained a second platform.
    const std::array<Case, 2> cases{{
        {ByteOrder::LittleEndian, "PC 1.0+", SourcePlatform::Windows, SourcePlatform::Windows},
        {ByteOrder::BigEndian, "2.6", SourcePlatform::Unknown, SourcePlatform::MacOS}}};

    for (const auto& testCase : cases) {
        const auto result = Reader::read<TestXmlDocument>(
            makeCodaBannerMus(testCase.byteOrder, testCase.product));
        const std::string what = std::string("Coda-banner ") + testCase.product;
        expect(result.report.formatEpoch == FormatEpoch::CodaBanner,
            what + " was not classified as the Coda-banner era");
        expect(result.report.byteOrder == testCase.byteOrder,
            what + " byte order was not taken from the banner product");
        expect(result.report.sourcePlatform == testCase.platform
                && result.report.defaultsPlatform == testCase.baseline,
            what + " selected the wrong platform or baseline");
        expect(result.report.savingProduct == testCase.product,
            what + " did not report its product");

        // Both orders must yield the same logical document, which is what shows the order is
        // being applied rather than the bytes merely being accepted.
        const auto clefs = result.document->getOptions()->get<ClefOptions>();
        expect(clefs && clefs->clefDefs.size() == 18,
            what + " did not complete the clef collection");
        expect(clefs->getClefDef(0)->middleCPos == -10 && clefs->getClefDef(0)->clefChar == 38
                && clefs->getClefDef(0)->staffPosition == -6,
            what + " did not recover its first clef");
        expect(clefs->getClefDef(4)->clefChar == 214 && clefs->getClefDef(7)->staffPosition == -4,
            what + " did not recover its later clefs");
        expect(field(result, "options.clefOptions.clefDefs[7].middleCPos").origin
                == ValueOrigin::LegacyMus,
            what + " reported a stored clef as synthesized");
        expect(clefs->clefChangePercent == 75 && clefs->clefChangeOffset == -12
                && clefs->clefFrontSepar == 24,
            what + " did not recover its clef scalars");
        // The era has no such option, so this is behavior rather than a record or a default.
        expect(clefs->cautionaryClefChanges
                && field(result, "options.clefOptions.cautionaryClefChanges").origin
                    == ValueOrigin::LegacyBehavior,
            what + " lost the era's unconditional courtesy clef");
        // These three do not exist in the era either, but the baseline already carries what
        // Finale 27 produces for them, so they are ordinary defaults.
        expect(clefs->clefKeySepar == 0 && clefs->clefTimeSepar == 0
                && !clefs->showClefFirstSystemOnly,
            what + " invented a value for an option the era does not have");
        expectNoScoreContent(result);
    }
}

void testMalformedInput()
{
    // Failure is returned, not thrown: a null document is the signal, and the reason
    // arrives as an Error diagnostic in the same report every other message uses. A caller
    // sweeping a corpus can therefore skip a bad file without wrapping each call.
    const auto result = Reader::read<TestXmlDocument>(std::vector<std::uint8_t>{1, 2, 3, 4});
    expect(result.document == nullptr, "Arbitrary input was accepted as a MUS file");
    expect(hasDiagnostic(result.report, musx::util::Logger::LogLevel::Error),
        "A failed import did not report why at error level");
    // Error is the one level with an absolute meaning, so it must not appear when a
    // document was produced. Every other level accompanies a usable result.
    const auto good = Reader::read<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2012/F2012-upstem-flags.mus");
    expect(good.document != nullptr, "A known-good fixture failed to import");
    expect(!hasDiagnostic(good.report, musx::util::Logger::LogLevel::Error),
        "A successful import reported an error-level diagnostic");
}

} // namespace

TEST_CASE("Controlled DCL file", "[reader]") { testControlledDclFile(); }
TEST_CASE("Font definitions", "[reader]") { testFontDefinitions(); }
TEST_CASE("Font options capture", "[reader]") { testFontOptionsCapture(); }
TEST_CASE("Clef options capture", "[reader]") { testClefOptionsCapture(); }
TEST_CASE("Uncompressed font options", "[reader]") { testUncompressedFontOptions(); }
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
TEST_CASE("Embedded graphics", "[reader]") { testEmbeddedGraphics(); }
TEST_CASE("Coda-banner byte order", "[reader]") { testCodaBannerByteOrder(); }
TEST_CASE("Malformed input", "[reader]") { testMalformedInput(); }

// Every banner spelling is recognized through the one parser, so a file that carries the
// Finale 1.0.0 spelling reports a product and a version like any other era. Before the
// parser was unified this file read as an error, because the container and the identity
// code each carried their own copy of the spellings and neither knew the third.
TEST_CASE("Finale 1.0.0 banner spelling", "[banner]")
{
    // `Finale` + the MacRoman trademark sign + a version, terminated by `ENIGA Structures`
    // (sic) where every later era puts a copyright notice.
    std::vector<std::uint8_t> data(0x400, 0);
    const std::string banner = "Finale\xaa 1.0.0 ENIGA Structures Copyright 1987 by Coda.";
    std::copy(banner.begin(), banner.end(), data.begin());
    // A body prologue: the record count, then the body offset itself, which is what
    // confirms the era.
    data[0x203] = 0x01;
    data[0x205] = 0x00;
    data[0x206] = 0x02;

    const auto parsed = finale_mus_reader::banner::parse(data.data(), data.size());
    REQUIRE(parsed.spelling == finale_mus_reader::banner::Spelling::MacTrademark);
    CHECK(parsed.offset == 0);
    CHECK(parsed.product == "1.0.0");
    CHECK(parsed.hasNumericProduct());

    const auto version = finale_mus_reader::banner::versionFromProduct(parsed.product);
    REQUIRE(version.has_value());
    CHECK(version->major == 1);
    CHECK(version->minor == 0);
    CHECK(version->maint == 0);
}

TEST_CASE("The other two banner spellings still parse", "[banner]")
{
    const auto coda = [] {
        std::vector<std::uint8_t> data(0x100, 0);
        const std::string text = "Finale(TM) 2.6 Copyright 1987 by Coda.";
        std::copy(text.begin(), text.end(), data.begin());
        return finale_mus_reader::banner::parse(data.data(), data.size());
    }();
    CHECK(coda.spelling == finale_mus_reader::banner::Spelling::Trademark);
    CHECK(coda.product == "2.6");
    CHECK(coda.isPreSignature());

    // The registered spelling sits at 0x20, after the ENIGMA signature.
    const auto signature = [] {
        std::vector<std::uint8_t> data(0x100, 0);
        const std::string sig = "ENIGMA BINARY FILE";
        std::copy(sig.begin(), sig.end(), data.begin());
        const std::string text = "Finale(R) 2003 Copyright (c) 1987-2002 Coda Music Technology";
        std::copy(text.begin(), text.end(), data.begin() + 0x20);
        return finale_mus_reader::banner::parse(data.data(), data.size());
    }();
    CHECK(signature.spelling == finale_mus_reader::banner::Spelling::Registered);
    CHECK(signature.offset == 0x20);
    CHECK(signature.product == "2003");
    CHECK_FALSE(signature.isPreSignature());
}
