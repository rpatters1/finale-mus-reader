// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/details.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace details {
namespace {

using PercussionNoteCodeTarget = musx::dom::details::PercussionNoteCode;

constexpr records::LegacyTag percussionNoteCodeClass = 0x0451;
constexpr std::size_t percussionNoteCodeStride = 10;

void reportPercussionNoteCode(const ImportContext &context, const PercussionNoteCodeTarget &target,
                              const RecordFamilySource &source, const records::LegacyRow &row,
                              std::size_t elementOffset) {
    withReporting(context.report, [&]<typename Reporting>(Reporting &reporting) {
        const auto entryNumber = target.getEntryNumber();
        const auto key = reporting.template instanceKey<PercussionNoteCodeTarget>(
            target.getSourcePartId(), static_cast<musx::dom::Cmper>(entryNumber >> 16U),
            target.getInci(), static_cast<musx::dom::Cmper>(entryNumber));
        reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
        reporting.report().setField(key, "noteId",
                                    {Reporting::Origin::LegacyMus, row.blockOffset,
                                     row.decodedOffset + elementOffset, target.noteId,
                                     source.identity});
        reporting.report().setField(key, "noteCode",
                                    {Reporting::Origin::LegacyMus, row.blockOffset,
                                     row.decodedOffset + elementOffset + 2, target.noteCode,
                                     source.identity});
    });
}

} // namespace

void importPercussionNoteCodes(const ImportContext &context) {
    // Believed: a pre-zlib percussion entry's pitch selects a DF input key; no
    // standalone note-assignment record family is known for those epochs.
    if (context.profile.epoch != FormatEpoch::ZlibLegacy)
        return;

    const RecordFamilySource source{&context.index.getClassDetails(), percussionNoteCodeClass, true,
                                    true};
    for (const auto [partId, entryHigh] : recordKeys(source)) {
        for (const auto entryLow :
             source.pool->secondCmpersForTag(source.identity, entryHigh, partId)) {
            const auto rows = source.pool->getArray(source.identity, entryHigh, entryLow, partId);
            if (rows.empty())
                continue;
            const auto payload = collectRecordPayload(source, rows);
            if (payload.size() % percussionNoteCodeStride != 0) {
                context.report.diagnostics.push_back(
                    {musx::util::Logger::LogLevel::Info,
                     "Percussion note-code detail for entry " +
                         std::to_string((static_cast<std::uint32_t>(entryHigh) << 16U) | entryLow) +
                         " has an incomplete trailing element."});
            }
            for (std::size_t at = 0; at + percussionNoteCodeStride <= payload.size();
                 at += percussionNoteCodeStride) {
                const auto inci = static_cast<musx::dom::Inci>(at / percussionNoteCodeStride);
                auto target = createDetailsRecordTarget<PercussionNoteCodeTarget>(
                    context.document, source, rows.front(), entryHigh, entryLow, inci);
                if (!target)
                    continue;
                target->noteId = payloadWord(payload, at, context.profile.byteOrder);
                target->noteCode = payloadWord(payload, at + 2, context.profile.byteOrder);
                if (!std::all_of(payload.begin() + static_cast<std::ptrdiff_t>(at + 4),
                                 payload.begin() +
                                     static_cast<std::ptrdiff_t>(at + percussionNoteCodeStride),
                                 [](std::uint8_t value) { return value == 0; })) {
                    context.report.diagnostics.push_back(
                        {musx::util::Logger::LogLevel::Info,
                         "Percussion note-code detail for entry " +
                             std::to_string(target->getEntryNumber()) +
                             " has nonzero trailing words."});
                }
                reportPercussionNoteCode(context, *target, source, rows.front(), at);
                context.document->getDetails()->add(PercussionNoteCodeTarget::XmlNodeName,
                                                    std::move(target));
            }
        }
    }
}

} // namespace details
} // namespace finale_mus_reader
