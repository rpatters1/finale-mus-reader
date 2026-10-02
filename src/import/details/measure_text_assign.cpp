// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/details.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "import/shared/page_text_records.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace details {
namespace {

using MeasureTextTarget = musx::dom::details::MeasureTextAssign;
constexpr records::LegacyTag measureTextClass = 0x0420;
constexpr std::size_t measureTextWordCount = 5;
constexpr std::uint16_t measureTextHiddenFlag = 0x0001U;

using EarlyBlockKeys = std::set<std::pair<musx::dom::Cmper, musx::dom::Cmper>>;

/// @brief A stored vertical offset that awaits its staff's size on the containing system.
struct ScaledVertical
{
    std::shared_ptr<MeasureTextTarget> target;
    const records::LegacyRow* row{};
    std::size_t byteOffset{};
    std::int16_t stored{};
};

// Believed: before the DCL epoch the vertical offset is scaled by the staff's size on the system
// that contains the measure, so the unscaled displacement divides by that size. A system's own
// size does not scale it, and later sources store it unscaled.
void unscaleVerticals(const ImportContext& context, const RecordFamilySource& source, const std::vector<ScaledVertical>& verticals)
{
    using StaffSystem = musx::dom::others::StaffSystem;
    const auto systems = context.document->getOthers()->getArray<StaffSystem>(musx::dom::SCORE_PARTID);
    for (const auto& vertical : verticals) {
        const auto meas = vertical.target->getCmper2();
        // A source saved without page layout has no systems, and its offset is not reduced. musxdom's
        // measure-to-system lookup reads page ranges it resolves only once the document is
        // finished, so the saved systems are searched directly.
        const auto system =
            std::ranges::find_if(systems, [meas](const auto& candidate) { return meas >= candidate->startMeas && meas < candidate->endMeas; });
        if (system == systems.end()) {
            continue;
        }
        const auto scaling = (*system)->calcStaffScaling(vertical.target->getCmper1());
        if (scaling == 1 || scaling <= 0) {
            continue;
        }
        vertical.target->yDisp = static_cast<musx::dom::Evpu>(std::lround((musx::util::Fraction(vertical.stored) / scaling).toDouble()));
        withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
            const auto key = reporting.template instanceKey<MeasureTextTarget>(
                musx::dom::SCORE_PARTID, vertical.target->getCmper1(), vertical.target->getInci().value_or(0), meas);
            reportLegacyField(
                reporting, key, source, *vertical.row, "yDisp", vertical.byteOffset, vertical.stored, Reporting::Origin::LegacyMusAdjusted);
        });
    }
}

void reportMeasureText(const ImportContext& context, const RecordFamilySource& source, std::span<const records::LegacyRow> rows,
    const MeasureTextTarget& target, std::size_t at, std::uint16_t partId, bool earlyCoordinates)
{
    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
        const auto key =
            reporting.template instanceKey<MeasureTextTarget>(partId, target.getCmper1(), target.getInci().value_or(0), target.getCmper2());
        reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
        const auto field = [&](const char* name, std::size_t slot, std::int64_t value,
                               typename Reporting::Origin origin = Reporting::Origin::LegacyMus) {
            const auto physicalSlot = at + slot;
            reportLegacyField(
                reporting, key, source, source.rowOfWord(rows, physicalSlot), name, source.byteOffsetInRow(physicalSlot * 2), value, origin);
        };
        field("block", 0, target.block);
        if (earlyCoordinates) {
            field("xDispEvpu", 1, target.xDispEvpu, Reporting::Origin::LegacyMusAdjusted);
            reportFallbackField(reporting, key, "xDispEdu", Reporting::Origin::LegacyBehavior, target.xDispEdu);
        } else {
            field("xDispEdu", 1, target.xDispEdu);
            field("xDispEvpu", 1, target.xDispEvpu);
        }
        field("yDisp", 2, target.yDisp);
        field("hidden", 4, target.hidden);
    });
}

void importMeasureTextFamily(
    const ImportContext& context, const RecordFamilySource& source, EarlyBlockKeys& earlyBlocks, std::vector<ScaledVertical>& verticals)
{
    // Believed: before the DCL epoch the horizontal word is an EVPU offset from the measure's
    // left edge and the vertical word is staff-scaled. Later sources store a positive Edu or
    // negative EVPU displacement from the measure's first beat. Converting the earlier
    // horizontal offset needs the measure's layout, so it is kept as an EVPU displacement.
    const bool earlyCoordinates = !sourceAtOrAfter(context.profile, FormatEpoch::DclLegacy);
    const bool earlyConnectors = !context.index.getOthers().cmpersForTag(others::earlyTextBlockTag).empty();
    for (const auto& [partId, staffId] : recordKeys(source)) {
        for (const auto meas : source.pool->secondCmpersForTag(source.identity, staffId, partId)) {
            const auto rows = source.pool->getArray(source.identity, staffId, meas, partId);
            const auto words = collectRecordWords(source, rows, context.profile.byteOrder);
            if (words.size() % measureTextWordCount != 0) {
                context.report.diagnostics.push_back(
                    {musx::util::Logger::LogLevel::Info, "Measure text assignment for staff " + std::to_string(staffId) + ", measure "
                                                             + std::to_string(meas) + " has an incomplete trailing tuple."});
            }
            for (std::size_t at = 0; at + measureTextWordCount <= words.size(); at += measureTextWordCount) {
                const auto inci = static_cast<musx::dom::Inci>(at / measureTextWordCount);
                auto target = createDetailsRecordTarget<MeasureTextTarget>(context.document, source, rows.front(), staffId, meas, inci);
                if (!target) {
                    continue;
                }
                target->block = static_cast<musx::dom::Cmper>(words[at]);
                const auto xDisp = words[at + 1];
                if (earlyCoordinates || xDisp < 0) {
                    target->xDispEvpu = xDisp;
                } else {
                    target->xDispEdu = xDisp;
                }
                target->yDisp = words[at + 2];
                target->hidden = (static_cast<std::uint16_t>(words[at + 4]) & measureTextHiddenFlag) != 0;
                if (earlyConnectors) {
                    earlyBlocks.emplace(partId, target->block);
                }
                reportMeasureText(context, source, rows, *target, at, partId, earlyCoordinates);
                if (earlyCoordinates && target->yDisp != 0) {
                    verticals.push_back({target, &source.rowOfWord(rows, at + 2), source.byteOffsetInRow((at + 2) * 2), words[at + 2]});
                }
                context.document->getDetails()->add(MeasureTextTarget::XmlNodeName, std::move(target));
            }
        }
    }
}

} // namespace

void importMeasureTextAssigns(const ImportContext& context)
{
    // Coda-banner sources spell the fixed-row selector differently; neither spelling is
    // version-gated.
    const auto fixedTag =
        context.index.getDetails().cmpersForTag(others::measureTextTag).empty() ? others::codaMeasureTextTag : others::measureTextTag;
    const auto source =
        selectRecordFamilySource(context, context.index.getDetails(), context.index.getClassDetails(), fixedTag, measureTextClass, true);
    if (!source) {
        return;
    }
    EarlyBlockKeys earlyBlocks;
    std::vector<ScaledVertical> verticals;
    importMeasureTextFamily(context, *source, earlyBlocks, verticals);
    if (!verticals.empty()) {
        // Staff systems and their staff sizes finish before a vertical offset is unscaled.
        context.pending.checks.push_back(
            [&context, source = *source, verticals = std::move(verticals)] { unscaleVerticals(context, source, verticals); });
    }
    if (!earlyBlocks.empty()) {
        // Stored TextBlocks finish before a block without one resolves its PT connector.
        context.pending.checks.push_back([&context, earlyBlocks = std::move(earlyBlocks)] {
            for (const auto& [partId, blockId] : earlyBlocks) {
                others::resolveEarlyTextBlock(context, partId, blockId);
            }
        });
    }
}

} // namespace details
} // namespace finale_mus_reader
