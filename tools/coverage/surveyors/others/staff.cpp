// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/support/source_gate.h"
#include "coverage/surveyors/shared/staff_fields.h"
#include "coverage/surveyors/shared/staff_style_semantics.h"
#include "musx/musx.h"

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace {

using namespace finale_mus_reader::coverage;
using FretInstrumentSurveyTarget = musx::dom::others::FretInstrument;
using StaffSurveyTarget = musx::dom::others::Staff;

[[nodiscard]] bool companionOmitsStaffFromScrollView(const DifferenceContext& context)
{
    if (!context.companionDocument) return false;
    const auto staffId = staff_fields::staffLikeCmperFromComparisonPath(context.path);
    if (!staffId) return false;
    return !context.companionDocument->getScrollViewStaves(musx::dom::SCORE_PARTID)
                .getIndexForStaff(*staffId);
}

[[nodiscard]] bool sourceStaffUsesSixWordFallbacks(const DifferenceContext& context)
{
    const auto staffId = staff_fields::staffLikeCmperFromComparisonPath(context.path);
    if (!staffId) return false;
    const auto* restOffset = context.sourceReport.findField<StaffSurveyTarget>(
        "dwRestOffset", musx::dom::SCORE_PARTID, *staffId);
    return restOffset && restOffset->origin == finale_mus_reader::ValueOrigin::Finale27Default;
}

[[nodiscard]] bool sourceStaffUsesSynthesizedFretInstrument(const DifferenceContext& context)
{
    if (!context.sourceValue.isInteger()) return false;
    const auto fretInstId = context.sourceValue.asInteger();
    if (fretInstId <= 0 || fretInstId > (std::numeric_limits<musx::dom::Cmper>::max)()) {
        return false;
    }
    const auto instance = finale_mus_reader::instanceKey<FretInstrumentSurveyTarget>(
        musx::dom::SCORE_PARTID, static_cast<musx::dom::Cmper>(fretInstId));
    const auto* origin = context.sourceReport.findInstanceOrigin(instance);
    return origin && *origin == finale_mus_reader::ValueOrigin::LegacyBehavior;
}

std::optional<DifferenceClassification> classifyStaffDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    constexpr std::string_view useNoteFontSuffix = ".use_note_font";
    constexpr std::string_view noteFontSizeSuffix = ".note_font.font_size";
    constexpr std::string_view fullNameTextIdSuffix = ".full_name_text_id";
    constexpr std::string_view abbrvNameTextIdSuffix = ".abbrv_name_text_id";
    constexpr std::string_view transpositionPresentSuffix = ".transposition.present";
    constexpr std::string_view keysigPresentSuffix = ".transposition.keysig.present";
    constexpr std::string_view keysigIntervalSuffix = ".transposition.keysig.interval";
    constexpr std::string_view keysigAdjustSuffix = ".transposition.keysig.adjust";
    constexpr std::string_view hideMeasNumsSuffix = ".hide_meas_nums";
    constexpr std::string_view hideNameInScoreSuffix = ".hide_name_in_score";
    constexpr std::string_view hideModeSuffix = ".hide_mode";
    constexpr std::string_view hideKeySigsShowAccisSuffix = ".hide_key_sigs_show_accis";
    constexpr std::string_view fretInstIdSuffix = ".fret_inst_id";
    constexpr std::string_view breakTabLinesAtNotesSuffix = ".break_tab_lines_at_notes";
    const auto noneHideMode = static_cast<std::int64_t>(StaffSurveyTarget::HideMode::None);
    const auto scoreHideMode = static_cast<std::int64_t>(StaffSurveyTarget::HideMode::Score);
    if (const auto classification =
            staff_fields::classifyDisabledNoteFontSize(context, "staff[")) {
        return classification;
    }
    if (const auto classification =
            staff_fields::classifyNoteAttachedItemsExpressionUpgradeLoss(context, "staff[")) {
        return classification;
    }
    if (deferredRecoveryClassified() && context.category == Differs &&
        context.origin == "legacy-mus" && comparisonPathStartsWith(context.path, "staff[")) {
        for (const auto suffix : {fullNameTextIdSuffix, abbrvNameTextIdSuffix, noteFontSizeSuffix,
                 transpositionPresentSuffix, keysigPresentSuffix, keysigIntervalSuffix,
                 keysigAdjustSuffix, hideMeasNumsSuffix, hideNameInScoreSuffix}) {
            if (comparisonPathEndsWith(context.path, suffix) &&
                companionOmitsStaffFromScrollView(context)) {
                return DifferenceClassification::AwaitsDependentRecovery;
            }
        }
    }
    if (context.category == Differs && context.origin == "legacy-mus" &&
        comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, useNoteFontSuffix) && context.sourceValue.isBool() &&
        context.companionValue.isBool() &&
        context.sourceValue.asBool() != context.companionValue.asBool() &&
        sourcePredatesVersion(context.epoch, context.sourceVersion,
            finale_mus_reader::FormatEpoch::ZlibLegacy, finale_mus_reader::versions::finale2012)) {
        const auto objectPath =
            context.path.substr(0, context.path.size() - useNoteFontSuffix.size());
        const auto notationStyle =
            comparisonIntegerLeaf(context.source, std::string(objectPath) + ".notation_style");
        if (notationStyle ==
                static_cast<std::int64_t>(StaffSurveyTarget::NotationStyle::Percussion) ||
            notationStyle ==
                static_cast<std::int64_t>(StaffSurveyTarget::NotationStyle::Tablature)) {
            return DifferenceClassification::FinaleUpgradeLoss;
        }
    }
    if (context.category == Differs && context.origin == "legacy-behavior" &&
        comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, fretInstIdSuffix) &&
        sourceStaffUsesSynthesizedFretInstrument(context)) {
        return DifferenceClassification::DifferentDefaults;
    }
    if (context.category == Differs && context.origin == "finale27-default" &&
        comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, breakTabLinesAtNotesSuffix)) {
        return DifferenceClassification::DifferentDefaults;
    }
    if (context.category == Differs && context.origin == "unmapped" &&
        comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, ".hide_key_sigs") &&
        sourceStaffUsesSixWordFallbacks(context)) {
        return DifferenceClassification::PossiblyUnrecoverable;
    }
    if (deferredRecoveryClassified() && context.category == Differs &&
        context.origin == "unmapped" && comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, ".has_styles")) {
        return DifferenceClassification::AwaitsDependentRecovery;
    }
    if (deferredRecoveryClassified() && context.category == Differs &&
        context.origin == "legacy-mus-adjusted" &&
        comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, ".has_styles") && context.sourceValue.isBool() &&
        !context.sourceValue.asBool() && context.companionValue.isBool() &&
        context.companionValue.asBool() &&
        sourcePredatesVersion(context.epoch, context.sourceVersion,
                              finale_mus_reader::FormatEpoch::UncompressedLegacy,
                              finale_mus_reader::versions::finale2000)) {
        return DifferenceClassification::AwaitsDependentRecovery;
    }
    if (context.category == Differs && context.origin == "legacy-mus-adjusted" &&
        comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, ".has_styles") && context.sourceValue.isBool() &&
        !context.sourceValue.asBool() && context.companionValue.isBool() &&
        context.companionValue.asBool() &&
        sourceAtOrAfter(context.epoch, context.sourceVersion,
                        finale_mus_reader::FormatEpoch::UncompressedLegacy,
                        finale_mus_reader::versions::finale2000)) {
        const auto partId = staff_fields::partIdFromComparisonPath(context.path);
        const auto staffId = staff_fields::staffLikeCmperFromComparisonPath(context.path);
        if (context.sourceReport.staffStyleAssignmentAuditComplete && partId && staffId &&
            !context.sourceReport.findStaffStyleAssignmentAudit(
                *partId, static_cast<musx::dom::Cmper>(*staffId))) {
            return DifferenceClassification::FinaleUpgradeSynthesis;
        }
    }
    if (context.category == Differs && context.origin == "finale27-default" &&
        comparisonPathStartsWith(context.path, "staff[")) {
        for (const auto suffix : {".dw_rest_offset", ".w_rest_offset", ".h_rest_offset",
                 ".other_rest_offset", ".stem_reversal"}) {
            if (comparisonPathEndsWith(context.path, suffix)) {
                return DifferenceClassification::PossiblyUnrecoverable;
            }
        }
        if (comparisonPathEndsWith(context.path, fretInstIdSuffix) &&
            context.sourceValue.isInteger() && context.sourceValue.asInteger() == 0 &&
            context.companionValue.isInteger()) {
            return DifferenceClassification::PossiblyUnrecoverable;
        }
    }
    if (context.category == Differs && context.origin == "legacy-behavior" &&
        comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, ".inst_uuid") &&
        sourcePredatesVersion(context.epoch, context.sourceVersion,
            finale_mus_reader::FormatEpoch::ZlibLegacy, finale_mus_reader::versions::finale2012)) {
        return DifferenceClassification::DifferentDefaults;
    }
    if (context.category == Differs && context.origin == "legacy-behavior" &&
        comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, hideKeySigsShowAccisSuffix) &&
        context.sourceValue.isBool() && !context.sourceValue.asBool() &&
        context.companionValue.isBool() && context.companionValue.asBool() &&
        sourceIsBeta(context.sourceVersion) &&
        sourceIsVersion(context.epoch, context.sourceVersion,
            finale_mus_reader::FormatEpoch::ZlibLegacy,
            finale_mus_reader::versions::finale2012)) {
        return DifferenceClassification::BetaDiscrepancy;
    }
    if (context.category == Differs && context.origin == "finale27-default" &&
        comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, hideModeSuffix) && context.sourceValue.isInteger() &&
        context.companionValue.isInteger() && context.sourceValue.asInteger() == noneHideMode) {
        return DifferenceClassification::DifferentDefaults;
    }
    if (context.category == Differs && context.origin == "legacy-mus" &&
        comparisonPathStartsWith(context.path, "staff[") &&
        comparisonPathEndsWith(context.path, hideModeSuffix) && context.sourceValue.isInteger() &&
        context.companionValue.isInteger() && context.companionValue.asInteger() == scoreHideMode &&
        sourcePredatesVersion(context.epoch, context.sourceVersion,
            finale_mus_reader::FormatEpoch::ZlibLegacy, finale_mus_reader::versions::finale2011)) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

template <typename T> Value staffValue(T value)
{
    if constexpr (std::is_enum_v<T>)
        return Value(static_cast<std::int64_t>(value));
    else
        return Value(value);
}

std::string staffOrigin(
    const StaffSurveyTarget& staff, const SurveyContext& context, std::string_view member)
{
    return fieldOrigin<StaffSurveyTarget>(context, member, staff);
}

template <typename T>
void addStaffLeaf(Value::Object& object, const StaffSurveyTarget& staff,
    const SurveyContext& context, std::string_view reportMember, std::string_view originMember,
    std::string_view leaf, T value)
{
    object.emplace(std::string(leaf), staffValue(value));
    object.emplace(
        "origin_" + std::string(originMember), staffOrigin(staff, context, reportMember));
}

template <typename T>
void addStaffLeaf(Value::Object& object, const StaffSurveyTarget& staff,
    const SurveyContext& context, std::string_view member, std::string_view leaf, T value)
{
    addStaffLeaf(object, staff, context, member, member, leaf, value);
}

Value observeStaff(const StaffSurveyTarget& staff, const SurveyContext& context)
{
    Value::Object result;
    result.emplace("cmper", staff.getCmper());
    result.emplace("part_id", staff.getSourcePartId());
    result.emplace("share_mode", static_cast<std::int64_t>(staff.getShareMode()));
    const auto instance = finale_mus_reader::instanceKey<StaffSurveyTarget>(
        staff.getSourcePartId(), staff.getCmper());
    if (const auto* origin = context.report.findInstanceOrigin(instance)) {
        result.emplace("origin", originName(*origin));
    }

#define STAFF_LEAF(member, leaf) addStaffLeaf(result, staff, context, #member, #leaf, staff.member)
    STAFF_LEAF(notationStyle, notation_style);
    STAFF_LEAF(useNoteShapes, use_note_shapes);
    STAFF_LEAF(useNoteFont, use_note_font);
    STAFF_LEAF(defaultClef, default_clef);
    STAFF_LEAF(transposedClef, transposed_clef);
    addStaffLeaf(result, staff, context, "staffLines", "staff_lines", staff.staffLines.value_or(0));
    Value::Array customStaff;
    if (staff.customStaff) {
        for (const auto line : *staff.customStaff) customStaff.emplace_back(line);
    }
    result.emplace("custom_staff", std::move(customStaff));
    result.emplace("origin_customStaff", staffOrigin(staff, context, "customStaff"));
    STAFF_LEAF(lineSpace, line_space);
    STAFF_LEAF(instUuid, inst_uuid);
    STAFF_LEAF(capoPos, capo_pos);
    STAFF_LEAF(lowestFret, lowest_fret);
    STAFF_LEAF(floatKeys, float_keys);
    STAFF_LEAF(floatTime, float_time);
    STAFF_LEAF(blineBreak, bline_break);
    STAFF_LEAF(rbarBreak, rbar_break);
    STAFF_LEAF(hasStyles, has_styles);
    STAFF_LEAF(showNameInParts, show_name_in_parts);
    STAFF_LEAF(showNoteColors, show_note_colors);
    STAFF_LEAF(hideNameInScore, hide_name_in_score);
    STAFF_LEAF(botBarlineOffset, bot_barline_offset);
    STAFF_LEAF(altNotation, alt_notation);
    STAFF_LEAF(altLayer, alt_layer);
    STAFF_LEAF(altHideArtics, alt_hide_artics);
    STAFF_LEAF(altHideLyrics, alt_hide_lyrics);
    STAFF_LEAF(altHideSmartShapes, alt_hide_smart_shapes);
    STAFF_LEAF(altRhythmStemsUp, alt_rhythm_stems_up);
    STAFF_LEAF(altSlashDots, alt_slash_dots);
    STAFF_LEAF(altHideOtherNotes, alt_hide_other_notes);
    STAFF_LEAF(altHideOtherArtics, alt_hide_other_artics);
    STAFF_LEAF(altHideExpressions, alt_hide_expressions);
    STAFF_LEAF(altHideOtherLyrics, alt_hide_other_lyrics);
    STAFF_LEAF(altHideOtherSmartShapes, alt_hide_other_smart_shapes);
    STAFF_LEAF(altHideOtherExpressions, alt_hide_other_expressions);
    STAFF_LEAF(hideRepeatBottomDot, hide_repeat_bottom_dot);
    STAFF_LEAF(flatBeams, flat_beams);
    STAFF_LEAF(hideFretboards, hide_fretboards);
    STAFF_LEAF(blankMeasure, blank_measure);
    STAFF_LEAF(hideRepeatTopDot, hide_repeat_top_dot);
    STAFF_LEAF(hideLyrics, hide_lyrics);
    STAFF_LEAF(noOptimize, no_optimize);
    STAFF_LEAF(topBarlineOffset, top_barline_offset);
    STAFF_LEAF(hideMeasNums, hide_meas_nums);
    STAFF_LEAF(hideRepeats, hide_repeats);
    STAFF_LEAF(hideBarlines, hide_barlines);
    STAFF_LEAF(hideRptBars, hide_rpt_bars);
    STAFF_LEAF(hideKeySigs, hide_key_sigs);
    STAFF_LEAF(hideTimeSigs, hide_time_sigs);
    STAFF_LEAF(hideClefs, hide_clefs);
    STAFF_LEAF(hideStaffLines, hide_staff_lines);
    STAFF_LEAF(hideChords, hide_chords);
    STAFF_LEAF(noKey, no_key);
    STAFF_LEAF(dwRestOffset, dw_rest_offset);
    STAFF_LEAF(wRestOffset, w_rest_offset);
    STAFF_LEAF(hRestOffset, h_rest_offset);
    STAFF_LEAF(otherRestOffset, other_rest_offset);
    STAFF_LEAF(hideRests, hide_rests);
    STAFF_LEAF(hideTies, hide_ties);
    STAFF_LEAF(hideDots, hide_dots);
    STAFF_LEAF(stemReversal, stem_reversal);
    STAFF_LEAF(fullNameTextId, full_name_text_id);
    STAFF_LEAF(abbrvNameTextId, abbrv_name_text_id);
    STAFF_LEAF(botRepeatDotOff, bot_repeat_dot_off);
    STAFF_LEAF(topRepeatDotOff, top_repeat_dot_off);
    STAFF_LEAF(vertTabNumOff, vert_tab_num_off);
    STAFF_LEAF(showTabClefAllSys, show_tab_clef_all_sys);
    STAFF_LEAF(useTabLetters, use_tab_letters);
    STAFF_LEAF(breakTabLinesAtNotes, break_tab_lines_at_notes);
    STAFF_LEAF(hideTuplets, hide_tuplets);
    STAFF_LEAF(fretInstId, fret_inst_id);
    STAFF_LEAF(hideStems, hide_stems);
    STAFF_LEAF(stemDirection, stem_direction);
    STAFF_LEAF(hideBeams, hide_beams);
    STAFF_LEAF(stemStartFromStaff, stem_start_from_staff);
    STAFF_LEAF(stemsFixedEnd, stems_fixed_end);
    STAFF_LEAF(stemsFixedStart, stems_fixed_start);
    STAFF_LEAF(horzStemOffUp, horz_stem_off_up);
    STAFF_LEAF(horzStemOffDown, horz_stem_off_down);
    STAFF_LEAF(vertStemStartOffUp, vert_stem_start_off_up);
    STAFF_LEAF(vertStemStartOffDown, vert_stem_start_off_down);
    STAFF_LEAF(vertStemEndOffUp, vert_stem_end_off_up);
    STAFF_LEAF(vertStemEndOffDown, vert_stem_end_off_down);
    STAFF_LEAF(hideMode, hide_mode);
    STAFF_LEAF(redisplayLayerAccis, redisplay_layer_accis);
    STAFF_LEAF(hideTimeSigsInParts, hide_time_sigs_in_parts);
    STAFF_LEAF(autoNumbering, auto_numbering);
    STAFF_LEAF(useAutoNumbering, use_auto_numbering);
    STAFF_LEAF(hideKeySigsShowAccis, hide_key_sigs_show_accis);
#undef STAFF_LEAF

    Value::Object noteFont;
    const auto font = staff.noteFont;
#define STAFF_FONT_LEAF(member, leaf)                                                              \
    addStaffLeaf(noteFont, staff, context, "noteFont." #member, #member, #leaf,                    \
        font ? font->member : decltype(font->member){})
    STAFF_FONT_LEAF(fontId, font_id);
    STAFF_FONT_LEAF(fontSize, font_size);
    STAFF_FONT_LEAF(bold, bold);
    STAFF_FONT_LEAF(italic, italic);
    STAFF_FONT_LEAF(underline, underline);
    STAFF_FONT_LEAF(strikeout, strikeout);
    STAFF_FONT_LEAF(absolute, absolute);
    STAFF_FONT_LEAF(hidden, hidden);
#undef STAFF_FONT_LEAF
    result.emplace("note_font", std::move(noteFont));

    Value::Object transposition;
    transposition.emplace("present", bool(staff.transposition));
    const auto trans = staff.transposition;
    addStaffLeaf(transposition, staff, context, "transposition.setToClef", "setToClef",
        "set_to_clef", trans ? trans->setToClef : false);
    addStaffLeaf(transposition, staff, context, "transposition.noSimplifyKey", "noSimplifyKey",
        "no_simplify_key", trans ? trans->noSimplifyKey : false);
    Value::Object keysig;
    addStaffLeaf(keysig, staff, context, "transposition.keysig.interval", "interval", "interval",
        trans && trans->keysig ? trans->keysig->interval : 0);
    addStaffLeaf(keysig, staff, context, "transposition.keysig.adjust", "adjust", "adjust",
        trans && trans->keysig ? trans->keysig->adjust : 0);
    keysig.emplace("present", bool(trans && trans->keysig));
    transposition.emplace("keysig", std::move(keysig));
    Value::Object chromatic;
    addStaffLeaf(chromatic, staff, context, "transposition.chromatic.alteration", "alteration",
        "alteration", trans && trans->chromatic ? trans->chromatic->alteration : 0);
    addStaffLeaf(chromatic, staff, context, "transposition.chromatic.diatonic", "diatonic",
        "diatonic", trans && trans->chromatic ? trans->chromatic->diatonic : 0);
    chromatic.emplace("present", bool(trans && trans->chromatic));
    transposition.emplace("chromatic", std::move(chromatic));
    result.emplace("transposition", std::move(transposition));
    return Value(std::move(result));
}

Value observeStaffs(const SurveyContext& context)
{
    Value::Array result;
    for (const auto& staff : sourceInstances<StaffSurveyTarget>(context)) {
        if (staff->getCmper() == musx::dom::STUDIO_VIEW_STAFF_ID) continue;
        result.push_back(observeStaff(*staff, context));
    }
    return Value(std::move(result));
}

COVERAGE_CLASS("others", "staff", observeStaffs, classifyStaffDifference);

} // namespace
