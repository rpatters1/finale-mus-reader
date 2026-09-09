// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"
#include "coverage/comparison.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

using PercussionNoteInfoTestTarget = musx::dom::others::PercussionNoteInfo;

musx::dom::DocumentPtr makePercussionNoteInfoDocument() {
    using FontDefinition = musx::dom::others::FontDefinition;
    using FontOptions = musx::dom::options::FontOptions;

    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto definition = std::make_shared<FontDefinition>(document, musx::dom::SCORE_PARTID,
                                                       musx::dom::EnigmaBase::ShareMode::All,
                                                       musx::dom::Cmper(5));
    definition->charsetBank = FontDefinition::CharacterSetBank::MacOS;
    definition->charsetVal = 0;
    document->getOthers()->add(FontDefinition::XmlNodeName, std::move(definition));

    auto options = std::make_shared<FontOptions>(document);
    auto percussionFont = std::make_shared<musx::dom::FontInfo>(document);
    percussionFont->fontId = 5;
    options->fontOptions[FontOptions::FontType::Percussion] = std::move(percussionFont);
    document->getOptions()->add(FontOptions::XmlNodeName, std::move(options));
    return document;
}

ImportReport importPercussionNoteInfo(const finale_mus_reader::container::ParsedContainer &parsed,
                                      const SourceProfile &profile,
                                      const musx::dom::DocumentPtr &document) {
    ImportReport report(profile.epoch);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{LegacyRecordIndex::build(parsed),
                                                   profile,
                                                   noSource,
                                                   document,
                                                   reference,
                                                   report,
                                                   pending,
                                                   construction};
    finale_mus_reader::others::importPercussionNoteInfo(context);
    return report;
}

TEST_CASE("Finale 2010 percussion-note collections use six narrow words", "[class]") {
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        auto profile = profileFor(15);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = byteOrder;
        const auto document = makePercussionNoteInfoDocument();
        const auto report = importPercussionNoteInfo(
            makeClassContainer(0x0139, {35, 7, 0x00c7, 48, 49, 50, 4131, -2, 51, 52, 53, 54},
                               byteOrder, 9),
            profile, document);

        const auto first = document->getOthers()->get<PercussionNoteInfoTestTarget>(
            musx::dom::SCORE_PARTID, 9, musx::dom::Inci(0));
        const auto second = document->getOthers()->get<PercussionNoteInfoTestTarget>(
            musx::dom::SCORE_PARTID, 9, musx::dom::Inci(1));
        REQUIRE(first);
        CHECK(first->percNoteType == 35);
        CHECK(first->staffPosition == 7);
        CHECK(first->closedNotehead == U'\u00ab');
        CHECK(first->halfNotehead == U'0');
        CHECK(first->wholeNotehead == U'1');
        CHECK(first->dwholeNotehead == U'2');
        REQUIRE(second);
        CHECK(second->percNoteType == 4131);
        CHECK(second->staffPosition == -2);
        CHECK(second->closedNotehead == U'3');
        CHECK(second->dwholeNotehead == U'6');
        CHECK(reportedFieldCount(report) == 12);
        CHECK(field(report, "others.percussionNoteInfo[9,0].closedNotehead").origin ==
              ValueOrigin::LegacyMus);
        CHECK(field(report, "others.percussionNoteInfo[9,1].staffPosition").rawValue == -2);
    }
}

TEST_CASE("Finale 2012 percussion-note collections use wide codepoints and padding", "[class]") {
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        auto profile = profileFor(17);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = byteOrder;
        const auto document = makePercussionNoteInfoDocument();
        const auto report =
            importPercussionNoteInfo(makeClassContainer(0x0139,
                                                        {38, 5, std::int16_t(-2494),
                                                         1, 0x00fa, 0, 0x00db, 0, 0x00c0, 0, 0, 0},
                                                        byteOrder, 4),
                                     profile, document);

        const auto note = document->getOthers()->get<PercussionNoteInfoTestTarget>(
            musx::dom::SCORE_PARTID, 4, musx::dom::Inci(0));
        REQUIRE(note);
        CHECK(note->percNoteType == 38);
        CHECK(note->staffPosition == 5);
        CHECK(note->closedNotehead == U'\U0001f642');
        CHECK(note->halfNotehead == char32_t{0xfa});
        CHECK(note->wholeNotehead == char32_t{0xdb});
        CHECK(note->dwholeNotehead == char32_t{0xc0});
        CHECK(reportedFieldCount(report) == 6);
        CHECK(field(report, "others.percussionNoteInfo[4,0].closedNotehead").rawValue == 0x1f642);
    }
}

TEST_CASE("Fixed-row percussion maps use DS selection in both storage epochs", "[class]") {
    for (const auto epoch : {FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            auto parsed = makeContainer({{7, "DS", {1, 0, 0, 0, 0x7000, 1}}}, epoch, byteOrder);
            for (const auto &[mapId, midiKey, words] :
                 std::vector<std::tuple<std::uint16_t, std::uint16_t, std::vector<std::int16_t>>>{
                     {1, 60, {60, 7, 207, 250, 0}},
                     {1, 61, {61, 0, 207, 250, 0}},
                     {1, 62, {62, 8, 120, 89, 0}},
                     {1, 63, {63, 9, 120, 89, 0}},
                     {1, 64, {60, 10, 121, 89, 0}},
                     {2, 60, {60, 9, 207, 250, 0}}}) {
                auto extra = makeDetailContainer(epoch, mapId, midiKey, words, "DF", byteOrder);
                parsed.blocks.push_back(std::move(extra.blocks.front()));
            }
            auto profile = SourceProfile(epoch);
            profile.byteOrder = byteOrder;
            const auto document = makePercussionNoteInfoDocument();
            const auto report = importPercussionNoteInfo(parsed, profile, document);

            const auto notes = document->getOthers()->getArray<PercussionNoteInfoTestTarget>(
                musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
            REQUIRE(notes.size() == 4);
            CHECK(notes[0]->percNoteType == 32);
            CHECK(notes[0]->staffPosition == 7);
            CHECK(notes[0]->closedNotehead == U'\u0153');
            CHECK(notes[0]->halfNotehead == U'\u02d9');
            CHECK(notes[0]->wholeNotehead == U'\u02d9');
            CHECK(notes[0]->dwholeNotehead == U'\u02d9');
            CHECK(notes[1]->staffPosition == 0);
            CHECK(notes[1]->closedNotehead == U'\u0153');
            CHECK(notes[2]->percNoteType == 37);
            CHECK(notes[2]->staffPosition == 8);
            CHECK(notes[2]->closedNotehead == U'x');
            CHECK(notes[2]->halfNotehead == U'Y');
            CHECK(notes[2]->wholeNotehead == U'Y');
            CHECK(notes[2]->dwholeNotehead == U'Y');
            CHECK(notes[3]->percNoteType == 0x1020);
            CHECK(notes[3]->staffPosition == 10);
            CHECK(reportedFieldCount(report) == 24);
            CHECK(field(report, "others.percussionNoteInfo[1,0].percNoteType").origin ==
                  ValueOrigin::LegacyBehavior);
            CHECK(field(report, "others.percussionNoteInfo[1,0].staffPosition").origin ==
                  ValueOrigin::LegacyMus);
            CHECK(field(report, "others.percussionNoteInfo[1,0].wholeNotehead").origin ==
                  ValueOrigin::LegacyBehavior);
        }
    }
}

TEST_CASE("Finale 2008 retains DF rows and DS selection as zlib classes", "[class]") {
    const auto unassigned =
        readFixture("evidence/F2008/F2008-percussion-staff.mus", fixtureLegacySymbolFonts);
    CHECK(unassigned.document->getOthers()
              ->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID,
                                                       musx::dom::Cmper(1))
              .empty());

    const auto assigned =
        readFixture("evidence/F2008/F2008-percussion-staff-edit.mus", fixtureLegacySymbolFonts);
    const auto notes = assigned.document->getOthers()->getArray<PercussionNoteInfoTestTarget>(
        musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    REQUIRE(notes.size() == 1);
    CHECK(notes.front()->percNoteType == 32);
    CHECK(notes.front()->staffPosition == 6);
    CHECK(std::uint32_t(notes.front()->closedNotehead) == 208);
    CHECK(std::uint32_t(notes.front()->halfNotehead) == 194);
    CHECK(std::uint32_t(notes.front()->wholeNotehead) == 194);
    CHECK(std::uint32_t(notes.front()->dwholeNotehead) == 194);
}

TEST_CASE("Reusable readers apply the first supplied named percussion table", "[class][reader]") {
    const auto reader = [] {
        const std::string first = R"xml(
            <FinaleNameDocument><MasterDeviceNames>
            <NoteNameList Name="Percussion Map 1">
            <Note Number="60" PercNoteType="236" />
            </NoteNameList></MasterDeviceNames></FinaleNameDocument>)xml";
        const std::string duplicate = R"xml(
            <FinaleNameDocument><MasterDeviceNames>
            <NoteNameList Name="Percussion Map 1">
            <Note Number="60" PercNoteType="5" />
            </NoteNameList></MasterDeviceNames></FinaleNameDocument>)xml";
        const auto bytes = [](const std::string &value) {
            return std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t *>(value.data()), value.size());
        };
        const std::string symbolFontContents(fixtureLegacySymbolFonts);
        const finale_mus_reader::ReaderOptions options{
            bytes(symbolFontContents), {bytes(first), bytes(duplicate)}};
        return finale_mus_reader::Reader::create<TestXmlDocument>(options);
    }();

    for (int readCount = 0; readCount < 2; ++readCount) {
        const auto result = reader.readWithReport(
            std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) /
            "evidence/F2008/F2008-percussion-staff-edit.mus");
        const auto notes = result.document->getOthers()->getArray<PercussionNoteInfoTestTarget>(
            musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
        REQUIRE(notes.size() == 1);
        CHECK(notes.front()->percNoteType == 236);
        CHECK(field(result.report, "others.percussionNoteInfo[1,0].percNoteType").rawValue == 60);
    }
}

TEST_CASE("An unreferenced DCL DF map is not constructed", "[class]") {
    const auto result =
        readFixture("evidence/F2006/F2006-linked-tiff.mus", fixtureLegacySymbolFonts);
    const auto notes = result.document->getOthers()->getArray<PercussionNoteInfoTestTarget>(
        musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    CHECK(notes.empty());
}

TEST_CASE("Coda-banner files predate percussion maps", "[class]") {
    auto parsed = makeContainer({{7, "DS", {1, 0, 0, 0, 0x1000, 0}}}, FormatEpoch::CodaBanner);
    auto detail = makeDetailContainer(FormatEpoch::CodaBanner, 1, 60, {60, 7, 207, 250, 0}, "DF");
    parsed.blocks.push_back(std::move(detail.blocks.front()));
    auto profile = SourceProfile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto document = makePercussionNoteInfoDocument();
    const auto report = importPercussionNoteInfo(parsed, profile, document);
    CHECK(document->getOthers()
              ->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID)
              .empty());
    CHECK(reportedFieldCount(report) == 0);
}

TEST_CASE("Finale 98 DF maps without a staff selection are not constructed", "[class]") {
    const auto result = readFixture("evidence/F98/F98-baseline.mus", fixtureLegacySymbolFonts);
    CHECK(result.document->getOthers()
              ->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID)
              .empty());
}

TEST_CASE("Coverage drops only unreferenced companion percussion maps", "[coverage][percussion]") {
    using namespace finale_mus_reader::coverage;
    const auto drumStaff = [](std::int64_t mapId) {
        return Value::Object{{"cmper", 1}, {"which_drum_lib", mapId}};
    };
    const auto mapNote = [](std::int64_t mapId) {
        return Value::Object{{"cmper", mapId}, {"inci", 0}};
    };

    SECTION("active uncompressed map") {
        SurveySnapshot source{
            {"drum_staff", Value::Array{drumStaff(1)}},
            {"percussion_note_info", Value::Array{}},
        };
        SurveySnapshot companion{
            {"drum_staff", Value::Array{drumStaff(1)}},
            {"percussion_note_info", Value::Array{mapNote(1), mapNote(2)}},
        };
        std::map<ComparisonTransformation, std::uint64_t> transformations;
        ComparisonPreparationContext context{source, companion, transformations,
                                             FormatEpoch::UncompressedLegacy};
        runComparisonPreparers(context);

        const auto &notes = companion.at("percussion_note_info").asArray();
        REQUIRE(notes.size() == 1);
        CHECK(notes.front().find("cmper")->asInteger() == 1);
        CHECK(transformations[ComparisonTransformation::FinaleSynthesizedPercussionMapNote] == 1);
    }

    SECTION("source-defined map") {
        SurveySnapshot source{
            {"percussion_note_info", Value::Array{mapNote(3)}},
        };
        SurveySnapshot companion{
            {"percussion_note_info", Value::Array{mapNote(3), mapNote(4)}},
        };
        std::map<ComparisonTransformation, std::uint64_t> transformations;
        ComparisonPreparationContext context{source, companion, transformations,
                                             FormatEpoch::DclLegacy};
        runComparisonPreparers(context);

        const auto &notes = companion.at("percussion_note_info").asArray();
        REQUIRE(notes.size() == 1);
        CHECK(notes.front().find("cmper")->asInteger() == 3);
        CHECK(transformations[ComparisonTransformation::FinaleSynthesizedPercussionMapNote] == 1);
    }

    SECTION("source-defined zlib map") {
        SurveySnapshot source{
            {"percussion_note_info", Value::Array{mapNote(2)}},
        };
        SurveySnapshot companion{
            {"percussion_note_info", Value::Array{mapNote(2), mapNote(3)}},
        };
        std::map<ComparisonTransformation, std::uint64_t> transformations;
        ComparisonPreparationContext context{source, companion, transformations,
                                             FormatEpoch::ZlibLegacy};
        runComparisonPreparers(context);

        CHECK(companion.at("percussion_note_info").asArray().size() == 1);
        CHECK(transformations[ComparisonTransformation::FinaleSynthesizedPercussionMapNote] == 1);
    }

    SECTION("dormant zlib drum staff") {
        SurveySnapshot source{
            {"drum_staff", Value::Array{drumStaff(1)}},
            {"percussion_note_info", Value::Array{}},
        };
        SurveySnapshot companion{
            {"percussion_note_info", Value::Array{mapNote(1)}},
        };
        std::map<ComparisonTransformation, std::uint64_t> transformations;
        ComparisonPreparationContext context{source, companion, transformations,
                                             FormatEpoch::ZlibLegacy};
        runComparisonPreparers(context);

        CHECK(companion.at("percussion_note_info").asArray().empty());
        CHECK(transformations[ComparisonTransformation::FinaleSynthesizedPercussionMapNote] == 1);
    }
}

TEST_CASE("Coverage aligns upgraded legacy percussion maps through their staffs",
          "[coverage][percussion]") {
    using namespace finale_mus_reader::coverage;
    const auto drumStaff = [](std::int64_t staffId, std::int64_t mapId, bool legacy) {
        Value::Object result{{"cmper", staffId}, {"which_drum_lib", mapId}};
        if (legacy)
            result.emplace("origin_whichDrumLib", "legacy-mus");
        return result;
    };
    const auto mapNote = [](std::int64_t mapId, std::int64_t inci, std::int64_t noteType,
                            std::int64_t staffPosition, bool legacy) {
        Value::Object result{{"closed_notehead", 207},
                             {"cmper", mapId},
                             {"dwhole_notehead", 250},
                             {"half_notehead", 250},
                             {"inci", inci},
                             {"perc_note_type", noteType},
                             {"staff_position", staffPosition},
                             {"whole_notehead", 250}};
        if (legacy)
            result.emplace("origin_percNoteType", "legacy-behavior");
        return result;
    };
    const auto compare = [&](SurveySnapshot source, SurveySnapshot companion) {
        const ImportReport report(FormatEpoch::ZlibLegacy);
        return compareSnapshots(std::move(source), std::move(companion),
                                makePercussionNoteInfoDocument(), makePercussionNoteInfoDocument(),
                                FormatEpoch::ZlibLegacy, ByteOrder::BigEndian, nullptr, report);
    };

    SECTION("renumbered map with map-specific note types") {
        const auto comparison =
            compare({{"drum_staff", Value::Array{drumStaff(20, 5, true)}},
                     {"percussion_note_info",
                      Value::Array{mapNote(5, 0, 28, 2, true), mapNote(5, 1, 27, 4, true)}}},
                    {{"drum_staff", Value::Array{drumStaff(20, 1, false)}},
                     {"percussion_note_info",
                      Value::Array{mapNote(1, 0, 127, 2, false), mapNote(1, 1, 128, 4, false),
                                   mapNote(5, 0, 1, 9, false)}}});
        const auto &noteStats = comparison.classes.at("others").at("percussion_note_info");
        CHECK(noteStats.same == 12);
        CHECK(noteStats.expected == 0);
        CHECK(noteStats.unexpected == 2);
        CHECK(noteStats.sourceOnly == 0);
        CHECK(noteStats.companionOnly == 0);
        const auto &staffStats = comparison.classes.at("others").at("drum_staff");
        CHECK(staffStats.same == 0);
        CHECK(staffStats.expected == 1);
        CHECK(staffStats.unexpected == 0);
        CHECK(comparison.expected.at(DifferenceClassification::FinaleReplacedLegacyPercussionMap) ==
              1);
        CHECK(comparison.transformations.at(
                  ComparisonTransformation::FinaleSynthesizedPercussionMapNote) == 1);
    }

    SECTION("shared legacy map split by staff") {
        const auto comparison =
            compare({{"drum_staff", Value::Array{drumStaff(20, 5, true), drumStaff(21, 5, true)}},
                     {"percussion_note_info", Value::Array{mapNote(5, 0, 28, 2, true)}}},
                    {{"drum_staff", Value::Array{drumStaff(20, 1, false), drumStaff(21, 2, false)}},
                     {"percussion_note_info",
                      Value::Array{mapNote(1, 3, 28, 2, false), mapNote(2, 4, 28, 2, false)}}});
        const auto &noteStats = comparison.classes.at("others").at("percussion_note_info");
        CHECK(noteStats.same == 14);
        CHECK(noteStats.unexpected == 0);
        CHECK(noteStats.sourceOnly == 0);
        CHECK(noteStats.companionOnly == 0);
        const auto &staffStats = comparison.classes.at("others").at("drum_staff");
        CHECK(staffStats.expected == 2);
        CHECK(staffStats.unexpected == 0);
    }

    SECTION("map without a companion staff") {
        const auto comparison =
            compare({{"drum_staff", Value::Array{drumStaff(20, 5, true)}},
                     {"percussion_note_info", Value::Array{mapNote(5, 0, 28, 2, true)}}},
                    {{"drum_staff", Value::Array{}},
                     {"percussion_note_info", Value::Array{mapNote(5, 0, 1, 9, false)}}});
        const auto &noteStats = comparison.classes.at("others").at("percussion_note_info");
        CHECK(noteStats.unexpected == 0);
        CHECK(noteStats.sourceOnly == 7);
        CHECK(noteStats.companionOnly == 0);
    }
}

TEST_CASE("Percussion-note collections diagnose malformed element tails", "[class]") {
    SECTION("incomplete element") {
        auto profile = profileFor(15);
        profile.epoch = FormatEpoch::ZlibLegacy;
        const auto document = makePercussionNoteInfoDocument();
        const auto report = importPercussionNoteInfo(
            makeClassContainer(0x0139, {35, 7, 48, 49, 50, 51, 99}, ByteOrder::BigEndian, 9),
            profile, document);

        CHECK(document->getOthers()
                  ->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID,
                                                           musx::dom::Cmper(9))
                  .size() == 1);
        REQUIRE(report.diagnostics.size() == 1);
        CHECK(report.diagnostics.front().message.find("incomplete trailing element") !=
              std::string::npos);
    }

    SECTION("nonzero Finale 2012 padding") {
        auto profile = profileFor(17);
        profile.epoch = FormatEpoch::ZlibLegacy;
        const auto document = makePercussionNoteInfoDocument();
        const auto report = importPercussionNoteInfo(
            makeClassContainer(0x0139, {38, 5, 48, 0, 49, 0, 50, 0, 51, 0, 1, 0},
                               ByteOrder::BigEndian, 4),
            profile, document);

        REQUIRE(report.diagnostics.size() == 1);
        CHECK(report.diagnostics.front().message.find("nonzero trailing words") !=
              std::string::npos);
    }
}

} // namespace
} // namespace finale_mus_reader_tests
