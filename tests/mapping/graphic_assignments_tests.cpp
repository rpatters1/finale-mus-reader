// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "mapping_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace mapping;

void testGraphicAssignmentsAcrossEpochs()
{
    using PageGraphicAssign = musx::dom::others::PageGraphicAssign;
    const std::vector<std::int16_t> tuple{
        0x100, 120, -48, 640, 320, 7, 0, 0x11, 0x014c,
        4, 4, 1, 1280, 640, 144, -24, 0x0192, 2};
    const std::vector<SyntheticRow> fixedRows{
        {4, "pg", {tuple[0], tuple[1], tuple[2], tuple[3], tuple[4], tuple[5]}},
        {4, "pg", {tuple[6], tuple[7], tuple[8], tuple[9], tuple[10], tuple[11]}},
        {4, "pg", {tuple[12], tuple[13], tuple[14], tuple[15], tuple[16], tuple[17]}}};

    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        const auto parsed = epoch == FormatEpoch::ZlibLegacy
            ? makeClassContainer(0x00bc, tuple, ByteOrder::BigEndian, 4)
            : makeContainer(fixedRows, epoch);
        const auto index = LegacyRecordIndex::build(parsed);
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto referenceSession = musx::factory::DocumentFactory::begin();
        const auto reference = std::move(referenceSession).finish();
        ImportReport report(FormatEpoch::UncompressedLegacy);
        finale_mus_reader::PendingReferences pending;
        SourceProfile profile(epoch);
        profile.byteOrder = parsed.byteOrder;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::others::importPageGraphicAssignments(context);
        const auto assignment = document->getOthers()
            ->get<PageGraphicAssign>(musx::dom::SCORE_PARTID, 4, musx::dom::Inci(0));
        expectMapping(assignment && assignment->version == 0x100
                && assignment->left == 120 && assignment->bottom == -48
                && assignment->width == 640 && assignment->height == 320
                && assignment->fDescId == 7 && assignment->hidden
                && assignment->displayType == PageGraphicAssign::PageAssignType::One
                && assignment->hAlign == PageGraphicAssign::HorizontalAlignment::Center
                && assignment->vAlign == PageGraphicAssign::VerticalAlignment::Top
                && assignment->posFrom == PageGraphicAssign::PositionFrom::Margins
                && assignment->fixedPerc && assignment->startPage == 4
                && assignment->endPage == 4 && assignment->savedRecord
                && assignment->origWidth == 1280 && assignment->origHeight == 640
                && assignment->rightPgLeft == 144 && assignment->rightPgBottom == -24
                && assignment->rightPgHAlign == PageGraphicAssign::HorizontalAlignment::Right
                && assignment->rightPgVAlign == PageGraphicAssign::VerticalAlignment::Bottom
                && assignment->rightPgPosFrom == PageGraphicAssign::PositionFrom::PageEdge
                && assignment->rightPgFixedPerc && assignment->graphicCmper == 2,
            "A PageGraphicAssign field failed in epoch "
                + std::to_string(static_cast<int>(epoch)));
        expectMapping(reportedFieldCount(report) == 24,
            "A PageGraphicAssign did not report every persisted field");
    }

    const std::array<std::pair<std::int16_t, PageGraphicAssign::PageAssignType>, 4>
        displayTypes{{
            {std::int16_t(0x0001), PageGraphicAssign::PageAssignType::One},
            {std::int16_t(0x0002), PageGraphicAssign::PageAssignType::AllPages},
            {std::int16_t(0x0004), PageGraphicAssign::PageAssignType::Odd},
            {std::int16_t(0x0008), PageGraphicAssign::PageAssignType::Even},
        }};
    std::vector<SyntheticRow> displayRows;
    for (const auto& [displayFlags, expected] : displayTypes) {
        (void)expected;
        auto displayTuple = tuple;
        displayTuple[7] = displayFlags;
        for (std::size_t at = 0; at < displayTuple.size();
                at += finale_mus_reader::records::otherWordCount) {
            SyntheticRow row{4, "pg", {}};
            for (std::size_t slot = 0;
                    slot < finale_mus_reader::records::otherWordCount; ++slot) {
                row.words[slot] = displayTuple[at + slot];
            }
            displayRows.push_back(row);
        }
    }
    const auto displayParsed = makeContainer(displayRows, FormatEpoch::UncompressedLegacy);
    const auto displayIndex = LegacyRecordIndex::build(displayParsed);
    auto displaySession = musx::factory::DocumentFactory::begin();
    const auto displayDocument = displaySession.getDocument();
    auto displayReferenceSession = musx::factory::DocumentFactory::begin();
    const auto displayReference = std::move(displayReferenceSession).finish();
    ImportReport displayReport(FormatEpoch::UncompressedLegacy);
    finale_mus_reader::PendingReferences displayPending;
    SourceProfile displayProfile(FormatEpoch::UncompressedLegacy);
    displayProfile.byteOrder = displayParsed.byteOrder;
    musx::factory::ConstructionContext displayConstruction;
    const finale_mus_reader::ImportContext displayContext{displayIndex, displayProfile, noSource,
        displayDocument, displayReference, displayReport, displayPending, displayConstruction};
    finale_mus_reader::others::importPageGraphicAssignments(displayContext);
    for (std::size_t index = 0; index < displayTypes.size(); ++index) {
        const auto assignment = displayDocument->getOthers()->get<PageGraphicAssign>(
            musx::dom::SCORE_PARTID, 4, musx::dom::Inci(static_cast<int>(index)));
        expectMapping(assignment && assignment->displayType == displayTypes[index].second
                && !assignment->hidden,
            "A page graphic display flag did not map to its page-selection type");
    }
}

void testEmbeddedGraphicFraming()
{
    finale_mus_reader::container::ParsedContainer parsed(FormatEpoch::ZlibLegacy);
    parsed.byteOrder = ByteOrder::LittleEndian;
    finale_mus_reader::container::DecodedBlock block;
    block.info.type = 0x0013;
    block.info.stored = true;
    const auto appendItem = [&](std::span<const std::uint8_t> payload) {
        block.data.push_back(9);
        block.data.push_back(0);
        const auto size = static_cast<std::uint32_t>(payload.size());
        for (int shift = 0; shift <= 24; shift += 8) {
            block.data.push_back(static_cast<std::uint8_t>(size >> shift));
        }
        block.data.insert(block.data.end(), payload.begin(), payload.end());
        block.data.insert(block.data.end(), {1, 0, 0, 0, 0});
    };
    const std::array<std::uint8_t, 8> png{0x89, 'P', 'N', 'G', 13, 10, 0x1a, 10};
    const std::array<std::uint8_t, 14> eps{'%', '!', 'P', 'S', '-', 'A', 'd', 'o', 'b', 'e',
        '-', '3', '.', '0'};
    const std::array<std::uint8_t, 4> unknown{'N', 'O', 'P', 'E'};
    appendItem(png);
    appendItem(eps);
    appendItem(unknown);
    parsed.blocks.push_back(std::move(block));
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const auto graphics = finale_mus_reader::recoverEmbeddedGraphics(parsed, report);
    expectMapping(graphics.size() == 2 && graphics.at(1).extension == "png"
            && graphics.at(1).bytes == std::vector<std::uint8_t>(png.begin(), png.end())
            && graphics.at(2).extension == "eps"
            && graphics.at(2).bytes == std::vector<std::uint8_t>(eps.begin(), eps.end())
            && std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                [](const auto& diagnostic) {
                    return diagnostic.level == musx::util::Logger::LogLevel::Info
                        && diagnostic.message.find(
                               "Embedded graphic 3 has an unrecognized file signature")
                            != std::string::npos;
                }),
        "The stored graphics block did not map encounter order to embedded cmper ids");

    auto unsupportedFooter = parsed;
    unsupportedFooter.blocks.front().data[6 + png.size()] = 2;
    ImportReport unsupportedFooterReport(FormatEpoch::UncompressedLegacy);
    const auto withoutFirst = finale_mus_reader::recoverEmbeddedGraphics(
        unsupportedFooter, unsupportedFooterReport);
    expectMapping(withoutFirst.size() == 1
            && std::any_of(unsupportedFooterReport.diagnostics.begin(),
                unsupportedFooterReport.diagnostics.end(), [](const auto& diagnostic) {
                    return diagnostic.level == musx::util::Logger::LogLevel::Info
                        && diagnostic.message.find(
                               "Embedded graphic 1 has an unsupported footer version")
                            != std::string::npos;
                }),
        "An unsupported graphic footer did not report its embedded graphic comparator");

    parsed.blocks.front().data.pop_back();
    ImportReport truncatedReport(FormatEpoch::UncompressedLegacy);
    const auto truncated = finale_mus_reader::recoverEmbeddedGraphics(parsed, truncatedReport);
    expectMapping(truncated.size() == 2
            && std::any_of(truncatedReport.diagnostics.begin(), truncatedReport.diagnostics.end(),
                [](const auto& diagnostic) {
                    return diagnostic.level == musx::util::Logger::LogLevel::Warning
                        && diagnostic.message.find("truncated") != std::string::npos;
                }),
        "A truncated embedded-graphic footer was not bounded and reported");
}

void testShapeGraphicAssignmentsAcrossEpochs()
{
    using ShapeGraphicAssign = musx::dom::others::ShapeGraphicAssign;
    const std::vector<std::int16_t> tuple{
        0x100, 800, -520, 64, 20, 1, 0, 1, 0x0189,
        0, 0, 1, 64, 20, 0, 0, 0, 3};
    const std::vector<SyntheticRow> fixedRows{
        {1, "sg", {tuple[0], tuple[1], tuple[2], tuple[3], tuple[4], tuple[5]}},
        {1, "sg", {tuple[6], tuple[7], tuple[8], tuple[9], tuple[10], tuple[11]}},
        {1, "sg", {tuple[12], tuple[13], tuple[14], tuple[15], tuple[16], tuple[17]}}};
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        const auto parsed = epoch == FormatEpoch::ZlibLegacy
            ? makeClassContainer(0x00d8, tuple, ByteOrder::BigEndian, 1)
            : makeContainer(fixedRows, epoch);
        const auto index = LegacyRecordIndex::build(parsed);
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto referenceSession = musx::factory::DocumentFactory::begin();
        const auto reference = std::move(referenceSession).finish();
        ImportReport report(FormatEpoch::UncompressedLegacy);
        finale_mus_reader::PendingReferences pending;
        SourceProfile profile(epoch);
        profile.byteOrder = parsed.byteOrder;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::others::importShapeGraphicAssignments(context);
        const auto assignment = ShapeGraphicAssign::findForGraphic(
            document, musx::dom::SCORE_PARTID, 3);
        expectMapping(assignment && assignment->getCmper() == 1
                && assignment->left == 800 && assignment->bottom == -520
                && assignment->width == 64 && assignment->height == 20
                && assignment->fDescId == 1 && !assignment->hidden
                && assignment->hAlign == ShapeGraphicAssign::HorizontalAlignment::Left
                && assignment->vAlign == ShapeGraphicAssign::VerticalAlignment::Top
                && assignment->fixedPerc && assignment->savedRecord
                && assignment->origWidth == 64 && assignment->origHeight == 20
                && assignment->graphicCmper == 3,
            "A ShapeGraphicAssign field failed in epoch "
                + std::to_string(static_cast<int>(epoch)));
        expectMapping(reportedFieldCount(report) == 14,
            "A ShapeGraphicAssign did not report every persisted field");
    }
}

TEST_CASE("Graphic assignments span four epochs", "[mapping]") { testGraphicAssignmentsAcrossEpochs(); }
TEST_CASE("Embedded graphic framing", "[mapping]") { testEmbeddedGraphicFraming(); }
TEST_CASE("Shape graphic assignments span four epochs", "[mapping]") { testShapeGraphicAssignmentsAcrossEpochs(); }

} // namespace
} // namespace finale_mus_reader_tests
