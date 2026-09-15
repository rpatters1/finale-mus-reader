// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "class_test_support.h"
#include "coverage/registry.h"
#include "coverage/surveyors/shared/staff_fields.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;
using StaffUsed = musx::dom::others::StaffUsed;
using StaffSystem = musx::dom::others::StaffSystem;
using Measure = musx::dom::others::Measure;

std::vector<std::int16_t> staffUsedWords(std::uint16_t staffId, std::int32_t distance, std::int16_t startMeas = 1, std::int32_t startEdu = 0,
    std::int16_t endMeas = 32767, std::int32_t endEdu = (std::numeric_limits<std::int32_t>::max)(), ByteOrder byteOrder = ByteOrder::BigEndian)
{
    const auto appendLong = [byteOrder](std::vector<std::int16_t>& result, std::int32_t value) {
        const auto high = static_cast<std::int16_t>(static_cast<std::uint32_t>(value) >> 16U);
        const auto low = static_cast<std::int16_t>(value);
        result.push_back(byteOrder == ByteOrder::BigEndian ? high : low);
        result.push_back(byteOrder == ByteOrder::BigEndian ? low : high);
    };
    std::vector<std::int16_t> result{static_cast<std::int16_t>(staffId), 0, 0, 0};
    appendLong(result, distance);
    result.push_back(startMeas);
    appendLong(result, startEdu);
    result.push_back(endMeas);
    appendLong(result, endEdu);
    return result;
}

void appendStaffUsedRows(std::vector<SyntheticRow>& rows, std::uint16_t cmper, const std::vector<std::int16_t>& words, const char* tag = "Iu")
{
    for (std::size_t offset = 0; offset < words.size(); offset += 6) {
        rows.push_back({cmper, tag, {words[offset], words[offset + 1], words[offset + 2], words[offset + 3], words[offset + 4], words[offset + 5]}});
    }
}

struct StaffUsedImport
{
    musx::dom::DocumentPtr document;
    ImportReport report;
};

StaffUsedImport importStaffUsed(
    const finale_mus_reader::container::ParsedContainer& parsed, const SourceProfile& profile, bool includeLayoutClasses = false)
{
    auto session = musx::factory::DocumentFactory::begin();
    auto document = session.getDocument();
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importStaffUsed(context);
    if (includeLayoutClasses) {
        finale_mus_reader::others::importStaffSystems(context);
        finale_mus_reader::others::importPartGlobals(context);
    }
    finale_mus_reader::runDeferredChecks(pending);
    return {std::move(document), std::move(report)};
}

void checkWholeDocumentRange(const StaffUsed& value)
{
    REQUIRE(value.range);
    CHECK(value.range->startMeas == 1);
    CHECK(value.range->startEdu == 0);
    CHECK(value.range->endMeas == 32767);
    CHECK(value.range->endEdu == (std::numeric_limits<std::int32_t>::max)());
}

TEST_CASE("StaffUsed recovers every represented physical layout", "[class][staff-used]")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        {
            const auto parsed = makeContainer({{0, "IU", {1, 0, -80, 2, 0, -380}}}, FormatEpoch::CodaBanner, byteOrder);
            auto profile = SourceProfile(FormatEpoch::CodaBanner);
            profile.byteOrder = byteOrder;
            auto imported = importStaffUsed(parsed, profile);
            const auto values = imported.document->getOthers()->getArray<StaffUsed>(0, 0);
            REQUIRE(values.size() == 2);
            CHECK(values[0]->staffId == 1);
            CHECK(values[0]->distFromTop == 0);
            CHECK(values[1]->staffId == 2);
            CHECK(values[1]->distFromTop == -300);
            checkWholeDocumentRange(*values[0]);
            CHECK(reportedFieldCount(imported.report) == 12);
        }
        {
            auto firstWords = staffUsedWords(1, -188, 1, 0, 32767, (std::numeric_limits<std::int32_t>::max)(), byteOrder);
            auto secondWords = staffUsedWords(2, -488, 1, 0, 32767, (std::numeric_limits<std::int32_t>::max)(), byteOrder);
            firstWords.resize(6);
            secondWords.resize(6);
            const auto parsed =
                makeContainer({{0, "IU", {firstWords[0], firstWords[1], firstWords[2], firstWords[3], firstWords[4], firstWords[5]}},
                                  {0, "IU", {secondWords[0], secondWords[1], secondWords[2], secondWords[3], secondWords[4], secondWords[5]}}},
                    FormatEpoch::UncompressedLegacy, byteOrder);
            auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
            profile.byteOrder = byteOrder;
            auto imported = importStaffUsed(parsed, profile);
            const auto values = imported.document->getOthers()->getArray<StaffUsed>(0, 0);
            REQUIRE(values.size() == 2);
            CHECK(values[0]->distFromTop == 0);
            CHECK(values[1]->distFromTop == -300);
            checkWholeDocumentRange(*values[1]);
        }
        {
            std::vector<SyntheticRow> rows;
            appendStaffUsedRows(rows, 0, staffUsedWords(1, -80, 2, 128, 7, 512, byteOrder));
            appendStaffUsedRows(rows, 0, staffUsedWords(2, -380, 3, 256, 8, 1024, byteOrder));
            auto profile = SourceProfile(FormatEpoch::DclLegacy);
            profile.byteOrder = byteOrder;
            auto imported = importStaffUsed(makeContainer(rows, FormatEpoch::DclLegacy, byteOrder), profile);
            const auto values = imported.document->getOthers()->getArray<StaffUsed>(0, 0);
            REQUIRE(values.size() == 2);
            CHECK(values[0]->distFromTop == -80);
            CHECK(values[1]->distFromTop == -380);
            REQUIRE(values[1]->range);
            CHECK(values[1]->range->startMeas == 3);
            CHECK(values[1]->range->startEdu == 256);
            CHECK(values[1]->range->endMeas == 8);
            CHECK(values[1]->range->endEdu == 1024);
        }
        {
            auto words = staffUsedWords(1, -80, 2, 128, 7, 512, byteOrder);
            const auto second = staffUsedWords(2, -380, 3, 256, 8, 1024, byteOrder);
            words.insert(words.end(), second.begin(), second.end());
            auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
            profile.byteOrder = byteOrder;
            auto imported = importStaffUsed(makeClassContainer(0x009f, words, byteOrder, 0), profile);
            const auto values = imported.document->getOthers()->getArray<StaffUsed>(0, 0);
            REQUIRE(values.size() == 2);
            CHECK(values[0]->distFromTop == -80);
            CHECK(values[1]->distFromTop == -380);
            CHECK(values[1]->range->endEdu == 1024);
            const auto* source = imported.report.findField<StaffUsed>("range.endEdu", 0, 0, musx::dom::Inci(1));
            REQUIRE(source);
            CHECK(source->origin == ValueOrigin::LegacyMus);
            CHECK(source->rawValue == 1024);
        }
    }
}

TEST_CASE("StaffUsed two-entry IU layout ends at Finale 3.5", "[class][staff-used]")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        const auto earlyParsed = makeContainer({{0, "IU", {1, 1, -188, 2, 1, -436}}}, FormatEpoch::UncompressedLegacy, byteOrder);
        auto finale32Profile = SourceProfile(FormatEpoch::UncompressedLegacy);
        finale32Profile.version = SourceVersion{.major = 3, .minor = 2};
        finale32Profile.byteOrder = byteOrder;
        const auto finale32 = importStaffUsed(earlyParsed, finale32Profile);
        const auto earlyValues = finale32.document->getOthers()->getArray<StaffUsed>(0, 0);
        REQUIRE(earlyValues.size() == 2);
        CHECK(earlyValues[0]->staffId == 1);
        CHECK(earlyValues[0]->distFromTop == 0);
        CHECK(earlyValues[1]->staffId == 2);
        CHECK(earlyValues[1]->distFromTop == -248);

        auto firstWords = staffUsedWords(1, -188, 1, 0, 32767, (std::numeric_limits<std::int32_t>::max)(), byteOrder);
        auto secondWords = staffUsedWords(2, -436, 1, 0, 32767, (std::numeric_limits<std::int32_t>::max)(), byteOrder);
        firstWords.resize(6);
        secondWords.resize(6);
        const auto laterParsed =
            makeContainer({{0, "IU", {firstWords[0], firstWords[1], firstWords[2], firstWords[3], firstWords[4], firstWords[5]}},
                              {0, "IU", {secondWords[0], secondWords[1], secondWords[2], secondWords[3], secondWords[4], secondWords[5]}}},
                FormatEpoch::UncompressedLegacy, byteOrder);
        auto finale35Profile = SourceProfile(FormatEpoch::UncompressedLegacy);
        finale35Profile.version = SourceVersion{.major = 3, .minor = 5};
        finale35Profile.byteOrder = byteOrder;
        const auto finale35 = importStaffUsed(laterParsed, finale35Profile);
        const auto laterValues = finale35.document->getOthers()->getArray<StaffUsed>(0, 0);
        REQUIRE(laterValues.size() == 2);
        CHECK(laterValues[0]->staffId == 1);
        CHECK(laterValues[0]->distFromTop == 0);
        CHECK(laterValues[1]->staffId == 2);
        CHECK(laterValues[1]->distFromTop == -248);
    }
}

TEST_CASE("Coda StaffUsed remaps the four early Staff Set comparators", "[class][staff-used]")
{
    const auto parsed =
        makeContainer({{65530, "IU", {1, 0, -80, 0, 0, 0}}, {65533, "IU", {2, 0, -160, 0, 0, 0}}}, FormatEpoch::CodaBanner, ByteOrder::BigEndian);
    auto coda = importStaffUsed(parsed, SourceProfile(FormatEpoch::CodaBanner));
    CHECK(coda.document->getOthers()->getArray<StaffUsed>(0, musx::dom::STAFF_SET_1_SYSTEM_ID).size() == 1);
    CHECK(coda.document->getOthers()->getArray<StaffUsed>(0, musx::dom::Cmper(musx::dom::STAFF_SET_1_SYSTEM_ID + 3)).size() == 1);
    CHECK(coda.document->getOthers()->getArray<StaffUsed>(0, 65530).empty());
    CHECK(coda.document->getOthers()->getArray<StaffUsed>(0, 65533).empty());

    const auto laterParsed = makeContainer({{65530, "IU", {1, 0, 0, 0, -1, -80}}}, FormatEpoch::UncompressedLegacy, ByteOrder::BigEndian);
    auto later = importStaffUsed(laterParsed, SourceProfile(FormatEpoch::UncompressedLegacy));
    CHECK(later.document->getOthers()->getArray<StaffUsed>(0, 65530).size() == 1);
}

TEST_CASE("StaffUsed completes systems from the extraction list and preserves authored lists", "[class][staff-used]")
{
    std::vector<SyntheticRow> rows{{GLOBALS_CMPER, "23", {0, 0, 0, 0, 17, 0}}};
    appendStaffUsedRows(rows, 0, {1, 0, 0, 0, -1, -80}, "IU");
    appendStaffUsedRows(rows, 0, {2, 0, 0, 0, -1, -320}, "IU");
    appendStaffUsedRows(rows, 17, {2, 0, 0, 0, -1, -200}, "IU");
    appendStaffUsedRows(rows, 2, {3, 0, 0, 0, -1, -300}, "IU");
    rows.insert(rows.end(), {{1, "SS", {-10, 0, 0, -200, 1, 0}}, {1, "SS", {2, 0, 0, 100, 0, 0}}, {2, "SS", {-20, 0, 0, -200, 2, 0x4000}},
                                {2, "SS", {3, 0, 0, 100, 0, 0}}});
    auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    auto imported = importStaffUsed(makeContainer(rows), profile, true);

    REQUIRE(imported.document->getOthers()->get<StaffSystem>(0, 1));
    REQUIRE(imported.document->getOthers()->get<StaffSystem>(0, 2));
    const auto firstList = imported.document->getOthers()->getArray<StaffUsed>(0, 1);
    REQUIRE(firstList.size() == 1);
    CHECK(firstList[0]->staffId == 2);
    CHECK(firstList[0]->distFromTop == 0);
    const auto secondList = imported.document->getOthers()->getArray<StaffUsed>(0, 2);
    REQUIRE(secondList.size() == 1);
    CHECK(secondList[0]->staffId == 3);
    CHECK(secondList[0]->distFromTop == 0);

    const auto firstSystem = imported.document->getOthers()->get<StaffSystem>(0, 1);
    const auto secondSystem = imported.document->getOthers()->get<StaffSystem>(0, 2);
    REQUIRE(firstSystem);
    REQUIRE(secondSystem);
    CHECK(firstSystem->top == -210);
    CHECK(secondSystem->top == -300);
    CHECK(firstSystem->distanceToPrev == 0);
    CHECK(secondSystem->distanceToPrev == -20);
    const auto* synthesized = imported.report.findField<StaffUsed>("staffId", 0, 1, musx::dom::Inci(0));
    REQUIRE(synthesized);
    CHECK(synthesized->origin == ValueOrigin::LegacyBehavior);
    const auto* authored = imported.report.findField<StaffUsed>("staffId", 0, 2, musx::dom::Inci(0));
    REQUIRE(authored);
    CHECK(authored->origin == ValueOrigin::LegacyMus);
    const auto* top = imported.report.findField<StaffSystem>("top", 0, 1);
    REQUIRE(top);
    CHECK(top->origin == ValueOrigin::LegacyMusAdjusted);
    CHECK(top->rawValue == -200);
    REQUIRE(top->finaleUpgradeLossValue);
    CHECK(*top->finaleUpgradeLossValue == -90);
}

TEST_CASE("Unoptimized StaffUsed systems ignore stale system lists", "[class][staff-used]")
{
    std::vector<SyntheticRow> rows;
    appendStaffUsedRows(rows, 0, staffUsedWords(1, -188));
    appendStaffUsedRows(rows, 0, staffUsedWords(2, -476));
    appendStaffUsedRows(rows, 0, staffUsedWords(3, -764));
    appendStaffUsedRows(rows, 2, staffUsedWords(1, -188));
    appendStaffUsedRows(rows, 2, staffUsedWords(2, -437));
    rows.insert(rows.end(), {{1, "SS", {-100, 48, 0, -211, 1, 1}}, {1, "SS", {70, 0, 10000, 100, 0, 0}}, {2, "SS", {-149, 48, 0, -211, 70, 1}},
                                {2, "SS", {75, 0, 11519, 100, 0, 0}}});

    auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
    profile.version = SourceVersion{.major = finale_mus_reader::versions::finale2000.major};
    profile.byteOrder = ByteOrder::BigEndian;
    auto imported = importStaffUsed(makeContainer(rows), profile, true);

    const auto values = imported.document->getOthers()->getArray<StaffUsed>(0, 2);
    REQUIRE(values.size() == 3);
    CHECK(values[0]->staffId == 1);
    CHECK(values[0]->distFromTop == 0);
    CHECK(values[1]->staffId == 2);
    CHECK(values[1]->distFromTop == -288);
    CHECK(values[2]->staffId == 3);
    CHECK(values[2]->distFromTop == -576);
    const auto system = imported.document->getOthers()->get<StaffSystem>(0, 2);
    REQUIRE(system);
    CHECK(system->top == -188);
    const auto* synthesized = imported.report.findField<StaffUsed>("staffId", 0, 2, musx::dom::Inci(2));
    REQUIRE(synthesized);
    CHECK(synthesized->origin == ValueOrigin::LegacyBehavior);
}

TEST_CASE("DCL StaffUsed preserves the authored list origin", "[class][staff-used]")
{
    std::vector<SyntheticRow> rows;
    appendStaffUsedRows(rows, 1, staffUsedWords(8, -98));
    appendStaffUsedRows(rows, 1, staffUsedWords(2, 0));
    appendStaffUsedRows(rows, 1, staffUsedWords(4, -206));
    appendStaffUsedRows(rows, 1, staffUsedWords(5, -530));
    rows.insert(rows.end(), {{1, "SS", {-188, 48, 0, -134, 1, 0x4000}}, {1, "SS", {76, 0, 10344, 100, 188, 1536}}, {1, "SS", {0, 0, 0, 0, 0, 0}}});

    auto profile = SourceProfile(FormatEpoch::DclLegacy);
    profile.version = SourceVersion{.major = finale_mus_reader::versions::finale2006.major};
    profile.byteOrder = ByteOrder::BigEndian;
    auto imported = importStaffUsed(makeContainer(rows, FormatEpoch::DclLegacy), profile, true);

    const auto values = imported.document->getOthers()->getArray<StaffUsed>(0, 1);
    REQUIRE(values.size() == 4);
    CHECK(values[0]->distFromTop == -98);
    CHECK(values[1]->distFromTop == 0);
    CHECK(values[2]->distFromTop == -206);
    CHECK(values[3]->distFromTop == -530);
    const auto system = imported.document->getOthers()->get<StaffSystem>(0, 1);
    REQUIRE(system);
    CHECK(system->top == -188);
    const auto* distance = imported.report.findField<StaffUsed>("distFromTop", 0, 1, musx::dom::Inci(0));
    REQUIRE(distance);
    CHECK(distance->origin == ValueOrigin::LegacyMus);
    const auto* top = imported.report.findField<StaffSystem>("top", 0, 1);
    REQUIRE(top);
    CHECK(top->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Compact StaffUsed lists normalize from their highest stored position", "[class][staff-used]")
{
    const auto parsed =
        makeContainer({{1, "IU", {4, 0, -184, 12, 0, -163}}, {1, "SS", {-24, 0, 0, -144, 1, 0x4000}}}, FormatEpoch::CodaBanner, ByteOrder::BigEndian);
    auto profile = SourceProfile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    const auto imported = importStaffUsed(parsed, profile, true);
    const auto values = imported.document->getOthers()->getArray<StaffUsed>(0, 1);
    REQUIRE(values.size() == 2);
    CHECK(values[0]->staffId == 4);
    CHECK(values[0]->distFromTop == -21);
    CHECK(values[1]->staffId == 12);
    CHECK(values[1]->distFromTop == 0);
    const auto system = imported.document->getOthers()->get<StaffSystem>(0, 1);
    REQUIRE(system);
    CHECK(system->top == -187);
}

TEST_CASE("StaffUsed normalization ends in the DCL epoch", "[class][staff-used]")
{
    const auto importAt = [&](const FormatEpoch epoch) {
        std::vector<SyntheticRow> rows;
        for (auto words : {staffUsedWords(1, -80), staffUsedWords(2, -380)}) {
            if (epoch == FormatEpoch::UncompressedLegacy) {
                words.resize(6);
            }
            appendStaffUsedRows(rows, 0, words, epoch == FormatEpoch::UncompressedLegacy ? "IU" : "Iu");
        }
        auto profile = SourceProfile(epoch);
        profile.byteOrder = ByteOrder::BigEndian;
        return importStaffUsed(makeContainer(rows, epoch), profile);
    };

    const auto finale2000 = importAt(FormatEpoch::UncompressedLegacy);
    const auto normalized = finale2000.document->getOthers()->getArray<StaffUsed>(0, 0);
    REQUIRE(normalized.size() == 2);
    CHECK(normalized[0]->distFromTop == 0);
    CHECK(normalized[1]->distFromTop == -300);
    const auto* normalizedDistance = finale2000.report.findField<StaffUsed>("distFromTop", 0, 0, musx::dom::Inci(0));
    REQUIRE(normalizedDistance);
    CHECK(normalizedDistance->origin == ValueOrigin::LegacyMusAdjusted);

    const auto finale2001 = importAt(FormatEpoch::DclLegacy);
    const auto preserved = finale2001.document->getOthers()->getArray<StaffUsed>(0, 0);
    REQUIRE(preserved.size() == 2);
    CHECK(preserved[0]->distFromTop == -80);
    CHECK(preserved[1]->distFromTop == -380);
    const auto* preservedDistance = finale2001.report.findField<StaffUsed>("distFromTop", 0, 0, musx::dom::Inci(0));
    REQUIRE(preservedDistance);
    CHECK(preservedDistance->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("Finale 2011 StaffUsed systems use their system lists without the legacy optimization flag", "[class][staff-used]")
{
    const auto firstBase = staffUsedWords(1, -188, 1, 0, 32767, (std::numeric_limits<std::int32_t>::max)(), ByteOrder::LittleEndian);
    const auto secondBase = staffUsedWords(2, -476, 1, 0, 32767, (std::numeric_limits<std::int32_t>::max)(), ByteOrder::LittleEndian);
    auto baseWords = firstBase;
    baseWords.insert(baseWords.end(), secondBase.begin(), secondBase.end());
    const auto systemWords = staffUsedWords(2, -400, 1, 0, 32767, (std::numeric_limits<std::int32_t>::max)(), ByteOrder::LittleEndian);
    const std::vector<std::int16_t> staffSystemWords{0, 0, 0, -96, 1, 0, 2, 0, 10000, 100, 0, 0};
    const auto parsed = makeClassContainer(
        {SyntheticClassRow{0x009f, baseWords, 0, 0}, SyntheticClassRow{0x009f, systemWords, 1, 0}, SyntheticClassRow{0x00df, staffSystemWords, 1, 0}},
        ByteOrder::LittleEndian);
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.version = SourceVersion{.major = finale_mus_reader::versions::finale2011.major};
    profile.byteOrder = ByteOrder::LittleEndian;
    auto imported = importStaffUsed(parsed, profile, true);

    const auto values = imported.document->getOthers()->getArray<StaffUsed>(0, 1);
    REQUIRE(values.size() == 1);
    CHECK(values.front()->staffId == 2);
    CHECK(values.front()->distFromTop == -400);
    const auto system = imported.document->getOthers()->get<StaffSystem>(0, 1);
    REQUIRE(system);
    CHECK(system->top == 0);
    const auto* authored = imported.report.findField<StaffUsed>("staffId", 0, 1, musx::dom::Inci(0));
    REQUIRE(authored);
    CHECK(authored->origin == ValueOrigin::LegacyMus);
    const auto* distance = imported.report.findField<StaffUsed>("distFromTop", 0, 1, musx::dom::Inci(0));
    REQUIRE(distance);
    CHECK(distance->origin == ValueOrigin::LegacyMus);
}

TEST_CASE("StaffUsed synthesizes score and linked-part systems with their owning share modes", "[class][staff-used]")
{
    const auto baseWords = staffUsedWords(1, -80, 1, 0, 32767, (std::numeric_limits<std::int32_t>::max)(), ByteOrder::LittleEndian);
    const std::vector<std::int16_t> systemWords{0, 0, 0, -96, 1, 1, 2, 0, 10000, 100, 0, 0};
    const auto parsed = makeClassContainer({SyntheticClassRow{0x009f, baseWords, 0, 0}, SyntheticClassRow{0x009f, baseWords, 0, 1},
                                               SyntheticClassRow{0x00df, systemWords, 1, 0}, SyntheticClassRow{0x00df, systemWords, 1, 1}},
        ByteOrder::LittleEndian);
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    auto imported = importStaffUsed(parsed, profile, true);

    const auto score = imported.document->getOthers()->getArray<StaffUsed>(0, 1);
    const auto part = imported.document->getOthers()->getArray<StaffUsed>(1, 1);
    REQUIRE(score.size() == 1);
    REQUIRE(part.size() == 1);
    CHECK(score.front()->getShareMode() == musx::dom::EnigmaBase::ShareMode::All);
    CHECK(part.front()->getShareMode() == musx::dom::EnigmaBase::ShareMode::None);
}

TEST_CASE("StaffUsed ignores system-range lists without a corresponding StaffSystem", "[class][staff-used]")
{
    const auto rows = std::vector<SyntheticRow>{{2, "IU", {1, 0, 0, 0, -1, -80}}, {3, "IU", {2, 0, 0, 0, -1, -160}},
        {65400, "IU", {3, 0, 0, 0, -1, -240}}, {1, "SS", {-10, 0, 0, -200, 1, 0}}, {1, "SS", {2, 0, 0, 100, 0, 0}},
        {2, "SS", {-20, 0, 0, -200, 2, 0x4000}}, {2, "SS", {3, 0, 0, 100, 0, 0}}};
    auto profile = SourceProfile(FormatEpoch::UncompressedLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    auto imported = importStaffUsed(makeContainer(rows), profile, true);
    CHECK(imported.document->getOthers()->getArray<StaffUsed>(0, 2).size() == 1);
    CHECK(imported.document->getOthers()->getArray<StaffUsed>(0, 3).empty());
    CHECK(imported.document->getOthers()->getArray<StaffUsed>(0, musx::dom::STUDIO_VIEW_SYSTEM_ID).size() == 1);
}

TEST_CASE("StaffUsed retains complete elements before a truncated tail", "[class][staff-used]")
{
    auto words = staffUsedWords(1, -80, 1, 0, 32767, (std::numeric_limits<std::int32_t>::max)(), ByteOrder::LittleEndian);
    words.push_back(2);
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    auto imported = importStaffUsed(makeClassContainer(0x009f, words, ByteOrder::LittleEndian, 0), profile);
    CHECK(imported.document->getOthers()->getArray<StaffUsed>(0, 0).size() == 1);
    CHECK(imported.report.diagnostics.size() == 1);
}

TEST_CASE("StaffUsed survey emits the complete persisted field manifest", "[class][staff-used][coverage]")
{
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    auto imported = importStaffUsed(
        makeClassContainer(0x009f, staffUsedWords(1, -80, 1, 0, 32767, (std::numeric_limits<std::int32_t>::max)(), ByteOrder::LittleEndian),
            ByteOrder::LittleEndian, 0),
        profile);
    const auto surveyed = finale_mus_reader::coverage::runAllSurveyors({imported.document, imported.report});
    const auto& values = surveyed.snapshot.at("staff_used").asArray();
    REQUIRE(values.size() == 1);
    const auto& object = values.front().asObject();
    const std::set<std::string> expected{"_report_match_key", "_classifier_inci", "cmper", "part_id", "share_mode", "origin", "staff_id",
        "dist_from_top", "range", "origin_staffId", "origin_distFromTop"};
    std::set<std::string> actual;
    for (const auto& [key, value] : object) {
        static_cast<void>(value);
        actual.insert(key);
    }
    CHECK(actual == expected);
    CHECK(object.at("_report_match_key").asString() == "part=0,cmper=0,staff=1");
    const auto& range = object.at("range").asObject();
    for (const auto* leaf :
        {"start_meas", "start_edu", "end_meas", "end_edu", "origin_startMeas", "origin_startEdu", "origin_endMeas", "origin_endEdu"}) {
        CHECK(range.contains(leaf));
    }
    CHECK(reportedFieldCount(imported.report) == 6);
}

TEST_CASE("StaffUsed comparison recognizes staffs inserted during Finale layout recalculation", "[class][staff-used][coverage]")
{
    using namespace finale_mus_reader::coverage;
    ComparisonLeaves source;
    ComparisonLeaves companion;
    ComparisonLeaves sourceDocument;
    ComparisonLeaves companionDocument;
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const auto classify = differenceClassifier("staff_used");
    REQUIRE(classify);
    const auto addItem = [](ComparisonLeaves& leaves, musx::dom::Cmper cmper, std::int64_t inci, std::int64_t staffId, std::int64_t distance,
                             std::string_view origin) {
        const auto object = "staff_used[cmper=" + std::to_string(cmper) + ",staff=" + std::to_string(staffId) + "]";
        leaves.emplace(object + ".staff_id", std::pair{Value(staffId), std::string(origin)});
        leaves.emplace(object + ".dist_from_top", std::pair{Value(distance), std::string(origin)});
        leaves.emplace(object + "._classifier_inci", std::pair{Value(inci), std::string{}});
    };
    addItem(source, 0, 0, 1, 0, "legacy-mus");
    addItem(source, 0, 1, 2, -252, "legacy-mus");
    addItem(source, 0, 2, 3, -504, "legacy-mus");
    addItem(source, 0, 3, 4, -756, "legacy-mus");
    addItem(source, 1, 0, 1, 0, "legacy-mus");
    addItem(source, 1, 1, 3, -252, "legacy-mus");
    addItem(source, 1, 2, 4, -504, "legacy-mus");
    addItem(companion, 0, 0, 1, 0, "enigma-xml");
    addItem(companion, 0, 1, 2, -252, "enigma-xml");
    addItem(companion, 0, 2, 3, -504, "enigma-xml");
    addItem(companion, 0, 3, 4, -756, "enigma-xml");
    addItem(companion, 1, 0, 1, 0, "enigma-xml");
    addItem(companion, 1, 1, 2, -252, "enigma-xml");
    addItem(companion, 1, 2, 3, -504, "enigma-xml");
    addItem(companion, 1, 3, 4, -756, "enigma-xml");
    sourceDocument.emplace("staff_systems[cmper=1].start_meas", std::pair{Value(1), std::string("legacy-mus")});
    companionDocument.emplace("staff_systems[cmper=1].start_meas", std::pair{Value(1), std::string("enigma-xml")});
    const Value absent;
    const auto classifyPath = [&](std::string_view path, DifferenceCategory category, const Value& sourceValue, const Value& companionValue,
                                  std::string_view origin = "legacy-mus") {
        return classify(DifferenceContext{path, category, origin, sourceValue, companionValue, source, companion, FormatEpoch::UncompressedLegacy,
            ByteOrder::BigEndian, nullptr, report, {}, {}, &sourceDocument, &companionDocument});
    };

    CHECK(classifyPath("staff_used[cmper=1,staff=2].staff_id", DifferenceCategory::CompanionOnly, absent, Value(2), {})
          == DifferenceClassification::FinaleLayoutRecalculation);
    CHECK(classifyPath("staff_used[cmper=1,staff=3].dist_from_top", DifferenceCategory::Differs, Value(-252), Value(-504))
          == DifferenceClassification::FinaleLayoutRecalculation);
    CHECK_FALSE(classifyPath("staff_used[cmper=1,staff=3].staff_id", DifferenceCategory::Differs, Value(3), Value(4)));

    sourceDocument.clear();
    CHECK(classifyPath("staff_used[cmper=1,staff=2].staff_id", DifferenceCategory::CompanionOnly, absent, Value(2), {})
          == DifferenceClassification::FinaleLayoutRecalculation);
    sourceDocument.emplace("staff_systems[cmper=1].start_meas", std::pair{Value(1), std::string("legacy-mus")});
    companionDocument.clear();
    CHECK_FALSE(classifyPath("staff_used[cmper=1,staff=2].staff_id", DifferenceCategory::CompanionOnly, absent, Value(2), {}));
    companionDocument.emplace("staff_systems[cmper=1].start_meas", std::pair{Value(1), std::string("enigma-xml")});
    companion.at("staff_used[cmper=1,staff=2].staff_id").first = Value(4);
    CHECK_FALSE(classifyPath("staff_used[cmper=1,staff=2].staff_id", DifferenceCategory::CompanionOnly, absent, Value(4), {}));
    companion.at("staff_used[cmper=1,staff=2].staff_id").first = Value(2);
    companion.at("staff_used[cmper=1,staff=4].dist_from_top").first = Value(-700);
    CHECK_FALSE(classifyPath("staff_used[cmper=1,staff=4].dist_from_top", DifferenceCategory::Differs, Value(-504), Value(-700)));
    companion.at("staff_used[cmper=1,staff=4].dist_from_top").first = Value(-756);
    source.at("staff_used[cmper=1,staff=3].staff_id").second = "legacy-behavior";
    CHECK_FALSE(classifyPath("staff_used[cmper=1,staff=2].staff_id", DifferenceCategory::CompanionOnly, absent, Value(2), {}));
    source.at("staff_used[cmper=1,staff=3].staff_id").second = "legacy-mus";
    companion.at("staff_used[cmper=1,staff=3]._classifier_inci").first = Value(3);
    companion.at("staff_used[cmper=1,staff=4]._classifier_inci").first = Value(2);
    CHECK_FALSE(classifyPath("staff_used[cmper=1,staff=2].staff_id", DifferenceCategory::CompanionOnly, absent, Value(2), {}));
}

TEST_CASE("StaffUsed comparison classifies whole-list Finale layout transformations", "[class][staff-used][coverage]")
{
    using namespace finale_mus_reader::coverage;
    CHECK(staff_fields::partIdFromComparisonPath("staff_used[semantic=part=7,cmper=41,staff=1].staff_id") == musx::dom::Cmper(7));
    ComparisonLeaves source;
    ComparisonLeaves companion;
    ComparisonLeaves sourceDocument;
    ComparisonLeaves companionDocument;
    ImportReport report(FormatEpoch::CodaBanner);
    const auto classify = differenceClassifier("staff_used");
    REQUIRE(classify);
    const Value absent;
    const auto classifyPath = [&](std::string_view path, DifferenceCategory category, FormatEpoch epoch = FormatEpoch::CodaBanner) {
        return classify(DifferenceContext{path, category, category == DifferenceCategory::ReaderOnly ? "legacy-mus" : "", Value(1), Value(1), source,
            companion, epoch, ByteOrder::BigEndian, nullptr, report, {}, {}, &sourceDocument, &companionDocument});
    };

    CHECK(classifyPath("staff_used[cmper=65400,staff=32767].staff_id", DifferenceCategory::CompanionOnly)
          == DifferenceClassification::FinaleUpgradeSynthesis);
    CHECK_FALSE(classifyPath("staff_used[cmper=65400,staff=32767].staff_id", DifferenceCategory::CompanionOnly, FormatEpoch::ZlibLegacy));

    companionDocument.emplace("staff_systems[cmper=3].start_meas", std::pair{Value(1), std::string{}});
    CHECK(classifyPath("staff_used[cmper=3,staff=1].staff_id", DifferenceCategory::CompanionOnly)
          == DifferenceClassification::FinaleLayoutRecalculation);
    sourceDocument.emplace("staff_systems[cmper=3].start_meas", std::pair{Value(1), std::string("legacy-mus")});
    CHECK_FALSE(classifyPath("staff_used[cmper=3,staff=1].staff_id", DifferenceCategory::CompanionOnly));

    CHECK(
        classifyPath("staff_used[cmper=41,staff=1].staff_id", DifferenceCategory::ReaderOnly) == DifferenceClassification::FinaleLayoutRecalculation);
    CHECK(classifyPath("staff_used[cmper=41,staff=1].staff_id", DifferenceCategory::CompanionOnly)
          == DifferenceClassification::FinaleLayoutRecalculation);
    CHECK_FALSE(classifyPath("staff_used[cmper=65531,staff=2].staff_id", DifferenceCategory::ReaderOnly));
    CHECK_FALSE(classifyPath("staff_used[cmper=65501,staff=2].staff_id", DifferenceCategory::CompanionOnly));
}

TEST_CASE("StaffUsed comparison recognizes Special Part Extraction layout reduction", "[class][staff-used][coverage]")
{
    using namespace finale_mus_reader::coverage;
    ComparisonLeaves source;
    ComparisonLeaves companion;
    ComparisonLeaves sourceDocument;
    ComparisonLeaves companionDocument;
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const auto classify = differenceClassifier("staff_used");
    REQUIRE(classify);
    const auto addItem = [](ComparisonLeaves& leaves, musx::dom::Cmper cmper, std::int64_t inci, std::int64_t staffId, std::int64_t distance,
                             std::string_view origin) {
        const auto object = "staff_used[cmper=" + std::to_string(cmper) + ",staff=" + std::to_string(staffId) + "]";
        leaves.emplace(object + ".staff_id", std::pair{Value(staffId), std::string(origin)});
        leaves.emplace(object + ".dist_from_top", std::pair{Value(distance), std::string(origin)});
        leaves.emplace(object + "._classifier_inci", std::pair{Value(inci), std::string{}});
    };
    addItem(source, 17, 0, 2, 0, "legacy-mus");
    addItem(source, 1, 0, 1, 0, "legacy-mus");
    addItem(source, 1, 1, 2, -224, "legacy-mus");
    addItem(source, 1, 2, 3, -448, "legacy-mus");
    addItem(companion, 17, 0, 2, 0, "enigma-xml");
    addItem(companion, 1, 0, 2, 0, "enigma-xml");
    sourceDocument.emplace("part_globals[cmper=65534].special_part_extraction_i_u_list", std::pair{Value(17), std::string("legacy-mus")});
    sourceDocument.emplace("staff_systems[cmper=1].start_meas", std::pair{Value(1), std::string("legacy-mus")});
    companionDocument.emplace("staff_systems[cmper=1].start_meas", std::pair{Value(1), std::string("enigma-xml")});
    const Value absent;
    const auto classifyPath = [&](std::string_view path, DifferenceCategory category, const Value& sourceValue, const Value& companionValue) {
        return classify(DifferenceContext{path, category, "legacy-mus", sourceValue, companionValue, source, companion,
            FormatEpoch::UncompressedLegacy, ByteOrder::BigEndian, nullptr, report, {}, {}, &sourceDocument, &companionDocument});
    };

    CHECK(classifyPath("staff_used[cmper=1,staff=1].staff_id", DifferenceCategory::ReaderOnly, Value(1), absent)
          == DifferenceClassification::FinaleLayoutRecalculation);
    CHECK(classifyPath("staff_used[cmper=1,staff=1].range.start_meas", DifferenceCategory::ReaderOnly, Value(1), absent)
          == DifferenceClassification::FinaleLayoutRecalculation);
    CHECK(classifyPath("staff_used[cmper=1,staff=2].dist_from_top", DifferenceCategory::Differs, Value(-224), Value(0))
          == DifferenceClassification::FinaleLayoutRecalculation);

    sourceDocument.erase("part_globals[cmper=65534].special_part_extraction_i_u_list");
    CHECK_FALSE(classifyPath("staff_used[cmper=1,staff=1].staff_id", DifferenceCategory::ReaderOnly, Value(1), absent));
}

TEST_CASE("StaffUsed comparison recognizes uniform companion respacing", "[class][staff-used][coverage]")
{
    using namespace finale_mus_reader::coverage;
    ComparisonLeaves source;
    ComparisonLeaves companion;
    ComparisonLeaves sourceDocument;
    ComparisonLeaves companionDocument;
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const auto classify = differenceClassifier("staff_used");
    REQUIRE(classify);
    const auto addItem = [](ComparisonLeaves& leaves, std::int64_t inci, std::int64_t staffId, std::int64_t distance, std::string_view origin) {
        const auto object = "staff_used[cmper=10,staff=" + std::to_string(staffId) + "]";
        leaves.emplace(object + ".staff_id", std::pair{Value(staffId), std::string(origin)});
        leaves.emplace(object + ".dist_from_top", std::pair{Value(distance), std::string(origin)});
        leaves.emplace(object + "._classifier_inci", std::pair{Value(inci), std::string{}});
    };
    for (std::int64_t i = 0; i < 4; ++i) {
        addItem(source, i, i + 1, i * -228, "legacy-mus");
        addItem(companion, i, i + 1, i * -240, "enigma-xml");
    }
    sourceDocument.emplace("staff_systems[cmper=10].start_meas", std::pair{Value(1), std::string("legacy-mus")});
    companionDocument.emplace("staff_systems[cmper=10].start_meas", std::pair{Value(1), std::string("enigma-xml")});
    const auto classifyDistance = [&](std::string_view corpusId = {}) {
        const auto& sourceValue = source.at("staff_used[cmper=10,staff=2].dist_from_top").first;
        const auto& companionValue = companion.at("staff_used[cmper=10,staff=2].dist_from_top").first;
        return classify(DifferenceContext{"staff_used[cmper=10,staff=2].dist_from_top", DifferenceCategory::Differs, "legacy-mus-adjusted",
            sourceValue, companionValue, source, companion, FormatEpoch::UncompressedLegacy, ByteOrder::BigEndian, nullptr, report, {}, {},
            &sourceDocument, &companionDocument, nullptr, corpusId});
    };

    CHECK(classifyDistance() == DifferenceClassification::FinaleLayoutRecalculation);
    companion.at("staff_used[cmper=10,staff=4].dist_from_top").first = Value(-710);
    CHECK_FALSE(classifyDistance());
    companion.at("staff_used[cmper=10,staff=4].dist_from_top").first = Value(-720);
    source.at("staff_used[cmper=10,staff=4].staff_id").first = Value(9);
    CHECK_FALSE(classifyDistance());
    source.at("staff_used[cmper=10,staff=4].staff_id").first = Value(4);
    for (std::int64_t i = 1; i < 4; ++i) {
        source.at("staff_used[cmper=10,staff=" + std::to_string(i + 1) + "].dist_from_top").first = Value(i * -240);
    }
    CHECK_FALSE(classifyDistance());
    source.at("staff_used[cmper=10,staff=2].dist_from_top").first = Value(-239);
    CHECK(classifyDistance("mus-13e307b184ece4d2") == DifferenceClassification::FinaleLayoutRecalculation);
}

TEST_CASE("StaffUsed recovers the controlled source layouts", "[class][staff-used][fixture]")
{
    for (const auto* relative : {"evidence/F97/F97-1stsys-top.mus", "evidence/F2000/F2000-update-layout.mus", "evidence/F2006/F2006-empty.mus",
             "evidence/F2012/F2012-baseline.mus"}) {
        const auto imported = readFixture(relative);
        const auto scrollView = imported.document->getOthers()->getArray<StaffUsed>(0, musx::dom::BASE_SYSTEM_ID);
        REQUIRE_FALSE(scrollView.empty());
        CHECK(scrollView.front()->distFromTop == 0);
        checkWholeDocumentRange(*scrollView.front());
        for (const auto& system : imported.document->getOthers()->getArray<StaffSystem>(0)) {
            const auto list = imported.document->getOthers()->getArray<StaffUsed>(0, system->getCmper());
            REQUIRE_FALSE(list.empty());
            CHECK(list.front()->distFromTop == 0);
        }
    }

    const auto codaStaffSet = readFixture("evidence/F100/F100-quartet-oboeview.mus");
    const auto oboeOnly = codaStaffSet.document->getOthers()->getArray<StaffUsed>(0, musx::dom::Cmper(musx::dom::STAFF_SET_1_SYSTEM_ID + 1));
    REQUIRE(oboeOnly.size() == 1);
    CHECK(oboeOnly.front()->staffId == 2);
    CHECK(codaStaffSet.document->getOthers()->getArray<StaffUsed>(0, 65531).empty());
}

} // namespace
} // namespace finale_mus_reader_tests
