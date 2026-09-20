// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using BeatChartTarget = musx::dom::others::BeatChartElement;

constexpr records::LegacyTag beatChartTag = records::packTag("BC");
constexpr records::LegacyTag beatChartClass = 0x007a;
constexpr std::size_t beatChartTupleSize = 12;

template <typename Reporting>
void reportBeatChartField(Reporting& reporting, std::uint16_t partId, musx::dom::Cmper cmper, musx::dom::Inci inci, const char* member,
    std::int64_t value, const records::LegacyRow& row)
{
    const auto key = reporting.template instanceKey<BeatChartTarget>(partId, cmper, inci);
    reporting.report().setField(key, member, {Reporting::Origin::LegacyMus, row.blockOffset, row.decodedOffset, value});
}

template <typename Reporting>
void reportDefaultBeatChartField(Reporting& reporting, std::uint16_t partId, musx::dom::Cmper cmper, musx::dom::Inci inci, const char* member)
{
    const auto key = reporting.template instanceKey<BeatChartTarget>(partId, cmper, inci);
    reporting.report().setField(key, member, {Reporting::Origin::Finale27Default, 0, 0, 0});
}

// Coda coalesces the control and first positioned element in physical incidence 0. Later physical
// incidences retain that one-position displacement, while fields introduced later remain defaults.
void importCodaBeatChartFamily(const ImportContext& context, const RecordFamilySource& source, std::uint16_t partId, musx::dom::Cmper cmper,
    std::span<const records::LegacyRow> rows)
{
    const auto payload = collectRecordPayload(source, rows);
    if (payload.size() % beatChartTupleSize != 0) {
        context.report.diagnostics.push_back(
            {musx::util::Logger::LogLevel::Info, "Beat chart " + std::to_string(cmper) + " has an incomplete trailing element."});
    }
    for (std::size_t at = 0; at + beatChartTupleSize <= payload.size(); at += beatChartTupleSize) {
        const auto sourceInci = static_cast<musx::dom::Inci>(at / beatChartTupleSize);
        const auto elementInci = static_cast<musx::dom::Inci>(sourceInci + 1);
        const auto edu = payloadLong(payload, at, context.profile.byteOrder, nativeLongWordOrder(context.profile.byteOrder));
        const auto pos = static_cast<std::int16_t>(payloadWord(payload, at + 4, context.profile.byteOrder));
        const auto endPos = static_cast<std::int16_t>(payloadWord(payload, at + 6, context.profile.byteOrder));
        const auto trailing = static_cast<std::int16_t>(payloadWord(payload, at + 8, context.profile.byteOrder));
        const auto& longRow = source.rowOfWord(rows, at / 2);
        const auto& posRow = source.rowOfWord(rows, at / 2 + 2);
        const auto& endPosRow = source.rowOfWord(rows, at / 2 + 3);
        const auto& trailingRow = source.rowOfWord(rows, at / 2 + 4);
        if (sourceInci == 0) {
            auto control = createOthersRecordTarget<BeatChartTarget>(context.document, source, rows.front(), cmper, 0);
            if (control) {
                control->control = std::make_shared<BeatChartTarget::Control>();
                control->control->totalDur = edu;
                control->control->totalWidth = trailing;
                withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                    reportBeatChartField<Reporting>(reporting, partId, cmper, 0, "control.totalDur", edu, longRow);
                    reportBeatChartField<Reporting>(reporting, partId, cmper, 0, "control.totalWidth", trailing, trailingRow);
                    reportDefaultBeatChartField<Reporting>(reporting, partId, cmper, 0, "control.minWidth");
                    reportDefaultBeatChartField<Reporting>(reporting, partId, cmper, 0, "control.allotWidth");
                    reporting.report().setInstanceOrigin(
                        reporting.template instanceKey<BeatChartTarget>(partId, cmper, 0), Reporting::Origin::LegacyMus);
                });
                context.document->getOthers()->add(BeatChartTarget::XmlNodeName, std::move(control));
            }
        }
        auto element = createOthersRecordTarget<BeatChartTarget>(context.document, source, rows.front(), cmper, elementInci);
        if (!element) {
            continue;
        }
        element->dur = sourceInci == 0 ? 0 : edu;
        element->pos = pos;
        element->endPos = endPos;
        withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
            if (sourceInci == 0) {
                reportDefaultBeatChartField<Reporting>(reporting, partId, cmper, elementInci, "dur");
            } else {
                reportBeatChartField<Reporting>(reporting, partId, cmper, elementInci, "dur", edu, longRow);
            }
            reportBeatChartField<Reporting>(reporting, partId, cmper, elementInci, "pos", pos, posRow);
            reportBeatChartField<Reporting>(reporting, partId, cmper, elementInci, "endPos", endPos, endPosRow);
            reportDefaultBeatChartField<Reporting>(reporting, partId, cmper, elementInci, "minPos");
            reporting.report().setInstanceOrigin(
                reporting.template instanceKey<BeatChartTarget>(partId, cmper, elementInci), Reporting::Origin::LegacyMus);
        });
        context.document->getOthers()->add(BeatChartTarget::XmlNodeName, std::move(element));
    }
}

void importBeatChartFamily(const ImportContext& context, const RecordFamilySource& source, std::uint16_t partId, musx::dom::Cmper cmper)
{
    const auto rows = source.pool->getArray(source.identity, cmper, 0, partId);
    if (rows.empty()) {
        return;
    }
    if (context.profile.epoch == FormatEpoch::CodaBanner) {
        importCodaBeatChartFamily(context, source, partId, cmper, rows);
        return;
    }
    const auto payload = collectRecordPayload(source, rows);
    if (payload.size() % beatChartTupleSize != 0) {
        context.report.diagnostics.push_back(
            {musx::util::Logger::LogLevel::Info, "Beat chart " + std::to_string(cmper) + " has an incomplete trailing element."});
    }
    for (std::size_t at = 0; at + beatChartTupleSize <= payload.size(); at += beatChartTupleSize) {
        const auto inci = static_cast<musx::dom::Inci>(at / beatChartTupleSize);
        auto target = createOthersRecordTarget<BeatChartTarget>(context.document, source, rows.front(), cmper, inci);
        if (!target) {
            continue;
        }
        const auto edu = payloadLong(payload, at, context.profile.byteOrder, nativeLongWordOrder(context.profile.byteOrder));
        const auto first = static_cast<std::int16_t>(payloadWord(payload, at + 4, context.profile.byteOrder));
        const auto second = static_cast<std::int16_t>(payloadWord(payload, at + 6, context.profile.byteOrder));
        const auto third = static_cast<std::int16_t>(payloadWord(payload, at + 8, context.profile.byteOrder));
        withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
            const auto& longRow = source.rowOfWord(rows, at / 2);
            const auto& firstRow = source.rowOfWord(rows, at / 2 + 2);
            const auto& secondRow = source.rowOfWord(rows, at / 2 + 3);
            const auto& thirdRow = source.rowOfWord(rows, at / 2 + 4);
            if (inci == 0) {
                target->control = std::make_shared<BeatChartTarget::Control>();
                target->control->totalDur = edu;
                target->control->totalWidth = first;
                target->control->minWidth = second;
                target->control->allotWidth = third;
                reportBeatChartField<Reporting>(reporting, partId, cmper, inci, "control.totalDur", edu, longRow);
                reportBeatChartField<Reporting>(reporting, partId, cmper, inci, "control.totalWidth", first, firstRow);
                reportBeatChartField<Reporting>(reporting, partId, cmper, inci, "control.minWidth", second, secondRow);
                reportBeatChartField<Reporting>(reporting, partId, cmper, inci, "control.allotWidth", third, thirdRow);
            } else {
                target->dur = edu;
                target->pos = first;
                target->endPos = second;
                target->minPos = third;
                reportBeatChartField<Reporting>(reporting, partId, cmper, inci, "dur", edu, longRow);
                reportBeatChartField<Reporting>(reporting, partId, cmper, inci, "pos", first, firstRow);
                reportBeatChartField<Reporting>(reporting, partId, cmper, inci, "endPos", second, secondRow);
                reportBeatChartField<Reporting>(reporting, partId, cmper, inci, "minPos", third, thirdRow);
            }
            reporting.report().setInstanceOrigin(reporting.template instanceKey<BeatChartTarget>(partId, cmper, inci), Reporting::Origin::LegacyMus);
        });
        context.document->getOthers()->add(BeatChartTarget::XmlNodeName, std::move(target));
    }
}

} // namespace

void importBeatChartElements(const ImportContext& context)
{
    const auto source = selectRecordFamilySource(context, context.index.getOthers(), context.index.getClassOthers(), beatChartTag, beatChartClass);
    if (!source) {
        return;
    }
    for (const auto& [partId, cmper] : recordKeys(*source)) {
        importBeatChartFamily(context, *source, partId, cmper);
    }
}

} // namespace others
} // namespace finale_mus_reader
