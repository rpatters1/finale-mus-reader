// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <cstddef>
#include <string>
#include <utility>

#include "import/support/percussion_records.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using DrumStaffTarget = musx::dom::others::DrumStaff;
using DrumStaffStyleTarget = musx::dom::others::DrumStaffStyle;

constexpr std::size_t whichDrumLibOffset = 0;
constexpr std::size_t whichDrumLibSize = 2;

template <typename Target>
void reportDrumStaffFamily(const ImportContext& context, const Target& target, const RecordFamilySource& source, const records::LegacyRow& row)
{
    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
        const auto key = reporting.template instanceKey<Target>(target.getSourcePartId(), target.getCmper());
        reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
        reporting.report().setField(key, "whichDrumLib",
            typename Reporting::FieldInfo{
                Reporting::Origin::LegacyMus, row.blockOffset, row.decodedOffset + whichDrumLibOffset, target.whichDrumLib, source.identity});
    });
}

template <typename Target>
void importDrumStaffFamily(const ImportContext& context, const RecordFamilySource& source, std::string_view diagnosticName)
{
    for (const auto& [partId, cmper] : recordKeys(source)) {
        const auto rows = source.pool->getArray(source.identity, cmper, 0, partId);
        if (rows.empty()) {
            continue;
        }
        const auto payload = collectRecordPayload(source, rows);
        if (payload.size() < whichDrumLibSize) {
            context.report.diagnostics.push_back(
                {musx::util::Logger::LogLevel::Info, std::string(diagnosticName) + " " + std::to_string(cmper) + " is shorter than its layout."});
            continue;
        }

        auto target = createOthersRecordTarget<Target>(context.document, source, rows.front(), cmper);
        if (!target) {
            continue;
        }
        target->whichDrumLib = payloadWord(payload, whichDrumLibOffset, context.profile.byteOrder);
        reportDrumStaffFamily(context, *target, source, rows.front());
        context.document->getOthers()->add(Target::XmlNodeName, std::move(target));
    }
}

} // namespace

void importDrumStaff(const ImportContext& context)
{
    // The Coda-banner epoch predates percussion maps.
    if (context.profile.epoch == FormatEpoch::CodaBanner) {
        return;
    }
    const auto drumStaffSource = selectRecordFamilySource(
        context, context.index.getOthers(), context.index.getClassOthers(), percussion_records::drumStaffTag, percussion_records::drumStaffClass);
    if (drumStaffSource) {
        importDrumStaffFamily<DrumStaffTarget>(context, *drumStaffSource, "Drum staff");
    }

    // Staff styles begin in Finale 2000.
    if (!sourceAtOrAfter(context.profile, FormatEpoch::UncompressedLegacy, versions::finale2000)) {
        return;
    }
    const auto drumStaffStyleSource = selectRecordFamilySource(context, context.index.getOthers(), context.index.getClassOthers(),
        percussion_records::drumStaffStyleTag, percussion_records::drumStaffStyleClass);
    if (drumStaffStyleSource) {
        importDrumStaffFamily<DrumStaffStyleTarget>(context, *drumStaffStyleSource, "Drum staff style");
    }
}

} // namespace others
} // namespace finale_mus_reader
