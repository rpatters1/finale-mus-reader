// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "container/mus_container.h"
#include "container/product_banner.h"
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
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
#endif // !defined(MUSX_USE_PUGIXML)

#include "musx/xml/PugiXmlImpl.h"

#ifdef FINALE_MUS_READER_TEST_UNDEFINE_MUSX_USE_PUGIXML
#undef MUSX_USE_PUGIXML
#undef FINALE_MUS_READER_TEST_UNDEFINE_MUSX_USE_PUGIXML
#endif // defined(FINALE_MUS_READER_TEST_UNDEFINE_MUSX_USE_PUGIXML)

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
// major 12, deliberately outside the Finale 3.0 through 2002 range, so that the epoch has to
// carry the semantic layout rather than the version.
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
std::vector<std::uint8_t> makeCodaBannerMus(
    ByteOrder byteOrder, std::string_view product, bool includeBlankShape = false)
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
    if (includeBlankShape) {
        appendOther(pool, 19, "SD", words(19, 19, 0, 0, -584, 868), byteOrder);
        appendOther(pool, 19, "SL", words(0, 0, 0, 0, 0, 0), byteOrder);
    }
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
    std::vector<std::uint8_t> stored;
    if (terminalType == 0x0013) {
        // Each item is type, byte length, raw file bytes, footer version, and one
        // still-opaque footer byte. The embedded cmper is its one-based encounter order.
        write16(stored, 9, byteOrder);
        write32(stored, static_cast<std::uint32_t>(graphic.size()), byteOrder);
        stored.insert(stored.end(), graphic.begin(), graphic.end());
        write32(stored, 1, byteOrder);
        stored.push_back(0);
    } else {
        stored.insert(stored.end(), graphic.begin(), graphic.end());
    }
    write16(result, terminalType, byteOrder);
    write32(result, static_cast<std::uint32_t>(stored.size() + 6), byteOrder);
    result.insert(result.end(), stored.begin(), stored.end());
    return result;
}

// Taken by value rather than by const reference: see the note on the equivalent helper in
// mapping_tests.cpp about -Wdangling-reference.
const FieldInfo& field(const ImportResult& result, std::string_view target)
{
    std::string desiredMember(target);
    std::optional<musx::dom::Cmper> cmper;
    if (target.starts_with("options.fontOptions[")) {
        desiredMember = "fonts[" + std::string(target.substr(std::string_view("options.fontOptions[").size()));
    } else if (target.starts_with("options.fontOptionsPhysical[")) {
        desiredMember = "physical[" + std::string(
            target.substr(std::string_view("options.fontOptionsPhysical[").size()));
    } else if (target.starts_with("options.")) {
        const auto dot = target.find('.', std::string_view("options.").size());
        if (dot != std::string_view::npos) desiredMember = target.substr(dot + 1);
    } else if (const auto open = target.find('['); open != std::string_view::npos) {
        const auto close = target.find(']', open);
        const auto dot = close == std::string_view::npos ? close : target.find('.', close);
        if (close != std::string_view::npos && dot != std::string_view::npos) {
            cmper = static_cast<musx::dom::Cmper>(std::stoul(
                std::string(target.substr(open + 1, close - open - 1))));
            desiredMember = target.substr(dot + 1);
        }
    }
    for (const auto& [instance, fields] : result.report.fields) {
        if (cmper && instance.cmper1 != cmper) continue;
        for (const auto& [member, value] : fields) {
            if (member == desiredMember) return value;
        }
    }
    throw std::runtime_error(std::string("Missing field report for ").append(target));
}

template <typename Predicate>
bool anyReportedField(const finale_mus_reader::ImportReport& report, Predicate predicate)
{
    for (const auto& [instance, fields] : report.fields) {
        (void)instance;
        for (const auto& [member, info] : fields) {
            if (predicate(member, info)) return true;
        }
    }
    return false;
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
            const auto* shapeField = result.report.findField<others::ShapeDef>(
                "instructionList", SCORE_PARTID, shape->getCmper());
            const auto recovered = shapeField && shapeField->origin == ValueOrigin::LegacyMus;
            expect(recovered || referencedShapes.count(shape->getCmper()) != 0,
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
    const auto result = Reader::readWithReport<TestXmlDocument>(path);
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
    const auto first = Reader::readWithReport<TestXmlDocument>(path);
    const auto second = Reader::readWithReport<TestXmlDocument>(path);
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
    const auto result = Reader::readWithReport<TestXmlDocument>(path);
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
        return Reader::readWithReport<TestXmlDocument>(
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
        return Reader::readWithReport<TestXmlDocument>(
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
        const auto* courtesy = coda.report.findField<musx::dom::options::ClefOptions>(
            "cautionaryClefChanges");
        expect(courtesy != nullptr,
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

// Stem connections are a source-owned collection: nothing is seeded, and a document gets
// exactly the connections its own record states. One fixture per epoch, because the element
// changed unit at Finale 3.5 and width at Finale 2012, and because an epoch left out of the
// gate would recover nothing while looking like a document that stores nothing.
void testStemConnectionCapture()
{
    using StemOptions = musx::dom::options::StemOptions;
    const auto read = [](const char* relative) {
        return Reader::readWithReport<TestXmlDocument>(
            std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / relative);
    };
    const auto stems = [](const ImportResult& result) {
        const auto options = result.document->getOptions()->get<StemOptions>();
        expect(static_cast<bool>(options), "The imported document has no stem options");
        return options;
    };
    // Every era stores this connection for the default music font, so agreement across all
    // of them is what shows the three physical layouts describe one logical table. The
    // adjustments are Efix here even where the source stated Evpu.
    const auto expectDefaultConnection = [&](const auto& options, const char* era) {
        expect(!options->stemConnections.empty(),
            std::string("No stem connection was recovered from ") + era);
        const auto& first = options->stemConnections.front();
        expect(first->fontId == 0 && first->symbol == 192,
            std::string("The default stem connection was not recovered from ") + era);
        expect(first->upStemVert == 768 && first->downStemVert == -768
                && first->upStemHorz == 0 && first->downStemHorz == 0,
            std::string("The default stem adjustments were wrong for ") + era);
    };

    // Coda banner. This era states the adjustments in Evpu, so 12 and -12 must arrive as 768
    // and -768; both exact Finale 27 companions carry exactly those Efix numbers. The report
    // keeps the stored Evpu word, which is the only place the original number survives.
    const auto f100 = read("evidence/F100/F100-baseline.mus");
    expect(f100.report.formatEpoch == FormatEpoch::CodaBanner,
        "The Finale 1.0.0 fixture is not the Coda-banner epoch");
    expectDefaultConnection(stems(f100), "Finale 1.0.0");
    expect(field(f100, "options.stemOptions.stemConnections[0].upStemVert").rawValue == 12,
        "The stored Evpu adjustment was not reported for Finale 1.0.0");
    expect(field(f100, "options.stemOptions.stemConnections[0].upStemVert").origin
            == ValueOrigin::LegacyMus,
        "A recovered stem adjustment was not reported as read from the source");
    expectDefaultConnection(stems(read("evidence/F263/F263-baseline.mus")), "Finale 2.6.3");

    // Uncompressed, from Finale 3.5 on: the same words, already in Efix. The raw report
    // value separates the two eras, because the assigned Efix cannot.
    const auto f372 = read("evidence/F372/F372-baseline.mus");
    expect(f372.report.formatEpoch == FormatEpoch::UncompressedLegacy,
        "The Finale 3.7.2 fixture is not the uncompressed epoch");
    expectDefaultConnection(stems(f372), "Finale 3.7.2");
    expect(field(f372, "options.stemOptions.stemConnections[0].upStemVert").rawValue == 768,
        "A Finale 3.7.2 adjustment was scaled as though it were Evpu");
    expectDefaultConnection(stems(read("evidence/F2000/F2000-baseline.mus")), "Finale 2000");

    // Finale 97 is the one tracked fixture with a full table, and it is also the terminator
    // case: 32 of its 128 incidences carry data, but only three precede the first element
    // with no symbol. The 29 after it are what Finale ignores and its Finale 27 conversion
    // nevertheless writes out, so this reader deliberately reports fewer connections than
    // the companion does.
    const auto f97 = read("evidence/F97/Fin97-baseline.mus");
    const auto f97Stems = stems(f97);
    expectDefaultConnection(f97Stems, "Finale 97");
    expect(f97Stems->stemConnections.size() == 3,
        "The stem-connection table did not stop at its terminator");
    const auto& flagUp = f97Stems->stemConnections[1];
    expect(flagUp->symbol == 131 && flagUp->upStemVert == -2304
            && flagUp->downStemVert == 2304 && flagUp->upStemHorz == -1024
            && flagUp->downStemHorz == 1024,
        "The Finale 97 flag connection was not recovered");
    expect(f97Stems->stemConnections[2]->symbol == 132,
        "The third Finale 97 connection was not recovered");

    // DCL, then the zlib era's two element widths. Finale 2007 keeps the twelve-byte element
    // in a big-endian class record; Finale 2012 widened the symbol to a long, so reading it
    // as the narrow element would leave upStemVert at zero and slide the pair one word down.
    const auto f2005 = read("evidence/F2005/F2005-baseline.mus");
    expect(f2005.report.formatEpoch == FormatEpoch::DclLegacy,
        "The Finale 2005 fixture is not the DCL epoch");
    expectDefaultConnection(stems(f2005), "Finale 2005");

    const auto f2007 = read("evidence/F2007/F2007-lyric-hyphens.mus");
    expect(f2007.report.byteOrder == ByteOrder::BigEndian,
        "The Finale 2007 fixture is not big-endian");
    expectDefaultConnection(stems(f2007), "Finale 2007");

    const auto f2012 = read("evidence/F2012/F2012-upstem-flags.mus");
    expect(f2012.report.formatEpoch == FormatEpoch::ZlibLegacy,
        "The Finale 2012 fixture is not the zlib epoch");
    expectDefaultConnection(stems(f2012), "Finale 2012");

    // Nothing may come from the baseline. Every recovered connection is reported, and a
    // document that stores one connection has one, not the Finale 27 table's three.
    for (const auto* result : {&f100, &f372, &f2005, &f2007, &f2012}) {
        expect(result->document->getOptions()->get<StemOptions>()->stemConnections.size() == 1,
            "A single-connection document did not keep exactly its own connection");
    }
    expect(!anyReportedField(f100.report, [](const auto& member, const FieldInfo& value) {
               return member.find("stemConnections") != std::string::npos
                   && value.origin != ValueOrigin::LegacyMus;
           }),
        "A stem connection was reported as anything other than recovered");
}

// The eight StemOptions scalars are scattered across five numeric globals, and two of them
// change meaning before Finale 3.5. Every expected value below is what that fixture's exact
// Finale 27 companion carries.
void testStemScalarRecovery()
{
    using StemOptions = musx::dom::options::StemOptions;
    const auto read = [](const char* relative) {
        return Reader::readWithReport<TestXmlDocument>(
            std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / relative);
    };
    struct Expected
    {
        const char* path;
        const char* era;
        int halfStemLength;
        int stemLength;
        int shortStemLength;
        int revStemAdj;
        int stemWidth;
        int stemOffset;
        bool useStemConnections;
    };
    // Coda banner: the three lengths are stated in staff positions and scale by twelve, the
    // half-stem length is not stored at all, and the thickness and offset selectors hold
    // something else entirely, so all three keep the pinned baseline's 18, 115 and 256.
    // Finale 1.0.0 does not even carry the latter two selectors. Note that Finale 27's own
    // conversion of these files invents a thickness of its own -- 224 for the Finale 1.0.0
    // fixture and 128 for the Finale 2.6.3 one -- which is why the companion is not the
    // expectation here: neither number is in the source.
    const Expected fixtures[] = {
        {"evidence/F100/F100-baseline.mus", "Finale 1.0.0", 18, 84, 60, 216, 115, 256, false},
        {"evidence/F263/F263-baseline.mus", "Finale 2.6.3", 18, 84, 60, 216, 115, 256, true},
        // Finale 3.7 onward: every field in its own place, in Evpu and Efix.
        {"evidence/F372/F372-baseline.mus", "Finale 3.7.2", 18, 84, 60, 432, 118, 128, false},
        {"evidence/F97/Fin97-baseline.mus", "Finale 97", 18, 84, 60, 216, 128, 128, true},
        {"evidence/F2000/F2000-baseline.mus", "Finale 2000", 18, 84, 60, 432, 118, 256, false},
        {"evidence/F2005/F2005-baseline.mus", "Finale 2005", 18, 84, 60, 432, 224, 256, false},
        // The zlib era addresses the same options by byte offset. Finale 2007 is big-endian
        // and Finale 2012 little-endian, and the stem offset is the field that tells them
        // apart: it is two payload words, high word first, not a plain four-byte read.
        {"evidence/F2007/F2007-lyric-hyphens.mus", "Finale 2007", 18, 84, 60, 432, 224, 256, false},
        {"evidence/F2012/F2012-upstem-flags.mus", "Finale 2012", 18, 84, 60, 216, 115, 256, true},
    };
    for (const auto& fixture : fixtures) {
        const auto result = read(fixture.path);
        const auto stems = result.document->getOptions()->get<StemOptions>();
        expect(static_cast<bool>(stems),
            std::string("No stem options for ") + fixture.era);
        const auto wrong = [&](const char* field) {
            return std::string("The stem ") + field + " was wrong for " + fixture.era;
        };
        expect(stems->halfStemLength == fixture.halfStemLength, wrong("half length"));
        expect(stems->stemLength == fixture.stemLength, wrong("length"));
        expect(stems->shortStemLength == fixture.shortStemLength, wrong("short length"));
        expect(stems->revStemAdj == fixture.revStemAdj, wrong("reverse adjustment"));
        expect(stems->stemWidth == fixture.stemWidth, wrong("width"));
        expect(stems->stemOffset == fixture.stemOffset, wrong("offset"));
        expect(stems->useStemConnections == fixture.useStemConnections,
            wrong("connection switch"));
        // No corpus document in either survey sets this bit, so every fixture must be false.
        expect(!stems->noReverseStems, wrong("reverse-stemming bit"));
    }

    // What separates the two eras is provenance, not the value. Before Finale 3.5 the stored
    // word is a twelfth of the Evpu it becomes, and the report keeps that stored word; the
    // half-stem length and the two sizes report as Finale 27 defaults, because that era
    // states none of them.
    const auto f263 = read("evidence/F263/F263-baseline.mus");
    expect(field(f263, "options.stemOptions.stemLength").rawValue == 7
            && field(f263, "options.stemOptions.stemLength").origin == ValueOrigin::LegacyMus,
        "The stored staff-position stem length was not reported for the Coda era");
    for (const char* target : {"options.stemOptions.halfStemLength",
             "options.stemOptions.stemWidth", "options.stemOptions.stemOffset"}) {
        expect(field(f263, target).origin == ValueOrigin::Finale27Default,
            std::string("A Coda-era field the source does not state was claimed as read: ")
                + target);
    }
    const auto f97 = read("evidence/F97/Fin97-baseline.mus");
    expect(field(f97, "options.stemOptions.stemLength").rawValue == 84,
        "A Finale 97 stem length was scaled as though it were staff positions");
    expect(field(f97, "options.stemOptions.stemWidth").origin == ValueOrigin::LegacyMus,
        "The Finale 97 stem thickness was not recovered from the source");

    // A controlled Finale 2002 pair, which is what settles the packed spelling of the
    // reverse-stemming flag. Switching it off moves selector 41 word 1 from 26 to 30 -- a gain
    // of 4, so bit 2 -- where the same edit in Finale 1.0.0 and 3.7.2 moves the word to 1. The
    // same save lengthens the normal stem to 96, the one Evpu-era row the corpus never varies.
    const auto f2002Edit = read("evidence/F2002/F2002-norevstem-len96.mus");
    const auto f2002EditStems = f2002Edit.document->getOptions()->get<StemOptions>();
    expect(f2002EditStems->stemLength == 96,
        "The controlled Finale 2002 stem length was not recovered");
    expect(f2002EditStems->noReverseStems,
        "The packed reverse-stemming flag was not read from bit 2");
    expect(f2002EditStems->shortStemLength == 60 && f2002EditStems->halfStemLength == 18
            && f2002EditStems->revStemAdj == 432,
        "A controlled Finale 2002 stem edit disturbed a field it did not touch");

    // A controlled Finale 3.7.2 pair, which settles two rows the corpus never varies. The
    // half-stem length moves 18 -> 19 in selector 03 word 2, and "Display Reverse Stemming"
    // moves selector 41 word 1 from 0 to **1** -- bit 0, the same spelling Finale 1.0.0 uses
    // and not the bit 2 the framework names for its own era. Only those two rows differ.
    const auto f372Edit = read("evidence/F372/F372-revstem-halfstem.mus");
    const auto f372EditStems = f372Edit.document->getOptions()->get<StemOptions>();
    expect(f372EditStems->halfStemLength == 19,
        "The controlled Finale 3.7.2 half-stem length was not recovered");
    expect(f372EditStems->noReverseStems,
        "The Finale 3.7.2 reverse-stemming flag was not read from bit 0");
    expect(f372EditStems->stemLength == 84 && f372EditStems->shortStemLength == 60
            && f372EditStems->revStemAdj == 432 && f372EditStems->stemWidth == 118,
        "A controlled Finale 3.7.2 stem edit disturbed a field it did not touch");

    // Two controlled Finale 1.0.0 saves. The first lengthens the normal and shortened stems by
    // one staff position each and switches off "Display Reverse Stemming"; nothing else in its
    // record stream moves. It is what makes the staff-position unit a measurement rather than
    // an inference, because the corpus only ever stores the era's defaults.
    const auto changed = read("evidence/F100/F100-stemopts-changed.mus");
    const auto changedStems = changed.document->getOptions()->get<StemOptions>();
    expect(changedStems->stemLength == 96 && changedStems->shortStemLength == 72,
        "The controlled Finale 1.0.0 stem lengths were not converted from staff positions");
    expect(field(changed, "options.stemOptions.stemLength").rawValue == 8,
        "The stored staff-position count for the edited length was not reported");
    // The Coda era keeps this flag in bit 0 where every later era uses bit 2. Reading the
    // later bit here would leave it false, which is what the whole corpus looks like.
    expect(changedStems->noReverseStems,
        "The Coda-era reverse-stemming flag was not recovered from its own bit");
    expect(!changedStems->useStemConnections
            && changedStems->revStemAdj == 216 && changedStems->halfStemLength == 18,
        "A controlled stem edit disturbed the fields it did not touch");

    // The reverse stem adjustment is the third length that era states in staff positions.
    // Setting it to 25 moves selector 21 word 2 from 18, and the companion carries 300 --
    // twelve times the stored number, the same factor the two lengths above establish.
    const auto revstem = read("evidence/F100/F100-revstem-25.mus");
    const auto revstemStems = revstem.document->getOptions()->get<StemOptions>();
    expect(revstemStems->revStemAdj == 300,
        "The controlled Finale 1.0.0 reverse stem adjustment was not converted");
    expect(field(revstem, "options.stemOptions.revStemAdj").rawValue == 25,
        "The stored staff-position reverse adjustment was not reported");
    expect(revstemStems->stemLength == 84 && revstemStems->shortStemLength == 60,
        "The reverse-adjustment save disturbed the two stem lengths");

    // Enabling stem connections in Finale 1.0.0 moves selector 31 word 5 from 0 to 1 and
    // moves nothing else in the file, so this pair is what pins that location in an era whose
    // corpus files never vary it. The companion gains <useStemConnections/> where the baseline
    // has none.
    const auto enabled = read("evidence/F100/F100-stemconn-enabled.mus");
    const auto enabledStems = enabled.document->getOptions()->get<StemOptions>();
    expect(enabledStems->useStemConnections,
        "The controlled Finale 1.0.0 connection switch was not recovered");
    expect(field(enabled, "options.stemOptions.useStemConnections").origin
            == ValueOrigin::LegacyMus,
        "The recovered connection switch was not reported as read from the source");
    expect(enabledStems->stemLength == 84 && enabledStems->shortStemLength == 60
            && !enabledStems->noReverseStems && enabledStems->stemConnections.size() == 1,
        "Enabling stem connections disturbed a field the save did not touch");

    // The third save chose "Disable" on a document that was already disabled and changed no
    // record at all, the Finale 1.0.0 dialog giving no indication of the current state. It is
    // the regression test for finding no difference where there is none.
    const auto disabled = read("evidence/F100/F100-stemconn-disabled.mus");
    const auto disabledStems = disabled.document->getOptions()->get<StemOptions>();
    expect(!disabledStems->useStemConnections && disabledStems->stemLength == 84
            && disabledStems->shortStemLength == 60 && !disabledStems->noReverseStems,
        "The Finale 1.0.0 connection switch save did not read like its baseline");
}

// MultimeasureRestOptions has two fixed-row layouts and a zlib one, and the boundary between
// the fixed-row pair falls inside the uncompressed epoch: Finale 3.5 grew the record from one
// six-word incidence to two, moving the number adjustment and the shape down two slots. Every
// expected value below is what that fixture's exact Finale 27 companion carries.
void testMultimeasureRestRecovery()
{
    using MmRest = musx::dom::options::MultimeasureRestOptions;
    const auto read = [](const char* relative) {
        return Reader::readWithReport<TestXmlDocument>(
            std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / relative);
    };
    struct Expected
    {
        const char* path;
        const char* era;
        FormatEpoch epoch;
        int measWidth;
        int numAdjY;
        int shapeDef;
        int startAdjust;
        int endAdjust;
        bool useSymbols;
        bool autoUpdate;
    };
    // The early layout first: two Coda-banner documents whose slots 4 and 5 differ from each
    // other, which is what shows the pair is read rather than guessed. Read through the later
    // table, the Finale 2.6.3 fixture would report a shape of 0 and a number adjustment of 24.
    const Expected fixtures[] = {
        {"evidence/F100/F100-baseline.mus", "Finale 1.0.0", FormatEpoch::CodaBanner,
            360, 12, 0, 0, 0, false, false},
        {"evidence/F263/F263-baseline.mus", "Finale 2.6.3", FormatEpoch::CodaBanner,
            320, -28, 1, 0, 0, false, false},
        // The later fixed-row layout, across both epochs that carry it. Finale 3.7.2 has the
        // two-incidence record but no selector 83 at all, and Finale 97 is the one tracked
        // fixture that sets the character-rest-style flag.
        {"evidence/F372/F372-baseline.mus", "Finale 3.7.2", FormatEpoch::UncompressedLegacy,
            360, -12, 1, 0, 0, false, false},
        {"evidence/F97/Fin97-baseline.mus", "Finale 97", FormatEpoch::UncompressedLegacy,
            216, -28, 1, 0, 0, true, false},
        {"evidence/F2000/F2000-baseline.mus", "Finale 2000", FormatEpoch::UncompressedLegacy,
            360, -12, 1, 0, 0, false, false},
        {"evidence/F2005/F2005-baseline.mus", "Finale 2005", FormatEpoch::DclLegacy,
            360, -12, 1, 0, 0, false, false},
        // The zlib era in both byte orders. Finale 2012 is the fixture that exercises the
        // second row through the class encoding: it is the only one whose H-bar adjustments
        // are not zero, and reading them from the wrong offsets would leave them so.
        {"evidence/F2007/F2007-lyric-hyphens.mus", "Finale 2007", FormatEpoch::ZlibLegacy,
            360, -12, 1, 0, 0, false, true},
        {"evidence/F2012/F2012-upstem-flags.mus", "Finale 2012", FormatEpoch::ZlibLegacy,
            360, -32, 1, 30, -30, false, true},
    };
    for (const auto& fixture : fixtures) {
        const auto result = read(fixture.path);
        expect(result.report.formatEpoch == fixture.epoch,
            std::string("The fixture for ") + fixture.era + " is not the expected epoch");
        const auto mmRest = result.document->getOptions()->get<MmRest>();
        expect(static_cast<bool>(mmRest),
            std::string("No multimeasure rest options for ") + fixture.era);
        const auto wrong = [&](const char* name) {
            return std::string("The multimeasure rest ") + name + " was wrong for "
                + fixture.era;
        };
        expect(mmRest->measWidth == fixture.measWidth, wrong("measure width"));
        expect(mmRest->numAdjY == fixture.numAdjY, wrong("vertical number adjustment"));
        expect(mmRest->shapeDef == fixture.shapeDef, wrong("H-bar shape"));
        expect(mmRest->startAdjust == fixture.startAdjust, wrong("start adjustment"));
        expect(mmRest->endAdjust == fixture.endAdjust, wrong("end adjustment"));
        expect(mmRest->useSymbols == fixture.useSymbols, wrong("symbol style flag"));
        expect(mmRest->autoUpdateMmRests == fixture.autoUpdate, wrong("automatic update"));
        // "Stretch Horizontally" arrived with Finale 27, so no legacy era can state it and
        // every fixture must report it as known-false rather than as a synthesized default.
        expect(!mmRest->noHorizontalStretch, wrong("horizontal stretch flag"));
        expect(field(result, "options.multimeasureRestOptions.noHorizontalStretch").origin
                == ValueOrigin::LegacyBehavior,
            wrong("horizontal stretch provenance"));
        // Three values no document in either corpus varies, and every companion agrees with
        // the pinned baseline on all three. The early era does not store them at all, so this
        // also checks that its shorter record leaves them alone rather than reading past it.
        expect(mmRest->numStart == 2 && mmRest->useSymsThreshold == 9
                && mmRest->symSpacing == 48 && mmRest->numAdjX == 0,
            wrong("unvaried group"));
    }

    // Provenance separates the two fixed-row layouts where the values cannot. Finale 3.7.2
    // reads its H-bar adjustments from the record and finds zeros; Finale 2.6.3 has no record
    // to read them from, so they are asserted as era behavior rather than claimed as read or
    // left at the baseline's 30 and -30.
    const auto f263 = read("evidence/F263/F263-baseline.mus");
    for (const char* target : {"options.multimeasureRestOptions.startAdjust",
             "options.multimeasureRestOptions.endAdjust",
             "options.multimeasureRestOptions.autoUpdateMmRests"}) {
        expect(field(f263, target).origin == ValueOrigin::LegacyBehavior,
            std::string("A Coda-era field the source cannot state was not reported as era "
                        "behavior: ")
                + target);
    }
    expect(field(f263, "options.multimeasureRestOptions.numAdjY").origin
                == ValueOrigin::LegacyMus
            && field(f263, "options.multimeasureRestOptions.numAdjY").rawValue == -28,
        "The Coda-era number adjustment was not reported as read from slot 4");
    expect(field(f263, "options.multimeasureRestOptions.symSpacing").origin
            == ValueOrigin::Finale27Default,
        "A Coda-era field the source does not store was claimed as read");

    const auto f372 = read("evidence/F372/F372-baseline.mus");
    expect(field(f372, "options.multimeasureRestOptions.startAdjust").origin
            == ValueOrigin::LegacyMus,
        "The Finale 3.7.2 start adjustment was not read from its second incidence");
    // Selector 83 arrives with Finale 97, so a 3.7.2 document states nothing about automatic
    // updating and must not inherit the baseline's switched-on value.
    expect(field(f372, "options.multimeasureRestOptions.autoUpdateMmRests").origin
            == ValueOrigin::LegacyBehavior,
        "A Finale 3.7.2 document claimed an automatic-update setting its era has no record for");
    const auto f97 = read("evidence/F97/Fin97-baseline.mus");
    expect(field(f97, "options.multimeasureRestOptions.autoUpdateMmRests").origin
            == ValueOrigin::LegacyMus,
        "The Finale 97 automatic-update word was not read from selector 83");
    // Word 2 of that record is also set in most later documents and is not this flag. Reading
    // it instead would switch automatic updating on for the whole Finale 97 to 2006 corpus.
    expect(!f97.document->getOptions()->get<MmRest>()->autoUpdateMmRests,
        "The Finale 97 automatic-update flag was read from the wrong word");
}

// LyricOptions is spread over six numeric globals that arrive at four different releases, so
// each is gated by its own record rather than by one boundary. Every expected value below is
// what that fixture's exact Finale 27 companion carries.
void testLyricOptionsRecovery()
{
    using Lyrics = musx::dom::options::LyricOptions;
    using SyllableType = Lyrics::SyllablePosStyleType;
    using ConnectType = Lyrics::WordExtConnectStyleType;
    using ConnectIndex = Lyrics::WordExtConnectIndex;
    using AlignJustify = musx::dom::AlignJustify;
    const auto read = [](const char* relative) {
        return Reader::readWithReport<TestXmlDocument>(
            std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / relative);
    };

    struct Expected
    {
        const char* path;
        const char* era;
        FormatEpoch epoch;
        int maxHyphenSeparation;
        int wordExtLineWidth;
        bool optionalPositionsOn;
        int wordExtHorzOffset;
        int wordExtVertOffset;
    };
    // The line width is the one scalar the tracked fixtures vary, and it varies in all three
    // epochs that store it. The Coda-banner pair state neither it nor the syllable positions,
    // and their companions show exactly what the reader asserts for both.
    const Expected fixtures[] = {
        {"evidence/F100/F100-baseline.mus", "Finale 1.0.0", FormatEpoch::CodaBanner,
            144, 224, false, 4, 4},
        {"evidence/F263/F263-baseline.mus", "Finale 2.6.3", FormatEpoch::CodaBanner,
            144, 224, false, 4, 4},
        {"evidence/F372/F372-baseline.mus", "Finale 3.7.2", FormatEpoch::UncompressedLegacy,
            144, 118, false, 4, 4},
        {"evidence/F97/Fin97-baseline.mus", "Finale 97", FormatEpoch::UncompressedLegacy,
            144, 118, false, 4, 4},
        {"evidence/F2000/F2000-baseline.mus", "Finale 2000", FormatEpoch::UncompressedLegacy,
            144, 118, true, 4, 1},
        // The one tracked document that clears the "use this positioning" bit while carrying
        // the selector, which is what separates the bit from the record's mere presence.
        {"evidence/F2000/F2000-multilayer.mus", "Finale 2000 multilayer",
            FormatEpoch::UncompressedLegacy, 144, 118, false, 4, 4},
        {"evidence/F2002/F2002-baseline.mus", "Finale 2002", FormatEpoch::DclLegacy,
            144, 118, true, 4, 1},
        {"evidence/F2005/F2005-baseline.mus", "Finale 2005", FormatEpoch::DclLegacy,
            144, 224, true, 4, 1},
        // Finale 2006 moves the starting connection's horizontal offset alone, which is what
        // shows the class-level offsets are that connection's rather than a coincidence.
        {"evidence/F2006/F2006-embedded-tif.mus", "Finale 2006", FormatEpoch::DclLegacy,
            144, 224, true, 8, 1},
        // The zlib era in both byte orders.
        {"evidence/F2007/F2007-lyric-hyphens.mus", "Finale 2007", FormatEpoch::ZlibLegacy,
            144, 224, true, 4, 1},
        {"evidence/F2012/F2012-upstem-flags.mus", "Finale 2012", FormatEpoch::ZlibLegacy,
            144, 115, true, 4, 1},
    };
    for (const auto& fixture : fixtures) {
        const auto result = read(fixture.path);
        expect(result.report.formatEpoch == fixture.epoch,
            std::string("The fixture for ") + fixture.era + " is not the expected epoch");
        const auto lyrics = result.document->getOptions()->get<Lyrics>();
        expect(static_cast<bool>(lyrics),
            std::string("No lyric options for ") + fixture.era);
        const auto wrong = [&](const char* name) {
            return std::string("The lyric ") + name + " was wrong for " + fixture.era;
        };
        expect(lyrics->maxHyphenSeparation == fixture.maxHyphenSeparation,
            wrong("maximum hyphen separation"));
        expect(lyrics->wordExtLineWidth == fixture.wordExtLineWidth,
            wrong("word extension line width"));
        expect(lyrics->wordExtHorzOffset == fixture.wordExtHorzOffset,
            wrong("word extension horizontal offset"));
        expect(lyrics->wordExtVertOffset == fixture.wordExtVertOffset,
            wrong("word extension vertical offset"));
        // "Lift" and "Push" have homes of their own on selectors 29 and 30 in every epoch, so
        // these two are the only fields of this class a Coda-banner document supplies.
        expect(field(result, "options.lyricOptions.wordExtVertOffset").origin
                    == ValueOrigin::LegacyMus
                && field(result, "options.lyricOptions.wordExtHorzOffset").origin
                    == ValueOrigin::LegacyMus,
            wrong("word extension offset provenance"));
        // The four alignments never vary across the tracked fixtures, but the legacy list is
        // in neither musxdom's order nor its reverse, so an untranslated value would show up
        // here as `left` where the companion says `center`.
        const auto style = [&](SyllableType type) {
            const auto found = lyrics->syllablePosStyles.find(type);
            expect(found != lyrics->syllablePosStyles.end() && found->second,
                wrong("syllable position style"));
            return found->second;
        };
        expect(style(SyllableType::Default)->align == AlignJustify::Center
                && style(SyllableType::Default)->justify == AlignJustify::Center,
            wrong("default syllable position"));
        expect(style(SyllableType::WordExt)->align == AlignJustify::Left
                && style(SyllableType::WordExt)->justify == AlignJustify::Left,
            wrong("word extension syllable position"));
        expect(style(SyllableType::First)->align == AlignJustify::Center
                && style(SyllableType::First)->justify == AlignJustify::Left,
            wrong("first syllable position"));
        expect(style(SyllableType::SystemStart)->align == AlignJustify::Center
                && style(SyllableType::SystemStart)->justify == AlignJustify::Left,
            wrong("system start syllable position"));
        for (const auto type :
                {SyllableType::WordExt, SyllableType::First, SyllableType::SystemStart}) {
            expect(style(type)->on == fixture.optionalPositionsOn,
                wrong("optional syllable positioning switch"));
        }
        // Not ignored in any era before Finale 2012, which the pinned baseline says the
        // opposite of, so every earlier fixture asserts it. A Finale 2012 document reads
        // selector 57 word 4 instead: all six tracked fixtures clear it, matching companions
        // that omit the element.
        const bool isFinale2012 = fixture.epoch == FormatEpoch::ZlibLegacy
            && result.report.sourceVersion && result.report.sourceVersion->major >= 17;
        expect(lyrics->lyricUseEdgePunctuation == !isFinale2012,
            wrong("syllable edge punctuation setting"));
        expect(field(result, "options.lyricOptions.lyricUseEdgePunctuation").origin
                == (isFinale2012 ? ValueOrigin::LegacyMus : ValueOrigin::LegacyBehavior),
            wrong("syllable edge punctuation provenance"));

        // Both settings postdate Finale 2012, so no source of any era states either, but they
        // are handled differently on purpose. The switch is known false and is asserted even
        // though the baseline agrees; the character can only come from the baseline, because
        // restating U+002D in code would duplicate the pinned resource. See
        // testLyricPostFormatAssertions, which seeds the opposite of both.
        expect(lyrics->hyphenChar == U'-', wrong("hyphen character"));
        expect(!lyrics->useAltHyphenFont, wrong("alternate hyphen font switch"));
        expect(field(result, "options.lyricOptions.hyphenChar").origin
                == ValueOrigin::Finale27Default,
            wrong("hyphen character provenance"));
        expect(field(result, "options.lyricOptions.useAltHyphenFont").origin
                == ValueOrigin::LegacyBehavior,
            wrong("alternate hyphen font switch provenance"));
        // The pinned <lyricOptions> carries no <altHyphenFont>, so the reader declines to
        // invent one and reports nothing for it. musxdom synthesizes it in integrityCheck,
        // which has run by the time a caller sees the document.
        expect(static_cast<bool>(lyrics->altHyphenFont),
            wrong("alternate hyphen font, which musxdom should have synthesized"));
        expect(!anyReportedField(result.report, [](const auto& member, const auto&) {
                   return member.find("altHyphenFont.") != std::string::npos;
               }),
            wrong("alternate hyphen font, which the reader reported without importing"));
    }

    // Two Finale 2004 one-variable saves for the two switches that live on selector 57's
    // neighbours. Both words exist in every era and mean nothing before Finale 2004, so each
    // pair is what separates "this document turned the option off" from "this document's era
    // had no such option".
    const auto noSmartWext = read("evidence/F2004/F2004-lyropts-nosmart-wext.mus");
    const auto needUnderscore = read("evidence/F2004/F2004-lyropts-needuscore.mus");
    const auto f2004Base = read("evidence/F2004/F2004-baseline.mus");
    expect(f2004Base.document->getOptions()->get<Lyrics>()->useSmartWordExtensions
            && !noSmartWext.document->getOptions()->get<Lyrics>()->useSmartWordExtensions,
        "Smart word extensions were not read from selector 34 word 5");
    expect(!f2004Base.document->getOptions()->get<Lyrics>()->wordExtNeedUnderscore
            && needUnderscore.document->getOptions()->get<Lyrics>()->wordExtNeedUnderscore,
        "The underscore requirement was not read from selector 57 word 1");
    for (const auto& [result, target] :
            {std::pair{std::cref(noSmartWext), "options.lyricOptions.useSmartWordExtensions"},
                std::pair{std::cref(needUnderscore),
                    "options.lyricOptions.wordExtNeedUnderscore"}}) {
        expect(field(result.get(), target).origin == ValueOrigin::LegacyMus,
            std::string("A Finale 2004 lyric switch was not reported as read: ") + target);
    }
    // Neither option exists before Finale 2004, so both are known false for an earlier
    // document rather than merely unstated, and neither word may be read: Finale 2.6.3 stores
    // 12 in selector 34 word 5, which is no boolean at all.
    //
    // Smart word extensions are this class's one deliberate disagreement with Finale 27. Every
    // pre-2004 companion carries <useSmartWordExtensions/> and the pinned baseline switches it
    // on, and the reader asserts false anyway, because an upgrade turning a modern feature on
    // for an old document is not the same as the old document having had it.
    const auto f263 = read("evidence/F263/F263-baseline.mus");
    expect(!f263.document->getOptions()->get<Lyrics>()->useSmartWordExtensions
            && !f263.document->getOptions()->get<Lyrics>()->wordExtNeedUnderscore
            && !f263.document->getOptions()->get<Lyrics>()->useSmartHyphens,
        "A Coda-banner document claimed a Finale 2004 lyric feature");
    for (const char* target : {"options.lyricOptions.useSmartWordExtensions",
             "options.lyricOptions.wordExtNeedUnderscore",
             "options.lyricOptions.useSmartHyphens"}) {
        expect(field(f263, target).origin == ValueOrigin::LegacyBehavior,
            std::string("A pre-Finale-2004 lyric switch was not reported as era behavior: ")
                + target);
    }

    // Finale 2000 is the first release with a dedicated Lyric Options dialog, and the only
    // one of the legacy enum's three alignments that no corpus document uses is `right`.
    // This fixture supplies it twice over: it sets the first syllable's alignment to Right and
    // the system-start syllable's justification to Right, so both fields are exercised
    // independently and a mapping that translated only one of them would fail here. Against
    // its parent F2000-multilayer the only options record that moves is selector 87's second
    // incidence.
    const auto align = read("evidence/F2000/F2000-lyropts-align-just.mus");
    const auto alignLyrics = align.document->getOptions()->get<Lyrics>();
    const auto alignStyle = [&](SyllableType type) {
        const auto found = alignLyrics->syllablePosStyles.find(type);
        expect(found != alignLyrics->syllablePosStyles.end() && found->second,
            "A syllable position style is missing from the Finale 2000 alignment fixture");
        return found->second;
    };
    expect(alignStyle(SyllableType::First)->align == AlignJustify::Right
            && alignStyle(SyllableType::First)->justify == AlignJustify::Left,
        "The legacy alignment value 3 was not translated to right");
    expect(alignStyle(SyllableType::SystemStart)->justify == AlignJustify::Right
            && alignStyle(SyllableType::SystemStart)->align == AlignJustify::Center,
        "The legacy justification value 3 was not translated to right");
    // Setting a position's alignment enables it, and the word extension position is left off,
    // so the same record carries both states of the flag bit.
    expect(alignStyle(SyllableType::First)->on && alignStyle(SyllableType::SystemStart)->on
            && !alignStyle(SyllableType::WordExt)->on,
        "The Finale 2000 alignment fixture did not read its positioning switches");

    // The controlled Finale 1.0.0 pair that names "Lift" and "Push". It moves selector 29
    // word 5 from 4 to 5 and selector 30 word 5 from 4 to 6 and moves no other word in the
    // file, so it is what separates these two from every other value the era stores. Before
    // it, the Coda-banner epoch recovered nothing at all for this class.
    const auto lift = read("evidence/F100/F100-wext-push-6-lift-5.mus");
    const auto liftLyrics = lift.document->getOptions()->get<Lyrics>();
    expect(liftLyrics->wordExtVertOffset == 5 && liftLyrics->wordExtHorzOffset == 6,
        "The Coda-banner lift and push were not read from selectors 29 and 30");
    // With no selector 55 in this era, the starting connection takes the dialog's own values,
    // which is what the companion shows.
    expect(liftLyrics->wordExtConnectStyles.at(ConnectType::DefaultStart)->xOffset == 6
            && liftLyrics->wordExtConnectStyles.at(ConnectType::DefaultStart)->yOffset == 5,
        "The Coda-banner starting connection did not take the recovered lift and push");
    // Its sibling holds 4 and 4, so the pair discriminates rather than merely agreeing with a
    // default that happens to match.
    const auto liftBase = read("evidence/F100/F100-baseline.mus");
    expect(liftBase.document->getOptions()->get<Lyrics>()->wordExtVertOffset == 4
            && liftBase.document->getOptions()->get<Lyrics>()->wordExtHorzOffset == 4,
        "The Coda-banner baseline did not read its own lift and push");

    // Finale 3.7.2 exposes two lyric settings the Coda banner does not -- hyphen spacing and
    // word extension line thickness -- and this fixture moves all four of the era's options at
    // once. It is the only tracked document that varies the hyphen separation anywhere, and it
    // confirms lift and push a second time in a different epoch. Its ETF prints the same rows.
    const auto f372opts = read("evidence/F372/F372-lyricopts-changed.mus");
    const auto f372Lyrics = f372opts.document->getOptions()->get<Lyrics>();
    expect(f372Lyrics->maxHyphenSeparation == 157 && f372Lyrics->wordExtLineWidth == 134
            && f372Lyrics->wordExtVertOffset == 5 && f372Lyrics->wordExtHorzOffset == 6,
        "The Finale 3.7.2 four-option save was not read from selectors 15, 29, 30 and 67");
    for (const char* target : {"options.lyricOptions.maxHyphenSeparation",
             "options.lyricOptions.wordExtLineWidth", "options.lyricOptions.wordExtVertOffset",
             "options.lyricOptions.wordExtHorzOffset"}) {
        expect(field(f372opts, target).origin == ValueOrigin::LegacyMus,
            std::string("A Finale 3.7.2 lyric option was not reported as read: ") + target);
    }
    // The era stores no connection table, so the starting connection takes the dialog's own
    // values, as its companion does. Its `oneEntryEnd` is the deliberate difference: the
    // companion moves that element's horizontal offset with Push, 42 to 44, and this reader
    // keeps the pinned baseline's 42 rather than reproducing a formula one specimen cannot
    // distinguish from several others. See FORMAT_NOTES.
    expect(f372Lyrics->wordExtConnectStyles.at(ConnectType::DefaultStart)->xOffset == 6
            && f372Lyrics->wordExtConnectStyles.at(ConnectType::DefaultStart)->yOffset == 5,
        "The Finale 3.7.2 starting connection did not take the recovered lift and push");
    expect(f372Lyrics->wordExtConnectStyles.at(ConnectType::OneEntryEnd)->xOffset == 42,
        "The reader reproduced a synthesized one-entry offset instead of keeping the baseline");

    // The controlled one-variable proof for selector 57 word 4. Every other tracked Finale
    // 2012 fixture leaves "Ignore Syllable Edge Punctuation" on, so this is the only published
    // document anywhere that exercises the word set rather than clear: it moves byte 8 of
    // class 0x0047 from 0 to 1, nothing else in that record moves, and its companion gains
    // <lyricUseEdgePunctuation/>.
    const auto noIgnore = read("evidence/F2012/F2012-lyropts-noign-punct.mus");
    const auto noIgnoreLyrics = noIgnore.document->getOptions()->get<Lyrics>();
    expect(noIgnoreLyrics->lyricUseEdgePunctuation,
        "A Finale 2012 document with the ignore switch cleared did not read as not-ignored");
    expect(field(noIgnore, "options.lyricOptions.lyricUseEdgePunctuation").origin
                == ValueOrigin::LegacyMus
            && field(noIgnore, "options.lyricOptions.lyricUseEdgePunctuation").rawValue == 1,
        "The cleared ignore switch was not reported as read from selector 57 word 4");
    // Its list is the stock one, so the record carries no tail even though ignoring is off.
    expect(field(noIgnore, "options.lyricOptions.lyricPunctuationToIgnore").origin
            == ValueOrigin::Finale27Default,
        "A document with no punctuation tail was reported as carrying one");

    // Automatic lyric numbering arrives with Finale 2011 in a record that grew to hold it:
    // selector 58 is six words before it and twelve after, which is what selects the layout
    // rather than a version. Three saves carry a binary code across the four fields, so each
    // has a unique signature and none can be confused with another.
    using AutoNum = Lyrics::AutoNumberingAlign;
    struct AutoNumCase
    {
        const char* path;
        bool verses;
        bool choruses;
        bool sections;
        AutoNum type;
    };
    const AutoNumCase autoNumCases[] = {
        {"evidence/F2011/F2011-baseline.mus", false, false, false, AutoNum::Align},
        {"evidence/F2011/F2011-lyropts-autonum-type.mus", true, true, true, AutoNum::None},
        {"evidence/F2011/F2011-autonum-vs.mus", true, false, true, AutoNum::Align},
        {"evidence/F2011/F2011-autonum-cs.mus", false, true, true, AutoNum::Align},
    };
    for (const auto& c : autoNumCases) {
        const auto result = read(c.path);
        const auto ly = result.document->getOptions()->get<Lyrics>();
        const auto wrongAt = [&](const char* what) {
            return std::string("The lyric ") + what + " was wrong for " + c.path;
        };
        expect(ly->showAutoNumbersOnVerses == c.verses, wrongAt("verse auto-number switch"));
        expect(ly->showAutoNumbersOnChoruses == c.choruses,
            wrongAt("chorus auto-number switch"));
        expect(ly->showAutoNumbersOnSections == c.sections,
            wrongAt("section auto-number switch"));
        expect(ly->lyricAutoNumType == c.type, wrongAt("auto-number type"));
        for (const char* target : {"options.lyricOptions.showAutoNumbersOnVerses",
                 "options.lyricOptions.showAutoNumbersOnChoruses",
                 "options.lyricOptions.showAutoNumbersOnSections",
                 "options.lyricOptions.lyricAutoNumType"}) {
            expect(field(result, target).origin == ValueOrigin::LegacyMus,
                wrongAt("auto-number provenance"));
        }
    }
    // A Finale 2007 document carries the short record, so the same reader must assert the
    // three switches off rather than read words that are not there. All 22 companion-backed
    // Finale 2010 documents of the installs survey carry twelve bytes here and all 597
    // Finale 2011 ones carry twenty-four, which is where the marker comes from.
    const auto f2007auto = read("evidence/F2007/F2007-lyric-hyphens.mus");
    const auto f2007Lyrics = f2007auto.document->getOptions()->get<Lyrics>();
    expect(!f2007Lyrics->showAutoNumbersOnVerses && !f2007Lyrics->showAutoNumbersOnChoruses
            && !f2007Lyrics->showAutoNumbersOnSections,
        "A Finale 2007 document claimed automatic lyric numbers");
    expect(field(f2007auto, "options.lyricOptions.showAutoNumbersOnVerses").origin
            == ValueOrigin::LegacyBehavior,
        "A pre-Finale-2011 auto-number switch was not reported as era behavior");
    expect(f2007Lyrics->lyricAutoNumType == AutoNum::Align
            && field(f2007auto, "options.lyricOptions.lyricAutoNumType").origin
                == ValueOrigin::Finale27Default,
        "A pre-Finale-2011 document did not leave the numbering type to the baseline");

    // Finale 2011 stores the same tail in a different layout, and the pair of fixtures is what
    // shows they are two layouts rather than one: packed 8-bit bytes in the saving platform's
    // code page, against Finale 2012's one 16-bit code unit per character. Both containers are
    // little-endian, so reading the 2011 byte string through the word path would transpose
    // every pair of characters. The two non-ASCII bytes are what make the code page a
    // measurement: 0xc7 0xc8 is the guillemet pair in Mac Roman and `ÇÈ` in Windows-1252.
    const auto f2011punct = read("evidence/F2011/F2011-lyric-punct.mus");
    const auto f2011Lyrics = f2011punct.document->getOptions()->get<Lyrics>();
    expect(f2011Lyrics->lyricPunctuationToIgnore == "#@%&\u00ab\u00bb",
        "The Finale 2011 punctuation tail was not decoded as packed Mac Roman bytes");
    expect(field(f2011punct, "options.lyricOptions.lyricPunctuationToIgnore").origin
                == ValueOrigin::LegacyMus
            && field(f2011punct, "options.lyricOptions.lyricPunctuationToIgnore").rawValue == 6,
        "The Finale 2011 tail was not reported as read, with its byte count");
    // Its baseline sibling carries no tail and keeps musxdom's own default list, and both
    // read the switch from the same record: Finale 2011 is where that word becomes live.
    const auto f2011base = read("evidence/F2011/F2011-baseline.mus");
    expect(f2011base.document->getOptions()->get<Lyrics>()->lyricPunctuationToIgnore
            == ",.?!;:\'\"\u201c\u201d\u2018\u2019",
        "A Finale 2011 document with no tail did not keep musxdom's default list");
    for (const auto& r : {std::cref(f2011base), std::cref(f2011punct)}) {
        expect(!r.get().document->getOptions()->get<Lyrics>()->lyricUseEdgePunctuation
                && field(r.get(), "options.lyricOptions.lyricUseEdgePunctuation").origin
                    == ValueOrigin::LegacyMus,
            "A Finale 2011 document did not read the edge punctuation switch from its record");
    }

    // The ignore list is a variable-length tail on selector 57, written only when it is not
    // the stock set. The controlled fixture sets it to `#@%&`, four characters chosen to share
    // nothing with the stock list so that finding them proves where the tail begins.
    const auto punct = read("evidence/F2012/F2012-lyric-punct.mus");
    const auto punctLyrics = punct.document->getOptions()->get<Lyrics>();
    expect(punctLyrics->lyricPunctuationToIgnore == "#@%&",
        "The custom punctuation list was not read from the selector 57 tail");
    expect(field(punct, "options.lyricOptions.lyricPunctuationToIgnore").origin
                == ValueOrigin::LegacyMus
            && field(punct, "options.lyricOptions.lyricPunctuationToIgnore").rawValue == 4,
        "The custom punctuation list was not reported as read, with its code unit count");
    // Its sibling keeps the stock list, which the record does not carry at all. musxdom's
    // integrityCheck owns that default, so the reader must leave the field empty rather than
    // state the set a second time.
    const auto stock = read("evidence/F2012/F2012-baseline.mus");
    expect(stock.document->getOptions()->get<Lyrics>()->lyricPunctuationToIgnore
            == ",.?!;:\'\"\u201c\u201d\u2018\u2019",
        "A document with no punctuation tail did not fall to musxdom's own default list");
    expect(field(stock, "options.lyricOptions.lyricPunctuationToIgnore").origin
            == ValueOrigin::Finale27Default,
        "An absent punctuation tail was reported as read");
    // Finale rewrote the word extension connection table when the Lyric Options dialog was
    // dismissed, so this fixture is a second specimen for it: five of its nine styles carry a
    // vertical offset of 5 where every other tracked document carries zero.
    expect(punctLyrics->wordExtConnectStyles.at(ConnectType::DefaultEnd)->yOffset == 5
            && punctLyrics->wordExtConnectStyles.at(ConnectType::SystemStart)->yOffset == 1,
        "The rewritten connection table was not read from the punctuation fixture");

    // The connection table itself, from the fixture whose offsets are all distinct. Its
    // connection points cover all six legacy numbers, and two of the six sit in different
    // places in musxdom's enum, so a straight cast would swap the system and dotted
    // attachments.
    const auto f2006 = read("evidence/F2006/F2006-embedded-tif.mus");
    const auto connect = f2006.document->getOptions()->get<Lyrics>()->wordExtConnectStyles;
    struct ExpectedConnection
    {
        ConnectType type;
        ConnectIndex index;
        int xOffset;
        int yOffset;
    };
    const ExpectedConnection connections[] = {
        {ConnectType::DefaultStart, ConnectIndex::LyricRightBottom, 8, 1},
        {ConnectType::DefaultEnd, ConnectIndex::HeadRightLyrBaseline, 0, 5},
        {ConnectType::SystemStart, ConnectIndex::SystemLeft, 12, 1},
        {ConnectType::SystemEnd, ConnectIndex::SystemRight, -12, 5},
        {ConnectType::DottedEnd, ConnectIndex::DotRightLyrBaseline, 0, 5},
        {ConnectType::DurationEnd, ConnectIndex::DurationLyrBaseline, -8, 5},
        {ConnectType::OneEntryEnd, ConnectIndex::LyricRightBottom, 46, 5},
        {ConnectType::ZeroLengthEnd, ConnectIndex::LyricRightBottom, 0, 0},
        {ConnectType::ZeroOffset, ConnectIndex::HeadRightLyrBaseline, 0, 0},
    };
    for (const auto& expected : connections) {
        const auto found = connect.find(expected.type);
        expect(found != connect.end() && found->second,
            "A word extension connection style is missing from the Finale 2006 fixture");
        expect(found->second->connectIndex == expected.index,
            "A word extension connection point was translated wrongly");
        expect(found->second->xOffset == expected.xOffset
                && found->second->yOffset == expected.yOffset,
            "A word extension connection offset was read from the wrong word");
    }

    // Provenance separates the eras where the values cannot. A Coda-banner document states
    // neither the line width nor the syllable positions, so both must be reported as era
    // behavior rather than claimed as read or left at a Finale 27 setting that says otherwise.
    const auto f100 = read("evidence/F100/F100-baseline.mus");
    expect(field(f100, "options.lyricOptions.wordExtLineWidth").origin
            == ValueOrigin::LegacyBehavior,
        "The Coda-era word extension line width was not reported as era behavior");
    expect(field(f100, "options.lyricOptions.syllablePosStyles[wordExt].on").origin
            == ValueOrigin::LegacyBehavior,
        "A Coda-era syllable positioning switch was not reported as era behavior");
    expect(field(f100, "options.lyricOptions.maxHyphenSeparation").origin
            == ValueOrigin::Finale27Default,
        "The Coda-era selector 15 word 1 was claimed as a hyphen separation");
    // Selector 55 exists in this era and is a different option; reading it as the connection
    // table would replace nine connection styles with another option's bytes. Those bytes are
    // 16128 and 16448 in this fixture, so the failure would be loud: no connection point at
    // all, and offsets of tens of thousands. Nothing is reported for the collection either,
    // because the capture pass reports only what it reads.
    const auto f100Connect =
        f100.document->getOptions()->get<Lyrics>()->wordExtConnectStyles;
    const auto f100Start = f100Connect.find(ConnectType::DefaultStart);
    expect(f100Start != f100Connect.end() && f100Start->second
            && f100Start->second->connectIndex == ConnectIndex::LyricRightBottom
            && f100Start->second->xOffset == 4 && f100Start->second->yOffset == 4,
        "The Coda-era selector 55 was read as the word extension connection table");
    expect(!anyReportedField(f100.report, [](const auto& member, const auto&) {
               return member.find("wordExtConnectStyles") != std::string::npos;
           }),
        "A Coda-era document reported a word extension connection it never stored");

    // Finale 2003 is the last release before the smart-lyric group. All three of its switches
    // are asserted false rather than read: the words that carry them are zero in that era
    // whether the option is off or has not been invented yet, so reading them would be reading
    // nothing, and inheriting the baseline would claim a rendering this reader cannot build.
    // Smart hyphens and smart word extensions are implemented as hyphen and word-extension
    // smart shapes, which Finale 27 manufactures on upgrade and this reader does not.
    const auto f2003 = read("evidence/F2003/F2003-baseline.mus");
    const auto f2003Lyrics = f2003.document->getOptions()->get<Lyrics>();
    expect(!f2003Lyrics->useSmartHyphens && !f2003Lyrics->useSmartWordExtensions
            && !f2003Lyrics->wordExtNeedUnderscore,
        "A Finale 2003 document claimed a smart-lyric feature its era did not have");
    for (const char* target : {"options.lyricOptions.useSmartHyphens",
             "options.lyricOptions.useSmartWordExtensions",
             "options.lyricOptions.wordExtNeedUnderscore"}) {
        expect(field(f2003, target).origin == ValueOrigin::LegacyBehavior,
            std::string("A pre-Finale-2004 smart-lyric switch was not reported as era "
                        "behavior: ")
                + target);
    }
    const auto f2004 = read("evidence/F2004/F2004-baseline.mus");
    expect(f2004.document->getOptions()->get<Lyrics>()->useSmartHyphens
            && field(f2004, "options.lyricOptions.useSmartHyphens").origin
                == ValueOrigin::LegacyMus,
        "The Finale 2004 smart-hyphen switch was not read from selector 35");
    expect(f2004.document->getOptions()->get<Lyrics>()->wordExtMinLength == 38
            && field(f2004, "options.lyricOptions.wordExtMinLength").origin
                == ValueOrigin::LegacyMus,
        "The Finale 2004 word extension minimum length was not read from selector 57");
}

// Selector 24 is the default-font array from well before the DCL era, but the reader used to
// gate that layout to DCL alone, so every Finale 3.0 through 2000 document reported all 45
// font options as Finale 27 defaults while its source held 40 of them.
void testUncompressedFontOptions()
{
    using FontOptions = musx::dom::options::FontOptions;
    using FontType = FontOptions::FontType;
    const auto read = [](const char* relative) {
        return Reader::readWithReport<TestXmlDocument>(
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
    const auto synthetic = Reader::readWithReport<TestXmlDocument>(makeUncompressedMusWithFontOptions());
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
        return Reader::readWithReport<TestXmlDocument>(
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
    const auto result = Reader::readWithReport<TestXmlDocument>(
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
    const auto result = Reader::readWithReport<TestXmlDocument>(
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
    const auto result = Reader::readWithReport<TestXmlDocument>(
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

void testFinale2006EmbeddedTiff()
{
    const auto evidence = std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
        / "evidence/F2006";
    const auto linked = Reader::readWithReport<TestXmlDocument>(evidence / "F2006-linked-tiff.mus");
    const auto embeddedTif = Reader::readWithReport<TestXmlDocument>(evidence / "F2006-embedded-tif.mus");
    const auto embeddedTiff = Reader::readWithReport<TestXmlDocument>(evidence / "F2006-embedded-tiff.mus");
    const auto epsThenTiff = Reader::readWithReport<TestXmlDocument>(evidence / "F2006-eps-then-tiff.mus");

    expect(linked.document->getEmbeddedGraphics().empty(),
        "A linked Finale 2006 TIFF was mistaken for an embedded file");
    const auto& oneGraphic = embeddedTif.document->getEmbeddedGraphics();
    expect(oneGraphic.size() == 1 && oneGraphic.at(1).extension == "tif"
            && oneGraphic.at(1).bytes.size() == 458252,
        "The Finale 2006 .tif embedded payload was not recovered");
    const auto& twoGraphics = embeddedTiff.document->getEmbeddedGraphics();
    expect(twoGraphics.size() == 2 && twoGraphics.at(1).extension == "tif"
            && twoGraphics.at(2).extension == "tif"
            && twoGraphics.at(1).bytes == twoGraphics.at(2).bytes
            && twoGraphics.at(1).bytes == oneGraphic.at(1).bytes,
        "The Finale 2006 page and ShapeDef TIFF payloads did not preserve encounter order");
    const auto shapeAssignments = embeddedTiff.document->getOthers()
        ->getArray<musx::dom::others::ShapeGraphicAssign>(musx::dom::SCORE_PARTID);
    expect(std::any_of(shapeAssignments.begin(), shapeAssignments.end(), [&](const auto& assignment) {
        return twoGraphics.contains(assignment->graphicCmper);
    }), "The Finale 2006 ShapeDef graphic assignment did not resolve to an embedded TIFF");
    const auto& orderedGraphics = epsThenTiff.document->getEmbeddedGraphics();
    expect(orderedGraphics.size() == 2 && orderedGraphics.at(1).extension == "eps"
            && orderedGraphics.at(1).bytes.size() == 82381
            && orderedGraphics.at(2).extension == "tif"
            && orderedGraphics.at(2).bytes == oneGraphic.at(1).bytes,
        "Finale 2006 embedded comparators did not follow EPS-then-TIFF insertion order");
    const auto measureGraphic = epsThenTiff.document->getDetails()
        ->get<musx::dom::details::MeasureGraphicAssign>(
            musx::dom::SCORE_PARTID, 1, 2, musx::dom::Inci(0));
    expect(measureGraphic && measureGraphic->version == 0x100
            && measureGraphic->left == 120 && measureGraphic->bottom == -324
            && measureGraphic->width == 336 && measureGraphic->height == 168
            && measureGraphic->fDescId == 1 && measureGraphic->savedRecord
            && measureGraphic->origWidth == 336 && measureGraphic->origHeight == 168
            && measureGraphic->graphicCmper == 1
            && orderedGraphics.contains(measureGraphic->graphicCmper),
        "The Finale 2006 measure graphic did not resolve to its embedded EPS");
}

void testFinale372MeasureGraphic()
{
    const auto result = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F372/F372-measure-graphic.mus");
    const auto assignment = result.document->getDetails()
        ->get<musx::dom::details::MeasureGraphicAssign>(
            musx::dom::SCORE_PARTID, 1, 3, musx::dom::Inci(0));
    expect(assignment && assignment->version == 0x100
            && assignment->left == 116 && assignment->bottom == -348
            && assignment->width == 336 && assignment->height == 168
            && assignment->fDescId == 1 && assignment->savedRecord
            && assignment->origWidth == 336 && assignment->origHeight == 168
            && assignment->graphicCmper == 0,
        "The Finale 3.7.2 linked measure graphic was not recovered");
    expect(result.document->getEmbeddedGraphics().empty(),
        "A Finale 3.7.2 linked graphic was mistaken for an embedded payload");
}

void testFinale372PageGraphic()
{
    using PageGraphicAssign = musx::dom::others::PageGraphicAssign;
    const auto result = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F372/F372-page-graphic.mus");
    const auto assignment = result.document->getOthers()
        ->get<PageGraphicAssign>(musx::dom::SCORE_PARTID, 1, musx::dom::Inci(0));
    expect(assignment && assignment->version == 0x100
            && assignment->left == 920 && assignment->bottom == -508
            && assignment->width == 727 && assignment->height == 764
            && assignment->fDescId == 1 && !assignment->hidden
            && assignment->displayType == PageGraphicAssign::PageAssignType::One
            && assignment->hAlign == PageGraphicAssign::HorizontalAlignment::Left
            && assignment->vAlign == PageGraphicAssign::VerticalAlignment::Top
            && assignment->posFrom == PageGraphicAssign::PositionFrom::PageEdge
            && assignment->startPage == 1 && assignment->endPage == 1
            && assignment->savedRecord
            && assignment->origWidth == 727 && assignment->origHeight == 764
            && assignment->graphicCmper == 0,
        "The Finale 3.7.2 linked page graphic was not recovered");
    expect(result.document->getEmbeddedGraphics().empty(),
        "A Finale 3.7.2 linked TIFF was mistaken for an embedded payload");
}

void testFinale2012GraphicTypes()
{
    const auto result = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2012/F2012-graphics-types.mus");
    const auto& graphics = result.document->getEmbeddedGraphics();
    expect(graphics.size() == 6
            && graphics.at(1).extension == "gif"
            && graphics.at(2).extension == "jpg"
            && graphics.at(3).extension == "jpg"
            && graphics.at(4).extension == "gif"
            && graphics.at(5).extension == "tif"
            && graphics.at(6).extension == "pdf"
            && graphics.at(1).bytes == graphics.at(4).bytes
            && graphics.at(2).bytes == graphics.at(3).bytes,
        "The Finale 2012 graphic types or per-assignment copies were not recovered");

    const auto page = result.document->getOthers()
        ->get<musx::dom::others::PageGraphicAssign>(
            musx::dom::SCORE_PARTID, 1, musx::dom::Inci(0));
    const auto gifMeasure = result.document->getDetails()
        ->get<musx::dom::details::MeasureGraphicAssign>(
            musx::dom::SCORE_PARTID, 1, 2, musx::dom::Inci(0));
    const auto pdfMeasure = result.document->getDetails()
        ->get<musx::dom::details::MeasureGraphicAssign>(
            musx::dom::SCORE_PARTID, 1, 10, musx::dom::Inci(0));
    expect(page && page->graphicCmper == 2 && graphics.contains(page->graphicCmper)
            && gifMeasure && gifMeasure->graphicCmper == 1
            && graphics.contains(gifMeasure->graphicCmper)
            && pdfMeasure && pdfMeasure->graphicCmper == 6
            && graphics.contains(pdfMeasure->graphicCmper),
        "A Finale 2012 page or measure assignment did not resolve its embedded graphic");

    const auto shapeAssignments = result.document->getOthers()
        ->getArray<musx::dom::others::ShapeGraphicAssign>(musx::dom::SCORE_PARTID);
    expect(shapeAssignments.size() == 3
            && shapeAssignments[0]->graphicCmper == 3
            && shapeAssignments[1]->graphicCmper == 4
            && shapeAssignments[2]->graphicCmper == 5
            && std::all_of(shapeAssignments.begin(), shapeAssignments.end(), [&](const auto& item) {
                return graphics.contains(item->graphicCmper);
            }),
        "The Finale 2012 ShapeDef assignments did not resolve all three embedded graphics");
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
        const auto result = Reader::readWithReport<TestXmlDocument>(path);
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
    const auto result = Reader::readWithReport<TestXmlDocument>(makeUncompressedMus());
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
    const auto result = Reader::readWithReport<TestXmlDocument>(makeCodaBannerMus());
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
    const auto result = Reader::readWithReport<TestXmlDocument>(makeZlibMus());
    expect(result.report.formatEpoch == FormatEpoch::ZlibLegacy,
        "Synthetic Finale 2012 file was not classified as zlib legacy");
    expect(result.report.blocks.size() == 5, "Synthetic zlib block count is incorrect");
    expect(result.report.blocks.front().decodedSize == 3,
        "Synthetic zlib decoded size is incorrect");
    expect(field(result, "options.musicSpacing.minWidth").origin
        == ValueOrigin::Finale27Default,
        "Unsupported zlib-era option was not retained as a default");
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
         {std::pair<std::uint16_t, std::string_view>{std::uint16_t(0x0013),
              "\x89PNG\r\n\x1a\n padding"},
          std::pair<std::uint16_t, std::string_view>{std::uint16_t(0x001d),
              "%!PS-Adobe-3.0 EPSF-3.0"}}) {
        const auto result = Reader::readWithReport<TestXmlDocument>(
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
        const auto expectedStoredSize = graphic.size() + (terminalType == 0x0013 ? 11 : 0);
        expect(terminal.decodedSize == expectedStoredSize,
            "A stored block's payload begins after six header bytes, not ten");
        expect(!terminal.checksumPresent,
            "A stored block was reported as carrying a checksum");
        if (terminalType == 0x0013) {
            const auto& embedded = result.document->getEmbeddedGraphics();
            expect(embedded.size() == 1 && embedded.at(1).bytes.size() == graphic.size()
                    && embedded.at(1).extension == "png",
                "A framed embedded graphic was not installed in the musxdom document");
        } else {
            expect(result.document->getEmbeddedGraphics().empty(),
                "An unrelated stored terminal block was mistaken for embedded graphics");
        }
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
        const auto result = Reader::readWithReport<TestXmlDocument>(
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
    const auto result = Reader::readWithReport<TestXmlDocument>(std::vector<std::uint8_t>{1, 2, 3, 4});
    expect(result.document == nullptr, "Arbitrary input was accepted as a MUS file");
    expect(hasDiagnostic(result.report, musx::util::Logger::LogLevel::Error),
        "A failed import did not report why at error level");
    // Error is the one level with an absolute meaning, so it must not appear when a
    // document was produced. Every other level accompanies a usable result.
    const auto good = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2012/F2012-upstem-flags.mus");
    expect(good.document != nullptr, "A known-good fixture failed to import");
    expect(!hasDiagnostic(good.report, musx::util::Logger::LogLevel::Error),
        "A successful import reported an error-level diagnostic");
}

void testShapeDefinitions()
{
    using namespace musx::dom;
    const auto readShapeFixture = [](std::string_view relative) {
        return Reader::readWithReport<TestXmlDocument>(
            std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / relative);
    };

    const auto verifyModern = [&](std::string_view path, const std::string& what) {
        const auto result = readShapeFixture(path);
        const auto shape = result.document->getOthers()->get<others::ShapeDef>(SCORE_PARTID, 1);
        const auto instructions = result.document->getOthers()
            ->get<others::ShapeInstructionList>(SCORE_PARTID, 1);
        const auto data = result.document->getOthers()->get<others::ShapeData>(SCORE_PARTID, 1);
        expect(shape && shape->instructionList == 1 && shape->dataList == 1
                && shape->shapeType == others::ShapeDef::ShapeType::Other,
            what + " did not recover the shape definition");
        // The fixed rows contain 21 slots, but the last is the zero terminator/padding.
        expect(instructions && instructions->instructions.size() == 20,
            what + " did not recover the complete instruction list");
        expect(instructions->instructions[0]->type == ShapeDefInstructionType::StartGroup
                && instructions->instructions[0]->numData == 11
                && instructions->instructions[1]->type == ShapeDefInstructionType::StartObject
                && instructions->instructions.back()->type == ShapeDefInstructionType::EndGroup,
            what + " mistranslated packed instruction tags");
        expect(data && data->values.size() == 66 && data->values[0] == 0
                && data->values[2] == (std::numeric_limits<int>::max)()
                && data->values[3] == (std::numeric_limits<int>::min)(),
            what + " did not recover signed 32-bit shape data");
        expect(field(result, "others.shapeDef[1].instructionList").origin
                    == ValueOrigin::LegacyMus
                && field(result, "others.shapeList[1].instructions[0].numData").rawValue == 11
                && field(result, "others.shapeData[1].values[3]").rawValue
                    == (std::numeric_limits<int>::min)(),
            what + " did not report shape provenance and raw values");
    };

    // Same logical record in the fixed-row and both zlib byte orders.
    verifyModern("evidence/F2002/F2002-baseline.mus", "Finale 2002");
    verifyModern("evidence/F2007/F2007-lyric-hyphens.mus", "Finale 2007");
    verifyModern("evidence/F2012/F2012-upstem-flags.mus", "Finale 2012");

    const auto zlibTypes = readShapeFixture("evidence/F2012/F2012-graphics-types.mus");
    expect(zlibTypes.document->getOthers()
                ->get<others::ShapeDef>(SCORE_PARTID, 2)->shapeType
                == others::ShapeDef::ShapeType::Clef
            && zlibTypes.document->getOthers()
                ->get<others::ShapeDef>(SCORE_PARTID, 3)->shapeType
                == others::ShapeDef::ShapeType::Clef
            && zlibTypes.document->getOthers()
                ->get<others::ShapeDef>(SCORE_PARTID, 4)->shapeType
                == others::ShapeDef::ShapeType::Expression,
        "Zlib ShapeDef word 2 was not recovered as shapeType");

    // Shape list 2 in the fixed-row fixture has two stale packed instructions after
    // its zero terminator. Finale's own ETF/MUSX representation stops at the zero.
    const auto terminated = readShapeFixture("evidence/F2002/F2002-baseline.mus");
    const auto terminatedList = terminated.document->getOthers()
        ->get<others::ShapeInstructionList>(SCORE_PARTID, 2);
    const auto terminatedData = terminated.document->getOthers()
        ->get<others::ShapeData>(SCORE_PARTID, 2);
    expect(terminatedList && terminatedList->instructions.size() == 12
            && terminatedData && terminatedData->values.size() == 45,
        "A zero shape-instruction terminator did not suppress stale trailing instructions");

    const auto early = readShapeFixture("evidence/F263/F263-baseline.mus");
    const auto earlyShape = early.document->getOthers()
        ->get<others::ShapeDef>(SCORE_PARTID, 1);
    const auto earlyInstructions = early.document->getOthers()
        ->get<others::ShapeInstructionList>(SCORE_PARTID, 1);
    const auto earlyData = early.document->getOthers()->get<others::ShapeData>(SCORE_PARTID, 1);
    expect(earlyShape && earlyShape->instructionList == 1 && earlyShape->dataList == 1
            && earlyShape->shapeType == others::ShapeDef::ShapeType::Other,
        "Finale 2.6 bounding words were mistaken for a modern shape type");
    expect(earlyInstructions && earlyInstructions->instructions.size() == 9
            && earlyInstructions->instructions[0]->type == ShapeDefInstructionType::LineWidth
            && earlyInstructions->instructions[0]->numData == 1,
        "Finale 2.6 revision-1 instructions were not translated");
    expect(earlyData && earlyData->values.size() == 15 && earlyData->values[0] == 1024,
        "Finale 2.6 hundredths-of-a-point line width was not converted to Efix (size "
            + std::to_string(earlyData ? earlyData->values.size() : 0) + ", first "
            + std::to_string(earlyData && !earlyData->values.empty()
                    ? earlyData->values.front() : 0)
            + ")");
    expect(field(early, "others.shapeData[1].values[0]").rawValue == 400,
        "The converted Finale 2.6 line width did not retain its source value in the report");

    const auto blank = Reader::readWithReport<TestXmlDocument>(makeCodaBannerMus(
        ByteOrder::LittleEndian, "PC 1.0+", true));
    const auto blankShape = blank.document->getOthers()
        ->get<others::ShapeDef>(SCORE_PARTID, 19);
    expect(blankShape && blankShape->instructionList == 19 && blankShape->dataList == 19
            && blankShape->recognize() == KnownShapeDefType::Blank,
        "An intentionally blank Coda shape was not recognized as Blank without losing its references");
    expect(field(blank, "others.shapeDef[19].instructionList").origin == ValueOrigin::LegacyMus
            && field(blank, "others.shapeDef[19].instructionList").rawValue == 19
            && field(blank, "others.shapeDef[19].dataList").origin == ValueOrigin::LegacyMus
            && field(blank, "others.shapeDef[19].dataList").rawValue == 19,
        "A blank Coda shape did not retain its source references in the report");
}

void testSmartShapeCustomLines()
{
    using namespace musx::dom;
    using LineTarget = others::SmartShapeCustomLine;

    // Fixed-row Finale 2000: cmper 1 is a Char line ('~', size 24, baseline -88 EMs), cmper
    // 2 a bare Solid line (width 118 Efix). Confirmed against the tracked ETF/MUSX companion.
    const auto fixedRow = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2000/F2000-baseline.mus");
    const auto charLine = fixedRow.document->getOthers()->get<LineTarget>(SCORE_PARTID, 1);
    REQUIRE(charLine != nullptr);
    CHECK(charLine->lineStyle == LineTarget::LineStyle::Char);
    REQUIRE(charLine->charParams != nullptr);
    CHECK(charLine->charParams->lineChar == U'~');
    CHECK(charLine->charParams->font->fontSize == 24);
    CHECK(charLine->charParams->baselineShiftEms == -88);
    expect(field(fixedRow, "others.smartShapeCustomLine[1].charParams.lineChar").origin
                == ValueOrigin::LegacyMus
            && field(fixedRow, "others.smartShapeCustomLine[1].charParams.lineChar").rawValue
                == 126,
        "A recovered Char line style did not report its source raw character");

    const auto solidLine = fixedRow.document->getOthers()->get<LineTarget>(SCORE_PARTID, 2);
    REQUIRE(solidLine != nullptr);
    CHECK(solidLine->lineStyle == LineTarget::LineStyle::Solid);
    REQUIRE(solidLine->solidParams != nullptr);
    CHECK(solidLine->solidParams->lineWidth == 118);

    // Finale 27 upgrades this baseline document to a third, synthesized default (a Solid
    // line with an ArrowheadPreset end cap) that the Finale 2000 source itself never wrote.
    // The importer must not fabricate that object from nothing.
    CHECK(fixedRow.document->getOthers()->get<LineTarget>(SCORE_PARTID, 3) == nullptr);

    // Every text-anchor and line-adjustment offset of one record, each set to its own value
    // so that no two slots can be confused. The five X offsets interleave with the Y offsets
    // they pair with rather than being grouped, and the line adjustments follow in musxdom's
    // own declaration order. Confirmed against the Finale 27 companion, which names all
    // fifteen.
    const auto offsets = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2000/F2000-ssline-offsets.mus");
    const auto positioned = offsets.document->getOthers()->get<LineTarget>(SCORE_PARTID, 3);
    REQUIRE(positioned != nullptr);
    CHECK(positioned->leftStartRawTextId == 1);
    CHECK(positioned->leftContRawTextId == 2);
    CHECK(positioned->rightEndRawTextId == 3);
    CHECK(positioned->centerFullRawTextId == 4);
    CHECK(positioned->centerAbbrRawTextId == 5);
    CHECK(positioned->leftStartX == 11);
    CHECK(positioned->leftStartY == 13);
    CHECK(positioned->leftContX == 17);
    CHECK(positioned->leftContY == 19);
    CHECK(positioned->rightEndX == 23);
    CHECK(positioned->rightEndY == -29);
    CHECK(positioned->centerFullX == 31);
    CHECK(positioned->centerFullY == 37);
    CHECK(positioned->centerAbbrX == 41);
    CHECK(positioned->centerAbbrY == 43);
    CHECK(positioned->lineStartX == 47);
    CHECK(positioned->lineEndX == 59);
    CHECK(positioned->lineContX == 53);
    // Finale offers one vertical control for the whole line and writes it to both slots, so
    // the two must come out equal from two different words rather than one being copied.
    CHECK(positioned->lineStartY == 61);
    CHECK(positioned->lineEndY == 61);
    expect(field(offsets, "others.smartShapeCustomLine[3].lineEndY").origin
            == ValueOrigin::LegacyMus,
        "The second half of the synced vertical adjustment was not recovered from the source");

    // The remaining three line styles of the same document, each settling one thing the
    // earlier samples could not distinguish.
    const auto dashed = offsets.document->getOthers()->get<LineTarget>(SCORE_PARTID, 4);
    REQUIRE(dashed != nullptr);
    REQUIRE(dashed->dashedParams != nullptr);
    CHECK(dashed->dashedParams->lineWidth == 118);
    // 3 and 7 EVPU. Unequal at last, so the two slots are told apart rather than assumed.
    CHECK(dashed->dashedParams->dashOn == 192);
    CHECK(dashed->dashedParams->dashOff == 448);

    // A Char line in a named text font, bold and italic. The stored character is a byte in
    // that font's encoding, so Mac Roman 199 is the code point 171 and not the number the
    // file holds; the report keeps the source value while the document carries the code point.
    const auto styled = offsets.document->getOthers()->get<LineTarget>(SCORE_PARTID, 5);
    REQUIRE(styled != nullptr);
    REQUIRE(styled->charParams != nullptr);
    CHECK(styled->charParams->lineChar == U'\u00ab');
    CHECK(styled->charParams->font->fontId == 6);
    CHECK(styled->charParams->font->fontSize == 17);
    CHECK(styled->charParams->font->bold);
    CHECK(styled->charParams->font->italic);
    CHECK_FALSE(styled->charParams->font->underline);
    CHECK(styled->charParams->baselineShiftEms == -83);
    expect(field(offsets, "others.smartShapeCustomLine[5].charParams.lineChar").rawValue == 199,
        "The report did not keep the byte the source actually stored");

    // A hook at one end and a custom arrowhead shape at the other, so each cap's shared value
    // slot is read as the one its own type names.
    const auto capped = offsets.document->getOthers()->get<LineTarget>(SCORE_PARTID, 6);
    REQUIRE(capped != nullptr);
    CHECK(capped->lineCapStartType == LineTarget::LineCapType::Hook);
    CHECK(capped->lineCapEndType == LineTarget::LineCapType::ArrowheadCustom);
    CHECK(capped->lineCapStartHookLength == 1984);
    CHECK(capped->lineCapEndArrowId == 2);
    CHECK(capped->lineCapStartArrowId == 0);
    CHECK(capped->lineCapEndHookLength == 0);

    // Confirmed absent, not merely unread: no `ls` row occurs anywhere in a genuinely
    // pre-2000 uncompressed source, and none at all in CodaBanner.
    const auto pre2000 = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / "evidence/F97/Fin97-baseline.mus");
    CHECK(pre2000.document->getOthers()->getArray<LineTarget>(SCORE_PARTID).empty());
    const auto coda = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / "evidence/F263/F263-baseline.mus");
    CHECK(coda.document->getOthers()->getArray<LineTarget>(SCORE_PARTID).empty());

    // Zlib class 0x00de, Finale 2012: the character slot widens from one word to two and
    // every later field shifts by one word to keep the record 36 words long. cmper 2 here
    // is a bare Solid line (width 224); cmper 3 adds an ArrowheadPreset end cap, which
    // depends on the post-shift cap-type and cap-arrow slots being read correctly.
    const auto zlib = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2012/F2012-baseline.mus");
    const auto zlibChar = zlib.document->getOthers()->get<LineTarget>(SCORE_PARTID, 1);
    REQUIRE(zlibChar != nullptr);
    REQUIRE(zlibChar->charParams != nullptr);
    CHECK(zlibChar->charParams->lineChar == U'~');
    CHECK(zlibChar->charParams->font->fontSize == 24);
    CHECK(zlibChar->charParams->baselineShiftEms == -88);

    // The same six line styles back-saved to Finale 2012. Only the Char parameter block moves
    // with the widened character: a Dashed record keeps its dash lengths where every earlier
    // layout put them, while everything after the parameter block shifts for all three styles.
    const auto wide = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2012/F2012-ssline-offsets.mus");
    const auto wideDashed = wide.document->getOthers()->get<LineTarget>(SCORE_PARTID, 4);
    REQUIRE(wideDashed != nullptr);
    REQUIRE(wideDashed->dashedParams != nullptr);
    CHECK(wideDashed->dashedParams->dashOn == 192);
    CHECK(wideDashed->dashedParams->dashOff == 448);
    const auto wideStyled = wide.document->getOthers()->get<LineTarget>(SCORE_PARTID, 5);
    REQUIRE(wideStyled != nullptr);
    REQUIRE(wideStyled->charParams != nullptr);
    // Already a code point in this era, so nothing is decoded and the answer is the same one
    // the fixed-row source reaches by decoding.
    CHECK(wideStyled->charParams->lineChar == U'\u00ab');
    CHECK(wideStyled->charParams->font->fontId == 6);
    CHECK(wideStyled->charParams->font->fontSize == 17);
    CHECK(wideStyled->charParams->font->bold);
    CHECK(wideStyled->charParams->font->italic);
    CHECK(wideStyled->charParams->baselineShiftEms == -83);
    const auto wideCapped = wide.document->getOthers()->get<LineTarget>(SCORE_PARTID, 6);
    REQUIRE(wideCapped != nullptr);
    CHECK(wideCapped->lineCapStartHookLength == 1984);
    CHECK(wideCapped->lineCapEndArrowId == 2);
    const auto widePositions = wide.document->getOthers()->get<LineTarget>(SCORE_PARTID, 3);
    REQUIRE(widePositions != nullptr);
    CHECK(widePositions->leftStartX == 11);
    CHECK(widePositions->centerAbbrY == 43);
    CHECK(widePositions->lineContX == 53);

    const auto zlibCapped = zlib.document->getOthers()->get<LineTarget>(SCORE_PARTID, 3);
    REQUIRE(zlibCapped != nullptr);
    CHECK(zlibCapped->lineStyle == LineTarget::LineStyle::Solid);
    REQUIRE(zlibCapped->solidParams != nullptr);
    CHECK(zlibCapped->solidParams->lineWidth == 224);
    CHECK(zlibCapped->lineCapEndType == LineTarget::LineCapType::ArrowheadPreset);
    CHECK(zlibCapped->lineCapEndArrowId == 1);
}


// A clef and a stem connection both store their character as a byte in the encoding of the
// font that draws them, not as a code point. Every sampled document sets both in a music
// font, where the byte is a glyph number and survives unchanged, so the distinction is only
// visible once one of them is pointed at a text font.
void testLegacyCharacterFonts()
{
    using namespace musx::dom;

    const auto source = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2002/F2002-clef-stem-font.mus");

    // Font 9 is Arial, a Mac-bank text font, so Mac Roman 199 is the code point 171.
    const auto clefs = source.document->getOptions()->get<options::ClefOptions>();
    REQUIRE(clefs != nullptr);
    REQUIRE(clefs->clefDefs.size() > 4);
    const auto& owned = clefs->clefDefs[4];
    CHECK(owned->useOwnFont);
    REQUIRE(owned->font != nullptr);
    CHECK(owned->font->fontId == 9);
    CHECK(owned->font->fontSize == 23);
    CHECK(owned->font->bold);
    CHECK(owned->font->italic);
    CHECK(owned->clefChar == U'\u00ab');
    // The report keeps the byte the source stored, which is the only place it survives.
    expect(field(source, "options.clefOptions.clefDefs[4].clefChar").rawValue == 199,
        "The clef report did not keep the byte the source actually stored");

    const auto stems = source.document->getOptions()->get<options::StemOptions>();
    REQUIRE(stems != nullptr);
    REQUIRE(stems->stemConnections.size() > 1);
    // Connection 0 names font 0, the document's default music font: a symbol font, whose byte
    // is a glyph number and must come through untouched.
    CHECK(stems->stemConnections[0]->fontId == 0);
    CHECK(stems->stemConnections[0]->symbol == 192);
    CHECK(stems->stemConnections[1]->fontId == 9);
    CHECK(stems->stemConnections[1]->symbol == U'\u00ab');
    CHECK(stems->stemConnections[1]->upStemVert == 320);
    CHECK(stems->stemConnections[1]->downStemHorz == -448);
    expect(field(source, "options.stemOptions.stemConnections[1].symbol").rawValue == 199,
        "The stem report did not keep the byte the source actually stored");

    // The flat symbol insert of Text Options, the third record type that stores a bare
    // character. The four inserts beside it are the control: they keep font 0, so their bytes
    // are glyph numbers and must read back exactly as stored.
    const auto texts = source.document->getOptions()->get<options::TextOptions>();
    REQUIRE(texts != nullptr);
    const auto flat = texts->symbolInserts.find(options::AccidentalInsertSymbolType::Flat);
    REQUIRE(flat != texts->symbolInserts.end());
    REQUIRE(flat->second != nullptr);
    REQUIRE(flat->second->symFont != nullptr);
    CHECK(flat->second->symFont->fontId == 9);
    CHECK(flat->second->symChar == U'\u00ab');
    expect(field(source, "options.textOptions.symbolInserts[flat].symChar").rawValue == 199,
        "The symbol-insert report did not keep the byte the source actually stored");
    const auto natural
        = texts->symbolInserts.find(options::AccidentalInsertSymbolType::Natural);
    REQUIRE(natural != texts->symbolInserts.end());
    REQUIRE(natural->second != nullptr);
    REQUIRE(natural->second->symFont != nullptr);
    CHECK(natural->second->symFont->fontId == 0);
    CHECK(natural->second->symChar == 110);

    // Its parent, which differs only by the three edits under test, has neither.
    const auto parent = Reader::readWithReport<TestXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR)
            / "evidence/F2002/F2002-empty.mus");
    const auto parentStems = parent.document->getOptions()->get<options::StemOptions>();
    REQUIRE(parentStems != nullptr);
    CHECK(parentStems->stemConnections.size() == 1);
}

} // namespace

TEST_CASE("Controlled DCL file", "[reader]") { testControlledDclFile(); }
TEST_CASE("Font definitions", "[reader]") { testFontDefinitions(); }
TEST_CASE("Font options capture", "[reader]") { testFontOptionsCapture(); }
TEST_CASE("Clef options capture", "[reader]") { testClefOptionsCapture(); }
TEST_CASE("Legacy character fonts", "[reader]") { testLegacyCharacterFonts(); }
TEST_CASE("Stem connection capture", "[reader]") { testStemConnectionCapture(); }
TEST_CASE("Stem scalar recovery", "[reader]") { testStemScalarRecovery(); }
TEST_CASE("Multimeasure rest recovery", "[reader]") { testMultimeasureRestRecovery(); }
TEST_CASE("Lyric options recovery", "[reader]") { testLyricOptionsRecovery(); }
TEST_CASE("Uncompressed font options", "[reader]") { testUncompressedFontOptions(); }
TEST_CASE("Uncompressed fixtures", "[reader]") { testUncompressedFixtures(); }
TEST_CASE("Class record era", "[reader]") { testClassRecordEra(); }
TEST_CASE("Big-endian class records", "[reader]") { testBigEndianClassRecords(); }
TEST_CASE("Finale 2006 remains fixed-row", "[reader]")
{
    testFinale2006RemainsFixedRow();
}
TEST_CASE("Finale 2006 embedded TIFF", "[reader]")
{
    testFinale2006EmbeddedTiff();
}
TEST_CASE("Finale 3.7.2 measure graphic", "[reader]")
{
    testFinale372MeasureGraphic();
}
TEST_CASE("Finale 3.7.2 page graphic", "[reader]")
{
    testFinale372PageGraphic();
}
TEST_CASE("Finale 2012 graphic types", "[reader]")
{
    testFinale2012GraphicTypes();
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
// A Coda-banner document's pools, built directly so the walk can be given an empty one. The
// era chains pools nose to tail: an 8-byte prologue of page count and page size, then that
// many 512-byte pages, and the chain ends where the next four bytes are not the page size.
std::vector<std::uint8_t> makeCodaBannerPools(const std::vector<std::uint32_t>& pageCounts)
{
    std::vector<std::uint8_t> result(0x200, 0);
    const std::string banner = "Finale(TM) 2.6 Copyright 1987 by Coda. All rights reserved.";
    writeFixed(result, 0, banner, banner.size());
    for (const auto pages : pageCounts) {
        write32(result, pages, ByteOrder::BigEndian);
        write32(result, 0x200, ByteOrder::BigEndian);
        result.resize(result.size() + std::size_t{pages} * 0x200, 0);
    }
    // The text region that follows the last pool. Its first four bytes are a chunk length, so
    // they end the walk on their own without a terminator of the era's making.
    const std::string text = "^text()";
    write32(result, static_cast<std::uint32_t>(text.size()), ByteOrder::BigEndian);
    result.insert(result.end(), text.begin(), text.end());
    return result;
}

// An empty pool is an ordinary pool, not the end of the chain. The earliest documents store
// nothing in their details pool and still carry an entries pool behind it, so a walk that
// stopped on the first zero-page prologue would never reach it.
void testCodaBannerEmptyPool()
{
    const auto full = finale_mus_reader::container::parse(
        makeCodaBannerPools({1, 1, 1}).data(), makeCodaBannerPools({1, 1, 1}).size());
    expect(full.formatEpoch == FormatEpoch::CodaBanner && full.blocks.size() == 3,
        "Three non-empty pools were not all found");

    const auto bytes = makeCodaBannerPools({1, 0, 1});
    const auto parsed = finale_mus_reader::container::parse(bytes.data(), bytes.size());
    expect(parsed.formatEpoch == FormatEpoch::CodaBanner,
        "A document with an empty pool was not classified as the Coda-banner era");
    expect(parsed.blocks.size() == 3,
        "An empty second pool ended the walk instead of being reported as empty");
    expect(parsed.blocks[1].data.empty() && parsed.blocks[1].info.decodedSize == 0,
        "The empty pool was not reported as empty");
    expect(parsed.blocks[2].data.size() == 0x200,
        "The pool behind the empty one was not reached");
    expect(parsed.blocks[0].info.type == 1 && parsed.blocks[1].info.type == 2
            && parsed.blocks[2].info.type == 3,
        "An empty pool did not take its place in the type numbering");

    // A run of empty pools advances by the prologue each time rather than spinning, and the
    // text region still ends the chain behind them.
    const auto sparse = makeCodaBannerPools({0, 0, 0});
    const auto walked = finale_mus_reader::container::parse(sparse.data(), sparse.size());
    expect(walked.blocks.size() == 3, "A run of empty pools was not walked to its end");
}

TEST_CASE("Coda-banner byte order", "[reader]") { testCodaBannerByteOrder(); }
TEST_CASE("Coda-banner empty pool", "[reader]") { testCodaBannerEmptyPool(); }
TEST_CASE("Malformed input", "[reader]") { testMalformedInput(); }
TEST_CASE("Shape definitions", "[reader]") { testShapeDefinitions(); }
TEST_CASE("Smart shape custom lines", "[reader]") { testSmartShapeCustomLines(); }

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
