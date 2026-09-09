// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <cstddef>
#include <string>
#include <utility>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using DrumStaffTarget = musx::dom::others::DrumStaff;

constexpr records::LegacyTag fixedDrumStaffTag = records::packTag("DS");
constexpr records::LegacyTag drumStaffClass = 0x0084;
constexpr std::size_t whichDrumLibOffset = 0;
constexpr std::size_t whichDrumLibSize = 2;

void reportDrumStaff(const ImportContext &context, const DrumStaffTarget &target,
                     const RecordFamilySource &source, const records::LegacyRow &row) {
    withReporting(context.report, [&]<typename Reporting>(Reporting &reporting) {
        const auto key = reporting.template instanceKey<DrumStaffTarget>(target.getSourcePartId(),
                                                                         target.getCmper());
        reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
        reporting.report().setField(
            key, "whichDrumLib",
            typename Reporting::FieldInfo{Reporting::Origin::LegacyMus, row.blockOffset,
                                          row.decodedOffset + whichDrumLibOffset,
                                          target.whichDrumLib, source.identity});
    });
}

} // namespace

void importDrumStaff(const ImportContext &context) {
    // The Coda-banner epoch predates percussion maps.
    if (context.profile.epoch == FormatEpoch::CodaBanner)
        return;
    const auto selected =
        selectRecordFamilySource(context, context.index.getOthers(), context.index.getClassOthers(),
                                 fixedDrumStaffTag, drumStaffClass);
    if (!selected)
        return;
    const auto &source = *selected;

    for (const auto [partId, staffId] : recordKeys(source)) {
        const auto rows = source.pool->getArray(source.identity, staffId, 0, partId);
        if (rows.empty())
            continue;
        const auto payload = collectRecordPayload(source, rows);
        if (payload.size() < whichDrumLibSize) {
            context.report.diagnostics.push_back(
                {musx::util::Logger::LogLevel::Info,
                 "Drum staff " + std::to_string(staffId) + " is shorter than its layout."});
            continue;
        }

        auto target = createOthersRecordTarget<DrumStaffTarget>(context.document, source,
                                                                rows.front(), staffId);
        if (!target)
            continue;
        target->whichDrumLib = payloadWord(payload, whichDrumLibOffset, context.profile.byteOrder);
        reportDrumStaff(context, *target, source, rows.front());
        context.document->getOthers()->add(DrumStaffTarget::XmlNodeName, std::move(target));
    }
}

} // namespace others
} // namespace finale_mus_reader
