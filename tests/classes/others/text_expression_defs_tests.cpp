// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"
#include "coverage/registry.h"

namespace finale_mus_reader_tests {
namespace {
using namespace classes;
using TestExpression = musx::dom::others::TextExpressionDef;

TEST_CASE("Text expressions recover stored playback and synthesize early text",
          "[class][text-expression]")
{
    for (const auto path :
         {"evidence/F263/F263-nodrop-64th.mus", "evidence/F2000/F2000-lyropts-align-just.mus"}) {
        const auto result = readFixture(path);
        const auto expression =
            result.document->getOthers()->get<TestExpression>(musx::dom::SCORE_PARTID, 1);
        REQUIRE(expression);
        CHECK(expression->value == 88);
        CHECK(expression->playbackType == musx::dom::others::PlaybackType::KeyVelocity);
        CHECK(expression->categoryId == 7);
        CHECK(expression->horzMeasExprAlign == musx::dom::others::HorizontalMeasExprAlign::Manual);
        CHECK(expression->vertMeasExprAlign == musx::dom::others::VerticalMeasExprAlign::Manual);
        CHECK(field(result.report, "others.textExprDef[1].horzMeasExprAlign").origin ==
              ValueOrigin::LegacyBehavior);
        CHECK(field(result.report, "others.textExprDef[1].vertMeasExprAlign").origin ==
              ValueOrigin::LegacyBehavior);
        const auto block = expression->getTextBlock();
        REQUIRE(block);
        CHECK(block->textType == musx::dom::others::TextBlock::TextType::Expression);
        const auto raw =
            result.document->getTexts()->get<musx::dom::texts::ExpressionText>(block->textId);
        REQUIRE(raw);
        CHECK(raw->text.ends_with("f"));
        CHECK(raw->text.find(std::string_view(path).find("F263") != std::string_view::npos
                                 ? "^size(24)"
                                 : "^size(28)") != std::string::npos);
    }
}

TEST_CASE("F2002 expression hiding is limited to bracketed spans", "[class][text-expression]")
{
    const auto result = readFixture("evidence/F2002/F2002-exp-hidepartial.mus");
    const auto expression = result.document->getOthers()->get<TestExpression>(0, 1);
    REQUIRE(expression);
    const auto raw = expression->getRawTextCtx(0).getRawText();
    REQUIRE(raw);
    CHECK(raw->text ==
          "^font(Times)^size(12)^nfx(0)test is ^nfx(128)hidden^nfx(0) text");
    const auto unbalanced = readFixture("evidence/F2002/F2002-exp-unbalanced.mus");
    const auto unbalancedExpression = unbalanced.document->getOthers()->get<TestExpression>(0, 1);
    REQUIRE(unbalancedExpression);
    const auto unbalancedText = unbalancedExpression->getRawTextCtx(0).getRawText();
    REQUIRE(unbalancedText);
    CHECK(unbalancedText->text == "^font(Times)^size(12)^nfx(0)test is ^nfx(128)hidden?");
}

TEST_CASE("Inline hidden spans preserve surrounding text and effects", "[class][text-expression]")
{
    using namespace finale_mus_reader;
    for (const auto version : {versions::finale2001, versions::finale2002, versions::finale2003}) {
        for (const bool enabled : {false, true}) {
            auto session = musx::factory::DocumentFactory::begin();
            const auto document = session.getDocument();
            auto font = std::make_shared<musx::dom::others::FontDefinition>(
                document, musx::dom::Cmper(0), musx::dom::EnigmaBase::ShareMode::All,
                musx::dom::Cmper(1));
            font->name = "Times";
            document->getOthers()->add(font->XmlNodeName, font);
            const std::string plain = "A>x>>B<y<z>^#<open";
            std::vector<SyntheticRow> rows{
                {1,
                 "DT",
                 {0x0c01, 1, 0, 0, 0, static_cast<std::int16_t>(enabled ? 0xc200 : 0xc000)}}};
            for (std::size_t offset = 0; offset <= plain.size(); offset += 12) {
                SyntheticRow row{1, "DT", {}};
                for (std::size_t i = 0; i < 12 && offset + i < plain.size(); ++i) {
                    row.words[i / 2] |=
                        static_cast<std::uint16_t>(static_cast<unsigned char>(plain[offset + i]))
                        << (i % 2 ? 0 : 8);
                }
                rows.push_back(row);
            }
            auto parsed = makeContainer(rows, FormatEpoch::DclLegacy);
            auto profile = profileFor(version.major);
            profile.epoch = FormatEpoch::DclLegacy;
            const auto index = LegacyRecordIndex::build(parsed);
            ImportReport report(profile.epoch);
            PendingReferences pending;
            const ImportContext context{
                index,    profile, noSource, document,
                document, report,  pending,  session.getConstructionContext()};
            others::importTextExpressionDefs(context);
            for (const auto& check : pending.checks)
                check();
            const auto expression = document->getOthers()->get<TestExpression>(0, 1);
            REQUIRE(expression);
            const auto raw = expression->getRawTextCtx(0).getRawText();
            REQUIRE(raw);
            const bool partial = version.major >= versions::finale2002.major;
            const std::string body =
                enabled && partial
                    ? "^nfx(1)A^nfx(1)x^nfx(1)^nfx(1)B^nfx(129)y^nfx("
                      "129)z^nfx(1)^^#^nfx(129)open"
                    : std::string(enabled ? "^nfx(129)" : "^nfx(1)") + "A>x>>B<y<z>^^#<open";
            CHECK(raw->text == "^font(Times)^size(12)" + body);
        }
    }
}

TEST_CASE("Prime Enigma text expression references survive fixed-row import",
          "[class][text-expression]")
{
    const auto result = readFixture("evidence/F2006/F2006-text-inserts.mus");
    const auto expression =
        result.document->getOthers()->get<TestExpression>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(expression);
    CHECK(expression->textIdKey == 19);
    CHECK(expression->categoryId == 7);
    CHECK(expression->horzMeasExprAlign == musx::dom::others::HorizontalMeasExprAlign::Manual);
    CHECK(expression->rehearsalMarkStyle == musx::dom::others::RehearsalMarkStyle::None);
    CHECK(field(result.report, "others.textExprDef[1].rehearsalMarkStyle").origin ==
          ValueOrigin::LegacyBehavior);
    CHECK_FALSE(expression->matchPlayback);
    CHECK(field(result.report, "others.textExprDef[1].matchPlayback").origin ==
          ValueOrigin::LegacyBehavior);
    CHECK(expression->vertMeasExprAlign == musx::dom::others::VerticalMeasExprAlign::TopNote);
    CHECK(field(result.report, "others.textExprDef[1].textIdKey").origin == ValueOrigin::LegacyMus);
    const auto described = readFixture("evidence/F2006/F2006-embedded-tiff.mus");
    CHECK(described.document->getOthers()
              ->get<TestExpression>(musx::dom::SCORE_PARTID, 1)
              ->description == "Below Staff (Vel. 127)");
}

TEST_CASE("Unicode era expressions recover category and signed positioning",
          "[class][text-expression]")
{
    const auto result = readFixture("evidence/F2012/F2012-noteartexp-unlnk-move.mus");
    const auto expression =
        result.document->getOthers()->get<TestExpression>(musx::dom::SCORE_PARTID, 1);
    REQUIRE(expression);
    CHECK(expression->textIdKey == 2);
    CHECK(expression->categoryId == 1);
    CHECK(expression->horzMeasExprAlign ==
          musx::dom::others::HorizontalMeasExprAlign::LeftOfPrimaryNotehead);
    CHECK(expression->horzExprJustification == musx::dom::AlignJustify::Center);
    CHECK(expression->vertMeasExprAlign ==
          musx::dom::others::VerticalMeasExprAlign::BelowStaffOrEntry);
    CHECK(expression->yAdjustBaseline == 16);
    CHECK(expression->yAdjustEntry == -72);
}

TEST_CASE("SmartMusic playback normalizes with retained source provenance",
          "[class][text-expression]")
{
    const auto result = readFixture("evidence/F2006/F2006-embedded-tiff.mus");
    for (const auto cmper : {29, 30, 31, 32, 17}) {
        const bool smartMusic = cmper != 17;
        const auto expression = result.document->getOthers()->get<TestExpression>(
            musx::dom::SCORE_PARTID, musx::dom::Cmper(cmper));
        REQUIRE(expression);
        CHECK(expression->playbackType == (smartMusic ? musx::dom::others::PlaybackType::None
                                                      : musx::dom::others::PlaybackType::Tempo));
        CHECK(expression->auxData1 == (smartMusic ? 0 : 1024));
        CHECK(expression->useAuxData);
        if (smartMusic) {
            CHECK(expression->hasEnclosure);
            CHECK(expression->breakMmRest);
            CHECK(expression->yAdjustBaseline == 18);
        }
        const auto prefix = "others.textExprDef[" + std::to_string(cmper) + "].";
        const auto& auxiliary = field(result.report, prefix + "auxData1");
        const auto& playback = field(result.report, prefix + "playbackType");
        if (smartMusic) {
            const auto& baseline = field(result.report, prefix + "yAdjustBaseline");
            CHECK(baseline.origin == ValueOrigin::LegacyMus);
            CHECK(baseline.rawValue == 18);
        }
        const auto origin = smartMusic ? ValueOrigin::LegacyMusAdjusted : ValueOrigin::LegacyMus;
        CHECK(auxiliary.origin == origin);
        CHECK(playback.origin == origin);
        CHECK(auxiliary.rawValue == (smartMusic ? (cmper == 30 ? 15 : 14) : 1024));
        CHECK(playback.rawValue == (smartMusic ? 0x1c0f : 0x1001));
        CHECK(auxiliary.sourceIdentity == 0x4454);
        CHECK(playback.decodedOffset == auxiliary.decodedOffset + 4);
        CHECK(field(result.report, prefix + "useAuxData").origin == ValueOrigin::LegacyMus);
    }
}

TEST_CASE("Rehearsal marks independently hide measure numbers and match playback",
          "[class][text-expression]")
{
    const char* paths[] = {"evidence/F2011/F2011-rehearsal-mark.mus",
                           "evidence/F2011/F2011-rehearsal-mark-hidemeas.mus",
                           "evidence/F2011/F2011-rehearsal-mark-matchplay.mus"};
    for (const auto variant : {0, 1, 2}) {
        const bool hidden = variant == 1;
        const bool match = variant == 2;
        const auto result = readFixture(paths[variant]);
        const auto expression =
            result.document->getOthers()->get<TestExpression>(musx::dom::SCORE_PARTID, 1);
        REQUIRE(expression);
        CHECK(expression->hideMeasureNum == hidden);
        CHECK(expression->matchPlayback == match);
        CHECK(expression->useAuxData == match);
        CHECK(expression->auxData1 == (match ? 1024 : 0));
        CHECK(expression->playbackType == (match ? musx::dom::others::PlaybackType::Tempo
                                                 : musx::dom::others::PlaybackType::None));
        CHECK(expression->rehearsalMarkStyle ==
              musx::dom::others::RehearsalMarkStyle::MeasureNumber);
        CHECK(expression->breakMmRest);
        const auto& flag = field(result.report, "others.textExprDef[1].hideMeasureNum");
        CHECK(flag.origin == ValueOrigin::LegacyMus);
        CHECK(flag.rawValue == static_cast<std::int16_t>(hidden  ? 0x8400
                                                         : match ? 0x5401
                                                                 : 0x0400));
        CHECK(flag.sourceIdentity == 0x00f1);
        CHECK(flag.blockOffset == 0x200);
        CHECK(flag.decodedOffset == 0x23bc + 10);
        const auto& matchFlag = field(result.report, "others.textExprDef[1].matchPlayback");
        CHECK(matchFlag.origin == ValueOrigin::LegacyMus);
        CHECK(matchFlag.rawValue == flag.rawValue);
        CHECK(matchFlag.decodedOffset == flag.decodedOffset);
    }
}

TEST_CASE("Expression UTF-16 descriptions and bounds are independent of byte order",
          "[class][text-expression]")
{
    for (auto order : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
        auto session = musx::factory::DocumentFactory::begin();
        auto document = session.getDocument();
        std::vector<std::int16_t> words(18);
        words[1] = 4;
        words.insert(words.end(), {0x03a9, static_cast<std::int16_t>(0xd834),
                                   static_cast<std::int16_t>(0xdd1e), 0});
        auto parsed = makeClassContainer(
            {SyntheticClassRow{0xf1, words, 1}, SyntheticClassRow{0xf1, {1, 2, 3}, 2}}, order);
        auto profile = profileFor(finale_mus_reader::versions::finale2012.major);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = order;
        const auto index = LegacyRecordIndex::build(parsed);
        ImportReport report(profile.epoch);
        finale_mus_reader::PendingReferences pending;
        const finale_mus_reader::ImportContext context{
            index,    profile, noSource, document,
            document, report,  pending,  session.getConstructionContext()};
        finale_mus_reader::others::importTextExpressionDefs(context);
        const auto expression =
            document->getOthers()->get<TestExpression>(musx::dom::SCORE_PARTID, 1);
        REQUIRE(expression);
        CHECK(expression->description == "Ω𝄞");
        CHECK(expression->rehearsalMarkStyle ==
              musx::dom::others::RehearsalMarkStyle::LettersNumbersLowerCase);
        const auto& style = field(report, "others.textExprDef[1].rehearsalMarkStyle");
        CHECK(style.origin == ValueOrigin::LegacyMus);
        CHECK(style.rawValue == 4);
        CHECK_FALSE(document->getOthers()->get<TestExpression>(musx::dom::SCORE_PARTID, 2));
        CHECK(report.diagnostics.size() == 1);
        CHECK(reportedFieldCount(report) == 23);
        CHECK(TestExpression::xmlMappingArray().size() == reportedFieldCount(report));
        CHECK(field(report, "others.textExprDef[1].createdByHp").origin == ValueOrigin::Unmapped);
        const auto observed = finale_mus_reader::coverage::runAllSurveyors({document, report});
        const auto& item = observed.snapshot.at("text_expression_defs").asArray().front();
        std::size_t members = 0;
        std::size_t origins = 0;
        for (const auto& [name, value] : item.asObject()) {
            if (name.starts_with("origin_"))
                ++origins;
            else if (name != "cmper" && name != "part_id" && name != "origin" &&
                     name != "share_mode")
                ++members;
        }
        CHECK(members + 6 == TestExpression::xmlMappingArray().size());
        CHECK(origins == members);
        for (const auto name :
             {"horz_meas_expr_align", "vert_meas_expr_align", "horz_expr_justification",
              "meas_x_adjust", "y_adjust_entry", "y_adjust_baseline", "origin_horzMeasExprAlign",
              "origin_vertMeasExprAlign", "origin_horzExprJustification", "origin_measXAdjust",
              "origin_yAdjustEntry", "origin_yAdjustBaseline"}) {
            CHECK_FALSE(item.find(name));
        }
        CHECK(item.find("use_category_pos"));
        CHECK(item.find("created_by_hp"));
        CHECK(item.find("origin_createdByHp")->asString() == "unmapped");
    }
}

TEST_CASE("Expression observations are omitted from companion comparison",
          "[coverage][text-expression]")
{
    using namespace finale_mus_reader::coverage;
    const auto expressionType = static_cast<std::int64_t>(
        musx::dom::others::TextBlock::TextType::Expression);
    const auto blockType =
        static_cast<std::int64_t>(musx::dom::others::TextBlock::TextType::Block);
    const auto snapshot = [&] {
        return SurveySnapshot{
            {"text_expression_defs", Value::Array{Value::Object{{"cmper", 1}}}},
            {"expression_texts", Value::Array{Value::Object{{"number", 1}}}},
            {"text_blocks",
                Value::Array{Value::Object{{"cmper", 1}, {"text_type", expressionType}},
                    Value::Object{{"cmper", 2}, {"text_type", blockType}}}}};
    };
    auto source = snapshot();
    auto companion = snapshot();
    std::map<ComparisonTransformation, std::uint64_t> transformations;
    ComparisonPreparationContext context{
        source, companion, transformations, FormatEpoch::ZlibLegacy};
    runComparisonPreparers(context);
    for (const auto* side : {&source, &companion}) {
        CHECK_FALSE(side->contains("text_expression_defs"));
        CHECK_FALSE(side->contains("expression_texts"));
        REQUIRE(side->at("text_blocks").asArray().size() == 1);
        CHECK(side->at("text_blocks").asArray().front().find("cmper")->asInteger() == 2);
    }
}

TEST_CASE("Legacy expression note selectors translate independently", "[class][text-expression]")
{
    using H = musx::dom::others::HorizontalMeasExprAlign;
    using V = musx::dom::others::VerticalMeasExprAlign;
    const H horizontal[] = {H::LeftOfAllNoteheads,
                            H::Manual,
                            H::Stem,
                            H::CenterPrimaryNotehead,
                            H::CenterAllNoteheads,
                            H::RightOfAllNoteheads,
                            H::LeftOfPrimaryNotehead};
    const V vertical[] = {V::Manual,     V::AboveStaff,        V::BelowStaff,
                          V::TopNote,    V::BottomNote,        V::AboveEntry,
                          V::BelowEntry, V::AboveStaffOrEntry, V::BelowStaffOrEntry};
    for (const bool modern : {false, true}) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        std::vector<SyntheticClassRow> rows;
        for (std::int16_t code = 0; code <= 9; ++code) {
            std::vector<std::int16_t> words(18);
            words[9] = words[14] = code;
            rows.push_back({0xf1, words, static_cast<musx::dom::Cmper>(code + 1)});
        }
        auto parsed = makeClassContainer(rows, ByteOrder::LittleEndian);
        auto profile = profileFor(modern ? finale_mus_reader::versions::finale2009.major
                                         : finale_mus_reader::versions::finale2008.major);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::LittleEndian;
        const auto index = LegacyRecordIndex::build(parsed);
        ImportReport report(profile.epoch);
        finale_mus_reader::PendingReferences pending;
        const finale_mus_reader::ImportContext context{
            index,    profile, noSource, document,
            document, report,  pending,  session.getConstructionContext()};
        finale_mus_reader::others::importTextExpressionDefs(context);
        for (int code = 0; code <= 9; ++code) {
            const auto expression = document->getOthers()->get<TestExpression>(
                0, musx::dom::Cmper(code + 1));
            REQUIRE(expression);
            const auto prefix = "others.textExprDef[" + std::to_string(code + 1) + "].";
            const auto& h = field(report, prefix + "horzMeasExprAlign");
            const auto& v = field(report, prefix + "vertMeasExprAlign");
            if (modern) {
                CHECK(expression->horzMeasExprAlign == H::LeftBarline);
                CHECK(expression->vertMeasExprAlign == V::AboveStaff);
                CHECK(h.rawValue == 0);
                CHECK(v.rawValue == 0);
            } else {
                if (code < 7) {
                    CHECK(expression->horzMeasExprAlign == horizontal[code]);
                    CHECK(h.origin == ValueOrigin::LegacyMus);
                    CHECK(h.rawValue == code);
                } else
                    CHECK(h.origin == ValueOrigin::Unmapped);
                if (code < 9) {
                    CHECK(expression->vertMeasExprAlign == vertical[code]);
                    CHECK(v.origin == ValueOrigin::LegacyMus);
                    CHECK(v.rawValue == code);
                } else
                    CHECK(v.origin == ValueOrigin::Unmapped);
            }
        }
    }
}

} // namespace
} // namespace finale_mus_reader_tests
