// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/details.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "import/shared/staff_defaults.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace details {
namespace {

using StaffStyleAssignTarget = musx::dom::others::StaffStyleAssign;
using StaffStyleAssignStaffTarget = musx::dom::others::Staff;
using StaffStyleAssignStyleTarget = musx::dom::others::StaffStyle;

constexpr auto gframeHoldTag = records::packTag("GF");
constexpr std::size_t gframeHoldEarlyFlagsSlot = 4;
constexpr std::size_t gframeHoldFinale98FlagsSlot = 1;
constexpr std::uint16_t gframeHoldAlternateNotationMask = 0x000f;

[[nodiscard]] std::size_t gframeHoldFlagsSlot(const SourceProfile& profile)
{
    // Preliminary: presumed flags occupy word 4 before Finale 98 and word 1 beginning
    // with Finale 98. No structural discriminator is known; an absent version uses word 4.
    return sourceAtOrAfter(profile, FormatEpoch::UncompressedLegacy, versions::finale98) ? gframeHoldFinale98FlagsSlot : gframeHoldEarlyFlagsSlot;
}

struct AlternateNotationStyle
{
    StaffStyleAssignStaffTarget::AlternateNotation notation;
    musx::dom::Cmper preferredStyleId;
    std::string_view name;
};

struct AlternateNotationRun
{
    std::uint16_t partId{};
    musx::dom::Cmper staffId{};
    musx::dom::MeasCmper startMeas{};
    musx::dom::MeasCmper endMeas{};
    AlternateNotationStyle style;
    const records::LegacyRow* firstRow{};
    std::size_t flagsSlot{};
};

constexpr std::array canonicalAlternateNotationStyles{
    AlternateNotationStyle{StaffStyleAssignStaffTarget::AlternateNotation::Normal, musx::dom::Cmper{1}, "Normal Notation"},
    AlternateNotationStyle{StaffStyleAssignStaffTarget::AlternateNotation::SlashBeats, musx::dom::Cmper{2}, "Slash Notation"},
    AlternateNotationStyle{StaffStyleAssignStaffTarget::AlternateNotation::Rhythmic, musx::dom::Cmper{3}, "Rhythmic Notation"},
    AlternateNotationStyle{StaffStyleAssignStaffTarget::AlternateNotation::OneBarRepeat, musx::dom::Cmper{4}, "One Bar Repeats"},
    AlternateNotationStyle{StaffStyleAssignStaffTarget::AlternateNotation::TwoBarRepeat, musx::dom::Cmper{5}, "Two Bar Repeats"},
    AlternateNotationStyle{StaffStyleAssignStaffTarget::AlternateNotation::Blank, musx::dom::Cmper{6}, "Blank Notation"},
};

[[nodiscard]] std::optional<AlternateNotationStyle> alternateNotationStyle(std::uint16_t stored)
{
    switch (stored) {
    case 1: return canonicalAlternateNotationStyles[1];
    case 2: return canonicalAlternateNotationStyles[2];
    case 3: return canonicalAlternateNotationStyles[3];
    case 4:
    case 5: return canonicalAlternateNotationStyles[4];
    case 6: return canonicalAlternateNotationStyles[5];
    default: return std::nullopt;
    }
}

void applyAlternateNotationStyleBehavior(StaffStyleAssignStyleTarget& target)
{
    using Notation = StaffStyleAssignStaffTarget::AlternateNotation;
    target.copyable = true;
    target.addToMenu = true;
    target.altSlashDots = true;
    switch (target.altNotation) {
    case Notation::SlashBeats:
        target.altHideSmartShapes = true;
        target.altHideOtherNotes = true;
        target.altHideOtherArtics = true;
        target.altHideOtherLyrics = true;
        target.altHideOtherSmartShapes = true;
        target.altHideOtherExpressions = true;
        break;
    case Notation::Rhythmic:
        target.altHideOtherNotes = true;
        target.altHideOtherArtics = true;
        target.altHideOtherLyrics = true;
        target.altHideOtherSmartShapes = true;
        target.altHideOtherExpressions = true;
        break;
    case Notation::OneBarRepeat:
    case Notation::TwoBarRepeat:
        target.altHideArtics = true;
        target.altHideLyrics = true;
        target.altHideSmartShapes = true;
        target.altHideOtherNotes = true;
        target.altHideOtherArtics = true;
        target.altHideOtherLyrics = true;
        target.altHideOtherSmartShapes = true;
        target.altHideOtherExpressions = true;
        target.hideChords = true;
        target.hideFretboards = true;
        break;
    case Notation::Blank: target.altHideSmartShapes = true; break;
    case Notation::Normal:
    case Notation::BlankWithRests: break;
    }
}

void reportSynthesizedAlternateNotationStyle(const ImportContext& context, const StaffStyleAssignStyleTarget& target)
{
    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
        const auto key = reporting.template instanceKey<StaffStyleAssignStyleTarget>(target.getSourcePartId(), target.getCmper());
        reporting.report().setInstanceOrigin(key, Reporting::Origin::Unmapped);
        const auto behavior = [&](const char* member, std::int64_t value) {
            reporting.report().setField(key, member, {Reporting::Origin::LegacyBehavior, 0, 0, value});
        };
        const auto defaulted = [&](const char* member, std::int64_t value) {
            reporting.report().setField(key, member, {Reporting::Origin::Finale27Default, 0, 0, value});
        };
        behavior("styleName", static_cast<std::int64_t>(target.styleName.size()));
        behavior("copyable", target.copyable);
        behavior("addToMenu", target.addToMenu);
        behavior("instUuid", 0);
        behavior("staffLines", target.staffLines.value_or(0));
        behavior("altNotation", static_cast<std::int64_t>(target.altNotation));
        behavior("altLayer", target.altLayer);
        behavior("altHideArtics", target.altHideArtics);
        behavior("altHideLyrics", target.altHideLyrics);
        behavior("altHideSmartShapes", target.altHideSmartShapes);
        behavior("altRhythmStemsUp", target.altRhythmStemsUp);
        behavior("altSlashDots", target.altSlashDots);
        behavior("altHideOtherNotes", target.altHideOtherNotes);
        behavior("altHideOtherArtics", target.altHideOtherArtics);
        behavior("altHideExpressions", target.altHideExpressions);
        behavior("altHideOtherLyrics", target.altHideOtherLyrics);
        behavior("altHideOtherSmartShapes", target.altHideOtherSmartShapes);
        behavior("altHideOtherExpressions", target.altHideOtherExpressions);
        behavior("hideChords", target.hideChords);
        behavior("hideFretboards", target.hideFretboards);
        behavior("masks.altNotation", target.masks->altNotation);
        behavior("masks.hideChords", target.masks->hideChords);
        behavior("masks.hideFretboards", target.masks->hideFretboards);
        defaulted("botRepeatDotOff", target.botRepeatDotOff);
        defaulted("topRepeatDotOff", target.topRepeatDotOff);
        defaulted("dwRestOffset", target.dwRestOffset);
        defaulted("wRestOffset", target.wRestOffset);
        defaulted("hRestOffset", target.hRestOffset);
        defaulted("otherRestOffset", target.otherRestOffset);
        defaulted("lineSpace", target.lineSpace);
        defaulted("noteFont.fontSize", target.noteFont->fontSize);
        defaulted("stemReversal", target.stemReversal);
    });
}

void reportAlternateNotationStyleSource(const ImportContext& context, const StaffStyleAssignStyleTarget& target, const records::LegacyRow& row,
    std::size_t flagsSlot, std::uint16_t storedType)
{
    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
        const auto key = reporting.template instanceKey<StaffStyleAssignStyleTarget>(target.getSourcePartId(), target.getCmper());
        reporting.report().setField(key, "altNotation",
            {Reporting::Origin::LegacyMusAdjusted, row.blockOffset, row.decodedOffset + flagsSlot * sizeof(std::uint16_t), storedType,
                gframeHoldTag});
    });
}

[[nodiscard]] std::shared_ptr<const StaffStyleAssignStyleTarget> findAlternateNotationStyle(
    const ImportContext& context, StaffStyleAssignStaffTarget::AlternateNotation notation)
{
    const auto styles = context.document->getOthers()->getAllSources<StaffStyleAssignStyleTarget>();
    const auto found = std::ranges::find_if(styles, [&](const auto& style) {
        return style->getSourcePartId() == musx::dom::SCORE_PARTID && style->masks && style->masks->altNotation && style->altNotation == notation;
    });
    return found == styles.end() ? nullptr : *found;
}

[[nodiscard]] std::shared_ptr<const StaffStyleAssignStyleTarget> ensureAlternateNotationStyle(const ImportContext& context,
    const AlternateNotationStyle& style, const records::LegacyRow* sourceRow = nullptr, std::size_t flagsSlot = 0, std::uint16_t storedType = 0)
{
    if (auto existing = findAlternateNotationStyle(context, style.notation)) {
        if (sourceRow) {
            reportAlternateNotationStyleSource(context, *existing, *sourceRow, flagsSlot, storedType);
        }
        return existing;
    }

    auto styleId = style.preferredStyleId;
    if (context.document->getOthers()->get<StaffStyleAssignStyleTarget>(musx::dom::SCORE_PARTID, styleId)) {
        const auto nextStyleId = context.document->getOthers()->nextFreeCmper<StaffStyleAssignStyleTarget>(musx::dom::SCORE_PARTID);
        if (!nextStyleId) {
            context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info, "Alternate notation range has no available Staff Style "
                                                                                      "identifier."});
            return nullptr;
        }
        styleId = *nextStyleId;
    }
    auto target =
        std::make_shared<StaffStyleAssignStyleTarget>(context.document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, styleId);
    target->styleName = style.name;
    target->altNotation = style.notation;
    target->staffLines = 5;
    target->instUuid = std::string(musx::dom::uuid::BlankStaff);
    const auto& defaults = finale27StaffDefaults(context);
    if (!target->noteFont) {
        target->noteFont = std::make_shared<musx::dom::FontInfo>(target->getDocument());
    }
    target->botRepeatDotOff = defaults.botRepeatDotOff;
    target->topRepeatDotOff = defaults.topRepeatDotOff;
    target->dwRestOffset = defaults.dwRestOffset;
    target->wRestOffset = defaults.wRestOffset;
    target->hRestOffset = defaults.hRestOffset;
    target->otherRestOffset = defaults.otherRestOffset;
    target->lineSpace = defaults.lineSpace;
    target->noteFont->fontSize = finale27NoteheadFontSize(context);
    target->stemReversal = defaults.stemReversal;
    target->masks = std::make_shared<StaffStyleAssignStyleTarget::Masks>(target);
    target->masks->altNotation = true;
    target->masks->hideChords = true;
    target->masks->hideFretboards = true;
    applyAlternateNotationStyleBehavior(*target);
    reportSynthesizedAlternateNotationStyle(context, *target);
    if (sourceRow) {
        reportAlternateNotationStyleSource(context, *target, *sourceRow, flagsSlot, storedType);
    }
    context.document->getOthers()->add(StaffStyleAssignStyleTarget::XmlNodeName, target);
    return target;
}

void reportSynthesizedAlternateNotationAssignment(
    const ImportContext& context, const StaffStyleAssignTarget& target, const AlternateNotationRun& run, std::uint16_t storedType)
{
    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
        const auto key = reporting.template instanceKey<StaffStyleAssignTarget>(target.getSourcePartId(), target.getCmper(), target.getInci());
        reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMusAdjusted);
        const auto adjusted = [&](const char* member, std::int64_t value) {
            reporting.report().setField(
                key, member, {Reporting::Origin::LegacyMusAdjusted, run.firstRow->blockOffset, run.firstRow->decodedOffset, value, gframeHoldTag});
        };
        adjusted("styleId", storedType);
        adjusted("startMeas", run.startMeas);
        adjusted("endMeas", run.endMeas);
        reporting.report().setField(key, "startEdu", {Reporting::Origin::LegacyBehavior, 0, 0, 0});
        reporting.report().setField(key, "endEdu", {Reporting::Origin::LegacyBehavior, 0, 0, target.endEdu});
    });
}

void addAlternateNotationRun(const ImportContext& context, const AlternateNotationRun& run, std::uint16_t storedType)
{
    const auto style = ensureAlternateNotationStyle(context, run.style, run.firstRow, run.flagsSlot, storedType);
    if (!style) {
        return;
    }
    const auto existing = context.document->getOthers()->getArray<StaffStyleAssignTarget>(run.partId, run.staffId);
    const auto inci = static_cast<musx::dom::Inci>(existing.size());
    auto target = std::make_shared<StaffStyleAssignTarget>(context.document, run.partId, musx::dom::EnigmaBase::ShareMode::All, run.staffId, inci);
    target->styleId = style->getCmper();
    target->startMeas = run.startMeas;
    target->startEdu = 0;
    target->endMeas = run.endMeas;
    target->endEdu = (std::numeric_limits<musx::dom::Edu>::max)();
    reportSynthesizedAlternateNotationAssignment(context, *target, run, storedType);
    context.document->getOthers()->add(StaffStyleAssignTarget::XmlNodeName, std::move(target));
    withReporting(context.report,
        [&]<typename Reporting>(Reporting& reporting) { reporting.report().expectStaffStyleAssignments(run.partId, run.staffId, 1, false); });
}

void synthesizeAlternateNotationRanges(const ImportContext& context)
{
    // GF is the pre-Finale-2000 representation. The version gate is confined to
    // the uncompressed epoch; every Coda-banner source predates it, and later
    // epochs do not use it.
    if (!sourcePredatesVersion(context.profile, FormatEpoch::UncompressedLegacy, versions::finale2000)) {
        return;
    }

    for (const auto& style : canonicalAlternateNotationStyles) {
        static_cast<void>(ensureAlternateNotationStyle(context, style));
    }

    const RecordFamilySource source{&context.index.getDetails(), gframeHoldTag, false, true};
    std::set<std::uint16_t> unknownTypes;
    for (const auto& [partId, staffId] : recordKeys(source)) {
        std::optional<AlternateNotationRun> run;
        std::uint16_t runStoredType{};
        const auto finishRun = [&] {
            if (run) {
                addAlternateNotationRun(context, *run, runStoredType);
            }
            run.reset();
        };
        for (const auto measure : source.pool->secondCmpersForTag(source.identity, staffId, partId)) {
            const auto rows = source.pool->getArray(source.identity, staffId, measure, partId);
            if (rows.empty()) {
                continue;
            }
            const auto flagsSlot = gframeHoldFlagsSlot(context.profile);
            const auto storedType =
                static_cast<std::uint16_t>(static_cast<std::uint16_t>(rows.front().words[flagsSlot]) & gframeHoldAlternateNotationMask);
            const auto style = alternateNotationStyle(storedType);
            if (!style) {
                finishRun();
                if (storedType != 0) {
                    unknownTypes.insert(storedType);
                }
                continue;
            }
            const auto measureId = static_cast<musx::dom::MeasCmper>(measure);
            if (run && run->style.notation == style->notation && measureId == static_cast<musx::dom::MeasCmper>(run->endMeas + 1)) {
                run->endMeas = measureId;
                continue;
            }
            finishRun();
            run = AlternateNotationRun{partId, staffId, measureId, measureId, *style, &rows.front(), flagsSlot};
            runStoredType = storedType;
        }
        finishRun();
    }
    for (const auto storedType : unknownTypes) {
        context.report.diagnostics.push_back(
            {musx::util::Logger::LogLevel::Info, "Alternate notation range has unsupported type " + std::to_string(storedType) + "."});
    }
}

} // namespace

void importGFrameHolds(const ImportContext& context)
{
    context.pending.materialize.push_back([&context] { synthesizeAlternateNotationRanges(context); });
}

} // namespace details
} // namespace finale_mus_reader
