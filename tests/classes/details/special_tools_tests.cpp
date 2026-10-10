// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

#include <tuple>

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

template <typename Target>
ImportReport importBeamDetail(const finale_mus_reader::container::ParsedContainer& parsed, FormatEpoch epoch, const musx::dom::DocumentPtr& document,
    std::optional<SourceVersion> version = std::nullopt)
{
    ImportReport report(epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    SourceProfile profile(epoch);
    profile.byteOrder = parsed.byteOrder;
    profile.version = version;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    if constexpr (std::is_same_v<Target, musx::dom::details::BeamAlterationsDownStem>) {
        finale_mus_reader::details::importBeamAlterationsDownStem(context);
    } else if constexpr (std::is_same_v<Target, musx::dom::details::BeamAlterationsUpStem>) {
        finale_mus_reader::details::importBeamAlterationsUpStem(context);
    } else if constexpr (std::is_same_v<Target, musx::dom::details::SecondaryBeamAlterationsDownStem>) {
        finale_mus_reader::details::importSecondaryBeamAlterationsDownStem(context);
    } else {
        finale_mus_reader::details::importSecondaryBeamAlterationsUpStem(context);
    }
    return report;
}

template <typename Target>
ImportReport importStemDetail(const finale_mus_reader::container::ParsedContainer& parsed, FormatEpoch epoch, const musx::dom::DocumentPtr& document)
{
    ImportReport report(epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    SourceProfile profile(epoch);
    profile.byteOrder = parsed.byteOrder;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    if constexpr (std::is_same_v<Target, musx::dom::details::StemAlterations>) {
        finale_mus_reader::details::importStemAlterations(context);
    } else {
        finale_mus_reader::details::importStemAlterationsUnderBeam(context);
    }
    return report;
}

TEST_CASE("Stem alterations recover signed packed offsets for both selectors", "[class][special-tools]")
{
    using Plain = musx::dom::details::StemAlterations;
    using Beamed = musx::dom::details::StemAlterationsUnderBeam;
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        for (const auto order : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            auto session = musx::factory::DocumentFactory::begin();
            const auto document = session.getDocument();
            const auto plainReport = importStemDetail<Plain>(makeDetailContainer(epoch, 0, 42, {-12, 21, 0, 0, -2809}, "ST", order), epoch, document);
            const auto beamedReport =
                importStemDetail<Beamed>(makeDetailContainer(epoch, 0, 43, {9, -15, 0, 0, std::int16_t(0x0efa)}, "St", order), epoch, document);
            const auto plain = document->getDetails()->get<Plain>(musx::dom::SCORE_PARTID, 42);
            const auto beamed = document->getDetails()->get<Beamed>(musx::dom::SCORE_PARTID, 43);
            REQUIRE(plain);
            REQUIRE(beamed);
            CHECK(plain->upVertAdjust == -12);
            CHECK(plain->downVertAdjust == 21);
            CHECK(plain->upHorzAdjust == -11);
            CHECK(plain->downHorzAdjust == 7);
            CHECK(beamed->upVertAdjust == 9);
            CHECK(beamed->downVertAdjust == -15);
            CHECK(beamed->upHorzAdjust == 14);
            CHECK(beamed->downHorzAdjust == -6);
            for (const auto* name : {"upVertAdjust", "downVertAdjust", "upHorzAdjust", "downHorzAdjust"}) {
                const auto* plainField =
                    plainReport.findField<Plain>(name, musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(42));
                const auto* beamedField =
                    beamedReport.findField<Beamed>(name, musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(43));
                REQUIRE(plainField);
                REQUIRE(beamedField);
                CHECK(plainField->origin == ValueOrigin::LegacyMus);
                CHECK(beamedField->origin == ValueOrigin::LegacyMus);
            }
        }
    }
}

TEST_CASE("Stem alterations recover zlib class records", "[class][special-tools]")
{
    using Plain = musx::dom::details::StemAlterations;
    using Beamed = musx::dom::details::StemAlterationsUnderBeam;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto plainReport =
        importStemDetail<Plain>(makeDetailClassContainer(0, 42, musx::dom::SCORE_PARTID, {-12, 21, 0, 0, -2809}, ByteOrder::LittleEndian, 0x042a),
            FormatEpoch::ZlibLegacy, document);
    const auto beamedReport = importStemDetail<Beamed>(
        makeDetailClassContainer(0, 43, musx::dom::SCORE_PARTID, {9, -15, 0, 0, std::int16_t(0x0efa)}, ByteOrder::LittleEndian, 0x03ff),
        FormatEpoch::ZlibLegacy, document);
    const auto plain = document->getDetails()->get<Plain>(musx::dom::SCORE_PARTID, 42);
    const auto beamed = document->getDetails()->get<Beamed>(musx::dom::SCORE_PARTID, 43);
    REQUIRE(plain);
    REQUIRE(beamed);
    CHECK(plain->upVertAdjust == -12);
    CHECK(plain->downVertAdjust == 21);
    CHECK(plain->upHorzAdjust == -11);
    CHECK(plain->downHorzAdjust == 7);
    CHECK(beamed->upVertAdjust == 9);
    CHECK(beamed->downVertAdjust == -15);
    CHECK(beamed->upHorzAdjust == 14);
    CHECK(beamed->downHorzAdjust == -6);
    const auto* plainField =
        plainReport.findField<Plain>("upHorzAdjust", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(42));
    const auto* beamedField =
        beamedReport.findField<Beamed>("downHorzAdjust", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(43));
    REQUIRE(plainField);
    REQUIRE(beamedField);
    CHECK(plainField->origin == ValueOrigin::LegacyMus);
    CHECK(beamedField->origin == ValueOrigin::LegacyMus);
    CHECK(plainField->rawValue == -11);
    CHECK(beamedField->rawValue == -6);
}

TEST_CASE("Stem alterations use incidence zero when later incidences contain data", "[class][special-tools]")
{
    using Plain = musx::dom::details::StemAlterations;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto report = importStemDetail<Plain>(
        makeDetailContainer(FormatEpoch::UncompressedLegacy, 0, 42, {1, 2, 0, 0, 3, 4, 5, 0, 0, 6}, "ST"), FormatEpoch::UncompressedLegacy, document);
    const auto stem = document->getDetails()->get<Plain>(musx::dom::SCORE_PARTID, 42);
    REQUIRE(stem);
    CHECK(stem->upVertAdjust == 1);
    CHECK(stem->downVertAdjust == 2);
    CHECK(stem->downHorzAdjust == 3);
    CHECK(report.diagnostics.empty());
}

TEST_CASE("Stem alterations retain the first detail before blank trailing rows", "[class][special-tools]")
{
    using Plain = musx::dom::details::StemAlterations;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto report =
        importStemDetail<Plain>(makeDetailContainer(FormatEpoch::UncompressedLegacy, 0, 42, {18, -6, 0, 0, -2809, 0, 0, 0, 0, 0}, "ST"),
            FormatEpoch::UncompressedLegacy, document);
    const auto stem = document->getDetails()->get<Plain>(musx::dom::SCORE_PARTID, 42);
    REQUIRE(stem);
    CHECK(stem->upVertAdjust == 18);
    CHECK(stem->downVertAdjust == -6);
    CHECK(stem->upHorzAdjust == -11);
    CHECK(stem->downHorzAdjust == 7);
    CHECK(report.diagnostics.empty());
}

TEST_CASE("Stem alterations retain zero adjustments only for valid entries", "[class][special-tools]")
{
    using Plain = musx::dom::details::StemAlterations;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto report = importStemDetail<Plain>(
        makeDetailContainer(FormatEpoch::DclLegacy, 0, 3, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, "ST"), FormatEpoch::DclLegacy, document);
    const auto stem = document->getDetails()->get<Plain>(musx::dom::SCORE_PARTID, 3);
    REQUIRE(stem);
    CHECK(stem->upVertAdjust == 0);
    CHECK(stem->downVertAdjust == 0);
    CHECK(report.diagnostics.empty());

    auto zeroSession = musx::factory::DocumentFactory::begin();
    const auto zeroDocument = zeroSession.getDocument();
    const auto zeroReport = importStemDetail<Plain>(
        makeDetailContainer(FormatEpoch::DclLegacy, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, "ST"), FormatEpoch::DclLegacy, zeroDocument);
    CHECK(zeroDocument->getDetails()->getAllSources<Plain>().empty());
    CHECK(zeroReport.diagnostics.empty());

    auto singleZeroSession = musx::factory::DocumentFactory::begin();
    const auto singleZeroDocument = singleZeroSession.getDocument();
    const auto singleZeroReport = importStemDetail<musx::dom::details::StemAlterationsUnderBeam>(
        makeDetailContainer(FormatEpoch::DclLegacy, 0, 0, {8, -4, 0, 0, std::int16_t(0x0102)}, "St"), FormatEpoch::DclLegacy, singleZeroDocument);
    CHECK(singleZeroDocument->getDetails()->getAllSources<musx::dom::details::StemAlterationsUnderBeam>().empty());
    CHECK(singleZeroReport.diagnostics.empty());
}

TEST_CASE("Finale 2002 beamed stem edit recovers vertical and horizontal offsets", "[class][special-tools]")
{
    using Beamed = musx::dom::details::StemAlterationsUnderBeam;
    const auto baseline = readFixture("evidence/F2002/F2002-16ths.mus");
    CHECK(baseline.document->getDetails()->getAllSources<Beamed>().empty());

    const auto edited = readFixture("evidence/F2002/F2002-16ths-mvstem.mus");
    const auto stem = edited.document->getDetails()->get<Beamed>(musx::dom::SCORE_PARTID, 2);
    REQUIRE(stem);
    CHECK(stem->upVertAdjust == 42);
    CHECK(stem->downVertAdjust == 0);
    CHECK(stem->upHorzAdjust == -16);
    CHECK(stem->downHorzAdjust == 0);
    const auto* vertical =
        edited.report.findField<Beamed>("upVertAdjust", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(2));
    const auto* horizontal =
        edited.report.findField<Beamed>("upHorzAdjust", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(2));
    REQUIRE(vertical);
    REQUIRE(horizontal);
    CHECK(vertical->rawValue == 42);
    CHECK(horizontal->rawValue == -16);
    CHECK(vertical->origin == ValueOrigin::LegacyMus);
    CHECK(horizontal->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Primary beam alterations recover both stem directions", "[class][special-tools]")
{
    using Down = musx::dom::details::BeamAlterationsDownStem;
    using Up = musx::dom::details::BeamAlterationsUpStem;
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        const std::vector<std::int16_t> downWords = {-4, 12, 6, -8, 0, 2, -1, 0, 0, 0};
        const std::vector<std::int16_t> upWords = {3, -5, 7, 9, 0, 1, 64, 0, 0, 0};
        const auto downReport = importBeamDetail<Down>(makeDetailContainer(epoch, 0, 42, downWords, "BL"), epoch, document);
        const auto upReport = importBeamDetail<Up>(makeDetailContainer(epoch, 0, 43, upWords, "BH"), epoch, document);
        const auto down = document->getDetails()->get<Down>(musx::dom::SCORE_PARTID, 42);
        const auto up = document->getDetails()->get<Up>(musx::dom::SCORE_PARTID, 43);
        REQUIRE(down);
        REQUIRE(up);
        CHECK(down->leftOffsetH == -4);
        CHECK(down->leftOffsetY == 12);
        CHECK(down->rightOffsetH == 6);
        CHECK(down->rightOffsetY == -8);
        CHECK(down->dura == 0);
        CHECK(down->flattenStyle == Down::FlattenStyle::OnStandardNote);
        CHECK(down->beamWidth == -1);
        CHECK(up->leftOffsetH == 3);
        CHECK(up->leftOffsetY == -5);
        CHECK(up->rightOffsetH == 7);
        CHECK(up->rightOffsetY == 9);
        CHECK(up->flattenStyle == Up::FlattenStyle::AlwaysFlat);
        CHECK(up->beamWidth == (epoch == FormatEpoch::DclLegacy ? 64 : 4096));
        const auto* downField =
            downReport.findField<Down>("beamWidth", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(42));
        REQUIRE(downField);
        CHECK(downField->origin == ValueOrigin::LegacyMus);
        CHECK(downField->rawValue == -1);
        const auto* upField = upReport.findField<Up>("leftOffsetH", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(43));
        REQUIRE(upField);
        CHECK(upField->rawValue == 3);
    }
}

TEST_CASE("Primary beam alteration rejects an incomplete detail", "[class][special-tools]")
{
    using Down = musx::dom::details::BeamAlterationsDownStem;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto report =
        importBeamDetail<Down>(makeDetailContainer(FormatEpoch::UncompressedLegacy, 0, 42, {1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, "BL"),
            FormatEpoch::UncompressedLegacy, document);
    CHECK(document->getDetails()->getAllSources<Down>().empty());
    REQUIRE(report.diagnostics.size() == 1);
}

TEST_CASE("Primary beam alterations accept the five-word layout", "[class][special-tools]")
{
    using Down = musx::dom::details::BeamAlterationsDownStem;
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        const auto report = importBeamDetail<Down>(makeDetailContainer(epoch, 0, 42, {1, -2, 3, -4, 0}, "BL"), epoch, document);
        const auto beam = document->getDetails()->get<Down>(musx::dom::SCORE_PARTID, 42);
        REQUIRE(beam);
        CHECK(beam->leftOffsetH == 1);
        CHECK(beam->leftOffsetY == -2);
        CHECK(beam->rightOffsetH == 3);
        CHECK(beam->rightOffsetY == -4);
        CHECK(beam->dura == 0);
        CHECK(beam->beamWidth == -1);
        const auto* mode = report.findField<Down>("flattenStyle", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(42));
        REQUIRE(mode);
        CHECK(mode->origin == ValueOrigin::Finale27Default);
        const auto* width = report.findField<Down>("beamWidth", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(42));
        REQUIRE(width);
        CHECK(width->origin == ValueOrigin::LegacyBehavior);
    }
}

TEST_CASE("Primary beam alterations recover zlib class records", "[class][special-tools]")
{
    using Down = musx::dom::details::BeamAlterationsDownStem;
    using Up = musx::dom::details::BeamAlterationsUpStem;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const std::vector<std::int16_t> words = {1, -2, 3, -4, 0, 3, -1, 0, 0, 0};
    const auto downReport = importBeamDetail<Down>(
        makeDetailClassContainer(0, 42, musx::dom::SCORE_PARTID, words, ByteOrder::LittleEndian, 0x0401), FormatEpoch::ZlibLegacy, document);
    const auto upReport = importBeamDetail<Up>(
        makeDetailClassContainer(0, 43, musx::dom::SCORE_PARTID, words, ByteOrder::LittleEndian, 0x0402), FormatEpoch::ZlibLegacy, document);
    const auto down = document->getDetails()->get<Down>(musx::dom::SCORE_PARTID, 42);
    const auto up = document->getDetails()->get<Up>(musx::dom::SCORE_PARTID, 43);
    REQUIRE(down);
    REQUIRE(up);
    CHECK(down->leftOffsetH == 1);
    CHECK(down->leftOffsetY == -2);
    CHECK(down->flattenStyle == Down::FlattenStyle::OnExtremeNote);
    CHECK(down->beamWidth == -1);
    CHECK(up->rightOffsetH == 3);
    CHECK(up->rightOffsetY == -4);
    const auto* downField =
        downReport.findField<Down>("flattenStyle", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(42));
    REQUIRE(downField);
    CHECK(downField->origin == ValueOrigin::LegacyMus);
    const auto* upField = upReport.findField<Up>("beamWidth", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(43));
    REQUIRE(upField);
    CHECK(upField->rawValue == -1);
}

TEST_CASE("Primary beam width changes units at Finale 2002", "[class][special-tools]")
{
    using Down = musx::dom::details::BeamAlterationsDownStem;
    for (const auto& [stored, before2002, from2002] : {std::tuple{std::int16_t(12), musx::dom::Efix(768), musx::dom::Efix(12)},
             std::tuple{std::int16_t(0), musx::dom::Efix(-1), musx::dom::Efix(0)},
             std::tuple{std::int16_t(-1), musx::dom::Efix(-1), musx::dom::Efix(-1)},
             std::tuple{std::int16_t(-6), musx::dom::Efix(-6), musx::dom::Efix(-6)}}) {
        for (const auto& [major, expected] : {std::pair{std::uint8_t(6), before2002}, std::pair{std::uint8_t(7), from2002}}) {
            auto session = musx::factory::DocumentFactory::begin();
            const auto document = session.getDocument();
            const auto report = importBeamDetail<Down>(makeDetailContainer(FormatEpoch::DclLegacy, 0, 42, {0, 0, 0, 0, 0, 2, stored, 0, 0, 0}, "BL"),
                FormatEpoch::DclLegacy, document, SourceVersion{.major = major});
            const auto beam = document->getDetails()->get<Down>(musx::dom::SCORE_PARTID, 42);
            REQUIRE(beam);
            CHECK(beam->beamWidth == expected);
            const auto* width = report.findField<Down>("beamWidth", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(42));
            REQUIRE(width);
            CHECK(width->rawValue == stored);
        }
    }
}

TEST_CASE("Finale 2001 primary beam width is stored in whole EVPUs", "[class][special-tools]")
{
    using Down = musx::dom::details::BeamAlterationsDownStem;
    const auto baseline = readFixture("evidence/F2001/F2001Win-16ths.mus");
    CHECK(baseline.document->getDetails()->getAllSources<Down>().empty());

    const auto edited = readFixture("evidence/F2001/F2001Win-16ths-beamwidth.mus");
    const auto beam = edited.document->getDetails()->get<Down>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(beam);
    CHECK(beam->beamWidth == 192);
    const auto* width = edited.report.findField<Down>("beamWidth", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(1));
    REQUIRE(width);
    CHECK(width->rawValue == 3);
    CHECK(width->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Finale 2002 primary beam width is stored in EFIX", "[class][special-tools]")
{
    using Up = musx::dom::details::BeamAlterationsUpStem;
    const auto baseline = readFixture("evidence/F2002/F2002-16ths.mus");
    CHECK(baseline.document->getDetails()->getAllSources<Up>().empty());

    const auto edited = readFixture("evidence/F2002/F2002-16ths-bmwidths.mus");
    const auto beam = edited.document->getDetails()->get<Up>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(beam);
    CHECK(beam->beamWidth == 384);
    const auto* width = edited.report.findField<Up>("beamWidth", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), std::nullopt, musx::dom::Cmper(1));
    REQUIRE(width);
    CHECK(width->rawValue == 384);
    CHECK(width->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Secondary beam alterations retain their width word without primary conversion", "[class][special-tools]")
{
    using Down = musx::dom::details::SecondaryBeamAlterationsDownStem;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto report = importBeamDetail<Down>(makeDetailContainer(FormatEpoch::DclLegacy, 0, 42, {0, 0, 0, 0, 256, 2, -3, 0, 0, 0}, "bL"),
        FormatEpoch::DclLegacy, document, SourceVersion{.major = 6});
    const auto beam = document->getDetails()->get<Down>(musx::dom::SCORE_PARTID, 42, musx::dom::Inci(0));
    REQUIRE(beam);
    CHECK(beam->dura == 256);
    CHECK(beam->beamWidth == -3);
    const auto* width = report.findField<Down>("beamWidth", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), musx::dom::Inci(0), musx::dom::Cmper(42));
    REQUIRE(width);
    CHECK(width->origin == ValueOrigin::LegacyMus);
    CHECK(width->rawValue == -3);
}

TEST_CASE("Secondary beam alterations recover the controlled stem-direction edit", "[class][special-tools]")
{
    using Down = musx::dom::details::SecondaryBeamAlterationsDownStem;
    using Up = musx::dom::details::SecondaryBeamAlterationsUpStem;
    const auto baseline = readFixture("evidence/F2008/F2008-16ths.mus");
    CHECK(baseline.document->getDetails()->getAllSources<Down>().empty());
    CHECK(baseline.document->getDetails()->getAllSources<Up>().empty());

    const auto edited = readFixture("evidence/F2008/F2008-16ths-secdnup.mus");
    const auto down = edited.document->getDetails()->getArray<Down>(musx::dom::SCORE_PARTID, 9);
    const auto up = edited.document->getDetails()->getArray<Up>(musx::dom::SCORE_PARTID, 5);
    REQUIRE(down.size() == 1);
    REQUIRE(up.size() == 1);
    CHECK(down.front()->getInci() == musx::dom::Inci(0));
    CHECK(down.front()->leftOffsetH == 0);
    CHECK(down.front()->leftOffsetY == 6);
    CHECK(down.front()->rightOffsetH == 0);
    CHECK(down.front()->rightOffsetY == -6);
    CHECK(down.front()->dura == 256);
    CHECK(down.front()->beamWidth == -1);
    CHECK(up.front()->getInci() == musx::dom::Inci(0));
    CHECK(up.front()->leftOffsetY == -6);
    CHECK(up.front()->rightOffsetY == -6);
    CHECK(up.front()->dura == 256);
    CHECK(up.front()->beamWidth == -1);
    const auto* downDuration =
        edited.report.findField<Down>("dura", musx::dom::SCORE_PARTID, musx::dom::Cmper(0), musx::dom::Inci(0), musx::dom::Cmper(9));
    REQUIRE(downDuration);
    CHECK(downDuration->rawValue == 256);
    CHECK(downDuration->origin == ValueOrigin::LegacyMus);
}

} // namespace
} // namespace finale_mus_reader_tests
