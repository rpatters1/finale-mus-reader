// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "import/shared/smart_shape_adjustment_flags.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using Shape = musx::dom::others::SmartShape;
constexpr records::LegacyTag smartShapeTag = records::packTag("Sx");
constexpr records::LegacyTag smartShapeClass = 0x00d9;
constexpr records::LegacyTag earlyStartTag = records::packTag("sX");
constexpr records::LegacyTag earlyEndTag = records::packTag("eX");
constexpr records::LegacyTag earlyEndpointTag = records::packTag("DY");
constexpr records::LegacyTag earlyOffsetTag = records::packTag("oX");

void importEarlySmartShapes(const ImportContext& context)
{
    if (context.profile.epoch != FormatEpoch::CodaBanner && context.profile.epoch != FormatEpoch::UncompressedLegacy) {
        return;
    }
    const auto& pool = context.index.getOthers();
    const RecordFamilySource startSource{&pool, earlyStartTag, false};
    const RecordFamilySource endSource{&pool, earlyEndTag, false};
    const RecordFamilySource endpointSource{&pool, earlyEndpointTag, false};
    const RecordFamilySource offsetSource{&pool, earlyOffsetTag, false};
    // Believed: matching DY rows give the endpoint measure, staff, and position.
    for (const auto shapeId : pool.cmpersForTag(earlyStartTag)) {
        if (shapeId == 0) {
            continue;
        }
        const auto starts = pool.getArray(earlyStartTag, shapeId);
        const auto ends = pool.getArray(earlyEndTag, shapeId);
        if (starts.size() != 1 || ends.size() != 1 || starts.front().words[1] != ends.front().words[1] || starts.front().words[1] < 0
            || starts.front().words[1] > 7) {
            continue;
        }
        std::vector<std::pair<std::uint16_t, const records::LegacyRow*>> endpoints;
        for (const auto measure : pool.cmpersForTag(earlyEndpointTag)) {
            for (const auto& row : pool.getArray(earlyEndpointTag, measure)) {
                if (row.words[0] == shapeId) {
                    endpoints.emplace_back(measure, &row);
                }
            }
        }
        if (endpoints.size() != 2) {
            continue;
        }
        const auto& startRow = starts.front();
        const auto& endRow = ends.front();
        const auto& [startMeasure, startEndpoint] = endpoints[0];
        const auto& [endMeasure, endEndpoint] = endpoints[1];
        const bool chronologicalMarkers = startRow.words[0] == startMeasure && endRow.words[0] == endMeasure;
        const bool reversedMarkers = !chronologicalMarkers && startRow.words[0] == endMeasure && endRow.words[0] == startMeasure;
        if (!chronologicalMarkers && !reversedMarkers) {
            continue;
        }
        auto shape = createOthersRecordTarget<Shape>(context.document, startSource, startRow, shapeId);
        shape->integrityCheck(shape);
        shape->shapeType = static_cast<Shape::ShapeType>(startRow.words[1]);
        shape->makeHorz = false;
        shape->noPushEndStart = false;
        shape->startTermSeg->endPoint->staffId = static_cast<std::uint16_t>(startEndpoint->words[5]) & 0x00ff;
        shape->startTermSeg->endPoint->measId = startMeasure;
        shape->startTermSeg->endPoint->eduPosition = startEndpoint->words[1];
        shape->startTermSeg->endPointAdj->vertOffset = startEndpoint->words[2];
        shape->endTermSeg->endPoint->staffId = static_cast<std::uint16_t>(endEndpoint->words[5]) & 0x00ff;
        shape->endTermSeg->endPoint->measId = endMeasure;
        shape->endTermSeg->endPoint->eduPosition = endEndpoint->words[1];
        shape->endTermSeg->endPointAdj->vertOffset = endEndpoint->words[2];
        // Believed: an early adjustment is active when its offset record is present.
        shape->startTermSeg->endPointAdj->active = true;
        shape->endTermSeg->endPointAdj->active = true;
        const auto offsetRows = pool.getArray(earlyOffsetTag, shapeId);
        const auto* offsetRow = offsetRows.size() == 1 ? &offsetRows.front() : nullptr;
        if (offsetRow) {
            shape->startTermSeg->breakAdj->active = true;
            shape->endTermSeg->breakAdj->active = true;
            shape->startTermSeg->endPointAdj->horzOffset = offsetRow->words[0];
            shape->startTermSeg->breakAdj->horzOffset = offsetRow->words[1];
            shape->startTermSeg->breakAdj->vertOffset = offsetRow->words[2];
            shape->endTermSeg->endPointAdj->horzOffset = offsetRow->words[3];
            shape->endTermSeg->breakAdj->horzOffset = offsetRow->words[4];
            shape->endTermSeg->breakAdj->vertOffset = offsetRow->words[5];
        }
        withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
            const auto key = reporting.template instanceKey<Shape>(startRow.partId, shapeId);
            const auto field = [&](const RecordFamilySource& source, const records::LegacyRow& row, const char* member, std::size_t offset,
                                   std::int64_t value) { reportLegacyField(reporting, key, source, row, member, offset, value); };
            field(startSource, startRow, "shapeType", 2, startRow.words[1]);
            field(endpointSource, *startEndpoint, "startTermSeg.endPoint.staffId", 10, shape->startTermSeg->endPoint->staffId);
            field(reversedMarkers ? endSource : startSource, reversedMarkers ? endRow : startRow, "startTermSeg.endPoint.measId", 0, startMeasure);
            field(endpointSource, *startEndpoint, "startTermSeg.endPoint.eduPosition", 2, shape->startTermSeg->endPoint->eduPosition);
            field(endpointSource, *startEndpoint, "startTermSeg.endPointAdj.vertOffset", 4, shape->startTermSeg->endPointAdj->vertOffset);
            field(endpointSource, *endEndpoint, "endTermSeg.endPoint.staffId", 10, shape->endTermSeg->endPoint->staffId);
            field(reversedMarkers ? startSource : endSource, reversedMarkers ? startRow : endRow, "endTermSeg.endPoint.measId", 0, endMeasure);
            field(endpointSource, *endEndpoint, "endTermSeg.endPoint.eduPosition", 2, shape->endTermSeg->endPoint->eduPosition);
            field(endpointSource, *endEndpoint, "endTermSeg.endPointAdj.vertOffset", 4, shape->endTermSeg->endPointAdj->vertOffset);
            for (const char* member : {"makeHorz", "noPushEndStart"}) {
                reportFallbackField(reporting, key, member, Reporting::Origin::LegacyBehavior, false);
            }
            if (offsetRow) {
                field(offsetSource, *offsetRow, "startTermSeg.endPointAdj.horzOffset", 0, shape->startTermSeg->endPointAdj->horzOffset);
                field(offsetSource, *offsetRow, "startTermSeg.breakAdj.horzOffset", 2, shape->startTermSeg->breakAdj->horzOffset);
                field(offsetSource, *offsetRow, "startTermSeg.breakAdj.vertOffset", 4, shape->startTermSeg->breakAdj->vertOffset);
                field(offsetSource, *offsetRow, "endTermSeg.endPointAdj.horzOffset", 6, shape->endTermSeg->endPointAdj->horzOffset);
                field(offsetSource, *offsetRow, "endTermSeg.breakAdj.horzOffset", 8, shape->endTermSeg->breakAdj->horzOffset);
                field(offsetSource, *offsetRow, "endTermSeg.breakAdj.vertOffset", 10, shape->endTermSeg->breakAdj->vertOffset);
            }
            reportFallbackField(reporting, key, "startTermSeg.endPointAdj.active", Reporting::Origin::LegacyBehavior, true);
            reportFallbackField(reporting, key, "endTermSeg.endPointAdj.active", Reporting::Origin::LegacyBehavior, true);
            if (offsetRow) {
                reportFallbackField(reporting, key, "startTermSeg.breakAdj.active", Reporting::Origin::LegacyBehavior, true);
                reportFallbackField(reporting, key, "endTermSeg.breakAdj.active", Reporting::Origin::LegacyBehavior, true);
            }
            reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
        });
        context.document->getOthers()->add(Shape::XmlNodeName, std::move(shape));
    }
}

bool smartShapeIsEntryBased(const void* instance)
{
    const auto* shape = static_cast<const Shape*>(instance);
    return shape->entryBased || shape->calcIsLyricShape();
}

bool smartShapeIsBeatBased(const void* instance)
{
    return !smartShapeIsEntryBased(instance);
}

std::optional<musx::dom::LyricTextType> lyricTypeFromBytes(std::span<const std::uint8_t> payload, std::size_t offset)
{
    // Fixed records use two-letter text tags; compressed records can encode the same kind numerically.
    if (payload.size() <= offset + 1) {
        return std::nullopt;
    }
    if ((payload[offset] == 0x04 && payload[offset + 1] == 0x1a) || (payload[offset] == 0x1a && payload[offset + 1] == 0x04)
        || (payload[offset] == 0x54 && payload[offset + 1] == 0x04)) {
        return musx::dom::LyricTextType::Verse;
    }
    if ((payload[offset] == 0x04 && payload[offset + 1] == 0x18) || (payload[offset] == 0x18 && payload[offset + 1] == 0x04)
        || (payload[offset] == 0x52 && payload[offset + 1] == 0x04)) {
        return musx::dom::LyricTextType::Chorus;
    }
    if ((payload[offset] == 0x04 && payload[offset + 1] == 0x19) || (payload[offset] == 0x19 && payload[offset + 1] == 0x04)
        || (payload[offset] == 0x53 && payload[offset + 1] == 0x04)) {
        return musx::dom::LyricTextType::Section;
    }
    if (payload[offset] == 'v' && payload[offset + 1] == 'e') {
        return musx::dom::LyricTextType::Verse;
    }
    if (payload[offset] == 'c' && payload[offset + 1] == 'h') {
        return musx::dom::LyricTextType::Chorus;
    }
    if (payload[offset] == 's' && payload[offset + 1] == 'e') {
        return musx::dom::LyricTextType::Section;
    }
    return std::nullopt;
}

MappingTarget createSmartShape(
    const musx::dom::DocumentPtr& document, const RecordFamilySource& source, const records::LegacyRow& row, std::uint16_t cmper)
{
    if (cmper == 0) {
        return {};
    }
    const auto payloadSize = source.pool->payloadOf(row).size();
    if (payloadSize != 84 && payloadSize != 96) {
        return {};
    }
    auto shape = createOthersRecordTarget<Shape>(document, source, row, cmper);
    shape->integrityCheck(shape);
    auto* raw = shape.get();
    document->getOthers()->add(Shape::XmlNodeName, std::move(shape));
    return makeMappingTarget(row.partId, cmper, raw);
}

MappingTarget createFixedSmartShape(
    const musx::dom::DocumentPtr& document, const RecordFamilySource& source, const records::LegacyRow& row, std::uint16_t cmper)
{
    if (cmper == 0) {
        return {};
    }
    const auto rows = source.pool->getArray(source.identity, cmper, 0, row.partId);
    if (rows.size() != 7 && rows.size() != 8) {
        return {};
    }
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (rows[i].inci != i || source.pool->payloadOf(rows[i]).size() != 12) {
            return {};
        }
    }
    auto shape = createOthersRecordTarget<Shape>(document, source, row, cmper);
    shape->integrityCheck(shape);
    auto* raw = shape.get();
    document->getOthers()->add(Shape::XmlNodeName, std::move(shape));
    return makeMappingTarget(row.partId, cmper, raw);
}

// The low half of each four-byte value precedes its high half in this layout.
// The record is 84 or 96 bytes; the latter has lyric fields and four bytes of row padding.
const FieldMapping smartShapeFields[] = {
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 0, nullptr, shapeType, static_cast<Shape::ShapeType>(value)),
    MUS_CLASS_FIELD_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 2, ValueWidth::Long, LongWordOrder::LowFirst, (BitRange{30, 1}), nullptr, entryBased),
    MUS_CLASS_FIELD_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 2, ValueWidth::Long, LongWordOrder::LowFirst, (BitRange{29, 1}), nullptr, rotate),
    MUS_CLASS_FIELD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 2, ValueWidth::Long, LongWordOrder::LowFirst, (BitRange{15, 1}), nullptr,
        noPresetShape, value == 0),
    MUS_CLASS_BIT(Shape, smartShapeClass, CMPER_FROM_TARGET, 2, 10, makeHorz),
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 2, nullptr, noPushEndStart, (value & 0x0200) == 0),
    MUS_CLASS_BIT(Shape, smartShapeClass, CMPER_FROM_TARGET, 2, 8, makeVert),
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 2, nullptr, engraverSlurState,
        (value & 0x10)   ? Shape::EngraverSlurState::Off
        : (value & 0x20) ? Shape::EngraverSlurState::On
                         : Shape::EngraverSlurState::Auto),
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 2, nullptr, slurAvoidAcciState,
        (value & 0x04)   ? Shape::SlurAvoidAccidentalsState::Off
        : (value & 0x08) ? Shape::SlurAvoidAccidentalsState::On
                         : Shape::SlurAvoidAccidentalsState::Auto),
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 2, nullptr, yBreakType,
        (value & 0x01) ? Shape::SystemBreakType::Opposite : Shape::SystemBreakType::Same),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 6, startTermSeg->endPoint->staffId),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 8, startTermSeg->endPoint->measId),
    MUS_CLASS_LONG_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 10, LongWordOrder::LowFirst, &smartShapeIsEntryBased, startTermSeg->endPoint->entryNumber),
    MUS_CLASS_LONG_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 10, LongWordOrder::LowFirst, &smartShapeIsBeatBased, startTermSeg->endPoint->eduPosition),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 14, startTermSeg->endPointAdj->horzOffset),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 16, startTermSeg->endPointAdj->vertOffset),
    MUS_CLASS_BIT(Shape, smartShapeClass, CMPER_FROM_TARGET, 18, smart_shape_adjustment_flags::activeBit, startTermSeg->endPointAdj->active),
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 18, nullptr, startTermSeg->endPointAdj->contextDir,
        smart_shape_adjustment_flags::direction(value)),
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 18, nullptr, startTermSeg->endPointAdj->contextEntCnct,
        smart_shape_adjustment_flags::entryConnection(value)),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 20, startTermSeg->ctlPtAdj->startCtlPtX),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 22, startTermSeg->ctlPtAdj->startCtlPtY),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 24, startTermSeg->ctlPtAdj->endCtlPtX),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 26, startTermSeg->ctlPtAdj->endCtlPtY),
    MUS_CLASS_BIT(Shape, smartShapeClass, CMPER_FROM_TARGET, 28, smart_shape_adjustment_flags::activeBit, startTermSeg->ctlPtAdj->active),
    MUS_CLASS_WORD_AS_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 28, nullptr, startTermSeg->ctlPtAdj->contextDir, smart_shape_adjustment_flags::direction(value)),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 30, startTermSeg->breakAdj->horzOffset),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 32, startTermSeg->breakAdj->vertOffset),
    MUS_CLASS_BIT(Shape, smartShapeClass, CMPER_FROM_TARGET, 34, smart_shape_adjustment_flags::activeBit, startTermSeg->breakAdj->active),
    MUS_CLASS_WORD_AS_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 34, nullptr, startTermSeg->breakAdj->contextDir, smart_shape_adjustment_flags::direction(value)),
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 34, nullptr, startTermSeg->breakAdj->contextEntCnct,
        smart_shape_adjustment_flags::entryConnection(value)),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 36, endTermSeg->endPoint->staffId),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 38, endTermSeg->endPoint->measId),
    MUS_CLASS_LONG_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 40, LongWordOrder::LowFirst, &smartShapeIsEntryBased, endTermSeg->endPoint->entryNumber),
    MUS_CLASS_LONG_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 40, LongWordOrder::LowFirst, &smartShapeIsBeatBased, endTermSeg->endPoint->eduPosition),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 44, endTermSeg->endPointAdj->horzOffset),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 46, endTermSeg->endPointAdj->vertOffset),
    MUS_CLASS_BIT(Shape, smartShapeClass, CMPER_FROM_TARGET, 48, smart_shape_adjustment_flags::activeBit, endTermSeg->endPointAdj->active),
    MUS_CLASS_WORD_AS_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 48, nullptr, endTermSeg->endPointAdj->contextDir, smart_shape_adjustment_flags::direction(value)),
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 48, nullptr, endTermSeg->endPointAdj->contextEntCnct,
        smart_shape_adjustment_flags::entryConnection(value)),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 50, endTermSeg->ctlPtAdj->startCtlPtX),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 52, endTermSeg->ctlPtAdj->startCtlPtY),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 54, endTermSeg->ctlPtAdj->endCtlPtX),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 56, endTermSeg->ctlPtAdj->endCtlPtY),
    MUS_CLASS_BIT(Shape, smartShapeClass, CMPER_FROM_TARGET, 58, smart_shape_adjustment_flags::activeBit, endTermSeg->ctlPtAdj->active),
    MUS_CLASS_WORD_AS_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 58, nullptr, endTermSeg->ctlPtAdj->contextDir, smart_shape_adjustment_flags::direction(value)),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 60, endTermSeg->breakAdj->horzOffset),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 62, endTermSeg->breakAdj->vertOffset),
    MUS_CLASS_BIT(Shape, smartShapeClass, CMPER_FROM_TARGET, 64, smart_shape_adjustment_flags::activeBit, endTermSeg->breakAdj->active),
    MUS_CLASS_WORD_AS_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 64, nullptr, endTermSeg->breakAdj->contextDir, smart_shape_adjustment_flags::direction(value)),
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 64, nullptr, endTermSeg->breakAdj->contextEntCnct,
        smart_shape_adjustment_flags::entryConnection(value)),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 66, fullCtlPtAdj->startCtlPtX),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 68, fullCtlPtAdj->startCtlPtY),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 70, fullCtlPtAdj->endCtlPtX),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 72, fullCtlPtAdj->endCtlPtY),
    MUS_CLASS_BIT(Shape, smartShapeClass, CMPER_FROM_TARGET, 74, smart_shape_adjustment_flags::activeBit, fullCtlPtAdj->active),
    MUS_CLASS_WORD_AS_IF(
        Shape, smartShapeClass, CMPER_FROM_TARGET, 74, nullptr, fullCtlPtAdj->contextDir, smart_shape_adjustment_flags::direction(value)),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 76, startNoteId),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 78, endNoteId),
    MUS_CLASS_WORD(Shape, smartShapeClass, CMPER_FROM_TARGET, 80, lineStyleId),
    MUS_CLASS_WORD_AS_IF(Shape, smartShapeClass, CMPER_FROM_TARGET, 82, nullptr, direction,
        (value & 0x2000)   ? musx::dom::ShapeDirection::Over
        : (value & 0x1000) ? musx::dom::ShapeDirection::Under
                           : musx::dom::ShapeDirection::Automatic),
    MUS_CLASS_BIT(Shape, smartShapeClass, CMPER_FROM_TARGET, 82, 0, hidden),
};

const auto fixedSmartShapeFields = [] {
    std::array<FieldMapping, std::size(smartShapeFields)> result{};
    std::copy(std::begin(smartShapeFields), std::end(smartShapeFields), result.begin());
    // Fixed rows address words in one incidence stream; class records address bytes.
    for (auto& field : result) {
        field.source.identity = smartShapeTag;
        field.source.wordSlot /= 2;
    }
    return result;
}();

const MappingTable& fixedSmartShapeTable()
{
    static const MappingTable table{.reportPrefix = "others.smartShape",
        .epochs = EpochMask::FixedRow,
        .targetKind = TargetKind::OthersFromRecords,
        .recordIdentity = smartShapeTag,
        .createTarget = &createFixedSmartShape,
        .fields = fixedSmartShapeFields.data(),
        .fieldCount = fixedSmartShapeFields.size()};
    return table;
}

const MappingTable& smartShapeTable()
{
    static const MappingTable table{.reportPrefix = "others.smartShape",
        .epochs = EpochMask::Zlib,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OthersFromRecords,
        .recordIdentity = smartShapeClass,
        .createTarget = &createSmartShape,
        .fields = smartShapeFields,
        .fieldCount = std::size(smartShapeFields)};
    return table;
}

void importLyricTypes(const ImportContext& context)
{
    const auto source = selectRecordFamilySource(context, context.index.getOthers(), context.index.getClassOthers(), smartShapeTag, smartShapeClass);
    if (!source) {
        return;
    }
    for (const auto& [partId, cmper] : recordKeys(*source)) {
        const auto rows = source->pool->getArray(source->identity, cmper, 0, partId);
        if (rows.empty()) {
            continue;
        }
        const auto payload = collectRecordPayload(*source, rows);
        if (payload.size() < 92) {
            continue;
        }
        auto shape = std::const_pointer_cast<Shape>(context.document->getOthers()->get<Shape>(partId, cmper));
        if (!shape) {
            continue;
        }
        // Hyphens and word extensions carry lyric data even when the lyric-based flag is clear.
        if (!shape->calcIsLyricShape()) {
            withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                const auto key = reporting.template instanceKey<Shape>(partId, cmper);
                for (const char* member : {"startLyricNum", "endLyricNum", "startLyricType", "endLyricType"}) {
                    reportFallbackField(reporting, key, member, Reporting::Origin::LegacyBehavior, 0);
                }
            });
            continue;
        }
        for (const auto& [offset, member, target] :
            {std::tuple{std::size_t{84}, "startLyricNum", &Shape::startLyricNum}, std::tuple{std::size_t{86}, "endLyricNum", &Shape::endLyricNum}}) {
            const auto value = payloadWord(payload, offset, context.profile.byteOrder);
            shape.get()->*target = value;
            withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                const auto key = reporting.template instanceKey<Shape>(partId, cmper);
                const auto& row = source->rowOfByte(rows, offset);
                reportLegacyField(reporting, key, *source, row, member, source->byteOffsetInRow(offset), value);
            });
        }
        for (const auto& [offset, member] : {std::pair{std::size_t{88}, "startLyricType"}, std::pair{std::size_t{90}, "endLyricType"}}) {
            const auto decoded = lyricTypeFromBytes(payload, offset);
            if (!decoded) {
                continue;
            }
            if (offset == 88) {
                shape->startLyricType = decoded;
            } else {
                shape->endLyricType = decoded;
            }
            withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                const auto key = reporting.template instanceKey<Shape>(partId, cmper);
                const auto& row = source->rowOfByte(rows, offset);
                reportLegacyField(reporting, key, *source, row, member, source->byteOffsetInRow(offset), static_cast<int>(*decoded));
            });
        }
    }
}

void inheritZeroPartShapeMetadata(const ImportContext& context)
{
    // Believed: zeroed context in an inactive part adjustment, and a zeroed part slur mode,
    // inherit the score value even when the continuation mask marks those bits editable.
    if (context.profile.epoch != FormatEpoch::ZlibLegacy) {
        return;
    }
    const auto& pool = context.index.getClassOthers();
    for (const auto& source : context.document->getOthers()->getAllSources<Shape>()) {
        if (source->getSourcePartId() == musx::dom::SCORE_PARTID || source->getShareMode() != Shape::ShareMode::Partial) {
            continue;
        }
        const auto partId = source->getSourcePartId();
        const auto cmper = source->getCmper();
        const auto* row = pool.get(smartShapeClass, cmper, 0, 0, partId);
        const auto score = context.document->getOthers()->get<Shape>(musx::dom::SCORE_PARTID, cmper);
        if (!row || !score || !row->continuationOverlayReady) {
            continue;
        }
        const auto payload = pool.payloadOf(*row);
        const auto continuation = pool.continuationOf(*row);
        const auto zeroPartWord = [&](std::size_t offset) {
            return offset + 2 <= payload.size() && offset + 6 <= continuation.size() && payloadWord(payload, offset, context.profile.byteOrder) == 0
                   && payloadWord(continuation, offset + 4, context.profile.byteOrder) == 0xffff;
        };
        const auto reportAdjusted = [&](const char* member, std::size_t offset) {
            withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                const auto key = reporting.template instanceKey<Shape>(partId, cmper);
                reporting.report().setField(key, member, {Reporting::Origin::LegacyMusAdjusted, row->blockOffset, row->decodedOffset + offset, 0});
            });
        };
        auto part = std::const_pointer_cast<Shape>(source);
        const auto inheritDirection = [&](auto& partAdjustment, const auto& scoreAdjustment, std::size_t offset, const char* member) {
            if (zeroPartWord(offset) && !partAdjustment->active && scoreAdjustment->contextDir != musx::dom::smartshape::DirectionType::None) {
                partAdjustment->contextDir = scoreAdjustment->contextDir;
                reportAdjusted(member, offset);
            }
        };
        const auto inheritConnection = [&](auto& partAdjustment, const auto& scoreAdjustment, std::size_t offset, const char* member) {
            if (zeroPartWord(offset) && !partAdjustment->active
                && scoreAdjustment->contextEntCnct != musx::dom::smartshape::EntryConnectionType::HeadLeftTop) {
                partAdjustment->contextEntCnct = scoreAdjustment->contextEntCnct;
                reportAdjusted(member, offset);
            }
        };
        inheritDirection(part->startTermSeg->endPointAdj, score->startTermSeg->endPointAdj, 18, "startTermSeg.endPointAdj.contextDir");
        inheritConnection(part->startTermSeg->endPointAdj, score->startTermSeg->endPointAdj, 18, "startTermSeg.endPointAdj.contextEntCnct");
        inheritDirection(part->startTermSeg->ctlPtAdj, score->startTermSeg->ctlPtAdj, 28, "startTermSeg.ctlPtAdj.contextDir");
        inheritDirection(part->startTermSeg->breakAdj, score->startTermSeg->breakAdj, 34, "startTermSeg.breakAdj.contextDir");
        inheritConnection(part->startTermSeg->breakAdj, score->startTermSeg->breakAdj, 34, "startTermSeg.breakAdj.contextEntCnct");
        inheritDirection(part->endTermSeg->endPointAdj, score->endTermSeg->endPointAdj, 48, "endTermSeg.endPointAdj.contextDir");
        inheritConnection(part->endTermSeg->endPointAdj, score->endTermSeg->endPointAdj, 48, "endTermSeg.endPointAdj.contextEntCnct");
        inheritDirection(part->endTermSeg->ctlPtAdj, score->endTermSeg->ctlPtAdj, 58, "endTermSeg.ctlPtAdj.contextDir");
        inheritDirection(part->endTermSeg->breakAdj, score->endTermSeg->breakAdj, 64, "endTermSeg.breakAdj.contextDir");
        inheritConnection(part->endTermSeg->breakAdj, score->endTermSeg->breakAdj, 64, "endTermSeg.breakAdj.contextEntCnct");
        inheritDirection(part->fullCtlPtAdj, score->fullCtlPtAdj, 74, "fullCtlPtAdj.contextDir");

        if (payload.size() >= 4 && continuation.size() >= 8 && part->engraverSlurState == Shape::EngraverSlurState::Auto
            && score->engraverSlurState != Shape::EngraverSlurState::Auto && (payloadWord(payload, 2, context.profile.byteOrder) & 0x30) == 0
            && (payloadWord(continuation, 6, context.profile.byteOrder) & 0x30) == 0x30) {
            part->engraverSlurState = score->engraverSlurState;
            reportAdjusted("engraverSlurState", 2);
        }
    }
}

} // namespace

void importSmartShapes(const ImportContext& context)
{
    applyMappingTables({&fixedSmartShapeTable(), &smartShapeTable()}, context.index, context.profile, context.document, context.report);
    importEarlySmartShapes(context);
    importLyricTypes(context);
    inheritZeroPartShapeMetadata(context);
    for (const auto& shape : context.document->getOthers()->getAllSources<Shape>()) {
        withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
            const auto key = reporting.template instanceKey<Shape>(shape->getSourcePartId(), shape->getCmper());
            reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
            for (const char* name : {"shapeType", "entryBased", "rotate", "noPresetShape", "makeHorz", "noPushEndStart", "makeVert",
                     "engraverSlurState", "slurAvoidAcciState", "yBreakType", "hidden", "direction", "startNoteId", "endNoteId", "lineStyleId",
                     "startLyricNum", "endLyricNum", "startLyricType", "endLyricType"}) {
                reporting.unmappedField(key, name, 0);
            }
            for (const char* segment : {"startTermSeg", "endTermSeg"}) {
                for (const char* member : {"endPoint.staffId", "endPoint.measId", "endPoint.eduPosition", "endPoint.entryNumber",
                         "endPointAdj.horzOffset", "endPointAdj.vertOffset", "endPointAdj.active", "endPointAdj.contextDir",
                         "endPointAdj.contextEntCnct", "ctlPtAdj.startCtlPtX", "ctlPtAdj.startCtlPtY", "ctlPtAdj.endCtlPtX", "ctlPtAdj.endCtlPtY",
                         "ctlPtAdj.active", "ctlPtAdj.contextDir", "breakAdj.horzOffset", "breakAdj.vertOffset", "breakAdj.active",
                         "breakAdj.contextDir", "breakAdj.contextEntCnct"}) {
                    reporting.unmappedField(key, std::string(segment) + "." + member, 0);
                }
            }
            for (const char* member : {"startCtlPtX", "startCtlPtY", "endCtlPtX", "endCtlPtY", "active", "contextDir"}) {
                reporting.unmappedField(key, std::string("fullCtlPtAdj.") + member, 0);
            }
        });
    }
}

} // namespace others
} // namespace finale_mus_reader
