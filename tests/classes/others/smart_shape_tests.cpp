// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

#include <array>
#include <tuple>
#include <utility>
#include <vector>

namespace finale_mus_reader_tests {
namespace {

using namespace classes;
using Shape = musx::dom::others::SmartShape;

TEST_CASE("Finale 2007 lyric shapes recover their source endpoints", "[class][smart-shape]")
{
    const auto result = readFixture("evidence/F2007/F2007-lyric-hyphens.mus");
    const auto first = result.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    const auto second = result.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(2));
    REQUIRE(first);
    REQUIRE(second);
    CHECK(first->shapeType == Shape::ShapeType::Hyphen);
    CHECK(second->shapeType == Shape::ShapeType::WordExtension);
    CHECK(first->startTermSeg->endPoint->staffId == 1);
    CHECK(first->startTermSeg->endPoint->measId == 1);
    CHECK_FALSE(first->entryBased);
    CHECK(first->startTermSeg->endPoint->entryNumber == 13);
    CHECK(first->endTermSeg->endPoint->measId == 2);
    CHECK(first->endTermSeg->endPoint->entryNumber == 15);
    CHECK(first->startLyricNum == 1);
    CHECK(first->startLyricType == musx::dom::LyricTextType::Verse);
    CHECK(first->endLyricType == musx::dom::LyricTextType::Verse);
    CHECK(result.report.findField<Shape>("shapeType", musx::dom::SCORE_PARTID, musx::dom::Cmper(1))->origin == ValueOrigin::LegacyMus);
    CHECK(result.report.findField<Shape>("startLyricType", musx::dom::SCORE_PARTID, musx::dom::Cmper(1))->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Inactive part SmartShape adjustments retain score attachment context", "[class][smart-shape]")
{
    std::vector<std::int16_t> scoreWords(48);
    scoreWords[0] = 15;
    scoreWords[1] = static_cast<std::int16_t>(0x8211);
    scoreWords[3] = 1;
    scoreWords[4] = 14;
    scoreWords[8] = 6;
    scoreWords[9] = 0x4205;
    scoreWords[17] = 0x4201;
    scoreWords[18] = 1;
    scoreWords[19] = 14;
    scoreWords[24] = 0x4201;
    scoreWords[37] = 0x4200;
    auto partWords = scoreWords;
    partWords[1] = static_cast<std::int16_t>(0x8201);
    partWords[8] = 0;
    partWords[9] = 0;
    partWords[17] = 0x4103;
    partWords[24] = 0;
    partWords[37] = 0;
    const auto parsed = makeClassContainer(
        {{0x00d9, scoreWords, 72}, {0x00d9, partWords, 72, 1, true, std::vector<std::uint16_t>(46, 0xffff)}}, ByteOrder::LittleEndian);
    SourceProfile profile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importSmartShapes(context);

    const auto score = document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(72));
    const auto part = document->getOthers()->get<Shape>(musx::dom::Cmper(1), musx::dom::Cmper(72));
    REQUIRE(score);
    REQUIRE(part);
    CHECK(score->startTermSeg->endPointAdj->contextDir == musx::dom::smartshape::DirectionType::Over);
    CHECK_FALSE(part->startTermSeg->endPointAdj->active);
    CHECK(part->startTermSeg->endPointAdj->vertOffset == 0);
    CHECK(part->startTermSeg->endPointAdj->contextDir == musx::dom::smartshape::DirectionType::Over);
    CHECK(part->startTermSeg->endPointAdj->contextEntCnct == musx::dom::smartshape::EntryConnectionType::StemRightTop);
    CHECK(part->endTermSeg->endPointAdj->contextDir == musx::dom::smartshape::DirectionType::Over);
    CHECK(part->fullCtlPtAdj->contextDir == musx::dom::smartshape::DirectionType::Over);
    CHECK(part->startTermSeg->breakAdj->contextDir == musx::dom::smartshape::DirectionType::Under);
    CHECK(part->startTermSeg->breakAdj->contextEntCnct == musx::dom::smartshape::EntryConnectionType::HeadLeftBottom);
    CHECK(part->engraverSlurState == Shape::EngraverSlurState::Off);
    const auto* adjusted = report.findField<Shape>("startTermSeg.endPointAdj.contextDir", musx::dom::Cmper(1), musx::dom::Cmper(72));
    REQUIRE(adjusted);
    CHECK(adjusted->origin == ValueOrigin::LegacyMusAdjusted);
    CHECK(adjusted->rawValue == 0);
}

TEST_CASE("Part slur edit retains linked shape identity and inactive adjustments", "[class][smart-shape]")
{
    const auto linked = readFixture("evidence/F2012/F2012-slur-part.mus");
    const auto edited = readFixture("evidence/F2012/F2012-slur-part-edited.mus");
    const auto linkedScore = linked.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    const auto linkedPart = linked.document->getOthers()->get<Shape>(musx::dom::Cmper(1), musx::dom::Cmper(1));
    const auto editedScore = edited.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    const auto editedPart = edited.document->getOthers()->get<Shape>(musx::dom::Cmper(1), musx::dom::Cmper(1));
    REQUIRE(linkedScore);
    REQUIRE(linkedPart);
    REQUIRE(editedScore);
    REQUIRE(editedPart);
    CHECK(linked.document->getOthers()->getAllSources<Shape>().size() == 1);
    CHECK(linkedPart->shapeType == Shape::ShapeType::SlurAuto);
    CHECK(editedPart != editedScore);
    CHECK(edited.document->getOthers()->getAllSources<Shape>().size() == 2);
    CHECK(editedPart->getShareMode() == Shape::ShareMode::Partial);
    CHECK(editedScore->shapeType == Shape::ShapeType::SlurAuto);
    CHECK(editedPart->shapeType == Shape::ShapeType::SlurDown);
    const auto* partType = edited.report.findField<Shape>("shapeType", musx::dom::Cmper(1), musx::dom::Cmper(1));
    REQUIRE(partType);
    CHECK(partType->origin == ValueOrigin::LegacyMus);
    CHECK(partType->rawValue == 0);
    CHECK_FALSE(editedPart->startTermSeg->endPointAdj->active);
    CHECK_FALSE(editedPart->endTermSeg->endPointAdj->active);
}

TEST_CASE("Seven-incidence SmartShapes retain endpoints through saved upgrades", "[class][smart-shape]")
{
    for (const char* fixture : {"evidence/F372/F372-F300-slur.mus", "evidence/F97/F97-F300-slur.mus", "evidence/F2000/F2000-F300-slur.mus"}) {
        const auto result = readFixture(fixture);
        const auto shape = result.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
        REQUIRE(shape);
        CHECK(shape->shapeType == Shape::ShapeType::SlurDown);
        CHECK_FALSE(shape->entryBased);
        CHECK(shape->startTermSeg->endPoint->staffId == 1);
        CHECK(shape->startTermSeg->endPoint->measId == 1);
        CHECK(shape->startTermSeg->endPoint->eduPosition == 142);
        CHECK(shape->startTermSeg->endPoint->entryNumber == 0);
        CHECK(shape->endTermSeg->endPoint->eduPosition == 1138);
        CHECK(shape->startTermSeg->endPointAdj->vertOffset == -100);
        CHECK(shape->endTermSeg->endPointAdj->vertOffset == -80);
        CHECK(shape->startTermSeg->ctlPtAdj->startCtlPtX == 171);
        CHECK(shape->startTermSeg->ctlPtAdj->startCtlPtY == -72);
        CHECK(shape->startTermSeg->endPointAdj->active);
        CHECK(shape->fullCtlPtAdj->active);
        CHECK(result.report.findField<Shape>("startTermSeg.endPoint.eduPosition", musx::dom::SCORE_PARTID, musx::dom::Cmper(1))->origin
              == ValueOrigin::LegacyMus);
    }
}

TEST_CASE("SmartShape comparator zero does not create a shape", "[class][smart-shape]")
{
    const std::vector<std::array<std::int16_t, 6>> payload{{3, static_cast<std::int16_t>(0x8200), 0, 1, 504, 3368}, {0, 0, -184, 0x4000, 0, 0},
        {0, 0, 0, 0, 0, 0}, {1, 504, 4096, 0, 0, -184}, {0x4000, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}};
    std::vector<SyntheticRow> rows;
    for (const auto cmper : {0, 36}) {
        for (const auto& words : payload) {
            rows.push_back({static_cast<std::uint16_t>(cmper), "Sx", words});
        }
    }
    const auto parsed = makeContainer(rows);
    SourceProfile profile(FormatEpoch::UncompressedLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importSmartShapes(context);

    CHECK_FALSE(document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(0)));
    const auto valid = document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(36));
    REQUIRE(valid);
    CHECK(valid->shapeType == Shape::ShapeType::Crescendo);
    CHECK(valid->startTermSeg->endPoint->measId == 504);
    CHECK(valid->startTermSeg->endPoint->staffId == 1);
    CHECK(valid->startTermSeg->endPoint->eduPosition == 3368);
    CHECK(valid->endTermSeg->endPoint->eduPosition == 4096);
    CHECK(document->getOthers()->getAllSources<Shape>().size() == 1);
}

TEST_CASE("Only lyric-based SmartShapes read the lyric tail", "[class][smart-shape]")
{
    std::vector<SyntheticRow> rows;
    for (const auto& [cmper, type, lyricFlag, lyricNum] : {std::tuple{1, 15, 0x6000, 81}, std::tuple{2, 38, 0, 1}}) {
        std::array<std::array<std::int16_t, 6>, 8> payload{};
        payload[0] = {static_cast<std::int16_t>(type), static_cast<std::int16_t>(0x8200), static_cast<std::int16_t>(lyricFlag), 1, 1, 0};
        payload[7] = {static_cast<std::int16_t>(lyricNum), static_cast<std::int16_t>(cmper == 1 ? 0x7000 : 1), 0x7665, 0x7665, 0, 0};
        for (const auto& words : payload) {
            rows.push_back({static_cast<std::uint16_t>(cmper), "Sx", words});
        }
    }
    const auto parsed = makeContainer(rows);
    SourceProfile profile(FormatEpoch::DclLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importSmartShapes(context);

    const auto slur = document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    const auto hyphen = document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(2));
    REQUIRE(slur);
    REQUIRE(hyphen);
    CHECK(slur->startLyricNum == 0);
    CHECK(slur->endLyricNum == 0);
    CHECK_FALSE(slur->startLyricType);
    CHECK_FALSE(slur->endLyricType);
    CHECK(hyphen->startLyricNum == 1);
    CHECK(hyphen->endLyricNum == 1);
    CHECK(hyphen->startLyricType == musx::dom::LyricTextType::Verse);
    CHECK(hyphen->endLyricType == musx::dom::LyricTextType::Verse);
    CHECK(report.findField<Shape>("startLyricNum", musx::dom::SCORE_PARTID, musx::dom::Cmper(1))->origin == ValueOrigin::LegacyBehavior);
    CHECK(report.findField<Shape>("startLyricNum", musx::dom::SCORE_PARTID, musx::dom::Cmper(2))->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Sources without a verified SmartShape record create no shapes", "[class][smart-shape]")
{
    for (const char* fixture :
        {"evidence/F100/F100-shapexp.mus", "evidence/F2000/F2000-empty.mus", "evidence/F2004/F2004-lyropts-nosmart-wext.mus"}) {
        const auto result = readFixture(fixture);
        CHECK(result.document->getOthers()->getArray<Shape>(musx::dom::SCORE_PARTID).empty());
    }
}

TEST_CASE("Early SmartShapes pair endpoint rows by shape comparator", "[class][smart-shape]")
{
    const auto result = readFixture("evidence/F263/F263-cresc-dim.mus");
    const auto cresc = result.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    const auto dim = result.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(2));
    REQUIRE(cresc);
    REQUIRE(dim);
    CHECK(cresc->shapeType == Shape::ShapeType::Crescendo);
    CHECK_FALSE(cresc->makeHorz);
    CHECK_FALSE(cresc->noPushEndStart);
    CHECK(cresc->startTermSeg->endPoint->eduPosition == 0);
    CHECK(cresc->endTermSeg->endPoint->eduPosition == 987);
    CHECK(cresc->startTermSeg->endPointAdj->vertOffset == -148);
    CHECK(cresc->endTermSeg->endPointAdj->vertOffset == -152);
    CHECK(dim->shapeType == Shape::ShapeType::Decrescendo);
    CHECK(dim->startTermSeg->endPoint->staffId == 1);
    CHECK(dim->startTermSeg->endPoint->measId == 1);
    CHECK(dim->startTermSeg->endPoint->eduPosition == 1888);
    CHECK(dim->endTermSeg->endPoint->eduPosition == 3456);
    CHECK(dim->startTermSeg->endPointAdj->vertOffset == -140);
    CHECK(dim->endTermSeg->endPointAdj->vertOffset == -192);
    CHECK(cresc->startTermSeg->endPointAdj->active);
    CHECK(dim->endTermSeg->breakAdj->active);
    CHECK(result.report.findField<Shape>("endTermSeg.endPoint.eduPosition", musx::dom::SCORE_PARTID, musx::dom::Cmper(2))->origin
          == ValueOrigin::LegacyMus);
    const auto* noPush = result.report.findField<Shape>("noPushEndStart", musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    const auto* makeHorz = result.report.findField<Shape>("makeHorz", musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    REQUIRE(noPush);
    REQUIRE(makeHorz);
    CHECK(noPush->origin == ValueOrigin::LegacyBehavior);
    CHECK(makeHorz->origin == ValueOrigin::LegacyBehavior);
}

TEST_CASE("Early SmartShape offset row populates endpoint and break adjustments", "[class][smart-shape]")
{
    const std::vector<SyntheticRow> rows{{1, "sX", {1, 3, 0, 0, 12, static_cast<std::int16_t>(0x8200)}},
        {1, "eX", {1, 3, 0, 0, 12, static_cast<std::int16_t>(0x8200)}}, {1, "DY", {1, 100, -40, 0, 0, 1}}, {1, "DY", {1, 900, -60, 0, 0, 1}},
        {1, "oX", {-11, 6, -22, -19, 12, -30}}};
    const auto parsed = makeContainer(rows);
    SourceProfile profile(FormatEpoch::UncompressedLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importSmartShapes(context);

    const auto shape = document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    REQUIRE(shape);
    CHECK(shape->startTermSeg->endPointAdj->horzOffset == -11);
    CHECK(shape->startTermSeg->breakAdj->horzOffset == 6);
    CHECK(shape->startTermSeg->breakAdj->vertOffset == -22);
    CHECK(shape->endTermSeg->endPointAdj->horzOffset == -19);
    CHECK(shape->endTermSeg->breakAdj->horzOffset == 12);
    CHECK(shape->endTermSeg->breakAdj->vertOffset == -30);
    const auto* startOffset = report.findField<Shape>("startTermSeg.endPointAdj.horzOffset", musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    const auto* endBreak = report.findField<Shape>("endTermSeg.breakAdj.vertOffset", musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    REQUIRE(startOffset);
    REQUIRE(endBreak);
    CHECK(startOffset->origin == ValueOrigin::LegacyMus);
    CHECK(startOffset->sourceIdentity == finale_mus_reader::records::packTag("oX"));
    CHECK(startOffset->rawValue == -11);
    CHECK(endBreak->origin == ValueOrigin::LegacyMus);
    CHECK(endBreak->rawValue == -30);
    CHECK(endBreak->decodedOffset == startOffset->decodedOffset + 10);
}

TEST_CASE("Early SmartShape without an offset row leaves break adjustments inactive", "[class][smart-shape]")
{
    const auto parsed = makeContainer({{1, "sX", {1, 3, 0, 0, 12, static_cast<std::int16_t>(0x8200)}},
        {1, "eX", {1, 3, 0, 0, 12, static_cast<std::int16_t>(0x8200)}}, {1, "DY", {1, 100, -40, 0, 0, 1}}, {1, "DY", {1, 900, -60, 0, 0, 1}}});
    SourceProfile profile(FormatEpoch::UncompressedLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importSmartShapes(context);

    const auto shape = document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    REQUIRE(shape);
    CHECK(shape->startTermSeg->endPointAdj->active);
    CHECK(shape->endTermSeg->endPointAdj->active);
    CHECK_FALSE(shape->startTermSeg->breakAdj->active);
    CHECK_FALSE(shape->endTermSeg->breakAdj->active);
    const auto* endpointActive = report.findField<Shape>("startTermSeg.endPointAdj.active", musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    const auto* breakActive = report.findField<Shape>("startTermSeg.breakAdj.active", musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    REQUIRE(endpointActive);
    REQUIRE(breakActive);
    CHECK(endpointActive->origin == ValueOrigin::LegacyBehavior);
    CHECK(breakActive->origin == ValueOrigin::Unmapped);
}

TEST_CASE("Eight-incidence lyric shapes decode text-kind tags", "[class][smart-shape]")
{
    const auto result = readFixture("evidence/F2005/F2005-breakwexts.mus");
    const auto shape = result.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(6));
    REQUIRE(shape);
    CHECK(shape->startLyricType == musx::dom::LyricTextType::Verse);
    CHECK(result.report.findField<Shape>("startLyricType", musx::dom::SCORE_PARTID, musx::dom::Cmper(6))->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Lyric shape tags retain their text kinds across saves", "[class][smart-shape]")
{
    for (const char* fixture :
        {"evidence/F2006/F2006-lyric-vcs.mus", "evidence/F2008/F2008-F2006-lyric-vcs.mus", "evidence/F2012/F2012-F2006-lyric-vcs.mus"}) {
        INFO(fixture);
        const auto result = readFixture(fixture);
        for (const auto& [cmper, expectedType] : {std::pair{1, musx::dom::LyricTextType::Verse}, std::pair{8, musx::dom::LyricTextType::Chorus},
                 std::pair{11, musx::dom::LyricTextType::Section}}) {
            INFO(cmper);
            const auto shape = result.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(cmper));
            REQUIRE(shape);
            CHECK(shape->startLyricType == expectedType);
            CHECK(shape->endLyricType == expectedType);
            CHECK(
                result.report.findField<Shape>("startLyricType", musx::dom::SCORE_PARTID, musx::dom::Cmper(cmper))->origin == ValueOrigin::LegacyMus);
        }
        const auto extension = result.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(9));
        REQUIRE(extension);
        CHECK(extension->startLyricType == musx::dom::LyricTextType::Chorus);
        CHECK_FALSE(extension->endLyricType.has_value());
    }
}

TEST_CASE("Early slurs retain their own endpoints", "[class][smart-shape]")
{
    for (const auto& [fixture, start, end] :
        {std::tuple{"evidence/F263/F263-slur.mus", 58, 1016}, std::tuple{"evidence/F300/F300-slur.mus", 142, 1138}}) {
        const auto result = readFixture(fixture);
        const auto shape = result.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
        REQUIRE(shape);
        CHECK(shape->shapeType == Shape::ShapeType::SlurDown);
        CHECK_FALSE(shape->makeHorz);
        CHECK_FALSE(shape->noPushEndStart);
        CHECK(shape->startTermSeg->endPoint->eduPosition == start);
        CHECK(shape->endTermSeg->endPoint->eduPosition == end);
        CHECK(shape->startTermSeg->endPointAdj->active);
        CHECK(shape->startTermSeg->breakAdj->active);
        CHECK(shape->endTermSeg->endPointAdj->active);
        CHECK(shape->endTermSeg->breakAdj->active);
        const auto* activation = result.report.findField<Shape>("startTermSeg.endPointAdj.active", musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
        REQUIRE(activation);
        CHECK(activation->origin == ValueOrigin::LegacyBehavior);
        const auto* noPush = result.report.findField<Shape>("noPushEndStart", musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
        const auto* makeHorz = result.report.findField<Shape>("makeHorz", musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
        REQUIRE(noPush);
        REQUIRE(makeHorz);
        CHECK(noPush->origin == ValueOrigin::LegacyBehavior);
        CHECK(makeHorz->origin == ValueOrigin::LegacyBehavior);
    }
}

} // namespace
} // namespace finale_mus_reader_tests
