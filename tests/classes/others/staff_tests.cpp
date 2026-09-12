// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

#include "coverage/classification_rules.h"
#include "coverage/comparison.h"
#include "coverage/comparison_text.h"
#include "coverage/registry.h"
#include "coverage/surveyors/shared/staff_style_semantics.h"
#include "support/finale_version.h"

namespace finale_mus_reader_tests
{
namespace
{

using namespace classes;
using Staff = musx::dom::others::Staff;

std::optional<finale_mus_reader::coverage::DifferenceClassification> classifyStaffDifference(
    const finale_mus_reader::coverage::DifferenceContext &context)
{
    const auto classify = finale_mus_reader::coverage::differenceClassifier("staff");
    return classify ? classify(context) : std::nullopt;
}

constexpr std::size_t staffFieldManifestSize = 101;
constexpr int evpusPerSpace = static_cast<int>(musx::dom::EVPU_PER_SPACE);
constexpr musx::dom::Cmper staffCmper1{1};
constexpr musx::dom::Cmper staffCmper7{7};
constexpr musx::dom::Cmper staffCmper9{9};
constexpr musx::dom::Cmper staffCmper10{10};
constexpr musx::dom::Cmper staffCmper11{11};
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
                         const SourceProfile &profile, const musx::dom::DocumentPtr &document,
                         bool styles = false)
{
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto referenceDocument = referenceSession.getDocument();
    auto referenceStaff =
        std::make_shared<Staff>(referenceDocument, musx::dom::SCORE_PARTID,
                                musx::dom::EnigmaBase::ShareMode::All, musx::dom::Cmper{1});
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
    if (styles)
    {
        finale_mus_reader::others::importStaffStyles(context);
    }
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
            CHECK(staffField(report, "transposition.chromatic.alteration").origin ==
                  ValueOrigin::LegacyMus);
            CHECK(staffField(report, "transposition.chromatic.diatonic").origin ==
                  ValueOrigin::LegacyMus);
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
            result.report.findField<Staff>("botRepeatDotOff", musx::dom::SCORE_PARTID, staffCmper1);
        const auto *topDot =
            result.report.findField<Staff>("topRepeatDotOff", musx::dom::SCORE_PARTID, staffCmper1);
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
    const auto report = staffImport(parsed, profile, document);

    const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);
    REQUIRE(staff);
    CHECK(staff->altNotation == Staff::AlternateNotation::Blank);
    CHECK(staff->altHideSmartShapes);
    CHECK(staffField(report, "altHideSmartShapes").origin == ValueOrigin::LegacyMusAdjusted);
}

TEST_CASE("Finale 2000 through 2008 combine Smart Shape hiding sources")
{
    struct Expected
    {
        Staff::AlternateNotation notation;
        bool hideSmartShapes;
    };
    for (const auto expected : {
             Expected{Staff::AlternateNotation::Normal, false},
             Expected{Staff::AlternateNotation::SlashBeats, true},
             Expected{Staff::AlternateNotation::Rhythmic, false},
             Expected{Staff::AlternateNotation::OneBarRepeat, true},
             Expected{Staff::AlternateNotation::TwoBarRepeat, true},
             Expected{Staff::AlternateNotation::BlankWithRests, true},
             Expected{Staff::AlternateNotation::Blank, true},
         })
    {
        const auto altFlags = std::int16_t(0x0100 | static_cast<std::int16_t>(expected.notation));
        const auto parsed = makeContainer({{7, "IS", {0, 0, altFlags, 0, 0, 0}},
                                           {7, "IS", {0, 0, 0, 0, 0, 0}},
                                           {7, "IS", {0, 0, 0, 0, 0, 0}}},
                                          FormatEpoch::UncompressedLegacy);
        const auto document = emptyStaffDocument();
        auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
        profile.version = SourceVersion{.major = finale_mus_reader::versions::finale2000.major};
        profile.byteOrder = ByteOrder::BigEndian;
        const auto report = staffImport(parsed, profile, document);
        const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);

        REQUIRE(staff);
        CHECK(staff->altHideSmartShapes == expected.hideSmartShapes);
        CHECK(staffField(report, "altHideSmartShapes").origin == ValueOrigin::LegacyMusAdjusted);
    }
}

TEST_CASE("Finale 2009 directly recovers alternate-notation Smart Shape hiding")
{
    std::vector<std::int16_t> words(42, 0);
    words[2] = std::int16_t(0x0401);
    const auto parsed = makeClassContainer(0x00e7, words, ByteOrder::LittleEndian, 7);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.version = SourceVersion{.major = finale_mus_reader::versions::finale2009.major};
    profile.byteOrder = ByteOrder::LittleEndian;
    const auto report = staffImport(parsed, profile, document);
    const auto staff = document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, 7);

    REQUIRE(staff);
    CHECK(staff->altNotation == Staff::AlternateNotation::SlashBeats);
    CHECK_FALSE(staff->altHideSmartShapes);
    CHECK(staffField(report, "altHideSmartShapes").origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Finale 2000 through 2008 expand the note-attached-items setting "
          "without expressions")
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
        CHECK_FALSE(staff->altHideExpressions);
        CHECK(staff->hideFretboards);
        CHECK(staff->hideChords);
        CHECK_FALSE(staff->hasStyles);

        for (const auto *member :
             {"altHideArtics", "altHideLyrics", "hideFretboards", "hideChords"})
        {
            const auto *field =
                result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, staffCmper1);
            REQUIRE(field);
            CHECK(field->origin == ValueOrigin::LegacyMus);
        }
        CHECK(staffField(result.report, "altHideSmartShapes", staffCmper1).origin ==
              ValueOrigin::LegacyMusAdjusted);
        CHECK(staffField(result.report, "altHideExpressions", staffCmper1).origin ==
              ValueOrigin::Finale27Default);
        const auto *hasStyles =
            result.report.findField<Staff>("hasStyles", musx::dom::SCORE_PARTID, staffCmper1);
        REQUIRE(hasStyles);
        CHECK(hasStyles->origin == ValueOrigin::LegacyMusAdjusted);
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
            const auto *field =
                result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, staffCmper1);
            REQUIRE(field);
            CHECK(field->origin == (member == "altHideSmartShapes" ? ValueOrigin::LegacyMusAdjusted
                                    : member == "altHideExpressions" ? ValueOrigin::Finale27Default
                                                                     : ValueOrigin::LegacyMus));
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
            const auto *field =
                result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, staffCmper1);
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
        const auto *field =
            result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, staffCmper1);
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
        const auto appendLong = [&](std::int32_t value) {
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
            result.report.findField<Staff>("capoPos", musx::dom::SCORE_PARTID, staffCmper1);
        const auto *lowestFretSource =
            result.report.findField<Staff>("lowestFret", musx::dom::SCORE_PARTID, staffCmper1);
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

    const auto importAt = [&](finale_mus_reader::VersionBound version) {
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
    const auto *fretSource =
        report.findField<Staff>("fretInstId", musx::dom::SCORE_PARTID, staffCmper10);
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
    CHECK(standardStaff->notationStyle == Staff::NotationStyle::Standard);
    CHECK(staffField(report, "notationStyle").origin == ValueOrigin::LegacyMusAdjusted);
    CHECK(staffField(report, "useNoteShapes").origin == ValueOrigin::LegacyMusAdjusted);
    const auto *tabFontSource =
        report.findField<Staff>("useNoteFont", musx::dom::SCORE_PARTID, staffCmper9);
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
        report.findField<Staff>("topBarlineOffset", musx::dom::SCORE_PARTID, staffCmper11);
    REQUIRE(topBarline);
    CHECK(topBarline->origin == ValueOrigin::LegacyMusAdjusted);
    for (const auto staffId : {musx::dom::Cmper{7}, musx::dom::Cmper{8}, musx::dom::Cmper{9}})
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
    const auto *fullName =
        report.findField<Staff>("fullNameTextId", musx::dom::SCORE_PARTID, staffCmper7);
    const auto *abbreviatedName =
        report.findField<Staff>("abbrvNameTextId", musx::dom::SCORE_PARTID, staffCmper7);
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

    const auto field = [&](const char *member) {
        return result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, staffCmper1);
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
        result.report.findField<Staff>("transposedClef", musx::dom::SCORE_PARTID, staffCmper1);
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

    const auto field = [&](const char *member) {
        return result.report.findField<Staff>(member, musx::dom::SCORE_PARTID, staffCmper1);
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
    const auto checkDefaultNoteheadFontSize = [](const auto &result, const auto &staff) {
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
        finale2011.report.findField<Staff>("hideStaffLines", musx::dom::SCORE_PARTID, staffCmper1);
    const auto *baselineSource =
        baseline.report.findField<Staff>("hideStaffLines", musx::dom::SCORE_PARTID, staffCmper1);
    const auto *hiddenSource =
        hidden.report.findField<Staff>("hideStaffLines", musx::dom::SCORE_PARTID, staffCmper1);
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
        finale2011.report.findField<Staff>("autoNumbering", musx::dom::SCORE_PARTID, staffCmper1);
    const auto *earlierUuid =
        finale2011.report.findField<Staff>("instUuid", musx::dom::SCORE_PARTID, staffCmper1);
    const auto *earlierEnabled = finale2011.report.findField<Staff>(
        "useAutoNumbering", musx::dom::SCORE_PARTID, staffCmper1);
    const auto *baselineStyle =
        baseline.report.findField<Staff>("autoNumbering", musx::dom::SCORE_PARTID, staffCmper1);
    const auto *baselineEnabled =
        baseline.report.findField<Staff>("useAutoNumbering", musx::dom::SCORE_PARTID, staffCmper1);
    const auto *numberedStyle =
        numbered.report.findField<Staff>("autoNumbering", musx::dom::SCORE_PARTID, staffCmper1);
    const auto *numberedEnabled =
        numbered.report.findField<Staff>("useAutoNumbering", musx::dom::SCORE_PARTID, staffCmper1);
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
    REQUIRE(comparison_text::compareStaffNameReferents("staff_style[cmper=1].full_name_text_id", 7,
                                                       19, source, equivalent) == true);
    REQUIRE(comparison_text::compareStaffNameReferents("staff[cmper=1].abbrv_name_text_id", 7, 7,
                                                       source, different) == false);
    REQUIRE(comparison_text::compareStaffNameReferents("staff_style[cmper=1].abbrv_name_text_id", 7,
                                                       7, source, different) == false);
    REQUIRE(comparison_text::compareStaffNameReferents("staff[cmper=1].full_name_text_id", 0, 29,
                                                       source, dangling) == true);
    REQUIRE(comparison_text::compareStaffNameReferents("staff[cmper=1].abbrv_name_text_id", 23, 0,
                                                       formattingOnly, equivalent) == true);
    REQUIRE(comparison_text::compareStaffNameReferents("staff[cmper=1].full_name_text_id", 0, 19,
                                                       source, equivalent) == false);
    REQUIRE_FALSE(comparison_text::compareStaffNameReferents("staff[cmper=1].default_clef", 7, 19,
                                                             source, equivalent));

    const auto snapshot = [](std::int64_t fullName, std::int64_t abbreviatedName) {
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
                             DifferenceCategory category = DifferenceCategory::Differs) {
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
                             const finale_mus_reader::SourceVersion *version) {
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
    const auto addStaff = [&](musx::dom::Cmper cmper) {
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
                             const finale_mus_reader::SourceVersion *version) {
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
                             const finale_mus_reader::SourceVersion *version) {
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
                             const finale_mus_reader::SourceVersion *version) {
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
                             DifferenceCategory category, finale_mus_reader::FormatEpoch epoch) {
        return DifferenceContext{path,    category,  origin,
                                 source,  companion, leaves,
                                 leaves,  epoch,     finale_mus_reader::ByteOrder::BigEndian,
                                 nullptr, report};
    };

    const Value shown{true};
    const Value hidden{false};
    const auto hideKeySigs = [&](finale_mus_reader::FormatEpoch epoch) {
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
    report.setField(finale_mus_reader::instanceKey<Staff>(musx::dom::SCORE_PARTID, staffCmper1),
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
    report.setInstanceOrigin(finale_mus_reader::instanceKey<FretInstrument>(musx::dom::SCORE_PARTID,
                                                                            musx::dom::Cmper{2}),
                             ValueOrigin::LegacyBehavior);
    const auto context = [&](std::string_view path, std::string_view origin,
                             DifferenceCategory category, const Value &source,
                             const Value &companion) {
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
    const auto context = [&](std::string_view path, std::string_view origin,
                             DifferenceCategory category) {
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
                             DifferenceCategory category, finale_mus_reader::FormatEpoch epoch) {
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
    const Value noStyles{false};
    const Value hasStyles{true};
    const finale_mus_reader::SourceVersion finale98{
        .major = finale_mus_reader::versions::finale98.major};
    const DifferenceContext preFinale2000HasStyles{
        "staff[cmper=1].has_styles",
        DifferenceCategory::Differs,
        "legacy-mus-adjusted",
        noStyles,
        hasStyles,
        leaves,
        leaves,
        finale_mus_reader::FormatEpoch::UncompressedLegacy,
        finale_mus_reader::ByteOrder::BigEndian,
        &finale98,
        report};
    REQUIRE(classifyStaffDifference(preFinale2000HasStyles) ==
            DifferenceClassification::AwaitsDependentRecovery);
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

TEST_CASE("Post-Finale-2000 verified assignment-source absence explains Staff hasStyles",
          "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value disabled(false);
    const Value enabled(true);
    const ComparisonLeaves leaves;
    const SourceVersion finale2000{.major = finale_mus_reader::versions::finale2000.major};
    const SourceVersion finale2006{.major = finale_mus_reader::versions::finale2006.major};
    const auto context = [&](const ImportReport& report, FormatEpoch epoch,
                             const SourceVersion& version,
                             std::string_view path = "staff[cmper=1].has_styles") {
        return DifferenceContext{path,
                                 DifferenceCategory::Differs,
                                 "legacy-mus-adjusted",
                                 disabled,
                                 enabled,
                                 leaves,
                                 leaves,
                                 epoch,
                                 ByteOrder::BigEndian,
                                 &version,
                                 report};
    };

    ImportReport incompleteAudit(FormatEpoch::DclLegacy);
    CHECK_FALSE(classifyStaffDifference(
        context(incompleteAudit, FormatEpoch::DclLegacy, finale2006)));

    ImportReport verifiedAbsent(FormatEpoch::DclLegacy);
    verifiedAbsent.staffStyleAssignmentAuditComplete = true;
    REQUIRE(classifyStaffDifference(context(verifiedAbsent, FormatEpoch::DclLegacy, finale2006)) ==
            DifferenceClassification::FinaleUpgradeSynthesis);

    ImportReport sourcePresent(FormatEpoch::DclLegacy);
    sourcePresent.expectStaffStyleAssignments(0, 1, 1, false);
    sourcePresent.staffStyleAssignmentAuditComplete = true;
    CHECK_FALSE(classifyStaffDifference(
        context(sourcePresent, FormatEpoch::DclLegacy, finale2006)));

    ImportReport malformedSource(FormatEpoch::DclLegacy);
    malformedSource.expectStaffStyleAssignments(0, 1, 0, true);
    malformedSource.staffStyleAssignmentAuditComplete = true;
    CHECK_FALSE(classifyStaffDifference(
        context(malformedSource, FormatEpoch::DclLegacy, finale2006)));

    ImportReport partSource(FormatEpoch::DclLegacy);
    partSource.expectStaffStyleAssignments(3, 1, 1, false);
    partSource.staffStyleAssignmentAuditComplete = true;
    CHECK_FALSE(classifyStaffDifference(context(partSource, FormatEpoch::DclLegacy, finale2006,
        "staff[part_id=3,cmper=1].has_styles")));

    CHECK(classifyStaffDifference(
              context(verifiedAbsent, FormatEpoch::UncompressedLegacy, finale2000)) ==
          DifferenceClassification::FinaleUpgradeSynthesis);
}

TEST_CASE("Legacy note-attached-items expansion loses fallback expression display", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value disabled(false);
    const Value enabled(true);
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const SourceVersion finale98{.major = finale_mus_reader::versions::finale98.major};
    const SourceVersion finale2000{.major = finale_mus_reader::versions::finale2000.major};
    const SourceVersion finale2006{.major = finale_mus_reader::versions::finale2006.major};
    const SourceVersion finale2008{.major = finale_mus_reader::versions::finale2008.major};
    const SourceVersion finale2009{.major = finale_mus_reader::versions::finale2009.major};

    for (const auto &[className, classPrefix, otherPrefix] : {
             std::tuple{"staff", "staff[", "staff_style["},
             std::tuple{"staff_style", "staff_style[", "staff["},
         })
    {
        CAPTURE(className);
        const auto classify = differenceClassifier(className);
        REQUIRE(classify);
        const auto objectPath = std::string(classPrefix) + "cmper=1]";
        const auto differencePath = objectPath + ".alt_hide_expressions";
        ComparisonLeaves sourceLeaves{
            {objectPath + ".alt_hide_artics", {Value(true), "legacy-mus"}},
            {objectPath + ".alt_hide_lyrics", {Value(true), "legacy-mus"}},
            {objectPath + ".alt_hide_smart_shapes", {Value(true), "legacy-mus-adjusted"}},
            {objectPath + ".hide_chords", {Value(true), "legacy-mus"}},
            {objectPath + ".hide_fretboards", {Value(true), "legacy-mus"}},
            {objectPath + ".alt_notation",
             {Value(static_cast<std::int64_t>(Staff::AlternateNotation::Normal)), "legacy-mus"}},
        };
        ComparisonLeaves companionLeaves = sourceLeaves;
        const auto context = [&](const ComparisonLeaves &source, const ComparisonLeaves &companion,
                                 FormatEpoch epoch, const SourceVersion *version,
                                 const Value *sourceValue = nullptr,
                                 const Value *companionValue = nullptr) {
            return DifferenceContext{differencePath,
                                     DifferenceCategory::Differs,
                                     "finale27-default",
                                     sourceValue ? *sourceValue : disabled,
                                     companionValue ? *companionValue : enabled,
                                     source,
                                     companion,
                                     epoch,
                                     ByteOrder::BigEndian,
                                     version,
                                     report};
        };

        REQUIRE(classify(context(sourceLeaves, companionLeaves, FormatEpoch::UncompressedLegacy,
                                 &finale2000)) == DifferenceClassification::FinaleUpgradeLoss);
        REQUIRE(classify(context(sourceLeaves, companionLeaves, FormatEpoch::DclLegacy,
                                 &finale2006)) == DifferenceClassification::FinaleUpgradeLoss);
        REQUIRE(classify(context(sourceLeaves, companionLeaves, FormatEpoch::ZlibLegacy,
                                 &finale2008)) == DifferenceClassification::FinaleUpgradeLoss);
        CHECK_FALSE(classify(
            context(sourceLeaves, companionLeaves, FormatEpoch::UncompressedLegacy, &finale98)));
        REQUIRE(classify(context(sourceLeaves, companionLeaves, FormatEpoch::ZlibLegacy,
                                 &finale2009)) == DifferenceClassification::FinaleUpgradeLoss);

        auto missingAggregateLeaf = sourceLeaves;
        missingAggregateLeaf.erase(objectPath + ".alt_hide_smart_shapes");
        CHECK_FALSE(classify(context(missingAggregateLeaf, companionLeaves,
                                     FormatEpoch::UncompressedLegacy, &finale2000)));
        auto wrongAggregateOrigin = sourceLeaves;
        wrongAggregateOrigin.at(objectPath + ".alt_hide_lyrics").second = "finale27-default";
        CHECK_FALSE(classify(context(wrongAggregateOrigin, companionLeaves,
                                     FormatEpoch::UncompressedLegacy, &finale2000)));
        auto disabledAggregateLeaf = sourceLeaves;
        disabledAggregateLeaf.at(objectPath + ".hide_chords").first = Value(false);
        CHECK_FALSE(classify(context(disabledAggregateLeaf, companionLeaves,
                                     FormatEpoch::UncompressedLegacy, &finale2000)));
        auto differingCompanionExpansion = companionLeaves;
        differingCompanionExpansion.at(objectPath + ".hide_chords").first = Value(false);
        CHECK(classify(context(sourceLeaves, differingCompanionExpansion,
                               FormatEpoch::UncompressedLegacy, &finale2000)) ==
              DifferenceClassification::FinaleUpgradeLoss);
        auto rhythmicSource = sourceLeaves;
        rhythmicSource.at(objectPath + ".alt_notation").first =
            Value(static_cast<std::int64_t>(Staff::AlternateNotation::Rhythmic));
        CHECK(classify(context(rhythmicSource, companionLeaves, FormatEpoch::UncompressedLegacy,
                               &finale2000)) == DifferenceClassification::FinaleUpgradeLoss);
        CHECK_FALSE(classify(context(sourceLeaves, companionLeaves, FormatEpoch::UncompressedLegacy,
                                     &finale2000, &enabled, &disabled)));

        auto wrongClass =
            context(sourceLeaves, companionLeaves, FormatEpoch::UncompressedLegacy, &finale2000);
        const auto otherPath = std::string(otherPrefix) + "cmper=1].alt_hide_expressions";
        wrongClass.path = otherPath;
        CHECK_FALSE(classify(wrongClass));
        auto wrongOrigin =
            context(sourceLeaves, companionLeaves, FormatEpoch::UncompressedLegacy, &finale2000);
        wrongOrigin.origin = "legacy-behavior";
        CHECK_FALSE(classify(wrongOrigin));
    }
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
        document->calcScrollViewCmper(musx::dom::SCORE_PARTID), musx::dom::Inci{0});
    staffUsed->staffId = musx::dom::StaffCmper{1};
    document->getOthers()->add(musx::dom::others::StaffUsed::XmlNodeName, std::move(staffUsed));
    const auto context = [&](std::string_view path, std::string_view origin,
                             DifferenceCategory category) {
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

TEST_CASE("Finale 2000 StaffStyles share notation-derived Smart Shape hiding")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    const auto result = readFixture("evidence/F2000/F2000-F372-facingpages-parts.mus");
    constexpr bool expected[] = {true, false, true, true, true};

    for (std::size_t index = 0; index < std::size(expected); ++index)
    {
        const auto cmper = static_cast<musx::dom::Cmper>(index + 1);
        const auto style =
            result.document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID, cmper);
        REQUIRE(style);
        CHECK(style->altHideSmartShapes == expected[index]);
        CHECK_FALSE(style->altHideExpressions);
        const auto *field = result.report.findField<StaffStyle>("altHideSmartShapes",
                                                                musx::dom::SCORE_PARTID, cmper);
        REQUIRE(field);
        CHECK(field->origin == ValueOrigin::LegacyMusAdjusted);
        const auto *expressions = result.report.findField<StaffStyle>(
            "altHideExpressions", musx::dom::SCORE_PARTID, cmper);
        REQUIRE(expressions);
        CHECK(expressions->origin == ValueOrigin::Finale27Default);
    }
}

TEST_CASE("StaffStyle reuses each Staff payload layout and decodes its trailer")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    struct Expected
    {
        const char *path;
        const char *name;
    };
    constexpr Expected expected[] = {
        {"evidence/F2000/F2000-staff-style.mus", "1-line staff"},
        {"evidence/F2003/F2003-staffstyle.mus", "1-staff line"},
        {"evidence/F2011/F2011-staffstyle.mus", "1-line staff"},
    };
    for (const auto &item : expected)
    {
        const auto result = readFixture(item.path);
        const auto style = result.document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID,
                                                                         musx::dom::Cmper{1});
        REQUIRE(style);
        CHECK(style->styleName == item.name);
        CHECK(style->customStaff == std::optional<std::vector<int>>{{13}});
        CHECK(style->lineSpace == 24);
        CHECK(style->botBarlineOffset == -24);
        CHECK(style->topBarlineOffset == 24);
        CHECK(style->dwRestOffset == -5);
        CHECK(style->wRestOffset == -6);
        CHECK(style->hRestOffset == -4);
        CHECK(style->otherRestOffset == -4);
        CHECK(style->stemReversal == -4);
        CHECK(style->botRepeatDotOff == -5);
        CHECK(style->topRepeatDotOff == -3);
        REQUIRE(style->masks);
        CHECK(style->masks->staffType);
        CHECK_FALSE(style->copyable);
        CHECK(style->addToMenu);
    }
}

TEST_CASE("Finale 2000 StaffStyle menu and copy controls are independent")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    struct Expected
    {
        const char *path;
        bool addToMenu;
        bool copyable;
    };
    constexpr Expected expected[] = {
        {"evidence/F2000/F2000-style-menu.mus", true, false},
        {"evidence/F2000/F2000-style-nomenu.mus", false, false},
        {"evidence/F2000/F2000-style-menu-copyable.mus", true, true},
    };

    for (const auto &item : expected)
    {
        const auto result = readFixture(item.path);
        const auto style = result.document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID,
                                                                         musx::dom::Cmper{1});
        REQUIRE(style);
        CHECK(style->addToMenu == item.addToMenu);
        CHECK(style->copyable == item.copyable);
    }

    const auto implicitStyle = [](std::int16_t secondMask) {
        const auto parsed = makeContainer({{1, "SY", {0, 0, 0, 0, 0, 0}},
                                           {1, "SY", {0, 0, 0, 0, 0, 0}},
                                           {1, "SY", {0, 0, 0, 0, 0, 0}},
                                           {1, "SY", {0, secondMask, 0, 0, 0, 0}},
                                           {1, "SY", {0, 0, 0, 0, 0, 0}},
                                           {1, "SY", {0, 0, 0, 0, 0, 0}},
                                           {1, "SY", {0, 0, 0, 0, 0, 0}},
                                           {1, "SY", {0, 0, 0, 0, 0, 0}}});
        const auto document = emptyStaffDocument();
        staffImport(parsed, profileFor(finale_mus_reader::versions::finale2000.major), document,
                    true);
        return document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1});
    };
    const auto ordinary = implicitStyle(0);
    REQUIRE(ordinary);
    CHECK(ordinary->addToMenu);
    CHECK_FALSE(ordinary->copyable);
    const auto alternateNotation = implicitStyle(0x1000);
    REQUIRE(alternateNotation);
    CHECK(alternateNotation->addToMenu);
    CHECK(alternateNotation->copyable);
}

TEST_CASE("StaffStyle maps every stored mask and control bit")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    const auto parsed = makeContainer({{1, "SY", {-24, 0, 0, 0, 0, 0}},
                                       {1, "SY", {0, 1, 4, 24, 0, 0}},
                                       {1, "SY", {-1285, -772, -4, 0, 0, 0}},
                                       {1, "SY", {-1, -1, 0x03ff, 0, 0, 0x0006}},
                                       {1, "SY", {0x540d, 0x5465, 0x7374, 0, 0, 0}},
                                       {1, "SY", {0, 0, 0, 0, 0, 0}},
                                       {1, "SY", {0, 0, 0, 0, 0, 0}},
                                       {1, "SY", {0, 0, 0, 0, 0, 0}}});
    const auto document = emptyStaffDocument();
    auto report = staffImport(parsed, profileFor(finale_mus_reader::versions::finale2000.major),
                              document, true);
    const auto style =
        document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1});
    REQUIRE(style);
    CHECK(style->styleName == "T\nTest");
    CHECK(style->copyable);
    CHECK(style->addToMenu);
    REQUIRE(style->masks);
#define CHECK_STAFF_STYLE_MASK(member) CHECK(style->masks->member)
    CHECK_STAFF_STYLE_MASK(floatNoteheadFont);
    CHECK_STAFF_STYLE_MASK(useNoteShapes);
    CHECK_STAFF_STYLE_MASK(flatBeams);
    CHECK_STAFF_STYLE_MASK(blankMeasureRest);
    CHECK_STAFF_STYLE_MASK(noOptimize);
    CHECK_STAFF_STYLE_MASK(notationStyle);
    CHECK_STAFF_STYLE_MASK(defaultClef);
    CHECK_STAFF_STYLE_MASK(staffType);
    CHECK_STAFF_STYLE_MASK(transposition);
    CHECK_STAFF_STYLE_MASK(blineBreak);
    CHECK_STAFF_STYLE_MASK(rbarBreak);
    CHECK_STAFF_STYLE_MASK(negMnumb);
    CHECK_STAFF_STYLE_MASK(negRepeat);
    CHECK_STAFF_STYLE_MASK(negNameScore);
    CHECK_STAFF_STYLE_MASK(hideBarlines);
    CHECK_STAFF_STYLE_MASK(fullName);
    CHECK_STAFF_STYLE_MASK(abrvName);
    CHECK_STAFF_STYLE_MASK(floatKeys);
    CHECK_STAFF_STYLE_MASK(floatTime);
    CHECK_STAFF_STYLE_MASK(hideRptBars);
    CHECK_STAFF_STYLE_MASK(negKey);
    CHECK_STAFF_STYLE_MASK(negTime);
    CHECK_STAFF_STYLE_MASK(negClef);
    CHECK_STAFF_STYLE_MASK(hideStaff);
    CHECK_STAFF_STYLE_MASK(noKey);
    CHECK_STAFF_STYLE_MASK(fullNamePos);
    CHECK_STAFF_STYLE_MASK(abrvNamePos);
    CHECK_STAFF_STYLE_MASK(altNotation);
    CHECK_STAFF_STYLE_MASK(showTies);
    CHECK_STAFF_STYLE_MASK(showDots);
    CHECK_STAFF_STYLE_MASK(showRests);
    CHECK_STAFF_STYLE_MASK(showStems);
    CHECK_STAFF_STYLE_MASK(hideChords);
    CHECK_STAFF_STYLE_MASK(hideFretboards);
    CHECK_STAFF_STYLE_MASK(hideLyrics);
    CHECK_STAFF_STYLE_MASK(showNameParts);
    CHECK_STAFF_STYLE_MASK(showNoteColors);
    CHECK_STAFF_STYLE_MASK(hideStaffLines);
    CHECK_STAFF_STYLE_MASK(redisplayLayerAccis);
    CHECK_STAFF_STYLE_MASK(negTimeParts);
    CHECK_STAFF_STYLE_MASK(hideKeySigsShowAccis);
#undef CHECK_STAFF_STYLE_MASK

    const auto key =
        finale_mus_reader::instanceKey<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1});
    REQUIRE(report.fields.contains(key));
    CHECK(report.fields.at(key).size() == 145);
    CHECK(report.fields.at(key).at("styleName").origin == ValueOrigin::LegacyMus);
    CHECK(report.fields.at(key).at("masks.staffType").origin == ValueOrigin::LegacyMus);
    CHECK(report.fields.at(key).at("masks.negTimeParts").origin == ValueOrigin::LegacyBehavior);
    for (const auto *member : {
             "transposition.setToClef",
             "transposition.noSimplifyKey",
             "transposition.keysig.interval",
             "transposition.keysig.adjust",
             "transposition.chromatic.alteration",
             "transposition.chromatic.diatonic",
         })
    {
        CHECK(report.fields.at(key).at(member).origin == ValueOrigin::LegacyMus);
    }

    const auto observed = finale_mus_reader::coverage::runAllSurveyors({document, report});
    const auto &styles = observed.snapshot.at("staff_style").asArray();
    REQUIRE(styles.size() == 1);
    const auto &object = styles.front().asObject();
    CHECK(object.at("style_name").asString() == "T\nTest");
    CHECK(object.at("origin_styleName").asString() == "legacy-mus");
    CHECK(object.at("masks").asObject().size() == 82);
}

TEST_CASE("StaffStyle alternate notation mask includes its chord and fretboard "
          "overrides")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    const auto parsed = makeContainer({{1, "SY", {-24, 0, 0, 0, 0, 0}},
                                       {1, "SY", {0, 1, 4, 24, 0, 0}},
                                       {1, "SY", {-1285, -772, -4, 0, 0, 0}},
                                       {1, "SY", {0, 0x1000, 0, 0, 0, 0}},
                                       {1, "SY", {0x5465, 0x7374, 0, 0, 0, 0}},
                                       {1, "SY", {0, 0, 0, 0, 0, 0}},
                                       {1, "SY", {0, 0, 0, 0, 0, 0}},
                                       {1, "SY", {0, 0, 0, 0, 0, 0}}});
    const auto document = emptyStaffDocument();
    const auto report = staffImport(
        parsed, profileFor(finale_mus_reader::versions::finale2000.major), document, true);
    const auto style =
        document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1});

    REQUIRE(style);
    REQUIRE(style->masks);
    CHECK(style->masks->altNotation);
    CHECK(style->masks->hideChords);
    CHECK(style->masks->hideFretboards);

    const auto &fields = report.fields.at(
        finale_mus_reader::instanceKey<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1}));
    CHECK(fields.at("masks.altNotation").decodedOffset ==
          fields.at("masks.hideChords").decodedOffset);
    CHECK(fields.at("masks.altNotation").decodedOffset ==
          fields.at("masks.hideFretboards").decodedOffset);
}

TEST_CASE("Legacy Note Shapes notation uses the staff-specific StaffStyle mask")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    const auto parsed = makeContainer({{1, "SY", {-24, 0, 0, 0, 0, 0x0080}},
                                       {1, "SY", {0, 1, 4, 24, 0, 0}},
                                       {1, "SY", {-1285, -772, -4, 0, 0, 0}},
                                       {1, "SY", {0x0004, 0, 0, 0, 0, 0}},
                                       {1, "SY", {0x5465, 0x7374, 0, 0, 0}},
                                       {1, "SY", {0, 0, 0, 0, 0, 0}},
                                       {1, "SY", {0, 0, 0, 0, 0, 0}},
                                       {1, "SY", {0, 0, 0, 0, 0, 0}}});
    const auto document = emptyStaffDocument();
    const auto report = staffImport(
        parsed, profileFor(finale_mus_reader::versions::finale2000.major), document, true);
    const auto style =
        document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1});

    REQUIRE(style);
    REQUIRE(style->masks);
    CHECK(style->notationStyle == Staff::NotationStyle::Standard);
    CHECK(style->useNoteShapes);
    CHECK_FALSE(style->masks->notationStyle);
    CHECK(style->masks->useNoteShapes);

    const auto &fields = report.fields.at(
        finale_mus_reader::instanceKey<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1}));
    CHECK(fields.at("notationStyle").origin == ValueOrigin::LegacyMusAdjusted);
    CHECK(fields.at("useNoteShapes").origin == ValueOrigin::LegacyMusAdjusted);
    CHECK(fields.at("masks.notationStyle").origin == ValueOrigin::LegacyMusAdjusted);
    CHECK(fields.at("masks.useNoteShapes").origin == ValueOrigin::LegacyMusAdjusted);
    CHECK(fields.at("masks.notationStyle").decodedOffset ==
          fields.at("masks.useNoteShapes").decodedOffset);
}

TEST_CASE("Finale 2006 Note Shapes StaffStyle uses only its staff-specific "
          "notation mask")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    const auto result = readFixture("evidence/F2006/F2006-embedded-tif.mus");
    const auto style = result.document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID,
                                                                     musx::dom::Cmper{19});

    REQUIRE(style);
    CHECK(style->styleName == "15.  Note Shapes");
    CHECK(style->notationStyle == Staff::NotationStyle::Standard);
    CHECK(style->useNoteShapes);
    REQUIRE(style->masks);
    CHECK_FALSE(style->masks->notationStyle);
    CHECK(style->masks->useNoteShapes);

    const auto *notationMask = result.report.findField<StaffStyle>(
        "masks.notationStyle", musx::dom::SCORE_PARTID, musx::dom::Cmper{19});
    const auto *noteShapesMask = result.report.findField<StaffStyle>(
        "masks.useNoteShapes", musx::dom::SCORE_PARTID, musx::dom::Cmper{19});
    REQUIRE(notationMask);
    REQUIRE(noteShapesMask);
    CHECK(notationMask->origin == ValueOrigin::LegacyMusAdjusted);
    CHECK(noteShapesMask->origin == ValueOrigin::LegacyMusAdjusted);
}

TEST_CASE("Finale 2012 Note Shapes settings retain independent StaffStyle masks")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    std::vector<std::int16_t> words(48, 0);
    words[5] = 0x0080;
    words.insert(words.end(), {0x0004, 0, 0, 0, 0, 0});
    words.resize(150, 0);
    const auto parsed = makeClassContainer(0x00e8, words, ByteOrder::LittleEndian, 1);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    profile.version = SourceVersion{.major = finale_mus_reader::versions::finale2012.major};
    const auto report = staffImport(parsed, profile, document, true);
    const auto style =
        document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1});

    REQUIRE(style);
    REQUIRE(style->masks);
    CHECK(style->notationStyle == Staff::NotationStyle::Standard);
    CHECK(style->useNoteShapes);
    CHECK(style->masks->notationStyle);
    CHECK_FALSE(style->masks->useNoteShapes);

    const auto &fields = report.fields.at(
        finale_mus_reader::instanceKey<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1}));
    CHECK(fields.at("notationStyle").origin == ValueOrigin::LegacyMus);
    CHECK(fields.at("useNoteShapes").origin == ValueOrigin::LegacyMus);
    CHECK(fields.at("masks.notationStyle").origin == ValueOrigin::LegacyMus);
    CHECK(fields.at("masks.useNoteShapes").origin == ValueOrigin::LegacyMus);
}

TEST_CASE("StaffStyle selects and decodes its Unicode trailer structurally")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    std::vector<std::int16_t> words(48, 0);
    words.insert(words.end(), {0, 0x1000, 0x0006, 0, 0, 0x0006});
    words.resize(150, 0);
    const std::u16string name = u"Élan U0001D11E";
    for (std::size_t index = 0; index < name.size(); ++index)
        words[54 + index] = static_cast<std::int16_t>(name[index]);
    const auto parsed = makeClassContainer(0x00e8, words, ByteOrder::LittleEndian, 1);
    const auto document = emptyStaffDocument();
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    const auto report = staffImport(parsed, profile, document, true);

    const auto style =
        document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1});
    REQUIRE(style);
    CHECK(style->styleName == "Élan U0001D11E");
    CHECK(style->copyable);
    CHECK(style->addToMenu);
    REQUIRE(style->masks);
    CHECK(style->masks->altNotation);
    CHECK(style->masks->hideChords);
    CHECK(style->masks->hideFretboards);
    CHECK(report.fields
              .at(finale_mus_reader::instanceKey<StaffStyle>(musx::dom::SCORE_PARTID,
                                                             musx::dom::Cmper{1}))
              .at("styleName")
              .origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Finale 2012 StaffStyle records recover their stored names and controls")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    const auto result = readFixture("evidence/F2012/F2012-bookmarks.mus");
    constexpr std::string_view names[] = {"Normal Notation", "Slash Notation",  "Rhythmic Notation",
                                          "One Bar Repeats", "Two Bar Repeats", "Blank Notation"};
    for (std::size_t index = 0; index < std::size(names); ++index)
    {
        const auto style = result.document->getOthers()->get<StaffStyle>(
            musx::dom::SCORE_PARTID, static_cast<musx::dom::Cmper>(index + 1));
        REQUIRE(style);
        CHECK(style->styleName == names[index]);
        CHECK(style->copyable);
        CHECK(style->addToMenu);
        REQUIRE(style->masks);
        CHECK(style->masks->altNotation);
        CHECK(style->masks->hideChords);
        CHECK(style->masks->hideFretboards);
    }
}

TEST_CASE("StaffStyle uses its Finale 2012 boundary for unrepresented record "
          "lengths")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    const auto parsed =
        makeClassContainer(0x00e8, std::vector<std::int16_t>(100), ByteOrder::LittleEndian, 1);
    const auto importWithVersion = [&](std::uint8_t major) {
        const auto document = emptyStaffDocument();
        auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
        profile.byteOrder = ByteOrder::LittleEndian;
        profile.version = SourceVersion{.major = major};
        return std::pair{document, staffImport(parsed, profile, document, true)};
    };

    const auto [finale2011Document, finale2011Report] =
        importWithVersion(finale_mus_reader::versions::finale2011.major);
    CHECK(finale2011Document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID,
                                                           musx::dom::Cmper{1}));
    CHECK(finale2011Report.diagnostics.empty());

    const auto [finale2012Document, finale2012Report] =
        importWithVersion(finale_mus_reader::versions::finale2012.major);
    CHECK_FALSE(finale2012Document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID,
                                                                 musx::dom::Cmper{1}));
    REQUIRE(finale2012Report.diagnostics.size() == 1);
    CHECK(finale2012Report.diagnostics.front().message.find("shorter than") != std::string::npos);
}

TEST_CASE("StaffStyle remains absent when no source record exists")
{
    using StaffStyle = musx::dom::others::StaffStyle;
    for (const auto path : {"evidence/F100/F100-baseline.mus", "evidence/F97/F97-def-measrest.mus"})
    {
        const auto result = readFixture(path);
        CHECK(result.document->getOthers()->getAllSources<StaffStyle>().empty());
    }
}

TEST_CASE("Finale 2000 through 2008 StaffStyle chord and fretboard aggregate is lost "
          "during upgrade",
          "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    constexpr std::string_view objectPath = "staff_style[cmper=5]";
    const auto classify = differenceClassifier("staff_style");
    REQUIRE(classify);
    ComparisonLeaves sourceLeaves{
        {std::string(objectPath) + ".alt_hide_artics", {Value(true), "legacy-mus"}},
        {std::string(objectPath) + ".alt_hide_lyrics", {Value(true), "legacy-mus"}},
        {std::string(objectPath) + ".alt_hide_smart_shapes", {Value(true), "legacy-mus-adjusted"}},
        {std::string(objectPath) + ".hide_chords", {Value(true), "legacy-mus"}},
        {std::string(objectPath) + ".hide_fretboards", {Value(true), "legacy-mus"}},
        {std::string(objectPath) + ".masks.hide_chords", {Value(true), "legacy-mus"}},
        {std::string(objectPath) + ".masks.hide_fretboards", {Value(true), "legacy-mus"}},
    };
    const ComparisonLeaves companionLeaves = sourceLeaves;
    const Value enabled(true);
    const Value disabled(false);
    ImportReport report(FormatEpoch::DclLegacy);
    const SourceVersion finale98{.major = finale_mus_reader::versions::finale98.major};
    const SourceVersion finale2000{.major = finale_mus_reader::versions::finale2000.major};
    const SourceVersion finale2006{.major = finale_mus_reader::versions::finale2006.major};
    const SourceVersion finale2008{.major = finale_mus_reader::versions::finale2008.major};
    const SourceVersion finale2009{.major = finale_mus_reader::versions::finale2009.major};
    const auto context = [&](const ComparisonLeaves &source, FormatEpoch epoch,
                             const SourceVersion *version, const Value *sourceValue = nullptr,
                             const Value *companionValue = nullptr,
                             std::string_view suffix = ".hide_fretboards") {
        const auto path = suffix == ".hide_chords" ? "staff_style[cmper=5].hide_chords"
                                                   : "staff_style[cmper=5].hide_fretboards";
        return DifferenceContext{path,
                                 DifferenceCategory::Differs,
                                 "legacy-mus",
                                 sourceValue ? *sourceValue : enabled,
                                 companionValue ? *companionValue : disabled,
                                 source,
                                 companionLeaves,
                                 epoch,
                                 ByteOrder::BigEndian,
                                 version,
                                 report};
    };

    for (const auto suffix : {".hide_chords", ".hide_fretboards"})
    {
        REQUIRE(classify(context(sourceLeaves, FormatEpoch::UncompressedLegacy, &finale2000,
                                 nullptr, nullptr, suffix)) ==
                DifferenceClassification::FinaleUpgradeLoss);
        REQUIRE(classify(context(sourceLeaves, FormatEpoch::DclLegacy, &finale2006, nullptr,
                                 nullptr, suffix)) == DifferenceClassification::FinaleUpgradeLoss);
        REQUIRE(classify(context(sourceLeaves, FormatEpoch::ZlibLegacy, &finale2008, nullptr,
                                 nullptr, suffix)) == DifferenceClassification::FinaleUpgradeLoss);
    }
    CHECK_FALSE(classify(context(sourceLeaves, FormatEpoch::UncompressedLegacy, &finale98)));
    CHECK_FALSE(classify(context(sourceLeaves, FormatEpoch::ZlibLegacy, &finale2009)));

    auto missingAggregateLeaf = sourceLeaves;
    missingAggregateLeaf.erase(std::string(objectPath) + ".alt_hide_artics");
    CHECK_FALSE(classify(context(missingAggregateLeaf, FormatEpoch::DclLegacy, &finale2006)));
    auto disabledMask = sourceLeaves;
    disabledMask.at(std::string(objectPath) + ".masks.hide_fretboards").first = Value(false);
    CHECK_FALSE(classify(context(disabledMask, FormatEpoch::DclLegacy, &finale2006)));

    auto wrongClass = context(sourceLeaves, FormatEpoch::DclLegacy, &finale2006);
    wrongClass.path = "staff[cmper=5].hide_fretboards";
    CHECK_FALSE(classify(wrongClass));
    auto wrongOrigin = context(sourceLeaves, FormatEpoch::DclLegacy, &finale2006);
    wrongOrigin.origin = "legacy-mus-adjusted";
    CHECK_FALSE(classify(wrongOrigin));
    CHECK_FALSE(
        classify(context(sourceLeaves, FormatEpoch::DclLegacy, &finale2006, &disabled, &enabled)));
}

TEST_CASE("Legacy StaffStyle Smart Shape values can change when Finale expands them", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using StaffStyle = musx::dom::others::StaffStyle;
    const auto classify = differenceClassifier("staff_style");
    REQUIRE(classify);

    const Value enabled(true);
    const Value disabled(false);
    const ComparisonLeaves leaves;
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const SourceVersion finale2000{.major = finale_mus_reader::versions::finale2000.major};
    const SourceVersion finale2009{.major = finale_mus_reader::versions::finale2009.major};
    const auto context = [&](std::string_view path, std::string_view origin, const Value &source,
                             const Value &companion, const SourceVersion *version = nullptr) {
        return DifferenceContext{path,
                                 DifferenceCategory::Differs,
                                 origin,
                                 source,
                                 companion,
                                 leaves,
                                 leaves,
                                 FormatEpoch::UncompressedLegacy,
                                 ByteOrder::BigEndian,
                                 version ? version : &finale2000,
                                 report};
    };

    REQUIRE(classify(context("staff_style[cmper=2].alt_hide_smart_shapes", "legacy-mus-adjusted",
                             enabled, disabled)) == DifferenceClassification::FinaleUpgradeLoss);
    CHECK_FALSE(classify(
        context("staff_style[cmper=2].alt_hide_smart_shapes", "legacy-mus", enabled, disabled)));
    CHECK_FALSE(classify(context("staff_style[cmper=2].alt_hide_smart_shapes",
                                 "legacy-mus-adjusted", disabled, enabled)));
    auto finale2009Context = context("staff_style[cmper=2].alt_hide_smart_shapes",
                                     "legacy-mus-adjusted", enabled, disabled, &finale2009);
    finale2009Context.epoch = FormatEpoch::ZlibLegacy;
    CHECK_FALSE(classify(finale2009Context));

    const auto key =
        finale_mus_reader::instanceKey<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{2});
    report.setField(key, "altHideOtherArtics",
                    {ValueOrigin::LegacyMus, 10, 20, 0, std::uint16_t{0x5359}});
    report.setField(key, "altHideOtherSmartShapes",
                    {ValueOrigin::LegacyMus, 10, 20, 0, std::uint16_t{0x5359}});
    REQUIRE(classify(context("staff_style[cmper=2].alt_hide_other_smart_shapes", "legacy-mus",
                             disabled, enabled)) == DifferenceClassification::FinaleUpgradeLoss);
    REQUIRE(classify(context("staff_style[cmper=2].alt_hide_other_smart_shapes", "legacy-mus",
                             enabled, disabled)) == DifferenceClassification::FinaleUpgradeLoss);

    report.findField(key, "altHideOtherSmartShapes")->decodedOffset = 22;
    CHECK_FALSE(classify(context("staff_style[cmper=2].alt_hide_other_smart_shapes", "legacy-mus",
                                 disabled, enabled)));
    CHECK_FALSE(classify(context("staff_style[cmper=2].alt_hide_other_smart_shapes",
                                 "legacy-mus-adjusted", disabled, enabled)));
    CHECK_FALSE(classify(context("staff_style[cmper=2].alt_hide_other_expressions", "legacy-mus",
                                 disabled, enabled)));
}

TEST_CASE("Legacy aggregate Two-Bar Repeat StaffStyles lose visible articulations "
          "and lyrics and may hide chords and fretboards during upgrade",
          "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using StaffStyle = musx::dom::others::StaffStyle;
    const auto classify = differenceClassifier("staff_style");
    REQUIRE(classify);

    constexpr std::string_view objectPath = "staff_style[cmper=6]";
    ComparisonLeaves source{
        {std::string(objectPath) + ".alt_notation",
         {Value(static_cast<std::int64_t>(Staff::AlternateNotation::TwoBarRepeat)), "legacy-mus"}},
        {std::string(objectPath) + ".alt_hide_other_artics", {Value(true), "legacy-mus"}}};
    const ComparisonLeaves companion = source;
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const auto key =
        finale_mus_reader::instanceKey<StaffStyle>(musx::dom::SCORE_PARTID, musx::dom::Cmper{6});
    report.setField(key, "altHideOtherArtics",
                    {ValueOrigin::LegacyMus, 10, 20, 0, std::uint16_t{0x5359}});
    report.setField(key, "altHideOtherSmartShapes",
                    {ValueOrigin::LegacyMus, 10, 20, 0, std::uint16_t{0x5359}});
    const SourceVersion finale2004{.major = finale_mus_reader::versions::finale2004.major};
    const SourceVersion finale2007{.major = finale_mus_reader::versions::finale2007.major};
    const Value visible(false);
    const Value hidden(true);
    const auto classifyCase = [&](std::string_view suffix, FormatEpoch epoch,
                                  const SourceVersion *version, std::string_view origin,
                                  const Value &sourceValue, const Value &companionValue,
                                  const ComparisonLeaves &sourceLeaves) {
        const auto path = std::string(objectPath) + std::string(suffix);
        return classify(DifferenceContext{path, DifferenceCategory::Differs, origin, sourceValue,
                                          companionValue, sourceLeaves, companion, epoch,
                                          ByteOrder::BigEndian, version, report});
    };
    const auto classifyUpgrade = [&](std::string_view suffix, FormatEpoch epoch,
                                     const SourceVersion *version) {
        return classifyCase(suffix, epoch, version, "legacy-mus", visible, hidden, source);
    };

    for (const auto suffix : {".alt_hide_artics", ".alt_hide_lyrics"})
    {
        CHECK(classifyUpgrade(suffix, FormatEpoch::UncompressedLegacy, &finale2004) ==
              DifferenceClassification::FinaleUpgradeLoss);
        CHECK(classifyUpgrade(suffix, FormatEpoch::ZlibLegacy, &finale2007) ==
              DifferenceClassification::FinaleUpgradeLoss);
    }
    for (const auto suffix : {".hide_chords", ".hide_fretboards"})
    {
        CHECK(classifyUpgrade(suffix, FormatEpoch::UncompressedLegacy, &finale2004) ==
              DifferenceClassification::FinaleUpgradeLoss);
    }

    auto otherItemsVisible = source;
    otherItemsVisible.at(std::string(objectPath) + ".alt_hide_other_artics").first = Value(false);
    CHECK_FALSE(classifyCase(".hide_chords", FormatEpoch::UncompressedLegacy, &finale2004,
                             "legacy-mus", visible, hidden, otherItemsVisible));

    auto wrongNotation = source;
    wrongNotation.at(std::string(objectPath) + ".alt_notation").first =
        Value(static_cast<std::int64_t>(Staff::AlternateNotation::OneBarRepeat));
    CHECK_FALSE(classifyCase(".alt_hide_artics", FormatEpoch::ZlibLegacy, &finale2007, "legacy-mus",
                             visible, hidden, wrongNotation));
    CHECK_FALSE(classifyCase(".alt_hide_artics", FormatEpoch::ZlibLegacy, &finale2007,
                             "legacy-mus-adjusted", visible, hidden, source));
    CHECK_FALSE(classifyCase(".alt_hide_artics", FormatEpoch::ZlibLegacy, &finale2007, "legacy-mus",
                             hidden, visible, source));
    report.findField(key, "altHideOtherSmartShapes")->decodedOffset = 22;
    CHECK_FALSE(classifyUpgrade(".alt_hide_artics", FormatEpoch::ZlibLegacy, &finale2007));
}

TEST_CASE("StaffStyle Unknown UUID upgrades are normalized", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto classify = differenceClassifier("staff_style");
    REQUIRE(classify);
    const auto objectPath = std::string("staff_style[cmper=1]");
    const ComparisonLeaves leaves{
        {objectPath + ".inst_uuid",
         {Value(std::string(musx::dom::uuid::Unknown)), "legacy-behavior"}},
        {objectPath + ".notation_style",
         {Value(static_cast<std::int64_t>(Staff::NotationStyle::Percussion)), "legacy-mus"}},
        {objectPath + ".masks.notation_style", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.default_clef", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.float_notehead_font", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.no_key", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.show_note_colors", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.hide_key_sigs_show_accis", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.staff_type", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.transposition", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.full_name", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.abrv_name", {Value(true), "legacy-mus"}},
        {objectPath + ".default_clef", {Value(std::int64_t(0)), "legacy-mus"}},
        {objectPath + ".use_note_font", {Value(true), "legacy-mus"}},
        {objectPath + ".no_key", {Value(true), "legacy-mus"}},
        {objectPath + ".show_note_colors", {Value(false), "legacy-mus"}},
        {objectPath + ".hide_key_sigs_show_accis", {Value(false), "legacy-mus"}}};
    ComparisonLeaves companionLeaves = leaves;
    companionLeaves.at(objectPath + ".inst_uuid").first =
        Value(std::string(musx::dom::uuid::PercussionGeneral));
    companionLeaves.at(objectPath + ".masks.default_clef").first = Value(true);
    companionLeaves.at(objectPath + ".masks.float_notehead_font").first = Value(false);
    companionLeaves.at(objectPath + ".masks.no_key").first = Value(false);
    companionLeaves.at(objectPath + ".masks.show_note_colors").first = Value(true);
    companionLeaves.at(objectPath + ".masks.hide_key_sigs_show_accis").first = Value(true);
    ImportReport report(FormatEpoch::DclLegacy);
    const Value unknown{std::string(musx::dom::uuid::Unknown)};
    const Value blank{std::string(musx::dom::uuid::BlankStaff)};
    const Value blank2{std::string(musx::dom::uuid::BlankStaff2)};
    const Value flute{std::string(musx::dom::uuid::Flute)};
    const Value percussion{std::string(musx::dom::uuid::PercussionGeneral)};
    const auto context = [&](const Value &source, const Value &companion,
                             const ComparisonLeaves *sourceLeaves = nullptr) {
        return DifferenceContext{"staff_style[cmper=1].inst_uuid",
                                 DifferenceCategory::Differs,
                                 "legacy-behavior",
                                 source,
                                 companion,
                                 sourceLeaves ? *sourceLeaves : leaves,
                                 companionLeaves,
                                 FormatEpoch::DclLegacy,
                                 ByteOrder::BigEndian,
                                 nullptr,
                                 report};
    };

    CHECK(classify(context(unknown, blank)) ==
          DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classify(context(unknown, blank2)) ==
          DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classify(context(unknown, percussion)) ==
          DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classify(context(unknown, flute)) ==
          DifferenceClassification::FinaleUpgradeNormalization);
    CHECK_FALSE(classify(context(blank, unknown)));

    for (const auto suffix : {".masks.default_clef", ".masks.float_notehead_font", ".masks.no_key",
                              ".masks.show_note_colors", ".masks.hide_key_sigs_show_accis"})
    {
        const auto path = objectPath + suffix;
        const DifferenceContext maskContext{path,
                                            DifferenceCategory::Differs,
                                            "legacy-mus",
                                            leaves.at(path).first,
                                            companionLeaves.at(path).first,
                                            leaves,
                                            companionLeaves,
                                            FormatEpoch::DclLegacy,
                                            ByteOrder::BigEndian,
                                            nullptr,
                                            report};
        CHECK(classify(maskContext) == DifferenceClassification::FinaleUpgradeNormalization);
    }

    auto unmaskedPercussion = leaves;
    unmaskedPercussion.at(objectPath + ".masks.notation_style").first = Value(false);
    CHECK(classify(context(unknown, percussion, &unmaskedPercussion)) ==
          DifferenceClassification::FinaleUpgradeNormalization);
    auto standardNotation = leaves;
    standardNotation.at(objectPath + ".notation_style").first =
        Value(static_cast<std::int64_t>(Staff::NotationStyle::Standard));
    CHECK(classify(context(unknown, percussion, &standardNotation)) ==
          DifferenceClassification::FinaleUpgradeNormalization);

    const SourceVersion finale2012{.major = finale_mus_reader::versions::finale2012.major};
    auto post2012 = context(unknown, flute);
    post2012.epoch = FormatEpoch::ZlibLegacy;
    post2012.sourceVersion = &finale2012;
    CHECK_FALSE(classify(post2012));

    auto changedActualSetting = companionLeaves;
    changedActualSetting.at(objectPath + ".no_key").first = Value(false);
    const auto noKeyMaskPath = objectPath + ".masks.no_key";
    const DifferenceContext changedNoKey{noKeyMaskPath,
                                         DifferenceCategory::Differs,
                                         "legacy-mus",
                                         leaves.at(noKeyMaskPath).first,
                                         companionLeaves.at(noKeyMaskPath).first,
                                         leaves,
                                         changedActualSetting,
                                         FormatEpoch::DclLegacy,
                                         ByteOrder::BigEndian,
                                         nullptr,
                                         report};
    CHECK_FALSE(classify(changedNoKey));

    auto wrongOrigin = context(unknown, blank);
    wrongOrigin.origin = "legacy-mus";
    CHECK_FALSE(classify(wrongOrigin));
    auto readerOnly = context(unknown, blank);
    readerOnly.category = DifferenceCategory::ReaderOnly;
    CHECK_FALSE(classify(readerOnly));
    auto wrongClass = context(unknown, blank);
    wrongClass.path = "staff[cmper=1].inst_uuid";
    CHECK_FALSE(classify(wrongClass));
}

TEST_CASE("Finale conversion may truncate a StaffStyle name", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto classify = differenceClassifier("staff_style");
    REQUIRE(classify);
    const ComparisonLeaves leaves;
    ImportReport report(FormatEpoch::ZlibLegacy);
    const auto classifyNames = [&](std::string_view path, std::string_view source,
                                   std::string_view companion,
                                   DifferenceCategory category = DifferenceCategory::Differs,
                                   std::string_view origin = "legacy-mus") {
        const Value sourceValue{std::string(source)};
        const Value companionValue{std::string(companion)};
        return classify(DifferenceContext{path, category, origin, sourceValue, companionValue,
                                          leaves, leaves, FormatEpoch::ZlibLegacy,
                                          ByteOrder::BigEndian, nullptr, report});
    };

    CHECK(classifyNames("staff_style[cmper=5].style_name",
                        "$$$Full Name$$$ Bass Clarinet in Bb\n(actual sou",
                        "$$$Full Name$$$ Bass Cla") == DifferenceClassification::FinaleUpgradeLoss);
    CHECK_FALSE(
        classifyNames("staff_style[cmper=5].style_name", "Long Style Name", "Different Name"));
    CHECK_FALSE(classifyNames("staff_style[cmper=5].style_name", "Long Style Name", ""));
    CHECK_FALSE(classifyNames("staff_style[cmper=5].style_name", "Long Style Name", "Long",
                              DifferenceCategory::ReaderOnly));
    CHECK_FALSE(classifyNames("staff[cmper=5].style_name", "Long Style Name", "Long"));
    CHECK_FALSE(classifyNames("staff_style[cmper=5].style_name", "Long Style Name", "Long",
                              DifferenceCategory::Differs, "legacy-behavior"));
}

TEST_CASE("Finale conversion may reinterpret a StaffStyle name through Mac Roman", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto classify = differenceClassifier("staff_style");
    REQUIRE(classify);
    const ComparisonLeaves leaves;
    ImportReport report(FormatEpoch::ZlibLegacy);
    const SourceVersion finale2010{.major = finale_mus_reader::versions::finale2010.major};
    const SourceVersion finale2012{.major = finale_mus_reader::versions::finale2012.major};
    const auto classifyNames =
        [&](std::string_view path, std::string_view source, std::string_view companion,
            DifferenceCategory category = DifferenceCategory::Differs,
            std::string_view origin = "legacy-mus", const SourceVersion *version = nullptr) {
            return classify(DifferenceContext{path, category, origin, Value(std::string(source)),
                                              Value(std::string(companion)), leaves, leaves,
                                              FormatEpoch::ZlibLegacy, ByteOrder::BigEndian,
                                              version ? version : &finale2010, report});
        };

    CHECK(classifyNames("staff_style[cmper=14].style_name", "17.  Transposition: Flöte",
                        "17.  Transposition: Flˆte") ==
          DifferenceClassification::TextEncodingError);
    CHECK(classifyNames("staff_style[cmper=19].style_name", "15.  Notenköpfe", "15.  Notenkˆpfe") ==
          DifferenceClassification::TextEncodingError);
    CHECK_FALSE(classifyNames("staff_style[cmper=14].style_name", "Flöte", "Flute"));
    CHECK_FALSE(classifyNames("staff_style[cmper=14].style_name", "Flˆte", "Flöte"));
    CHECK_FALSE(classifyNames("staff[cmper=14].style_name", "Flöte", "Flˆte"));
    CHECK_FALSE(classifyNames("staff_style[cmper=14].style_name", "Flöte", "Flˆte",
                              DifferenceCategory::ReaderOnly));
    CHECK_FALSE(classifyNames("staff_style[cmper=14].style_name", "Flöte", "Flˆte",
                              DifferenceCategory::Differs, "legacy-behavior"));
    CHECK_FALSE(classifyNames("staff_style[cmper=14].style_name", "Flöte", "Flˆte",
                              DifferenceCategory::Differs, "legacy-mus", &finale2012));
}

TEST_CASE("StaffStyle names compare without surrounding whitespace", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto snapshot = [](std::string name) {
        return SurveySnapshot{{"staff_style", Value::Array{Value::Object{
                                                  {"cmper", 1}, {"style_name", std::move(name)}}}}};
    };
    const auto sourceDocument = emptyStaffDocument();
    const auto companionDocument = emptyStaffDocument();
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const auto comparison = compareSnapshots(
        snapshot("  Normal Notation"), snapshot("Normal Notation \t"), sourceDocument,
        companionDocument, FormatEpoch::UncompressedLegacy, ByteOrder::BigEndian, nullptr, report);
    const auto &stats = comparison.classes.at("others").at("staff_style");
    CHECK(stats.same == 1);
    CHECK(stats.unexpected == 0);
}

TEST_CASE("StaffStyle comparison ignores tablature fields outside Tablature notation",
          "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto snapshot = [](Staff::NotationStyle notation, std::int64_t offset) {
        return SurveySnapshot{{"staff_style",
                               Value::Array{Value::Object{
                                   {"cmper", Value(1)},
                                   {"notation_style", Value(static_cast<std::int64_t>(notation))},
                                   {"vert_tab_num_off", Value(offset)}}}}};
    };
    const auto unexpected = [&](Staff::NotationStyle notation) {
        ImportReport report(FormatEpoch::ZlibLegacy);
        const auto comparison = compareSnapshots(
            snapshot(notation, 0), snapshot(notation, -1024), emptyStaffDocument(),
            emptyStaffDocument(), FormatEpoch::ZlibLegacy, ByteOrder::LittleEndian, nullptr,
            report);
        return comparison.classes.at("others").at("staff_style").unexpected;
    };

    CHECK(unexpected(Staff::NotationStyle::Standard) == 0);
    CHECK(unexpected(Staff::NotationStyle::Percussion) == 0);
    CHECK(unexpected(Staff::NotationStyle::Tablature) == 1);
}

TEST_CASE("Finale 2009 beta legacy StaffStyle layouts are not compared", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using StaffStyle = musx::dom::others::StaffStyle;
    const auto comparisonWasSkipped = [](const SourceVersion &version, bool aggregateLayout) {
        SurveySnapshot source{{"staff_style", Value::Array{Value::Object{{"cmper", 1}}}}};
        SurveySnapshot companion{{"staff_style", Value::Array{Value::Object{{"cmper", 1}}}}};
        std::map<ComparisonTransformation, std::uint64_t> transformations;
        ImportReport report(FormatEpoch::ZlibLegacy);
        const auto instance = finale_mus_reader::instanceKey<StaffStyle>(musx::dom::SCORE_PARTID,
                                                                         musx::dom::Cmper{1});
        report.setField(instance, "altHideOtherArtics",
                        {ValueOrigin::LegacyMus, 10, 20, 0, std::uint16_t{0x00e8}});
        report.setField(instance, "altHideOtherSmartShapes",
                        {ValueOrigin::LegacyMus, 10,
                         aggregateLayout ? std::size_t{20} : std::size_t{22}, 0,
                         std::uint16_t{0x00e8}});
        ComparisonPreparationContext context{
            source, companion, transformations, FormatEpoch::ZlibLegacy, &version, &report};
        runComparisonPreparers(context);
        return !source.contains("staff_style") && !companion.contains("staff_style");
    };

    const SourceVersion finale2009Beta{.major = finale_mus_reader::versions::finale2009.major,
                                       .devStatus = 2};
    const SourceVersion finale2009Release{.major = finale_mus_reader::versions::finale2009.major,
                                          .devStatus = 4};
    const SourceVersion finale2010Beta{.major = finale_mus_reader::versions::finale2010.major,
                                       .devStatus = 2};

    CHECK(comparisonWasSkipped(finale2009Beta, true));
    CHECK_FALSE(comparisonWasSkipped(finale2009Beta, false));
    CHECK_FALSE(comparisonWasSkipped(finale2009Release, true));
    CHECK_FALSE(comparisonWasSkipped(finale2010Beta, true));
}

TEST_CASE("StaffStyle menu metadata does not affect semantic coverage", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto classify = differenceClassifier("staff_style");
    REQUIRE(classify);
    const ComparisonLeaves leaves;
    ImportReport report(FormatEpoch::ZlibLegacy);
    const auto classifyBoolean = [&](std::string_view path, bool source, bool companion,
                                     DifferenceCategory category = DifferenceCategory::Differs) {
        return classify(DifferenceContext{path, category, "legacy-mus", Value(source),
                                          Value(companion), leaves, leaves, FormatEpoch::ZlibLegacy,
                                          ByteOrder::BigEndian, nullptr, report});
    };

    for (const auto suffix : {".copyable", ".add_to_menu"})
    {
        CHECK(classifyBoolean(std::string("staff_style[cmper=5]") + suffix, true, false) ==
              DifferenceClassification::DifferentDefaults);
        CHECK(classifyBoolean(std::string("staff_style[cmper=5]") + suffix, false, true) ==
              DifferenceClassification::DifferentDefaults);
    }
    CHECK_FALSE(classifyBoolean("staff_style[cmper=5].copyable", true, false,
                                DifferenceCategory::ReaderOnly));
    CHECK_FALSE(classifyBoolean("staff[cmper=5].copyable", true, false));
    CHECK_FALSE(classifyBoolean("staff_style[cmper=5].copyable_extra", true, false));
}

TEST_CASE("Pre-Finale 2012 instrument StaffStyles acquire every required mask", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto classify = differenceClassifier("staff_style");
    REQUIRE(classify);
    const auto objectPath = std::string("staff_style[cmper=1]");
    ComparisonLeaves source{
        {objectPath + ".inst_uuid",
         {Value(std::string(musx::dom::uuid::Unknown)), "legacy-behavior"}},
        {objectPath + ".notation_style",
         {Value(static_cast<std::int64_t>(Staff::NotationStyle::Standard)), "legacy-mus"}},
        {objectPath + ".default_clef", {Value(std::int64_t(3)), "legacy-mus"}},
        {objectPath + ".show_note_colors", {Value(false), "legacy-mus"}},
        {objectPath + ".hide_key_sigs_show_accis", {Value(false), "legacy-mus"}},
        {objectPath + ".transposition.present", {Value(true), "legacy-mus"}},
        {objectPath + ".transposition.chromatic.alteration",
         {Value(std::int64_t(0)), "legacy-mus"}},
        {objectPath + ".transposition.chromatic.diatonic", {Value(std::int64_t(3)), "legacy-mus"}},
        {objectPath + ".style_name", {Value("Source"), "legacy-mus"}},
        {objectPath + ".masks.notation_style", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.default_clef", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.show_note_colors", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.hide_key_sigs_show_accis", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.staff_type", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.transposition", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.full_name", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.abrv_name", {Value(true), "legacy-mus"}}};
    auto companion = source;
    companion.at(objectPath + ".inst_uuid").first = Value(std::string(musx::dom::uuid::Flute));
    companion.at(objectPath + ".masks.notation_style").first = Value(true);
    companion.at(objectPath + ".masks.show_note_colors").first = Value(true);
    companion.at(objectPath + ".masks.hide_key_sigs_show_accis").first = Value(true);
    ImportReport report(FormatEpoch::DclLegacy);
    const SourceVersion finale2006{.major = finale_mus_reader::versions::finale2006.major};
    const SourceVersion finale2012{.major = finale_mus_reader::versions::finale2012.major};
    const auto context = [&](std::string_view path, const ComparisonLeaves &sourceLeaves,
                             const ComparisonLeaves &companionLeaves,
                             std::string_view origin = "legacy-mus") {
        return DifferenceContext{path,
                                 DifferenceCategory::Differs,
                                 origin,
                                 sourceLeaves.at(std::string(path)).first,
                                 companionLeaves.at(std::string(path)).first,
                                 sourceLeaves,
                                 companionLeaves,
                                 FormatEpoch::DclLegacy,
                                 ByteOrder::BigEndian,
                                 &finale2006,
                                 report};
    };

    for (const auto suffix :
         {".masks.notation_style", ".masks.show_note_colors", ".masks.hide_key_sigs_show_accis"})
    {
        const auto path = objectPath + suffix;
        CHECK(classify(context(path, source, companion)) ==
              DifferenceClassification::FinaleUpgradeNormalization);
    }
    const auto uuidPath = objectPath + ".inst_uuid";
    CHECK(classify(context(uuidPath, source, companion, "legacy-behavior")) ==
          DifferenceClassification::FinaleUpgradeNormalization);

    auto changedValue = companion;
    changedValue.at(objectPath + ".show_note_colors").first = Value(true);
    CHECK(classify(context(objectPath + ".masks.show_note_colors", source, changedValue)) ==
          DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classify(context(objectPath + ".show_note_colors", source, changedValue)) ==
          DifferenceClassification::FinaleUpgradeNormalization);

    auto changedTransposition = companion;
    changedTransposition.at(objectPath + ".transposition.present").first = Value(false);
    changedTransposition.at(objectPath + ".transposition.chromatic.alteration").first =
        Value(std::int64_t(-1));
    changedTransposition.at(objectPath + ".transposition.chromatic.diatonic").first =
        Value(std::int64_t(4));
    for (const auto suffix : {".transposition.present", ".transposition.chromatic.alteration",
                              ".transposition.chromatic.diatonic"})
    {
        CHECK(classify(context(objectPath + suffix, source, changedTransposition)) ==
              DifferenceClassification::FinaleUpgradeNormalization);
    }

    auto dormantTransposition = source;
    dormantTransposition.at(objectPath + ".masks.transposition").first = Value(false);
    CHECK(classify(context(objectPath + ".transposition.present", dormantTransposition,
                           changedTransposition)) ==
          DifferenceClassification::FinaleUpgradeNormalization);

    auto changedName = companion;
    changedName.at(objectPath + ".style_name").first = Value("Companion");
    CHECK(classify(context(objectPath + ".style_name", source, changedName)) ==
          DifferenceClassification::FinaleUpgradeLoss);

    auto incompleteCompanion = companion;
    incompleteCompanion.at(objectPath + ".masks.hide_key_sigs_show_accis").first = Value(false);
    CHECK(classify(context(objectPath + ".masks.notation_style", source, incompleteCompanion)) ==
          DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classify(context(uuidPath, source, incompleteCompanion, "legacy-behavior")) ==
          DifferenceClassification::FinaleUpgradeNormalization);

    auto sourceWithoutInstrumentMasks = source;
    sourceWithoutInstrumentMasks.at(objectPath + ".masks.default_clef").first = Value(false);
    CHECK(classify(context(objectPath + ".masks.notation_style", sourceWithoutInstrumentMasks,
                           companion)) == DifferenceClassification::FinaleUpgradeNormalization);

    ComparisonLeaves pluginSource{
        {objectPath + ".masks.notation_style", {Value(true), "legacy-mus"}},
        {objectPath + ".masks.default_clef", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.show_note_colors", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.hide_key_sigs_show_accis", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.staff_type", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.transposition", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.full_name", {Value(false), "legacy-mus"}},
        {objectPath + ".masks.abrv_name", {Value(false), "legacy-mus"}},
        {objectPath + ".default_clef", {Value(std::int64_t(0)), "legacy-mus"}},
        {objectPath + ".show_note_colors", {Value(false), "legacy-mus"}},
        {objectPath + ".full_name_text_id", {Value(std::int64_t(0)), "legacy-mus"}},
        {objectPath + ".abbrv_name_text_id", {Value(std::int64_t(0)), "legacy-mus"}}};
    auto pluginCompanion = pluginSource;
    for (const auto suffix : {".masks.default_clef", ".masks.show_note_colors", ".masks.staff_type",
                              ".masks.transposition", ".masks.full_name", ".masks.abrv_name"})
    {
        pluginCompanion.at(objectPath + suffix).first = Value(true);
    }
    pluginCompanion.at(objectPath + ".default_clef").first = Value(std::int64_t(12));
    pluginCompanion.at(objectPath + ".full_name_text_id").first = Value(std::int64_t(326));
    pluginCompanion.at(objectPath + ".abbrv_name_text_id").first = Value(std::int64_t(328));
    for (const auto suffix : {".masks.default_clef", ".masks.show_note_colors", ".masks.staff_type",
                              ".masks.transposition", ".masks.full_name", ".masks.abrv_name",
                              ".default_clef", ".full_name_text_id", ".abbrv_name_text_id"})
    {
        CHECK(classify(context(objectPath + suffix, pluginSource, pluginCompanion)) ==
              DifferenceClassification::FinaleUpgradeNormalization);
    }

    auto incompletePluginCompanion = pluginCompanion;
    incompletePluginCompanion.at(objectPath + ".masks.abrv_name").first = Value(false);
    CHECK_FALSE(classify(
        context(objectPath + ".masks.default_clef", pluginSource, incompletePluginCompanion)));

    auto post2012 = context(objectPath + ".masks.notation_style", source, companion);
    post2012.epoch = FormatEpoch::ZlibLegacy;
    post2012.sourceVersion = &finale2012;
    CHECK_FALSE(classify(post2012));

    auto removedSource = source;
    removedSource.emplace(objectPath + ".staff_lines",
                          std::pair{Value(std::int64_t(0)), "legacy-mus"});
    removedSource.emplace(objectPath + ".transposed_clef",
                          std::pair{Value(std::int64_t(4)), "legacy-mus"});
    removedSource.emplace(objectPath + ".transposition.present",
                          std::pair{Value(true), "legacy-mus"});
    removedSource.emplace(objectPath + ".full_name_text_id",
                          std::pair{Value(std::int64_t(311)), "legacy-mus"});
    removedSource.emplace(objectPath + ".abbrv_name_text_id",
                          std::pair{Value(std::int64_t(312)), "legacy-mus"});
    auto removedCompanion = removedSource;
    for (const auto suffix :
         {".masks.notation_style", ".masks.default_clef", ".masks.show_note_colors",
          ".masks.hide_key_sigs_show_accis", ".masks.staff_type", ".masks.transposition",
          ".masks.full_name", ".masks.abrv_name"})
    {
        removedCompanion.at(objectPath + suffix).first = Value(false);
    }
    removedCompanion.at(objectPath + ".default_clef").first = Value(std::int64_t(0));
    removedCompanion.at(objectPath + ".staff_lines").first = Value(std::int64_t(5));
    removedCompanion.at(objectPath + ".transposed_clef").first = Value(std::int64_t(0));
    removedCompanion.at(objectPath + ".transposition.present").first = Value(false);
    removedCompanion.at(objectPath + ".full_name_text_id").first = Value(std::int64_t(0));
    removedCompanion.at(objectPath + ".abbrv_name_text_id").first = Value(std::int64_t(0));
    for (const auto suffix :
         {".masks.default_clef", ".masks.staff_type", ".masks.transposition", ".masks.full_name",
          ".masks.abrv_name", ".default_clef", ".staff_lines", ".transposed_clef",
          ".transposition.present", ".full_name_text_id", ".abbrv_name_text_id"})
    {
        CHECK(classify(context(objectPath + suffix, removedSource, removedCompanion)) ==
              DifferenceClassification::FinaleUpgradeNormalization);
    }

    auto removedPost2012 =
        context(objectPath + ".full_name_text_id", removedSource, removedCompanion);
    removedPost2012.epoch = FormatEpoch::ZlibLegacy;
    removedPost2012.sourceVersion = &finale2012;
    CHECK_FALSE(classify(removedPost2012));
}

TEST_CASE("Pre-Finale 2012 mixed StaffStyles split into companion styles", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto classify = differenceClassifier("staff_style");
    REQUIRE(classify);
    constexpr std::string_view retained = "staff_style[cmper=5]";
    constexpr std::string_view split = "staff_style[cmper=6]";
    const ComparisonLeaves source{
        {std::string(retained) + ".style_name", {Value("Mixed"), "legacy-mus"}},
        {std::string(retained) + ".masks.flat_beams", {Value(true), "legacy-mus"}},
        {std::string(retained) + ".masks.notation_style", {Value(true), "legacy-mus"}},
        {std::string(retained) + ".masks.default_clef", {Value(true), "legacy-mus"}},
        {std::string(retained) + ".masks.show_note_colors", {Value(false), "legacy-mus"}},
        {std::string(retained) + ".masks.hide_key_sigs_show_accis", {Value(false), "legacy-mus"}},
        {std::string(retained) + ".masks.staff_type", {Value(true), "legacy-mus"}},
        {std::string(retained) + ".masks.transposition", {Value(true), "legacy-mus"}},
        {std::string(retained) + ".masks.float_notehead_font", {Value(true), "legacy-mus"}},
        {std::string(retained) + ".masks.no_key", {Value(true), "legacy-mus"}},
        {std::string(retained) + ".notation_style",
         {Value(static_cast<std::int64_t>(Staff::NotationStyle::Percussion)), "legacy-mus"}},
        {std::string(retained) + ".default_clef", {Value(std::int64_t(12)), "legacy-mus"}},
        {std::string(retained) + ".show_note_colors", {Value(false), "legacy-mus"}},
        {std::string(retained) + ".hide_key_sigs_show_accis", {Value(false), "legacy-mus"}},
        {std::string(retained) + ".transposition.present", {Value(true), "legacy-mus"}},
        {std::string(retained) + ".custom_staff[0]", {Value(std::int64_t(13)), "legacy-mus"}},
        {std::string(retained) + ".use_note_font", {Value(true), "legacy-mus"}},
        {std::string(retained) + ".no_key", {Value(true), "legacy-mus"}},
        {std::string(retained) + ".bot_barline_offset", {Value(-24), "legacy-mus"}},
        {std::string(retained) + ".dw_rest_offset", {Value(-5), "legacy-mus"}},
        {std::string(retained) + ".staff_lines", {Value(0), "legacy-mus"}},
        {std::string(retained) + ".top_barline_offset", {Value(24), "legacy-mus"}},
        {std::string(retained) + ".w_rest_offset", {Value(-6), "legacy-mus"}},
        {std::string(retained) + ".line_space", {Value(24), "legacy-mus"}}};
    ComparisonLeaves companion{
        {std::string(retained) + ".style_name", {Value("Mixed "), {}}},
        {std::string(retained) + ".masks.flat_beams", {Value(true), {}}},
        {std::string(retained) + ".masks.notation_style", {Value(false), {}}},
        {std::string(retained) + ".masks.default_clef", {Value(false), {}}},
        {std::string(retained) + ".masks.show_note_colors", {Value(false), {}}},
        {std::string(retained) + ".masks.hide_key_sigs_show_accis", {Value(false), {}}},
        {std::string(retained) + ".masks.staff_type", {Value(false), {}}},
        {std::string(retained) + ".masks.transposition", {Value(false), {}}},
        {std::string(retained) + ".masks.float_notehead_font", {Value(false), {}}},
        {std::string(retained) + ".masks.no_key", {Value(false), {}}},
        {std::string(retained) + ".notation_style",
         {Value(static_cast<std::int64_t>(Staff::NotationStyle::Standard)), {}}},
        {std::string(retained) + ".default_clef", {Value(std::int64_t(0)), {}}},
        {std::string(retained) + ".show_note_colors", {Value(false), {}}},
        {std::string(retained) + ".hide_key_sigs_show_accis", {Value(false), {}}},
        {std::string(retained) + ".transposition.present", {Value(false), {}}},
        {std::string(retained) + ".custom_staff[0]", {Value(std::int64_t(11)), {}}},
        {std::string(retained) + ".use_note_font", {Value(true), {}}},
        {std::string(retained) + ".no_key", {Value(true), {}}},
        {std::string(retained) + ".bot_barline_offset", {Value(0), {}}},
        {std::string(retained) + ".dw_rest_offset", {Value(-4), {}}},
        {std::string(retained) + ".staff_lines", {Value(5), {}}},
        {std::string(retained) + ".top_barline_offset", {Value(0), {}}},
        {std::string(retained) + ".w_rest_offset", {Value(-4), {}}},
        {std::string(retained) + ".line_space", {Value(25), {}}},
        {std::string(split) + ".style_name", {Value("Mixed "), {}}},
        {std::string(split) + ".masks.notation_style", {Value(true), {}}},
        {std::string(split) + ".masks.default_clef", {Value(true), {}}},
        {std::string(split) + ".masks.show_note_colors", {Value(true), {}}},
        {std::string(split) + ".masks.hide_key_sigs_show_accis", {Value(true), {}}},
        {std::string(split) + ".masks.staff_type", {Value(true), {}}},
        {std::string(split) + ".masks.transposition", {Value(true), {}}},
        {std::string(split) + ".masks.full_name", {Value(true), {}}},
        {std::string(split) + ".masks.abrv_name", {Value(true), {}}},
        {std::string(split) + ".notation_style",
         {Value(static_cast<std::int64_t>(Staff::NotationStyle::Percussion)), {}}},
        {std::string(split) + ".default_clef", {Value(std::int64_t(12)), {}}},
        {std::string(split) + ".show_note_colors", {Value(false), {}}},
        {std::string(split) + ".hide_key_sigs_show_accis", {Value(false), {}}},
        {std::string(split) + ".transposition.present", {Value(true), {}}},
        {std::string(split) + ".custom_staff[0]", {Value(std::int64_t(13)), {}}},
        {std::string(split) + ".bot_barline_offset", {Value(-24), {}}},
        {std::string(split) + ".dw_rest_offset", {Value(-5), {}}},
        {std::string(split) + ".staff_lines", {Value(0), {}}},
        {std::string(split) + ".top_barline_offset", {Value(24), {}}},
        {std::string(split) + ".w_rest_offset", {Value(-6), {}}},
        {"staff_style[cmper=7].use_note_font", {Value(true), {}}}};
    ImportReport report(FormatEpoch::ZlibLegacy);
    const SourceVersion finale2011{.major = finale_mus_reader::versions::finale2011.major};
    const SourceVersion finale2012{.major = finale_mus_reader::versions::finale2012.major};
    const auto contextFrom = [&](std::string_view path, const ComparisonLeaves &sourceLeaves,
                                 const ComparisonLeaves &companionLeaves,
                                 const SourceVersion *version) {
        return DifferenceContext{path,
                                 DifferenceCategory::Differs,
                                 "legacy-mus",
                                 sourceLeaves.at(std::string(path)).first,
                                 companionLeaves.at(std::string(path)).first,
                                 sourceLeaves,
                                 companionLeaves,
                                 FormatEpoch::ZlibLegacy,
                                 ByteOrder::BigEndian,
                                 version,
                                 report};
    };
    const auto context = [&](std::string_view path, const ComparisonLeaves &companionLeaves,
                             const SourceVersion *version) {
        return contextFrom(path, source, companionLeaves, version);
    };

    for (const auto suffix :
         {".masks.notation_style", ".masks.default_clef", ".masks.staff_type",
          ".masks.transposition", ".bot_barline_offset", ".dw_rest_offset", ".staff_lines",
          ".top_barline_offset", ".transposition.present", ".custom_staff[0]", ".w_rest_offset"})
    {
        CHECK(classify(context(std::string(retained) + suffix, companion, &finale2011)) ==
              DifferenceClassification::FinaleUpgradeNormalization);
    }
    CHECK_FALSE(classify(context(std::string(retained) + ".line_space", companion, &finale2011)));
    CHECK_FALSE(classify(context(std::string(retained) + ".staff_lines", companion, &finale2012)));

    for (const auto suffix : {".masks.float_notehead_font", ".masks.no_key"})
    {
        CHECK(classify(context(std::string(retained) + suffix, companion, &finale2011)) ==
              DifferenceClassification::FinaleUpgradeNormalization);
    }

    auto sourceWithoutFormerGate = source;
    sourceWithoutFormerGate.at(std::string(retained) + ".masks.notation_style").first =
        Value(false);
    sourceWithoutFormerGate.at(std::string(retained) + ".masks.staff_type").first = Value(false);
    CHECK(classify(contextFrom(std::string(retained) + ".transposition.present",
                               sourceWithoutFormerGate, companion, &finale2011)) ==
          DifferenceClassification::FinaleUpgradeNormalization);

    auto mismatchedFork = companion;
    mismatchedFork.at(std::string(split) + ".custom_staff[0]").first = Value(std::int64_t(12));
    CHECK_FALSE(
        classify(context(std::string(retained) + ".custom_staff[0]", mismatchedFork, &finale2011)));

    auto noRetainedStyleSource = source;
    noRetainedStyleSource.at(std::string(retained) + ".masks.flat_beams").first = Value(false);
    CHECK_FALSE(classify(contextFrom(std::string(retained) + ".staff_lines", noRetainedStyleSource,
                                     companion, &finale2011)));

    auto incompleteSplit = companion;
    incompleteSplit.erase(std::string(split) + ".masks.staff_type");
    CHECK_FALSE(
        classify(context(std::string(retained) + ".staff_lines", incompleteSplit, &finale2011)));

    const Value absent;
    const DifferenceContext synthesizedNoteFont{
        "staff_style[cmper=7].use_note_font",
        DifferenceCategory::CompanionOnly,
        {},
        absent,
        companion.at("staff_style[cmper=7].use_note_font").first,
        source,
        companion,
        FormatEpoch::ZlibLegacy,
        ByteOrder::BigEndian,
        &finale2011,
        report};
    CHECK_FALSE(classify(synthesizedNoteFont));
}

TEST_CASE("Inactive StaffStyle overrides make governed values different "
          "defaults",
          "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto classify = differenceClassifier("staff_style");
    REQUIRE(classify);
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const auto classifyValue = [&](std::string_view valueSuffix, std::string_view maskSuffix,
                                   bool sourceMask = false, bool companionMask = false,
                                   DifferenceCategory category = DifferenceCategory::Differs,
                                   std::string_view classPrefix = "staff_style[cmper=1]") {
        const auto valuePath = std::string(classPrefix) + std::string(valueSuffix);
        const auto maskPath = std::string(classPrefix) + std::string(maskSuffix);
        const Value sourceValue(std::int64_t(1));
        const Value companionValue(std::int64_t(2));
        const ComparisonLeaves source{{maskPath, {Value(sourceMask), "legacy-mus"}}};
        const ComparisonLeaves companion{{maskPath, {Value(companionMask), {}}}};
        return classify(DifferenceContext{
            valuePath, category, "finale27-default", sourceValue, companionValue, source, companion,
            FormatEpoch::UncompressedLegacy, ByteOrder::BigEndian, nullptr, report});
    };

    constexpr std::pair<std::string_view, std::string_view> governedValues[] = {
        {".note_font.font_size", ".masks.float_notehead_font"},
        {".use_note_font", ".masks.float_notehead_font"},
        {".use_note_shapes", ".masks.use_note_shapes"},
        {".fret_inst_id", ".masks.notation_style"},
        {".line_space", ".masks.staff_type"},
        {".transposition.keysig.present", ".masks.transposition"},
        {".alt_hide_expressions", ".masks.alt_notation"},
        {".vert_stem_end_off_down", ".masks.show_stems"},
        {".hide_meas_nums", ".masks.neg_mnumb"},
        {".full_name_text_id", ".masks.full_name"},
        {".hide_ties", ".masks.show_ties"},
        {".hide_key_sigs_show_accis", ".masks.hide_key_sigs_show_accis"},
    };
    for (const auto &[valueSuffix, maskSuffix] : governedValues)
    {
        CHECK(classifyValue(valueSuffix, maskSuffix) ==
              DifferenceClassification::DifferentDefaults);
    }

    CHECK_FALSE(classifyValue(".line_space", ".masks.staff_type", true, false));
    CHECK_FALSE(classifyValue(".line_space", ".masks.staff_type", false, true));
    CHECK_FALSE(classifyValue(".line_space", ".masks.staff_type", true, true));
    CHECK_FALSE(classifyValue(".line_space", ".masks.staff_type", false, false,
                              DifferenceCategory::ReaderOnly));
    CHECK_FALSE(classifyValue(".style_name", ".masks.staff_type"));
    CHECK_FALSE(classifyValue(".line_space", ".masks.staff_type", false, false,
                              DifferenceCategory::Differs, "staff[cmper=1]"));

    const auto objectPath = std::string("staff_style[cmper=1]");
    const auto sizePath = objectPath + ".note_font.font_size";
    const auto useFontPath = objectPath + ".use_note_font";
    const auto maskPath = objectPath + ".masks.float_notehead_font";
    const Value storedZero(std::int64_t(0));
    const Value materializedSize(std::int64_t(24));
    ComparisonLeaves source{{useFontPath, {Value(false), "legacy-mus"}},
                            {maskPath, {Value(true), "legacy-mus"}}};
    const ComparisonLeaves companion{{useFontPath, {Value(false), {}}},
                                     {maskPath, {Value(true), {}}}};
    const auto disabledFontContext = [&] {
        return DifferenceContext{sizePath,
                                 DifferenceCategory::Differs,
                                 "legacy-mus",
                                 storedZero,
                                 materializedSize,
                                 source,
                                 companion,
                                 FormatEpoch::UncompressedLegacy,
                                 ByteOrder::BigEndian,
                                 nullptr,
                                 report};
    };
    CHECK(classify(disabledFontContext()) == DifferenceClassification::DifferentDefaults);
    source.at(useFontPath).first = Value(true);
    CHECK_FALSE(classify(disabledFontContext()));
}

} // namespace
} // namespace finale_mus_reader_tests
