// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"
#include "coverage/classification_rules.h"
#include "coverage/registry.h"

#include <algorithm>
#include <map>
#include <optional>

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

using Measure = musx::dom::others::Measure;
using BarlineType = Measure::BarlineType;
using PositioningType = Measure::PositioningType;
using ShowKeySigMode = Measure::ShowKeySigMode;
using ShowTimeSigMode = Measure::ShowTimeSigMode;

// Every persisted leaf of the musxdom class, contained key signature included. The reader and the
// class surveyor must both account for all of them, whatever the source layout supplies.
constexpr std::size_t measureFieldManifestSize = 43;

musx::dom::DocumentPtr emptyMeasureDocument()
{
    auto session = musx::factory::DocumentFactory::begin();
    return session.getDocument();
}

ImportReport measureImport(const finale_mus_reader::container::ParsedContainer& parsed,
    const SourceProfile& profile, const musx::dom::DocumentPtr& document)
{
    ImportReport report(profile.epoch);
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{
        index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importMeasures(context);
    return report;
}

const finale_mus_reader::FieldInfo& partField(const ImportReport& report, std::uint16_t partId,
    musx::dom::Cmper cmper, std::string_view member)
{
    const auto* info = report.findField(
        finale_mus_reader::instanceKey<Measure>(partId, cmper), member);
    expect(info != nullptr,
        "Missing measure report for part " + std::to_string(partId) + " member "
            + std::string(member));
    return *info;
}

TEST_CASE("Six-word measure records recover the words every era stores")
{
    // Finale 3.0 through Finale 98: one 16-byte row. The auxiliary word sets composite numerator,
    // never-show-time, and positioning mode 4; the measure word sets a new system, an expression,
    // a final barline, and a backwards repeat.
    const auto parsed = makeContainer(
        {{1, "MS", {600, 0x0201, 4, 1024, static_cast<std::int16_t>(0x0094),
             static_cast<std::int16_t>(0xc054)}}},
        FormatEpoch::UncompressedLegacy);
    const auto document = emptyMeasureDocument();
    const auto report = measureImport(parsed, SourceProfile(FormatEpoch::UncompressedLegacy),
        document);

    const auto measure = document->getOthers()->get<Measure>(musx::dom::SCORE_PARTID, 1);
    expect(measure != nullptr, "The six-word measure was not constructed");
    expect(measure->width == 600 && measure->beats == 4 && measure->divBeat == 1024,
        "The six-word measure did not recover its width or time signature");
    expect(measure->globalKeySig && measure->globalKeySig->key == 0x0201,
        "The six-word measure did not recover its key signature");
    expect(measure->compositeNumerator && !measure->compositeDenominator,
        "The composite numerator bit was not recovered");
    expect(measure->showTime == ShowTimeSigMode::Never
            && measure->showKey == ShowKeySigMode::IfNeeded,
        "The never-show-time bit did not select the show mode");
    expect(measure->positioningMode == PositioningType::TimeSigPlusPositioning,
        "Legacy positioning code 4 did not translate to the musxdom value");
    expect(measure->beginNewSystem && measure->hasExpression && measure->backwardsRepeatBar,
        "The measure flag word was not decoded");
    expect(measure->barlineType == BarlineType::Final,
        "Legacy barline code 5 did not translate to the musxdom value");
    // No word of this layout carries a left barline, and the era took it from the document's
    // barline options, which is what a later record's code 15 spells.
    expect(measure->leftBarlineType == BarlineType::OptionsDefault,
        "The six-word layout did not supply its left-barline behavior");
    expect(field(report, "others.measSpec[1].leftBarlineType").origin
            == ValueOrigin::LegacyBehavior,
        "The absent left barline was not reported as era behavior");
    // The display time signature lives in its own `ms` record here, and this measure has none, so
    // it has no display time signature rather than an unrecovered one.
    for (const auto* member : {"dispBeats", "dispDivbeat", "useDisplayTimesig",
             "compositeDispNumerator", "compositeDispDenominator"}) {
        expect(field(report, std::string("others.measSpec[1].") + member).origin
                == ValueOrigin::LegacyBehavior,
            std::string("A display time-signature member was not reported as era behavior: ")
                + member);
    }
    expect(!measure->useDisplayTimesig && measure->dispBeats == 0,
        "A measure with no display record was given a display time signature");
    // The era abbreviates by the document-wide time-signature option, not per measure.
    expect(field(report, "others.measSpec[1].abbrvTime").origin == ValueOrigin::LegacyBehavior,
        "The abbreviated-time flag was not reported as era behavior");
    // The era keeps chords as entry details, so the measure record has no word for the flag.
    expect(field(report, "others.measSpec[1].hasChord").origin == ValueOrigin::LegacyBehavior,
        "The chord flag was not reported as era behavior where the layout has no word for it");
    expect(field(report, "others.measSpec[1].globalKeySig.keyless").origin
            == ValueOrigin::LegacyBehavior,
        "The key-signature switches were not reported as era behavior");
    expect(reportedFieldCount(report) == measureFieldManifestSize,
        "The six-word report does not exhaust the Measure field manifest");
}

TEST_CASE("A six-word measure takes its display time signature from its own `ms` record")
{
    // `ms` mirrors `MS`: the display beats and divisions sit in the slots the measure record gives
    // the actual time signature, and the auxiliary word carries the composite bits at the same
    // positions. Measure 2 has no such record and therefore no display time signature.
    const auto parsed = makeContainer(
        {{1, "MS", {600, 0, 4, 1024, 0x0004, 0x0010}},
            {1, "ms", {0, 0, 3, 1536, static_cast<std::int16_t>(0x00c0), 0}},
            {2, "MS", {600, 0, 4, 1024, 0x0004, 0x0010}}},
        FormatEpoch::UncompressedLegacy);
    const auto document = emptyMeasureDocument();
    const auto report = measureImport(parsed, SourceProfile(FormatEpoch::UncompressedLegacy),
        document);

    const auto withDisplay = document->getOthers()->get<Measure>(musx::dom::SCORE_PARTID, 1);
    expect(withDisplay != nullptr, "The measure with a display record was not constructed");
    expect(withDisplay->beats == 4 && withDisplay->divBeat == 1024,
        "The display record overwrote the actual time signature");
    expect(withDisplay->dispBeats == 3 && withDisplay->dispDivbeat == 1536,
        "The display time signature was not read from the `ms` record");
    // The record's presence is what says the measure uses a display time signature.
    expect(withDisplay->useDisplayTimesig,
        "A measure with a display record was not marked as using one");
    expect(withDisplay->compositeDispNumerator && withDisplay->compositeDispDenominator,
        "The display composite bits were not read from the `ms` auxiliary word");
    // The report cites the record the value actually came from.
    expect(field(report, "others.measSpec[1].dispBeats").origin == ValueOrigin::LegacyMus,
        "The display time signature has the wrong origin");
    expect(field(report, "others.measSpec[1].dispBeats").sourceIdentity
            == finale_mus_reader::records::packTag("ms"),
        "The display time signature does not cite the record it came from");

    const auto without = document->getOthers()->get<Measure>(musx::dom::SCORE_PARTID, 2);
    expect(without && !without->useDisplayTimesig && without->dispBeats == 0
            && !without->compositeDispNumerator,
        "A measure with no display record was given a display time signature");
}

TEST_CASE("Coda-banner measure records read their own flag word")
{
    // Bit 0x0010 is the measure-has-expression flag here, where the later word begins a barline
    // code; 0x0020 is a double barline, 0x0040 a multimeasure-rest break, and 0x0080 a final one.
    struct Case
    {
        std::int16_t aux;
        std::int16_t measure;
        BarlineType barline;
        bool hasExpression;
        bool breakMmRest;
    };
    const Case cases[] = {
        {0x0006, 0x0000, BarlineType::Normal, false, false},
        {0x0006, 0x0010, BarlineType::Normal, true, false},
        {0x0006, 0x0020, BarlineType::Double, false, true},
        {0x0006, 0x0040, BarlineType::Normal, false, true},
        {0x0006, static_cast<std::int16_t>(0x00c0), BarlineType::Final, false, true},
        {0x1006, 0x0010, BarlineType::None, true, false},
    };
    for (const auto& testCase : cases) {
        const auto parsed = makeContainer(
            {{1, "MS", {600, 0, 4, 1024, testCase.aux, testCase.measure}}},
            FormatEpoch::CodaBanner);
        const auto document = emptyMeasureDocument();
        const auto report = measureImport(parsed, SourceProfile(FormatEpoch::CodaBanner), document);
        const auto measure = document->getOthers()->get<Measure>(musx::dom::SCORE_PARTID, 1);
        expect(measure != nullptr, "The Coda-banner measure was not constructed");
        expect(measure->barlineType == testCase.barline,
            "The Coda-banner barline was decoded as the later layout's code");
        expect(measure->hasExpression == testCase.hasExpression,
            "The Coda-banner expression flag was not read at its own bit");
        expect(measure->breakMmRest == testCase.breakMmRest,
            "The Coda-banner multimeasure-rest break was not decoded");
        // The bit that later overrides a staff group's barline says "no barline" here, so it is
        // not also read as the override.
        expect(!measure->groupBarlineOverride,
            "The Coda-banner no-barline bit was read as a group-barline override");
        expect(field(report, "others.measSpec[1].groupBarlineOverride").origin
                == ValueOrigin::LegacyBehavior,
            "The Coda-banner group-barline override was not reported as era behavior");
        expect(reportedFieldCount(report) == measureFieldManifestSize,
            "The Coda-banner report does not exhaust the Measure field manifest");
    }
}

TEST_CASE("Twelve-word measure records add the display time signature and front space")
{
    // Finale 2000 through Finale 2004: two 16-byte rows. The second carries the display time
    // signature, the new flag word, the two custom barline shapes, and the front space.
    const auto parsed = makeContainer(
        {{1, "MS", {360, 0, 4, 1024, 0x0006, 0x0010}},
            {1, "MS", {2, 2048, static_cast<std::int16_t>(0x0af6), 11, 12, -22}}},
        FormatEpoch::DclLegacy);
    const auto document = emptyMeasureDocument();
    const auto report = measureImport(parsed, SourceProfile(FormatEpoch::DclLegacy), document);

    const auto measure = document->getOthers()->get<Measure>(musx::dom::SCORE_PARTID, 1);
    expect(measure != nullptr, "The twelve-word measure was not constructed");
    expect(measure->dispBeats == 2 && measure->dispDivbeat == 2048,
        "The display time signature was not recovered from the second row");
    expect(measure->abbrvTime && measure->useDisplayTimesig,
        "The new flag word's time-signature bits were not decoded");
    expect(measure->pageBreak, "The page-break bit was not decoded");
    expect(!measure->hasChord, "A clear chord bit was decoded as set");
    expect(field(report, "others.measSpec[1].hasChord").origin == ValueOrigin::LegacyMus,
        "The chord flag was not reported as a stored value where its word exists");
    expect(measure->leftBarlineType == BarlineType::OptionsDefault,
        "Left-barline code 15 did not select the barline-options default");
    expect(measure->customBarShape == 11 && measure->customLeftBarShape == 12,
        "The custom barline shape comparators were not recovered");
    expect(measure->frontSpaceExtra == -22,
        "The front space was not recovered as a signed value");
    // The thirteenth word arrives with Finale 2005, so this layout has no back space at all.
    expect(measure->backSpaceExtra == 0, "A twelve-word record supplied a back space");
    expect(field(report, "others.measSpec[1].backSpaceExtra").origin == ValueOrigin::LegacyBehavior,
        "The absent back space was not reported as era behavior");
    // The offsets have to name the row the word actually lives in, not the start of the family.
    expect(field(report, "others.measSpec[1].frontSpaceExtra").decodedOffset
            > field(report, "others.measSpec[1].width").decodedOffset,
        "A second-row member reported a first-row offset");
    expect(reportedFieldCount(report) == measureFieldManifestSize,
        "The twelve-word report does not exhaust the Measure field manifest");
}

TEST_CASE("Thirteen-word measure records recover every member in both byte orders")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        const auto parsed = makeClassContainer(
            {SyntheticClassRow{0x00b0,
                {360, 0, 4, 1024, static_cast<std::int16_t>(0xa886),
                    static_cast<std::int16_t>(0xcc30), 2, 2048,
                    static_cast<std::int16_t>(0x03fe), 11, 12, -22, -9},
                1, 0}},
            byteOrder);
        const auto document = emptyMeasureDocument();
        SourceProfile profile(FormatEpoch::ZlibLegacy);
        profile.byteOrder = byteOrder;
        // "Show Full Staff & Group Names" is read only from Finale 2011, so the source has to say
        // which release wrote it.
        profile.version = SourceVersion{.major = 17};
        const auto report = measureImport(parsed, profile, document);

        const auto measure = document->getOthers()->get<Measure>(musx::dom::SCORE_PARTID, 1);
        expect(measure != nullptr, "The thirteen-word measure was not constructed");
        expect(measure->breakWordExt && measure->hasSmartShape && measure->showFullNames,
            "The high auxiliary bits were not decoded");
        expect(measure->compositeNumerator && !measure->compositeDenominator,
            "The composite time-signature bits were not decoded");
        expect(measure->showKey == ShowKeySigMode::Always
                && measure->showTime == ShowTimeSigMode::Always,
            "The always-show bits of the measure word did not select the show modes");
        expect(measure->beginNewSystem && measure->hasExpression,
            "The measure word's high bits were not decoded");
        expect(measure->barlineType == BarlineType::Dashed,
            "Legacy barline code 3 did not translate to the musxdom value");
        expect(measure->compositeDispNumerator && measure->compositeDispDenominator,
            "The display composite bits of the new flag word were not decoded");
        expect(measure->hasChord, "The chord bit of the new flag word was not decoded");
        expect(measure->backSpaceExtra == -9,
            "The thirteenth word did not supply the back space");
        expect(field(report, "others.measSpec[1].backSpaceExtra").origin == ValueOrigin::LegacyMus,
            "The recovered back space has the wrong origin");
        expect(reportedFieldCount(report) == measureFieldManifestSize,
            "The thirteen-word report does not exhaust the Measure field manifest");
    }
}

TEST_CASE("The full-names bit is read only from Finale 2011")
{
    // The same record, read by three releases. The bit predates the setting, so only the last of
    // them may take it as one; the others report what the era did instead.
    struct Case { std::optional<SourceVersion> version; bool expected; ValueOrigin origin; };
    const Case cases[] = {
        {SourceVersion{.major = 16}, true, ValueOrigin::LegacyMus},     // Finale 2011
        {SourceVersion{.major = 15}, false, ValueOrigin::LegacyBehavior}, // Finale 2010
        {std::nullopt, false, ValueOrigin::LegacyBehavior},             // version not recovered
    };
    for (const auto& testCase : cases) {
        const auto parsed = makeClassContainer(
            {SyntheticClassRow{0x00b0,
                {360, 0, 4, 1024, static_cast<std::int16_t>(0x0806), 0x0010, 0, 0,
                    static_cast<std::int16_t>(0x00f0), 0, 0, 0, 0},
                1, 0}},
            ByteOrder::LittleEndian);
        const auto document = emptyMeasureDocument();
        SourceProfile profile(FormatEpoch::ZlibLegacy);
        profile.byteOrder = ByteOrder::LittleEndian;
        profile.version = testCase.version;
        const auto report = measureImport(parsed, profile, document);
        const auto measure = document->getOthers()->get<Measure>(musx::dom::SCORE_PARTID, 1);
        expect(measure && measure->showFullNames == testCase.expected,
            "The full-names bit was not gated on the release that introduced the setting");
        expect(field(report, "others.measSpec[1].showFullNames").origin == testCase.origin,
            "The full-names bit has the wrong origin for its release");
    }
}

TEST_CASE("Never-show wins over always-show when a record sets both")
{
    const auto parsed = makeContainer(
        {{1, "MS", {600, 0, 4, 1024, 0x0036, static_cast<std::int16_t>(0x0c10)}}},
        FormatEpoch::UncompressedLegacy);
    const auto document = emptyMeasureDocument();
    measureImport(parsed, SourceProfile(FormatEpoch::UncompressedLegacy), document);
    const auto measure = document->getOthers()->get<Measure>(musx::dom::SCORE_PARTID, 1);
    expect(measure && measure->showKey == ShowKeySigMode::Never
            && measure->showTime == ShowTimeSigMode::Never,
        "A record setting both show bits did not resolve to never");
}

TEST_CASE("A compact part record overlays the score measure it is linked to")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        const auto parsed = makeClassContainer(
            {SyntheticClassRow{0x00b0,
                 {360, 0x0201, 4, 1024, 0x0006, static_cast<std::int16_t>(0x4010), 2, 2048,
                     static_cast<std::int16_t>(0x00f0), 11, 12, -22, -9},
                 1, 0},
                SyntheticClassRow{0x00b0,
                    {575, static_cast<std::int16_t>(0x4004), 7, 8}, 1, 1}},
            byteOrder);
        const auto document = emptyMeasureDocument();
        SourceProfile profile(FormatEpoch::ZlibLegacy);
        profile.byteOrder = byteOrder;
        const auto report = measureImport(parsed, profile, document);

        const auto part = document->getOthers()->get<Measure>(1, 1);
        expect(part != nullptr, "The compact part measure was not constructed");
        expect(part->getShareMode() == musx::dom::EnigmaBase::ShareMode::Partial,
            "A compact part record did not produce a partially linked object");
        // The four members the compact record carries.
        expect(part->width == 575, "The part width was not overlaid");
        expect(part->positioningMode == PositioningType::TimeSigPlusPositioning,
            "The part positioning mode was not read from the compact flag word");
        expect(part->pageBreak, "The compact page-break bit was not decoded");
        expect(part->frontSpaceExtra == 7 && part->backSpaceExtra == 8,
            "The compact spacing words were not overlaid");
        // Everything else is the score's, because that is what partial linkage means.
        expect(part->beats == 4 && part->divBeat == 1024 && part->dispBeats == 2,
            "The part measure did not inherit the score time signature");
        expect(part->globalKeySig && part->globalKeySig->key == 0x0201,
            "The part measure did not inherit the score key signature");
        expect(part->barlineType == BarlineType::Normal && part->hasExpression,
            "The part measure did not inherit the score measure flags");
        // An inherited member keeps the score record's provenance, and an overlaid one reports
        // the part record it came from.
        expect(partField(report, 1, 1, "beats").origin == ValueOrigin::LegacyMus
                && partField(report, 1, 1, "beats").decodedOffset
                    == partField(report, 0, 1, "beats").decodedOffset,
            "An inherited member did not keep the score measure's provenance");
        expect(partField(report, 1, 1, "width").decodedOffset
                != partField(report, 0, 1, "width").decodedOffset,
            "An overlaid member did not report the part record's own offset");
        const auto partKey = finale_mus_reader::instanceKey<Measure>(
            musx::dom::Cmper(1), musx::dom::Cmper(1));
        expect(report.fields.at(partKey).size() == measureFieldManifestSize,
            "The part report does not exhaust the Measure field manifest");
    }
}

TEST_CASE("Companion part measures that repeat their score measure leave the comparison",
          "[coverage][measure]")
{
    using namespace finale_mus_reader::coverage;
    const auto measure = [](std::int64_t partId, std::int64_t cmper, std::int64_t width,
                            std::int64_t posMode = 4) {
        return Value::Object{{"part_id", partId}, {"cmper", cmper}, {"width", width},
            {"positioning_mode", posMode}, {"share_mode", partId ? 1 : 0},
            {"origin_width", std::string("legacy-mus")}};
    };
    // The reader has one score measure. The companion has that measure plus three part instances:
    // one repeating it, one Finale re-laid out, and one for a measure the reader also has a part
    // record for.
    SurveySnapshot source{{"measures", Value::Array{measure(0, 1, 600), measure(2, 1, 600)}}};
    SurveySnapshot companion{{"measures", Value::Array{measure(0, 1, 600), measure(1, 1, 600),
        measure(3, 1, 575), measure(2, 1, 600)}}};
    std::map<ComparisonTransformation, std::uint64_t> transformations;
    ComparisonPreparationContext context{
        source, companion, transformations, FormatEpoch::ZlibLegacy};
    runComparisonPreparers(context);

    const auto& left = companion.at("measures").asArray();
    expect(left.size() == 3, "The materialized part measure was not dropped");
    expect(transformations[ComparisonTransformation::FinaleMaterializedPartMeasure] == 1,
        "The dropped part measure was not counted as a transformation");
    const auto has = [&](std::int64_t partId, std::int64_t width) {
        return std::any_of(left.begin(), left.end(), [&](const Value& v) {
            return v.find("part_id")->asInteger() == partId
                && v.find("width")->asInteger() == width;
        });
    };
    expect(has(0, 600), "The companion score measure was dropped");
    // Finale's own re-layout differs from the score measure, so it stays visible: the reader has
    // no record stating those values and never will.
    expect(has(3, 575), "A re-laid-out part measure was dropped");
    // A part the reader does have is never dropped, whatever it holds.
    expect(has(2, 600), "A part measure the reader also has was dropped");
    expect(source.at("measures").asArray().size() == 2, "The source side was modified");
}

TEST_CASE("A measure record at comparator zero is not imported")
{
    // Mild corruption Finale tolerates and discards on upgrade: a zero-comparator record with a
    // zero width and time signature. musxdom numbers measures from 1, so building one would put an
    // object where there is no place for it and make every real measure fail the sequence check.
    const auto parsed = makeContainer(
        {{0, "MS", {0, 0, 0, 0, 0x0400, 0}},
            {0, "MS", {0, 0, static_cast<std::int16_t>(0x00f0), 0, 0, 0}},
            {1, "MS", {600, 0, 4, 1024, 0x0004, 0x0010}},
            {1, "MS", {4, 1024, static_cast<std::int16_t>(0x00f0), 0, 0, 0}}},
        FormatEpoch::DclLegacy);
    const auto document = emptyMeasureDocument();
    const auto report = measureImport(parsed, SourceProfile(FormatEpoch::DclLegacy), document);

    const auto measures = document->getOthers()->getArray<Measure>(musx::dom::SCORE_PARTID);
    expect(measures.size() == 1 && measures.front()->getCmper() == 1,
        "The comparator-zero record was imported as a measure");
    expect(reportedFieldCount(report) == measureFieldManifestSize,
        "The skipped record still reported fields");
    expect(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
               [](const auto& d) {
                   return d.level == musx::util::Logger::LogLevel::Verbose
                       && d.message.find("comparator 0") != std::string::npos;
               }),
        "The skipped record was not reported as a diagnostic");
}

TEST_CASE("A document with no measure records builds no measures")
{
    const auto parsed = makeContainer({{1, "LA", {0, 0, 0, 0, 0, 0}}},
        FormatEpoch::UncompressedLegacy);
    const auto document = emptyMeasureDocument();
    const auto report = measureImport(parsed, SourceProfile(FormatEpoch::UncompressedLegacy),
        document);
    expect(document->getOthers()->getArray<Measure>(musx::dom::SCORE_PARTID).empty(),
        "A document with no measure records fabricated one");
    expect(reportedFieldCount(report) == 0, "An absent measure family reported fields");
}

TEST_CASE("A truncated measure record decodes the words it has")
{
    // One row short of the twelve-word layout the rest of the document uses. The reader reads what
    // is there and leaves the missing words at zero rather than reading past the payload.
    const auto parsed = makeContainer(
        {{1, "MS", {360, 0, 4, 1024, 0x0006, 0x0010}},
            {1, "MS", {2, 2048, static_cast<std::int16_t>(0x00f6), 11, 12, -22}},
            {2, "MS", {480, 0, 3, 1024, 0x0004, 0x0010}}},
        FormatEpoch::DclLegacy);
    const auto document = emptyMeasureDocument();
    const auto report = measureImport(parsed, SourceProfile(FormatEpoch::DclLegacy), document);
    const auto truncated = document->getOthers()->get<Measure>(musx::dom::SCORE_PARTID, 2);
    expect(truncated != nullptr, "The short measure was not constructed");
    expect(truncated->width == 480 && truncated->beats == 3,
        "The short measure lost the words it does have");
    expect(truncated->dispBeats == 0 && truncated->frontSpaceExtra == 0,
        "The short measure invented values for words it does not have");
    expect(truncated->leftBarlineType == BarlineType::None,
        "The short measure read a left barline past the end of its payload");
    const auto truncatedKey = finale_mus_reader::instanceKey<Measure>(
        musx::dom::Cmper(0), musx::dom::Cmper(2));
    expect(report.fields.at(truncatedKey).size() == measureFieldManifestSize,
        "The short measure's report does not exhaust the Measure field manifest");
}

// The measure rules live in their own surveyor translation unit, so the registry is how a test
// reaches them.
std::optional<finale_mus_reader::coverage::DifferenceClassification> classifyMeasureDifference(
    const finale_mus_reader::coverage::DifferenceContext& context)
{
    const auto classify = finale_mus_reader::coverage::differenceClassifier("measures");
    return classify ? classify(context) : std::nullopt;
}

TEST_CASE("Deferred measure recovery classifies both directions of a tabled member")
{
    using namespace finale_mus_reader::coverage;

    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);
    const Value no(false);
    const Value yes(true);
    const ComparisonLeaves leaves;
    const auto context = [&](std::string_view path, std::string_view origin, const Value& source,
                             const Value& companion) {
        return DifferenceContext{path, DifferenceCategory::Differs, origin, source, companion,
            leaves, leaves, finale_mus_reader::FormatEpoch::CodaBanner,
            finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
    };
    constexpr std::string_view smartShape = "measures[cmper=1].has_smart_shape";

    // The companion states a smart shape the reader cannot yet build, so the
    // value is owed.
    REQUIRE(classifyMeasureDifference(context(smartShape, "legacy-mus", no, yes)) ==
            DifferenceClassification::AwaitsDependentRecovery);

    // The other direction waits on the same class: with no shapes recovered, the
    // reader cannot know the source bit is stale, so it can neither keep nor
    // clear it on evidence.
    REQUIRE(classifyMeasureDifference(context(smartShape, "legacy-mus", yes, no)) ==
            DifferenceClassification::AwaitsDependentRecovery);

    // The rest of the "something is attached here" flags are tabled on the same
    // terms, each waiting on the class that holds the objects Finale 27
    // recomputes it from.
    for (const auto* member : {"has_expression", "has_text_block", "has_ossia"}) {
        const auto path = std::string("measures[cmper=1].") + member;
        REQUIRE(classifyMeasureDifference(context(path, "legacy-mus", no, yes)) ==
                DifferenceClassification::AwaitsDependentRecovery);
        REQUIRE(classifyMeasureDifference(context(path, "legacy-mus", yes, no)) ==
                DifferenceClassification::AwaitsDependentRecovery);
    }
    // The chord flag is tabled under both provenances it can report. The
    // twelve-word layout carries its bit and reports a stored value; the six-word
    // layout has no word for it.
    for (const auto* origin : {"legacy-mus", "legacy-behavior"}) {
        REQUIRE(classifyMeasureDifference(context("measures[cmper=1].has_chord", origin, no,
                    yes)) == DifferenceClassification::AwaitsDependentRecovery);
    }
    // A provenance neither layout produces is still not this rule's to absorb.
    REQUIRE_FALSE(
        classifyMeasureDifference(context("measures[cmper=1].has_chord", "unmapped", no, yes)));

    // A member that is not a presence flag is not deferred, in either direction.
    REQUIRE_FALSE(classifyMeasureDifference(
        context("measures[cmper=1].begin_new_system", "legacy-mus", no, yes)));
    REQUIRE_FALSE(classifyMeasureDifference(
        context("measures[cmper=1].begin_new_system", "legacy-mus", yes, no)));
    // Nor is the same member on another class.
    REQUIRE_FALSE(classifyMeasureDifference(
        context("staves[cmper=1].has_smart_shape", "legacy-mus", no, yes)));
    // Nor a value with different provenance than the rule names.
    REQUIRE_FALSE(classifyMeasureDifference(context(smartShape, "legacy-behavior", no, yes)));

    // The whole deferral backs out through one switch, which is what
    // --strict-deferred sets.
    setDeferredRecoveryClassified(false);
    REQUIRE_FALSE(classifyMeasureDifference(context(smartShape, "legacy-mus", no, yes)));
    REQUIRE_FALSE(classifyMeasureDifference(context(smartShape, "legacy-mus", yes, no)));
    setDeferredRecoveryClassified(true);
    REQUIRE(classifyMeasureDifference(context(smartShape, "legacy-mus", no, yes)) ==
            DifferenceClassification::AwaitsDependentRecovery);
}

TEST_CASE("A later beta's back-save explains a key-signature switch the format "
          "cannot carry")
{
    using namespace finale_mus_reader::coverage;

    // A Finale 2012 file whose creator was a Finale 2014 beta: the layout on disk
    // is 17, the release that made the document is 18, and its development status
    // is beta.
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);
    const auto creator = [](std::uint8_t major, std::uint8_t devStatus) {
        finale_mus_reader::SourceVersion version;
        version.major = major;
        version.devStatus = devStatus;
        return version;
    };
    report.creatorVersion = creator(18, 2);

    const Value no(false);
    const Value yes(true);
    const ComparisonLeaves leaves;
    const auto context = [&](std::string_view path) {
        return DifferenceContext{path, DifferenceCategory::Differs, "legacy-behavior", no, yes,
            leaves, leaves, finale_mus_reader::FormatEpoch::ZlibLegacy,
            finale_mus_reader::ByteOrder::LittleEndian, nullptr, report};
    };
    constexpr std::string_view hideAccis =
        "measures[cmper=1].global_key_sig.hide_key_sig_show_accis";
    constexpr std::string_view keyless = "measures[cmper=1].global_key_sig.keyless";

    REQUIRE(
        classifyMeasureDifference(context(hideAccis)) == DifferenceClassification::BetaDiscrepancy);
    REQUIRE(
        classifyMeasureDifference(context(keyless)) == DifferenceClassification::BetaDiscrepancy);

    // Both halves of the condition are load-bearing. A release build of the same
    // later version wrote what it wrote, so a difference there is not explained
    // away.
    report.creatorVersion = creator(18, 4);
    REQUIRE_FALSE(classifyMeasureDifference(context(hideAccis)));
    // Neither is a beta of the release that owns the format: it had no such
    // member to lose.
    report.creatorVersion = creator(17, 2);
    REQUIRE_FALSE(classifyMeasureDifference(context(hideAccis)));
    // A file with no creator tuple says nothing about what made it.
    report.creatorVersion.reset();
    REQUIRE_FALSE(classifyMeasureDifference(context(hideAccis)));

    // The rule reaches these two members and no others.
    report.creatorVersion = creator(18, 2);
    REQUIRE_FALSE(classifyMeasureDifference(context("measures[cmper=1].global_key_sig.key")));
    REQUIRE_FALSE(classifyMeasureDifference(context("measures[cmper=1].width")));
}

TEST_CASE("Finale's musx conversion writes a word-extension break the source "
          "does not carry")
{
    using namespace finale_mus_reader::coverage;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::DclLegacy);
    const Value no(false);
    const Value yes(true);
    constexpr std::string_view path = "measures[cmper=1].break_word_ext";

    // The sibling leaves the rule reads: the measure's barline and its backwards
    // repeat.
    const auto leaves = [](std::int64_t barline, bool repeat) {
        return ComparisonLeaves{{"measures[cmper=1].barline_type", {Value(barline), "legacy-mus"}},
            {"measures[cmper=1].backwards_repeat_bar", {Value(repeat), "legacy-mus"}}};
    };
    const auto classify = [&](const ComparisonLeaves& side, const Value& source,
                              const Value& companion) {
        const DifferenceContext context{path, DifferenceCategory::Differs, "legacy-mus", source,
            companion, side, side, finale_mus_reader::FormatEpoch::DclLegacy,
            finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
        return classifyMeasureDifference(context);
    };

    // Double, final and solid barlines, and a backwards repeat, are what Finale
    // ties it to.
    for (const std::int64_t barline : {3, 4, 5}) {
        REQUIRE(classify(leaves(barline, false), no, yes) ==
                DifferenceClassification::FinaleUpgradeLoss);
    }
    REQUIRE(classify(leaves(2, true), no, yes) == DifferenceClassification::FinaleUpgradeLoss);

    // A measure with none of them is not explained by this rule and stays
    // unexpected.
    REQUIRE_FALSE(classify(leaves(2, false), no, yes));
    REQUIRE_FALSE(classify(leaves(0, false), no, yes));
    // Nor is the other direction: a stored bit the companion drops would be a
    // real loss.
    REQUIRE_FALSE(classify(leaves(4, false), yes, no));
}

TEST_CASE("A composite time-signature word is deferred, a plain one is not")
{
    using namespace finale_mus_reader::coverage;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);

    const auto leaves = [](bool numerator, bool denominator) {
        return ComparisonLeaves{
            {"measures[cmper=1].composite_numerator", {Value(numerator), "legacy-mus"}},
            {"measures[cmper=1].composite_denominator", {Value(denominator), "legacy-mus"}}};
    };
    const auto classify = [&](std::string_view path, const ComparisonLeaves& side,
                              std::int64_t source, std::int64_t companion) {
        const Value a(source), b(companion);
        const DifferenceContext context{path, DifferenceCategory::Differs, "legacy-mus", a, b, side,
            side, finale_mus_reader::FormatEpoch::CodaBanner,
            finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
        return classifyMeasureDifference(context);
    };
    constexpr std::string_view beats = "measures[cmper=1].beats";
    constexpr std::string_view divBeat = "measures[cmper=1].div_beat";

    // Each word is deferred only when its own flag says it is a list comparator.
    REQUIRE(classify(beats, leaves(true, false), 1, 2) ==
            DifferenceClassification::AwaitsDependentRecovery);
    REQUIRE(classify(divBeat, leaves(false, true), 1, 2) ==
            DifferenceClassification::AwaitsDependentRecovery);

    // A plain beat count or Edu value that disagrees is a decoding failure, not a
    // renumbering.
    REQUIRE_FALSE(classify(beats, leaves(false, false), 1, 2));
    REQUIRE_FALSE(classify(divBeat, leaves(false, false), 1, 2));
    // And each flag governs only its own word.
    REQUIRE_FALSE(classify(beats, leaves(false, true), 1, 2));
    REQUIRE_FALSE(classify(divBeat, leaves(true, false), 1, 2));
}

} // namespace
} // namespace finale_mus_reader_tests
