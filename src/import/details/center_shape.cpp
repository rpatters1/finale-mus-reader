// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/details.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "import/shared/smart_shape_adjustment_flags.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace details {
namespace {

using CenterShapeTarget = musx::dom::details::CenterShape;
constexpr auto centerShapeTag = records::packTag("Cx");
constexpr records::LegacyTag centerShapeClass = 0x0406;
constexpr std::size_t centerShapeWords = 15;

void reportCenterShape(const ImportContext& context, const RecordFamilySource& source, std::span<const records::LegacyRow> rows,
    std::span<const std::int16_t> words, const CenterShapeTarget& target)
{
    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
        const auto key =
            reporting.template instanceKey<CenterShapeTarget>(target.getSourcePartId(), target.getCmper1(), std::nullopt, target.getCmper2());
        reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
        const auto stored = [&](const char* member, std::size_t slot, std::int64_t value) {
            reportLegacyField(reporting, key, source, source.rowOfWord(rows, slot), member, source.byteOffsetInRow(slot * 2), value);
        };
        stored("startBreakAdj.horzOffset", 0, target.startBreakAdj->horzOffset);
        stored("startBreakAdj.vertOffset", 1, target.startBreakAdj->vertOffset);
        for (const auto* member : {"startBreakAdj.active", "startBreakAdj.contextDir", "startBreakAdj.contextEntCnct"}) {
            stored(member, 2, words[2]);
        }
        stored("endBreakAdj.horzOffset", 3, target.endBreakAdj->horzOffset);
        stored("endBreakAdj.vertOffset", 4, target.endBreakAdj->vertOffset);
        for (const auto* member : {"endBreakAdj.active", "endBreakAdj.contextDir", "endBreakAdj.contextEntCnct"}) {
            stored(member, 5, words[5]);
        }
        stored("ctlPtAdj.startCtlPtX", 6, target.ctlPtAdj->startCtlPtX);
        stored("ctlPtAdj.startCtlPtY", 7, target.ctlPtAdj->startCtlPtY);
        stored("ctlPtAdj.endCtlPtX", 8, target.ctlPtAdj->endCtlPtX);
        stored("ctlPtAdj.endCtlPtY", 9, target.ctlPtAdj->endCtlPtY);
        stored("ctlPtAdj.active", 10, words[10]);
        stored("ctlPtAdj.contextDir", 10, words[10]);
    });
}

void importCenterShapeFamily(const ImportContext& context, const RecordFamilySource& source)
{
    for (const auto& [partId, shapeId] : recordKeys(source)) {
        for (const auto centerId : source.pool->secondCmpersForTag(source.identity, shapeId, partId)) {
            const auto rows = source.pool->getArray(source.identity, shapeId, centerId, partId);
            const auto words = collectRecordWords(source, rows, context.profile.byteOrder);
            if (words.size() < centerShapeWords) {
                context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                    "Center shape for shape " + std::to_string(shapeId) + ", center " + std::to_string(centerId) + " is truncated."});
                continue;
            }
            auto target = createDetailsRecordTarget<CenterShapeTarget>(context.document, source, rows.front(), shapeId, centerId);
            target->integrityCheck(target);
            target->startBreakAdj->horzOffset = words[0];
            target->startBreakAdj->vertOffset = words[1];
            target->startBreakAdj->active = smart_shape_adjustment_flags::active(words[2]);
            target->startBreakAdj->contextDir = smart_shape_adjustment_flags::direction(words[2]);
            target->startBreakAdj->contextEntCnct = smart_shape_adjustment_flags::entryConnection(words[2]);
            target->endBreakAdj->horzOffset = words[3];
            target->endBreakAdj->vertOffset = words[4];
            target->endBreakAdj->active = smart_shape_adjustment_flags::active(words[5]);
            target->endBreakAdj->contextDir = smart_shape_adjustment_flags::direction(words[5]);
            target->endBreakAdj->contextEntCnct = smart_shape_adjustment_flags::entryConnection(words[5]);
            target->ctlPtAdj->startCtlPtX = words[6];
            target->ctlPtAdj->startCtlPtY = words[7];
            target->ctlPtAdj->endCtlPtX = words[8];
            target->ctlPtAdj->endCtlPtY = words[9];
            target->ctlPtAdj->active = smart_shape_adjustment_flags::active(words[10]);
            target->ctlPtAdj->contextDir = smart_shape_adjustment_flags::direction(words[10]);
            reportCenterShape(context, source, rows, words, *target);
            context.document->getDetails()->add(CenterShapeTarget::XmlNodeName, std::move(target));
        }
    }
}

} // namespace

void importCenterShapes(const ImportContext& context)
{
    const auto source =
        selectRecordFamilySource(context, context.index.getDetails(), context.index.getClassDetails(), centerShapeTag, centerShapeClass, true);
    if (source) {
        importCenterShapeFamily(context, *source);
    }
}

} // namespace details
} // namespace finale_mus_reader
