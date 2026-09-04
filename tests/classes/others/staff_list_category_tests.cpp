// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

using CategoryName = musx::dom::others::StaffListCategoryName;
using CategoryParts = musx::dom::others::StaffListCategoryParts;
using CategoryScore = musx::dom::others::StaffListCategoryScore;

musx::dom::DocumentPtr emptyDocument()
{
    auto session = musx::factory::DocumentFactory::begin();
    return session.getDocument();
}

musx::dom::DocumentPtr categoryBaseline()
{
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    for (musx::dom::Cmper cmper = 1; cmper <= 8; ++cmper) {
        auto parts = std::make_shared<CategoryParts>(document, musx::dom::SCORE_PARTID,
                                                     musx::dom::EnigmaBase::ShareMode::All, cmper);
        parts->values = {-1};
        document->getOthers()->add(CategoryParts::XmlNodeName, parts);
        auto score = std::make_shared<CategoryScore>(document, musx::dom::SCORE_PARTID,
                                                     musx::dom::EnigmaBase::ShareMode::All, cmper);
        score->values = {-1};
        document->getOthers()->add(CategoryScore::XmlNodeName, score);
    }
    return std::move(session).finish();
}

ImportReport importStaffListCategories(const finale_mus_reader::container::ParsedContainer& parsed,
                                       const SourceProfile& profile,
                                       const musx::dom::DocumentPtr& document,
                                       const musx::dom::DocumentPtr& reference = emptyDocument())
{
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index,     profile, noSource, document,
                                                   reference, report,  pending,  construction};
    finale_mus_reader::others::importStaffListCategories(context);
    return report;
}

TEST_CASE("Finale 2009 category staff lists and platform names recover", "[class]")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        const auto parsed = makeClassContainer(
            {
                SyntheticClassRow{0x012f, {0x4180, 0, 0, 0, 0, 0}, 3},
                SyntheticClassRow{0x0130, {-1, 2, 7, 0, 0, 0}, 3},
                SyntheticClassRow{0x0132, {-2, 9, 0, 0, 0, 0}, 3},
            },
            byteOrder);
        auto profile = profileFor(finale_mus_reader::versions::finale2009.major);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = byteOrder;
        profile.platform = finale_mus_reader::SourcePlatform::Windows;
        const auto document = emptyDocument();
        const auto report = importStaffListCategories(parsed, profile, document);

        const auto name = document->getOthers()->get<CategoryName>(musx::dom::SCORE_PARTID, 3);
        const auto score = document->getOthers()->get<CategoryScore>(musx::dom::SCORE_PARTID, 3);
        const auto parts = document->getOthers()->get<CategoryParts>(musx::dom::SCORE_PARTID, 3);
        REQUIRE(name);
        CHECK(name->name == "€A");
        REQUIRE(score);
        CHECK(score->values == std::vector<musx::dom::StaffCmper>{-2, 9});
        REQUIRE(parts);
        CHECK(parts->values == std::vector<musx::dom::StaffCmper>{-1, 2, 7});
        CHECK(reportedFieldCount(report) == 5);
        const auto* nameOrigin = report.findInstanceOrigin(
            finale_mus_reader::instanceKey<CategoryName>(musx::dom::SCORE_PARTID, 3));
        REQUIRE(nameOrigin);
        CHECK(*nameOrigin == ValueOrigin::LegacyMus);
    }
}

TEST_CASE("Finale 2012 category names remain one-byte platform text", "[class]")
{
    auto profile = profileFor(finale_mus_reader::versions::finale2012.major);
    profile.epoch = FormatEpoch::ZlibLegacy;
    profile.byteOrder = ByteOrder::LittleEndian;
    profile.platform = finale_mus_reader::SourcePlatform::Windows;
    const auto document = emptyDocument();
    const auto report = importStaffListCategories(
        makeClassContainer(0x012f, {0x4180, 0, 0, 0, 0, 0}, ByteOrder::LittleEndian, 4), profile,
        document, categoryBaseline());
    const auto name = document->getOthers()->get<CategoryName>(musx::dom::SCORE_PARTID, 4);
    REQUIRE(name);
    CHECK(name->name == "€A");
    CHECK_FALSE(document->getOthers()->get<CategoryParts>(musx::dom::SCORE_PARTID, 4));
    CHECK_FALSE(document->getOthers()->get<CategoryScore>(musx::dom::SCORE_PARTID, 4));
    CHECK(document->getOthers()->get<CategoryParts>(musx::dom::SCORE_PARTID, 3));
    CHECK(document->getOthers()->get<CategoryScore>(musx::dom::SCORE_PARTID, 5));
    CHECK(report.findInstanceOrigin(
              finale_mus_reader::instanceKey<CategoryName>(musx::dom::SCORE_PARTID, 4)) != nullptr);
}

TEST_CASE("Pre-Finale 2009 files receive the pinned category staff lists", "[class]")
{
    const auto result = readFixture("evidence/F100/F100-baseline.mus");
    const auto parts =
        result.document->getOthers()->getArray<CategoryParts>(musx::dom::SCORE_PARTID);
    const auto score =
        result.document->getOthers()->getArray<CategoryScore>(musx::dom::SCORE_PARTID);
    CHECK(result.document->getOthers()->getArray<CategoryName>(musx::dom::SCORE_PARTID).empty());
    REQUIRE(parts.size() == 8);
    REQUIRE(score.size() == 8);
    for (std::size_t index = 0; index < 8; ++index) {
        CHECK(parts[index]->getCmper() == static_cast<musx::dom::Cmper>(index + 1));
        CHECK(parts[index]->values == std::vector<musx::dom::StaffCmper>{-1});
        CHECK(score[index]->getCmper() == static_cast<musx::dom::Cmper>(index + 1));
        CHECK(score[index]->values == std::vector<musx::dom::StaffCmper>{-1});
        const auto* partsOrigin =
            result.report.findInstanceOrigin(finale_mus_reader::instanceKey<CategoryParts>(
                musx::dom::SCORE_PARTID, static_cast<musx::dom::Cmper>(index + 1)));
        const auto* scoreOrigin =
            result.report.findInstanceOrigin(finale_mus_reader::instanceKey<CategoryScore>(
                musx::dom::SCORE_PARTID, static_cast<musx::dom::Cmper>(index + 1)));
        REQUIRE(partsOrigin);
        REQUIRE(scoreOrigin);
        CHECK(*partsOrigin == ValueOrigin::Finale27Default);
        CHECK(*scoreOrigin == ValueOrigin::Finale27Default);
    }
}

TEST_CASE("Finale 2009 category lists are filled from four through eight", "[class]")
{
    auto profile = profileFor(finale_mus_reader::versions::finale2009.major);
    profile.epoch = FormatEpoch::ZlibLegacy;
    profile.byteOrder = ByteOrder::LittleEndian;
    const auto document = emptyDocument();
    std::vector<SyntheticClassRow> rows;
    for (musx::dom::Cmper cmper = 1; cmper <= 4; ++cmper) {
        rows.emplace_back(
            0x0130, std::vector<std::int16_t>{static_cast<std::int16_t>(cmper), 0, 0, 0, 0, 0},
            cmper);
        rows.emplace_back(0x0132, std::vector<std::int16_t>{-2, 0, 0, 0, 0, 0}, cmper);
    }
    const auto report = importStaffListCategories(makeClassContainer(rows, ByteOrder::LittleEndian),
                                                  profile, document, categoryBaseline());
    CHECK(document->getOthers()->getAllSources<CategoryName>().empty());
    const auto parts = document->getOthers()->getArray<CategoryParts>(musx::dom::SCORE_PARTID);
    const auto score = document->getOthers()->getArray<CategoryScore>(musx::dom::SCORE_PARTID);
    REQUIRE(parts.size() == 8);
    REQUIRE(score.size() == 8);
    for (std::size_t index = 0; index < 4; ++index) {
        CHECK(score[index]->values == std::vector<musx::dom::StaffCmper>{-2});
        CHECK(parts[index]->values ==
              std::vector<musx::dom::StaffCmper>{static_cast<musx::dom::StaffCmper>(index + 1)});
    }
    for (std::size_t index = 4; index < 8; ++index) {
        CHECK(parts[index]->values == std::vector<musx::dom::StaffCmper>{-1});
        CHECK(score[index]->values == std::vector<musx::dom::StaffCmper>{-1});
        const auto* origin =
            report.findInstanceOrigin(finale_mus_reader::instanceKey<CategoryScore>(
                musx::dom::SCORE_PARTID, static_cast<musx::dom::Cmper>(index + 1)));
        REQUIRE(origin);
        CHECK(*origin == ValueOrigin::Finale27Default);
    }
    CHECK(reportedFieldCount(report) == 8);
}

TEST_CASE("Category staff-list overrides are reported and ignored", "[class]")
{
    auto profile = profileFor(finale_mus_reader::versions::finale2009.major);
    profile.epoch = FormatEpoch::ZlibLegacy;
    profile.byteOrder = ByteOrder::LittleEndian;
    const auto document = emptyDocument();
    const auto report =
        importStaffListCategories(makeClassContainer(
                                      {
                                          SyntheticClassRow{0x0131, {-1, 0, 0, 0, 0, 0}, 2},
                                          SyntheticClassRow{0x0133, {-2, 0, 0, 0, 0, 0}, 5, 3},
                                      },
                                      ByteOrder::LittleEndian),
                                  profile, document);
    CHECK(document->getOthers()->getAllSources<CategoryName>().empty());
    CHECK(document->getOthers()->getAllSources<CategoryParts>().empty());
    CHECK(document->getOthers()->getAllSources<CategoryScore>().empty());
    REQUIRE(report.diagnostics.size() == 2);
    CHECK(report.diagnostics[0].level == musx::util::Logger::LogLevel::Info);
    CHECK(report.diagnostics[0].message.find("score staff-list override 2") != std::string::npos);
    CHECK(report.diagnostics[1].level == musx::util::Logger::LogLevel::Info);
    CHECK(report.diagnostics[1].message.find("parts staff-list override 5 for part 3") !=
          std::string::npos);
}

} // namespace
} // namespace finale_mus_reader_tests
