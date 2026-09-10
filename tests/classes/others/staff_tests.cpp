// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

#include "coverage/classification_rules.h"
#include "coverage/comparison.h"
#include "coverage/comparison_text.h"
#include "coverage/registry.h"
#include "support/finale_version.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;
using Staff = musx::dom::others::Staff;

std::optional<finale_mus_reader::coverage::DifferenceClassification> classifyStaffDifference(
    const finale_mus_reader::coverage::DifferenceContext& context)
{
    const auto classify = finale_mus_reader::coverage::differenceClassifier("staff");
    return classify ? classify(context) : std::nullopt;
}

constexpr std::size_t staffFieldManifestSize = 101;
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

musx::dom::DocumentPtr emptyStaffDocument()
{
    auto session = musx::factory::DocumentFactory::begin();
    return session.getDocument();
}

ImportReport staffImport(const finale_mus_reader::container::ParsedContainer& parsed,
    const SourceProfile& profile, const musx::dom::DocumentPtr& document)
{
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto referenceDocument = referenceSession.getDocument();
    auto referenceStaff = std::make_shared<Staff>(
        referenceDocument, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, 1);
    referenceStaff->staffLines = 5;
    referenceStaff->lineSpace = 24;
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
    referenceFontOptions->fontOptions.emplace(
        musx::dom::options::FontOptions::FontType::Noteheads, std::move(referenceNoteheadFont));
    referenceDocument->getOptions()->add(
        musx::dom::options::FontOptions::XmlNodeName, std::move(referenceFontOptions));
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{
        index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importStaff(context);
    return report;
}

const FieldInfo& staffField(const ImportReport& report, std::string_view member)
{
    const auto* value =
        report.findField(finale_mus_reader::instanceKey<Staff>(musx::dom::SCORE_PARTID, 7), member);
    expect(value != nullptr, "Missing Staff report field " + std::string(member));
    return *value;
}

TEST_CASE("The Finale 2000 Staff base layout recovers its complete raw field "
          "surface")
{
    for (const auto epoch : {FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            const auto parsed = makeContainer(
                {{7, "IS", {-48, 0x0b17, static_cast<std::int16_t>(0xff12), 6, 0x1803, 0x69bd}},
                    {7, "IS",
                        {0x0302, 1, 4, 48, static_cast<std::int16_t>(0xcfbd),
                            static_cast<std::int16_t>(0xf5fb)}},
                    {7, "IS",
                        {static_cast<std::int16_t>(0xfafb), static_cast<std::int16_t>(0xfdfc), -4,
                            9, 10, 0}}},
                epoch, byteOrder);
            const auto document = emptyStaffDocument();
            auto profile = SourceProfile(epoch);
            if (epoch == FormatEpoch::UncompressedLegacy) {
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
            CHECK(staff->botBarlineOffset == -48);
            CHECK(staff->capoPos == 23);
            CHECK(staff->lowestFret == 11);
            CHECK(staff->topBarlineOffset == 48);
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
            CHECK(staff->lineSpace == 24);
            CHECK(staff->botRepeatDotOff == -5);
            CHECK(staff->topRepeatDotOff == -3);
            CHECK(staffField(report, "notationStyle").origin == ValueOrigin::LegacyMus);
            CHECK(staffField(report, "capoPos").origin == ValueOrigin::LegacyMus);
            CHECK(staffField(report, "lowestFret").origin == ValueOrigin::LegacyMus);
            CHECK(staffField(report, "hideLyrics").origin == ValueOrigin::LegacyMus);
            CHECK(staffField(report, "altHideOtherLyrics").origin == ValueOrigin::LegacyMus);
            CHECK(staffField(report, "lineSpace").origin == ValueOrigin::Finale27Default);
            CHECK(staffField(report, "botRepeatDotOff").origin == ValueOrigin::Finale27Default);
            CHECK(staffField(report, "topRepeatDotOff").origin == ValueOrigin::Finale27Default);
            CHECK(staffField(report, "redisplayLayerAccis").origin == ValueOrigin::LegacyBehavior);
            CHECK(staffField(report, "hideTimeSigsInParts").origin == ValueOrigin::LegacyBehavior);
            CHECK(staffField(report, "hideKeySigsShowAccis").origin == ValueOrigin::LegacyBehavior);
            CHECK(reportedFieldCount(report) == staffFieldManifestSize);
        }
    }
}

TEST_CASE("Finale 2000 through 2008 expand the note-attached-items setting")
{
    const auto baseline = readFixture("evidence/F2008/F2008-empty.mus");
    const auto baselineStaff =
        baseline.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(baselineStaff);
    CHECK_FALSE(baselineStaff->altHideExpressions);

    for (const auto* path : {
             "evidence/F2000/F2000-staff-unshownoteitems.mus",
             "evidence/F2006/F2006-staff-nonoteitems.mus",
             "evidence/F2008/F2008-staff-nohidenoteitems.mus",
         }) {
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

        for (const auto* member : {"altHideArtics", "altHideLyrics", "altHideSmartShapes",
                 "altHideExpressions", "hideFretboards", "hideChords"}) {
            const auto* field = result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1);
            REQUIRE(field);
            CHECK(field->origin == ValueOrigin::LegacyMus);
        }
        const auto* hasStyles =
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
        const char* path;
        bool hideOtherNotes;
        bool hideOtherItems;
    };
    for (const auto expected : {
             Expected{"evidence/F2000/F2000-staff-hideothernotes.mus", true, false},
             Expected{"evidence/F2000/F2000-staff-hideotheritems.mus", false, true},
         }) {
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

        for (const auto member : staffAlternateNotationFields) {
            const auto* field = result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1);
            REQUIRE(field);
            CHECK(field->origin == ValueOrigin::LegacyMus);
        }
    }
}

TEST_CASE("Pre-Finale-2000 Staff alternate notation is legacy "
          "behavior")
{
    for (const auto* path : {
             "evidence/F98/F98-baseline.mus",
             "evidence/F98/F98-altnotation-full.mus",
             "evidence/F98/F98-altnotation-partial.mus",
         }) {
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

        for (const auto member : staffAlternateNotationFields) {
            const auto* field = result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1);
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

    for (const auto* member : {
             "useNoteFont",
             "noteFont.fontId",
             "noteFont.fontSize",
         }) {
        const auto* field = result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1);
        REQUIRE(field);
        CHECK(field->origin == ValueOrigin::LegacyMus);
    }
}

TEST_CASE("The zlib Staff class retains the base words and its established "
          "extension")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        std::vector<std::int16_t> words{0, 0, 0x0700, 0, 0, 0, 0, 0, 5, 0, 0, 0,
            static_cast<std::int16_t>(0xfcfc), static_cast<std::int16_t>(0xfcfc), -4, 0, 0,
            static_cast<std::int16_t>(0xfdfb)};
        const auto appendLong = [&](std::int32_t value) {
            const auto bits = static_cast<std::uint32_t>(value);
            const auto high = static_cast<std::int16_t>(bits >> 16U);
            const auto low = static_cast<std::int16_t>(bits & 0xffffU);
            if (byteOrder == ByteOrder::BigEndian) {
                words.insert(words.end(), {high, low});
            } else {
                words.insert(words.end(), {low, high});
            }
        };
        if (byteOrder == ByteOrder::BigEndian) {
            words.insert(words.end(), {0, 1536, -1, -1024});
        } else {
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
        CHECK(staff->lineSpace == 24);
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
        const char* fixture;
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

    for (const auto& expected : cases) {
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

        const auto* capoSource =
            result.report.findField<Staff>("capoPos", musx::dom::SCORE_PARTID, 1);
        const auto* lowestFretSource =
            result.report.findField<Staff>("lowestFret", musx::dom::SCORE_PARTID, 1);
        REQUIRE(capoSource);
        REQUIRE(lowestFretSource);
        CHECK(capoSource->origin == ValueOrigin::LegacyMus);
        CHECK(lowestFretSource->origin == ValueOrigin::LegacyMus);
    }
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
    CHECK(staff->lineSpace == 24);
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
    CHECK(staffField(report, "botRepeatDotOff").origin == ValueOrigin::Finale27Default);
    CHECK(staffField(report, "topRepeatDotOff").origin == ValueOrigin::Finale27Default);
    CHECK(staffField(report, "defaultClef").origin == ValueOrigin::LegacyMus);
    CHECK(staffField(report, "hideTimeSigs").origin == ValueOrigin::LegacyMus);
    CHECK(staffField(report, "notationStyle").origin == ValueOrigin::Unmapped);
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
    CHECK(staff->lineSpace == 24);
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

    const auto field = [&](const char* member) {
        return result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1);
    };
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

TEST_CASE("Finale 2.6.3 optional Staff attributes recover custom lines and "
          "notehead font")
{
    const auto result = readFixture("evidence/F263/F263-staffopts.mus");
    const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(staff);
    CHECK_FALSE(staff->staffLines);
    REQUIRE(staff->customStaff);
    CHECK(*staff->customStaff == std::vector<int>{13});
    CHECK(staff->botBarlineOffset == -48);
    CHECK(staff->topBarlineOffset == 48);
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
    const auto fullNameBlock = result.document->getOthers()->get<musx::dom::others::TextBlock>(
        musx::dom::SCORE_PARTID, staff->fullNameTextId);
    const auto abbreviatedNameBlock =
        result.document->getOthers()->get<musx::dom::others::TextBlock>(
            musx::dom::SCORE_PARTID, staff->abbrvNameTextId);
    REQUIRE(fullNameBlock);
    REQUIRE(abbreviatedNameBlock);
    CHECK(fullNameBlock->textId != 0);
    CHECK(abbreviatedNameBlock->textId != 0);
    CHECK(fullNameBlock->textId != abbreviatedNameBlock->textId);

    const auto field = [&](const char* member) {
        return result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, 1);
    };
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
    CHECK(field("fretInstId")->origin == ValueOrigin::LegacyMus);
    CHECK(field("showTabClefAllSys")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("hideRests")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("hideDots")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("hideStems")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("hideTuplets")->origin == ValueOrigin::LegacyBehavior);
    CHECK(field("noteFont.fontSize")->origin == ValueOrigin::LegacyMus);
    CHECK(field("hideTimeSigs")->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("The Finale 2012 Staff extension recovers its stored instrument UUID")
{
    constexpr std::array<std::uint8_t, 16> uuidBytes{0xa9, 0x25, 0x64, 0x8a, 0xab, 0xc9, 0x4d, 0xc7,
        0xa6, 0x19, 0xa6, 0xce, 0x35, 0x5a, 0xd3, 0x3c};
    std::vector<std::int16_t> words(40, 0);
    for (std::size_t index = 0; index < uuidBytes.size(); index += 2) {
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

    const auto* earlierSource =
        finale2011.report.findField<Staff>("hideStaffLines", musx::dom::SCORE_PARTID, 1);
    const auto* baselineSource =
        baseline.report.findField<Staff>("hideStaffLines", musx::dom::SCORE_PARTID, 1);
    const auto* hiddenSource =
        hidden.report.findField<Staff>("hideStaffLines", musx::dom::SCORE_PARTID, 1);
    REQUIRE(earlierSource);
    REQUIRE(baselineSource);
    REQUIRE(hiddenSource);
    CHECK(earlierSource->origin == ValueOrigin::Unmapped);
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

    const auto* earlierStyle =
        finale2011.report.findField<Staff>("autoNumbering", musx::dom::SCORE_PARTID, 1);
    const auto* earlierUuid =
        finale2011.report.findField<Staff>("instUuid", musx::dom::SCORE_PARTID, 1);
    const auto* earlierEnabled =
        finale2011.report.findField<Staff>("useAutoNumbering", musx::dom::SCORE_PARTID, 1);
    const auto* baselineStyle =
        baseline.report.findField<Staff>("autoNumbering", musx::dom::SCORE_PARTID, 1);
    const auto* baselineEnabled =
        baseline.report.findField<Staff>("useAutoNumbering", musx::dom::SCORE_PARTID, 1);
    const auto* numberedStyle =
        numbered.report.findField<Staff>("autoNumbering", musx::dom::SCORE_PARTID, 1);
    const auto* numberedEnabled =
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

    const auto makeDocument = [](musx::dom::Cmper blockId, musx::dom::Cmper textId,
                                  std::string text) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto raw = std::make_shared<BlockText>(
            document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, textId);
        raw->text = std::move(text);
        document->getTexts()->add(BlockText::XmlNodeName, std::move(raw));
        auto block = std::make_shared<TextBlock>(
            document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, blockId);
        block->textId = textId;
        document->getOthers()->add(TextBlock::XmlNodeName, std::move(block));
        return document;
    };

    const auto source = makeDocument(7, 3, "Horn");
    const auto equivalent = makeDocument(19, 41, "Horn");
    const auto different = makeDocument(7, 3, "Trumpet");
    REQUIRE(comparison_text::compareStaffNameReferents(
                "staff[cmper=1].full_name_text_id", 7, 19, source, equivalent) == true);
    REQUIRE(comparison_text::compareStaffNameReferents(
                "staff[cmper=1].abbrv_name_text_id", 7, 7, source, different) == false);
    REQUIRE_FALSE(comparison_text::compareStaffNameReferents(
        "staff[cmper=1].full_name_text_id", 0, 19, source, equivalent));
    REQUIRE_FALSE(comparison_text::compareStaffNameReferents(
        "staff[cmper=1].abbrv_name_text_id", 7, 0, source, equivalent));
    REQUIRE_FALSE(comparison_text::compareStaffNameReferents(
        "staff[cmper=1].default_clef", 7, 19, source, equivalent));

    const auto snapshot = [](std::int64_t fullName, std::int64_t abbreviatedName) {
        return SurveySnapshot{
            {"staff", Value::Array{Value::Object{{"cmper", 1}, {"full_name_text_id", fullName},
                          {"abbrv_name_text_id", abbreviatedName}}}}};
    };
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);
    const auto zeroMismatch = compareSnapshots(snapshot(7, 0), snapshot(19, 7), source, equivalent,
        finale_mus_reader::FormatEpoch::CodaBanner, finale_mus_reader::ByteOrder::BigEndian,
        nullptr, report);
    const auto& zeroStats = zeroMismatch.classes.at("others").at("staff");
    CHECK(zeroStats.same == 1);
    CHECK(zeroStats.unexpected == 1);
    CHECK(zeroMismatch.transformations.at(ComparisonTransformation::EquivalentTextBlockReferent) ==
          1);

    const auto equalComparatorDifferentText = compareSnapshots(snapshot(7, 0), snapshot(7, 0),
        source, different, finale_mus_reader::FormatEpoch::CodaBanner,
        finale_mus_reader::ByteOrder::BigEndian, nullptr, report);
    const auto& differentStats = equalComparatorDifferentText.classes.at("others").at("staff");
    CHECK(differentStats.same == 1);
    CHECK(differentStats.unexpected == 1);
}

TEST_CASE("Disabled pre-Finale 2001 Staff note-font sizes have different defaults", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value zero{0};
    const Value one{1};
    const Value twentyThree{23};
    const Value twentyFour{24};
    const ComparisonLeaves disabled{{"staff[cmper=1].use_note_font", {Value(false), "legacy-mus"}}};
    const ComparisonLeaves enabled{{"staff[cmper=1].use_note_font", {Value(true), "legacy-mus"}}};
    const ComparisonLeaves none;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::UncompressedLegacy);
    const finale_mus_reader::SourceVersion finale2000{
        .major = finale_mus_reader::versions::finale2000.major};
    const finale_mus_reader::SourceVersion finale2001{
        .major = finale_mus_reader::versions::finale2001.major};
    const auto context = [&](const ComparisonLeaves& leaves, const Value& source,
                             const Value& companion, std::string_view path, std::string_view origin,
                             DifferenceCategory category, finale_mus_reader::FormatEpoch epoch,
                             const finale_mus_reader::SourceVersion* version) {
        return DifferenceContext{path, category, origin, source, companion, leaves, none, epoch,
            finale_mus_reader::ByteOrder::BigEndian, version, report};
    };

    REQUIRE(classifyStaffDifference(context(disabled, zero, twentyFour,
                "staff[cmper=1].note_font.font_size", "legacy-mus", DifferenceCategory::Differs,
                finale_mus_reader::FormatEpoch::UncompressedLegacy, &finale2000)) ==
            DifferenceClassification::DifferentDefaults);
    REQUIRE_FALSE(classifyStaffDifference(context(enabled, zero, twentyFour,
        "staff[cmper=1].note_font.font_size", "legacy-mus", DifferenceCategory::Differs,
        finale_mus_reader::FormatEpoch::UncompressedLegacy, &finale2000)));
    REQUIRE_FALSE(classifyStaffDifference(context(disabled, one, twentyFour,
        "staff[cmper=1].note_font.font_size", "legacy-mus", DifferenceCategory::Differs,
        finale_mus_reader::FormatEpoch::UncompressedLegacy, &finale2000)));
    REQUIRE_FALSE(classifyStaffDifference(context(disabled, zero, twentyThree,
        "staff[cmper=1].note_font.font_size", "legacy-mus", DifferenceCategory::Differs,
        finale_mus_reader::FormatEpoch::UncompressedLegacy, &finale2000)));
    REQUIRE_FALSE(classifyStaffDifference(
        context(disabled, zero, twentyFour, "staff[cmper=1].note_font.font_size", "legacy-mus",
            DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::DclLegacy, &finale2001)));
    REQUIRE_FALSE(classifyStaffDifference(context(disabled, zero, twentyFour,
        "staff[cmper=1].note_font.font_size", "unmapped", DifferenceCategory::Differs,
        finale_mus_reader::FormatEpoch::UncompressedLegacy, &finale2000)));
    REQUIRE_FALSE(classifyStaffDifference(context(disabled, zero, twentyFour,
        "staff[cmper=1].note_font.font_size", "legacy-mus", DifferenceCategory::ReaderOnly,
        finale_mus_reader::FormatEpoch::UncompressedLegacy, &finale2000)));
}

TEST_CASE("Through Finale 2008 percussion and tablature note-font enablement "
          "is upgrade loss",
    "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using NotationStyle = musx::dom::others::Staff::NotationStyle;
    const Value no(false);
    const Value yes(true);
    const finale_mus_reader::SourceVersion finale2008{
        .major = finale_mus_reader::versions::finale2008.major};
    const finale_mus_reader::SourceVersion finale2009{
        .major = finale_mus_reader::versions::finale2009.major};
    const finale_mus_reader::SourceVersion finale97{
        .major = finale_mus_reader::versions::finale97.major};
    const finale_mus_reader::SourceVersion finale2005{
        .major = finale_mus_reader::versions::finale2005.major};
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);
    const ComparisonLeaves percussion{{"staff[cmper=1].notation_style",
        {Value(static_cast<std::int64_t>(NotationStyle::Percussion)), "legacy-mus"}}};
    const ComparisonLeaves tablature{{"staff[cmper=1].notation_style",
        {Value(static_cast<std::int64_t>(NotationStyle::Tablature)), "legacy-mus"}}};
    const ComparisonLeaves standard{{"staff[cmper=1].notation_style",
        {Value(static_cast<std::int64_t>(NotationStyle::Standard)), "legacy-mus"}}};
    const ComparisonLeaves none;
    const auto context = [&](const ComparisonLeaves& leaves, const Value& source,
                             const Value& companion, std::string_view origin,
                             DifferenceCategory category, finale_mus_reader::FormatEpoch epoch,
                             const finale_mus_reader::SourceVersion* version) {
        return DifferenceContext{"staff[cmper=1].use_note_font", category, origin, source,
            companion, leaves, none, epoch, finale_mus_reader::ByteOrder::BigEndian, version,
            report};
    };

    REQUIRE(classifyStaffDifference(context(percussion, no, yes, "legacy-mus",
                DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::ZlibLegacy,
                &finale2008)) == DifferenceClassification::FinaleUpgradeLoss);
    REQUIRE(classifyStaffDifference(context(percussion, no, yes, "legacy-mus",
                DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::UncompressedLegacy,
                &finale97)) == DifferenceClassification::FinaleUpgradeLoss);
    REQUIRE(classifyStaffDifference(context(tablature, no, yes, "legacy-mus",
                DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::DclLegacy,
                &finale2005)) == DifferenceClassification::FinaleUpgradeLoss);
    REQUIRE_FALSE(classifyStaffDifference(context(standard, no, yes, "legacy-mus",
        DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2008)));
    REQUIRE_FALSE(classifyStaffDifference(context(percussion, yes, no, "legacy-mus",
        DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2008)));
    REQUIRE_FALSE(classifyStaffDifference(context(percussion, no, yes, "legacy-behavior",
        DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2008)));
    REQUIRE_FALSE(classifyStaffDifference(context(percussion, no, yes, "legacy-mus",
        DifferenceCategory::ReaderOnly, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2008)));
    REQUIRE_FALSE(classifyStaffDifference(context(percussion, no, yes, "legacy-mus",
        DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::ZlibLegacy, &finale2009)));
}

TEST_CASE("Coverage excludes the Studio View Staff", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using Staff = musx::dom::others::Staff;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto addStaff = [&](musx::dom::Cmper cmper) {
        auto staff = std::make_shared<Staff>(
            document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
        document->getOthers()->add(Staff::XmlNodeName, std::move(staff));
    };
    addStaff(1);
    addStaff(musx::dom::STUDIO_VIEW_STAFF_ID);

    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::DclLegacy);
    const auto observed = runAllSurveyors({document, report});
    const auto& staffs = observed.snapshot.at("staff").asArray();
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
                             const finale_mus_reader::SourceVersion* version) {
        return DifferenceContext{path, category, origin, source, companion, leaves, leaves,
            finale_mus_reader::FormatEpoch::ZlibLegacy, finale_mus_reader::ByteOrder::LittleEndian,
            version, report};
    };

    REQUIRE(classifyStaffDifference(context("staff[cmper=1].inst_uuid", "legacy-behavior",
                DifferenceCategory::Differs, &finale2011)) ==
            DifferenceClassification::DifferentDefaults);
    const DifferenceContext coda{"staff[cmper=1].inst_uuid", DifferenceCategory::Differs,
        "legacy-behavior", source, companion, leaves, leaves,
        finale_mus_reader::FormatEpoch::CodaBanner, finale_mus_reader::ByteOrder::BigEndian,
        nullptr, report};
    REQUIRE(classifyStaffDifference(coda) == DifferenceClassification::DifferentDefaults);
    REQUIRE_FALSE(classifyStaffDifference(context(
        "staff[cmper=1].inst_uuid", "legacy-mus", DifferenceCategory::Differs, &finale2011)));
    REQUIRE_FALSE(classifyStaffDifference(context(
        "staff[cmper=1].inst_uuid", "legacy-behavior", DifferenceCategory::Differs, &finale2012)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].inst_uuid", "legacy-behavior",
        DifferenceCategory::ReaderOnly, &finale2011)));
    REQUIRE_FALSE(classifyStaffDifference(context(
        "staff[cmper=1].line_space", "legacy-behavior", DifferenceCategory::Differs, &finale2011)));
}

TEST_CASE("Only derived Staff styles are deferred", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value source{0};
    const Value companion{24};
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);
    const auto context = [&](std::string_view path, std::string_view origin,
                             DifferenceCategory category, finale_mus_reader::FormatEpoch epoch) {
        return DifferenceContext{path, category, origin, source, companion, leaves, leaves, epoch,
            finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
    };

    const auto codaUnmapped = context("staff[cmper=1].dw_rest_offset", "unmapped",
        DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::CodaBanner);
    REQUIRE_FALSE(classifyStaffDifference(codaUnmapped));
    REQUIRE_FALSE(
        classifyStaffDifference(context("staff[cmper=1].dw_rest_offset", "legacy-behavior",
            DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::CodaBanner)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].dw_rest_offset", "unmapped",
        DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::UncompressedLegacy)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].dw_rest_offset", "unmapped",
        DifferenceCategory::ReaderOnly, finale_mus_reader::FormatEpoch::CodaBanner)));
    REQUIRE_FALSE(classifyStaffDifference(context("measures[cmper=1].dw_rest_offset", "unmapped",
        DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::CodaBanner)));

    const auto derivedStyles = context("staff[cmper=1].has_styles", "unmapped",
        DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::ZlibLegacy);
    REQUIRE(classifyStaffDifference(derivedStyles) ==
            DifferenceClassification::AwaitsDependentRecovery);
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].has_styles", "legacy-mus",
        DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::ZlibLegacy)));
    REQUIRE_FALSE(classifyStaffDifference(context("staff[cmper=1].line_space", "unmapped",
        DifferenceCategory::Differs, finale_mus_reader::FormatEpoch::ZlibLegacy)));

    setDeferredRecoveryClassified(false);
    CHECK_FALSE(classifyStaffDifference(derivedStyles));
    setDeferredRecoveryClassified(true);
    REQUIRE_FALSE(classifyStaffDifference(codaUnmapped));
    REQUIRE(classifyStaffDifference(derivedStyles) ==
            DifferenceClassification::AwaitsDependentRecovery);
}

} // namespace
} // namespace finale_mus_reader_tests
