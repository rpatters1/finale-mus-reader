// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

using PercussionNoteInfoTestTarget = musx::dom::others::PercussionNoteInfo;

musx::dom::DocumentPtr makePercussionNoteInfoDocument()
{
    using FontDefinition = musx::dom::others::FontDefinition;
    using FontOptions = musx::dom::options::FontOptions;

    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto definition = std::make_shared<FontDefinition>(document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, musx::dom::Cmper(5));
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

ImportReport importPercussionNoteInfo(
    const finale_mus_reader::container::ParsedContainer& parsed, const SourceProfile& profile, const musx::dom::DocumentPtr& document)
{
    ImportReport report(profile.epoch);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{
        LegacyRecordIndex::build(parsed), profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importPercussionNoteInfo(context);
    return report;
}

TEST_CASE("Finale 2010 percussion-note collections use six narrow words", "[class]")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        auto profile = profileFor(15);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = byteOrder;
        const auto document = makePercussionNoteInfoDocument();
        const auto report = importPercussionNoteInfo(
            makeClassContainer(0x0139, {35, 7, 0x00c7, 48, 49, 50, 4131, -2, 51, 52, 53, 54}, byteOrder, 9), profile, document);

        const auto first = document->getOthers()->get<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID, 9, musx::dom::Inci(0));
        const auto second = document->getOthers()->get<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID, 9, musx::dom::Inci(1));
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
        CHECK(field(report, "others.percussionNoteInfo[9,0].closedNotehead").origin == ValueOrigin::LegacyMus);
        CHECK(field(report, "others.percussionNoteInfo[9,1].staffPosition").rawValue == -2);
    }
}

TEST_CASE("Finale 2012 percussion-note collections use wide codepoints and padding", "[class]")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        auto profile = profileFor(17);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = byteOrder;
        const auto document = makePercussionNoteInfoDocument();
        const auto report = importPercussionNoteInfo(
            makeClassContainer(0x0139, {38, 5, std::int16_t(-2494), 1, 0x00fa, 0, 0x00db, 0, 0x00c0, 0, 0, 0}, byteOrder, 4), profile, document);

        const auto note = document->getOthers()->get<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID, 4, musx::dom::Inci(0));
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

TEST_CASE("Fixed-row percussion maps use DS selection in both storage epochs", "[class]")
{
    for (const auto epoch : {FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            auto parsed = makeContainer({{7, "DS", {1, 0, 0, 0, 0x7000, 1}}}, epoch, byteOrder);
            for (const auto& [mapId, midiKey, words] : std::vector<std::tuple<std::uint16_t, std::uint16_t, std::vector<std::int16_t>>>{
                     {1, 60, {60, 7, 207, 250, 0}}, {1, 61, {61, 0, 207, 250, 0}}, {1, 62, {62, 8, 120, 89, 0}}, {1, 63, {63, 9, 120, 89, 0}},
                     {1, 64, {60, 10, 121, 89, 0}}, {2, 60, {60, 9, 207, 250, 0}}}) {
                auto extra = makeDetailContainer(epoch, mapId, midiKey, words, "DF", byteOrder);
                parsed.blocks.push_back(std::move(extra.blocks.front()));
            }
            auto profile = SourceProfile(epoch);
            profile.byteOrder = byteOrder;
            const auto document = makePercussionNoteInfoDocument();
            const auto report = importPercussionNoteInfo(parsed, profile, document);

            const auto notes = document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
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
            CHECK(field(report, "others.percussionNoteInfo[1,0].percNoteType").origin == ValueOrigin::LegacyBehavior);
            CHECK(field(report, "others.percussionNoteInfo[1,0].staffPosition").origin == ValueOrigin::LegacyMus);
            CHECK(field(report, "others.percussionNoteInfo[1,0].wholeNotehead").origin == ValueOrigin::LegacyBehavior);
        }
    }
}

TEST_CASE("Legacy staff-style selections synthesize percussion-note collections", "[class]")
{
    const auto checkNotes = [](const finale_mus_reader::container::ParsedContainer& parsed, SourceProfile profile) {
        const auto document = makePercussionNoteInfoDocument();
        const auto report = importPercussionNoteInfo(parsed, profile, document);
        const auto notes = document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
        REQUIRE(notes.size() == 1);
        CHECK(notes.front()->percNoteType == 32);
        CHECK(notes.front()->staffPosition == 6);
        CHECK(field(report, "others.percussionNoteInfo[1,0].staffPosition").origin == ValueOrigin::LegacyMus);
    };

    for (const auto epoch : {FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        auto parsed = makeContainer({{7, "FY", {1, 0, 0, 0, 0x1000, 0}}}, epoch, ByteOrder::BigEndian);
        auto detail = makeDetailContainer(epoch, 1, 60, {60, 6, 207, 250, 0}, "DF", ByteOrder::BigEndian);
        parsed.blocks.push_back(std::move(detail.blocks.front()));
        auto profile = SourceProfile(epoch);
        profile.byteOrder = ByteOrder::BigEndian;
        profile.version = SourceVersion{.major = finale_mus_reader::versions::finale2000.major};
        checkNotes(parsed, profile);
    }

    auto parsed = makeClassContainer(0x0085, {1, 0, 0, 0, 0x1000, 0, 0, 0, 0, 0, 0, 0}, ByteOrder::LittleEndian, 7);
    auto detail = makeDetailClassContainer(1, 60, 0, {60, 6, 207, 250, 0}, ByteOrder::LittleEndian, 0x040e);
    parsed.blocks.push_back(std::move(detail.blocks.front()));
    auto profile = profileFor(13);
    profile.epoch = FormatEpoch::ZlibLegacy;
    profile.byteOrder = ByteOrder::LittleEndian;
    checkNotes(parsed, profile);
}

TEST_CASE("Pre-Finale 2000 FY selections do not synthesize percussion-note collections", "[class]")
{
    auto parsed = makeContainer({{7, "FY", {1, 0, 0, 0, 0x1000, 0}}}, FormatEpoch::UncompressedLegacy, ByteOrder::BigEndian);
    auto detail = makeDetailContainer(FormatEpoch::UncompressedLegacy, 1, 60, {60, 6, 207, 250, 0}, "DF", ByteOrder::BigEndian);
    parsed.blocks.push_back(std::move(detail.blocks.front()));
    auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    profile.version = SourceVersion{.major = finale_mus_reader::versions::finale98.major};
    const auto document = makePercussionNoteInfoDocument();
    importPercussionNoteInfo(parsed, profile, document);

    CHECK(document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID).empty());
}

TEST_CASE("Finale 2008 retains DF rows and DS selection as zlib classes", "[class]")
{
    const auto unassigned = readFixture("evidence/F2008/F2008-percussion-staff.mus", fixtureLegacySymbolFonts);
    CHECK(unassigned.document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1)).empty());

    const auto assigned = readFixture("evidence/F2008/F2008-percussion-staff-edit.mus", fixtureLegacySymbolFonts);
    const auto notes = assigned.document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    REQUIRE(notes.size() == 1);
    CHECK(notes.front()->percNoteType == 32);
    CHECK(notes.front()->staffPosition == 6);
    CHECK(std::uint32_t(notes.front()->closedNotehead) == 208);
    CHECK(std::uint32_t(notes.front()->halfNotehead) == 194);
    CHECK(std::uint32_t(notes.front()->wholeNotehead) == 194);
    CHECK(std::uint32_t(notes.front()->dwholeNotehead) == 194);
}

TEST_CASE("Finale 2008 staff-style selections upgrade their referenced percussion rows", "[class]")
{
    const std::string mapping = R"xml(
        <FinaleNameDocument><MasterDeviceNames>
        <NoteNameList Name="Percussion Map 1">
        <Note Number="2" PercNoteType="32" />
        <Note Number="7" PercNoteType="37" />
        </NoteNameList></MasterDeviceNames></FinaleNameDocument>)xml";
    const auto bytes = [](std::string_view value) {
        return std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
    };
    const finale_mus_reader::ReaderOptions options{bytes(fixtureLegacySymbolFonts), {bytes(mapping)}};

    for (const auto fixture : {"F2008-percstyle.mus", "F2008-percstyle-assigned.mus"}) {
        const auto result =
            Reader::readWithReport<TestXmlDocument>(std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / "evidence/F2008" / fixture, options);
        const auto notes = result.document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
        REQUIRE(notes.size() == 2);
        CHECK(notes[0]->percNoteType == 32);
        CHECK(notes[0]->staffPosition == 6);
        CHECK(notes[1]->percNoteType == 37);
        CHECK(notes[1]->staffPosition == 4);
    }
}

TEST_CASE("Reusable readers apply the first supplied named percussion table", "[class][reader]")
{
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
        const auto bytes = [](const std::string& value) {
            return std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
        };
        const std::string symbolFontContents(fixtureLegacySymbolFonts);
        const finale_mus_reader::ReaderOptions options{bytes(symbolFontContents), {bytes(first), bytes(duplicate)}};
        return finale_mus_reader::Reader::create<TestXmlDocument>(options);
    }();

    for (int readCount = 0; readCount < 2; ++readCount) {
        const auto result =
            reader.readWithReport(std::filesystem::path(FINALE_MUS_READER_TEST_SOURCE_DIR) / "evidence/F2008/F2008-percussion-staff-edit.mus");
        const auto notes = result.document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
        REQUIRE(notes.size() == 1);
        CHECK(notes.front()->percNoteType == 236);
        CHECK(field(result.report, "others.percussionNoteInfo[1,0].percNoteType").rawValue == 60);
    }
}

TEST_CASE("An unreferenced DCL DF map is not constructed", "[class]")
{
    const auto parsed = makeDetailContainer(FormatEpoch::DclLegacy, 1, 60, {60, 6, 207, 250, 0}, "DF", ByteOrder::BigEndian);
    auto profile = SourceProfile(FormatEpoch::DclLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto document = makePercussionNoteInfoDocument();
    importPercussionNoteInfo(parsed, profile, document);

    CHECK(document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID).empty());
}

TEST_CASE("Coda-banner files predate percussion maps", "[class]")
{
    auto parsed = makeContainer({{7, "DS", {1, 0, 0, 0, 0x1000, 0}}}, FormatEpoch::CodaBanner);
    auto detail = makeDetailContainer(FormatEpoch::CodaBanner, 1, 60, {60, 7, 207, 250, 0}, "DF");
    parsed.blocks.push_back(std::move(detail.blocks.front()));
    auto profile = SourceProfile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto document = makePercussionNoteInfoDocument();
    const auto report = importPercussionNoteInfo(parsed, profile, document);
    CHECK(document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID).empty());
    CHECK(reportedFieldCount(report) == 0);
}

TEST_CASE("Finale 98 DF maps without a staff selection are not constructed", "[class]")
{
    const auto result = readFixture("evidence/F98/F98-baseline.mus", fixtureLegacySymbolFonts);
    CHECK(result.document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID).empty());
}

TEST_CASE("Percussion-note collections diagnose malformed element tails", "[class]")
{
    SECTION("incomplete element")
    {
        auto profile = profileFor(15);
        profile.epoch = FormatEpoch::ZlibLegacy;
        const auto document = makePercussionNoteInfoDocument();
        const auto report =
            importPercussionNoteInfo(makeClassContainer(0x0139, {35, 7, 48, 49, 50, 51, 99}, ByteOrder::BigEndian, 9), profile, document);

        CHECK(document->getOthers()->getArray<PercussionNoteInfoTestTarget>(musx::dom::SCORE_PARTID, musx::dom::Cmper(9)).size() == 1);
        REQUIRE(report.diagnostics.size() == 1);
        CHECK(report.diagnostics.front().message.find("incomplete trailing element") != std::string::npos);
    }

    SECTION("nonzero Finale 2012 padding")
    {
        auto profile = profileFor(17);
        profile.epoch = FormatEpoch::ZlibLegacy;
        const auto document = makePercussionNoteInfoDocument();
        const auto report = importPercussionNoteInfo(
            makeClassContainer(0x0139, {38, 5, 48, 0, 49, 0, 50, 0, 51, 0, 1, 0}, ByteOrder::BigEndian, 4), profile, document);

        REQUIRE(report.diagnostics.size() == 1);
        CHECK(report.diagnostics.front().message.find("nonzero trailing words") != std::string::npos);
    }
}
} // namespace
} // namespace finale_mus_reader_tests
