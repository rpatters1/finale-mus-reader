// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace finale_mus_reader_tests {
namespace {

using namespace classes;
using ClefList = musx::dom::others::ClefList;
using musx::dom::ShowClefMode;
constexpr musx::dom::Cmper clefListCmper = 4;
constexpr musx::dom::Cmper fixtureClefListCmper = 1;
constexpr std::uint16_t clefListClassId = 0x007f;

ImportReport importClefList(const finale_mus_reader::container::ParsedContainer& parsed, musx::dom::DocumentPtr& document)
{
    const auto index = LegacyRecordIndex::build(parsed);
    auto session = musx::factory::DocumentFactory::begin();
    document = session.getDocument();
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    ImportReport report(parsed.formatEpoch);
    finale_mus_reader::PendingReferences pending;
    SourceProfile profile(parsed.formatEpoch);
    profile.byteOrder = parsed.byteOrder;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importClefLists(context);
    return report;
}

// A barline treble clef, then a hidden bass clef at Edu 1536 moved 12 EVPU down and 20 left,
// then a forced alto clef carrying the vertical-drag and after-barline bits.
constexpr std::array<std::int16_t, 6> barlineClefItem{0, 0, 0, 75, 0, 0};
constexpr std::array<std::int16_t, 6> hiddenClefItem{3, 1536, -12, 75, -20, 0x0002};
constexpr std::array<std::int16_t, 6> forcedClefItem{1, 3072, 0, 60, 0, 0x001c};

void checkClefList(const musx::dom::DocumentPtr& document, const ImportReport& report)
{
    const auto list = document->getOthers()->getArray<ClefList>(musx::dom::SCORE_PARTID, clefListCmper);
    REQUIRE(list.size() == 3);

    CHECK(list[0]->clefIndex == 0);
    CHECK(list[0]->xEduPos == 0);
    CHECK(list[0]->percent == 75);
    CHECK(list[0]->clefMode == ShowClefMode::WhenNeeded);
    CHECK_FALSE(list[0]->unlockVert);
    CHECK_FALSE(list[0]->afterBarline);

    CHECK(list[1]->clefIndex == 3);
    CHECK(list[1]->xEduPos == 1536);
    CHECK(list[1]->yEvpuPos == -12);
    CHECK(list[1]->percent == 75);
    CHECK(list[1]->xEvpuOffset == -20);
    CHECK(list[1]->clefMode == ShowClefMode::Never);
    CHECK_FALSE(list[1]->unlockVert);
    CHECK_FALSE(list[1]->afterBarline);

    CHECK(list[2]->clefIndex == 1);
    CHECK(list[2]->xEduPos == 3072);
    CHECK(list[2]->percent == 60);
    CHECK(list[2]->clefMode == ShowClefMode::Always);
    CHECK(list[2]->unlockVert);
    CHECK(list[2]->afterBarline);

    for (musx::dom::Inci inci = 0; inci < 3; ++inci) {
        const auto key = finale_mus_reader::instanceKey<ClefList>(musx::dom::SCORE_PARTID, clefListCmper, inci);
        REQUIRE(report.fields.contains(key));
        const auto& fields = report.fields.at(key);
        CHECK(fields.size() == ClefList::xmlMappingArray().size());
        for (const auto& [member, info] : fields) {
            INFO(member);
            CHECK(info.origin == ValueOrigin::LegacyMus);
        }
    }
    CHECK(field(report, "others.clefEnum[4,1].clefIndex").rawValue == 3);
    CHECK(field(report, "others.clefEnum[4,1].clefMode").rawValue == 0x0002);
    CHECK(field(report, "others.clefEnum[4,2].afterBarline").rawValue == 0x001c);
}

std::vector<SyntheticRow> fixedClefRows()
{
    return {{clefListCmper, "CE", barlineClefItem}, {clefListCmper, "CE", hiddenClefItem}, {clefListCmper, "CE", forcedClefItem}};
}

std::vector<std::int16_t> clefClassWords()
{
    std::vector<std::int16_t> words;
    for (const auto* item : {&barlineClefItem, &hiddenClefItem, &forcedClefItem}) {
        words.insert(words.end(), item->begin(), item->end());
    }
    return words;
}

TEST_CASE("ClefList recovers one item per incidence in every fixed-row epoch", "[class][clef-list]")
{
    for (const auto epoch : {FormatEpoch::UncompressedLegacy, FormatEpoch::DclLegacy}) {
        for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            musx::dom::DocumentPtr document;
            const auto report = importClefList(makeContainer(fixedClefRows(), epoch, byteOrder), document);
            checkClefList(document, report);
        }
    }
}

TEST_CASE("ClefList splits a zlib payload into six-word items", "[class][clef-list]")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        musx::dom::DocumentPtr document;
        const auto report = importClefList(makeClassContainer(clefListClassId, clefClassWords(), byteOrder, clefListCmper), document);
        checkClefList(document, report);
    }
}

TEST_CASE("ClefList is not imported from the Coda-banner layout", "[class][clef-list]")
{
    musx::dom::DocumentPtr document;
    const auto report = importClefList(makeContainer({{clefListCmper, "CE", {3, 145, 0, 75, 0, 0}}}, FormatEpoch::CodaBanner), document);
    CHECK(document->getOthers()->getArray<ClefList>(musx::dom::SCORE_PARTID, clefListCmper).empty());
    CHECK(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
        [](const auto& diagnostic) { return diagnostic.message.find("Mid-measure clefs") != std::string::npos; }));
}

TEST_CASE("ClefList rejects an incomplete trailing item", "[class][clef-list]")
{
    std::vector<std::int16_t> words(barlineClefItem.begin(), barlineClefItem.end());
    words.push_back(3);
    musx::dom::DocumentPtr document;
    const auto report = importClefList(makeClassContainer(clefListClassId, words, ByteOrder::LittleEndian, clefListCmper), document);
    CHECK(document->getOthers()->getArray<ClefList>(musx::dom::SCORE_PARTID, clefListCmper).size() == 1);
    CHECK(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
        [](const auto& diagnostic) { return diagnostic.message.find("incomplete trailing item") != std::string::npos; }));
}

TEST_CASE("ClefList recovers a controlled DCL list", "[class][clef-list][fixture]")
{
    const auto result = readFixture("evidence/F2001/F2001Win-midclef.mus");
    const auto list = result.document->getOthers()->getArray<ClefList>(musx::dom::SCORE_PARTID, fixtureClefListCmper);
    REQUIRE(list.size() == 3);
    CHECK(list[0]->clefIndex == 0);
    CHECK(list[0]->xEduPos == 0);
    CHECK(list[0]->percent == 75);
    CHECK_FALSE(list[0]->unlockVert);
    CHECK_FALSE(list[0]->afterBarline);
    CHECK(list[1]->clefIndex == 3);
    CHECK(list[1]->xEduPos == 1645);
    CHECK(list[1]->yEvpuPos == 12);
    CHECK(list[1]->percent == 75);
    CHECK(list[1]->xEvpuOffset == 0);
    CHECK(list[1]->clefMode == ShowClefMode::WhenNeeded);
    CHECK(list[1]->unlockVert);
    CHECK_FALSE(list[1]->afterBarline);
    CHECK(list[2]->clefIndex == 2);
    CHECK(list[2]->xEduPos == 2912);
    CHECK(list[2]->yEvpuPos == 0);
    CHECK_FALSE(list[2]->unlockVert);
    CHECK(list[2]->afterBarline);
    CHECK(field(result, "others.clefEnum[1,1].unlockVert").rawValue == 0x0004);
    CHECK(field(result, "others.clefEnum[1,2].afterBarline").rawValue == 0x0010);
}

TEST_CASE("ClefList recovers a Coda-banner list upgraded by Finale 3.7.2", "[class][clef-list][fixture]")
{
    const auto result = readFixture("evidence/F372/F372-F263-midclef1.mus");
    const auto list = result.document->getOthers()->getArray<ClefList>(musx::dom::SCORE_PARTID, fixtureClefListCmper);
    REQUIRE(list.size() == 2);
    CHECK(list[0]->clefIndex == 0);
    CHECK(list[0]->xEduPos == 0);
    CHECK(list[0]->percent == 0);
    CHECK(list[1]->clefIndex == 3);
    CHECK(list[1]->xEduPos == 356);
    CHECK(list[1]->yEvpuPos == -16);
    CHECK(list[1]->percent == 75);
}

TEST_CASE("ClefList skips controlled Coda-banner lists", "[class][clef-list][fixture]")
{
    for (const auto* fixture : {"evidence/F100/F100-midclef1.mus", "evidence/F100/F100-midclef2.mus", "evidence/F263/F263-midclef1.mus",
             "evidence/F263/F263-midclef2.mus"}) {
        CAPTURE(fixture);
        const auto result = readFixture(fixture);
        CHECK(result.document->getOthers()->getArray<ClefList>(musx::dom::SCORE_PARTID).empty());
        CHECK(std::any_of(result.report.diagnostics.begin(), result.report.diagnostics.end(),
            [](const auto& diagnostic) { return diagnostic.message.find("Mid-measure clefs") != std::string::npos; }));
    }
}

TEST_CASE("ClefList creates nothing without a stored list", "[class][clef-list][fixture]")
{
    for (const auto* fixture : {"evidence/F100/F100-clef.mus", "evidence/F2005/F2005-clef-baseline.mus"}) {
        CAPTURE(fixture);
        const auto parsed = readFixture(fixture);
        CHECK(parsed.document->getOthers()->getArray<ClefList>(musx::dom::SCORE_PARTID).empty());
        CHECK(std::none_of(parsed.report.diagnostics.begin(), parsed.report.diagnostics.end(), [](const auto& diagnostic) {
            return diagnostic.message.find("clef list") != std::string::npos || diagnostic.message.find("Mid-measure clefs") != std::string::npos;
        }));
    }
}

} // namespace
} // namespace finale_mus_reader_tests
