// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;
using CenterShape = musx::dom::details::CenterShape;

ImportReport importCenterShapes(
    const finale_mus_reader::container::ParsedContainer& parsed, const SourceProfile& profile, const musx::dom::DocumentPtr& document)
{
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::details::importCenterShapes(context);
    finale_mus_reader::runDeferredChecks(pending);
    return report;
}

TEST_CASE("CenterShape reads the three-incidence detail and class payload", "[class][center-shape]")
{
    const std::vector<std::int16_t> words = {11, -12, 0x4203, -13, 14, 0x4100, 21, -22, -23, 24, 0x4200, 0, 0, 0, 0};
    for (const auto epoch : {FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        for (const auto order : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            const auto parsed = epoch == FormatEpoch::ZlibLegacy ? makeDetailClassContainer(31, 7, musx::dom::SCORE_PARTID, words, order, 0x0406)
                                                                 : makeDetailContainer(epoch, 31, 7, words, "Cx", order);
            auto profile = SourceProfile(epoch);
            profile.byteOrder = order;
            auto session = musx::factory::DocumentFactory::begin();
            const auto document = session.getDocument();
            const auto report = importCenterShapes(parsed, profile, document);
            const auto shape = document->getDetails()->get<CenterShape>(musx::dom::SCORE_PARTID, 31, 7);
            REQUIRE(shape);
            REQUIRE(shape->startBreakAdj);
            REQUIRE(shape->endBreakAdj);
            REQUIRE(shape->ctlPtAdj);
            CHECK(shape->startBreakAdj->horzOffset == 11);
            CHECK(shape->startBreakAdj->vertOffset == -12);
            CHECK(shape->startBreakAdj->active);
            CHECK(shape->startBreakAdj->contextDir == musx::dom::smartshape::DirectionType::Over);
            CHECK(shape->startBreakAdj->contextEntCnct == musx::dom::smartshape::EntryConnectionType::HeadLeftBottom);
            CHECK(shape->endBreakAdj->horzOffset == -13);
            CHECK(shape->endBreakAdj->vertOffset == 14);
            CHECK(shape->endBreakAdj->active);
            CHECK(shape->endBreakAdj->contextDir == musx::dom::smartshape::DirectionType::Under);
            CHECK(shape->ctlPtAdj->startCtlPtX == 21);
            CHECK(shape->ctlPtAdj->startCtlPtY == -22);
            CHECK(shape->ctlPtAdj->endCtlPtX == -23);
            CHECK(shape->ctlPtAdj->endCtlPtY == 24);
            CHECK(shape->ctlPtAdj->active);
            CHECK(shape->ctlPtAdj->contextDir == musx::dom::smartshape::DirectionType::Over);
            const auto* field = report.findField<CenterShape>("ctlPtAdj.startCtlPtX", musx::dom::SCORE_PARTID, 31, std::nullopt, 7);
            REQUIRE(field);
            CHECK(field->origin == ValueOrigin::LegacyMus);
            CHECK(field->rawValue == 21);
        }
    }
}

TEST_CASE("CenterShape leaves absent early center records absent", "[class][center-shape]")
{
    const auto source = readFixture("evidence/F263/F263-11bars-8va-3to8.mus");
    CHECK(source.document->getDetails()->getAllSources<CenterShape>().empty());
}

TEST_CASE("CenterShape rejects an incomplete class record", "[class][center-shape]")
{
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto report = importCenterShapes(
        makeDetailClassContainer(31, 7, musx::dom::SCORE_PARTID, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, ByteOrder::LittleEndian, 0x0406), profile,
        document);
    CHECK(document->getDetails()->getAllSources<CenterShape>().empty());
    REQUIRE(report.diagnostics.size() == 1);
}

} // namespace
} // namespace finale_mus_reader_tests
