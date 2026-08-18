// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Texts pool recovery: the `^keyword(n) ... ^end` stream, the expression text the earlier
// eras embed in their own records, and the File Info strings in the header.
//
// Fixture cases assert exact strings rather than "something was recovered", because the whole
// value of this class is that the characters and the commands come out right. Synthetic cases
// cover the boundaries no fixture reaches: a stream that stops making sense, a keyword this
// reader does not know, and a command code it has no spelling for.

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "container/mus_container.h"
#include "import/support/legacy_mapping.h"
#include "import/texts.h"
#include "records/legacy_record_index.h"

#include "finale_mus_reader/reader.h"
#include "musx/musx.h"
#include "musx/factory/DocumentFactory.h"

#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#define FINALE_MUS_READER_TEXT_POOL_UNDEFINE_PUGIXML
#endif
#include "musx/xml/PugiXmlImpl.h"
#ifdef FINALE_MUS_READER_TEXT_POOL_UNDEFINE_PUGIXML
#undef MUSX_USE_PUGIXML
#undef FINALE_MUS_READER_TEXT_POOL_UNDEFINE_PUGIXML
#endif

namespace {

using finale_mus_reader::ByteOrder;
using finale_mus_reader::FormatEpoch;
using finale_mus_reader::ImportReport;
using finale_mus_reader::ImportResult;
using finale_mus_reader::Reader;
using finale_mus_reader::SourcePlatform;
using finale_mus_reader::SourceProfile;
using finale_mus_reader::ValueOrigin;
using finale_mus_reader::records::LegacyRecordIndex;
using TextPoolXmlDocument = musx::xml::pugi::Document;

void expectText(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

ImportResult readTextFixture(const char* relative)
{
    return Reader::read<TextPoolXmlDocument>(
        std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / relative);
}

template <typename Target>
std::string textOf(const ImportResult& result, musx::dom::Cmper number)
{
    const auto instance = result.document->getTexts()->get<Target>(number);
    return instance ? instance->text : std::string("<absent>");
}

template <typename Target>
std::size_t countOf(const ImportResult& result)
{
    return result.document->getTexts()->getArray<Target>().size();
}

const finale_mus_reader::FieldInfo& textField(const ImportResult& result, std::string_view target)
{
    const auto found = std::find_if(result.report.fields.begin(), result.report.fields.end(),
        [&](const finale_mus_reader::FieldInfo& info) { return info.target == target; });
    if (found == result.report.fields.end()) {
        throw std::runtime_error("no report entry for " + std::string(target));
    }
    return *found;
}

// -- synthetic stream support ------------------------------------------------------------

finale_mus_reader::container::ParsedContainer makeTextContainer(std::string_view stream)
{
    finale_mus_reader::container::ParsedContainer parsed;
    parsed.formatEpoch = FormatEpoch::UncompressedLegacy;
    parsed.byteOrder = ByteOrder::BigEndian;
    finale_mus_reader::container::DecodedBlock block;
    block.info.type = 0x0004;
    block.data.assign(stream.begin(), stream.end());
    block.info.decodedSize = block.data.size();
    parsed.blocks.push_back(std::move(block));
    return parsed;
}

/// @brief A document holding one named font, so a font command has something to resolve to.
musx::dom::DocumentPtr makeTextDocument()
{
    auto session = musx::factory::DocumentFactory::begin();
    auto document = session.getDocument();
    auto font = std::make_shared<musx::dom::others::FontDefinition>(
        document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, 4);
    font->name = "Times";
    font->charsetBank = musx::dom::others::FontDefinition::CharacterSetBank::MacOS;
    font->charsetVal = 0;
    document->getOthers()->add(musx::dom::others::FontDefinition::XmlNodeName, font);
    return document;
}

struct SyntheticImport
{
    musx::dom::DocumentPtr document;
    ImportReport report;
};

template <typename Target>
std::string textOf(const SyntheticImport& result, musx::dom::Cmper number)
{
    const auto instance = result.document->getTexts()->get<Target>(number);
    return instance ? instance->text : std::string("<absent>");
}

/// @brief Runs the text pool importer alone over a synthetic stream.
SyntheticImport importStream(std::string_view stream)
{
    SyntheticImport result;
    result.document = makeTextDocument();
    const auto reference = makeTextDocument();
    const auto index = LegacyRecordIndex::build(makeTextContainer(stream));
    SourceProfile profile;
    profile.epoch = FormatEpoch::UncompressedLegacy;
    profile.byteOrder = ByteOrder::BigEndian;
    profile.platform = SourcePlatform::MacOS;
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index, profile,
        std::span<const std::uint8_t>{}, result.document, reference, result.report, pending,
        construction};
    finale_mus_reader::texts::importTextPool(context);
    return result;
}

bool hasDiagnosticContaining(const ImportReport& report, std::string_view fragment)
{
    return std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
        [&](const finale_mus_reader::Diagnostic& diagnostic) {
            return diagnostic.message.find(fragment) != std::string::npos;
        });
}

// -- cases -------------------------------------------------------------------------------

// The uncompressed epoch spells its commands out, so this is where `^efx` becomes `^nfx` and
// where a carriage return becomes a line feed. Every string below is what Finale 27's own
// conversion of the same file writes, character for character.
void testUncompressedTextPool()
{
    using namespace musx::dom::texts;
    const auto result = readTextFixture("evidence/F97/F97-fileinfo-short.mus");
    expectText(result.report.formatEpoch == FormatEpoch::UncompressedLegacy,
        "Finale 97 fixture was not classified as uncompressed");

    expectText(textOf<BlockText>(result, 1) == "^font(Times)^size(12)^nfx(67)^page(0)",
        "An `^efx` run did not become one `^nfx` with the bold, italic and absolute bits");
    expectText(textOf<BlockText>(result, 3) == "^font(Times)^size(24)^nfx(65)^title()",
        "A block text carrying an insert was not recovered exactly");
    expectText(textOf<BlockText>(result, 5)
            == "^font(Times)^size(9)^nfx(64)Licensed by ASCAP\nOne Lincoln Plaza\n"
               "New York, NY  10023",
        "Legacy carriage returns did not become line feeds");
    expectText(textOf<BlockText>(result, 9)
            == "^font(Times)^size(9)^nfx(64)©^copyright() Great River Music\n"
               "All rights reserved.",
        "A non-ASCII character in a text font was not decoded through its code page");
    expectText(countOf<BlockText>(result) == 6,
        "Finale 97 fixture should carry exactly six block texts");

    // Nothing in this document is a lyric or a smart shape, so an importer that invented
    // objects would show up here.
    expectText(countOf<LyricsVerse>(result) == 0 && countOf<SmartShapeText>(result) == 0,
        "Objects were created for classes the source does not contain");

    expectText(textField(result, "texts.blockText[1].text").origin == ValueOrigin::LegacyMus,
        "A recovered block text was not reported as recovered");
}

// File Info lives in the header rather than in any pool, and the long variant runs past the
// customary 0x200 boundary, so the pair also tests that the read is bounded by the body
// offset the header itself states rather than by a constant.
void testFileInfoText()
{
    using musx::dom::texts::FileInfoText;
    const auto shortInfo = readTextFixture("evidence/F97/F97-fileinfo-short.mus");
    const auto longInfo = readTextFixture("evidence/F97/F97-fileinfo-long.mus");

    expectText(textOf<FileInfoText>(shortInfo, musx::dom::Cmper(FileInfoText::TextType::Title))
            == "File Info Short", "The header title was not recovered");
    expectText(textOf<FileInfoText>(shortInfo, musx::dom::Cmper(FileInfoText::TextType::Composer))
            == "Robert Patterson", "The header composer was not recovered");
    expectText(textOf<FileInfoText>(shortInfo, musx::dom::Cmper(FileInfoText::TextType::Copyright))
            == "2002", "The header copyright was not recovered");
    expectText(textOf<FileInfoText>(shortInfo, musx::dom::Cmper(FileInfoText::TextType::Description))
            == "This is the end of the description.",
        "The header description was not recovered");
    expectText(countOf<FileInfoText>(shortInfo) == 4,
        "Only the four located File Info fields should produce objects");

    const auto longDescription = textOf<FileInfoText>(
        longInfo, musx::dom::Cmper(FileInfoText::TextType::Description));
    expectText(longDescription.size() > 0x200,
        "The long description was truncated at the customary header size");
    expectText(longDescription.rfind("This is the end of the file info")
            == longDescription.size() - std::string_view("This is the end of the file info").size(),
        "The long description did not run to its own terminator");
    expectText(textOf<FileInfoText>(longInfo, musx::dom::Cmper(FileInfoText::TextType::Title))
            == "File Info Long", "The long variant's title was not recovered");

    // A document that filled nothing in should produce no objects at all, which is also what
    // Finale 27 writes for the same file.
    const auto blank = readTextFixture("evidence/F2000/F2000-multilayer.mus");
    expectText(countOf<FileInfoText>(blank) == 0,
        "Empty header slots should not create File Info objects");
}

// Expression text in this era is not in the text pool at all: it is a plain string inside the
// expression definition, with its font, size and style packed into the record header.
void testEmbeddedExpressionText()
{
    using musx::dom::texts::ExpressionText;
    const auto result = readTextFixture("evidence/F97/F97-fileinfo-short.mus");
    expectText(countOf<ExpressionText>(result) == 16,
        "Finale 97's default expression library should yield sixteen expression texts");

    // Font 0 is the default music font, so its bytes are glyph numbers and must survive as
    // themselves rather than being read as Mac Roman.
    expectText(textOf<ExpressionText>(result, 3) == "^font(Pmusic)^size(28)^nfx(0)Ä",
        "A symbol-font expression character was decoded as text");
    expectText(textOf<ExpressionText>(result, 5) == "^font(Pmusic)^size(28)^nfx(0)ffff",
        "A multi-character expression was not read across its incidences");
    // Font 16 is Patmm, an ordinary Mac-charset font in this file, so 0xb0 is Mac Roman's
    // infinity sign. This is the case that proves the two rules are actually different.
    expectText(textOf<ExpressionText>(result, 15)
            == "^font(Patmm)^size(12)^nfx(64)Tempo (∞=120)",
        "A text-font expression was not decoded through its own code page");
}

// The compressed epochs write their commands in the binary form, so this is where the code
// table and the digit encoding are exercised.
void testCompressedTextPool()
{
    using namespace musx::dom::texts;
    const auto dcl = readTextFixture("evidence/F2006/F2006-single-title.mus");
    expectText(dcl.report.formatEpoch == FormatEpoch::DclLegacy,
        "Finale 2006 fixture was not classified as DCL");
    expectText(textOf<BlockText>(dcl, 1) == "^font(Times)^size(12)^nfx(0)TEST",
        "Binary font, size and style commands were not spelled out");

    const auto mixed = readTextFixture("evidence/F2006/F2006-embedded-tiff.mus");
    expectText(countOf<SmartShapeText>(mixed) == 60,
        "Smart shape texts were not recovered from the DCL text pool");
    expectText(countOf<ExpressionText>(mixed) == 41,
        "Expression texts were not recovered from the DCL text pool");
    // One record that mixes both spellings: a named font command, then binary size and style.
    expectText(textOf<BlockText>(mixed, 9)
            == "^font(Times)^size(14)^nfx(0)Clarinet in B^size(20)^flat()",
        "A block text mixing a named font with later commands was not recovered exactly");
    expectText(textOf<BlockText>(mixed, 13) == "^font(Times)^size(24)^nfx(1)^title()",
        "A block text written entirely in binary commands was not recovered exactly");
    expectText(textOf<SmartShapeText>(mixed, 3) == "^font(Maestro)^size(24)^nfx(0)¡",
        "A symbol-font smart shape character was decoded as text");

    // Every binary command code this reader knows, one per record, against the document
    // written to establish them. The strings are what its ETF and its Finale 27 companion
    // both say, so a failure here means the table drifted from two independent witnesses.
    const auto inserts = readTextFixture("evidence/F2006/F2006-text-inserts.mus");
    struct ExpectedInsert
    {
        musx::dom::Cmper number;
        const char* text;
    };
    constexpr ExpectedInsert expectedBlocks[] = {
        {1, "^font(Times)^size(12)^nfx(0)^title()Title"},
        {2, "^font(Times)^size(12)^nfx(0)^composer() Composer"},
        {3, "^font(Times)^size(12)^nfx(0)^date(1) Current Date"},
        {4, "^font(Times)^size(12)^nfx(0)^fdate(0) File Date"},
        {5, "^font(Times)^size(12)^nfx(0)^page(0) Page Number"},
        // Eight digits holding a non-zero value: the case that shows the whole run is one
        // argument rather than two of four digits each.
        {6, "^font(Times)^size(12)^nfx(0)^page(275) Page Number With Offset"},
        {7, "^font(Times)^size(12)^nfx(0)^filename() File Name"},
        {8, "^font(Times)^size(12)^nfx(0)^fdate(2) File Date"},
        {9, "^font(Times)^size(12)^nfx(0)^cprsym() Copyright Symbol"},
        {10, "^font(Times)^size(12)^nfx(0)^copyright() Copyright Text"},
        {11, "^font(Times)^size(12)^nfx(0)^description() Description"},
        {12, "^font(Times)^size(12)^nfx(0)^sharp() Sharp"},
        {13, "^font(Times)^size(12)^nfx(0)^flat() Flat"},
        {14, "^font(Times)^size(12)^nfx(0)^natural() Natural"},
        {15, "^font(Times)^size(12)^nfx(0)^dbsharp() Double Sharp"},
        {16, "^font(Times)^size(12)^nfx(0)^dbflat() Double Flat"},
        {17, "^font(Times)^size(12)^nfx(0)^totpages() Total Pages"},
        {18, "^font(Times)^size(12)^nfx(0)^perftime(4) Performance Time"},
        // The three style commands are the only ones with an eight-digit argument besides
        // `^page`, and the only place a negative value appears. `0xfffffff3` is -13, not
        // 4294967283, and the same records carry the only observed nibble of 0xf: the
        // `^superscript(15)` below is spelled `01 01 01 01 01 01 01 10`.
        {19, "^font(Times)^size(12)^nfx(0)^baseline(-13)baseline -13 ^baseline(0)0"},
        {20, "^font(Times)^size(12)^nfx(0)^tracking(7) tracking 7 ^tracking(0)0"},
        {21, "^font(Times)^size(12)^nfx(0)^superscript(15)superscript 15 ^superscript(-13)-13"},
    };
    for (const auto& expected : expectedBlocks) {
        expectText(textOf<BlockText>(inserts, expected.number) == expected.text,
            "Insert block " + std::to_string(expected.number) + " read as \""
                + textOf<BlockText>(inserts, expected.number) + "\"");
    }
    constexpr ExpectedInsert expectedExpressions[] = {
        {1, "^font(Times)^size(12)^nfx(0)^value() Value Number"},
        {2, "^font(Times)^size(12)^nfx(0)^control() Controller Number"},
        {3, "^font(Times)^size(12)^nfx(0)^pass()Pass Number"},
    };
    for (const auto& expected : expectedExpressions) {
        expectText(textOf<ExpressionText>(inserts, expected.number) == expected.text,
            "Insert expression " + std::to_string(expected.number) + " read as \""
                + textOf<ExpressionText>(inserts, expected.number) + "\"");
    }
    // Nothing in that document should have been dropped for want of a spelling.
    expectText(std::none_of(inserts.report.diagnostics.begin(),
                   inserts.report.diagnostics.end(),
                   [](const finale_mus_reader::Diagnostic& diagnostic) {
                       return diagnostic.message.find("could not read")
                           != std::string::npos;
                   }),
        "A command code in the insert fixture was not read");

    const auto zlib = readTextFixture("evidence/F2007/F2007-lyric-hyphens.mus");
    expectText(zlib.report.formatEpoch == FormatEpoch::ZlibLegacy,
        "Finale 2007 fixture was not classified as zlib");
    expectText(textOf<LyricsVerse>(zlib, 1) == "^font(Times)^size(12)^nfx(0)a-test",
        "A lyric verse was not recovered from the zlib text pool");
    const auto verse = zlib.document->getTexts()->get<LyricsVerse>(1);
    expectText(verse && !verse->syllables.empty(),
        "musxdom's own resolver did not build syllable information for the recovered verse");

    // Finale 2012 stores the pool in UTF-8, so the same command codes arrive as their two-byte
    // encoding. Reading both spellings is what keeps one code table serving every era.
    const auto unicode = readTextFixture("evidence/F2012/F2012-baseline.mus");
    expectText(textOf<BlockText>(unicode, 1) == "^font(Times New Roman)^size(12)^nfx(0)Score",
        "A UTF-8 era binary command was not recognized");
}

// Finale 2008 is where the text pool takes over File Info, where four inserts are appended
// past the alphabetical run, and where the one-digit argument appears. It is also the only
// big-endian specimen of its era anywhere.
void testFinale2008Inserts()
{
    using namespace musx::dom::texts;
    const auto result = readTextFixture("evidence/F2008/F2008-BE-text-inserts.mus");
    expectText(result.report.formatEpoch == FormatEpoch::ZlibLegacy
            && result.report.byteOrder == finale_mus_reader::ByteOrder::BigEndian,
        "The Finale 2008 fixture was not classified as big-endian zlib");

    // All seven File Info types, from the pool rather than the header, keyed by musxdom's own
    // enumeration. The header offsets are empty in this file, so nothing here can have come
    // from them.
    const char* expectedInfo[] = {"File Info Title", "File Info Composer", "File Info Copyright",
        "File Info Description", "File Info Lyricist", "File Info Arranger",
        "File Info Subtitle"};
    for (musx::dom::Cmper type = 1; type <= 7; ++type) {
        expectText(textOf<FileInfoText>(result, type) == expectedInfo[type - 1],
            "File Info type " + std::to_string(type) + " read as \""
                + textOf<FileInfoText>(result, type) + "\"");
    }
    expectText(countOf<FileInfoText>(result) == 7,
        "The pool should have supplied exactly seven File Info objects");

    // The four appended inserts, each confirmed by the companion.
    expectText(textOf<BlockText>(result, 2) == "^font(Times)^size(12)^nfx(0)^subtitle() Subtitle"
            && textOf<BlockText>(result, 3) == "^font(Times)^size(12)^nfx(0)^arranger() Arranger"
            && textOf<BlockText>(result, 4) == "^font(Times)^size(12)^nfx(0)^lyricist() Lyricist"
            && textOf<BlockText>(result, 5) == "^font(Times)^size(12)^nfx(0)^partname() Partname",
        "An insert appended past the alphabetical run was not recovered");

    // The one-digit argument, and the one insert Finale 27 discards rather than converts.
    expectText(textOf<BlockText>(result, 6) == "^font(Times)^size(12)^nfx(0)^time(0) Time"
            && textOf<BlockText>(result, 7)
                == "^font(Times)^size(12)^nfx(0)^time(1) Time with seconds",
        "The one-digit time argument was not read");

    // A block with no style commands at all stays that way. Finale 27 supplies a font, size
    // and style the document never stated; recovering what the file says is the point here.
    expectText(textOf<BlockText>(result, 1) == "FULL SCORE",
        "A block text with no style commands did not survive unchanged");

    expectText(result.report.diagnostics.end()
            == std::find_if(result.report.diagnostics.begin(), result.report.diagnostics.end(),
                [](const finale_mus_reader::Diagnostic& diagnostic) {
                    return diagnostic.message.find("could not read") != std::string::npos
                        || diagnostic.message.find("does not import") != std::string::npos;
                }),
        "The Finale 2008 fixture still contains something this reader cannot read");
}

// Finale 2011, for the last four commands: the three that name a marking category's font and
// the automatic rehearsal mark. Finale 2009 introduced categories and Finale 2010 the
// rehearsal insert, so this is the earliest available release that can write all four.
void testFinale2011CategoryFonts()
{
    using namespace musx::dom::texts;
    const auto result = readTextFixture("evidence/F2011/F2011-text-inserts.mus");
    expectText(result.report.formatEpoch == FormatEpoch::ZlibLegacy,
        "The Finale 2011 fixture was not classified as zlib");

    // The three categorized font commands. Each stores a comparator, and each keeps its own
    // spelling because the marking category it names is the one thing `^fontid` cannot say.
    // The stored arguments are 9, 11 and 11, and the companion names the same two faces.
    expectText(textOf<ExpressionText>(result, 1)
                == "^fontTxt(Times New Roman)^size(14)^nfx(1)Category Text Font"
            && textOf<ExpressionText>(result, 2)
                == "^fontMus(EngraverTextT)^size(12)^nfx(0)Category Music Font"
            && textOf<ExpressionText>(result, 3)
                == "^fontNum(EngraverTextT)^size(12)^nfx(0)Category Number Font",
        "A categorized font command was not resolved to its font definition");

    // `^rehearsal` takes no argument, and this record is what shows it: the byte after the code
    // is the leading space of " Rehearsal", which is not a digit byte and so cannot be one.
    expectText(textOf<ExpressionText>(result, 4)
            == "^fontTxt(Times New Roman)^size(12)^nfx(0)^rehearsal() Rehearsal",
        "The rehearsal insert was not recovered without an argument");

    expectText(textOf<BlockText>(result, 1) == "^font(Times New Roman)^size(12)^nfx(0)Score",
        "The Finale 2011 block text was not recovered");

    expectText(result.report.diagnostics.end()
            == std::find_if(result.report.diagnostics.begin(), result.report.diagnostics.end(),
                [](const finale_mus_reader::Diagnostic& diagnostic) {
                    return diagnostic.message.find("could not read") != std::string::npos;
                }),
        "The Finale 2011 fixture still contains a command this reader cannot read");
}

// The Coda-banner epoch is uncovered on purpose. This asserts the absence so that widening a
// gate without evidence breaks a test rather than quietly producing objects.
void testCodaBannerEpochStaysUncovered()
{
    using namespace musx::dom::texts;
    for (const char* fixture :
        {"evidence/F100/F100-baseline.mus", "evidence/F263/F263-baseline.mus"}) {
        const auto result = readTextFixture(fixture);
        expectText(result.report.formatEpoch == FormatEpoch::CodaBanner,
            std::string(fixture) + " was not classified as a Coda-banner file");
        expectText(countOf<BlockText>(result) == 0 && countOf<ExpressionText>(result) == 0
                && countOf<FileInfoText>(result) == 0,
            std::string(fixture)
                + " produced texts, but this epoch's text framing is not yet decoded");
    }
}

void testSyntheticStreamBoundaries()
{
    using namespace musx::dom::texts;

    // An escaped caret is content, and stays escaped so that musxdom reads it back as one.
    const auto escaped = importStream("^block(1)^font(Times)a^^b^end");
    expectText(textOf<BlockText>(escaped, 1) == "^font(Times)a^^b",
        "An escaped caret was not preserved");

    // A font command whose comparator the document does not define falls back to `^fontid`,
    // which is the one spelling that needs no definition to exist. The synthetic document
    // defines comparator 4 alone, so `^\x85` naming 4 is written out by name and the same
    // command naming 7 is not. The categorized commands fall back the same way, and that
    // costs the marking category they name -- the one thing `^fontid` cannot carry.
    // Digits are stored one greater than their value, so `\x05` is comparator 4 and `\x08` is 7.
    const auto fontStream = [](char code, char comparator) {
        return std::string("^block(1)^") + code + std::string("\x01\x01\x01", 3) + comparator
            + "a^end";
    };
    const auto namedFont = importStream(fontStream('\x85', '\x05'));
    expectText(textOf<BlockText>(namedFont, 1) == "^font(Times)a",
        "A defined comparator was not written out under the font's name");
    const auto unnamedFont = importStream(fontStream('\x85', '\x08'));
    expectText(textOf<BlockText>(unnamedFont, 1) == "^fontid(7)a",
        "An undefined comparator did not fall back to `^fontid`");
    const auto unnamedCategory = importStream(fontStream('\xa5', '\x08'));
    expectText(textOf<BlockText>(unnamedCategory, 1) == "^fontid(7)a",
        "An undefined comparator kept a categorized spelling it cannot name a font for");
    const auto namedCategory = importStream(fontStream('\xa5', '\x05'));
    expectText(textOf<BlockText>(namedCategory, 1) == "^fontMus(Times)a",
        "A defined comparator lost the marking category its command names");

    // A keyword this reader does not import is named rather than silently skipped, which is
    // how an unobserved spelling would be found.
    const auto unknownKeyword = importStream("^block(1)x^end^bookmark(2)y^end");
    expectText(unknownKeyword.document->getTexts()->getArray<BlockText>().size() == 1,
        "A recognized record before an unrecognized one was lost");
    expectText(hasDiagnosticContaining(unknownKeyword.report, "bookmark"),
        "An unrecognized text keyword was not reported by name");

    // A binary command with no known spelling is dropped from the text and reported, because
    // guessing a name would produce a command that resolves to the wrong thing.
    const auto unknownCode = importStream(
        std::string("^block(1)^\xff\x01\x01\x01\x02here^end", 27));
    expectText(textOf<BlockText>(unknownCode, 1) == "here",
        "An unknown binary command did not leave the surrounding text intact");
    expectText(hasDiagnosticContaining(unknownCode.report, "0xff"),
        "An unknown binary command code was not reported");

    // A literal byte that falls in the digit range must survive: the argument width comes
    // from the table, so a four-digit argument takes four bytes and no more. Text set in a
    // symbol font is glyph numbers, which is how a byte this low reaches literal text at all.
    const std::string lowByteStream = std::string("^block(1)^\x84", 11)
        + std::string("\x01\x01\x01\x01", 4)   // the whole four-digit argument
        + std::string("\x10", 1)                // a literal byte that looks like a digit
        + "x^end";
    const auto lowByteLiteral = importStream(lowByteStream);
    expectText(textOf<BlockText>(lowByteLiteral, 1) == std::string("^nfx(0)\x10x", 9),
        "A literal byte in the digit range was eaten as a fifth argument digit, giving \""
            + textOf<BlockText>(lowByteLiteral, 1) + "\"");

    // Bytes that are not a chunk stop the walk instead of being scanned past.
    const auto malformed = importStream("^block(1)good^end\x01\x02\x03\x04");
    expectText(malformed.document->getTexts()->getArray<BlockText>().size() == 1,
        "The record before the malformed bytes was not kept");
    expectText(hasDiagnosticContaining(malformed.report, "stopped making sense"),
        "A malformed text pool was not reported");

    // A record whose terminator never arrives is the truncation case, and must not be
    // half-imported.
    const auto truncated = importStream("^block(1)unfinished");
    expectText(truncated.document->getTexts()->getArray<BlockText>().empty(),
        "An unterminated record was imported anyway");
    expectText(hasDiagnosticContaining(truncated.report, "stopped making sense"),
        "An unterminated text pool was not reported");

    // An empty pool is not an error and produces nothing.
    const auto empty = importStream("");
    expectText(empty.document->getTexts()->getArray<BlockText>().empty()
            && empty.report.diagnostics.empty(),
        "An empty text pool was not silent");
}

} // namespace

TEST_CASE("Uncompressed text pool", "[texts]") { testUncompressedTextPool(); }
TEST_CASE("File info text", "[texts]") { testFileInfoText(); }
TEST_CASE("Embedded expression text", "[texts]") { testEmbeddedExpressionText(); }
TEST_CASE("Compressed text pools", "[texts]") { testCompressedTextPool(); }
TEST_CASE("Finale 2008 inserts and pooled file info", "[texts]")
{
    testFinale2008Inserts();
}
TEST_CASE("Finale 2011 category fonts and rehearsal", "[texts]")
{
    testFinale2011CategoryFonts();
}
TEST_CASE("Coda banner texts stay uncovered", "[texts]")
{
    testCodaBannerEpochStaysUncovered();
}
TEST_CASE("Text pool stream boundaries", "[texts]") { testSyntheticStreamBoundaries(); }
