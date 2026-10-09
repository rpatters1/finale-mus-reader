// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

#include <algorithm>

namespace finale_mus_reader_tests {
namespace {

using namespace classes;
using CenterShape = musx::dom::details::CenterShape;
struct EarlyShapeCase
{
    musx::dom::Cmper shapeId;
    musx::dom::MeasCmper endMeasure;
};

struct EarlyCenterCase
{
    musx::dom::Cmper shapeId;
    musx::dom::Cmper centerId;
    musx::dom::Cmper measure;
};

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
            const auto* field = report.findField<CenterShape>(
                "ctlPtAdj.startCtlPtX", musx::dom::SCORE_PARTID, musx::dom::Cmper(31), std::nullopt, musx::dom::Cmper(7));
            REQUIRE(field);
            CHECK(field->origin == ValueOrigin::LegacyMus);
            CHECK(field->rawValue == 21);
        }
    }
}

TEST_CASE("Early spanning SmartShapes number default centers within each shape", "[class][center-shape]")
{
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    using Shape = musx::dom::others::SmartShape;
    for (const auto& [shapeId, endMeasure] : {EarlyShapeCase{1, 6}, EarlyShapeCase{2, 6}}) {
        auto shape = std::make_shared<Shape>(document, musx::dom::SCORE_PARTID, Shape::ShareMode::All, shapeId);
        shape->integrityCheck(shape);
        shape->startTermSeg->endPoint->measId = 3;
        shape->endTermSeg->endPoint->measId = endMeasure;
        document->getOthers()->add(Shape::XmlNodeName, std::move(shape));
    }
    const auto parsed = makeContainer({{1, "sX", {3, 0, 0, 0, 0, 0}}, {2, "sX", {3, 0, 0, 0, 0, 0}}});
    const auto report = importCenterShapes(parsed, SourceProfile(FormatEpoch::UncompressedLegacy), document);
    using MeasureAssign = musx::dom::others::SmartShapeMeasureAssign;
    const auto centers = document->getDetails()->getAllSources<CenterShape>();
    REQUIRE(centers.size() == 4);
    for (const auto& [shapeId, centerId, measure] :
        {EarlyCenterCase{1, 1, 4}, EarlyCenterCase{1, 2, 5}, EarlyCenterCase{2, 1, 4}, EarlyCenterCase{2, 2, 5}}) {
        const auto center = document->getDetails()->get<CenterShape>(musx::dom::SCORE_PARTID, shapeId, centerId);
        REQUIRE(center);
        CHECK(center->startBreakAdj->vertOffset == 0);
        CHECK_FALSE(center->startBreakAdj->active);
        CHECK(center->endBreakAdj->vertOffset == 0);
        CHECK_FALSE(center->endBreakAdj->active);
        CHECK_FALSE(center->ctlPtAdj->active);
        const auto assignments = document->getOthers()->getArray<MeasureAssign>(musx::dom::SCORE_PARTID, measure);
        REQUIRE(assignments.size() == 2);
        const auto assigned =
            std::find_if(assignments.begin(), assignments.end(), [shapeId](const auto& value) { return value->shapeNum == shapeId; });
        REQUIRE(assigned != assignments.end());
        CHECK((*assigned)->centerShapeNum == centerId);
    }
    const auto* field =
        report.findField<CenterShape>("startBreakAdj.vertOffset", musx::dom::SCORE_PARTID, musx::dom::Cmper(1), std::nullopt, musx::dom::Cmper(1));
    REQUIRE(field);
    CHECK(field->origin == ValueOrigin::LegacyBehavior);
}

TEST_CASE("Early 8va receives one default center and assignment per interior measure", "[class][center-shape]")
{
    const auto result = readFixture("evidence/F263/F263-11bars-8va-3to8.mus");
    using MeasureAssign = musx::dom::others::SmartShapeMeasureAssign;
    const auto centers = result.document->getDetails()->getAllSources<CenterShape>();
    REQUIRE(centers.size() == 4);
    for (int centerId = 1; centerId <= 4; ++centerId) {
        const auto center = result.document->getDetails()->get<CenterShape>(musx::dom::SCORE_PARTID, 1, musx::dom::Cmper(centerId));
        REQUIRE(center);
        CHECK_FALSE(center->startBreakAdj->active);
        CHECK(center->startBreakAdj->vertOffset == 0);
        CHECK_FALSE(center->endBreakAdj->active);
        CHECK(center->endBreakAdj->vertOffset == 0);
        const auto assignments = result.document->getOthers()->getArray<MeasureAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper(centerId + 3));
        REQUIRE(assignments.size() == 1);
        CHECK(assignments.front()->shapeNum == 1);
        CHECK(assignments.front()->centerShapeNum == centerId);
    }
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
