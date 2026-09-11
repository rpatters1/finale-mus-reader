// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

#include "coverage/comparison.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

std::vector<SyntheticRow> textBlockFixedRows(
    std::uint16_t cmper, const std::vector<std::int16_t>& words)
{
    std::vector<SyntheticRow> rows;
    for (std::size_t at = 0; at < words.size(); at += 6) {
        SyntheticRow row{cmper, "TX", {}};
        for (std::size_t slot = 0; slot < 6; ++slot) row.words[slot] = words[at + slot];
        rows.push_back(row);
    }
    return rows;
}

void textBlockImport(const finale_mus_reader::container::ParsedContainer& parsed,
    const SourceProfile& profile, const musx::dom::DocumentPtr& document, ImportReport& report)
{
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{
        index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importTextBlocks(context);
    // The Coda pass is deferred, because it pairs its rows against the imported text pool. A test
    // driving one importer alone still has to close the phase the registry closes.
    finale_mus_reader::runDeferredChecks(pending);
}

void testStoredTextBlocksAcrossEpochs()
{
    using Target = musx::dom::others::TextBlock;
    const std::vector<std::int16_t> words{
        9, 480, 240, 3, 125, -12, 34, 0x1e09, -1, -32, 0, 64,
        2004, 0, 0, 0, 0, 0};
    for (const auto epoch : {FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        ImportReport report(FormatEpoch::UncompressedLegacy);
        SourceProfile profile(epoch);
        profile.byteOrder = ByteOrder::BigEndian;
        if (epoch == FormatEpoch::ZlibLegacy) {
            textBlockImport(makeClassContainer(0x00b7, words, profile.byteOrder, 7),
                profile, document, report);
        } else {
            textBlockImport(makeContainer(textBlockFixedRows(7, words), epoch),
                profile, document, report);
        }
        const auto block = document->getOthers()->get<Target>(musx::dom::SCORE_PARTID, 7);
        expectMapping(block && block->textId == 9 && block->width == 480
                && block->height == 240 && block->shapeId == 3
                && block->lineSpacingPercentage == 125 && !block->lineSpacingEvpu
                && block->xAdd == -12 && block->yAdd == 34,
            "The stored TextBlock scalar fields were not recovered");
        expectMapping(block->justify == Target::TextJustify::Right && block->newPos36
                && block->showShape && block->noExpandSingleWord && block->wordWrap,
            "The stored TextBlock flags were not recovered");
        expectMapping(block->inset == -32 && block->stdLineThickness == 64,
            "The stored TextBlock Efix values were not read high-word first");
        expectMapping(block->textType == Target::TextType::Block,
            "The stored TextBlock block-family discriminator was not recovered");
        expectMapping(!block->roundCorners && block->cornerRadius == 0
                && field(report, "others.textBlock[7].roundCorners").origin
                    == ValueOrigin::LegacyBehavior
                && field(report, "others.textBlock[7].cornerRadius").origin
                    == ValueOrigin::LegacyBehavior,
            "The legacy TextBlock corner behavior was not reported");
        expectMapping(field(report, "others.textBlock[7].justify").rawValue == 1
                && field(report, "others.textBlock[7].textType").rawValue == 2004
                && field(report, "others.textBlock[7].lineSpacingPercentage").rawValue == 125,
            "The TextBlock report did not retain its stored values");
    }

    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    ImportReport report(FormatEpoch::UncompressedLegacy);
    SourceProfile profile(FormatEpoch::UncompressedLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    auto absolute = words;
    absolute[4] = 72;
    absolute[7] = 2;
    textBlockImport(makeContainer(textBlockFixedRows(8, absolute)),
        profile, document, report);
    const auto block = document->getOthers()->get<Target>(musx::dom::SCORE_PARTID, 8);
    expectMapping(block && block->lineSpacingEvpu == 72 && !block->lineSpacingPercentage
            && block->justify == Target::TextJustify::Center,
        "The absolute TextBlock spacing or center justification was not recovered");

    auto zeroSpacingSession = musx::factory::DocumentFactory::begin();
    const auto zeroSpacingDocument = zeroSpacingSession.getDocument();
    ImportReport zeroSpacingReport(FormatEpoch::UncompressedLegacy);
    auto zeroSpacing = absolute;
    zeroSpacing[4] = 0;
    textBlockImport(makeContainer(textBlockFixedRows(10, zeroSpacing)),
        profile, zeroSpacingDocument, zeroSpacingReport);
    const auto zeroSpacingBlock = zeroSpacingDocument->getOthers()->get<Target>(
        musx::dom::SCORE_PARTID, 10);
    expectMapping(zeroSpacingBlock && zeroSpacingBlock->lineSpacingEvpu == 0
            && !zeroSpacingBlock->lineSpacingPercentage
            && field(zeroSpacingReport,
                "others.textBlock[10].lineSpacingEvpu").rawValue == 0,
        "Zero-EVPU TextBlock spacing was not preserved as the stored setting");

    auto zeroPercentSession = musx::factory::DocumentFactory::begin();
    const auto zeroPercentDocument = zeroPercentSession.getDocument();
    ImportReport zeroPercentReport(FormatEpoch::UncompressedLegacy);
    auto zeroPercent = words;
    zeroPercent[4] = 0;
    textBlockImport(makeContainer(textBlockFixedRows(11, zeroPercent)),
        profile, zeroPercentDocument, zeroPercentReport);
    const auto zeroPercentBlock = zeroPercentDocument->getOthers()->get<Target>(
        musx::dom::SCORE_PARTID, 11);
    expectMapping(zeroPercentBlock && zeroPercentBlock->lineSpacingEvpu == 0
            && !zeroPercentBlock->lineSpacingPercentage
            && field(zeroPercentReport,
                "others.textBlock[11].lineSpacingPercentage").rawValue == 0
            && field(zeroPercentReport,
                "others.textBlock[11].lineSpacingEvpu").origin
                == ValueOrigin::LegacyBehavior,
        "Zero-percent TextBlock spacing was not upgraded to zero EVPU");

    for (const std::int16_t discriminator : {
             static_cast<std::int16_t>(finale_mus_reader::records::packTag("xp")),
             std::int16_t(2006)}) {
        auto expressionSession = musx::factory::DocumentFactory::begin();
        const auto expressionDocument = expressionSession.getDocument();
        ImportReport expressionReport(FormatEpoch::UncompressedLegacy);
        auto expression = words;
        expression[12] = discriminator;
        textBlockImport(makeContainer(textBlockFixedRows(9, expression), FormatEpoch::DclLegacy),
            profile, expressionDocument, expressionReport);
        const auto expressionBlock = expressionDocument->getOthers()->get<Target>(
            musx::dom::SCORE_PARTID, 9);
        expectMapping(expressionBlock
                && expressionBlock->textType == Target::TextType::Expression
                && field(expressionReport, "others.textBlock[9].textType").rawValue
                    == discriminator,
            "A stored TextBlock expression-family discriminator was not recovered");
    }

    auto taggedBlockSession = musx::factory::DocumentFactory::begin();
    const auto taggedBlockDocument = taggedBlockSession.getDocument();
    ImportReport taggedBlockReport(FormatEpoch::UncompressedLegacy);
    auto taggedBlockWords = words;
    taggedBlockWords[12] =
        static_cast<std::int16_t>(finale_mus_reader::records::packTag("bl"));
    textBlockImport(makeContainer(textBlockFixedRows(10, taggedBlockWords)),
        profile, taggedBlockDocument, taggedBlockReport);
    const auto taggedBlock = taggedBlockDocument->getOthers()->get<Target>(
        musx::dom::SCORE_PARTID, 10);
    expectMapping(taggedBlock && taggedBlock->textType == Target::TextType::Block
            && field(taggedBlockReport, "others.textBlock[10].textType").rawValue
                == taggedBlockWords[12],
        "The packed block-family TextBlock discriminator was not recovered");

    auto oldSession = musx::factory::DocumentFactory::begin();
    const auto oldDocument = oldSession.getDocument();
    ImportReport oldReport(FormatEpoch::UncompressedLegacy);
    auto oldWords = words;
    oldWords[12] = 0;
    textBlockImport(makeContainer(textBlockFixedRows(11, oldWords)),
        profile, oldDocument, oldReport);
    const auto oldBlock = oldDocument->getOthers()->get<Target>(musx::dom::SCORE_PARTID, 11);
    expectMapping(oldBlock && oldBlock->textType == Target::TextType::Block,
        "A TextBlock without a family discriminator did not retain the block default");
}

void testCodaTextBlockSynthesis()
{
    using BlockText = musx::dom::texts::BlockText;
    using Target = musx::dom::others::TextBlock;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    for (musx::dom::Cmper number : {musx::dom::Cmper{1}, musx::dom::Cmper{2}}) {
        auto text = std::make_shared<BlockText>(document, musx::dom::SCORE_PARTID,
            musx::dom::EnigmaBase::ShareMode::All, number);
        text->text = "block " + std::to_string(number);
        document->getTexts()->add(BlockText::XmlNodeName, std::move(text));
    }
    const auto parsed = makeContainer({{0, "HS", {0, 0, 1036, 0, 0, 0x0080}},
        {0, "HS", {0, 0, 1036, 0, 0, 0x0081}}}, FormatEpoch::CodaBanner);
    ImportReport report(FormatEpoch::UncompressedLegacy);
    SourceProfile profile(FormatEpoch::CodaBanner);
    profile.byteOrder = ByteOrder::BigEndian;
    textBlockImport(parsed, profile, document, report);

    const auto first = document->getOthers()->get<Target>(musx::dom::SCORE_PARTID, 1);
    const auto second = document->getOthers()->get<Target>(musx::dom::SCORE_PARTID, 2);
    expectMapping(first && first->textId == 1 && first->justify == Target::TextJustify::Left
            && second && second->textId == 2 && second->justify == Target::TextJustify::Right,
        "Coda TextBlocks did not follow the HS/HT structural order");
    expectMapping(first->lineSpacingPercentage == 100 && !first->newPos36
            && first->shapeId == 0 && !first->showShape && first->wordWrap
            && !first->noExpandSingleWord && !second->noExpandSingleWord
            && !first->roundCorners && first->cornerRadius == 0,
        "Coda TextBlock behavior was not synthesized");
    expectMapping(field(report, "others.textBlock[1].textId").origin == ValueOrigin::LegacyMus
            && field(report, "others.textBlock[1].newPos36").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].shapeId").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].showShape").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].noExpandSingleWord").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].wordWrap").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].roundCorners").origin
                == ValueOrigin::LegacyBehavior
            && field(report, "others.textBlock[1].cornerRadius").origin
                == ValueOrigin::LegacyBehavior,
        "Coda stored identity and era behavior were not reported separately");
}

TEST_CASE("Stored text blocks span three epochs", "[class]") { testStoredTextBlocksAcrossEpochs(); }
TEST_CASE("Coda text blocks are assembled from text structure", "[class]") { testCodaTextBlockSynthesis(); }

TEST_CASE("Pre-Finale-3.7 block texts align by semantic content", "[coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto snapshot = [](std::int64_t number) {
        Value::Object item;
        item.emplace("number", number);
        item.emplace("text", "^fontid(1)^size(12)^nfx(0)Staff Name");
        Value::Array items;
        items.emplace_back(std::move(item));
        SurveySnapshot result;
        result.emplace("block_texts", std::move(items));
        return result;
    };
    const auto document = [](musx::dom::Cmper number) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto building = session.getDocument();
        auto fontDefinition = std::make_shared<musx::dom::others::FontDefinition>(building,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, 1);
        fontDefinition->name = "Times New Roman";
        building->getOthers()->add(
            musx::dom::others::FontDefinition::XmlNodeName, std::move(fontDefinition));
        auto fontOptions = std::make_shared<musx::dom::options::FontOptions>(building);
        auto textBlockFont = std::make_shared<musx::dom::FontInfo>(building);
        textBlockFont->fontId = 1;
        textBlockFont->fontSize = 12;
        fontOptions->fontOptions.emplace(
            musx::dom::options::FontOptions::FontType::TextBlock, std::move(textBlockFont));
        building->getOptions()->add(
            musx::dom::options::FontOptions::XmlNodeName, std::move(fontOptions));
        auto textBlock = std::make_shared<musx::dom::others::TextBlock>(building,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, number);
        textBlock->textId = number;
        textBlock->textType = musx::dom::others::TextBlock::TextType::Block;
        textBlock->lineSpacingPercentage = 100;
        building->getOthers()->add(
            musx::dom::others::TextBlock::XmlNodeName, std::move(textBlock));
        auto staff = std::make_shared<musx::dom::others::Staff>(building,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, 1);
        staff->fullNameTextId = number;
        staff->staffLines = 5;
        building->getOthers()->add(musx::dom::others::Staff::XmlNodeName, std::move(staff));
        return std::move(session).finish();
    };
    const auto sourceDocument = document(1);
    const auto companionDocument = document(2);
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const SourceVersion finale35{.major = 3, .minor = 5};
    const SourceVersion finale37{.major = 3, .minor = 7};
    const auto compareAt = [&](const SourceVersion* version) {
        return compareSnapshots(snapshot(1), snapshot(2), sourceDocument, companionDocument,
            FormatEpoch::UncompressedLegacy, ByteOrder::BigEndian, version, report);
    };

    const auto early = compareAt(&finale35);
    CHECK(early.transformations.at(
              ComparisonTransformation::SemanticallyPairedPreFinale37BlockText) == 1);
    CHECK(early.classes.at("texts").at("block_texts").same == 2);
    CHECK(early.unexpectedExamples.empty());

    const auto later = compareAt(&finale37);
    CHECK_FALSE(later.transformations.contains(
        ComparisonTransformation::SemanticallyPairedPreFinale37BlockText));
    CHECK(later.classes.at("texts").at("block_texts").sourceOnly == 2);
    CHECK(later.classes.at("texts").at("block_texts").companionOnly == 2);
}

} // namespace
} // namespace finale_mus_reader_tests
