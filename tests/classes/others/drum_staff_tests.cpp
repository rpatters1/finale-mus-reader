// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

using DrumStaff = musx::dom::others::DrumStaff;

ImportReport importDrumStaff(const finale_mus_reader::container::ParsedContainer &parsed,
                             const SourceProfile &profile, const musx::dom::DocumentPtr &document) {
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index,     profile, noSource, document,
                                                   reference, report,  pending,  construction};
    finale_mus_reader::others::importDrumStaff(context);
    return report;
}

musx::dom::DocumentPtr emptyDrumStaffDocument() {
    auto session = musx::factory::DocumentFactory::begin();
    return session.getDocument();
}

TEST_CASE("Fixed-row drum staffs read their map association before the "
          "selection words") {
    for (const auto epoch : {FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            const auto parsed =
                makeContainer({{7, "DS", {3, 0, 0, 0, 0x4000, std::int16_t(-2206)}},
                               {7, "DS", {15, 0, 0, 0, 0, 0}}},
                              epoch, byteOrder);
            const auto document = emptyDrumStaffDocument();
            auto profile = SourceProfile(epoch);
            profile.byteOrder = byteOrder;
            const auto report = importDrumStaff(parsed, profile, document);

            const auto staff = document->getOthers()->get<DrumStaff>(musx::dom::SCORE_PARTID, 7);
            REQUIRE(staff);
            CHECK(staff->whichDrumLib == 3);
            CHECK(reportedFieldCount(report) == 1);
            CHECK(field(report, "others.drumStaff[7].whichDrumLib").origin ==
                  ValueOrigin::LegacyMus);
            CHECK(field(report, "others.drumStaff[7].whichDrumLib").rawValue == 3);
        }
    }
}

TEST_CASE("Coda-banner files predate drum staffs") {
    const auto parsed =
        makeContainer({{7, "DS", {3, 0, 0, 0, 0x4000, 0}}}, FormatEpoch::CodaBanner);
    const auto document = emptyDrumStaffDocument();
    SourceProfile profile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto report = importDrumStaff(parsed, profile, document);

    CHECK(document->getOthers()->getAllSources<DrumStaff>().empty());
    CHECK(reportedFieldCount(report) == 0);
}

TEST_CASE("Zlib drum staffs use word zero in both stored payload widths") {
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        for (const auto &words : {std::vector<std::int16_t>{9, 0, 0, 0, 0, 0},
                                  std::vector<std::int16_t>{9, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6}}) {
            const auto parsed = makeClassContainer(0x0084, words, byteOrder, 14);
            const auto document = emptyDrumStaffDocument();
            SourceProfile profile(FormatEpoch::ZlibLegacy);
            profile.byteOrder = byteOrder;
            const auto report = importDrumStaff(parsed, profile, document);

            const auto staff = document->getOthers()->get<DrumStaff>(musx::dom::SCORE_PARTID, 14);
            REQUIRE(staff);
            CHECK(staff->whichDrumLib == 9);
            CHECK(reportedFieldCount(report) == 1);
        }
    }
}

TEST_CASE("A drum-staff record without its map word is rejected") {
    const auto parsed = makeClassContainer(0x0084, {}, ByteOrder::LittleEndian, 4);
    const auto document = emptyDrumStaffDocument();
    SourceProfile profile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    const auto report = importDrumStaff(parsed, profile, document);

    CHECK(document->getOthers()->getAllSources<DrumStaff>().empty());
    REQUIRE(report.diagnostics.size() == 1);
    CHECK(report.diagnostics.front().message.find("shorter than its layout") != std::string::npos);
}

} // namespace
} // namespace finale_mus_reader_tests
