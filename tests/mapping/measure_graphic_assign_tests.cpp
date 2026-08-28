// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "mapping_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace mapping;

void testMeasureGraphicAssignmentsAcrossEpochs()
{
    using Target = musx::dom::details::MeasureGraphicAssign;
    const std::vector<std::int16_t> tuple{
        0x100, 120, -324, 336, 168, 1, 0, 1, 393, 0,
        0, 1, 336, 168, 0, 0, 0, 1, 0, 0};
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        const auto parsed = epoch == FormatEpoch::ZlibLegacy
            ? makeDetailClassContainer(1, 2, 0, tuple, ByteOrder::LittleEndian)
            : makeDetailContainer(epoch, 1, 2, tuple);
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
        finale_mus_reader::details::importMeasureGraphicAssignments(context);
        const auto assignment = document->getDetails()->get<Target>(
            musx::dom::SCORE_PARTID, 1, 2, musx::dom::Inci(0));
        expectMapping(assignment && assignment->version == 0x100
                && assignment->left == 120 && assignment->bottom == -324
                && assignment->width == 336 && assignment->height == 168
                && assignment->fDescId == 1 && !assignment->hidden
                && assignment->hAlign == Target::HorizontalAlignment::Left
                && assignment->vAlign == Target::VerticalAlignment::Top
                && assignment->posFrom == Target::PositionFrom::PageEdge
                && assignment->fixedPerc
                && assignment->savedRecord && assignment->origWidth == 336
                && assignment->origHeight == 168 && assignment->graphicCmper == 1,
            "A MeasureGraphicAssign field or comparator failed in epoch "
                + std::to_string(static_cast<int>(epoch)));
        expectMapping(reportedFieldCount(report) == 15,
            "A MeasureGraphicAssign did not report every persisted field");
        const auto instance = finale_mus_reader::instanceKey<Target>(
            musx::dom::SCORE_PARTID, musx::dom::Cmper(1), musx::dom::Inci(0),
            musx::dom::Cmper(2));
        for (const auto* member : {"hAlign", "vAlign", "posFrom", "fixedPerc"}) {
            const auto* info = report.findField(instance, member);
            expectMapping(info && info->origin == ValueOrigin::LegacyMus
                    && info->rawValue == 393,
                std::string("MeasureGraphicAssign did not recover ") + member
                    + " from the packed positioning word");
        }
    }
    const auto bigEndian = makeDetailClassContainer(7, 12, 2, tuple, ByteOrder::BigEndian);
    const auto bigEndianIndex = LegacyRecordIndex::build(bigEndian);
    const auto bigEndianRows = bigEndianIndex.getClassDetails().getArray(0x041d, 7, 12, 2);
    expectMapping(bigEndianRows.size() == 1 && bigEndianRows.front().partId == 2
            && bigEndianRows.front().inci == 0
            && bigEndianIndex.getClassDetails().get(0x041d, 7, 12, 0) == nullptr
            && bigEndianIndex.getClassDetails().get(0x041d, 7, 12, 0, 2)
                == &bigEndianRows.front()
            && bigEndianIndex.getClassDetails().payloadOf(bigEndianRows.front()).size() == 40,
        "A big-endian zlib detail did not preserve cmper2, part id, and payload length");

    auto partSession = musx::factory::DocumentFactory::begin();
    const auto partDocument = partSession.getDocument();
    auto partReferenceSession = musx::factory::DocumentFactory::begin();
    const auto partReference = std::move(partReferenceSession).finish();
    ImportReport partReport(FormatEpoch::UncompressedLegacy);
    finale_mus_reader::PendingReferences partPending;
    SourceProfile partProfile(FormatEpoch::ZlibLegacy);
    partProfile.byteOrder = ByteOrder::BigEndian;
    musx::factory::ConstructionContext partConstruction;
    const finale_mus_reader::ImportContext partContext{bigEndianIndex, partProfile, noSource,
        partDocument, partReference, partReport, partPending, partConstruction};
    finale_mus_reader::details::importMeasureGraphicAssignments(partContext);
    expectMapping(!partDocument->getDetails()->get<Target>(
            musx::dom::SCORE_PARTID, 7, 12, musx::dom::Inci(0)),
        "A part-owned zlib detail was imported into the score pool");
}

TEST_CASE("Measure graphic assignments span four epochs", "[mapping]") { testMeasureGraphicAssignmentsAcrossEpochs(); }

} // namespace
} // namespace finale_mus_reader_tests
