// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"
#include "coverage/classification_rules.h"
#include "coverage/comparison_text.h"

#include <set>

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

using PartDefinition = musx::dom::others::PartDefinition;

// The score record every Finale 2012 document writes: name block 1, first in the list, one
// printed copy, no SmartMusic instrument and no default name source.
const std::vector<std::int16_t> scorePartWords{1, 0, 1, 0x0001, 0, 0};

// A linked part naming staff 7 as its default name, extracted and format-applied.
const std::vector<std::int16_t> staffNamedPartWords{0, 3, 2, 0x0106, -1, 7};

// A Finale 2011 linked part that has never been extracted: the extraction bit is clear, and the
// members that track being a linked part are set all the same.
const std::vector<std::int16_t> unextractedPartWords{0, 5, 1, 0x0108, 0, 0};

// The same part naming staff group 12 instead, which the record spells as a negated comparator.
const std::vector<std::int16_t> groupNamedPartWords{0, 3, 2, 0x0106, -1, -12};

/// @brief Runs the part-definition importer over one synthesized container.
ImportReport partDefinitionImport(const finale_mus_reader::container::ParsedContainer& parsed,
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
    finale_mus_reader::others::importPartDefinitions(context);
    return report;
}

SourceProfile zlibProfile(ByteOrder byteOrder, unsigned major)
{
    SourceProfile profile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = byteOrder;
    profile.version = SourceVersion{.major = static_cast<std::uint8_t>(major)};
    return profile;
}

TEST_CASE("Part definitions recover every stored field in both byte orders")
{
    for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        const auto era = byteOrder == ByteOrder::BigEndian ? " (big-endian)" : " (little-endian)";
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        const auto parsed = makeClassContainer(
            {SyntheticClassRow{0x011a, scorePartWords, 0},
                SyntheticClassRow{0x011a, staffNamedPartWords, 1},
                SyntheticClassRow{0x011a, groupNamedPartWords, 2}},
            byteOrder);
        const auto report = partDefinitionImport(parsed, zlibProfile(byteOrder, 17), document);

        const auto score = document->getOthers()->get<PartDefinition>(musx::dom::SCORE_PARTID, 0);
        expect(score != nullptr, std::string("The score part was not built") + era);
        expect(score->nameId == 1 && score->partOrder == 0 && score->copies == 1,
            std::string("The score part's stored words were not recovered") + era);
        expect(score->printPart && !score->extractPart && !score->applyFormat,
            std::string("The score part's flag word was not unpacked") + era);
        expect(score->smartMusicInst == 0 && score->defaultNameStaff == 0
                && score->defaultNameGroup == 0,
            std::string("The score part invented a value its record leaves clear") + era);

        const auto staffNamed =
            document->getOthers()->get<PartDefinition>(musx::dom::SCORE_PARTID, 1);
        expect(staffNamed != nullptr, std::string("The staff-named part was not built") + era);
        expect(staffNamed->partOrder == 3 && staffNamed->copies == 2
                && staffNamed->nameId == 0,
            std::string("A linked part's stored words were not recovered") + era);
        expect(!staffNamed->printPart && staffNamed->extractPart && staffNamed->applyFormat,
            std::string("A linked part's flag word was not unpacked") + era);
        expect(staffNamed->smartMusicInst == -1,
            std::string("A Finale 2012 SmartMusic instrument was not read") + era);
        expect(staffNamed->defaultNameStaff == 7 && staffNamed->defaultNameGroup == 0,
            std::string("A positive default-name word did not select the staff") + era);

        const auto groupNamed =
            document->getOthers()->get<PartDefinition>(musx::dom::SCORE_PARTID, 2);
        expect(groupNamed && groupNamed->defaultNameGroup == 12
                && groupNamed->defaultNameStaff == 0,
            std::string("A negative default-name word did not select the staff group") + era);

        for (const auto* member : {"nameId", "partOrder", "copies", "printPart", "extractPart",
                 "applyFormat", "smartMusicInst", "defaultNameStaff", "defaultNameGroup"}) {
            expect(field(report, std::string("others.partDef[1].") + member).origin
                    == ValueOrigin::LegacyMus,
                std::string("A recovered part definition member was not reported as legacy MUS"
                    " data: ") + member + era);
        }
        expect(staffNamed->needsRecalc && staffNamed->useAsSmpInst,
            std::string("A linked part did not take the era's derived members") + era);
        expect(!score->needsRecalc && !score->useAsSmpInst,
            std::string("The score took a member only an extracted part has") + era);
        for (const auto* member : {"needsRecalc", "useAsSmpInst"}) {
            expect(field(report, std::string("others.partDef[1].") + member).origin
                    == ValueOrigin::LegacyBehavior,
                std::string("A derived member was not reported as legacy behavior: ")
                    + member + era);
        }
        expect(field(report, "others.partDef[1].unlinkInsts").origin == ValueOrigin::Unmapped,
            std::string("The member with no located source was not reported unmapped") + era);
        // Twelve persisted members on each of three instances, and nothing else.
        expect(reportedFieldCount(report) == 36,
            std::string("The part definition report does not exhaust the musxdom field manifest")
                + era);
    }
}

TEST_CASE("A linked part carries the derived members whether or not it is extracted")
{
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto parsed = makeClassContainer(
        {SyntheticClassRow{0x011a, scorePartWords, 0},
            SyntheticClassRow{0x011a, unextractedPartWords, 6}},
        ByteOrder::BigEndian);
    const auto report =
        partDefinitionImport(parsed, zlibProfile(ByteOrder::BigEndian, 16), document);

    const auto part = document->getOthers()->get<PartDefinition>(musx::dom::SCORE_PARTID, 6);
    expect(part && !part->extractPart,
        "The unextracted part read an extraction bit its record leaves clear");
    expect(part->needsRecalc && part->useAsSmpInst,
        "A linked part lost the derived members because it was never extracted");
    expect(field(report, "others.partDef[6].needsRecalc").origin == ValueOrigin::LegacyBehavior,
        "A derived member was not reported as legacy behavior");
    // Finale 2011 stores the instrument, and stores it as absent.
    expect(part->smartMusicInst == 0,
        "A Finale 2011 part did not read its stored SmartMusic instrument");
    expect(field(report, "others.partDef[6].smartMusicInst").origin == ValueOrigin::LegacyMus,
        "A stored SmartMusic instrument was not reported as legacy MUS data");
}

TEST_CASE("Part definitions supply the SmartMusic instrument no release before Finale 2011 has")
{
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    // The same records a Finale 2012 file would carry, except that a Finale 2008 file leaves the
    // instrument word clear for its parts as well as for its score.
    const std::vector<std::int16_t> earlyPartWords{0, 3, 2, 0x0006, 0, 7};
    const auto parsed = makeClassContainer(
        {SyntheticClassRow{0x011a, scorePartWords, 0},
            SyntheticClassRow{0x011a, earlyPartWords, 1}},
        ByteOrder::BigEndian);
    const auto report =
        partDefinitionImport(parsed, zlibProfile(ByteOrder::BigEndian, 13), document);

    const auto score = document->getOthers()->get<PartDefinition>(musx::dom::SCORE_PARTID, 0);
    const auto part = document->getOthers()->get<PartDefinition>(musx::dom::SCORE_PARTID, 1);
    expect(score && score->smartMusicInst == 0,
        "The score acquired a SmartMusic instrument it never had");
    expect(part && part->smartMusicInst == -1,
        "A pre-Finale-2011 part did not take its era's absent SmartMusic instrument");
    expect(field(report, "others.partDef[1].smartMusicInst").origin
            == ValueOrigin::LegacyBehavior,
        "The era's SmartMusic instrument was not reported as legacy behavior");
    expect(part->defaultNameStaff == 7,
        "The version gate reached a field that does not depend on it");
}

TEST_CASE("Every era acquires the score part definition it has no record for")
{
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        const auto era = " in epoch " + std::to_string(static_cast<int>(epoch));
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        SourceProfile profile(epoch);
        profile.byteOrder = ByteOrder::BigEndian;
        // A record family the era does write, so the pool is not empty for the wrong reason.
        const auto parsed = epoch == FormatEpoch::ZlibLegacy
            ? makeClassContainer(0x00a3, {0, 0, 0, 0, 0, 0}, ByteOrder::BigEndian, 0)
            : makeContainer({SyntheticRow{0, "LA", {}}}, epoch);
        const auto report = partDefinitionImport(parsed, profile, document);

        const auto parts =
            document->getOthers()->getArray<PartDefinition>(musx::dom::SCORE_PARTID);
        expect(parts.size() == 1, "A source with no part definition built more than the score"
            + era);
        const auto score = parts.front();
        expect(score->getCmper() == musx::dom::SCORE_PARTID && score->isScore(),
            "The synthesized part is not the score" + era);
        expect(score->partOrder == 0 && score->copies == 1 && score->printPart,
            "The score part did not take its era's behavior" + era);
        expect(score->nameId == 0,
            "The score part named a text block the source never stored" + era);
        expect(!score->extractPart && !score->applyFormat && score->smartMusicInst == 0
                && score->defaultNameStaff == 0 && score->defaultNameGroup == 0,
            "The score part carries a linked-part value it cannot have" + era);
        for (const auto* member : {"nameId", "partOrder", "copies", "printPart", "extractPart",
                 "applyFormat", "needsRecalc", "useAsSmpInst", "smartMusicInst",
                 "defaultNameStaff", "defaultNameGroup"}) {
            expect(field(report, std::string("others.partDef[0].") + member).origin
                    == ValueOrigin::LegacyBehavior,
                std::string("The era's own value was not reported as legacy behavior: ")
                    + member + era);
        }
        expect(!score->needsRecalc && !score->useAsSmpInst && !score->unlinkInsts,
            "The score part carries a linked-part member it cannot have" + era);
        expect(field(report, "others.partDef[0].unlinkInsts").origin == ValueOrigin::Unmapped,
            "The member no known record supplies was not reported unmapped" + era);
        expect(reportedFieldCount(report) == 12,
            "The synthesized score part does not exhaust the musxdom field manifest" + era);
    }
}

TEST_CASE("A truncated part definition builds nothing and leaves the score part intact")
{
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto parsed = makeClassContainer(
        {SyntheticClassRow{0x011a, {1, 0, 1}, 1}}, ByteOrder::BigEndian);
    const auto report =
        partDefinitionImport(parsed, zlibProfile(ByteOrder::BigEndian, 17), document);
    const auto parts = document->getOthers()->getArray<PartDefinition>(musx::dom::SCORE_PARTID);
    expect(parts.size() == 1 && parts.front()->isScore(),
        "A short record built a part definition out of bytes it does not have");
    expect(!report.diagnostics.empty(),
        "A short part definition record was discarded without saying so");
}

TEST_CASE("A part-owned part definition keeps its own identity and share mode")
{
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto parsed = makeClassContainer(
        {SyntheticClassRow{0x011a, scorePartWords, 0},
            SyntheticClassRow{0x011a, staffNamedPartWords, 1, /*partId*/ 4}},
        ByteOrder::BigEndian);
    partDefinitionImport(parsed, zlibProfile(ByteOrder::BigEndian, 17), document);
    const auto sources = document->getOthers()->getAllSources<PartDefinition>(1);
    expect(sources.size() == 1, "The part-owned part definition was not built");
    expect(sources.front()->getSourcePartId() == 4,
        "The part-owned part definition lost the part that owns it");
    expect(sources.front()->getShareMode() == musx::dom::EnigmaBase::ShareMode::None,
        "A standalone part record was not read as unshared");
}

TEST_CASE("The synthesized score name is the only part-definition difference "
          "classified",
    "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using finale_mus_reader::FormatEpoch;
    finale_mus_reader::ImportReport report(FormatEpoch::UncompressedLegacy);
    const ComparisonLeaves none;
    const auto classify = [&](std::string_view path, FormatEpoch epoch, std::string_view origin,
                              const Value& source, const Value& companion) {
        const DifferenceContext context{path, DifferenceCategory::Differs, origin, source,
            companion, none, none, epoch, finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
        return classifyPartDefinitionDifference(context);
    };
    const Value zero(std::int64_t{0});
    const Value block(std::int64_t{28});
    constexpr std::string_view scoreName = "part_defs[cmper=0].name_id";

    REQUIRE(classify(scoreName, FormatEpoch::UncompressedLegacy, "legacy-behavior", zero, block) ==
            DifferenceClassification::SynthesizedScoreName);
    REQUIRE(classify(scoreName, FormatEpoch::CodaBanner, "legacy-behavior", zero, block) ==
            DifferenceClassification::SynthesizedScoreName);
    REQUIRE(classify(scoreName, FormatEpoch::DclLegacy, "legacy-behavior", zero, block) ==
            DifferenceClassification::SynthesizedScoreName);

    // Every condition is load-bearing: none of these may be swallowed by the
    // rule.
    SECTION("the epoch that stores the member is never classified")
    {
        REQUIRE_FALSE(classify(scoreName, FormatEpoch::ZlibLegacy, "legacy-behavior", zero, block));
    }
    SECTION("a recovered value that disagrees is never classified")
    {
        REQUIRE_FALSE(classify(
            scoreName, FormatEpoch::ZlibLegacy, "legacy-mus", block, Value(std::int64_t{29})));
        REQUIRE_FALSE(classify(scoreName, FormatEpoch::UncompressedLegacy, "legacy-mus", block,
            Value(std::int64_t{29})));
    }
    SECTION("a linked part is never classified")
    {
        REQUIRE_FALSE(classify("part_defs[cmper=1].name_id", FormatEpoch::UncompressedLegacy,
            "legacy-behavior", zero, block));
    }
    SECTION("another member of the score part is never classified")
    {
        REQUIRE_FALSE(classify("part_defs[cmper=0].copies", FormatEpoch::UncompressedLegacy,
            "legacy-behavior", zero, block));
    }
    SECTION("a non-null reader value is never classified")
    {
        REQUIRE_FALSE(classify(scoreName, FormatEpoch::UncompressedLegacy, "legacy-behavior", block,
            Value(std::int64_t{29})));
    }
    SECTION("a companion that supplies no name is never classified")
    {
        REQUIRE_FALSE(
            classify(scoreName, FormatEpoch::UncompressedLegacy, "legacy-behavior", zero, zero));
    }

    SECTION("an unreachable member the companion sets is possibly unrecoverable")
    {
        constexpr std::string_view unlink = "part_defs[cmper=14].unlink_insts";
        REQUIRE(classify(unlink, FormatEpoch::ZlibLegacy, "unmapped", Value(false), Value(true)) ==
                DifferenceClassification::PossiblyUnrecoverable);
        // A recovered value that disagrees is a real defect and must stay
        // unexpected, as must a reader value the companion does not share the
        // direction of.
        REQUIRE_FALSE(
            classify(unlink, FormatEpoch::ZlibLegacy, "legacy-mus", Value(false), Value(true)));
        REQUIRE_FALSE(
            classify(unlink, FormatEpoch::ZlibLegacy, "unmapped", Value(true), Value(false)));
    }
}

TEST_CASE("A part name requires the reader to name the text, and the synthesis "
          "does not",
    "[coverage]")
{
    using finale_mus_reader::coverage::partNameTextMatches;
    using finale_mus_reader::coverage::synthesizedScoreNameText;
    const std::set<std::int64_t> none;
    const std::set<std::int64_t> names337{337};
    const std::set<std::int64_t> names12{12};

    // Both sides naming a text must agree on which, so an unrelated block is not
    // swept in.
    REQUIRE(partNameTextMatches(names337, names337, 337));
    REQUIRE_FALSE(partNameTextMatches(names337, names12, 337));
    REQUIRE_FALSE(partNameTextMatches(names12, names337, 337));
    // A companion that names none defers to the reader.
    REQUIRE(partNameTextMatches(names337, none, 337));

    // A text only the companion names is not a part name the reader got wrong. It
    // is the score name Finale synthesized, and it is classified as that instead.
    REQUIRE_FALSE(partNameTextMatches(none, names337, 337));
    REQUIRE(synthesizedScoreNameText(none, names337, 337));
    REQUIRE_FALSE(synthesizedScoreNameText(none, names337, 12));

    // The two are mutually exclusive, and neither fires when nothing names the
    // text.
    REQUIRE_FALSE(synthesizedScoreNameText(names337, names337, 337));
    REQUIRE_FALSE(synthesizedScoreNameText(names12, names337, 337));
    REQUIRE_FALSE(partNameTextMatches(none, none, 337));
    REQUIRE_FALSE(synthesizedScoreNameText(none, none, 337));
}

} // namespace
} // namespace finale_mus_reader_tests
