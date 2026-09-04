// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>

#include "musx/musx.h"

#include "coverage/schema.h"

#include "coverage/common/font_info.h"
#include "coverage/common/music_symbol_info.h"
#include "coverage/common/note_rest_info.h"
#include "coverage/common/page_format_info.h"
#include "coverage/common/part_definition_info.h"
#include "coverage/common/part_name_text.h"
#include "coverage/common/tie_options_info.h"

namespace finale_mus_reader_tests {
namespace {

TEST_CASE("Coverage source instances enumerate physical parts without score fallbacks",
    "[coverage]")
{
    using Target = musx::dom::others::FretboardStyle;
    using ShareMode = musx::dom::EnigmaBase::ShareMode;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto score = std::make_shared<Target>(document, musx::dom::SCORE_PARTID,
        ShareMode::All, musx::dom::Cmper{1});
    auto scoreOnly = std::make_shared<Target>(document, musx::dom::SCORE_PARTID,
        ShareMode::All, musx::dom::Cmper{2});
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
    finale_mus_reader::ImportReport report(
        finale_mus_reader::FormatEpoch::UncompressedLegacy);
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

    report.setField(defaultInstance, "family",
        {finale_mus_reader::ValueOrigin::Unmapped, 0, 0, 0});
    REQUIRE(finale_mus_reader::coverage::fieldOrigin<FontDefinition>(
                context, "family", defaultInstance) == "unmapped");

    const auto recoveredInstance = finale_mus_reader::instanceKey<FontDefinition>(
        musx::dom::SCORE_PARTID, musx::dom::Cmper(2));
    report.setInstanceOrigin(
        recoveredInstance, finale_mus_reader::ValueOrigin::LegacyMus);
    REQUIRE(finale_mus_reader::coverage::fieldOrigin<FontDefinition>(
                context, "pitch", recoveredInstance) == "legacy-mus");
    REQUIRE(report.findField(recoveredInstance, "pitch"));
}

TEST_CASE("Font-definition symbol charsets are platform-equivalent", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using FontDefinition = musx::dom::others::FontDefinition;
    using Bank = FontDefinition::CharacterSetBank;
    constexpr std::string_view prefix =
        "font_definitions.definitions[normalized_name=maestro].";
    const auto bankPath = std::string(prefix) + "charset_bank";
    const auto valuePath = std::string(prefix) + "charset_val";
    auto symbolPath = std::string(prefix);
    symbolPath.append(fontDefinitionIsSymbolField);
    REQUIRE(isClassifierMetadataPath(symbolPath));
    REQUIRE_FALSE(isClassifierMetadataPath(bankPath));
    const auto makeLeaves = [&bankPath, &valuePath, &symbolPath](Bank bank, int charset) {
        return ComparisonLeaves{
            {bankPath,
                {Value(static_cast<std::int64_t>(bank)), "legacy-mus"}},
            {valuePath, {Value(charset), "legacy-mus"}},
            {symbolPath, {Value(true), "legacy-mus"}}};
    };
    const auto classify = classifyFontDefinitionDifference;
    finale_mus_reader::ImportReport report(
        finale_mus_reader::FormatEpoch::UncompressedLegacy);

    for (const bool reverse : {false, true}) {
        auto source = reverse
            ? makeLeaves(Bank::Windows, FontDefinition::SYMBOL_CHARSET_WIN)
            : makeLeaves(Bank::MacOS, FontDefinition::SYMBOL_CHARSET_MAC);
        auto companion = reverse
            ? makeLeaves(Bank::MacOS, FontDefinition::SYMBOL_CHARSET_MAC)
            : makeLeaves(Bank::Windows, FontDefinition::SYMBOL_CHARSET_WIN);
        for (const auto field : {"charset_bank", "charset_val"}) {
            const auto path = std::string(prefix) + field;
            const DifferenceContext context{path, DifferenceCategory::Differs, "legacy-mus",
                source.at(path).first, companion.at(path).first, source, companion,
                finale_mus_reader::FormatEpoch::UncompressedLegacy,
                finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
            REQUIRE(classify(context) == DifferenceClassification::SymbolFontEquivalence);
        }

        const auto pitchPath = std::string(prefix) + "pitch";
        const Value sourcePitch(0);
        const Value companionPitch(1);
        const DifferenceContext pitchContext{pitchPath, DifferenceCategory::Differs,
            "legacy-mus", sourcePitch, companionPitch, source, companion,
            finale_mus_reader::FormatEpoch::UncompressedLegacy,
            finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
        REQUIRE_FALSE(classify(pitchContext));
    }

    auto source = makeLeaves(Bank::MacOS, FontDefinition::SYMBOL_CHARSET_MAC);
    auto companion = makeLeaves(Bank::Windows, 0);
    companion.at(symbolPath).first = Value(false);
    const auto path = std::string(prefix) + "charset_val";
    const DifferenceContext context{path, DifferenceCategory::Differs, "legacy-mus",
        source.at(path).first, companion.at(path).first, source, companion,
        finale_mus_reader::FormatEpoch::UncompressedLegacy,
        finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
    REQUIRE_FALSE(classify(context));
}

TEST_CASE("Seeded font-definition pitch differences are expected across epochs", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    constexpr std::string_view pitchPath =
        "font_definitions.definitions[normalized_name=maestropercussion].pitch";
    const Value sourcePitch(2);
    const Value companionPitch(0);
    const ComparisonLeaves leaves;

    for (const auto epoch : {finale_mus_reader::FormatEpoch::CodaBanner,
             finale_mus_reader::FormatEpoch::UncompressedLegacy,
             finale_mus_reader::FormatEpoch::DclLegacy,
             finale_mus_reader::FormatEpoch::ZlibLegacy}) {
        finale_mus_reader::ImportReport report(epoch);
        const DifferenceContext context{pitchPath, DifferenceCategory::Differs,
            "finale27-default", sourcePitch, companionPitch, leaves, leaves, epoch,
            finale_mus_reader::ByteOrder::Unknown, nullptr, report};
        REQUIRE(classifyFontDefinitionDifference(context) ==
            DifferenceClassification::CharsetPitchDifference);
    }

    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);
    DifferenceContext context{pitchPath, DifferenceCategory::Differs, "legacy-mus", sourcePitch,
        companionPitch, leaves, leaves, finale_mus_reader::FormatEpoch::ZlibLegacy,
        finale_mus_reader::ByteOrder::Unknown, nullptr, report};
    REQUIRE_FALSE(classifyFontDefinitionDifference(context));

    context.origin = "finale27-default";
    context.path = "font_definitions.definitions[normalized_name=maestropercussion].family";
    REQUIRE_FALSE(classifyFontDefinitionDifference(context));

    context.path = pitchPath;
    context.category = DifferenceCategory::ReaderOnly;
    REQUIRE_FALSE(classifyFontDefinitionDifference(context));
}

TEST_CASE("Zero-charset font definitions permit platform shifts", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using FontDefinition = musx::dom::others::FontDefinition;
    using Bank = FontDefinition::CharacterSetBank;
    constexpr std::string_view prefix =
        "font_definitions.definitions[normalized_name=xt].";
    const auto bankPath = std::string(prefix) + "charset_bank";
    const auto valuePath = std::string(prefix) + "charset_val";
    const auto makeLeaves = [&bankPath, &valuePath](Bank bank, int charset) {
        return ComparisonLeaves{
            {bankPath, {Value(static_cast<std::int64_t>(bank)), "legacy-mus"}},
            {valuePath, {Value(charset), "legacy-mus"}}};
    };
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);

    for (const bool reverse : {false, true}) {
        auto source = makeLeaves(reverse ? Bank::Windows : Bank::MacOS, 0);
        auto companion = makeLeaves(reverse ? Bank::MacOS : Bank::Windows, 0);
        DifferenceContext context{bankPath, DifferenceCategory::Differs, "legacy-mus",
            source.at(bankPath).first, companion.at(bankPath).first, source, companion,
            finale_mus_reader::FormatEpoch::ZlibLegacy,
            finale_mus_reader::ByteOrder::LittleEndian, nullptr, report};
        REQUIRE(classifyFontDefinitionDifference(context) ==
            DifferenceClassification::FontPlatformShift);

        source.at(valuePath).first = Value(1);
        REQUIRE_FALSE(classifyFontDefinitionDifference(context));
        source.at(valuePath).first = Value(0);
        companion.at(valuePath).first = Value(1);
        REQUIRE_FALSE(classifyFontDefinitionDifference(context));
        companion.at(valuePath).first = Value(0);
        context.category = DifferenceCategory::ReaderOnly;
        REQUIRE_FALSE(classifyFontDefinitionDifference(context));
    }
}

TEST_CASE("Seeded Windows ANSI and default charsets are equivalent", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using FontDefinition = musx::dom::others::FontDefinition;
    constexpr std::string_view prefix =
        "font_definitions.definitions[normalized_name=timesnewroman].";
    const auto bankPath = std::string(prefix) + "charset_bank";
    const auto valuePath = std::string(prefix) + "charset_val";
    const auto makeLeaves = [&bankPath, &valuePath](FontDefinition::CharacterSetBank bank,
                                int charset) {
        return ComparisonLeaves{
            {bankPath, {Value(static_cast<std::int64_t>(bank)), "finale27-default"}},
            {valuePath, {Value(charset), "finale27-default"}}};
    };
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::ZlibLegacy);

    for (const bool reverse : {false, true}) {
        auto source = makeLeaves(FontDefinition::CharacterSetBank::Windows,
            reverse ? finale_mus_reader::text::windowsDefaultCharset
                    : finale_mus_reader::text::windowsAnsiCharset);
        auto companion = makeLeaves(FontDefinition::CharacterSetBank::Windows,
            reverse ? finale_mus_reader::text::windowsAnsiCharset
                    : finale_mus_reader::text::windowsDefaultCharset);
        DifferenceContext context{valuePath, DifferenceCategory::Differs, "finale27-default",
            source.at(valuePath).first, companion.at(valuePath).first, source, companion,
            finale_mus_reader::FormatEpoch::ZlibLegacy,
            finale_mus_reader::ByteOrder::LittleEndian, nullptr, report};
        REQUIRE(classifyFontDefinitionDifference(context) ==
            DifferenceClassification::CharsetEquivalence);

        context.origin = "legacy-mus";
        REQUIRE_FALSE(classifyFontDefinitionDifference(context));
        context.origin = "finale27-default";
        context.category = DifferenceCategory::ReaderOnly;
        REQUIRE_FALSE(classifyFontDefinitionDifference(context));

        context.category = DifferenceCategory::Differs;
        source.at(bankPath).first =
            Value(static_cast<std::int64_t>(FontDefinition::CharacterSetBank::MacOS));
        REQUIRE_FALSE(classifyFontDefinitionDifference(context));
        source.at(bankPath).first =
            Value(static_cast<std::int64_t>(FontDefinition::CharacterSetBank::Windows));
        companion.at(bankPath).first =
            Value(static_cast<std::int64_t>(FontDefinition::CharacterSetBank::MacOS));
        REQUIRE_FALSE(classifyFontDefinitionDifference(context));
    }
}

TEST_CASE("Finale conversion loses the legacy double-whole slash glyph", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value legacyGlyph(218);
    const Value filledNoteheadSlash(213);
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);
    DifferenceContext context{"music_symbol_options.dbl_whole_slash",
        DifferenceCategory::Differs, "finale27-default", legacyGlyph, filledNoteheadSlash,
        leaves, leaves, finale_mus_reader::FormatEpoch::CodaBanner,
        finale_mus_reader::ByteOrder::BigEndian, nullptr, report};

    REQUIRE(classifyDoubleWholeSlashConversionLoss(context) ==
        DifferenceClassification::FinaleUpgradeLoss);

    context.origin = "legacy-mus";
    REQUIRE_FALSE(classifyDoubleWholeSlashConversionLoss(context));
    context.origin = "finale27-default";
    context.path = "music_symbol_options.slash_bar";
    REQUIRE_FALSE(classifyDoubleWholeSlashConversionLoss(context));

    const DifferenceContext reverseContext{"music_symbol_options.dbl_whole_slash",
        DifferenceCategory::Differs, "finale27-default", filledNoteheadSlash, legacyGlyph,
        leaves, leaves, finale_mus_reader::FormatEpoch::CodaBanner,
        finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
    REQUIRE_FALSE(classifyDoubleWholeSlashConversionLoss(reverseContext));
}

TEST_CASE("Coda slash defaults permit early font layout shifts", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value companionGlyph(124);
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);
    for (const auto& [path, sourceGlyph] : {
             std::pair{std::string_view("music_symbol_options.half_slash"), Value(250)},
             std::pair{std::string_view("music_symbol_options.whole_slash"), Value(119)},
        }) {
        const DifferenceContext context{path, DifferenceCategory::Differs,
            "legacy-mus", sourceGlyph, companionGlyph, leaves, leaves,
            finale_mus_reader::FormatEpoch::CodaBanner,
            finale_mus_reader::ByteOrder::LittleEndian, nullptr, report};
        REQUIRE(classifyVersionlessCodaSlashDefault(context) ==
            DifferenceClassification::DifferentDefaults);
    }

    const Value sourceGlyph(250);
    const finale_mus_reader::SourceVersion version{.raw = 1, .major = 3};
    const DifferenceContext versioned{"music_symbol_options.half_slash",
        DifferenceCategory::Differs, "finale27-default", sourceGlyph, companionGlyph,
        leaves, leaves, finale_mus_reader::FormatEpoch::CodaBanner,
        finale_mus_reader::ByteOrder::LittleEndian, &version, report};
    REQUIRE_FALSE(classifyVersionlessCodaSlashDefault(versioned));

    const finale_mus_reader::SourceVersion productVersion{.major = 2, .minor = 6};
    const DifferenceContext productVersioned{"music_symbol_options.half_slash",
        DifferenceCategory::Differs, "legacy-mus", sourceGlyph, companionGlyph, leaves,
        leaves, finale_mus_reader::FormatEpoch::CodaBanner,
        finale_mus_reader::ByteOrder::LittleEndian, &productVersion, report};
    REQUIRE(classifyVersionlessCodaSlashDefault(productVersioned) ==
        DifferenceClassification::DifferentDefaults);

    for (const auto path : {
             std::string_view("music_symbol_options.quarter_slash"),
             std::string_view("music_symbol_options.slash_bar"),
         }) {
        const DifferenceContext codaDefault{path, DifferenceCategory::Differs,
            "finale27-default", sourceGlyph, companionGlyph, leaves, leaves,
            finale_mus_reader::FormatEpoch::CodaBanner,
            finale_mus_reader::ByteOrder::LittleEndian, &version, report};
        REQUIRE(classifyVersionlessCodaSlashDefault(codaDefault) ==
            DifferenceClassification::DifferentDefaults);
    }

    const DifferenceContext versionedDoubleWhole{
        "music_symbol_options.dbl_whole_slash", DifferenceCategory::Differs,
        "finale27-default", sourceGlyph, companionGlyph, leaves, leaves,
        finale_mus_reader::FormatEpoch::CodaBanner,
        finale_mus_reader::ByteOrder::LittleEndian, &version, report};
    REQUIRE_FALSE(classifyVersionlessCodaSlashDefault(versionedDoubleWhole));

    const DifferenceContext wrongEpoch{"music_symbol_options.half_slash",
        DifferenceCategory::Differs, "legacy-mus", sourceGlyph, companionGlyph, leaves,
        leaves, finale_mus_reader::FormatEpoch::UncompressedLegacy,
        finale_mus_reader::ByteOrder::LittleEndian, nullptr, report};
    REQUIRE_FALSE(classifyVersionlessCodaSlashDefault(wrongEpoch));

    for (const auto path : {
             std::string_view("music_symbol_options.dbl_whole_slash"),
             std::string_view("music_symbol_options.quarter_slash"),
             std::string_view("music_symbol_options.slash_bar"),
         }) {
        const DifferenceContext retainedDefault{path, DifferenceCategory::Differs,
            "finale27-default", sourceGlyph, companionGlyph, leaves, leaves,
            finale_mus_reader::FormatEpoch::CodaBanner,
            finale_mus_reader::ByteOrder::LittleEndian, nullptr, report};
        REQUIRE(classifyVersionlessCodaSlashDefault(retainedDefault) ==
            DifferenceClassification::DifferentDefaults);

        const DifferenceContext recovered{path, DifferenceCategory::Differs,
            "legacy-mus", sourceGlyph, companionGlyph, leaves, leaves,
            finale_mus_reader::FormatEpoch::CodaBanner,
            finale_mus_reader::ByteOrder::LittleEndian, nullptr, report};
        REQUIRE_FALSE(classifyVersionlessCodaSlashDefault(recovered));
    }
}

TEST_CASE("Seeded rest-drop differences are different defaults", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value sourceValue(-24);
    const Value companionValue(0);
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);

    for (const auto leaf : {noteRestDrop8thLeaf, noteRestDrop16thLeaf,
             noteRestDrop32ndLeaf, noteRestDrop64thLeaf, noteRestDrop128thLeaf}) {
        const auto path = std::string("note_rest_options.") + std::string(leaf);
        DifferenceContext context{path, DifferenceCategory::Differs, "finale27-default",
            sourceValue, companionValue, leaves, leaves,
            finale_mus_reader::FormatEpoch::CodaBanner,
            finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
        REQUIRE(classifyNoteRestOptionsDifference(context) ==
            DifferenceClassification::DifferentDefaults);

        context.origin = "legacy-mus";
        REQUIRE_FALSE(classifyNoteRestOptionsDifference(context));
        context.origin = "finale27-default";
        context.category = DifferenceCategory::ReaderOnly;
        REQUIRE_FALSE(classifyNoteRestOptionsDifference(context));
    }

    const DifferenceContext unrelated{"note_rest_options.draw_outline",
        DifferenceCategory::Differs, "finale27-default", sourceValue, companionValue,
        leaves, leaves, finale_mus_reader::FormatEpoch::CodaBanner,
        finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
    REQUIRE_FALSE(classifyNoteRestOptionsDifference(unrelated));
}

TEST_CASE("Page adjustment scope differences are different defaults", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value sourceValue(0);
    const Value companionValue(1);
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::DclLegacy);
    DifferenceContext context{"page_format_options.adjust_page_scope",
        DifferenceCategory::Differs, "finale27-default", sourceValue, companionValue,
        leaves, leaves, finale_mus_reader::FormatEpoch::DclLegacy,
        finale_mus_reader::ByteOrder::BigEndian, nullptr, report};

    REQUIRE(classifyPageFormatOptionsDifference(context) ==
        DifferenceClassification::DifferentDefaults);

    context.origin = "legacy-mus";
    REQUIRE_FALSE(classifyPageFormatOptionsDifference(context));
    context.origin = "finale27-default";
    context.category = DifferenceCategory::ReaderOnly;
    REQUIRE_FALSE(classifyPageFormatOptionsDifference(context));
    context.category = DifferenceCategory::Differs;
    context.path = "page_format_options.avoid_system_margin_collisions";
    REQUIRE_FALSE(classifyPageFormatOptionsDifference(context));
}

TEST_CASE("Unrecovered scattered-layout tie options are different defaults", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value sourceValue(8);
    const Value companionValue(0);
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);
    DifferenceContext context{"tie_options.tie_connect_styles[0].offset_x",
        DifferenceCategory::Differs, "finale27-default", sourceValue, companionValue,
        leaves, leaves, finale_mus_reader::FormatEpoch::CodaBanner,
        finale_mus_reader::ByteOrder::BigEndian, nullptr, report};
    REQUIRE(classifyTieOptionsDifference(context) ==
            DifferenceClassification::DifferentDefaults);

    context.origin = "legacy-mus";
    REQUIRE_FALSE(classifyTieOptionsDifference(context));
    context.origin = "finale27-default";
    context.category = DifferenceCategory::ReaderOnly;
    REQUIRE_FALSE(classifyTieOptionsDifference(context));
    context.category = DifferenceCategory::Differs;
    context.epoch = finale_mus_reader::FormatEpoch::UncompressedLegacy;
    REQUIRE_FALSE(classifyTieOptionsDifference(context));

    report.setField(
        finale_mus_reader::instanceKey<musx::dom::options::TieOptions>(),
        "breakForTimeSigs",
        {finale_mus_reader::ValueOrigin::LegacyBehavior, 0, 0, 0});
    REQUIRE(classifyTieOptionsDifference(context) ==
            DifferenceClassification::DifferentDefaults);
}

TEST_CASE("Coda source-derived tie thickness differences are upgrade loss", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value sourceValue(6);
    const Value companionValue(0);
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::CodaBanner);
    DifferenceContext context{"tie_options.thickness_left", DifferenceCategory::Differs,
        "legacy-mus-adjusted", sourceValue, companionValue, leaves, leaves,
        finale_mus_reader::FormatEpoch::CodaBanner, finale_mus_reader::ByteOrder::BigEndian,
        nullptr, report};

    REQUIRE(classifyTieOptionsDifference(context) ==
            DifferenceClassification::FinaleUpgradeLoss);
    context.path = "tie_options.thickness_right";
    REQUIRE(classifyTieOptionsDifference(context) ==
            DifferenceClassification::FinaleUpgradeLoss);

    const Value otherSourceValue(7);
    const DifferenceContext otherValue{"tie_options.thickness_right", DifferenceCategory::Differs,
        "legacy-mus", otherSourceValue, companionValue, leaves, leaves,
        finale_mus_reader::FormatEpoch::CodaBanner, finale_mus_reader::ByteOrder::BigEndian,
        nullptr, report};
    REQUIRE(classifyTieOptionsDifference(otherValue) ==
            DifferenceClassification::FinaleUpgradeLoss);

    context.path = "tie_options.tie_tip_width";
    REQUIRE_FALSE(classifyTieOptionsDifference(context));
    context.path = "tie_options.thickness_left";
    context.origin = "finale27-default";
    REQUIRE(classifyTieOptionsDifference(context) ==
            DifferenceClassification::DifferentDefaults);
    context.origin = "legacy-mus-adjusted";
    context.epoch = finale_mus_reader::FormatEpoch::UncompressedLegacy;
    REQUIRE_FALSE(classifyTieOptionsDifference(context));
}

TEST_CASE("Finale 2006 mixed-stem direction conversion loss is upgrade loss", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const Value sourceValue(2);
    const Value companionValue(0);
    const ComparisonLeaves leaves;
    finale_mus_reader::ImportReport report(finale_mus_reader::FormatEpoch::DclLegacy);
    const finale_mus_reader::SourceVersion finale2006{.major = 11};
    DifferenceContext context{"tie_options.mixed_stem_direction",
        DifferenceCategory::Differs, "legacy-mus", sourceValue, companionValue, leaves, leaves,
        finale_mus_reader::FormatEpoch::DclLegacy,
        finale_mus_reader::ByteOrder::LittleEndian, &finale2006, report};

    REQUIRE(classifyTieOptionsDifference(context) ==
            DifferenceClassification::FinaleUpgradeLoss);

    const finale_mus_reader::SourceVersion finale2005{.major = 10};
    context.sourceVersion = &finale2005;
    REQUIRE_FALSE(classifyTieOptionsDifference(context));
    context.sourceVersion = &finale2006;
    context.origin = "finale27-default";
    REQUIRE_FALSE(classifyTieOptionsDifference(context));
}


TEST_CASE("The synthesized score name is the only part-definition difference classified",
    "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    using finale_mus_reader::FormatEpoch;
    finale_mus_reader::ImportReport report(FormatEpoch::UncompressedLegacy);
    const ComparisonLeaves none;
    const auto classify = [&](std::string_view path, FormatEpoch epoch,
                              std::string_view origin, const Value& source,
                              const Value& companion) {
        const DifferenceContext context{path, DifferenceCategory::Differs, origin, source,
            companion, none, none, epoch, finale_mus_reader::ByteOrder::BigEndian, nullptr,
            report};
        return classifyPartDefinitionDifference(context);
    };
    const Value zero(std::int64_t{0});
    const Value block(std::int64_t{28});
    constexpr std::string_view scoreName = "part_defs[cmper=0].name_id";

    REQUIRE(classify(scoreName, FormatEpoch::UncompressedLegacy, "legacy-behavior", zero, block)
        == DifferenceClassification::SynthesizedScoreName);
    REQUIRE(classify(scoreName, FormatEpoch::CodaBanner, "legacy-behavior", zero, block)
        == DifferenceClassification::SynthesizedScoreName);
    REQUIRE(classify(scoreName, FormatEpoch::DclLegacy, "legacy-behavior", zero, block)
        == DifferenceClassification::SynthesizedScoreName);

    // Every condition is load-bearing: none of these may be swallowed by the rule.
    SECTION("the epoch that stores the member is never classified")
    {
        REQUIRE_FALSE(classify(scoreName, FormatEpoch::ZlibLegacy, "legacy-behavior", zero,
            block));
    }
    SECTION("a recovered value that disagrees is never classified")
    {
        REQUIRE_FALSE(classify(scoreName, FormatEpoch::ZlibLegacy, "legacy-mus", block,
            Value(std::int64_t{29})));
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
        REQUIRE_FALSE(classify(scoreName, FormatEpoch::UncompressedLegacy, "legacy-behavior",
            block, Value(std::int64_t{29})));
    }
    SECTION("a companion that supplies no name is never classified")
    {
        REQUIRE_FALSE(classify(scoreName, FormatEpoch::UncompressedLegacy, "legacy-behavior",
            zero, zero));
    }

    SECTION("an unreachable member the companion sets is possibly unrecoverable")
    {
        constexpr std::string_view unlink = "part_defs[cmper=14].unlink_insts";
        REQUIRE(classify(unlink, FormatEpoch::ZlibLegacy, "unmapped", Value(false), Value(true))
            == DifferenceClassification::PossiblyUnrecoverable);
        // A recovered value that disagrees is a real defect and must stay unexpected, as must a
        // reader value the companion does not share the direction of.
        REQUIRE_FALSE(classify(unlink, FormatEpoch::ZlibLegacy, "legacy-mus", Value(false),
            Value(true)));
        REQUIRE_FALSE(classify(unlink, FormatEpoch::ZlibLegacy, "unmapped", Value(true),
            Value(false)));
    }
}


TEST_CASE("A part name requires the reader to name the text, and the synthesis does not",
    "[coverage]")
{
    using finale_mus_reader::coverage::partNameTextMatches;
    using finale_mus_reader::coverage::synthesizedScoreNameText;
    const std::set<std::int64_t> none;
    const std::set<std::int64_t> names337{337};
    const std::set<std::int64_t> names12{12};

    // Both sides naming a text must agree on which, so an unrelated block is not swept in.
    REQUIRE(partNameTextMatches(names337, names337, 337));
    REQUIRE_FALSE(partNameTextMatches(names337, names12, 337));
    REQUIRE_FALSE(partNameTextMatches(names12, names337, 337));
    // A companion that names none defers to the reader.
    REQUIRE(partNameTextMatches(names337, none, 337));

    // A text only the companion names is not a part name the reader got wrong. It is the score
    // name Finale synthesized, and it is classified as that instead.
    REQUIRE_FALSE(partNameTextMatches(none, names337, 337));
    REQUIRE(synthesizedScoreNameText(none, names337, 337));
    REQUIRE_FALSE(synthesizedScoreNameText(none, names337, 12));

    // The two are mutually exclusive, and neither fires when nothing names the text.
    REQUIRE_FALSE(synthesizedScoreNameText(names337, names337, 337));
    REQUIRE_FALSE(synthesizedScoreNameText(names12, names337, 337));
    REQUIRE_FALSE(partNameTextMatches(none, none, 337));
    REQUIRE_FALSE(synthesizedScoreNameText(none, none, 337));
}

} // namespace
} // namespace finale_mus_reader_tests
