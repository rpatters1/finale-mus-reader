// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

#include "coverage/classification_rules.h"
#include "coverage/comparison.h"
#include "coverage/comparison_text.h"
#include "coverage/registry.h"
#include "support/finale_version.h"

namespace finale_mus_reader_tests
{
namespace
{

using namespace classes;
using Staff = musx::dom::others::Staff;

std::optional<finale_mus_reader::coverage::DifferenceClassification>
classifyStaffDifference(const finale_mus_reader::coverage::DifferenceContext &context)
{
    const auto classify = finale_mus_reader::coverage::differenceClassifier("staff");
    return classify ? classify(context) : std::nullopt;
}

constexpr std::size_t staffFieldManifestSize = 101;
constexpr int evpusPerSpace = static_cast<int>(musx::dom::EVPU_PER_SPACE);
constexpr std::string_view staffAlternateNotationFields[] = {
    "altNotation",
    "altLayer",
    "altHideArtics",
    "altHideLyrics",
    "altHideSmartShapes",
    "altRhythmStemsUp",
    "altSlashDots",
    "altHideOtherNotes",
    "altHideOtherArtics",
    "altHideExpressions",
    "altHideOtherLyrics",
    "altHideOtherSmartShapes",
    "altHideOtherExpressions",
};

musx::dom::DocumentPtr emptyStaffDocument(musx::dom::Cmper musicFontId = 0)
{
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto fontOptions = std::make_shared<musx::dom::options::FontOptions>(document);
    auto musicFont = std::make_shared<musx::dom::FontInfo>(document);
    musicFont->fontId = musicFontId;
    fontOptions->fontOptions.emplace(musx::dom::options::FontOptions::FontType::Music,
                                     std::move(musicFont));
    auto noteheadFont = std::make_shared<musx::dom::FontInfo>(document);
    noteheadFont->fontSize = 24;
    fontOptions->fontOptions.emplace(musx::dom::options::FontOptions::FontType::Noteheads,
                                     std::move(noteheadFont));
    document->getOptions()->add(musx::dom::options::FontOptions::XmlNodeName,
                                std::move(fontOptions));
    return document;
}

ImportReport staffImport(const finale_mus_reader::container::ParsedContainer &parsed,
                         const SourceProfile &profile, const musx::dom::DocumentPtr &document)
{
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto referenceDocument = referenceSession.getDocument();
    auto referenceStaff = std::make_shared<Staff>(referenceDocument, musx::dom::SCORE_PARTID,
                                                  musx::dom::EnigmaBase::ShareMode::All, 1);
    referenceStaff->staffLines = 5;
    referenceStaff->lineSpace = evpusPerSpace;
    referenceStaff->dwRestOffset = -4;
    referenceStaff->wRestOffset = -4;
    referenceStaff->hRestOffset = -4;
    referenceStaff->otherRestOffset = -4;
    referenceStaff->stemReversal = -4;
    referenceStaff->botRepeatDotOff = -5;
    referenceStaff->topRepeatDotOff = -3;
    referenceDocument->getOthers()->add(Staff::XmlNodeName, referenceStaff);
    auto referenceFontOptions =
        std::make_shared<musx::dom::options::FontOptions>(referenceDocument);
    auto referenceNoteheadFont = std::make_shared<musx::dom::FontInfo>(referenceDocument);
    referenceNoteheadFont->fontSize = 24;
    referenceFontOptions->fontOptions.emplace(musx::dom::options::FontOptions::FontType::Noteheads,
                                              std::move(referenceNoteheadFont));
    referenceDocument->getOptions()->add(musx::dom::options::FontOptions::XmlNodeName,
                                         std::move(referenceFontOptions));
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index,     profile, noSource, document,
                                                   reference, report,  pending,  construction};
    finale_mus_reader::others::importStaff(context);
    finale_mus_reader::runDeferredChecks(pending);
    return report;
}

const FieldInfo &staffField(const ImportReport &report, std::string_view member,
                            musx::dom::Cmper cmper = 7)
{
    const auto *value = report.findField(
        finale_mus_reader::instanceKey<Staff>(musx::dom::SCORE_PARTID, cmper), member);
    expect(value != nullptr, "Missing Staff report field " + std::string(member));
    return *value;
}

TEST_CASE("The Finale 2000 Staff base layout recovers its complete raw field "
          "surface")
{
    for (const auto epoch : {FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy})
    {
        for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian})
        {
            const auto parsed = makeContainer(
                {{7, "IS", {-48, 0x0b17, static_cast<std::int16_t>(0xff12), 6, 0x1803, 0x69bd}},
                 {7,
                  "IS",
                  {0x0302, 1, 4, 48, static_cast<std::int16_t>(0xcfbd),
                   static_cast<std::int16_t>(0xf5fb)}},
                 {7,
                  "IS",
                  {static_cast<std::int16_t>(0xfafb), static_cast<std::int16_t>(0xfdfc), -4, 9, 10,
                   0}}},
                epoch, byteOrder);
            const auto document = emptyStaffDocument();
            auto profile = SourceProfile(epoch);
            if (epoch == FormatEpoch::UncompressedLegacy)
            {
                SourceVersion version;
                version.major = finale_mus_reader::versions::finale2000.major;
                profile.version = version;
            }
            profile.byteOrder = byteOrder;
            const auto report = staffImport(parsed, profile, document);

            const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
            REQUIRE(staff);
            CHECK(staff->notationStyle == Staff::NotationStyle::Percussion);
            CHECK(staff->altNotation == Staff::AlternateNotation::Rhythmic);
            CHECK(staff->altLayer == 1);
            CHECK_FALSE(staff->altHideArtics);
            CHECK_FALSE(staff->altHideLyrics);
            CHECK_FALSE(staff->altHideSmartShapes);
            CHECK(staff->altRhythmStemsUp);
            CHECK(staff->altSlashDots);
            CHECK_FALSE(staff->altHideOtherNotes);
            CHECK_FALSE(staff->altHideOtherArtics);
            CHECK_FALSE(staff->altHideExpressions);
            CHECK(staff->useNoteShapes);
            CHECK(staff->useNoteFont);
            CHECK_FALSE(staff->hasStyles);
            CHECK(staff->hideFretboards);
            CHECK(staff->hideLyrics);
            CHECK(staff->showNameInParts);
            CHECK(staff->showNoteColors);
            REQUIRE(staff->noteFont);
            CHECK(staff->noteFont->fontId == 6);
            CHECK(staff->noteFont->fontSize == 24);
            CHECK(staff->noteFont->bold);
            CHECK(staff->noteFont->italic);
            CHECK(staff->defaultClef == 2);
            CHECK(staff->transposedClef == 3);
            CHECK_FALSE(staff->staffLines);
            REQUIRE(staff->customStaff);
            CHECK(*staff->customStaff == std::vector<int>{13});
            CHECK(staff->botBarlineOffset == -2 * evpusPerSpace);
            CHECK(staff->capoPos == 23);
            CHECK(staff->lowestFret == 11);
            CHECK(staff->topBarlineOffset == 2 * evpusPerSpace);
            REQUIRE(staff->transposition);
            REQUIRE(staff->transposition->keysig);
            CHECK(staff->transposition->setToClef);
            CHECK(staff->transposition->noSimplifyKey);
            CHECK(staff->transposition->keysig->interval == -2);
            CHECK(staff->transposition->keysig->adjust == -3);
            CHECK((staff->floatKeys && staff->floatTime && staff->blineBreak));
            CHECK((staff->hideMeasNums && staff->hideNameInScore && staff->noKey));
            CHECK(staff->hideChords);
            CHECK_FALSE(staff->redisplayLayerAccis);
            CHECK(staff->hideTimeSigs);
            CHECK(staff->hideTimeSigsInParts == staff->hideTimeSigs);
            CHECK_FALSE(staff->hideKeySigsShowAccis);
            CHECK(staff->dwRestOffset == -5);
            CHECK(staff->wRestOffset == -6);
            CHECK(staff->hRestOffset == -4);
            CHECK(staff->otherRestOffset == -3);
            CHECK(staff->stemReversal == -4);
            CHECK(staff->fullNameTextId == 9);
            CHECK(staff->abbrvNameTextId == 10);
            CHECK(staff->lineSpace == evpusPerSpace);
            CHECK(staff->botRepeatDotOff == -5);
            CHECK(staff->topRepeatDotOff == -3);
            CHECK(staffField(report, "notationStyle").origin == ValueOrigin::LegacyMus);
            CHECK(staffField(report, "capoPos").origin == ValueOrigin::LegacyMus);
            CHECK(staffField(report, "lowestFret").origin == ValueOrigin::LegacyMus);
            CHECK(staffField(report, "hideLyrics").origin == ValueOrigin::LegacyMus);
            CHECK(staffField(report, "altHideOtherLyrics").origin == ValueOrigin::LegacyMus);
            CHECK(staffField(report, "lineSpace").origin == ValueOrigin::Finale27Default);
            CHECK(staffField(report, "botRepeatDotOff").origin == ValueOrigin::LegacyBehavior);
            CHECK(staffField(report, "topRepeatDotOff").origin == ValueOrigin::LegacyBehavior);
            CHECK(staffField(report, "redisplayLayerAccis").origin == ValueOrigin::LegacyBehavior);
            CHECK(staffField(report, "hideTimeSigsInParts").origin == ValueOrigin::LegacyBehavior);
            CHECK(staffField(report, "hideKeySigsShowAccis").origin == ValueOrigin::LegacyBehavior);
            CHECK(reportedFieldCount(report) == staffFieldManifestSize);
        }
    }
}

TEST_CASE("Finale 2000 custom Staff masks preserve line order across both words")
{
    for (const auto &[top, bottom, expected] : {
             std::tuple<std::int16_t, std::int16_t, std::vector<int>>{1, 0x000f, {11, 12, 13, 14}},
             std::tuple<std::int16_t, std::int16_t, std::vector<int>>{1, 0x001e, {12, 13, 14, 15}},
             std::tuple<std::int16_t, std::int16_t, std::vector<int>>{
                 static_cast<std::int16_t>(0x8021), 0, {0, 10}},
             std::tuple<std::int16_t, std::int16_t, std::vector<int>>{1, 0, {}},
         })
    {
        const auto parsed = makeContainer(
            {
                {7, "IS", {0, 0, 0, 0, 0, 0}},
                {7, "IS", {0, top, bottom, 0, 0, 0}},
                {7, "IS", {0, 0, 0, 0, 0, 0}},
            },
            FormatEpoch::UncompressedLegacy);
        const auto document = emptyStaffDocument();
        auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
        profile.version = SourceVersion{.major = finale_mus_reader::versions::finale2000.major};
        profile.byteOrder = ByteOrder::BigEndian;
        const auto report = staffImport(parsed, profile, document);

        const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
        REQUIRE(staff);
        REQUIRE(staff->customStaff);
        CHECK(*staff->customStaff == expected);
        if (expected.empty())
        {
            CHECK(staff->botRepeatDotOff == -5);
            CHECK(staff->topRepeatDotOff == -3);
            CHECK(staffField(report, "botRepeatDotOff").origin == ValueOrigin::LegacyBehavior);
            CHECK(staffField(report, "topRepeatDotOff").origin == ValueOrigin::LegacyBehavior);
        }
    }

    struct CustomStaffCase
    {
        const char *path;
        std::vector<int> lines;
        int bottomRepeatDot;
        int topRepeatDot;
    };
    for (const auto &expected : {
             CustomStaffCase{"evidence/F2000/F2000-lines-upper4.mus", {11, 12, 13, 14}, -3, -1},
             CustomStaffCase{"evidence/F2000/F2000-lines-lower4.mus", {12, 13, 14, 15}, -5, -3},
             CustomStaffCase{"evidence/F2000/F2000-lines-cl10.mus", {10}, 1, 3},
             CustomStaffCase{"evidence/F2000/F2000-lines-cl0.mus", {0}, 21, 23},
         })
    {
        const auto result = readFixture(expected.path);
        const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
        REQUIRE(staff);
        REQUIRE(staff->customStaff);
        CHECK(*staff->customStaff == expected.lines);
        CHECK(staff->botRepeatDotOff == expected.bottomRepeatDot);
        CHECK(staff->topRepeatDotOff == expected.topRepeatDot);
        const auto *bottomDot =
            result.report.findField<Staff>("botRepeatDotOff", musx::dom::SCORE_PARTID, 1);
        const auto *topDot =
            result.report.findField<Staff>("topRepeatDotOff", musx::dom::SCORE_PARTID, 1);
        REQUIRE(bottomDot);
        REQUIRE(topDot);
        CHECK(bottomDot->origin == ValueOrigin::LegacyBehavior);
        CHECK(topDot->origin == ValueOrigin::LegacyBehavior);
    }
}

TEST_CASE("Legacy Staff alternate notation accepts the terminal Blank value")
{
    const auto parsed = makeContainer(
        {
            {7, "IS", {0, 0, 6, 0, 0, 0}},
            {7, "IS", {0, 0, 5, 0, 0, 0}},
            {7, "IS", {0, 0, 0, 0, 0, 0}},
        },
        FormatEpoch::UncompressedLegacy);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
    profile.version = SourceVersion{.major = finale_mus_reader::versions::finale2000.major};
    profile.byteOrder = ByteOrder::BigEndian;
    staffImport(parsed, profile, document);

    const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(staff);
    CHECK(staff->altNotation == Staff::AlternateNotation::Blank);
}

TEST_CASE("Finale 2000 through 2008 expand the note-attached-items setting")
{
    const auto baseline = readFixture("evidence/F2008/F2008-empty.mus");
    const auto baselineStaff =
        baseline.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(baselineStaff);
    CHECK_FALSE(baselineStaff->altHideExpressions);

    for (const auto *path : {
             "evidence/F2000/F2000-staff-unshownoteitems.mus",
             "evidence/F2006/F2006-staff-nonoteitems.mus",
             "evidence/F2008/F2008-staff-nohidenoteitems.mus",
         })
    {
        const auto result = readFixture(path);
        const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
        REQUIRE(staff);
        CHECK(staff->altNotation == Staff::AlternateNotation::Normal);
        CHECK(staff->altHideArtics);
        CHECK(staff->altHideLyrics);
        CHECK(staff->altHideSmartShapes);
        CHECK(staff->altHideExpressions);
        CHECK(staff->hideFretboards);
        CHECK(staff->hideChords);
        CHECK_FALSE(staff->hasStyles);

        for (const auto *member : {"altHideArtics", "altHideLyrics", "altHideSmartShapes",
                                   "altHideExpressions", "hideFretboards", "hideChords"})
        {
            const auto *field = result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1);
            REQUIRE(field);
            CHECK(field->origin == ValueOrigin::LegacyMus);
        }
        const auto *hasStyles =
            result.report.findField<Staff>("hasStyles", musx::dom::SCORE_PARTID, 1);
        REQUIRE(hasStyles);
        CHECK(hasStyles->origin == ValueOrigin::Unmapped);
    }
}

TEST_CASE("Short post-Finale-2000 Staff layouts expand the aggregate "
          "other-layer settings")
{
    struct Expected
    {
        const char *path;
        bool hideOtherNotes;
        bool hideOtherItems;
    };
    for (const auto expected : {
             Expected{"evidence/F2000/F2000-staff-hideothernotes.mus", true, false},
             Expected{"evidence/F2000/F2000-staff-hideotheritems.mus", false, true},
         })
    {
        const auto result = readFixture(expected.path);
        const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
        REQUIRE(staff);

        CHECK_FALSE(staff->altHideArtics);
        CHECK_FALSE(staff->altHideLyrics);
        CHECK_FALSE(staff->altHideSmartShapes);
        CHECK_FALSE(staff->altHideExpressions);
        CHECK(staff->altHideOtherNotes == expected.hideOtherNotes);
        CHECK(staff->altHideOtherArtics == expected.hideOtherItems);
        CHECK(staff->altHideOtherLyrics == expected.hideOtherItems);
        CHECK(staff->altHideOtherSmartShapes == expected.hideOtherItems);
        CHECK(staff->altHideOtherExpressions == expected.hideOtherItems);

        for (const auto member : staffAlternateNotationFields)
        {
            const auto *field = result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1);
            REQUIRE(field);
            CHECK(field->origin == ValueOrigin::LegacyMus);
        }
    }
}

TEST_CASE("Pre-Finale-2000 Staff alternate notation is legacy "
          "behavior")
{
    for (const auto *path : {
             "evidence/F98/F98-baseline.mus",
             "evidence/F98/F98-altnotation-full.mus",
             "evidence/F98/F98-altnotation-partial.mus",
         })
    {
        const auto result = readFixture(path);
        const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
        REQUIRE(staff);

        CHECK(staff->altNotation == Staff::AlternateNotation::Normal);
        CHECK(staff->altLayer == 0);
        CHECK_FALSE(staff->altHideArtics);
        CHECK_FALSE(staff->altHideLyrics);
        CHECK_FALSE(staff->altHideSmartShapes);
        CHECK_FALSE(staff->altRhythmStemsUp);
        CHECK(staff->altSlashDots);
        CHECK_FALSE(staff->altHideOtherNotes);
        CHECK_FALSE(staff->altHideOtherArtics);
        CHECK_FALSE(staff->altHideExpressions);
        CHECK_FALSE(staff->altHideOtherLyrics);
        CHECK_FALSE(staff->altHideOtherSmartShapes);
        CHECK_FALSE(staff->altHideOtherExpressions);

        for (const auto member : staffAlternateNotationFields)
        {
            const auto *field = result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1);
            REQUIRE(field);
            CHECK(field->origin == ValueOrigin::LegacyBehavior);
        }
    }
}

TEST_CASE("Finale 3.7.2 uses the later Staff notehead-font representation")
{
    const auto result = readFixture("evidence/F372/F372-notehead-font.mus");
    const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(staff);
    REQUIRE(staff->noteFont);
    CHECK(staff->useNoteFont);
    CHECK(staff->noteFont->fontId == 6);
    CHECK(staff->noteFont->fontSize == 23);
    CHECK_FALSE(staff->noteFont->bold);
    CHECK_FALSE(staff->noteFont->italic);
    CHECK_FALSE(staff->noteFont->underline);

    for (const auto *member : {
             "useNoteFont",
             "noteFont.fontId",
             "noteFont.fontSize",
         })
    {
        const auto *field = result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1);
        REQUIRE(field);
        CHECK(field->origin == ValueOrigin::LegacyMus);
    }
}

TEST_CASE("The zlib Staff class retains the base words and its established "
          "extension")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian})
    {
        std::vector<std::int16_t> words{0,
                                        0,
                                        0x0700,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        5,
                                        0,
                                        0,
                                        0,
                                        static_cast<std::int16_t>(0xfcfc),
                                        static_cast<std::int16_t>(0xfcfc),
                                        -4,
                                        0,
                                        0,
                                        static_cast<std::int16_t>(0xfdfb)};
        const auto appendLong = [&](std::int32_t value)
        {
            const auto bits = static_cast<std::uint32_t>(value);
            const auto high = static_cast<std::int16_t>(bits >> 16U);
            const auto low = static_cast<std::int16_t>(bits & 0xffffU);
            if (byteOrder == ByteOrder::BigEndian)
            {
                words.insert(words.end(), {high, low});
            }
            else
            {
                words.insert(words.end(), {low, high});
            }
        };
        if (byteOrder == ByteOrder::BigEndian)
        {
            words.insert(words.end(), {0, 1536, -1, -1024});
        }
        else
        {
            words.insert(words.end(), {1536, 0, -1024, -1});
        }
        words.insert(words.end(), {static_cast<std::int16_t>(0x9bb4), 23});
        appendLong(-64);
        appendLong(128);
        appendLong(-192);
        appendLong(256);
        appendLong(-320);
        appendLong(384);
        words.push_back(0x0002);
        const auto parsed = makeClassContainer(0x00e7, words, byteOrder, 7);
        const auto document = emptyStaffDocument();
        auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
        profile.byteOrder = byteOrder;
        const auto report = staffImport(parsed, profile, document);

        const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
        REQUIRE(staff);
        REQUIRE(staff->staffLines);
        CHECK(*staff->staffLines == 5);
        CHECK(staff->lineSpace == evpusPerSpace);
        CHECK(staff->botRepeatDotOff == -5);
        CHECK(staff->topRepeatDotOff == -3);
        CHECK(staff->vertTabNumOff == -1024);
        CHECK(staff->showTabClefAllSys);
        CHECK(staff->hideRests);
        CHECK_FALSE(staff->hideTies);
        CHECK(staff->hideDots);
        CHECK_FALSE(staff->hideStems);
        CHECK(staff->stemDirection == musx::dom::StemDirection::AlwaysDown);
        CHECK(staff->stemStartFromStaff);
        CHECK(staff->stemsFixedEnd);
        CHECK(staff->useTabLetters);
        CHECK(staff->hideBeams);
        CHECK(staff->breakTabLinesAtNotes);
        CHECK(staff->stemsFixedStart);
        CHECK(staff->hideTuplets);
        CHECK(staff->hideMode == Staff::HideMode::ScoreParts);
        CHECK(staff->fretInstId == 23);
        CHECK(staff->horzStemOffUp == -64);
        CHECK(staff->horzStemOffDown == 128);
        CHECK(staff->vertStemStartOffUp == -192);
        CHECK(staff->vertStemStartOffDown == 256);
        CHECK(staff->vertStemEndOffUp == -320);
        CHECK(staff->vertStemEndOffDown == 384);
        CHECK(staff->altHideOtherLyrics);
        CHECK_FALSE(staff->altHideOtherSmartShapes);
        CHECK(staff->altHideOtherExpressions);
        CHECK(staffField(report, "lineSpace").origin == ValueOrigin::LegacyMus);
        CHECK(staffField(report, "lineSpace").rawValue == 1536);
        CHECK(staffField(report, "vertTabNumOff").origin == ValueOrigin::LegacyMus);
        CHECK(staffField(report, "vertTabNumOff").rawValue == -1024);
        CHECK(staffField(report, "stemDirection").origin == ValueOrigin::LegacyMus);
        CHECK(staffField(report, "fretInstId").rawValue == 23);
        CHECK(staffField(report, "horzStemOffUp").rawValue == -64);
        CHECK(staffField(report, "altHideOtherLyrics").origin == ValueOrigin::LegacyMus);
        CHECK(reportedFieldCount(report) == staffFieldManifestSize);
    }
}

TEST_CASE("Controlled Finale 2005 tablature settings recover from Staff")
{
    struct TabCase
    {
        const char *fixture;
        int capoPos;
        int lowestFret;
        bool showClefs;
        bool useLetters;
        bool breakLines;
        bool hideTuplets;
    };
    constexpr TabCase cases[]{
        {"evidence/F2005/F2005-tab-breaklines.mus", 23, 11, false, false, true, false},
        {"evidence/F2005/F2005-tab-notuplets.mus", 5, 13, false, false, false, true},
        {"evidence/F2005/F2005-tab-showclefs.mus", 23, 11, true, false, false, false},
        {"evidence/F2005/F2005-tab-showletters.mus", 23, 11, false, true, false, false},
    };

    for (const auto &expected : cases)
    {
        const auto result = readFixture(expected.fixture);
        const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
        REQUIRE(staff);
        CHECK(staff->notationStyle == Staff::NotationStyle::Tablature);
        CHECK(staff->capoPos == expected.capoPos);
        CHECK(staff->lowestFret == expected.lowestFret);
        CHECK(staff->showTabClefAllSys == expected.showClefs);
        CHECK(staff->useTabLetters == expected.useLetters);
        CHECK(staff->breakTabLinesAtNotes == expected.breakLines);
        CHECK(staff->hideTuplets == expected.hideTuplets);
        CHECK(staff->fretInstId == 2);
        CHECK(staff->vertTabNumOff == -1088);

        const auto *capoSource =
            result.report.findField<Staff>("capoPos", musx::dom::SCORE_PARTID, 1);
        const auto *lowestFretSource =
            result.report.findField<Staff>("lowestFret", musx::dom::SCORE_PARTID, 1);
        REQUIRE(capoSource);
        REQUIRE(lowestFretSource);
        CHECK(capoSource->origin == ValueOrigin::LegacyMus);
        CHECK(lowestFretSource->origin == ValueOrigin::LegacyMus);
    }
}

TEST_CASE("Staff hiding changes representation in Finale 2011")
{
    std::vector<std::int16_t> words(23, 0);
    words[11] = 0x0004;
    words[22] = static_cast<std::int16_t>(0x8000);
    const auto parsed = makeClassContainer(0x00e7, words, ByteOrder::BigEndian, 7);

    const auto importAt = [&](finale_mus_reader::VersionBound version)
    {
        const auto document = emptyStaffDocument();
        auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
        profile.version =
            SourceVersion{.major = version.major, .minor = version.minor, .maint = version.maint};
        profile.byteOrder = ByteOrder::BigEndian;
        auto report = staffImport(parsed, profile, document);
        const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
        REQUIRE(staff);
        return std::pair{staff, std::move(report)};
    };

    const auto [finale2010Staff, finale2010Report] =
        importAt(finale_mus_reader::versions::finale2010);
    CHECK(finale2010Staff->hideMode == Staff::HideMode::Cutaway);
    CHECK(staffField(finale2010Report, "hideMode").origin == ValueOrigin::LegacyMus);

    const auto [finale2011Staff, finale2011Report] =
        importAt(finale_mus_reader::versions::finale2011);
    CHECK(finale2011Staff->hideMode == Staff::HideMode::ScoreParts);
    CHECK(staffField(finale2011Report, "hideMode").origin == ValueOrigin::LegacyMus);
}

TEST_CASE("The six-word Staff layout survives the uncompressed container "
          "transition")
{
    const auto parsed = makeContainer(
        {
            {7, "IS", {4, 0, 0, 0, 0, 0x0010}},
            {7, "IA", {0, 0, 3, 0, 0, 0x0040}},
        },
        FormatEpoch::UncompressedLegacy);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
    profile.version = SourceVersion{.major = 3, .minor = 0};
    profile.byteOrder = ByteOrder::BigEndian;
    const auto report = staffImport(parsed, profile, document);

    const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(staff);
    CHECK(staff->defaultClef == 4);
    CHECK(staff->hideTimeSigs);
    CHECK(staff->staffLines == 3);
    CHECK(staffField(report, "defaultClef").origin == ValueOrigin::LegacyMus);
    CHECK(staffField(report, "staffLines").origin == ValueOrigin::LegacyMus);
    CHECK(staffField(report, "lineSpace").origin == ValueOrigin::Finale27Default);
    CHECK(reportedFieldCount(report) == staffFieldManifestSize);
}

TEST_CASE("The distinct Coda Staff layout constructs a valid explicitly "
          "partial Staff")
{
    const auto parsed = makeContainer({{7, "IS", {0, 0, 4, 1024, 0, 0}}}, FormatEpoch::CodaBanner);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto report = staffImport(parsed, profile, document);

    const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(staff);
    CHECK(staff->staffLines == 5);
    REQUIRE(staff->noteFont);
    CHECK(staff->noteFont->fontSize == 24);
    CHECK(staff->lineSpace == evpusPerSpace);
    CHECK(staff->dwRestOffset == -4);
    CHECK(staff->wRestOffset == -4);
    CHECK(staff->hRestOffset == -4);
    CHECK(staff->otherRestOffset == -4);
    CHECK(staff->stemReversal == -4);
    CHECK(staff->botRepeatDotOff == -5);
    CHECK(staff->topRepeatDotOff == -3);
    CHECK(staff->altNotation == Staff::AlternateNotation::Normal);
    CHECK(staff->altLayer == 0);
    CHECK_FALSE(staff->altHideArtics);
    CHECK_FALSE(staff->altHideLyrics);
    CHECK_FALSE(staff->altHideSmartShapes);
    CHECK_FALSE(staff->altRhythmStemsUp);
    CHECK(staff->altSlashDots);
    CHECK_FALSE(staff->altHideOtherNotes);
    CHECK_FALSE(staff->altHideOtherArtics);
    CHECK_FALSE(staff->altHideExpressions);
    CHECK_FALSE(staff->altHideOtherLyrics);
    CHECK_FALSE(staff->altHideOtherSmartShapes);
    CHECK_FALSE(staff->altHideOtherExpressions);
    CHECK_FALSE(staff->redisplayLayerAccis);
    CHECK_FALSE(staff->hideTimeSigsInParts);
    CHECK_FALSE(staff->hideKeySigsShowAccis);
    CHECK(staff->instUuid == musx::dom::uuid::Unknown);
    CHECK(staffField(report, "staffLines").origin == ValueOrigin::LegacyBehavior);
    CHECK(staffField(report, "noteFont.fontSize").origin == ValueOrigin::Finale27Default);
    CHECK(staffField(report, "lineSpace").origin == ValueOrigin::Finale27Default);
    CHECK(staffField(report, "dwRestOffset").origin == ValueOrigin::Finale27Default);
    CHECK(staffField(report, "stemReversal").origin == ValueOrigin::Finale27Default);
    CHECK(staffField(report, "fretInstId").origin == ValueOrigin::Finale27Default);
    CHECK(staffField(report, "botRepeatDotOff").origin == ValueOrigin::Finale27Default);
    CHECK(staffField(report, "topRepeatDotOff").origin == ValueOrigin::Finale27Default);
    for (const auto *member : {
             "transposedClef",
             "capoPos",
             "lowestFret",
             "showNameInParts",
             "showNoteColors",
             "hideRepeatBottomDot",
             "hideRepeatTopDot",
             "flatBeams",
             "hideFretboards",
             "hideLyrics",
             "noOptimize",
             "hideBarlines",
             "hideRptBars",
             "hideChords",
         })
    {
        CHECK(staffField(report, member).origin == ValueOrigin::Finale27Default);
    }
    CHECK(staffField(report, "defaultClef").origin == ValueOrigin::LegacyMus);
    CHECK(staffField(report, "hideTimeSigs").origin == ValueOrigin::LegacyMus);
    for (const auto *member :
         {"customStaff", "notationStyle", "useNoteShapes", "useNoteFont", "blankMeasure"})
    {
        CHECK(staffField(report, member).origin == ValueOrigin::Finale27Default);
    }
    CHECK(staffField(report, "vertTabNumOff").origin == ValueOrigin::LegacyBehavior);
    for (const auto member : staffAlternateNotationFields)
        CHECK(staffField(report, member).origin == ValueOrigin::LegacyBehavior);
    CHECK(staffField(report, "redisplayLayerAccis").origin == ValueOrigin::LegacyBehavior);
    CHECK(staffField(report, "hideTimeSigsInParts").origin == ValueOrigin::LegacyBehavior);
    CHECK(staffField(report, "hideKeySigsShowAccis").origin == ValueOrigin::LegacyBehavior);
    CHECK(staffField(report, "instUuid").origin == ValueOrigin::LegacyBehavior);
    CHECK(staffField(report, "autoNumbering").origin == ValueOrigin::LegacyBehavior);
    CHECK(staffField(report, "useAutoNumbering").origin == ValueOrigin::LegacyBehavior);
    CHECK(reportedFieldCount(report) == staffFieldManifestSize);
}

TEST_CASE("Coda Staff default clef occupies the low three bits of its display word")
{
    const auto parsed =
        makeContainer({{7, "IS", {7, 0, 4, 1024, 0, 0x051c}}}, FormatEpoch::CodaBanner);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto report = staffImport(parsed, profile, document);

    const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(staff);
    CHECK(staff->defaultClef == 4);
    CHECK(staff->hideClefs);
    CHECK(staffField(report, "defaultClef").origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Coda Staff display flags retain their later meanings")
{
    constexpr std::int16_t displayFlags = static_cast<std::int16_t>(0xf738);
    const auto parsed =
        makeContainer({{7, "IS", {0, 0, 4, 1024, 0, displayFlags}}}, FormatEpoch::CodaBanner);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto report = staffImport(parsed, profile, document);

    const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(staff);
    CHECK(staff->floatKeys);
    CHECK(staff->floatTime);
    CHECK(staff->blineBreak);
    CHECK(staff->rbarBreak);
    CHECK(staff->hideMeasNums);
    CHECK(staff->hideRepeats);
    CHECK(staff->hideNameInScore);
    CHECK(staff->hideKeySigs);
    CHECK(staff->hideTimeSigs);
    CHECK(staff->hideTimeSigsInParts);
    CHECK(staff->hideClefs);
    CHECK(staff->noKey);
    for (const auto *member : {
             "floatKeys",
             "floatTime",
             "blineBreak",
             "rbarBreak",
             "hideMeasNums",
             "hideRepeats",
             "hideNameInScore",
             "hideKeySigs",
             "hideTimeSigs",
             "hideClefs",
         })
    {
        CHECK(staffField(report, member).origin == ValueOrigin::LegacyMus);
    }
    CHECK(staffField(report, "noKey").origin == ValueOrigin::LegacyMusAdjusted);
}

TEST_CASE("Early uncompressed six-word Staff keeps key controls separate")
{
    const auto parsed =
        makeContainer({{7, "IS", {0, 0, 4, 1024, 0, 0x0020}}}, FormatEpoch::UncompressedLegacy);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto report = staffImport(parsed, profile, document);

    const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(staff);
    CHECK(staff->hideKeySigs);
    CHECK_FALSE(staff->noKey);
    CHECK(staffField(report, "hideKeySigs").origin == ValueOrigin::LegacyMus);
    CHECK(staffField(report, "noKey").origin == ValueOrigin::Finale27Default);
}

TEST_CASE("Coda Staff attributes distinguish line-count and one-line forms")
{
    const auto parsed = makeContainer(
        {
            {7, "IS", {0, 0, 4, 1024, 0, 0}},
            {7, "IA", {0, 0, 0, 0, 0, 0x0040}},
            {8, "IS", {0, 0, 4, 1024, 0, 0}},
            {8, "IA", {0, 0, 3, 0, 0, 0x0040}},
            {9, "IS", {0, 0, 4, 1024, 0, 0}},
            {9, "IA", {0, 0, 1, 0, 0, 0x0040}},
            {10, "IS", {0, 0, 4, 1024, 0, 0}},
            {10, "IA", {0, -948, -1, 2, 0x0900, 0x0340}},
        },
        FormatEpoch::CodaBanner);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto report = staffImport(parsed, profile, document);

    const auto fiveLine = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    const auto threeLine = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 8);
    const auto centered = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 9);
    const auto legacyTab = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 10);
    REQUIRE(fiveLine);
    REQUIRE(threeLine);
    REQUIRE(centered);
    REQUIRE(legacyTab);
    CHECK(fiveLine->staffLines == 0);
    CHECK(fiveLine->botRepeatDotOff == -5);
    CHECK(fiveLine->topRepeatDotOff == -3);
    CHECK(threeLine->staffLines == 3);
    CHECK_FALSE(fiveLine->useNoteFont);
    REQUIRE(centered->customStaff);
    CHECK(*centered->customStaff == std::vector<int>{13});
    CHECK(centered->botBarlineOffset == -2 * evpusPerSpace);
    CHECK(centered->topBarlineOffset == 2 * evpusPerSpace);
    REQUIRE(legacyTab->customStaff);
    CHECK(*legacyTab->customStaff == std::vector<int>{11});
    CHECK(legacyTab->blankMeasure);
    CHECK(legacyTab->vertTabNumOff == -1024);
    CHECK(legacyTab->botRepeatDotOff == -1);
    CHECK(legacyTab->topRepeatDotOff == 1);
    CHECK(legacyTab->fretInstId == 2);
    const auto fret = document->getOthers()->get<musx::dom::others::FretInstrument>(
        musx::dom::SCORE_PARTID, legacyTab->fretInstId);
    REQUIRE(fret);
    CHECK(fret->numFrets == 20);
    CHECK(fret->numStrings == 1);
    CHECK(fret->name == "E5");
    REQUIRE(fret->strings.size() == 1);
    CHECK(fret->strings.front()->pitch == 76);
    const auto *fretSource = report.findField<Staff>("fretInstId", musx::dom::SCORE_PARTID, 10);
    REQUIRE(fretSource);
    CHECK(fretSource->origin == ValueOrigin::LegacyBehavior);
}

TEST_CASE("Coda Staff attributes recover independent note settings")
{
    const auto parsed = makeContainer(
        {
            {7, "IS", {0, 0, 4, 1024, 0, 0}},
            {7, "IA", {0, 0, 0, 5, 0x0f00, 0x00a0}},
            {8, "IS", {0, 0, 4, 1024, 0, 0}},
            {8, "IA", {0, 0, 0, 2, 0, 0x0200}},
            {9, "IS", {0, 0, 4, 1024, 0, 0}},
            {9, "IA", {0, 0, 0, 3, 0, 0x0200}},
        },
        FormatEpoch::CodaBanner);
    const auto document = emptyStaffDocument(2);
    auto profile = SourceProfile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto report = staffImport(parsed, profile, document);

    const auto standardStaff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    const auto defaultTabFontStaff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 8);
    const auto independentTabFontStaff =
        document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 9);
    REQUIRE(standardStaff);
    REQUIRE(defaultTabFontStaff);
    REQUIRE(independentTabFontStaff);
    CHECK(standardStaff->useNoteFont);
    CHECK(standardStaff->useNoteShapes);
    REQUIRE(standardStaff->noteFont);
    CHECK(standardStaff->noteFont->fontId == 5);
    CHECK(standardStaff->noteFont->fontSize == 15);
    CHECK(defaultTabFontStaff->useNoteFont);
    CHECK(independentTabFontStaff->useNoteFont);
    CHECK(staffField(report, "useNoteFont").origin == ValueOrigin::LegacyMus);
    CHECK(staffField(report, "useNoteShapes").origin == ValueOrigin::LegacyMus);
    const auto *tabFontSource = report.findField<Staff>("useNoteFont", musx::dom::SCORE_PARTID, 9);
    REQUIRE(tabFontSource);
    CHECK(tabFontSource->origin == ValueOrigin::LegacyBehavior);
}

TEST_CASE("Finale 1.0 Staff float controls recover against their enabled baseline",
          "[class][reader]")
{
    struct FixtureCase
    {
        const char *fixture;
        bool floatKeys;
        bool floatTime;
        bool useNoteFont;
        bool useNoteShapes;
    };
    for (const auto &fixtureCase : {
             FixtureCase{"evidence/F100/staffopts/F100-floatkey.mus", true, false, false, false},
             FixtureCase{"evidence/F100/staffopts/F100-floattime.mus", false, true, false, false},
             FixtureCase{"evidence/F100/staffopts/F100-floatfont.mus", false, false, true, false},
             FixtureCase{"evidence/F100/staffopts/F100-floatnoteshapes.mus", false, false, false,
                         true},
         })
    {
        const auto result = readFixture(fixtureCase.fixture);
        const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
        REQUIRE(staff);
        INFO(fixtureCase.fixture);
        CHECK(staff->floatKeys == fixtureCase.floatKeys);
        CHECK(staff->floatTime == fixtureCase.floatTime);
        CHECK(staff->useNoteFont == fixtureCase.useNoteFont);
        CHECK(staff->useNoteShapes == fixtureCase.useNoteShapes);
        if (fixtureCase.useNoteFont)
        {
            REQUIRE(staff->noteFont);
            CHECK(staff->noteFont->fontId == 5);
            CHECK(staff->noteFont->fontSize == 15);
        }
    }
}

TEST_CASE("Finale 1.0 Staff display controls recover against the original baseline",
          "[class][reader]")
{
    struct FixtureCase
    {
        const char *fixture;
        const char *field;
        bool Staff::*member;
    };
    for (const auto &fixtureCase : {
             FixtureCase{"evidence/F100/staffopts/F100-break-barlines.mus", "blineBreak",
                         &Staff::blineBreak},
             FixtureCase{"evidence/F100/staffopts/F100-break-repeatbars.mus", "rbarBreak",
                         &Staff::rbarBreak},
             FixtureCase{"evidence/F100/staffopts/F100-hide-clefs.mus", "hideClefs",
                         &Staff::hideClefs},
             FixtureCase{"evidence/F100/staffopts/F100-hide-endingsrpts.mus", "hideRepeats",
                         &Staff::hideRepeats},
             FixtureCase{"evidence/F100/staffopts/F100-hide-measnums.mus", "hideMeasNums",
                         &Staff::hideMeasNums},
             FixtureCase{"evidence/F100/staffopts/F100-hide-name.mus", "hideNameInScore",
                         &Staff::hideNameInScore},
             FixtureCase{"evidence/F100/staffopts/F100-hide-timesigs.mus", "hideTimeSigs",
                         &Staff::hideTimeSigs},
             FixtureCase{"evidence/F100/staffopts/F100-negate-keys.mus", "hideKeySigs",
                         &Staff::hideKeySigs},
         })
    {
        const auto result = readFixture(fixtureCase.fixture);
        const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
        REQUIRE(staff);
        INFO(fixtureCase.fixture);
        CHECK(staff.get()->*fixtureCase.member);
        CHECK(staffField(result.report, fixtureCase.field, 1).origin == ValueOrigin::LegacyMus);
    }

    const auto timeResult = readFixture("evidence/F100/staffopts/F100-hide-timesigs.mus");
    const auto timeStaff = timeResult.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(timeStaff);
    CHECK(timeStaff->hideTimeSigsInParts);

    const auto keyResult = readFixture("evidence/F100/staffopts/F100-negate-keys.mus");
    const auto keyStaff = keyResult.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(keyStaff);
    CHECK(keyStaff->noKey);
    CHECK(staffField(keyResult.report, "noKey", 1).origin == ValueOrigin::LegacyMusAdjusted);
}

TEST_CASE("Coda Staff line overrides decode signed ordinary and custom forms")
{
    const auto parsed = makeContainer(
        {
            {7, "IS", {0, 0, 4, 1024, 0, 0}},
            {7, "IA", {0, 0, 2, 0, 0, 0x0040}},
            {8, "IS", {0, 0, 4, 1024, 0, 0}},
            {8, "IA", {0, 0, 4, 0, 0, 0x0040}},
            {9, "IS", {0, 0, 4, 1024, 0, 0}},
            {9, "IA", {0, 0, 17, 0, 0, 0x0040}},
            {10, "IS", {0, 0, 4, 1024, 0, 0}},
            {10, "IA", {0, 0, -3, 0, 0, 0x0040}},
            {11, "IS", {0, 0, 4, 1024, 0, 0}},
            {11, "IA", {0, 0, -11, 0, 0, 0x0040}},
            {12, "IS", {0, 0, 4, 1024, 0, 0}},
            {12, "IA", {0, 0, 17, 0, 0, 0}},
        },
        FormatEpoch::CodaBanner);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto report = staffImport(parsed, profile, document);

    const auto twoLine = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    const auto fourLine = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 8);
    const auto seventeenLine = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 9);
    REQUIRE(twoLine);
    REQUIRE(fourLine);
    REQUIRE(seventeenLine);
    CHECK(twoLine->staffLines == 2);
    CHECK(fourLine->staffLines == 4);
    CHECK(seventeenLine->staffLines == 17);
    for (const auto &staff : {twoLine, fourLine, seventeenLine})
    {
        CHECK(staff->botRepeatDotOff == -5);
        CHECK(staff->topRepeatDotOff == -3);
    }

    const auto line13 = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 10);
    REQUIRE(line13);
    REQUIRE(line13->customStaff);
    CHECK(*line13->customStaff == std::vector<int>{13});
    CHECK(line13->botBarlineOffset == 0);
    CHECK(line13->topBarlineOffset == 2 * evpusPerSpace);
    CHECK(line13->botRepeatDotOff == -5);
    CHECK(line13->topRepeatDotOff == -3);

    const auto line21 = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 11);
    REQUIRE(line21);
    REQUIRE(line21->customStaff);
    CHECK(*line21->customStaff == std::vector<int>{21});
    CHECK(line21->botBarlineOffset == 0);
    CHECK(line21->topBarlineOffset == 10 * evpusPerSpace);
    CHECK(line21->botRepeatDotOff == -21);
    CHECK(line21->topRepeatDotOff == -19);

    const auto inactive = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 12);
    REQUIRE(inactive);
    CHECK(inactive->staffLines == 5);
    const auto *topBarline =
        report.findField<Staff>("topBarlineOffset", musx::dom::SCORE_PARTID, 11);
    REQUIRE(topBarline);
    CHECK(topBarline->origin == ValueOrigin::LegacyMusAdjusted);
    for (const auto staffId : {7, 8, 9})
    {
        const auto *bottomDot =
            report.findField<Staff>("botRepeatDotOff", musx::dom::SCORE_PARTID, staffId);
        const auto *topDot =
            report.findField<Staff>("topRepeatDotOff", musx::dom::SCORE_PARTID, staffId);
        REQUIRE(bottomDot);
        REQUIRE(topDot);
        CHECK(bottomDot->origin == ValueOrigin::Finale27Default);
        CHECK(topDot->origin == ValueOrigin::Finale27Default);
    }
}

TEST_CASE("Short one-string tablature Staffs retain their legacy behavior")
{
    struct LegacyTabCase
    {
        finale_mus_reader::VersionBound version;
        bool modernLineCount;
    };
    constexpr LegacyTabCase cases[]{
        {finale_mus_reader::versions::finale97, false},
        {finale_mus_reader::versions::finale98, false},
        {finale_mus_reader::versions::finale2000, true},
    };

    for (const auto &testCase : cases)
    {
        const std::int16_t topLines = testCase.modernLineCount ? 0 : 1;
        const std::int16_t primary = testCase.modernLineCount ? 0x0301 : 0x0340;
        const auto parsed = makeContainer(
            {
                {7, "IS", {0, -948, 0, 3, 0x0900, primary}},
                {7, "IS", {0, topLines, 1, 0, 0, 0x0f38}},
                {7, "IS", {-772, -772, -4, 1, 0, 0}},
            },
            FormatEpoch::UncompressedLegacy);
        const auto document = emptyStaffDocument();
        auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
        profile.version = SourceVersion{.major = testCase.version.major,
                                        .minor = testCase.version.minor,
                                        .maint = testCase.version.maint};
        profile.byteOrder = ByteOrder::BigEndian;
        const auto report = staffImport(parsed, profile, document);

        const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
        REQUIRE(staff);
        if (testCase.modernLineCount)
        {
            CHECK(staff->staffLines == 1);
        }
        else
        {
            CHECK_FALSE(staff->staffLines);
            REQUIRE(staff->customStaff);
            CHECK(*staff->customStaff == std::vector<int>{11});
        }
        CHECK(staff->capoPos == 0);
        CHECK(staff->lowestFret == 0);
        CHECK(staff->vertTabNumOff == -1024);
        CHECK(staff->showTabClefAllSys);
        CHECK(staff->hideRests);
        CHECK(staff->hideDots);
        CHECK(staff->hideStems);
        CHECK(staff->hideTuplets);
        CHECK_FALSE(staff->breakTabLinesAtNotes);
        CHECK(staff->fretInstId == 2);
        CHECK(staff->botRepeatDotOff == (testCase.modernLineCount ? -5 : -1));
        CHECK(staff->topRepeatDotOff == (testCase.modernLineCount ? -3 : 1));
        if (!testCase.modernLineCount)
        {
            CHECK(staff->hideRepeatBottomDot);
            CHECK(staff->hideRepeatTopDot);
        }
        const auto fret = document->getOthers()->get<musx::dom::others::FretInstrument>(
            musx::dom::SCORE_PARTID, staff->fretInstId);
        REQUIRE(fret);
        CHECK(fret->name == "E5");
        REQUIRE(fret->strings.size() == 1);
        CHECK(fret->strings.front()->pitch == 76);
        CHECK(staffField(report, "vertTabNumOff").origin == ValueOrigin::LegacyMus);
        CHECK(staffField(report, "fretInstId").origin == ValueOrigin::LegacyBehavior);
        CHECK(staffField(report, "showTabClefAllSys").origin == ValueOrigin::LegacyBehavior);
        CHECK(staffField(report, "breakTabLinesAtNotes").origin == ValueOrigin::Finale27Default);
    }

    const auto controlled = readFixture("evidence/F2000/F2000-tablature.mus");
    const auto controlledStaff =
        controlled.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(controlledStaff);
    CHECK_FALSE(controlledStaff->breakTabLinesAtNotes);
    CHECK(staffField(controlled.report, "breakTabLinesAtNotes", 1).origin ==
          ValueOrigin::Finale27Default);
}

TEST_CASE("The 18-word tablature positions continue into the DCL epoch")
{
    const auto parsed = makeContainer(
        {
            {7, "IS", {0, -948, 0, 3, 0x0900, 0x0301}},
            {7, "IS", {0, 0, 1, 0, 0, 0x0f38}},
            {7, "IS", {-772, -772, -4, 1, 0, 0}},
        },
        FormatEpoch::DclLegacy);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::DclLegacy);
    profile.version = SourceVersion{.major = finale_mus_reader::versions::finale2001.major};
    profile.byteOrder = ByteOrder::BigEndian;
    staffImport(parsed, profile, document);

    const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(staff);
    CHECK(staff->staffLines == 1);
    CHECK(staff->capoPos == 0);
    CHECK(staff->lowestFret == 0);
    CHECK(staff->vertTabNumOff == -1024);
    CHECK(staff->fretInstId == 2);
    const auto fret = document->getOthers()->get<musx::dom::others::FretInstrument>(
        musx::dom::SCORE_PARTID, staff->fretInstId);
    REQUIRE(fret);
    REQUIRE(fret->strings.size() == 1);
    CHECK(fret->strings.front()->pitch == 76);
}

TEST_CASE("Controlled Finale 2002 Staff settings use the 18-word layout")
{
    const auto ignored = readFixture("evidence/F2002/F2002-ignore-key.mus");
    const auto ignoredStaff = ignored.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(ignoredStaff);
    CHECK(ignoredStaff->noKey);
    CHECK(staffField(ignored.report, "fretInstId", 1).origin == ValueOrigin::Finale27Default);
    constexpr std::string_view noTailFields[] = {
        "showTabClefAllSys",
        "hideRests",
        "hideTies",
        "hideDots",
        "hideStems",
        "stemDirection",
        "stemStartFromStaff",
        "stemsFixedEnd",
        "useTabLetters",
        "hideBeams",
        "breakTabLinesAtNotes",
        "stemsFixedStart",
        "hideTuplets",
        "horzStemOffUp",
        "horzStemOffDown",
        "vertStemStartOffUp",
        "vertStemStartOffDown",
        "vertStemEndOffUp",
        "vertStemEndOffDown",
    };
    for (const auto field : noTailFields)
    {
        CHECK(staffField(ignored.report, field, 1).origin == ValueOrigin::Finale27Default);
    }

    const auto tablature = readFixture("evidence/F2002/F2002-tablature.mus");
    const auto tablatureStaff =
        tablature.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(tablatureStaff);
    CHECK(tablatureStaff->notationStyle == Staff::NotationStyle::Tablature);
    CHECK(tablatureStaff->capoPos == 0);
    CHECK(tablatureStaff->lowestFret == 0);
    CHECK(tablatureStaff->fretInstId != 0);
    const auto tablatureFret =
        tablature.document->getOthers()->get<musx::dom::others::FretInstrument>(
            musx::dom::SCORE_PARTID, tablatureStaff->fretInstId);
    REQUIRE(tablatureFret);
    REQUIRE(tablatureFret->strings.size() == 1);
    CHECK(tablatureFret->strings.front()->pitch == 0);
    CHECK(staffField(tablature.report, "fretInstId", 1).origin == ValueOrigin::LegacyBehavior);
    CHECK(tablatureStaff->showTabClefAllSys);
    CHECK(tablatureStaff->hideRests);
    CHECK(tablatureStaff->hideDots);
    CHECK(tablatureStaff->hideStems);
    CHECK(tablatureStaff->hideTuplets);
    CHECK_FALSE(tablatureStaff->breakTabLinesAtNotes);
    for (const auto field :
         {"showTabClefAllSys", "hideRests", "hideDots", "hideStems", "hideTuplets"})
    {
        CHECK(staffField(tablature.report, field, 1).origin == ValueOrigin::LegacyBehavior);
    }
    CHECK(staffField(tablature.report, "breakTabLinesAtNotes", 1).origin ==
          ValueOrigin::Finale27Default);
    for (const auto field :
         {"hideTies", "stemDirection", "stemStartFromStaff", "stemsFixedEnd", "useTabLetters",
          "hideBeams", "stemsFixedStart", "horzStemOffUp", "horzStemOffDown", "vertStemStartOffUp",
          "vertStemStartOffDown", "vertStemEndOffUp", "vertStemEndOffDown"})
    {
        CHECK(staffField(tablature.report, field, 1).origin == ValueOrigin::Finale27Default);
    }

    const auto baseKey = readFixture("evidence/F2002/F2002-tablature-basekey47.mus");
    const auto baseKeyStaff = baseKey.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(baseKeyStaff);
    const auto baseKeyFret = baseKey.document->getOthers()->get<musx::dom::others::FretInstrument>(
        musx::dom::SCORE_PARTID, baseKeyStaff->fretInstId);
    REQUIRE(baseKeyFret);
    REQUIRE(baseKeyFret->strings.size() == 1);
    CHECK(baseKeyFret->strings.front()->pitch == 47);
    CHECK(baseKeyFret->name == "B2");
    CHECK(staffField(baseKey.report, "fretInstId", 1).origin == ValueOrigin::LegacyBehavior);
}

TEST_CASE("Parallel Staff names supplement missing stored name references")
{
    const auto parsed = makeContainer({
        {7, "IS", {0, 0, 0, 0, 0, 0}},
        {7, "IS", {0, 5, 0, 0, 0, 0}},
        {7, "IS", {0, 0, 0, 0, 0, 0}},
        {7, "IN", {0x4e61, 0x6d65, 0, 0, 0, 0}},
        {7, "in", {0x4162, 0x6272, 0, 0, 0, 0}},
        {8, "IS", {0, 0, 0, 0, 0, 0}},
        {8, "IS", {0, 5, 0, 0, 0, 0}},
        {8, "IS", {0, 0, 0, 21, 22, 0}},
        {8, "IN", {0x4967, 0x6e6f, 0x7265, 0x6400, 0, 0}},
        {8, "in", {0x4967, 0x6e6f, 0x7265, 0x6400, 0, 0}},
    });
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
    profile.version = SourceVersion{.major = 3, .minor = 5};
    profile.byteOrder = ByteOrder::BigEndian;
    const auto report = staffImport(parsed, profile, document);

    const auto recovered = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(recovered);
    CHECK(recovered->getFullName() == "Name");
    CHECK(recovered->getAbbreviatedName() == "Abbr");
    CHECK(recovered->fullNameTextId != 0);
    CHECK(recovered->abbrvNameTextId != 0);
    const auto *fullName = report.findField<Staff>("fullNameTextId", musx::dom::SCORE_PARTID, 7);
    const auto *abbreviatedName =
        report.findField<Staff>("abbrvNameTextId", musx::dom::SCORE_PARTID, 7);
    REQUIRE(fullName);
    REQUIRE(abbreviatedName);
    CHECK(fullName->origin == ValueOrigin::LegacyBehavior);
    CHECK(abbreviatedName->origin == ValueOrigin::LegacyBehavior);

    const auto stored = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 8);
    REQUIRE(stored);
    CHECK(stored->fullNameTextId == 21);
    CHECK(stored->abbrvNameTextId == 22);

    const auto finale37Document = emptyStaffDocument();
    profile.version = SourceVersion{.major = 3, .minor = 7};
    staffImport(parsed, profile, finale37Document);
    const auto finale37Staff =
        finale37Document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(finale37Staff);
    CHECK(finale37Staff->fullNameTextId == 0);
    CHECK(finale37Staff->abbrvNameTextId == 0);
}

TEST_CASE("Finale 1.0 Staff properties recover from the six-word row")
{
    const auto result = readFixture("evidence/F100/F100-staffprops.mus");
    const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(staff);
    CHECK(staff->staffLines == 5);
    REQUIRE(staff->noteFont);
    CHECK(staff->noteFont->fontSize == 24);
    CHECK(staff->defaultClef == 2);
    REQUIRE(staff->transposition);
    REQUIRE(staff->transposition->keysig);
    CHECK(staff->transposition->keysig->interval == 5);
    CHECK(staff->transposition->keysig->adjust == 3);
    CHECK(staff->lineSpace == evpusPerSpace);
    CHECK(staff->dwRestOffset == -4);
    CHECK(staff->wRestOffset == -4);
    CHECK(staff->hRestOffset == -4);
    CHECK(staff->otherRestOffset == -4);
    CHECK(staff->stemReversal == -4);
    CHECK(staff->botRepeatDotOff == -5);
    CHECK(staff->topRepeatDotOff == -3);
    CHECK(staff->getFullName() == "Blown Horn");
    CHECK(staff->getAbbreviatedName().empty());
    REQUIRE(staff->fullNameTextId != 0);
    const auto nameBlock = result.document->getOthers()->get<musx::dom::others::TextBlock>(
        musx::dom::SCORE_PARTID, staff->fullNameTextId);
    REQUIRE(nameBlock);
    CHECK(nameBlock->textId != 0);
    CHECK(nameBlock->textType == musx::dom::others::TextBlock::TextType::Block);
    CHECK(nameBlock->lineSpacingPercentage == 100);
    CHECK(nameBlock->newPos36);
    CHECK(nameBlock->showShape);
    CHECK(nameBlock->wordWrap);

    const auto field = [&](const char *member)
    { return result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1); };
    REQUIRE(field("defaultClef"));
    REQUIRE(field("transposition.keysig.interval"));
    REQUIRE(field("dwRestOffset"));
    REQUIRE(field("noteFont.fontSize"));
    CHECK(field("defaultClef")->origin == ValueOrigin::LegacyMus);
    CHECK(field("transposition.keysig.interval")->origin == ValueOrigin::LegacyMus);
    CHECK(field("dwRestOffset")->origin == ValueOrigin::Finale27Default);
    CHECK(field("noteFont.fontSize")->origin == ValueOrigin::Finale27Default);
    REQUIRE(field("fullNameTextId"));
    CHECK(field("fullNameTextId")->origin == ValueOrigin::LegacyBehavior);
}

TEST_CASE("The six-word Staff transposition word carries its set-to-clef index")
{
    const auto result = readFixture("evidence/F100/staffopts/F100-transp-settoclef7.mus");
    const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(staff);
    CHECK(staff->transposedClef == 7);
    REQUIRE(staff->transposition);
    CHECK(staff->transposition->setToClef);
    CHECK_FALSE(staff->transposition->noSimplifyKey);
    CHECK_FALSE(staff->transposition->chromatic);
    REQUIRE(staff->transposition->keysig);
    CHECK(staff->transposition->keysig->interval == 0);
    CHECK(staff->transposition->keysig->adjust == 0);
    const auto *field =
        result.report.findField<Staff>("transposedClef", musx::dom::SCORE_PARTID, 1);
    REQUIRE(field);
    CHECK(field->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Finale 2.6.3 optional Staff attributes recover custom lines and "
          "notehead font")
{
    const auto result = readFixture("evidence/F263/F263-staffopts.mus");
    const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(staff);
    CHECK_FALSE(staff->staffLines);
    REQUIRE(staff->customStaff);
    CHECK(*staff->customStaff == std::vector<int>{13});
    CHECK(staff->botBarlineOffset == -2 * evpusPerSpace);
    CHECK(staff->topBarlineOffset == 2 * evpusPerSpace);
    CHECK(staff->notationStyle == Staff::NotationStyle::Tablature);
    CHECK(staff->vertTabNumOff == 768);
    CHECK(staff->fretInstId == 2);
    CHECK(staff->showTabClefAllSys);
    CHECK(staff->hideRests);
    CHECK(staff->hideDots);
    CHECK(staff->hideStems);
    CHECK(staff->hideTuplets);
    CHECK_FALSE(staff->hideTies);
    CHECK_FALSE(staff->hideBeams);
    CHECK_FALSE(staff->useTabLetters);
    CHECK_FALSE(staff->breakTabLinesAtNotes);
    CHECK(staff->useNoteFont);
    REQUIRE(staff->noteFont);
    CHECK(staff->noteFont->fontId == 33);
    CHECK(staff->noteFont->getName() == "AGaramond");
    CHECK(staff->noteFont->fontSize == 13);
    CHECK_FALSE(staff->noteFont->bold);
    CHECK(staff->noteFont->italic);
    CHECK(staff->defaultClef == 1);
    CHECK(staff->hideTimeSigs);
    CHECK(staff->hideTimeSigsInParts);
    CHECK(staff->dwRestOffset == -4);
    CHECK(staff->stemReversal == -4);
    CHECK(staff->botRepeatDotOff == -5);
    CHECK(staff->topRepeatDotOff == -3);
    CHECK(staff->getFullName() == "Horn Blown");
    CHECK(staff->getAbbreviatedName() == "H.B.");
    REQUIRE(staff->fullNameTextId != 0);
    REQUIRE(staff->abbrvNameTextId != 0);
    CHECK(staff->fullNameTextId != staff->abbrvNameTextId);
    const auto fret = result.document->getOthers()->get<musx::dom::others::FretInstrument>(
        musx::dom::SCORE_PARTID, staff->fretInstId);
    REQUIRE(fret);
    REQUIRE(fret->strings.size() == 1);
    CHECK(fret->strings.front()->pitch == 1);
    CHECK(fret->name == "C#-1");
    const auto fullNameBlock = result.document->getOthers()->get<musx::dom::others::TextBlock>(
        musx::dom::SCORE_PARTID, staff->fullNameTextId);
    const auto abbreviatedNameBlock =
        result.document->getOthers()->get<musx::dom::others::TextBlock>(musx::dom::SCORE_PARTID,
                                                                        staff->abbrvNameTextId);
    REQUIRE(fullNameBlock);
    REQUIRE(abbreviatedNameBlock);
    CHECK(fullNameBlock->textId != 0);
    CHECK(abbreviatedNameBlock->textId != 0);
    CHECK(fullNameBlock->textId != abbreviatedNameBlock->textId);

    const auto field = [&](const char *member)
    { return result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1); };
    REQUIRE(field("customStaff"));
    REQUIRE(field("botBarlineOffset"));
    REQUIRE(field("topBarlineOffset"));
    REQUIRE(field("notationStyle"));
    REQUIRE(field("vertTabNumOff"));
    REQUIRE(field("fretInstId"));
    REQUIRE(field("showTabClefAllSys"));
    REQUIRE(field("hideRests"));
    REQUIRE(field("hideDots"));
    REQUIRE(field("hideStems"));
    REQUIRE(field("hideTuplets"));
    REQUIRE(field("noteFont.fontSize"));
    REQUIRE(field("hideTimeSigs"));
    CHECK(field("customStaff")->origin == ValueOrigin::LegacyMus);
    CHECK(field("botBarlineOffset")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("topBarlineOffset")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("notationStyle")->origin == ValueOrigin::LegacyMus);
    CHECK(field("vertTabNumOff")->origin == ValueOrigin::LegacyMus);
    CHECK(field("fretInstId")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("showTabClefAllSys")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("hideRests")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("hideDots")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("hideStems")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("hideTuplets")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("noteFont.fontSize")->origin == ValueOrigin::LegacyMus);
    CHECK(field("hideTimeSigs")->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Finale 2.6.3 Staff attributes recover the remaining tablature controls")
{
    const auto checkDefaultNoteheadFontSize = [](const auto &result, const auto &staff)
    {
        const auto defaultFont = musx::dom::options::FontOptions::getFontInfoOrNull(
            result.document, musx::dom::options::FontOptions::FontType::Noteheads);
        REQUIRE(defaultFont);
        REQUIRE(staff->noteFont);
        CHECK(staff->noteFont->fontSize == defaultFont->fontSize);
    };
    const auto baseline = readFixture("evidence/F263/staffopts/F263-tabstaff.mus");
    const auto baselineStaff =
        baseline.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(baselineStaff);
    CHECK(baselineStaff->notationStyle == Staff::NotationStyle::Tablature);
    CHECK(baselineStaff->useNoteFont);
    checkDefaultNoteheadFontSize(baseline, baselineStaff);
    CHECK(staffField(baseline.report, "noteFont.fontSize", 1).origin ==
          ValueOrigin::LegacyMusAdjusted);
    CHECK(baselineStaff->fretInstId == 2);
    const auto baselineFret =
        baseline.document->getOthers()->get<musx::dom::others::FretInstrument>(
            musx::dom::SCORE_PARTID, baselineStaff->fretInstId);
    REQUIRE(baselineFret);
    REQUIRE(baselineFret->strings.size() == 1);
    CHECK(baselineFret->strings.front()->pitch == 0);
    CHECK(baselineFret->name == "C-1");

    const auto baseKey = readFixture("evidence/F263/staffopts/F263-tabstaff-basekey48.mus");
    const auto baseKeyStaff = baseKey.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(baseKeyStaff);
    checkDefaultNoteheadFontSize(baseKey, baseKeyStaff);
    const auto baseKeyFret = baseKey.document->getOthers()->get<musx::dom::others::FretInstrument>(
        musx::dom::SCORE_PARTID, baseKeyStaff->fretInstId);
    REQUIRE(baseKeyFret);
    REQUIRE(baseKeyFret->strings.size() == 1);
    CHECK(baseKeyFret->strings.front()->pitch == 48);
    CHECK(baseKeyFret->name == "C3");

    const auto font = readFixture("evidence/F263/staffopts/F263-tabstaff-font13ital.mus");
    const auto fontStaff = font.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(fontStaff);
    REQUIRE(fontStaff->noteFont);
    CHECK(fontStaff->noteFont->fontId == 126);
    CHECK(fontStaff->noteFont->fontSize == 13);
    CHECK(fontStaff->noteFont->italic);
    CHECK(staffField(font.report, "noteFont.fontSize", 1).origin == ValueOrigin::LegacyMus);

    const auto offset = readFixture("evidence/F263/staffopts/F263-tabstaff-yoff29.mus");
    const auto offsetStaff = offset.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(offsetStaff);
    checkDefaultNoteheadFontSize(offset, offsetStaff);
    CHECK(offsetStaff->vertTabNumOff == 29 * 256);
}

TEST_CASE("Finale 2.6.3 Staff attributes recover blank empty measures")
{
    const auto result = readFixture("evidence/F263/staffopts/F263-blank-emptymeas.mus");
    const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(staff);
    CHECK(staff->blankMeasure);
}

TEST_CASE("The Finale 2012 Staff extension recovers its stored instrument UUID")
{
    constexpr std::array<std::uint8_t, 16> uuidBytes{0xa9, 0x25, 0x64, 0x8a, 0xab, 0xc9,
                                                     0x4d, 0xc7, 0xa6, 0x19, 0xa6, 0xce,
                                                     0x35, 0x5a, 0xd3, 0x3c};
    std::vector<std::int16_t> words(40, 0);
    for (std::size_t index = 0; index < uuidBytes.size(); index += 2)
    {
        words.push_back(static_cast<std::int16_t>(
            uuidBytes[index] | (static_cast<std::uint16_t>(uuidBytes[index + 1]) << 8U)));
    }
    const auto parsed = makeClassContainer(0x00e7, words, ByteOrder::LittleEndian, 7);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    const auto report = staffImport(parsed, profile, document);

    const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(staff);
    CHECK(staff->instUuid == "a925648a-abc9-4dc7-a619-a6ce355ad33c");
    CHECK(staffField(report, "instUuid").origin == ValueOrigin::LegacyMus);
    CHECK(staffField(report, "instUuid").rawValue == 16);
    CHECK(reportedFieldCount(report) == staffFieldManifestSize);
}

TEST_CASE("The Finale 2012 Staff layout adds independent staff-line hiding")
{
    const auto finale2011 = readFixture("evidence/F2011/F2011-baseline.mus");
    const auto baseline = readFixture("evidence/F2012/F2012-baseline.mus");
    const auto hidden = readFixture("evidence/F2012/F2012-hidestafflines.mus");

    const auto earlierStaff =
        finale2011.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    const auto baselineStaff =
        baseline.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    const auto hiddenStaff = hidden.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(earlierStaff);
    REQUIRE(baselineStaff);
    REQUIRE(hiddenStaff);
    CHECK_FALSE(earlierStaff->hideStaffLines);
    CHECK_FALSE(baselineStaff->hideStaffLines);
    CHECK(hiddenStaff->hideStaffLines);

    const auto *earlierSource =
        finale2011.report.findField<Staff>("hideStaffLines", musx::dom::SCORE_PARTID, 1);
    const auto *baselineSource =
        baseline.report.findField<Staff>("hideStaffLines", musx::dom::SCORE_PARTID, 1);
    const auto *hiddenSource =
        hidden.report.findField<Staff>("hideStaffLines", musx::dom::SCORE_PARTID, 1);
    REQUIRE(earlierSource);
    REQUIRE(baselineSource);
    REQUIRE(hiddenSource);
    CHECK(earlierSource->origin == ValueOrigin::Finale27Default);
    CHECK(baselineSource->origin == ValueOrigin::LegacyMus);
    CHECK(hiddenSource->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("The Finale 2012 Staff layout adds automatic name numbering")
{
    const auto finale2011 = readFixture("evidence/F2011/F2011-baseline.mus");
    const auto baseline = readFixture("evidence/F2012/F2012-baseline.mus");
    const auto numbered = readFixture("evidence/F2012/F2012-staff-autonum.mus");

    const auto earlierStaff =
        finale2011.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    const auto baselineStaff =
        baseline.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    const auto numberedStaff =
        numbered.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(earlierStaff);
    REQUIRE(baselineStaff);
    REQUIRE(numberedStaff);
    CHECK(earlierStaff->instUuid == musx::dom::uuid::Unknown);
    CHECK(static_cast<int>(earlierStaff->autoNumbering) == 0);
    CHECK_FALSE(earlierStaff->useAutoNumbering);
    CHECK(static_cast<int>(baselineStaff->autoNumbering) == 0);
    CHECK_FALSE(baselineStaff->useAutoNumbering);
    CHECK(numberedStaff->useAutoNumbering);
    CHECK(numberedStaff->autoNumbering == Staff::AutoNumberingStyle::OrdinalPrefix);

    const auto *earlierStyle =
        finale2011.report.findField<Staff>("autoNumbering", musx::dom::SCORE_PARTID, 1);
    const auto *earlierUuid =
        finale2011.report.findField<Staff>("instUuid", musx::dom::SCORE_PARTID, 1);
    const auto *earlierEnabled =
        finale2011.report.findField<Staff>("useAutoNumbering", musx::dom::SCORE_PARTID, 1);
    const auto *baselineStyle =
        baseline.report.findField<Staff>("autoNumbering", musx::dom::SCORE_PARTID, 1);
    const auto *baselineEnabled =
        baseline.report.findField<Staff>("useAutoNumbering", musx::dom::SCORE_PARTID, 1);
    const auto *numberedStyle =
        numbered.report.findField<Staff>("autoNumbering", musx::dom::SCORE_PARTID, 1);
    const auto *numberedEnabled =
        numbered.report.findField<Staff>("useAutoNumbering", musx::dom::SCORE_PARTID, 1);
    REQUIRE(earlierStyle);
    REQUIRE(earlierUuid);
    REQUIRE(earlierEnabled);
    REQUIRE(baselineStyle);
    REQUIRE(baselineEnabled);
    REQUIRE(numberedStyle);
    REQUIRE(numberedEnabled);
    CHECK(earlierStyle->origin == ValueOrigin::LegacyBehavior);
    CHECK(earlierUuid->origin == ValueOrigin::LegacyBehavior);
    CHECK(earlierEnabled->origin == ValueOrigin::LegacyBehavior);
    CHECK(baselineStyle->origin == ValueOrigin::LegacyMus);
    CHECK(baselineEnabled->origin == ValueOrigin::LegacyMus);
    CHECK(numberedStyle->origin == ValueOrigin::LegacyMus);
    CHECK(numberedEnabled->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("A short post-Coda Staff record is rejected")
{
    const auto parsed =
        makeClassContainer(0x00e7, std::vector<std::int16_t>(17), ByteOrder::LittleEndian, 7);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    const auto report = staffImport(parsed, profile, document);

    CHECK_FALSE(document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7));
    REQUIRE(report.diagnostics.size() == 1);
    CHECK(report.diagnostics.front().message.find("shorter than its base layout") !=
          std::string::npos);
}

TEST_CASE("Staff name references compare through their block text", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using BlockText = musx::dom::texts::BlockText;
    using TextBlock = musx::dom::others::TextBlock;

    const auto makeDocument =
        [](musx::dom::Cmper blockId, musx::dom::Cmper textId, std::string text)
    {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto raw = std::make_shared<BlockText>(document, musx::dom::SCORE_PARTID,
                                               musx::dom::EnigmaBase::ShareMode::All, textId);
        raw->text = std::move(text);
        document->getTexts()->add(BlockText::XmlNodeName, std::move(raw));
        auto block = std::make_shared<TextBlock>(document, musx::dom::SCORE_PARTID,
                                                 musx::dom::EnigmaBase::ShareMode::All, blockId);
        block->textId = textId;
        document->getOthers()->add(TextBlock::XmlNodeName, std::move(block));
        return document;
    };

    const auto source = makeDocument(7, 3, "Horn");
    const auto equivalent = makeDocument(19, 41, "Horn");
    const auto different = makeDocument(7, 3, "Trumpet");
    const auto formattingOnly = makeDocument(23, 47, "^font(Times)^size(14)^nfx(2)");
    auto danglingSession = musx::factory::DocumentFactory::begin();
    const auto dangling = danglingSession.getDocument();
    auto danglingBlock =
        std::make_shared<TextBlock>(dangling, musx::dom::SCORE_PARTID,
                                    musx::dom::EnigmaBase::ShareMode::All, musx::dom::Cmper(29));
    danglingBlock->textId = 53;
    dangling->getOthers()->add(TextBlock::XmlNodeName, std::move(danglingBlock));
    REQUIRE(comparison_text::compareStaffNameReferents("staff[cmper=1].full_name_text_id", 7, 19,
                                                       source, equivalent) == true);
    REQUIRE(comparison_text::compareStaffNameReferents("staff[cmper=1].abbrv_name_text_id", 7, 7,
                                                       source, different) == false);
    REQUIRE(comparison_text::compareStaffNameReferents("staff[cmper=1].full_name_text_id", 0, 29,
                                                       source, dangling) == true);
    REQUIRE(comparison_text::compareStaffNameReferents("staff[cmper=1].abbrv_name_text_id", 23, 0,
                                                       formattingOnly, equivalent) == true);
    REQUIRE(comparison_text::compareStaffNameReferents("staff[cmper=1].full_name_text_id", 0, 19,
                                                       source, equivalent) == false);
    REQUIRE_FALSE(comparison_text::compareStaffNameReferents("staff[cmper=1].default_clef", 7, 19,
                                                             source, equivalent));

    const auto snapshot = [](std::int64_t fullName, std::int64_t abbreviatedName)
    {
        return SurveySnapshot{
            {"staff", Value::Array{Value::Object{{"cmper", 1},
                                                 {"full_name_text_id", fullName},
                                                 {"abbrv_name_text_id", abbreviatedName}}}}};
    };
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);
    const auto zeroMismatch =
        compareSnapshots(snapshot(7, 0), snapshot(19, 7), source, equivalent,
                         finale_mus_reader::FormatEpoch::CodaBanner,
                         finale_mus_reader::ByteOrder::BigEndian, nullptr, report);
    const auto &zeroStats = zeroMismatch.classes.at("others").at("staff");
    CHECK(zeroStats.same == 2);
    CHECK(zeroStats.unexpected == 0);
    CHECK(zeroMismatch.transformations.at(ComparisonTransformation::EquivalentTextBlockReferent) ==
          2);

    const auto equalComparatorDifferentText =
        compareSnapshots(snapshot(7, 0), snapshot(7, 0), source, different,
                         finale_mus_reader::FormatEpoch::CodaBanner,
                         finale_mus_reader::ByteOrder::BigEndian, nullptr, report);
    const auto &differentStats = equalComparatorDifferentText.classes.at("others").at("staff");
    CHECK(differentStats.same == 1);
    CHECK(differentStats.unexpected == 1);
}

TEST_CASE("Disabled Staff note-font size differences are different defaults", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const ComparisonLeaves disabled{{"staff[cmper=1].use_note_font", {Value(false), {}}}};
    const ComparisonLeaves enabled{{"staff[cmper=1].use_note_font", {Value(true), {}}}};
    const ComparisonLeaves none;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);
    const auto context = [&](const ComparisonLeaves &source, std::int64_t sourceSize,
                             std::int64_t companionSize, std::string_view origin,
                             DifferenceCategory category = DifferenceCategory::Differs)
    {
        return DifferenceContext{"staff[cmper=1].note_font.font_size",
                                 category,
                                 origin,
                                 Value(sourceSize),
                                 Value(companionSize),
                                 source,
                                 none,
                                 finale_mus_reader::FormatEpoch::ZlibLegacy,
                                 finale_mus_reader::ByteOrder::LittleEndian,
                                 nullptr,
                                 report};
    };

    REQUIRE(classifyStaffDifference(context(disabled, 26, 0, "legacy-mus")) ==
            DifferenceClassification::DifferentDefaults);
    REQUIRE(classifyStaffDifference(context(disabled, 0, 24, "finale27-default")) ==
            DifferenceClassification::DifferentDefaults);
    REQUIRE_FALSE(classifyStaffDifference(context(enabled, 26, 0, "legacy-mus")));
    REQUIRE_FALSE(classifyStaffDifference(
        context(disabled, 26, 0, "legacy-mus", DifferenceCategory::ReaderOnly)));
}

TEST_CASE("Before Finale 2012 percussion and tablature note-font enablement "
          "is upgrade loss",
          "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using NotationStyle = musx::dom::others::Staff::NotationStyle;
    const Value no(false);
    const Value yes(true);
    const finale_mus_reader::SourceVersion finale2008{
        .major = finale_mus_reader::versions::finale2008.major};
    const finale_mus_reader::SourceVersion finale2011{
        .major = finale_mus_reader::versions::finale2011.major};
    const finale_mus_reader::SourceVersion finale2012{
        .major = finale_mus_reader::versions::finale2012.major};
    const finale_mus_reader::SourceVersion finale97{
        .major = finale_mus_reader::versions::finale97.major};
    const finale_mus_reader::SourceVersion finale2005{
        .major = finale_mus_reader::versions::finale2005.major};
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);
    const ComparisonLeaves percussion{
        {"staff[cmper=1].notation_style",
         {Value(static_cast<std::int64_t>(NotationStyle::Percussion)), "legacy-mus"}}};
    const ComparisonLeaves tablature{
        {"staff[cmper=1].notation_style",
         {Value(static_cast<std::int64_t>(NotationStyle::Tablature)), "legacy-mus"}}};
    const ComparisonLeaves standard{
        {"staff[cmper=1].notation_style",
         {Value(static_cast<std::int64_t>(NotationStyle::Standard)), "legacy-mus"}}};
    const ComparisonLeaves none;
    const auto context = [&](const ComparisonLeaves &leaves, const Value &source,
                             const Value &companion, std::string_view origin,
                             DifferenceCategory category, finale_mus_reader::FormatEpoch epoch,
                             const finale_mus_reader::SourceVersion *version)
    {
        return DifferenceContext{"staff[cmper=1].use_note_font",
                                 category,
                                 origin,
                                 source,
                                 companion,
                                 leaves,
                                 none,
                                 epoch,
                                 finale_mus_reader::ByteOrder::BigEndian,
                                 version,
                                 report};
    };

    REQUIRE(classifyStaffDifference(
                context(percussion, no, yes, "legacy-mus", DifferenceCategory::Differs,
                        finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2008)) ==
            DifferenceClassification::FinaleUpgradeLoss);
    REQUIRE(classifyStaffDifference(
                context(percussion, no, yes, "legacy-mus", DifferenceCategory::Differs,
                        finale_mus_reader::FormatEpoch::UncompressedLegacy, &finale97)) ==
            DifferenceClassification::FinaleUpgradeLoss);
    REQUIRE(classifyStaffDifference(
                context(tablature, no, yes, "legacy-mus", DifferenceCategory::Differs,
                        finale_mus_reader::FormatEpoch::DclLegacy, &finale2005)) ==
            DifferenceClassification::FinaleUpgradeLoss);
    REQUIRE_FALSE(classifyStaffDifference(
        context(standard, no, yes, "legacy-mus", DifferenceCategory::Differs,
                finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2008)));
    REQUIRE(classifyStaffDifference(
                context(percussion, yes, no, "legacy-mus", DifferenceCategory::Differs,
                        finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2008)) ==
            DifferenceClassification::FinaleUpgradeLoss);
    REQUIRE(classifyStaffDifference(
                context(percussion, yes, no, "legacy-mus", DifferenceCategory::Differs,
                        finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2011)) ==
            DifferenceClassification::FinaleUpgradeLoss);
    REQUIRE_FALSE(classifyStaffDifference(
        context(percussion, no, yes, "legacy-behavior", DifferenceCategory::Differs,
                finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2008)));
    REQUIRE_FALSE(classifyStaffDifference(
        context(percussion, no, yes, "legacy-mus", DifferenceCategory::ReaderOnly,
                finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2008)));
    REQUIRE_FALSE(classifyStaffDifference(
        context(percussion, no, yes, "legacy-mus", DifferenceCategory::Differs,
                finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2012)));
}

TEST_CASE("Coverage excludes the Studio View Staff", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using Staff = musx::dom::others::Staff;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto addStaff = [&](musx::dom::Cmper cmper)
    {
        auto staff = std::make_shared<Staff>(document, musx::dom::SCORE_PARTID,
                                             musx::dom::EnigmaBase::ShareMode::All, cmper);
        document->getOthers()->add(Staff::XmlNodeName, std::move(staff));
    };
    addStaff(1);
    addStaff(musx::dom::STUDIO_VIEW_STAFF_ID);

    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::DclLegacy);
    const auto observed = runAllSurveyors({document, report});
    const auto &staffs = observed.snapshot.at("staff").asArray();
    REQUIRE(staffs.size() == 1);
    CHECK(staffs.front().asObject().at("cmper").asInteger() == 1);
}

TEST_CASE("Only pre-Finale 2012 behavioral Staff UUIDs have different defaults", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value source{std::string(musx::dom::uuid::Unknown)};
    const Value companion{std::string(musx::dom::uuid::Flute)};
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);
    const finale_mus_reader::SourceVersion finale2011{
        .major = finale_mus_reader::versions::finale2011.major};
    const finale_mus_reader::SourceVersion finale2012{
        .major = finale_mus_reader::versions::finale2012.major};
    const auto context = [&](std::string_view path, std::string_view origin,
                             DifferenceCategory category,
                             const finale_mus_reader::SourceVersion *version)
    {
        return DifferenceContext{path,
                                 category,
                                 origin,
                                 source,
                                 companion,
                                 leaves,
                                 leaves,
                                 finale_mus_reader::FormatEpoch::ZlibLegacy,
                                 finale_mus_reader::ByteOrder::LittleEndian,
                                 version,
                                 report};
    };

    REQUIRE(classifyStaffDifference(context("staff[cmper=1].inst_uuid", "legacy-behavior",
                                            DifferenceCategory::Differs, &finale2011)) ==
            DifferenceClassification::DifferentDefaults);
    const DifferenceContext coda{"staff[cmper=1].inst_uuid",
                                 DifferenceCategory::Differs,
                                 "legacy-behavior",
                                 source,
                                 companion,
                                 leaves,
                                 leaves,
                                 finale_mus_reader::FormatEpoch::CodaBanner,
                                 finale_mus_reader::ByteOrder::BigEndian,
                                 nullptr,
                                 report};
    REQUIRE(classifyStaffDifference(coda) == DifferenceClassification::DifferentDefaults);
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].inst_uuid", "legacy-mus",
                                                  DifferenceCategory::Differs, &finale2011)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].inst_uuid", "legacy-behavior",
                                                  DifferenceCategory::Differs, &finale2012)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].inst_uuid", "legacy-behavior",
                                                  DifferenceCategory::ReaderOnly, &finale2011)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].line_space", "legacy-behavior",
                                                  DifferenceCategory::Differs, &finale2011)));
}

TEST_CASE("Unavailable and upgraded Staff hide-mode differences are different "
          "defaults",
          "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using HideMode = musx::dom::others::Staff::HideMode;
    const Value none{static_cast<std::int64_t>(HideMode::None)};
    const Value score{static_cast<std::int64_t>(HideMode::Score)};
    const Value cutaway{static_cast<std::int64_t>(HideMode::Cutaway)};
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);
    const finale_mus_reader::SourceVersion finale2010{
        .major = finale_mus_reader::versions::finale2010.major};
    const finale_mus_reader::SourceVersion finale2011{
        .major = finale_mus_reader::versions::finale2011.major};
    const auto context = [&](std::string_view path, std::string_view origin,
                             DifferenceCategory category, const Value &source,
                             const Value &companion, finale_mus_reader::FormatEpoch epoch,
                             const finale_mus_reader::SourceVersion *version)
    {
        return DifferenceContext{path,    category,  origin,
                                 source,  companion, leaves,
                                 leaves,  epoch,     finale_mus_reader::ByteOrder::LittleEndian,
                                 version, report};
    };

    REQUIRE(classifyStaffDifference(
                context("staff[cmper=1].hide_mode", "legacy-mus", DifferenceCategory::Differs, none,
                        score, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2010)) ==
            DifferenceClassification::DifferentDefaults);
    REQUIRE(classifyStaffDifference(
                context("staff[cmper=1].hide_mode", "legacy-mus", DifferenceCategory::Differs,
                        cutaway, score, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2010)) ==
            DifferenceClassification::DifferentDefaults);
    REQUIRE(classifyStaffDifference(context("staff[cmper=1].hide_mode", "finale27-default",
                                            DifferenceCategory::Differs, none, score,
                                            finale_mus_reader::FormatEpoch::CodaBanner, nullptr)) ==
            DifferenceClassification::DifferentDefaults);
    REQUIRE(classifyStaffDifference(context("staff[cmper=1].hide_mode", "finale27-default",
                                            DifferenceCategory::Differs, none, cutaway,
                                            finale_mus_reader::FormatEpoch::CodaBanner, nullptr)) ==
            DifferenceClassification::DifferentDefaults);
    REQUIRE(classifyStaffDifference(
                context("staff[cmper=1].hide_mode", "finale27-default", DifferenceCategory::Differs,
                        none, score, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2011)) ==
            DifferenceClassification::DifferentDefaults);
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=1].hide_mode", "legacy-mus", DifferenceCategory::Differs, none, score,
                finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2011)));
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=1].hide_mode", "legacy-mus", DifferenceCategory::Differs, cutaway,
                score, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2011)));
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=1].hide_mode", "legacy-mus", DifferenceCategory::Differs, none,
                cutaway, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2010)));
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=1].hide_mode", "legacy-mus", DifferenceCategory::Differs, score, none,
                finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2010)));
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=1].hide_mode", "finale27-default", DifferenceCategory::Differs, score,
                none, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2010)));
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=1].hide_mode", "unmapped", DifferenceCategory::Differs, none, score,
                finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2010)));
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=1].hide_mode", "legacy-mus", DifferenceCategory::ReaderOnly, none,
                score, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2010)));
}

TEST_CASE("Finale 2012 beta Staff accidental display behavior is upgrade loss", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value disabled{false};
    const Value enabled{true};
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);
    const finale_mus_reader::SourceVersion beta{
        .major = finale_mus_reader::versions::finale2012.major, .devStatus = 2};
    const finale_mus_reader::SourceVersion release{
        .major = finale_mus_reader::versions::finale2012.major};
    const auto context = [&](std::string_view path, std::string_view origin,
                             DifferenceCategory category, const Value &source,
                             const Value &companion,
                             const finale_mus_reader::SourceVersion *version)
    {
        return DifferenceContext{path,
                                 category,
                                 origin,
                                 source,
                                 companion,
                                 leaves,
                                 leaves,
                                 finale_mus_reader::FormatEpoch::ZlibLegacy,
                                 finale_mus_reader::ByteOrder::LittleEndian,
                                 version,
                                 report};
    };

    REQUIRE(classifyStaffDifference(context("staff[cmper=1].hide_key_sigs_show_accis",
                                            "legacy-behavior", DifferenceCategory::Differs,
                                            disabled, enabled, &beta)) ==
            DifferenceClassification::BetaDiscrepancy);
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].hide_key_sigs_show_accis",
                                                  "legacy-behavior", DifferenceCategory::Differs,
                                                  disabled, enabled, &release)));
    REQUIRE_FALSE(
        classifyStaffDifference(context("staff[cmper=1].hide_key_sigs_show_accis", "legacy-mus",
                                        DifferenceCategory::Differs, disabled, enabled, &beta)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].hide_key_sigs_show_accis",
                                                  "legacy-behavior", DifferenceCategory::Differs,
                                                  enabled, disabled, &beta)));
}

TEST_CASE("Six-word Staff defaults with fallback provenance are possibly "
          "unrecoverable",
          "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value source{-3};
    const Value companion{-2};
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);
    const auto context = [&](std::string_view path, std::string_view origin,
                             DifferenceCategory category, finale_mus_reader::FormatEpoch epoch)
    {
        return DifferenceContext{path,    category,  origin,
                                 source,  companion, leaves,
                                 leaves,  epoch,     finale_mus_reader::ByteOrder::BigEndian,
                                 nullptr, report};
    };

    const Value shown{true};
    const Value hidden{false};
    const auto hideKeySigs = [&](finale_mus_reader::FormatEpoch epoch)
    {
        return DifferenceContext{"staff[cmper=1].hide_key_sigs",
                                 DifferenceCategory::Differs,
                                 "unmapped",
                                 hidden,
                                 shown,
                                 leaves,
                                 leaves,
                                 epoch,
                                 finale_mus_reader::ByteOrder::BigEndian,
                                 nullptr,
                                 report};
    };
    REQUIRE_FALSE(
        classifyStaffDifference(hideKeySigs(finale_mus_reader::FormatEpoch::UncompressedLegacy)));
    report.setField(finale_mus_reader::instanceKey<Staff>(musx::dom::SCORE_PARTID, 1),
                    "dwRestOffset", {ValueOrigin::Finale27Default, 0, 0, -4});
    for (const auto epoch : {finale_mus_reader::FormatEpoch::CodaBanner,
                             finale_mus_reader::FormatEpoch::UncompressedLegacy})
    {
        REQUIRE(classifyStaffDifference(hideKeySigs(epoch)) ==
                DifferenceClassification::PossiblyUnrecoverable);
    }

    for (const auto suffix :
         {"dw_rest_offset", "w_rest_offset", "h_rest_offset", "other_rest_offset", "stem_reversal"})
    {
        for (const auto epoch : {finale_mus_reader::FormatEpoch::CodaBanner,
                                 finale_mus_reader::FormatEpoch::UncompressedLegacy})
        {
            REQUIRE(classifyStaffDifference(context("staff[cmper=1]." + std::string(suffix),
                                                    "finale27-default", DifferenceCategory::Differs,
                                                    epoch)) ==
                    DifferenceClassification::PossiblyUnrecoverable);
        }
    }
    const Value zero{0};
    const Value two{2};
    const DifferenceContext fretInstrument{"staff[cmper=1].fret_inst_id",
                                           DifferenceCategory::Differs,
                                           "finale27-default",
                                           zero,
                                           two,
                                           leaves,
                                           leaves,
                                           finale_mus_reader::FormatEpoch::CodaBanner,
                                           finale_mus_reader::ByteOrder::BigEndian,
                                           nullptr,
                                           report};
    REQUIRE(classifyStaffDifference(fretInstrument) ==
            DifferenceClassification::PossiblyUnrecoverable);
    const DifferenceContext reverseFretInstrument{"staff[cmper=1].fret_inst_id",
                                                  DifferenceCategory::Differs,
                                                  "finale27-default",
                                                  two,
                                                  zero,
                                                  leaves,
                                                  leaves,
                                                  finale_mus_reader::FormatEpoch::CodaBanner,
                                                  finale_mus_reader::ByteOrder::BigEndian,
                                                  nullptr,
                                                  report};
    REQUIRE_FALSE(classifyStaffDifference(reverseFretInstrument));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].dw_rest_offset",
                                                  "legacy-behavior", DifferenceCategory::Differs,
                                                  finale_mus_reader::FormatEpoch::CodaBanner)));
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=1].dw_rest_offset", "finale27-default", DifferenceCategory::ReaderOnly,
                finale_mus_reader::FormatEpoch::CodaBanner)));
    REQUIRE_FALSE(classifyStaffDifference(context("measures[cmper=1].dw_rest_offset",
                                                  "finale27-default", DifferenceCategory::Differs,
                                                  finale_mus_reader::FormatEpoch::CodaBanner)));
}

TEST_CASE("Synthesized Staff fret instrument references may be renumbered", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using FretInstrument = musx::dom::others::FretInstrument;
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::DclLegacy);
    report.setInstanceOrigin(
        finale_mus_reader::instanceKey<FretInstrument>(musx::dom::SCORE_PARTID, 2),
        ValueOrigin::LegacyBehavior);
    const auto context = [&](std::string_view path, std::string_view origin,
                             DifferenceCategory category, const Value &source,
                             const Value &companion)
    {
        return DifferenceContext{path,
                                 category,
                                 origin,
                                 source,
                                 companion,
                                 leaves,
                                 leaves,
                                 finale_mus_reader::FormatEpoch::DclLegacy,
                                 finale_mus_reader::ByteOrder::BigEndian,
                                 nullptr,
                                 report};
    };

    REQUIRE(classifyStaffDifference(context("staff[cmper=1].fret_inst_id", "legacy-behavior",
                                            DifferenceCategory::Differs, Value{2}, Value{1})) ==
            DifferenceClassification::DifferentDefaults);
    REQUIRE_FALSE(
        classifyStaffDifference(context("staff[cmper=1].fret_inst_id", "legacy-mus",
                                        DifferenceCategory::Differs, Value{2}, Value{1})));
    REQUIRE_FALSE(
        classifyStaffDifference(context("staff[cmper=1].fret_inst_id", "legacy-behavior",
                                        DifferenceCategory::Differs, Value{3}, Value{1})));
    REQUIRE_FALSE(
        classifyStaffDifference(context("staff[cmper=1].fret_inst_id", "legacy-behavior",
                                        DifferenceCategory::ReaderOnly, Value{2}, Value{1})));
    REQUIRE_FALSE(
        classifyStaffDifference(context("staff[cmper=1].notation_style", "legacy-behavior",
                                        DifferenceCategory::Differs, Value{2}, Value{1})));
}

TEST_CASE("Defaulted Staff tablature line breaking may differ", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::DclLegacy);
    const auto context =
        [&](std::string_view path, std::string_view origin, DifferenceCategory category)
    {
        return DifferenceContext{path,
                                 category,
                                 origin,
                                 Value{false},
                                 Value{true},
                                 leaves,
                                 leaves,
                                 finale_mus_reader::FormatEpoch::DclLegacy,
                                 finale_mus_reader::ByteOrder::BigEndian,
                                 nullptr,
                                 report};
    };

    REQUIRE(classifyStaffDifference(context("staff[cmper=1].break_tab_lines_at_notes",
                                            "finale27-default", DifferenceCategory::Differs)) ==
            DifferenceClassification::DifferentDefaults);
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].break_tab_lines_at_notes",
                                                  "legacy-behavior", DifferenceCategory::Differs)));
    REQUIRE_FALSE(
        classifyStaffDifference(context("staff[cmper=1].break_tab_lines_at_notes",
                                        "finale27-default", DifferenceCategory::ReaderOnly)));
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=1].hide_beams", "finale27-default", DifferenceCategory::Differs)));
}

TEST_CASE("Only derived Staff styles are deferred", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value source{0};
    const Value companion{24};
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);
    const auto context = [&](std::string_view path, std::string_view origin,
                             DifferenceCategory category, finale_mus_reader::FormatEpoch epoch)
    {
        return DifferenceContext{path,    category,  origin,
                                 source,  companion, leaves,
                                 leaves,  epoch,     finale_mus_reader::ByteOrder::BigEndian,
                                 nullptr, report};
    };

    const auto codaUnmapped =
        context("staff[cmper=1].dw_rest_offset", "unmapped", DifferenceCategory::Differs,
                finale_mus_reader::FormatEpoch::CodaBanner);
    REQUIRE_FALSE(classifyStaffDifference(codaUnmapped));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].dw_rest_offset",
                                                  "legacy-behavior", DifferenceCategory::Differs,
                                                  finale_mus_reader::FormatEpoch::CodaBanner)));
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=1].dw_rest_offset", "unmapped", DifferenceCategory::Differs,
                finale_mus_reader::FormatEpoch::UncompressedLegacy)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].dw_rest_offset", "unmapped",
                                                  DifferenceCategory::ReaderOnly,
                                                  finale_mus_reader::FormatEpoch::CodaBanner)));
    REQUIRE_FALSE(classifyStaffDifference(context("measures[cmper=1].dw_rest_offset", "unmapped",
                                                  DifferenceCategory::Differs,
                                                  finale_mus_reader::FormatEpoch::CodaBanner)));

    const auto derivedStyles =
        context("staff[cmper=1].has_styles", "unmapped", DifferenceCategory::Differs,
                finale_mus_reader::FormatEpoch::ZlibLegacy);
    REQUIRE(classifyStaffDifference(derivedStyles) ==
            DifferenceClassification::AwaitsDependentRecovery);
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].has_styles", "legacy-mus",
                                                  DifferenceCategory::Differs,
                                                  finale_mus_reader::FormatEpoch::ZlibLegacy)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].line_space", "unmapped",
                                                  DifferenceCategory::Differs,
                                                  finale_mus_reader::FormatEpoch::ZlibLegacy)));

    setDeferredRecoveryClassified(false);
    CHECK_FALSE(classifyStaffDifference(derivedStyles));
    setDeferredRecoveryClassified(true);
    REQUIRE_FALSE(classifyStaffDifference(codaUnmapped));
    REQUIRE(classifyStaffDifference(derivedStyles) ==
            DifferenceClassification::AwaitsDependentRecovery);
}

TEST_CASE("Staff fields omitted from the companion Scroll View await StaffUsed "
          "recovery",
          "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value source{26};
    const Value companion{0};
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);
    const auto document = emptyStaffDocument();
    auto staffUsed = std::make_shared<musx::dom::others::StaffUsed>(
        document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All,
        document->calcScrollViewCmper(musx::dom::SCORE_PARTID), 0);
    staffUsed->staffId = 1;
    document->getOthers()->add(musx::dom::others::StaffUsed::XmlNodeName, std::move(staffUsed));
    const auto context =
        [&](std::string_view path, std::string_view origin, DifferenceCategory category)
    {
        return DifferenceContext{path,
                                 category,
                                 origin,
                                 source,
                                 companion,
                                 leaves,
                                 leaves,
                                 finale_mus_reader::FormatEpoch::ZlibLegacy,
                                 finale_mus_reader::ByteOrder::LittleEndian,
                                 nullptr,
                                 report,
                                 {},
                                 {},
                                 nullptr,
                                 nullptr,
                                 document.get()};
    };

    for (const auto suffix : {".full_name_text_id", ".abbrv_name_text_id", ".note_font.font_size",
                              ".transposition.present", ".transposition.keysig.present",
                              ".transposition.keysig.interval", ".transposition.keysig.adjust",
                              ".hide_meas_nums", ".hide_name_in_score"})
    {
        REQUIRE(classifyStaffDifference(context(std::string("staff[cmper=2]") + suffix,
                                                "legacy-mus", DifferenceCategory::Differs)) ==
                DifferenceClassification::AwaitsDependentRecovery);
        REQUIRE_FALSE(classifyStaffDifference(context(std::string("staff[cmper=1]") + suffix,
                                                      "legacy-mus", DifferenceCategory::Differs)));
    }
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=2].use_note_font", "legacy-mus", DifferenceCategory::Differs)));
    REQUIRE_FALSE(classifyStaffDifference(
        context("staff[cmper=2].note_font.font_size", "unmapped", DifferenceCategory::Differs)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=2].note_font.font_size",
                                                  "legacy-mus", DifferenceCategory::ReaderOnly)));
    REQUIRE(classifyStaffDifference(context("staff[part_id=3,cmper=2].full_name_text_id",
                                            "legacy-mus", DifferenceCategory::Differs)) ==
            DifferenceClassification::AwaitsDependentRecovery);
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=invalid].full_name_text_id",
                                                  "legacy-mus", DifferenceCategory::Differs)));

    setDeferredRecoveryClassified(false);
    CHECK_FALSE(classifyStaffDifference(
        context("staff[cmper=2].note_font.font_size", "legacy-mus", DifferenceCategory::Differs)));
    setDeferredRecoveryClassified(true);
}

} // namespace
} // namespace finale_mus_reader_tests
