// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

#include <tuple>

#include "coverage/registry.h"
#include "coverage/surveyors/shared/staff_style_semantics.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

using StaffStyleAssign = musx::dom::others::StaffStyleAssign;
using Staff = musx::dom::others::Staff;
using StaffStyle = musx::dom::others::StaffStyle;

struct StaffStyleAssignImportResult
{
    musx::dom::DocumentPtr document;
    ImportReport report;
};

StaffStyleAssignImportResult importStaffStyleAssigns(const finale_mus_reader::container::ParsedContainer& parsed,
    bool addUnreportedAssignment = false, std::optional<SourceVersion> sourceVersion = std::nullopt)
{
    const auto index = LegacyRecordIndex::build(parsed);
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto staff = std::make_shared<Staff>(document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, musx::dom::Cmper{4});
    document->getOthers()->add(Staff::XmlNodeName, std::move(staff));
    auto style = std::make_shared<StaffStyle>(document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, musx::dom::Cmper{7});
    document->getOthers()->add(StaffStyle::XmlNodeName, std::move(style));
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto referenceDocument = referenceSession.getDocument();
    auto referenceStaff =
        std::make_shared<Staff>(referenceDocument, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, musx::dom::Cmper{1});
    referenceStaff->lineSpace = 24;
    referenceStaff->dwRestOffset = -4;
    referenceStaff->wRestOffset = -4;
    referenceStaff->hRestOffset = -4;
    referenceStaff->otherRestOffset = -4;
    referenceStaff->stemReversal = -4;
    referenceStaff->botRepeatDotOff = -5;
    referenceStaff->topRepeatDotOff = -3;
    referenceDocument->getOthers()->add(Staff::XmlNodeName, std::move(referenceStaff));
    auto referenceFontOptions = std::make_shared<musx::dom::options::FontOptions>(referenceDocument);
    auto referenceNoteheadFont = std::make_shared<musx::dom::FontInfo>(referenceDocument);
    referenceNoteheadFont->fontSize = 24;
    referenceFontOptions->fontOptions.emplace(musx::dom::options::FontOptions::FontType::Noteheads, std::move(referenceNoteheadFont));
    referenceDocument->getOptions()->add(musx::dom::options::FontOptions::XmlNodeName, std::move(referenceFontOptions));
    const auto reference = std::move(referenceSession).finish();
    ImportReport report(parsed.formatEpoch);
    finale_mus_reader::PendingReferences pending;
    SourceProfile profile(parsed.formatEpoch);
    profile.byteOrder = parsed.byteOrder;
    profile.version = sourceVersion;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importStaffStyleAssignments(context);
    finale_mus_reader::details::importGFrameHolds(context);
    if (addUnreportedAssignment) {
        auto assignment = std::make_shared<StaffStyleAssign>(
            document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, musx::dom::Cmper{4}, musx::dom::Inci{0});
        document->getOthers()->add(StaffStyleAssign::XmlNodeName, std::move(assignment));
    }
    finale_mus_reader::runDeferredChecks(pending);
    return {document, std::move(report)};
}

void checkStaffStyleAssign(const StaffStyleAssignImportResult& result)
{
    const auto assign = result.document->getOthers()->get<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{4}, musx::dom::Inci{0});
    REQUIRE(assign);
    CHECK(assign->styleId == 7);
    CHECK(assign->startMeas == 3);
    CHECK(assign->startEdu == 0x00010203);
    CHECK(assign->endMeas == 8);
    CHECK(assign->endEdu == (std::numeric_limits<musx::dom::Edu>::max)());
    const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, musx::dom::Cmper{4});
    REQUIRE(staff);
    CHECK(staff->hasStyles);
    CHECK(reportedFieldCount(result.report) == StaffStyleAssign::xmlMappingArray().size() + 1);
    for (const auto* member : {"styleId", "startMeas", "startEdu", "endMeas", "endEdu"}) {
        const auto* field = result.report.findField<StaffStyleAssign>(member, musx::dom::SCORE_PARTID, musx::dom::Cmper{4}, musx::dom::Inci{0});
        REQUIRE(field);
        CHECK(field->origin == ValueOrigin::LegacyMus);
    }
    const auto* hasStyles = result.report.findField<Staff>("hasStyles", musx::dom::SCORE_PARTID, musx::dom::Cmper{4});
    REQUIRE(hasStyles);
    CHECK(hasStyles->origin == ValueOrigin::LegacyMusAdjusted);
    CHECK(result.report.staffStyleAssignmentAuditComplete);
    const auto* audit = result.report.findStaffStyleAssignmentAudit(musx::dom::SCORE_PARTID, musx::dom::Cmper{4});
    REQUIRE(audit);
    CHECK(audit->expectedAssignments == 1);
    CHECK(audit->constructedAssignments == 1);
    CHECK_FALSE(audit->malformedSource);
}

TEST_CASE("StaffStyleAssign recovers fixed-row and class-record layouts")
{
    const std::vector<SyntheticRow> bigEndianRows{
        {4, "Sy", {7, 0, 0, 0, 0, 0}},
        {4, "Sy", {3, 1, 0x0203, 8, 0x7fff, -1}},
    };
    checkStaffStyleAssign(importStaffStyleAssigns(makeContainer(bigEndianRows, FormatEpoch::UncompressedLegacy, ByteOrder::BigEndian)));

    const std::vector<SyntheticRow> littleEndianRows{
        {4, "Sy", {7, 0, 0, 0, 0, 0}},
        {4, "Sy", {3, 0x0203, 1, 8, -1, 0x7fff}},
    };
    checkStaffStyleAssign(importStaffStyleAssigns(makeContainer(littleEndianRows, FormatEpoch::DclLegacy, ByteOrder::LittleEndian)));

    const std::vector<std::int16_t> classWords{7, 0, 0, 0, 0, 0, 3, 0x0203, 1, 8, -1, 0x7fff};
    checkStaffStyleAssign(importStaffStyleAssigns(makeClassContainer(0x00e9, classWords, ByteOrder::LittleEndian, 4)));
}

TEST_CASE("StaffStyleAssign rejects incomplete tuples and refreshes absent "
          "assignment state")
{
    const std::vector<SyntheticRow> rows{{4, "Sy", {7, 0, 0, 0, 0, 0}}};
    const auto incomplete = importStaffStyleAssigns(makeContainer(rows, FormatEpoch::UncompressedLegacy));
    CHECK(incomplete.document->getOthers()->getAllSources<StaffStyleAssign>().empty());
    CHECK(incomplete.report.diagnostics.size() == 1);
    const auto staff = incomplete.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, musx::dom::Cmper{4});
    REQUIRE(staff);
    CHECK_FALSE(staff->hasStyles);
    CHECK(incomplete.report.staffStyleAssignmentAuditComplete);
    const auto* incompleteAudit = incomplete.report.findStaffStyleAssignmentAudit(musx::dom::SCORE_PARTID, musx::dom::Cmper{4});
    REQUIRE(incompleteAudit);
    CHECK(incompleteAudit->expectedAssignments == 0);
    CHECK(incompleteAudit->constructedAssignments == 0);
    CHECK(incompleteAudit->malformedSource);

    const auto absent = importStaffStyleAssigns(makeContainer({}, FormatEpoch::CodaBanner));
    const auto absentStaff = absent.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, musx::dom::Cmper{4});
    REQUIRE(absentStaff);
    CHECK_FALSE(absentStaff->hasStyles);
    const auto* hasStyles = absent.report.findField<Staff>("hasStyles", musx::dom::SCORE_PARTID, musx::dom::Cmper{4});
    REQUIRE(hasStyles);
    CHECK(hasStyles->origin == ValueOrigin::LegacyMusAdjusted);
    CHECK(absent.report.staffStyleAssignmentAuditComplete);
    CHECK_FALSE(absent.report.findStaffStyleAssignmentAudit(musx::dom::SCORE_PARTID, musx::dom::Cmper{4}));
}

TEST_CASE("StaffStyleAssign instrumentation rejects an unreported assignment producer")
{
    try {
        static_cast<void>(importStaffStyleAssigns(makeContainer({}, FormatEpoch::DclLegacy), true));
        FAIL("Expected the assignment completeness audit to fail");
    } catch (const std::logic_error& error) {
        CHECK(std::string_view(error.what()) == "A staff-style assignment importer did not report its source structure");
    }
}

TEST_CASE("StaffStyleAssign survey emits its complete field manifest")
{
    const std::vector<SyntheticRow> rows{
        {4, "Sy", {7, 0, 0, 0, 0, 0}},
        {4, "Sy", {3, 1, 0x0203, 8, 0x7fff, -1}},
    };
    auto imported = importStaffStyleAssigns(makeContainer(rows, FormatEpoch::UncompressedLegacy));
    const auto surveyed = finale_mus_reader::coverage::runAllSurveyors({imported.document, imported.report});
    const auto& assignments = surveyed.snapshot.at("staff_style_assigns").asArray();
    REQUIRE(assignments.size() == 1);
    const auto& object = assignments.front().asObject();
    for (const auto* leaf : {"style_id", "start_meas", "start_edu", "end_meas", "end_edu", "origin_styleId", "origin_startMeas", "origin_startEdu",
             "origin_endMeas", "origin_endEdu"}) {
        CHECK(object.contains(leaf));
    }
    CHECK(object.contains("_staff_style_patch"));
    CHECK(StaffStyleAssign::xmlMappingArray().size() == 5);
}

TEST_CASE("StaffStyle assignment semantics exclude inactive style values", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value::Object style{
        {"cmper", Value(7)},
        {"style_name", Value("Five lines")},
        {"staff_lines", Value(5)},
        {"origin_staffLines", Value("legacy-mus")},
        {"hide_stems", Value(true)},
        {"masks", Value::Object{{"staff_type", Value(true)}, {"origin_staffType", Value("legacy-mus")}, {"show_stems", Value(false)}}},
    };

    const auto patch = staff_style_semantics::canonicalPatch(style);
    CHECK(patch.at("_classifier_masks.staff_type") == Value(true));
    CHECK(patch.at("staff_lines") == Value(5));
    CHECK(patch.at("staff_lines_origin") == Value("legacy-mus"));
    CHECK_FALSE(patch.contains("style_name"));
    CHECK_FALSE(patch.contains("hide_stems"));
    CHECK_FALSE(patch.contains("_classifier_masks.show_stems"));
}

TEST_CASE("StaffStyleAssign comparison composes exactly overlapping assignments", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto assignment = [](std::int64_t inci, std::int64_t startMeasure, Value::Object patch) {
        return Value::Object{{"part_id", Value(0)}, {"share_mode", Value(0)}, {"cmper", Value(1)}, {"inci", Value(inci)},
            {"style_id", Value(10 + inci)}, {"start_meas", Value(startMeasure)}, {"start_edu", Value(0)}, {"end_meas", Value(9)},
            {"end_edu", Value(0x7fffffff)}, {"_staff_style_patch", Value(std::move(patch))}};
    };

    SurveySnapshot source{
        {"font_definitions", Value::Object{{"definitions", Value::Array{Value::Object{{"cmper", Value(1)}, {"normalized_name", Value("Maestro")}}}}}},
        {"staff_style_assigns", Value::Array{assignment(2, 5, {{"_classifier_masks.staff_type", Value(true)}, {"staff_lines", Value(1)}}),
                                    assignment(5, 5, {{"_classifier_masks.show_stems", Value(true)}, {"hide_stems", Value(true)}}),
                                    assignment(7, 5,
                                        {{"_classifier_masks.staff_type", Value(true)}, {"staff_lines", Value(5)},
                                            {"_classifier_masks.float_notehead_font", Value(true)}, {"note_font.font_id", Value(1)}})}}};
    SurveySnapshot companion{
        {"font_definitions", Value::Object{{"definitions", Value::Array{Value::Object{{"cmper", Value(2)}, {"normalized_name", Value("Maestro")}}}}}},
        {"staff_style_assigns",
            Value::Array{assignment(8, 5,
                {{"_classifier_masks.staff_type", Value(true)}, {"staff_lines", Value(5)}, {"_classifier_masks.show_stems", Value(true)},
                    {"hide_stems", Value(true)}, {"_classifier_masks.float_notehead_font", Value(true)}, {"note_font.font_id", Value(2)}})}}};

    auto sourceSession = musx::factory::DocumentFactory::begin();
    auto companionSession = musx::factory::DocumentFactory::begin();
    ImportReport report(FormatEpoch::ZlibLegacy);
    const auto comparison = compareSnapshots(source, companion, std::move(sourceSession).finish(), std::move(companionSession).finish(),
        FormatEpoch::ZlibLegacy, ByteOrder::LittleEndian, nullptr, report);
    CHECK(comparison.classes.at("others").at("staff_style_assigns").unexpected == 0);
    std::map<ComparisonTransformation, std::uint64_t> transformations;
    ComparisonPreparationContext context{source, companion, transformations, FormatEpoch::ZlibLegacy};
    runComparisonPreparers(context);

    const auto& sourceGroups = source.at("staff_style_assigns").asArray();
    const auto& companionGroups = companion.at("staff_style_assigns").asArray();
    REQUIRE(sourceGroups.size() == 1);
    REQUIRE(companionGroups.size() == 1);
    const auto* sourceKey = sourceGroups.front().find("_report_match_key");
    const auto* companionKey = companionGroups.front().find("_report_match_key");
    REQUIRE(sourceKey);
    REQUIRE(companionKey);
    CHECK(*sourceKey == *companionKey);
    const auto* applied = sourceGroups.front().find("applied_style");
    const auto* companionApplied = companionGroups.front().find("applied_style");
    REQUIRE(applied);
    REQUIRE(companionApplied);
    CHECK(applied->asObject().at("staff_lines") == Value(5));
    CHECK(applied->asObject().at("note_font.font_id") == Value(1));
    CHECK(companionApplied->asObject().at("note_font.font_id") == Value(2));
}

TEST_CASE("StaffStyleAssign comparison completes split styles from the base Staff", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto assignment = [](std::int64_t inci, Value::Object patch) {
        return Value::Object{{"part_id", Value(0)}, {"share_mode", Value(0)}, {"cmper", Value(1)}, {"inci", Value(inci)},
            {"style_id", Value(10 + inci)}, {"start_meas", Value(5)}, {"start_edu", Value(0)}, {"end_meas", Value(9)}, {"end_edu", Value(0x7fffffff)},
            {"_staff_style_patch", Value(std::move(patch))}};
    };
    const auto staff =
        Value::Object{{"part_id", Value(0)}, {"cmper", Value(1)}, {"notation_style", Value(2)}, {"origin_notationStyle", Value("legacy-mus")}};
    SurveySnapshot source{{"staff", Value::Array{staff}}, {"staff_style_assigns", Value::Array{assignment(0, {{"hide_stems", Value(true)}})}}};
    SurveySnapshot companion{{"staff", Value::Array{staff}},
        {"staff_style_assigns", Value::Array{assignment(0, {{"hide_stems", Value(true)}}),
                                    assignment(1, {{"_classifier_masks.notation_style", Value(true)}, {"notation_style", Value(2)}})}}};
    std::map<ComparisonTransformation, std::uint64_t> transformations;
    ComparisonPreparationContext context{source, companion, transformations, FormatEpoch::ZlibLegacy};
    runComparisonPreparers(context);

    const auto* sourceAppliedValue = source.at("staff_style_assigns").asArray().front().find("applied_style");
    const auto* companionAppliedValue = companion.at("staff_style_assigns").asArray().front().find("applied_style");
    REQUIRE(sourceAppliedValue);
    REQUIRE(companionAppliedValue);
    const auto& sourceApplied = sourceAppliedValue->asObject();
    const auto& companionApplied = companionAppliedValue->asObject();
    CHECK(sourceApplied.at("notation_style") == Value(2));
    CHECK(companionApplied.at("notation_style") == Value(2));
    CHECK(sourceApplied.at("notation_style_origin") == Value("legacy-mus"));
}

TEST_CASE("StaffStyleAssign comparison ignores inactive tablature fields after composition", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto assignment = [](std::int64_t inci, Value::Object patch) {
        return Value::Object{{"part_id", Value(0)}, {"share_mode", Value(0)}, {"cmper", Value(1)}, {"inci", Value(inci)},
            {"style_id", Value(10 + inci)}, {"start_meas", Value(5)}, {"start_edu", Value(0)}, {"end_meas", Value(9)}, {"end_edu", Value(0x7fffffff)},
            {"_staff_style_patch", Value(std::move(patch))}};
    };
    const auto appliedStyle = [&](StaffStyle::NotationStyle notation) {
        SurveySnapshot source{{"staff_style_assigns", Value::Array{assignment(0, {{"notation_style", Value(static_cast<std::int64_t>(notation))}}),
                                                          assignment(1, {{"vert_tab_num_off", Value(-1024)}})}}};
        SurveySnapshot companion = source;
        std::map<ComparisonTransformation, std::uint64_t> transformations;
        ComparisonPreparationContext context{source, companion, transformations, FormatEpoch::ZlibLegacy};
        runComparisonPreparers(context);
        const auto* applied = source.at("staff_style_assigns").asArray().front().find("applied_style");
        REQUIRE(applied);
        return applied->asObject();
    };

    CHECK_FALSE(appliedStyle(StaffStyle::NotationStyle::Standard).contains("vert_tab_num_off"));
    CHECK_FALSE(appliedStyle(StaffStyle::NotationStyle::Percussion).contains("vert_tab_num_off"));
    CHECK(appliedStyle(StaffStyle::NotationStyle::Tablature).at("vert_tab_num_off") == Value(-1024));
}

TEST_CASE("StaffStyleAssign comparison retains approved StaffStyle upgrade rules", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    constexpr std::string_view prefix = "staff_style_assigns[semantic=part=0,staff=1,range=5:0-9:2147483647]."
                                        "applied_style";
    ComparisonLeaves source{
        {std::string(prefix) + ".alt_hide_artics", {Value(true), "legacy-mus"}},
        {std::string(prefix) + ".alt_hide_lyrics", {Value(true), "legacy-mus"}},
        {std::string(prefix) + ".alt_hide_smart_shapes", {Value(true), "legacy-mus-adjusted"}},
        {std::string(prefix) + ".hide_chords", {Value(true), "legacy-mus"}},
        {std::string(prefix) + ".hide_fretboards", {Value(true), "legacy-mus"}},
        {std::string(prefix) + ".alt_hide_expressions", {Value(false), "finale27-default"}},
        {std::string(prefix) + "._classifier_masks.hide_chords", {Value(true), std::string{}}},
        {std::string(prefix) + "._classifier_masks.hide_fretboards", {Value(true), std::string{}}},
    };
    ComparisonLeaves companion = source;
    companion.at(std::string(prefix) + ".alt_hide_expressions").first = Value(true);
    companion.at(std::string(prefix) + ".hide_chords").first = Value(false);
    companion.at(std::string(prefix) + ".hide_fretboards").first = Value(false);
    ImportReport report(FormatEpoch::ZlibLegacy);
    const SourceVersion finale2008{.major = finale_mus_reader::versions::finale2008.major};
    const auto classify = differenceClassifier("staff_style_assigns");
    REQUIRE(classify);
    const auto classifyPath = [&](std::string_view suffix) {
        const auto path = std::string(prefix) + std::string(suffix);
        return classify(DifferenceContext{path, DifferenceCategory::Differs, source.at(path).second, source.at(path).first, companion.at(path).first,
            source, companion, FormatEpoch::ZlibLegacy, ByteOrder::LittleEndian, &finale2008, report});
    };
    CHECK(classifyPath(".alt_hide_expressions") == DifferenceClassification::FinaleUpgradeLoss);
    CHECK(classifyPath(".hide_chords") == DifferenceClassification::FinaleUpgradeLoss);
    CHECK(classifyPath(".hide_fretboards") == DifferenceClassification::FinaleUpgradeLoss);
}

TEST_CASE("StaffStyleAssign comparison recognizes pre-2012 instrument splits", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    constexpr std::string_view prefix = "staff_style_assigns[semantic=part=0,staff=1,range=5:0-9:2147483647]."
                                        "applied_style";
    ComparisonLeaves source{
        {std::string(prefix) + "._classifier_assignment_count", {Value(1), std::string{}}},
        {std::string(prefix) + ".notation_style", {Value(static_cast<std::int64_t>(StaffStyle::NotationStyle::Percussion)), "legacy-mus"}},
        {std::string(prefix) + ".use_note_font", {Value(false), "legacy-mus"}},
        {std::string(prefix) + ".note_font.font_size", {Value(0), "legacy-mus"}},
        {std::string(prefix) + ".vert_tab_num_off", {Value(0), "legacy-mus"}},
    };
    ComparisonLeaves companion = source;
    companion.at(std::string(prefix) + "._classifier_assignment_count").first = Value(2);
    companion.at(std::string(prefix) + ".use_note_font").first = Value(true);
    companion.at(std::string(prefix) + ".note_font.font_size").first = Value(24);
    companion.at(std::string(prefix) + ".vert_tab_num_off").first = Value(-1024);
    ImportReport report(FormatEpoch::ZlibLegacy);
    const SourceVersion finale2011{.major = finale_mus_reader::versions::finale2011.major};
    const auto classify = differenceClassifier("staff_style_assigns");
    REQUIRE(classify);
    const auto classifyPath = [&](std::string_view suffix) {
        const auto path = std::string(prefix) + std::string(suffix);
        return classify(DifferenceContext{path, DifferenceCategory::Differs, source.at(path).second, source.at(path).first, companion.at(path).first,
            source, companion, FormatEpoch::ZlibLegacy, ByteOrder::LittleEndian, &finale2011, report});
    };
    CHECK(classifyPath(".use_note_font") == DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classifyPath(".note_font.font_size") == DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classifyPath(".vert_tab_num_off") == DifferenceClassification::FinaleUpgradeNormalization);
}

TEST_CASE("StaffStyleAssign comparison recognizes one-for-one instrument promotion", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    constexpr std::string_view prefix = "staff_style_assigns[semantic=part=2,staff=1,range=5:0-9:2147483647]."
                                        "applied_style";
    ComparisonLeaves source{
        {std::string(prefix) + "._classifier_assignment_count", {Value(1), std::string{}}},
        {std::string(prefix) + "._classifier_masks.notation_style", {Value(true), std::string{}}},
        {std::string(prefix) + "._classifier_masks.staff_type", {Value(true), std::string{}}},
        {std::string(prefix) + ".notation_style", {Value(static_cast<std::int64_t>(StaffStyle::NotationStyle::Standard)), "legacy-mus"}},
        {std::string(prefix) + ".staff_lines", {Value(5), "legacy-mus"}},
    };
    ComparisonLeaves companion = source;
    companion.at(std::string(prefix) + ".notation_style").first = Value(static_cast<std::int64_t>(StaffStyle::NotationStyle::Percussion));
    companion.at(std::string(prefix) + ".staff_lines").first = Value(0);
    for (const auto mask : {"transposition", "full_name", "abrv_name"}) {
        companion.emplace(std::string(prefix) + "._classifier_masks." + mask, std::pair{Value(true), std::string{}});
    }
    ImportReport report(FormatEpoch::ZlibLegacy);
    const SourceVersion finale2011{.major = finale_mus_reader::versions::finale2011.major};
    const auto classify = differenceClassifier("staff_style_assigns");
    REQUIRE(classify);
    const auto classifyPath = [&](std::string_view suffix) {
        const auto path = std::string(prefix) + std::string(suffix);
        return classify(DifferenceContext{path, DifferenceCategory::Differs, source.at(path).second, source.at(path).first, companion.at(path).first,
            source, companion, FormatEpoch::ZlibLegacy, ByteOrder::LittleEndian, &finale2011, report});
    };

    CHECK(classifyPath(".notation_style") == DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classifyPath(".staff_lines") == DifferenceClassification::FinaleUpgradeNormalization);
    companion.erase(std::string(prefix) + "._classifier_masks.abrv_name");
    CHECK_FALSE(classifyPath(".staff_lines"));
}

TEST_CASE("StaffStyleAssign comparison recognizes redundant noKey removal in a percussion split", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    constexpr std::string_view prefix = "staff_style_assigns[semantic=part=0,staff=1,range=5:0-9:2147483647]."
                                        "applied_style";
    ComparisonLeaves source{
        {std::string(prefix) + "._classifier_assignment_count", {Value(1), std::string{}}},
        {std::string(prefix) + "._classifier_masks.notation_style", {Value(true), std::string{}}},
        {std::string(prefix) + "._classifier_masks.no_key", {Value(true), std::string{}}},
        {std::string(prefix) + ".notation_style", {Value(static_cast<std::int64_t>(StaffStyle::NotationStyle::Percussion)), "legacy-mus"}},
        {std::string(prefix) + ".no_key", {Value(true), "legacy-mus"}},
    };
    ComparisonLeaves companion = source;
    companion.at(std::string(prefix) + "._classifier_assignment_count").first = Value(3);
    companion.erase(std::string(prefix) + "._classifier_masks.no_key");
    companion.at(std::string(prefix) + ".no_key").first = Value(false);
    for (const auto mask : {"staff_type", "transposition", "full_name", "abrv_name"}) {
        companion.emplace(std::string(prefix) + "._classifier_masks." + mask, std::pair{Value(true), std::string{}});
    }
    ImportReport report(FormatEpoch::ZlibLegacy);
    const SourceVersion finale2011{.major = finale_mus_reader::versions::finale2011.major};
    const auto classify = differenceClassifier("staff_style_assigns");
    REQUIRE(classify);
    const auto path = std::string(prefix) + ".no_key";
    const auto classifyNoKey = [&]() {
        return classify(DifferenceContext{path, DifferenceCategory::Differs, source.at(path).second, source.at(path).first, companion.at(path).first,
            source, companion, FormatEpoch::ZlibLegacy, ByteOrder::LittleEndian, &finale2011, report});
    };

    CHECK(classifyNoKey() == DifferenceClassification::FinaleUpgradeNormalization);
    source.at(std::string(prefix) + ".notation_style").first = Value(static_cast<std::int64_t>(StaffStyle::NotationStyle::Standard));
    companion.at(std::string(prefix) + ".notation_style").first = Value(static_cast<std::int64_t>(StaffStyle::NotationStyle::Standard));
    CHECK_FALSE(classifyNoKey());
}

TEST_CASE("StaffStyleAssign comparison recognizes masked payload dropped in a percussion collapse", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    constexpr std::string_view prefix = "staff_style_assigns[semantic=part=0,staff=1,range=2:0-15:2147483647]."
                                        "applied_style";
    ComparisonLeaves source{
        {std::string(prefix) + "._classifier_assignment_count", {Value(2), std::string{}}},
        {std::string(prefix) + ".notation_style", {Value(static_cast<std::int64_t>(StaffStyle::NotationStyle::Percussion)), "legacy-mus"}},
        {std::string(prefix) + ".no_key", {Value(true), "legacy-mus"}},
        {std::string(prefix) + ".use_note_font", {Value(true), "legacy-mus"}},
        {std::string(prefix) + ".note_font.font_id", {Value(5), "legacy-mus"}},
        {std::string(prefix) + ".note_font.font_size", {Value(24), "legacy-mus"}},
    };
    for (const auto mask : {"notation_style", "staff_type", "transposition", "full_name", "abrv_name", "no_key", "float_notehead_font"}) {
        source.emplace(std::string(prefix) + "._classifier_masks." + mask, std::pair{Value(true), std::string{}});
    }
    ComparisonLeaves companion = source;
    companion.at(std::string(prefix) + "._classifier_assignment_count").first = Value(1);
    companion.erase(std::string(prefix) + "._classifier_masks.no_key");
    companion.erase(std::string(prefix) + "._classifier_masks.float_notehead_font");
    companion.at(std::string(prefix) + ".no_key").first = Value(false);
    companion.at(std::string(prefix) + ".use_note_font").first = Value(false);
    companion.at(std::string(prefix) + ".note_font.font_id").first = Value(0);
    companion.at(std::string(prefix) + ".note_font.font_size").first = Value(0);

    ImportReport report(FormatEpoch::ZlibLegacy);
    const SourceVersion finale2011{.major = finale_mus_reader::versions::finale2011.major};
    const auto classify = differenceClassifier("staff_style_assigns");
    REQUIRE(classify);
    const auto classifyPath = [&](std::string_view suffix) {
        const auto path = std::string(prefix) + std::string(suffix);
        return classify(DifferenceContext{path, DifferenceCategory::Differs, source.at(path).second, source.at(path).first, companion.at(path).first,
            source, companion, FormatEpoch::ZlibLegacy, ByteOrder::LittleEndian, &finale2011, report});
    };

    CHECK(classifyPath(".no_key") == DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classifyPath(".use_note_font") == DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classifyPath(".note_font.font_id") == DifferenceClassification::FinaleUpgradeNormalization);
    CHECK(classifyPath(".note_font.font_size") == DifferenceClassification::FinaleUpgradeNormalization);

    companion.emplace(std::string(prefix) + "._classifier_masks.float_notehead_font", std::pair{Value(true), std::string{}});
    CHECK_FALSE(classifyPath(".note_font.font_size"));
}

TEST_CASE("StaffStyleAssign name references compare through their block text", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using BlockText = musx::dom::texts::BlockText;
    using TextBlock = musx::dom::others::TextBlock;
    const auto document = [](musx::dom::Cmper blockId, musx::dom::Cmper textId) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto building = session.getDocument();
        auto text = std::make_shared<BlockText>(building, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, textId);
        text->text = "Horn";
        building->getTexts()->add(BlockText::XmlNodeName, std::move(text));
        auto block = std::make_shared<TextBlock>(building, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, blockId);
        block->textId = textId;
        block->textType = TextBlock::TextType::Block;
        building->getOthers()->add(TextBlock::XmlNodeName, std::move(block));
        return std::move(session).finish();
    };
    const auto snapshot = [](std::int64_t textId) {
        return SurveySnapshot{{"staff_style_assigns",
            Value::Array{Value::Object{{"part_id", Value(0)}, {"share_mode", Value(0)}, {"cmper", Value(1)}, {"inci", Value(0)},
                {"style_id", Value(1)}, {"start_meas", Value(1)}, {"start_edu", Value(0)}, {"end_meas", Value(1)}, {"end_edu", Value(0x7fffffff)},
                {"_staff_style_patch", Value::Object{{"full_name_text_id", Value(textId)}}}}}}};
    };
    ImportReport report(FormatEpoch::DclLegacy);
    const auto comparison =
        compareSnapshots(snapshot(7), snapshot(19), document(7, 3), document(19, 41), FormatEpoch::DclLegacy, ByteOrder::BigEndian, nullptr, report);
    CHECK(comparison.classes.at("others").at("staff_style_assigns").unexpected == 0);
}

TEST_CASE("Controlled StaffStyleAssign fixtures recover their exact ranges")
{
    const auto finale2000 = readFixture("evidence/F2000/F2000-staffstyle-applied.mus");
    const auto assign2000 = finale2000.document->getOthers()->get<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1}, musx::dom::Inci{0});
    REQUIRE(assign2000);
    CHECK(assign2000->styleId == 1);
    CHECK(assign2000->startMeas == 1);
    CHECK(assign2000->startEdu == 1024);
    CHECK(assign2000->endMeas == 2);
    CHECK(assign2000->endEdu == 2047);

    const auto finale2003 = readFixture("evidence/F2003/F2003-staffstyle-assigned.mus");
    const auto assign2003 = finale2003.document->getOthers()->get<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1}, musx::dom::Inci{0});
    REQUIRE(assign2003);
    CHECK(assign2003->startMeas == 1);
    CHECK(assign2003->startEdu == 1024);
    CHECK(assign2003->endMeas == 2);
    CHECK(assign2003->endEdu == 2047);

    const auto finale2011 = readFixture("evidence/F2011/F2011-staffstyle-assigned.mus");
    const auto assign2011 = finale2011.document->getOthers()->get<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1}, musx::dom::Inci{0});
    REQUIRE(assign2011);
    CHECK(assign2011->startMeas == 1);
    CHECK(assign2011->startEdu == 1024);
    CHECK(assign2011->endMeas == 2);
    CHECK(assign2011->endEdu == 2047);
}

TEST_CASE("Pre-Finale-2000 alternate notation ranges synthesize Staff Styles and assignments")
{
    using Notation = Staff::AlternateNotation;
    struct Expected
    {
        const char* fixture;
        musx::dom::Cmper styleId;
        Notation notation;
        const char* styleName;
        musx::dom::MeasCmper endMeas;
        bool hidesAttachedItems;
    };

    for (const auto& expected : {
             Expected{"evidence/F98/F98-altnotation-partial.mus", musx::dom::Cmper(2), Notation::SlashBeats, "Slash Notation",
                 musx::dom::MeasCmper(3), false},
             Expected{"evidence/F98/F98-altnotation-full.mus", musx::dom::Cmper(5), Notation::TwoBarRepeat, "Two Bar Repeats",
                 musx::dom::MeasCmper(44), true},
         }) {
        const auto result = readFixture(expected.fixture);
        const auto style = result.document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID, expected.styleId);
        REQUIRE(style);
        REQUIRE(style->masks);
        CHECK(style->styleName == expected.styleName);
        CHECK(style->altNotation == expected.notation);
        CHECK(style->copyable);
        CHECK(style->addToMenu);
        CHECK(style->masks->altNotation);
        CHECK(style->masks->hideChords);
        CHECK(style->masks->hideFretboards);
        CHECK(style->hideChords == expected.hidesAttachedItems);
        CHECK(style->hideFretboards == expected.hidesAttachedItems);

        const auto assignment = result.document->getOthers()->get<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1}, musx::dom::Inci{0});
        REQUIRE(assignment);
        CHECK(assignment->styleId == expected.styleId);
        CHECK(assignment->startMeas == 1);
        CHECK(assignment->startEdu == 0);
        CHECK(assignment->endMeas == expected.endMeas);
        CHECK(assignment->endEdu == (std::numeric_limits<musx::dom::Edu>::max)());
        CHECK(result.document->getOthers()->getArray<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1}).size() == 1);
        CHECK(result.document->getOthers()->getAllSources<StaffStyle>().size() == 6);

        const auto staff = result.document->getOthers()->get<Staff>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1});
        REQUIRE(staff);
        CHECK(staff->hasStyles);

        const auto* styleType = result.report.findField<StaffStyle>("altNotation", musx::dom::SCORE_PARTID, expected.styleId);
        REQUIRE(styleType);
        CHECK(styleType->origin == ValueOrigin::LegacyMusAdjusted);
        CHECK(styleType->sourceIdentity == finale_mus_reader::records::packTag("GF"));
        const auto* assignmentStyle =
            result.report.findField<StaffStyleAssign>("styleId", musx::dom::SCORE_PARTID, musx::dom::Cmper{1}, musx::dom::Inci{0});
        REQUIRE(assignmentStyle);
        CHECK(assignmentStyle->origin == ValueOrigin::LegacyMusAdjusted);
        CHECK(assignmentStyle->sourceIdentity == finale_mus_reader::records::packTag("GF"));
    }

    const auto baseline = readFixture("evidence/F98/F98-baseline.mus");
    constexpr std::pair<std::string_view, Notation> canonicalStyles[]{
        {"Normal Notation", Notation::Normal},
        {"Slash Notation", Notation::SlashBeats},
        {"Rhythmic Notation", Notation::Rhythmic},
        {"One Bar Repeats", Notation::OneBarRepeat},
        {"Two Bar Repeats", Notation::TwoBarRepeat},
        {"Blank Notation", Notation::Blank},
    };
    const auto styles = baseline.document->getOthers()->getAllSources<StaffStyle>();
    REQUIRE(styles.size() == std::size(canonicalStyles));
    for (std::size_t index = 0; index < std::size(canonicalStyles); ++index) {
        const auto style = baseline.document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID, static_cast<musx::dom::Cmper>(index + 1));
        REQUIRE(style);
        CHECK(style->styleName == canonicalStyles[index].first);
        CHECK(style->altNotation == canonicalStyles[index].second);
        CHECK(style->instUuid == musx::dom::uuid::BlankStaff);
        CHECK(style->botRepeatDotOff == -5);
        CHECK(style->topRepeatDotOff == -3);
        CHECK(style->dwRestOffset == -4);
        CHECK(style->wRestOffset == -4);
        CHECK(style->hRestOffset == -4);
        CHECK(style->otherRestOffset == -4);
        CHECK(style->lineSpace == 24);
        REQUIRE(style->noteFont);
        CHECK(style->noteFont->fontSize == 24);
        CHECK(style->stemReversal == -4);
        REQUIRE(style->masks);
        CHECK(style->masks->altNotation);
        const auto* styleType =
            baseline.report.findField<StaffStyle>("altNotation", musx::dom::SCORE_PARTID, static_cast<musx::dom::Cmper>(index + 1));
        REQUIRE(styleType);
        CHECK(styleType->origin == ValueOrigin::LegacyBehavior);
        const auto* styleUuid = baseline.report.findField<StaffStyle>("instUuid", musx::dom::SCORE_PARTID, static_cast<musx::dom::Cmper>(index + 1));
        REQUIRE(styleUuid);
        CHECK(styleUuid->origin == ValueOrigin::LegacyBehavior);
        const auto* lineSpace = baseline.report.findField<StaffStyle>("lineSpace", musx::dom::SCORE_PARTID, static_cast<musx::dom::Cmper>(index + 1));
        REQUIRE(lineSpace);
        CHECK(lineSpace->origin == ValueOrigin::Finale27Default);
    }
    CHECK(baseline.document->getOthers()->getAllSources<StaffStyleAssign>().empty());
}

TEST_CASE("Controlled GFrameHolds recover every legacy alternate notation")
{
    constexpr std::tuple<musx::dom::Cmper, musx::dom::MeasCmper, musx::dom::MeasCmper> expectedAssignments[]{
        {musx::dom::Cmper(2), musx::dom::MeasCmper(2), musx::dom::MeasCmper(2)},
        {musx::dom::Cmper(3), musx::dom::MeasCmper(3), musx::dom::MeasCmper(3)},
        {musx::dom::Cmper(4), musx::dom::MeasCmper(4), musx::dom::MeasCmper(4)},
        {musx::dom::Cmper(5), musx::dom::MeasCmper(5), musx::dom::MeasCmper(6)},
        {musx::dom::Cmper(6), musx::dom::MeasCmper(7), musx::dom::MeasCmper(7)},
    };
    constexpr std::pair<musx::dom::Cmper, std::int64_t> expectedRawTypes[]{
        {musx::dom::Cmper(2), std::int64_t(1)},
        {musx::dom::Cmper(3), std::int64_t(2)},
        {musx::dom::Cmper(4), std::int64_t(3)},
        {musx::dom::Cmper(6), std::int64_t(6)},
    };

    for (const auto fixture : {"evidence/F263/F263-altnotation.mus", "evidence/F372/F372-altnotation.mus", "evidence/F97/F97-altnotation.mus",
             "evidence/F98/F98-altnotation.mus"}) {
        const auto result = readFixture(fixture);
        const auto assignments = result.document->getOthers()->getArray<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{1});
        REQUIRE(assignments.size() == std::size(expectedAssignments));
        for (std::size_t index = 0; index < std::size(expectedAssignments); ++index) {
            const auto& [styleId, startMeas, endMeas] = expectedAssignments[index];
            CHECK(assignments[index]->styleId == styleId);
            CHECK(assignments[index]->startMeas == startMeas);
            CHECK(assignments[index]->startEdu == 0);
            CHECK(assignments[index]->endMeas == endMeas);
            CHECK(assignments[index]->endEdu == (std::numeric_limits<musx::dom::Edu>::max)());
        }
        for (const auto& [styleId, rawType] : expectedRawTypes) {
            const auto* field = result.report.findField<StaffStyle>("altNotation", musx::dom::SCORE_PARTID, styleId);
            REQUIRE(field);
            CHECK(field->origin == ValueOrigin::LegacyMusAdjusted);
            CHECK(field->rawValue == rawType);
            CHECK(field->sourceIdentity == finale_mus_reader::records::packTag("GF"));
        }
    }
}

TEST_CASE("Pre-Finale-2000 alternate notation selects presumed GFrameHold flags at Finale 98")
{
    const auto shortGFrameHold = importStaffStyleAssigns(makeDetailContainer(FormatEpoch::CodaBanner, 4, 3, {9, 0, 0, 0, 0x1231}, "GF"));
    const auto shortAssignment =
        shortGFrameHold.document->getOthers()->get<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{4}, musx::dom::Inci{0});
    REQUIRE(shortAssignment);
    CHECK(shortAssignment->styleId == 2);
    CHECK(shortAssignment->startMeas == 3);
    CHECK(shortAssignment->endMeas == 3);

    const auto repeatedCodaGFrameHold =
        importStaffStyleAssigns(makeDetailContainer(FormatEpoch::CodaBanner, 4, 3, {9, 0x1231, 0, 0, 0, 9, 0x1231, 0, 0, 0}, "GF"));
    CHECK(repeatedCodaGFrameHold.document->getOthers()->getAllSources<StaffStyleAssign>().empty());

    const auto finale97GFrameHold =
        importStaffStyleAssigns(makeDetailContainer(FormatEpoch::UncompressedLegacy, 4, 3, {9, 0x1232, 0, 0, 0x1231}, "GF"), false,
            SourceVersion{.major = finale_mus_reader::versions::finale97.major});
    const auto finale97Assignment =
        finale97GFrameHold.document->getOthers()->get<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{4}, musx::dom::Inci{0});
    REQUIRE(finale97Assignment);
    CHECK(finale97Assignment->styleId == 2);

    const auto finale98GFrameHold =
        importStaffStyleAssigns(makeDetailContainer(FormatEpoch::UncompressedLegacy, 4, 3, {9, 0x1231, 0, 0, 0x1232}, "GF"), false,
            SourceVersion{.major = finale_mus_reader::versions::finale98.major});
    const auto finale98Assignment =
        finale98GFrameHold.document->getOthers()->get<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{4}, musx::dom::Inci{0});
    REQUIRE(finale98Assignment);
    CHECK(finale98Assignment->styleId == 2);

    for (const auto& [storedType, styleId, notation] : {
             std::tuple{2, musx::dom::Cmper{3}, Staff::AlternateNotation::Rhythmic},
             std::tuple{3, musx::dom::Cmper{4}, Staff::AlternateNotation::OneBarRepeat},
             std::tuple{6, musx::dom::Cmper{6}, Staff::AlternateNotation::Blank},
         }) {
        const auto imported = importStaffStyleAssigns(
            makeDetailContainer(FormatEpoch::CodaBanner, 4, 3, {9, 0, 0, 0, static_cast<std::int16_t>(0x1230 | storedType)}, "GF"));
        const auto style = imported.document->getOthers()->get<StaffStyle>(musx::dom::SCORE_PARTID, styleId);
        REQUIRE(style);
        CHECK(style->altNotation == notation);
        const auto storedAssignment =
            imported.document->getOthers()->get<StaffStyleAssign>(musx::dom::SCORE_PARTID, musx::dom::Cmper{4}, musx::dom::Inci{0});
        REQUIRE(storedAssignment);
        CHECK(storedAssignment->styleId == styleId);
    }
}

} // namespace
} // namespace finale_mus_reader_tests
