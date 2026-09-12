// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "coverage/classification.h"
#include "coverage/schema.h"
#include "coverage/support/source_gate.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace finale_mus_reader {
namespace coverage {
namespace staff_fields {

[[nodiscard]] inline std::optional<musx::dom::Cmper>
partIdFromComparisonPath(std::string_view path)
{
    constexpr std::string_view partIdKey = "part_id=";
    const auto identityBegin = path.find('[');
    const auto identityEnd = path.find(']', identityBegin);
    if (identityBegin == std::string_view::npos || identityEnd == std::string_view::npos)
        return std::nullopt;
    const auto valueBegin = path.find(partIdKey, identityBegin + 1);
    if (valueBegin == std::string_view::npos || valueBegin >= identityEnd)
        return musx::dom::SCORE_PARTID;
    const auto digitsBegin = valueBegin + partIdKey.size();
    const auto digitsEnd = path.find_first_of(",]", digitsBegin);
    if (digitsEnd == std::string_view::npos || digitsEnd > identityEnd)
        return std::nullopt;

    musx::dom::Cmper value{};
    const auto [parsedEnd, error] =
        std::from_chars(path.data() + digitsBegin, path.data() + digitsEnd, value);
    return error == std::errc{} && parsedEnd == path.data() + digitsEnd
        ? std::optional{value}
        : std::nullopt;
}

[[nodiscard]] inline std::optional<musx::dom::StaffCmper>
staffLikeCmperFromComparisonPath(std::string_view path)
{
    constexpr std::string_view cmperKey = "cmper=";
    const auto identityBegin = path.find('[');
    const auto identityEnd = path.find(']', identityBegin);
    if (identityBegin == std::string_view::npos || identityEnd == std::string_view::npos) {
        return std::nullopt;
    }
    const auto valueBegin = path.find(cmperKey, identityBegin + 1);
    if (valueBegin == std::string_view::npos || valueBegin >= identityEnd)
        return std::nullopt;
    const auto digitsBegin = valueBegin + cmperKey.size();
    const auto digitsEnd = path.find_first_of(",]", digitsBegin);
    if (digitsEnd == std::string_view::npos || digitsEnd > identityEnd)
        return std::nullopt;

    using UnsignedStaffCmper = std::make_unsigned_t<musx::dom::StaffCmper>;
    UnsignedStaffCmper value{};
    const auto [parsedEnd, error] =
        std::from_chars(path.data() + digitsBegin, path.data() + digitsEnd, value);
    if (error != std::errc{} || parsedEnd != path.data() + digitsEnd ||
        value >
            static_cast<UnsignedStaffCmper>((std::numeric_limits<musx::dom::StaffCmper>::max)())) {
        return std::nullopt;
    }
    return static_cast<musx::dom::StaffCmper>(value);
}

inline bool sourceHasNoteAttachedItemsExpansion(const DifferenceContext& context,
                                                std::string_view objectPath)
{
    struct ExpectedLeaf
    {
        std::string_view suffix;
        std::string_view sourceOrigin;
    };
    constexpr ExpectedLeaf expectedLeaves[] = {
        {".alt_hide_artics", "legacy-mus"},
        {".alt_hide_lyrics", "legacy-mus"},
        {".alt_hide_smart_shapes", "legacy-mus-adjusted"},
        {".hide_chords", "legacy-mus"},
        {".hide_fretboards", "legacy-mus"},
    };
    for (const auto& expected : expectedLeaves) {
        const auto path = std::string(objectPath) + std::string(expected.suffix);
        const auto source = context.source.find(path);
        if (source == context.source.end() || !source->second.first.isBool() ||
            !source->second.first.asBool() || source->second.second != expected.sourceOrigin) {
            return false;
        }
    }
    return true;
}

inline std::optional<DifferenceClassification>
classifyNoteAttachedItemsExpressionUpgradeLoss(const DifferenceContext& context,
                                               std::string_view classPrefix)
{
    constexpr std::string_view expressionSuffix = ".alt_hide_expressions";
    if (context.category != DifferenceCategory::Differs || context.origin != "finale27-default" ||
        !comparisonPathStartsWith(context.path, classPrefix) ||
        !comparisonPathEndsWith(context.path, expressionSuffix) || !context.sourceValue.isBool() ||
        context.sourceValue.asBool() || !context.companionValue.isBool() ||
        !context.companionValue.asBool() ||
        !sourceAtOrAfter(context.epoch, context.sourceVersion, FormatEpoch::UncompressedLegacy,
                         versions::finale2000)) {
        return std::nullopt;
    }

    const auto objectPath = context.path.substr(0, context.path.size() - expressionSuffix.size());
    return sourceHasNoteAttachedItemsExpansion(context, objectPath)
               ? std::optional{DifferenceClassification::FinaleUpgradeLoss}
               : std::nullopt;
}

inline std::optional<DifferenceClassification>
classifyAggregateOtherSmartShapeUpgradeLoss(const DifferenceContext& context,
                                            std::string_view classPrefix,
                                            bool usesAggregateOtherAttachedItems)
{
    if (context.category == DifferenceCategory::Differs && context.origin == "legacy-mus" &&
        comparisonPathStartsWith(context.path, classPrefix) &&
        comparisonPathEndsWith(context.path, ".alt_hide_other_smart_shapes") &&
        context.sourceValue.isBool() && context.companionValue.isBool() &&
        context.sourceValue.asBool() != context.companionValue.asBool() &&
        usesAggregateOtherAttachedItems) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    return std::nullopt;
}

inline std::optional<DifferenceClassification> classifyNoteAttachedItemsAggregateHideUpgradeLoss(
    const DifferenceContext& context, std::string_view classPrefix, std::string_view maskContainer,
    std::optional<std::string_view> requiredMaskOrigin = {})
{
    constexpr std::string_view aggregateHideSuffixes[] = {".hide_chords", ".hide_fretboards"};
    const auto suffix = std::ranges::find_if(aggregateHideSuffixes, [&](const auto candidate) {
        return comparisonPathEndsWith(context.path, candidate);
    });
    if (suffix == std::ranges::end(aggregateHideSuffixes) ||
        context.category != DifferenceCategory::Differs || context.origin != "legacy-mus" ||
        !comparisonPathStartsWith(context.path, classPrefix) || !context.sourceValue.isBool() ||
        !context.sourceValue.asBool() || !context.companionValue.isBool() ||
        context.companionValue.asBool() ||
        !sourceAtOrAfter(context.epoch, context.sourceVersion, FormatEpoch::UncompressedLegacy,
                         versions::finale2000) ||
        !sourcePredatesVersion(context.epoch, context.sourceVersion, FormatEpoch::ZlibLegacy,
                               versions::finale2009)) {
        return std::nullopt;
    }
    const auto objectPath = context.path.substr(0, context.path.size() - suffix->size());
    if (!sourceHasNoteAttachedItemsExpansion(context, objectPath))
        return std::nullopt;
    const auto mask = context.source.find(std::string(objectPath) + std::string(maskContainer) +
                                          std::string(suffix->substr(1)));
    if (mask == context.source.end() || !mask->second.first.isBool() ||
        !mask->second.first.asBool() ||
        (requiredMaskOrigin && mask->second.second != *requiredMaskOrigin)) {
        return std::nullopt;
    }
    return DifferenceClassification::FinaleUpgradeLoss;
}

inline std::optional<DifferenceClassification>
classifyDisabledNoteFontSize(const DifferenceContext& context, std::string_view classPrefix)
{
    constexpr std::string_view noteFontSizeSuffix = ".note_font.font_size";
    if (context.category != DifferenceCategory::Differs ||
        !comparisonPathStartsWith(context.path, classPrefix) ||
        !comparisonPathEndsWith(context.path, noteFontSizeSuffix)) {
        return std::nullopt;
    }
    const auto objectPath = context.path.substr(0, context.path.size() - noteFontSizeSuffix.size());
    const auto useNoteFont = context.source.find(std::string(objectPath) + ".use_note_font");
    if (useNoteFont != context.source.end() && useNoteFont->second.first.isBool() &&
        !useNoteFont->second.first.asBool()) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

template <typename T> Value staffLikeValue(T value)
{
    if constexpr (std::is_enum_v<T>)
        return Value(static_cast<std::int64_t>(value));
    else
        return Value(value);
}

template <typename Target>
std::string staffLikeOrigin(const Target& staff, const SurveyContext& context,
                            std::string_view member)
{
    return fieldOrigin<Target>(context, member, staff);
}

template <typename Target, typename T>
void addStaffLikeLeaf(Value::Object& object, const Target& staff, const SurveyContext& context,
                      std::string_view reportMember, std::string_view originMember,
                      std::string_view leaf, T value)
{
    object.emplace(std::string(leaf), staffLikeValue(value));
    object.emplace("origin_" + std::string(originMember),
                   staffLikeOrigin(staff, context, reportMember));
}

template <typename Target, typename T>
void addStaffLikeLeaf(Value::Object& object, const Target& staff, const SurveyContext& context,
                      std::string_view member, std::string_view leaf, T value)
{
    addStaffLikeLeaf(object, staff, context, member, member, leaf, value);
}

template <typename Target>
Value::Object observeStaffLike(const Target& staff, const SurveyContext& context)
{
    Value::Object result;
    result.emplace("cmper", staff.getCmper());
    result.emplace("part_id", staff.getSourcePartId());
    result.emplace("share_mode", static_cast<std::int64_t>(staff.getShareMode()));
    const auto instance =
        finale_mus_reader::instanceKey<Target>(staff.getSourcePartId(), staff.getCmper());
    if (const auto* origin = context.report.findInstanceOrigin(instance))
        result.emplace("origin", originName(*origin));

#define STAFF_LIKE_LEAF(member, leaf)                                                              \
    addStaffLikeLeaf(result, staff, context, #member, #leaf, staff.member)
    STAFF_LIKE_LEAF(notationStyle, notation_style);
    STAFF_LIKE_LEAF(useNoteShapes, use_note_shapes);
    STAFF_LIKE_LEAF(useNoteFont, use_note_font);
    STAFF_LIKE_LEAF(defaultClef, default_clef);
    STAFF_LIKE_LEAF(transposedClef, transposed_clef);
    addStaffLikeLeaf(result, staff, context, "staffLines", "staff_lines",
                     staff.staffLines.value_or(0));
    Value::Array customStaff;
    if (staff.customStaff) {
        for (const auto line : *staff.customStaff)
            customStaff.emplace_back(line);
    }
    result.emplace("custom_staff", std::move(customStaff));
    result.emplace("origin_customStaff", staffLikeOrigin(staff, context, "customStaff"));
    STAFF_LIKE_LEAF(lineSpace, line_space);
    STAFF_LIKE_LEAF(instUuid, inst_uuid);
    STAFF_LIKE_LEAF(capoPos, capo_pos);
    STAFF_LIKE_LEAF(lowestFret, lowest_fret);
    STAFF_LIKE_LEAF(floatKeys, float_keys);
    STAFF_LIKE_LEAF(floatTime, float_time);
    STAFF_LIKE_LEAF(blineBreak, bline_break);
    STAFF_LIKE_LEAF(rbarBreak, rbar_break);
    STAFF_LIKE_LEAF(hasStyles, has_styles);
    STAFF_LIKE_LEAF(showNameInParts, show_name_in_parts);
    STAFF_LIKE_LEAF(showNoteColors, show_note_colors);
    STAFF_LIKE_LEAF(hideNameInScore, hide_name_in_score);
    STAFF_LIKE_LEAF(botBarlineOffset, bot_barline_offset);
    STAFF_LIKE_LEAF(altNotation, alt_notation);
    STAFF_LIKE_LEAF(altLayer, alt_layer);
    STAFF_LIKE_LEAF(altHideArtics, alt_hide_artics);
    STAFF_LIKE_LEAF(altHideLyrics, alt_hide_lyrics);
    STAFF_LIKE_LEAF(altHideSmartShapes, alt_hide_smart_shapes);
    STAFF_LIKE_LEAF(altRhythmStemsUp, alt_rhythm_stems_up);
    STAFF_LIKE_LEAF(altSlashDots, alt_slash_dots);
    STAFF_LIKE_LEAF(altHideOtherNotes, alt_hide_other_notes);
    STAFF_LIKE_LEAF(altHideOtherArtics, alt_hide_other_artics);
    STAFF_LIKE_LEAF(altHideExpressions, alt_hide_expressions);
    STAFF_LIKE_LEAF(altHideOtherLyrics, alt_hide_other_lyrics);
    STAFF_LIKE_LEAF(altHideOtherSmartShapes, alt_hide_other_smart_shapes);
    STAFF_LIKE_LEAF(altHideOtherExpressions, alt_hide_other_expressions);
    STAFF_LIKE_LEAF(hideRepeatBottomDot, hide_repeat_bottom_dot);
    STAFF_LIKE_LEAF(flatBeams, flat_beams);
    STAFF_LIKE_LEAF(hideFretboards, hide_fretboards);
    STAFF_LIKE_LEAF(blankMeasure, blank_measure);
    STAFF_LIKE_LEAF(hideRepeatTopDot, hide_repeat_top_dot);
    STAFF_LIKE_LEAF(hideLyrics, hide_lyrics);
    STAFF_LIKE_LEAF(noOptimize, no_optimize);
    STAFF_LIKE_LEAF(topBarlineOffset, top_barline_offset);
    STAFF_LIKE_LEAF(hideMeasNums, hide_meas_nums);
    STAFF_LIKE_LEAF(hideRepeats, hide_repeats);
    STAFF_LIKE_LEAF(hideBarlines, hide_barlines);
    STAFF_LIKE_LEAF(hideRptBars, hide_rpt_bars);
    STAFF_LIKE_LEAF(hideKeySigs, hide_key_sigs);
    STAFF_LIKE_LEAF(hideTimeSigs, hide_time_sigs);
    STAFF_LIKE_LEAF(hideClefs, hide_clefs);
    STAFF_LIKE_LEAF(hideStaffLines, hide_staff_lines);
    STAFF_LIKE_LEAF(hideChords, hide_chords);
    STAFF_LIKE_LEAF(noKey, no_key);
    STAFF_LIKE_LEAF(dwRestOffset, dw_rest_offset);
    STAFF_LIKE_LEAF(wRestOffset, w_rest_offset);
    STAFF_LIKE_LEAF(hRestOffset, h_rest_offset);
    STAFF_LIKE_LEAF(otherRestOffset, other_rest_offset);
    STAFF_LIKE_LEAF(hideRests, hide_rests);
    STAFF_LIKE_LEAF(hideTies, hide_ties);
    STAFF_LIKE_LEAF(hideDots, hide_dots);
    STAFF_LIKE_LEAF(stemReversal, stem_reversal);
    STAFF_LIKE_LEAF(fullNameTextId, full_name_text_id);
    STAFF_LIKE_LEAF(abbrvNameTextId, abbrv_name_text_id);
    STAFF_LIKE_LEAF(botRepeatDotOff, bot_repeat_dot_off);
    STAFF_LIKE_LEAF(topRepeatDotOff, top_repeat_dot_off);
    STAFF_LIKE_LEAF(vertTabNumOff, vert_tab_num_off);
    STAFF_LIKE_LEAF(showTabClefAllSys, show_tab_clef_all_sys);
    STAFF_LIKE_LEAF(useTabLetters, use_tab_letters);
    STAFF_LIKE_LEAF(breakTabLinesAtNotes, break_tab_lines_at_notes);
    STAFF_LIKE_LEAF(hideTuplets, hide_tuplets);
    STAFF_LIKE_LEAF(fretInstId, fret_inst_id);
    STAFF_LIKE_LEAF(hideStems, hide_stems);
    STAFF_LIKE_LEAF(stemDirection, stem_direction);
    STAFF_LIKE_LEAF(hideBeams, hide_beams);
    STAFF_LIKE_LEAF(stemStartFromStaff, stem_start_from_staff);
    STAFF_LIKE_LEAF(stemsFixedEnd, stems_fixed_end);
    STAFF_LIKE_LEAF(stemsFixedStart, stems_fixed_start);
    STAFF_LIKE_LEAF(horzStemOffUp, horz_stem_off_up);
    STAFF_LIKE_LEAF(horzStemOffDown, horz_stem_off_down);
    STAFF_LIKE_LEAF(vertStemStartOffUp, vert_stem_start_off_up);
    STAFF_LIKE_LEAF(vertStemStartOffDown, vert_stem_start_off_down);
    STAFF_LIKE_LEAF(vertStemEndOffUp, vert_stem_end_off_up);
    STAFF_LIKE_LEAF(vertStemEndOffDown, vert_stem_end_off_down);
    STAFF_LIKE_LEAF(hideMode, hide_mode);
    STAFF_LIKE_LEAF(redisplayLayerAccis, redisplay_layer_accis);
    STAFF_LIKE_LEAF(hideTimeSigsInParts, hide_time_sigs_in_parts);
    STAFF_LIKE_LEAF(autoNumbering, auto_numbering);
    STAFF_LIKE_LEAF(useAutoNumbering, use_auto_numbering);
    STAFF_LIKE_LEAF(hideKeySigsShowAccis, hide_key_sigs_show_accis);
#undef STAFF_LIKE_LEAF

    Value::Object noteFont;
    const auto font = staff.noteFont;
#define STAFF_LIKE_FONT_LEAF(member, leaf)                                                         \
    addStaffLikeLeaf(noteFont, staff, context, "noteFont." #member, #member, #leaf,                \
                     font ? font->member : decltype(font->member){})
    STAFF_LIKE_FONT_LEAF(fontId, font_id);
    STAFF_LIKE_FONT_LEAF(fontSize, font_size);
    STAFF_LIKE_FONT_LEAF(bold, bold);
    STAFF_LIKE_FONT_LEAF(italic, italic);
    STAFF_LIKE_FONT_LEAF(underline, underline);
    STAFF_LIKE_FONT_LEAF(strikeout, strikeout);
    STAFF_LIKE_FONT_LEAF(absolute, absolute);
    STAFF_LIKE_FONT_LEAF(hidden, hidden);
#undef STAFF_LIKE_FONT_LEAF
    result.emplace("note_font", std::move(noteFont));

    Value::Object transposition;
    transposition.emplace("present", bool(staff.transposition));
    const auto trans = staff.transposition;
    addStaffLikeLeaf(transposition, staff, context, "transposition.setToClef", "setToClef",
                     "set_to_clef", trans ? trans->setToClef : false);
    addStaffLikeLeaf(transposition, staff, context, "transposition.noSimplifyKey", "noSimplifyKey",
                     "no_simplify_key", trans ? trans->noSimplifyKey : false);
    Value::Object keysig;
    addStaffLikeLeaf(keysig, staff, context, "transposition.keysig.interval", "interval",
                     "interval", trans && trans->keysig ? trans->keysig->interval : 0);
    addStaffLikeLeaf(keysig, staff, context, "transposition.keysig.adjust", "adjust", "adjust",
                     trans && trans->keysig ? trans->keysig->adjust : 0);
    keysig.emplace("present", bool(trans && trans->keysig));
    transposition.emplace("keysig", std::move(keysig));
    Value::Object chromatic;
    addStaffLikeLeaf(chromatic, staff, context, "transposition.chromatic.alteration", "alteration",
                     "alteration", trans && trans->chromatic ? trans->chromatic->alteration : 0);
    addStaffLikeLeaf(chromatic, staff, context, "transposition.chromatic.diatonic", "diatonic",
                     "diatonic", trans && trans->chromatic ? trans->chromatic->diatonic : 0);
    chromatic.emplace("present", bool(trans && trans->chromatic));
    transposition.emplace("chromatic", std::move(chromatic));
    result.emplace("transposition", std::move(transposition));
    return result;
}

} // namespace staff_fields
} // namespace coverage
} // namespace finale_mus_reader
