// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <string>

#include "musx/musx.h"

#include "coverage/schema.h"

#include "coverage/common/byte_swap.h"
#include "coverage/common/font_info.h"
#include "coverage/registry.h"

namespace finale_mus_reader_tests {
namespace {

TEST_CASE("Coverage recognizes direct and nested font references", "[coverage]")
{
    using finale_mus_reader::coverage::isComparisonFontReference;
    const std::set<std::string> dynamicReferences;

    REQUIRE(isComparisonFontReference(
        "chord_suffix_elements[cmper=1,inci=0].font.font_id", dynamicReferences));
    REQUIRE(isComparisonFontReference("ss_line_styles[cmper=1].char_font_id", dynamicReferences));
    REQUIRE_FALSE(isComparisonFontReference(
        "font_definitions.definitions[normalized_name=maestro].cmper", dynamicReferences));
}

TEST_CASE("Coverage source instances enumerate physical parts without score "
          "fallbacks",
    "[coverage]")
{
    using Target = musx::dom::others::FretboardStyle;
    using ShareMode = musx::dom::EnigmaBase::ShareMode;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto score = std::make_shared<Target>(
        document, musx::dom::SCORE_PARTID, ShareMode::All, musx::dom::Cmper{1});
    auto scoreOnly = std::make_shared<Target>(
        document, musx::dom::SCORE_PARTID, ShareMode::All, musx::dom::Cmper{2});
    auto part = std::make_shared<Target>(
        document, musx::dom::Cmper{2}, ShareMode::Partial, musx::dom::Cmper{1});
    document->getOthers()->add(Target::XmlNodeName, std::move(score));
    document->getOthers()->add(Target::XmlNodeName, std::move(scoreOnly));
    document->getOthers()->add(Target::XmlNodeName, std::move(part));

    finale_mus_reader::ImportReport emptyReport(finale_mus_reader::FormatEpoch::ZlibLegacy);
    const finale_mus_reader::coverage::SurveyContext context{document, emptyReport};
    const auto instances = finale_mus_reader::coverage::sourceInstances<Target>(context);
    REQUIRE(instances.size() == 3);
    REQUIRE(instances[0]->getSourcePartId() == musx::dom::SCORE_PARTID);
    REQUIRE(instances[1]->getSourcePartId() == musx::dom::SCORE_PARTID);
    REQUIRE(instances[2]->getSourcePartId() == 2);
    REQUIRE(instances[2]->getCmper() == 1);
}

TEST_CASE("Coverage fields always materialize recorded provenance", "[coverage]")
{
    using FontDefinition = musx::dom::others::FontDefinition;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::UncompressedLegacy);
    const finale_mus_reader::coverage::SurveyContext context{document, report};

    report.setField(finale_mus_reader::instanceKey<FontDefinition>(
                        musx::dom::SCORE_PARTID, musx::dom::Cmper(0)),
        "name", {finale_mus_reader::ValueOrigin::LegacyMus, 0, 0, 0});
    REQUIRE(finale_mus_reader::coverage::fieldOrigin<FontDefinition>(
                context, "name", musx::dom::Cmper(0)) == "legacy-mus");

    const auto defaultInstance = finale_mus_reader::instanceKey<FontDefinition>(
        musx::dom::SCORE_PARTID, musx::dom::Cmper(1));
    REQUIRE(finale_mus_reader::coverage::fieldOrigin<FontDefinition>(
                context, "charsetBank", defaultInstance) == "finale27-default");
    REQUIRE(report.findField(defaultInstance, "charsetBank"));

    report.setField(defaultInstance, "family", {finale_mus_reader::ValueOrigin::Unmapped, 0, 0, 0});
    REQUIRE(finale_mus_reader::coverage::fieldOrigin<FontDefinition>(
                context, "family", defaultInstance) == "unmapped");

    const auto recoveredInstance = finale_mus_reader::instanceKey<FontDefinition>(
        musx::dom::SCORE_PARTID, musx::dom::Cmper(2));
    report.setInstanceOrigin(recoveredInstance, finale_mus_reader::ValueOrigin::LegacyMus);
    REQUIRE(finale_mus_reader::coverage::fieldOrigin<FontDefinition>(
                context, "pitch", recoveredInstance) == "legacy-mus");
    REQUIRE(report.findField(recoveredInstance, "pitch"));
}
TEST_CASE("A contiguous aligned byte-swapped string span is recognized", "[coverage]")
{
    using finale_mus_reader::coverage::hasContiguousAdjacentByteSwap;
    constexpr std::string_view source = "Major with root on 5th string";
    constexpr std::string_view converted = "Major with roo tno5 hts tring";

    REQUIRE(hasContiguousAdjacentByteSwap(source, converted));
    REQUIRE_FALSE(hasContiguousAdjacentByteSwap(source, "Major with roo tno5 hts strong"));
    REQUIRE_FALSE(hasContiguousAdjacentByteSwap(source, "Major with root on 5th string!"));
}
} // namespace
} // namespace finale_mus_reader_tests
