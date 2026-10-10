// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/details.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace details {
namespace {

constexpr std::size_t shortBeamWordCount = 5;
constexpr std::size_t fullBeamWordCount = 10;
constexpr records::LegacyTag downPrimaryBeamClass = 0x0401;
constexpr records::LegacyTag upPrimaryBeamClass = 0x0402;
constexpr records::LegacyTag downSecondaryBeamClass = 0x0403;
constexpr records::LegacyTag upSecondaryBeamClass = 0x0404;
constexpr records::LegacyTag plainStemClass = 0x042a;
constexpr records::LegacyTag beamedStemClass = 0x03ff;
constexpr std::size_t stemAlterationWordCount = 5;

template <typename Target>
void importStemAlterationFamily(const ImportContext& context, const char* tag, records::LegacyTag classId)
{
    const auto selected =
        selectRecordFamilySource(context, context.index.getDetails(), context.index.getClassDetails(), records::packTag(tag), classId, true);
    if (!selected) {
        return;
    }
    const auto& source = *selected;
    for (const auto& [partId, entryHigh] : recordKeys(source)) {
        for (const auto entryLow : source.pool->secondCmpersForTag(source.identity, entryHigh, partId)) {
            if (entryHigh == 0 && entryLow == 0) {
                continue;
            }
            const auto* row = source.pool->get(source.identity, entryHigh, entryLow, 0, partId);
            if (!row) {
                continue;
            }
            const auto words = collectRecordWords(source, std::span(row, 1), context.profile.byteOrder);
            if (words.size() != stemAlterationWordCount) {
                continue;
            }
            const auto entry = (static_cast<musx::dom::EntryNumber>(entryHigh) << 16U) | entryLow;
            auto target = std::make_shared<Target>(context.document, row->partId, recordShareMode(source, *row), entry);
            target->upVertAdjust = words[0];
            target->downVertAdjust = words[1];
            const auto packed = static_cast<std::uint16_t>(words[4]);
            const auto signedByte = [](std::uint16_t value) { return static_cast<musx::dom::Evpu>(value < 0x80 ? value : value - 0x100); };
            const auto upHorizontal = signedByte(packed >> 8U);
            const auto downHorizontal = signedByte(packed & 0xffU);
            target->upHorzAdjust = upHorizontal;
            target->downHorzAdjust = downHorizontal;
            withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                const auto key = reporting.template instanceKey<Target>(target->getSourcePartId(), entryHigh, std::nullopt, entryLow);
                reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
                const auto reportField = [&](const char* name, std::size_t byteOffset, std::int64_t value) {
                    reportLegacyField(reporting, key, source, *row, name, source.byteOffsetInRow(byteOffset), value);
                };
                reportField("upVertAdjust", 0, words[0]);
                reportField("downVertAdjust", 2, words[1]);
                const bool bigEndian = context.profile.byteOrder == ByteOrder::BigEndian;
                reportField("upHorzAdjust", 8 + (bigEndian ? 0 : 1), upHorizontal);
                reportField("downHorzAdjust", 8 + (bigEndian ? 1 : 0), downHorizontal);
            });
            context.document->getDetails()->add(Target::XmlNodeName, std::move(target));
        }
    }
}

template <typename Target>
void reportBeamAlteration(const ImportContext& context, const Target& target, const RecordFamilySource& source,
    std::span<const records::LegacyRow> rows, std::span<const std::int16_t> words, std::size_t at)
{
    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
        const auto entry = target.getEntryNumber();
        const auto key = reporting.template instanceKey<Target>(
            target.getSourcePartId(), static_cast<musx::dom::Cmper>(entry >> 16U), target.getInci(), static_cast<musx::dom::Cmper>(entry));
        reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
        constexpr std::array<const char*, 7> names = {
            "leftOffsetH", "leftOffsetY", "rightOffsetH", "rightOffsetY", "dura", "flattenStyle", "beamWidth"};
        const auto mappedFields = (std::min)(words.size(), names.size());
        for (std::size_t slot = 0; slot < mappedFields; ++slot) {
            const auto& row = source.rowOfWord(rows, at + slot);
            reporting.report().setField(key, names[slot],
                {Reporting::Origin::LegacyMus, row.blockOffset, row.decodedOffset + source.byteOffsetInRow((at + slot) * sizeof(std::uint16_t)),
                    words[slot], source.identity});
        }
        for (std::size_t slot = mappedFields; slot < names.size(); ++slot) {
            const auto value = slot == 5 ? static_cast<std::int64_t>(target.flattenStyle) : target.beamWidth;
            const auto origin = slot == 5 ? Reporting::Origin::Finale27Default : Reporting::Origin::LegacyBehavior;
            reportFallbackField(reporting, key, names[slot], origin, value);
        }
    });
}

template <typename Target, bool Secondary>
void importBeamAlterationFamily(const ImportContext& context, const char* tag, records::LegacyTag classId)
{
    const auto selected =
        selectRecordFamilySource(context, context.index.getDetails(), context.index.getClassDetails(), records::packTag(tag), classId, true);
    if (!selected) {
        return;
    }
    const auto& source = *selected;
    const bool beamWidthStoredAsEvpu = sourcePredatesVersion(context.profile, FormatEpoch::DclLegacy, versions::finale2002);
    for (const auto& [partId, entryHigh] : recordKeys(source)) {
        for (const auto entryLow : source.pool->secondCmpersForTag(source.identity, entryHigh, partId)) {
            const auto rows = source.pool->getArray(source.identity, entryHigh, entryLow, partId);
            const auto words = collectRecordWords(source, rows, context.profile.byteOrder);
            const auto entry = (static_cast<musx::dom::EntryNumber>(entryHigh) << 16U) | entryLow;
            std::size_t at = 0;
            musx::dom::Inci inci = 0;
            while (at < words.size()) {
                std::size_t count = 0;
                if constexpr (Secondary) {
                    if (source.classRecords) {
                        count = fullBeamWordCount;
                    } else if (at + fullBeamWordCount <= words.size() && words[at + fullBeamWordCount - 1] == 0) {
                        count = fullBeamWordCount;
                    } else {
                        count = shortBeamWordCount;
                    }
                } else {
                    count = words.size();
                }
                if (at + count > words.size() || (count != shortBeamWordCount && count != fullBeamWordCount)) {
                    context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                        "Beam alteration for entry " + std::to_string(entry) + " does not contain a complete five- or ten-word element."});
                    break;
                }
                const auto shareMode = recordShareMode(source, rows.front());
                std::shared_ptr<Target> target;
                if constexpr (Secondary) {
                    target = std::make_shared<Target>(context.document, rows.front().partId, shareMode, entry, inci);
                    if (shareMode == musx::dom::EnigmaBase::ShareMode::Partial) {
                        initializePartialFromScore(target, context.document->getDetails()->get<Target>(musx::dom::SCORE_PARTID, entry, inci));
                    }
                } else {
                    target = std::make_shared<Target>(context.document, rows.front().partId, shareMode, entry);
                    if (shareMode == musx::dom::EnigmaBase::ShareMode::Partial) {
                        initializePartialFromScore(target, context.document->getDetails()->get<Target>(musx::dom::SCORE_PARTID, entry));
                    }
                }
                const std::span<const std::int16_t> element(words.data() + at, count);
                target->leftOffsetH = element[0];
                target->leftOffsetY = element[1];
                target->rightOffsetH = element[2];
                target->rightOffsetY = element[3];
                target->dura = element[4];
                target->beamWidth = -1;
                if (count == fullBeamWordCount) {
                    target->flattenStyle = static_cast<typename Target::FlattenStyle>(element[5]);
                    if constexpr (!Secondary) {
                        const auto storedWidth = static_cast<musx::dom::Efix>(element[6]);
                        target->beamWidth = beamWidthStoredAsEvpu && storedWidth == 0 ? -1
                                            : beamWidthStoredAsEvpu && storedWidth > 0
                                                ? storedWidth * static_cast<musx::dom::Efix>(musx::dom::EFIX_PER_EVPU)
                                                : storedWidth;
                    } else {
                        target->beamWidth = element[6];
                    }
                }
                reportBeamAlteration(context, *target, source, rows, element, at);
                context.document->getDetails()->add(Target::XmlNodeName, std::move(target));
                at += count;
                ++inci;
            }
        }
    }
}

} // namespace

void importBeamAlterationsDownStem(const ImportContext& context)
{
    importBeamAlterationFamily<musx::dom::details::BeamAlterationsDownStem, false>(context, "BL", downPrimaryBeamClass);
}

void importBeamAlterationsUpStem(const ImportContext& context)
{
    importBeamAlterationFamily<musx::dom::details::BeamAlterationsUpStem, false>(context, "BH", upPrimaryBeamClass);
}

void importSecondaryBeamAlterationsDownStem(const ImportContext& context)
{
    importBeamAlterationFamily<musx::dom::details::SecondaryBeamAlterationsDownStem, true>(context, "bL", downSecondaryBeamClass);
}

void importSecondaryBeamAlterationsUpStem(const ImportContext& context)
{
    importBeamAlterationFamily<musx::dom::details::SecondaryBeamAlterationsUpStem, true>(context, "bH", upSecondaryBeamClass);
}

void importStemAlterations(const ImportContext& context)
{
    importStemAlterationFamily<musx::dom::details::StemAlterations>(context, "ST", plainStemClass);
}

void importStemAlterationsUnderBeam(const ImportContext& context)
{
    importStemAlterationFamily<musx::dom::details::StemAlterationsUnderBeam>(context, "St", beamedStemClass);
}

} // namespace details
} // namespace finale_mus_reader
