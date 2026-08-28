// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "mapping_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace mapping;

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
        ImportReport report(FormatEpoch::UncompressedLegacy);
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
        ImportReport report(FormatEpoch::UncompressedLegacy);
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
    SourceProfile unknown(FormatEpoch::ZlibLegacy);
    unknown.byteOrder = ByteOrder::LittleEndian;
    ImportReport report(FormatEpoch::UncompressedLegacy);
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
            finale_mus_reader::VersionBound{major} >= finale_mus_reader::versions::finale2012
                ? unicodeTail : byteTail,
            ByteOrder::LittleEndian);
        const auto document = makeLyricDocument();
        const auto reference = makeLyricDocument();
        auto profile = profileFor(major);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::LittleEndian;
        ImportReport report(FormatEpoch::UncompressedLegacy);
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
    ImportReport report(FormatEpoch::UncompressedLegacy);
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

TEST_CASE("Lyric word extension connection layouts", "[mapping]") { testLyricWordExtConnectionLayouts(); }
TEST_CASE("Lyric edge punctuation version gate", "[mapping]") { testLyricEdgePunctuationVersionGate(); }
TEST_CASE("Lyric punctuation tail encoding gate", "[mapping]") { testLyricPunctuationTailEncodingGate(); }
TEST_CASE("Lyric post-format assertions", "[mapping]") { testLyricPostFormatAssertions(); }

} // namespace
} // namespace finale_mus_reader_tests
