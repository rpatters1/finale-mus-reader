// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"
#include "coverage/comparison.h"
#include "coverage/registry.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

using PercussionNoteCodeTestTarget = musx::dom::details::PercussionNoteCode;

musx::dom::DocumentPtr makePercussionNoteCodeDocument() {
    auto session = musx::factory::DocumentFactory::begin();
    return session.getDocument();
}

ImportReport importPercussionNoteCodes(const finale_mus_reader::container::ParsedContainer &parsed,
                                       const SourceProfile &profile,
                                       const musx::dom::DocumentPtr &document) {
    ImportReport report(profile.epoch);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{LegacyRecordIndex::build(parsed),
                                                   profile,
                                                   noSource,
                                                   document,
                                                   reference,
                                                   report,
                                                   pending,
                                                   construction};
    finale_mus_reader::details::importPercussionNoteCodes(context);
    return report;
}

TEST_CASE("Finale 2010 percussion-note codes use five-word elements", "[class]") {
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
        profile.byteOrder = byteOrder;
        const auto document = makePercussionNoteCodeDocument();
        const auto report = importPercussionNoteCodes(
            makeDetailClassContainer(0x1234, 0x5678, 0, {1, 14, 0, 0, 0, 3, 16, 0, 0, 0}, byteOrder,
                                     0x0451),
            profile, document);

        constexpr musx::dom::EntryNumber entryNumber = 0x12345678;
        const auto first = document->getDetails()->get<PercussionNoteCodeTestTarget>(
            musx::dom::SCORE_PARTID, entryNumber, 0);
        const auto second = document->getDetails()->get<PercussionNoteCodeTestTarget>(
            musx::dom::SCORE_PARTID, entryNumber, 1);
        REQUIRE(first);
        CHECK(first->noteId == 1);
        CHECK(first->noteCode == 14);
        REQUIRE(second);
        CHECK(second->noteId == 3);
        CHECK(second->noteCode == 16);
        CHECK(reportedFieldCount(report) == 4);
        const auto firstKey = finale_mus_reader::instanceKey<PercussionNoteCodeTestTarget>(
            musx::dom::SCORE_PARTID, 0x1234, musx::dom::Inci(0), 0x5678);
        const auto secondKey = finale_mus_reader::instanceKey<PercussionNoteCodeTestTarget>(
            musx::dom::SCORE_PARTID, 0x1234, musx::dom::Inci(1), 0x5678);
        REQUIRE(report.findField(firstKey, "noteId"));
        REQUIRE(report.findField(secondKey, "noteCode"));
        CHECK(report.findField(firstKey, "noteId")->rawValue == 1);
        CHECK(report.findField(secondKey, "noteCode")->rawValue == 16);
    }
}

TEST_CASE("Percussion-note code rejects incomplete and diagnoses nonzero tails", "[class]") {
    auto profile = SourceProfile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;

    const auto incompleteDocument = makePercussionNoteCodeDocument();
    const auto incompleteReport = importPercussionNoteCodes(
        makeDetailClassContainer(0, 42, 0, {1, 14, 0}, ByteOrder::LittleEndian, 0x0451), profile,
        incompleteDocument);
    CHECK(incompleteDocument->getDetails()->getAllSources<PercussionNoteCodeTestTarget>().empty());
    CHECK(incompleteReport.diagnostics.size() == 1);

    const auto tailDocument = makePercussionNoteCodeDocument();
    const auto tailReport = importPercussionNoteCodes(
        makeDetailClassContainer(0, 43, 0, {1, 14, 0, 7, 0}, ByteOrder::LittleEndian, 0x0451),
        profile, tailDocument);
    CHECK(tailDocument->getDetails()->get<PercussionNoteCodeTestTarget>(musx::dom::SCORE_PARTID, 43,
                                                                        0));
    CHECK(tailReport.diagnostics.size() == 1);
}

TEST_CASE("Companion-only percussion-note codes await entry recovery", "[coverage][percussion]") {
    using namespace finale_mus_reader::coverage;
    const Value missing;
    const Value noteCode(7);
    const ComparisonLeaves leaves;
    const ImportReport report(FormatEpoch::UncompressedLegacy);
    const DifferenceContext context{
        "percussion_note_code[entry_number=8130,note_id=1].note_code",
        DifferenceCategory::CompanionOnly,
        {},
        missing,
        noteCode,
        leaves,
        leaves,
        FormatEpoch::UncompressedLegacy,
        ByteOrder::BigEndian,
        nullptr,
        report,
    };

    const auto classifier = differenceClassifier("percussion_note_code");
    REQUIRE(classifier);
    setDeferredRecoveryClassified(true);
    CHECK(classifier(context) == DifferenceClassification::AwaitsDependentRecovery);
    setDeferredRecoveryClassified(false);
    CHECK(classifier(context) == DifferenceClassification::Unexpected);
    setDeferredRecoveryClassified(true);
}

TEST_CASE("Percussion-note codes compare entries and notes by their keys",
          "[coverage][percussion]") {
    using namespace finale_mus_reader::coverage;
    const auto noteCode = [](std::int64_t entryNumber, std::int64_t noteId, std::int64_t code) {
        return Value::Object{
            {"entry_number", entryNumber}, {"note_code", code}, {"note_id", noteId}};
    };
    SurveySnapshot source{
        {"percussion_note_code", Value::Array{noteCode(10, 1, 7), noteCode(11, 1, 8),
                                              noteCode(12, 1, 9), noteCode(12, 2, 10)}}};
    SurveySnapshot companion{
        {"percussion_note_code",
         Value::Array{noteCode(10, 1, 7), noteCode(12, 2, 10), noteCode(12, 1, 9)}}};
    const auto sourceDocument = makePercussionNoteCodeDocument();
    const auto companionDocument = makePercussionNoteCodeDocument();
    const ImportReport report(FormatEpoch::ZlibLegacy);

    const auto comparison =
        compareSnapshots(std::move(source), std::move(companion), sourceDocument, companionDocument,
                         FormatEpoch::ZlibLegacy, ByteOrder::LittleEndian, nullptr, report);
    const auto &stats = comparison.classes.at("details").at("percussion_note_code");
    CHECK(stats.same == 9);
    CHECK(stats.sourceOnly == 3);
    CHECK(stats.unexpected == 0);
}

TEST_CASE("Legacy percussion-note codes are synthesized from decoded entries",
          "[.entry-recovery][class][percussion]") {
    FAIL("Enable this regression with a controlled legacy entry fixture when "
         "entry decoding is implemented.");
}

TEST_CASE("Pre-zlib percussion-note selectors remain uncovered", "[class]") {
    for (const auto epoch :
         {FormatEpoch::CodaBanner, FormatEpoch::DclLegacy, FormatEpoch::UncompressedLegacy}) {
        for (const auto *selector : {"NC", "nC"}) {
            auto profile = SourceProfile(epoch);
            profile.byteOrder = ByteOrder::BigEndian;
            const auto document = makePercussionNoteCodeDocument();
            const auto report = importPercussionNoteCodes(
                makeDetailContainer(epoch, 0, 42, {1, 14, 0, 0, 0}, selector), profile, document);
            CHECK(document->getDetails()->getAllSources<PercussionNoteCodeTestTarget>().empty());
            CHECK(reportedFieldCount(report) == 0);
        }
    }
}

} // namespace
} // namespace finale_mus_reader_tests
