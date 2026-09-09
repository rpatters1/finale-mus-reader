// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "import/support/percussion_mappings.h"
#include "import/support/text_encoding.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using PercussionNoteInfoTarget = musx::dom::others::PercussionNoteInfo;
using PercussionFontType = musx::dom::options::FontOptions::FontType;

constexpr records::LegacyTag percussionNoteInfoClass = 0x0139;
constexpr auto legacyDrumStaffTag = records::packTag("DS");
constexpr records::LegacyTag legacyDrumStaffClass = 0x0084;
constexpr auto legacyPercussionMapTag = records::packTag("DF");
constexpr records::LegacyTag legacyPercussionMapClass = 0x040e;
constexpr auto legacyPercussionMapNameTag = records::packTag("DL");
constexpr records::LegacyTag legacyPercussionMapNameClass = 0x0083;
constexpr std::size_t percussionNoteInfoNarrowStride = 12;
constexpr std::size_t percussionNoteInfoWideDataSize = 20;
constexpr std::size_t percussionNoteInfoWideStride = 24;

void reportPercussionNoteInfo(const ImportContext &context, const PercussionNoteInfoTarget &target,
                              const RecordFamilySource &source, const records::LegacyRow &row,
                              std::size_t elementOffset, bool wide) {
    withReporting(context.report, [&]<typename Reporting>(Reporting &reporting) {
        const auto key = reporting.template instanceKey<PercussionNoteInfoTarget>(
            target.getSourcePartId(), target.getCmper(), target.getInci());
        reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
        const auto report = [&](const char *member, std::size_t fieldOffset, auto value) {
            reporting.report().setField(key, member,
                                        {Reporting::Origin::LegacyMus, row.blockOffset,
                                         row.decodedOffset + elementOffset + fieldOffset, value,
                                         source.identity});
        };
        report("percNoteType", 0, target.percNoteType);
        report("staffPosition", 2, target.staffPosition);
        report("closedNotehead", 4, static_cast<std::uint32_t>(target.closedNotehead));
        report("halfNotehead", wide ? 8 : 6, static_cast<std::uint32_t>(target.halfNotehead));
        report("wholeNotehead", wide ? 12 : 8, static_cast<std::uint32_t>(target.wholeNotehead));
        report("dwholeNotehead", wide ? 16 : 10, static_cast<std::uint32_t>(target.dwholeNotehead));
    });
}

void reportLegacyPercussionNoteInfo(const ImportContext &context,
                                    const PercussionNoteInfoTarget &target,
                                    const RecordFamilySource &source,
                                    const records::LegacyRow &row,
                                    std::uint16_t storedMidiNote) {
    withReporting(context.report, [&]<typename Reporting>(Reporting &reporting) {
        using Origin = Reporting::Origin;
        const auto key = reporting.template instanceKey<PercussionNoteInfoTarget>(
            target.getSourcePartId(), target.getCmper(), target.getInci());
        reporting.report().setInstanceOrigin(key, Origin::LegacyBehavior);
        const auto report = [&](const char *member, Origin origin, std::size_t fieldOffset,
                                auto value) {
            reporting.report().setField(
                key, member,
                {origin, row.blockOffset, row.decodedOffset + fieldOffset, value, source.identity});
        };
        report("percNoteType", Origin::LegacyBehavior, 0, storedMidiNote);
        report("staffPosition", Origin::LegacyMus, 2, target.staffPosition);
        report("closedNotehead", Origin::LegacyMus, 4,
               static_cast<std::uint32_t>(target.closedNotehead));
        report("halfNotehead", Origin::LegacyBehavior, 6,
               static_cast<std::uint32_t>(target.halfNotehead));
        report("wholeNotehead", Origin::LegacyBehavior, 6,
               static_cast<std::uint32_t>(target.wholeNotehead));
        report("dwholeNotehead", Origin::LegacyBehavior, 6,
               static_cast<std::uint32_t>(target.dwholeNotehead));
    });
}

char32_t percussionNotehead(std::uint16_t stored, const musx::dom::DocumentPtr &document,
                            musx::dom::Cmper fontId) {
    return text::codepointFromByte(static_cast<std::uint8_t>(stored), document, fontId,
                                   text::UnresolvedFontFallback::Symbol);
}

std::optional<musx::dom::PercussionNoteTypeId> noteTypeForGeneralMidi(std::uint16_t midiNote) {
    constexpr std::size_t midiNoteCount = 128;
    if (midiNote >= midiNoteCount)
        return std::nullopt;
    static const auto noteTypes = [] {
        std::array<musx::dom::PercussionNoteTypeId, midiNoteCount> result{};
        constexpr std::uint32_t firstReservedCustomType = 3968;
        constexpr auto baseTypeLimit = firstReservedCustomType - 1U;
        for (std::uint32_t id = 1; id <= baseTypeLimit; ++id) {
            const auto &type = musx::dom::percussion::getPercussionNoteTypeFromId(
                static_cast<musx::dom::PercussionNoteTypeId>(id));
            if (type.instrumentId == static_cast<int>(id) && type.generalMidi >= 0 &&
                type.generalMidi < static_cast<int>(result.size())) {
                result[static_cast<std::size_t>(type.generalMidi)] =
                    static_cast<musx::dom::PercussionNoteTypeId>(id);
            }
        }
        return result;
    }();
    if (noteTypes[midiNote] == 0)
        return std::nullopt;
    return noteTypes[midiNote];
}

std::map<musx::dom::Cmper, std::string>
legacyPercussionMapNames(const ImportContext &context) {
    const auto selected = selectRecordFamilySource(
        context, context.index.getOthers(), context.index.getClassOthers(),
        legacyPercussionMapNameTag, legacyPercussionMapNameClass);
    std::map<musx::dom::Cmper, std::string> result;
    if (!selected)
        return result;
    const auto &source = *selected;
    for (const auto [partId, mapId] : recordKeys(source)) {
        if (partId != musx::dom::SCORE_PARTID)
            continue;
        const auto rows = source.pool->getArray(source.identity, mapId, 0, partId);
        if (rows.empty())
            continue;
        result.try_emplace(mapId, text::toUtf8(readRowText(*source.pool, rows),
                                               context.profile.platform));
    }
    return result;
}

std::map<musx::dom::Cmper, std::set<musx::dom::Cmper>>
selectedLegacyPercussionRows(const ImportContext &context) {
    const auto selected =
        selectRecordFamilySource(context, context.index.getOthers(), context.index.getClassOthers(),
                                 legacyDrumStaffTag, legacyDrumStaffClass);
    std::map<musx::dom::Cmper, std::set<musx::dom::Cmper>> result;
    if (!selected)
        return result;
    const auto &source = *selected;
    for (const auto [partId, staffId] : recordKeys(source)) {
        const auto rows = source.pool->getArray(source.identity, staffId, 0, partId);
        if (rows.empty())
            continue;
        const auto payload = collectRecordPayload(source, rows);
        if (payload.size() < 2)
            continue;

        const auto mapId = payloadWord(payload, 0, context.profile.byteOrder);
        auto &selectedRows = result[mapId];
        for (std::size_t offset = 2; offset + 2 <= payload.size(); offset += 2) {
            const auto bitmap = payloadWord(payload, offset, context.profile.byteOrder);
            const auto firstKey = static_cast<musx::dom::Cmper>((offset / 2U - 1U) * 16U);
            for (unsigned bit = 0; bit < 16; ++bit) {
                if ((bitmap & (std::uint16_t{1} << bit)) != 0) {
                    selectedRows.insert(static_cast<musx::dom::Cmper>(firstKey + bit));
                }
            }
        }
    }
    return result;
}

void importLegacyPercussionNoteInfo(const ImportContext &context) {
    const auto selected = selectRecordFamilySource(
        context, context.index.getDetails(), context.index.getClassDetails(),
        legacyPercussionMapTag, legacyPercussionMapClass, true);
    if (!selected)
        return;
    const auto &source = *selected;
    const auto percussionFont = musx::dom::options::FontOptions::getFontInfoOrNull(
        context.document, PercussionFontType::Percussion);
    const auto fontId = percussionFont ? percussionFont->fontId : musx::dom::Cmper{};
    const auto mapNames = legacyPercussionMapNames(context);
    for (const auto &[mapId, selectedRows] : selectedLegacyPercussionRows(context)) {
        std::map<musx::dom::PercussionNoteTypeId, std::uint16_t> typeOrders;
        musx::dom::Inci inci = 0;
        for (const auto midiKey : selectedRows) {
            const auto rows =
                source.pool->getArray(source.identity, mapId, midiKey, musx::dom::SCORE_PARTID);
            if (rows.empty())
                continue;
            const auto payload = source.pool->effectivePayloadOf(rows.front());
            if (payload.size() < records::detailWordCount * 2U)
                continue;
            const auto midiNote = payloadWord(payload, 0, context.profile.byteOrder);
            auto noteType = std::optional<musx::dom::PercussionNoteTypeId>{};
            if (context.profile.percussionMappings) {
                if (const auto name = mapNames.find(mapId); name != mapNames.end()) {
                    noteType = context.profile.percussionMappings->find(name->second, midiNote);
                }
            }
            const auto mapSpecificType = noteType.has_value();
            if (!noteType)
                noteType = noteTypeForGeneralMidi(midiNote);
            if (!noteType)
                continue;

            auto target = createOthersRecordTarget<PercussionNoteInfoTarget>(
                context.document, source, rows.front(), mapId, inci++);
            if (!target)
                continue;
            if (mapSpecificType) {
                target->percNoteType = *noteType;
            } else {
                const auto order = typeOrders[*noteType]++;
                target->percNoteType = musx::dom::PercussionNoteTypeId(
                    *noteType | ((order & 0xfU) << 12U));
            }
            target->staffPosition =
                static_cast<std::int16_t>(payloadWord(payload, 2, context.profile.byteOrder));
            target->closedNotehead = percussionNotehead(
                payloadWord(payload, 4, context.profile.byteOrder), context.document, fontId);
            target->halfNotehead = target->wholeNotehead = target->dwholeNotehead =
                percussionNotehead(payloadWord(payload, 6, context.profile.byteOrder),
                                   context.document, fontId);
            reportLegacyPercussionNoteInfo(context, *target, source, rows.front(), midiNote);
            context.document->getOthers()->add(PercussionNoteInfoTarget::XmlNodeName,
                                               std::move(target));
        }
    }
}

} // namespace

void importPercussionNoteInfo(const ImportContext &context) {
    // Percussion maps begin in Finale 3.5, after the Coda-banner epoch. Fixed-row
    // files use DS to select the DF rows that constitute each staff's map.
    if (context.profile.epoch == FormatEpoch::UncompressedLegacy ||
        context.profile.epoch == FormatEpoch::DclLegacy) {
        importLegacyPercussionNoteInfo(context);
        return;
    }
    // The Coda-banner epoch predates percussion maps.
    if (context.profile.epoch != FormatEpoch::ZlibLegacy)
        return;
    // Before the native packed map arrived, zlib files retained the DF layout as
    // class-detail 0x040e and the DS selection bitmap as class-other 0x0084.
    if (context.index.getClassOthers().partIdsForTag(percussionNoteInfoClass).empty()) {
        importLegacyPercussionNoteInfo(context);
        return;
    }
    const RecordFamilySource source{&context.index.getClassOthers(), percussionNoteInfoClass, true};

    const bool wide = versions::storesUnicodeCodepoints(context.profile.version);
    const auto stride = wide ? percussionNoteInfoWideStride : percussionNoteInfoNarrowStride;
    const auto percussionFont = musx::dom::options::FontOptions::getFontInfoOrNull(
        context.document, PercussionFontType::Percussion);
    const auto fontId = percussionFont ? percussionFont->fontId : musx::dom::Cmper{};

    for (const auto [partId, cmper] : recordKeys(source)) {
        const auto rows = source.pool->getArray(source.identity, cmper, 0, partId);
        if (rows.empty())
            continue;
        const auto payload = collectRecordPayload(source, rows);
        if (payload.size() % stride != 0) {
            context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                                                  "Percussion note map " + std::to_string(cmper) +
                                                      " has an incomplete trailing element."});
        }
        for (std::size_t at = 0; at + stride <= payload.size(); at += stride) {
            const auto inci = static_cast<musx::dom::Inci>(at / stride);
            const auto &row = rows.front();
            auto target = createOthersRecordTarget<PercussionNoteInfoTarget>(
                context.document, source, row, cmper, inci);
            if (!target)
                continue;

            target->percNoteType = payloadWord(payload, at, context.profile.byteOrder);
            target->staffPosition =
                static_cast<std::int16_t>(payloadWord(payload, at + 2, context.profile.byteOrder));
            if (wide) {
                target->closedNotehead = static_cast<char32_t>(payloadLong(
                    payload, at + 4, context.profile.byteOrder, LongWordOrder::LowFirst));
                target->halfNotehead = static_cast<char32_t>(payloadLong(
                    payload, at + 8, context.profile.byteOrder, LongWordOrder::LowFirst));
                target->wholeNotehead = static_cast<char32_t>(payloadLong(
                    payload, at + 12, context.profile.byteOrder, LongWordOrder::LowFirst));
                target->dwholeNotehead = static_cast<char32_t>(payloadLong(
                    payload, at + 16, context.profile.byteOrder, LongWordOrder::LowFirst));
                if (!std::all_of(payload.begin() + static_cast<std::ptrdiff_t>(
                                                       at + percussionNoteInfoWideDataSize),
                                 payload.begin() + static_cast<std::ptrdiff_t>(at + stride),
                                 [](std::uint8_t value) { return value == 0; })) {
                    context.report.diagnostics.push_back(
                        {musx::util::Logger::LogLevel::Info,
                         "Percussion note map " + std::to_string(cmper) +
                             " has nonzero trailing words in a Finale 2012 element."});
                }
            } else {
                target->closedNotehead =
                    percussionNotehead(payloadWord(payload, at + 4, context.profile.byteOrder),
                                       context.document, fontId);
                target->halfNotehead =
                    percussionNotehead(payloadWord(payload, at + 6, context.profile.byteOrder),
                                       context.document, fontId);
                target->wholeNotehead =
                    percussionNotehead(payloadWord(payload, at + 8, context.profile.byteOrder),
                                       context.document, fontId);
                target->dwholeNotehead =
                    percussionNotehead(payloadWord(payload, at + 10, context.profile.byteOrder),
                                       context.document, fontId);
            }
            reportPercussionNoteInfo(context, *target, source, row, at, wide);
            context.document->getOthers()->add(PercussionNoteInfoTarget::XmlNodeName,
                                               std::move(target));
        }
    }
}

} // namespace others
} // namespace finale_mus_reader
