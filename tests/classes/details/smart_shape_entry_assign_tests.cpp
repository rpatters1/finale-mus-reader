// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;
using EntryAssignTestTarget = musx::dom::details::SmartShapeEntryAssign;

TEST_CASE("SmartShape entry class tuple decodes in either byte order", "[class][smart-shape-entry-assign]")
{
    for (const auto order : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        const auto parsed = makeDetailClassContainer(0, 27, 0, {45, 7, 0, 0, 0}, order, 0x041a);
        auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
        profile.byteOrder = order;
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        ImportReport report(profile.epoch);
        const auto index = LegacyRecordIndex::build(parsed);
        auto referenceSession = musx::factory::DocumentFactory::begin();
        const auto reference = std::move(referenceSession).finish();
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::details::importSmartShapeEntryAssigns(context);
        const auto assignment = document->getDetails()->get<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 27, 0);
        REQUIRE(assignment);
        CHECK(assignment->shapeNum == 45);
    }
}

TEST_CASE("SmartShape entry assignments retain their shape identifiers", "[class][smart-shape-entry-assign]")
{
    const auto result = readFixture("evidence/F2006/F2006-lyric-vcs.mus");
    const auto assignments = result.document->getDetails()->getArray<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 27);
    REQUIRE(assignments.size() == 3);
    CHECK(assignments[0]->shapeNum == 4);
    CHECK(assignments[1]->shapeNum == 9);
    CHECK(assignments[2]->shapeNum == 16);
    const auto key = finale_mus_reader::instanceKey<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 0, 1, 27);
    const auto* shapeNum = result.report.findField(key, "shapeNum");
    REQUIRE(shapeNum);
    CHECK(shapeNum->origin == ValueOrigin::LegacyMus);
    CHECK(shapeNum->rawValue == 9);
}

TEST_CASE("Pre-Sx shapes have no entry assignments", "[class][smart-shape-entry-assign]")
{
    const auto result = readFixture("evidence/F263/F263-slur.mus");
    CHECK(result.document->getDetails()->getAllSources<EntryAssignTestTarget>().empty());
}

TEST_CASE("Zlib SmartShape entry assignments decode class tuples", "[class][smart-shape-entry-assign]")
{
    const auto result = readFixture("evidence/F2007/F2007-lyric-hyphens.mus");
    const auto first = result.document->getDetails()->get<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 13, 0);
    const auto second = result.document->getDetails()->get<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 15, 0);
    REQUIRE(first);
    REQUIRE(second);
    CHECK(first->shapeNum == 1);
    CHECK(second->shapeNum == 1);
    const auto key = finale_mus_reader::instanceKey<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 0, 0, 13);
    const auto* field = result.report.findField(key, "shapeNum");
    REQUIRE(field);
    CHECK(field->sourceIdentity == 0x041a);
}

TEST_CASE("Later zlib SmartShape entry class follows moved slur endpoints", "[class][smart-shape-entry-assign]")
{
    const auto baseline = readFixture("evidence/F2012/F2012-slur-part.mus");
    const auto changed = readFixture("evidence/F2012/F2012-slur-diffents.mus");
    const auto baselineStart = baseline.document->getDetails()->get<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 5, 0);
    const auto changedStart = changed.document->getDetails()->get<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 6, 0);
    const auto changedEnd = changed.document->getDetails()->get<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 7, 0);
    REQUIRE(baselineStart);
    REQUIRE(changedStart);
    REQUIRE(changedEnd);
    CHECK(baselineStart->shapeNum == 1);
    CHECK(changedStart->shapeNum == 1);
    CHECK(changedEnd->shapeNum == 1);
    CHECK_FALSE(changed.document->getDetails()->get<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 5, 0));
    const auto key = finale_mus_reader::instanceKey<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 0, 0, 7);
    const auto* field = changed.report.findField(key, "shapeNum");
    REQUIRE(field);
    CHECK(field->sourceIdentity == 0x0428);
}

TEST_CASE("Both zlib entry assignment classes can occur in one document", "[class][smart-shape-entry-assign]")
{
    const auto result = readFixture("evidence/F2008/F2008-F2006-lyric-vcs.mus");
    const auto older = result.document->getDetails()->get<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 17, 0);
    const auto later = result.document->getDetails()->get<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 27, 0);
    REQUIRE(older);
    REQUIRE(later);
    CHECK(older->shapeNum == 1);
    CHECK(later->shapeNum == 4);
    const auto olderKey = finale_mus_reader::instanceKey<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 0, 0, 17);
    const auto laterKey = finale_mus_reader::instanceKey<EntryAssignTestTarget>(musx::dom::SCORE_PARTID, 0, 0, 27);
    REQUIRE(result.report.findField(olderKey, "shapeNum"));
    REQUIRE(result.report.findField(laterKey, "shapeNum"));
    CHECK(result.report.findField(olderKey, "shapeNum")->sourceIdentity == 0x041a);
    CHECK(result.report.findField(laterKey, "shapeNum")->sourceIdentity == 0x0428);
}

} // namespace
} // namespace finale_mus_reader_tests
