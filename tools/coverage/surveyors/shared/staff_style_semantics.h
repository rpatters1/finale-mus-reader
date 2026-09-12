// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

#include "coverage/classification.h"
#include "coverage/surveyors/shared/staff_fields.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace coverage {
namespace staff_style_semantics {

using SurveyTarget = musx::dom::others::StaffStyle;

constexpr std::string_view notationStyleMaskSuffix = ".masks.notation_style";
constexpr std::string_view defaultClefMaskSuffix = ".masks.default_clef";
constexpr std::string_view showNoteColorsMaskSuffix = ".masks.show_note_colors";
constexpr std::string_view hideKeySigsShowAccisMaskSuffix = ".masks.hide_key_sigs_show_accis";
constexpr std::string_view staffTypeMaskSuffix = ".masks.staff_type";
constexpr std::string_view transpositionMaskSuffix = ".masks.transposition";
constexpr std::string_view fullNameMaskSuffix = ".masks.full_name";
constexpr std::string_view abrvNameMaskSuffix = ".masks.abrv_name";
constexpr std::string_view classifierMaskPrefix = "_classifier_masks.";
constexpr std::string_view classifierAggregateOtherAttachedItems =
    "_classifier_aggregate_other_attached_items";
constexpr std::string_view classifierAssignmentCount = "_classifier_assignment_count";

struct InstrumentMask
{
    std::string_view maskSuffix;
    std::string_view valueSuffix;
};

constexpr InstrumentMask instrumentOnlyMasks[] = {
    {notationStyleMaskSuffix, ".notation_style"},
    {defaultClefMaskSuffix, ".default_clef"},
    {showNoteColorsMaskSuffix, ".show_note_colors"},
    {hideKeySigsShowAccisMaskSuffix, ".hide_key_sigs_show_accis"}};

constexpr std::string_view instrumentRequiredSharedMasks[] = {
    staffTypeMaskSuffix, transpositionMaskSuffix, fullNameMaskSuffix, abrvNameMaskSuffix};

inline bool isInstrumentMask(std::string_view suffix)
{
    return std::ranges::any_of(instrumentOnlyMasks,
                               [&](const auto& field) { return suffix == field.maskSuffix; }) ||
           std::ranges::find(instrumentRequiredSharedMasks, suffix) !=
               std::ranges::end(instrumentRequiredSharedMasks);
}

inline bool hasActiveMask(const ComparisonLeaves& leaves, std::string_view prefix,
                          std::string_view suffix)
{
    const auto found = leaves.find(std::string(prefix) + std::string(suffix));
    return found != leaves.end() && found->second.first.isBool() && found->second.first.asBool();
}

inline bool hasActiveInstrumentMask(const ComparisonLeaves& leaves, std::string_view prefix,
                                    std::string_view maskSuffix,
                                    std::string_view maskContainer = "masks.")
{
    constexpr std::string_view maskPrefix = ".masks.";
    return maskSuffix.starts_with(maskPrefix) &&
           hasActiveMask(leaves, prefix,
                         "." + std::string(maskContainer) +
                             std::string(maskSuffix.substr(maskPrefix.size())));
}

inline bool hasAnyInstrumentOnlyMask(const ComparisonLeaves& leaves, std::string_view prefix,
                                     std::string_view maskContainer = "masks.")
{
    return std::ranges::any_of(instrumentOnlyMasks, [&](const auto& field) {
        return hasActiveInstrumentMask(leaves, prefix, field.maskSuffix, maskContainer);
    });
}

inline bool hasAllRequiredInstrumentMasks(const ComparisonLeaves& leaves, std::string_view prefix,
                                          std::string_view maskContainer = "masks.")
{
    return std::ranges::all_of(instrumentRequiredSharedMasks, [&](const auto suffix) {
        return hasActiveInstrumentMask(leaves, prefix, suffix, maskContainer);
    });
}

struct ValueMask
{
    std::string_view valueSuffix;
    std::string_view maskSuffix;
};

constexpr ValueMask valueMasks[] = {
    {".use_note_shapes", ".masks.use_note_shapes"},
    {".flat_beams", ".masks.flat_beams"},
    {".blank_measure", ".masks.blank_measure_rest"},
    {".no_optimize", ".masks.no_optimize"},
    {".default_clef", defaultClefMaskSuffix},
    {".bline_break", ".masks.bline_break"},
    {".rbar_break", ".masks.rbar_break"},
    {".hide_meas_nums", ".masks.neg_mnumb"},
    {".hide_repeats", ".masks.neg_repeat"},
    {".hide_name_in_score", ".masks.neg_name_score"},
    {".hide_barlines", ".masks.hide_barlines"},
    {".full_name_text_id", fullNameMaskSuffix},
    {".abbrv_name_text_id", abrvNameMaskSuffix},
    {".float_keys", ".masks.float_keys"},
    {".float_time", ".masks.float_time"},
    {".hide_rpt_bars", ".masks.hide_rpt_bars"},
    {".hide_key_sigs", ".masks.neg_key"},
    {".hide_time_sigs", ".masks.neg_time"},
    {".hide_clefs", ".masks.neg_clef"},
    {".hide_mode", ".masks.hide_staff"},
    {".no_key", ".masks.no_key"},
    {".hide_ties", ".masks.show_ties"},
    {".hide_dots", ".masks.show_dots"},
    {".hide_rests", ".masks.show_rests"},
    {".hide_chords", ".masks.hide_chords"},
    {".hide_fretboards", ".masks.hide_fretboards"},
    {".hide_lyrics", ".masks.hide_lyrics"},
    {".show_name_in_parts", ".masks.show_name_parts"},
    {".show_note_colors", showNoteColorsMaskSuffix},
    {".hide_staff_lines", ".masks.hide_staff_lines"},
    {".redisplay_layer_accis", ".masks.redisplay_layer_accis"},
    {".hide_time_sigs_in_parts", ".masks.neg_time_parts"},
    {".hide_key_sigs_show_accis", hideKeySigsShowAccisMaskSuffix},
};

constexpr std::string_view tablatureOnlyValueSuffixes[] = {
    ".capo_pos",
    ".lowest_fret",
    ".vert_tab_num_off",
    ".show_tab_clef_all_sys",
    ".use_tab_letters",
    ".break_tab_lines_at_notes",
    ".hide_tuplets",
    ".fret_inst_id"};

inline bool isTablatureOnlyValue(std::string_view relativePath)
{
    return std::ranges::find(tablatureOnlyValueSuffixes, relativePath) !=
           std::ranges::end(tablatureOnlyValueSuffixes);
}

inline void removeInactiveTablatureValues(Value::Object& style)
{
    const auto notation = style.find("notation_style");
    if (notation != style.end() && notation->second.isInteger() &&
        notation->second.asInteger() ==
            static_cast<std::int64_t>(SurveyTarget::NotationStyle::Tablature)) {
        return;
    }
    for (const auto suffix : tablatureOnlyValueSuffixes)
        style.erase(std::string(suffix.substr(1)));
}

inline std::optional<std::string_view> maskForValue(std::string_view relativePath)
{
    if (relativePath == ".use_note_font" || comparisonPathStartsWith(relativePath, ".note_font.")) {
        return ".masks.float_notehead_font";
    }
    if (relativePath == ".notation_style" || isTablatureOnlyValue(relativePath)) {
        return notationStyleMaskSuffix;
    }
    if (relativePath == ".staff_lines" || relativePath == ".custom_staff" ||
        comparisonPathStartsWith(relativePath, ".custom_staff[") || relativePath == ".line_space" ||
        relativePath == ".top_barline_offset" || relativePath == ".bot_barline_offset" ||
        relativePath == ".dw_rest_offset" || relativePath == ".w_rest_offset" ||
        relativePath == ".h_rest_offset" || relativePath == ".other_rest_offset" ||
        relativePath == ".bot_repeat_dot_off" || relativePath == ".top_repeat_dot_off" ||
        relativePath == ".stem_reversal" || relativePath == ".hide_repeat_bottom_dot" ||
        relativePath == ".hide_repeat_top_dot") {
        return staffTypeMaskSuffix;
    }
    if (relativePath == ".transposed_clef" ||
        comparisonPathStartsWith(relativePath, ".transposition.")) {
        return transpositionMaskSuffix;
    }
    if (relativePath == ".alt_notation" || relativePath == ".alt_layer" ||
        comparisonPathStartsWith(relativePath, ".alt_hide_") ||
        relativePath == ".alt_rhythm_stems_up" || relativePath == ".alt_slash_dots") {
        return ".masks.alt_notation";
    }
    if (relativePath == ".hide_stems" || relativePath == ".stem_direction" ||
        relativePath == ".hide_beams" || relativePath == ".stem_start_from_staff" ||
        relativePath == ".stems_fixed_end" || relativePath == ".stems_fixed_start" ||
        relativePath == ".horz_stem_off_up" || relativePath == ".horz_stem_off_down" ||
        relativePath == ".vert_stem_start_off_up" || relativePath == ".vert_stem_start_off_down" ||
        relativePath == ".vert_stem_end_off_up" || relativePath == ".vert_stem_end_off_down") {
        return ".masks.show_stems";
    }
    for (const auto& mapping : valueMasks) {
        if (relativePath == mapping.valueSuffix)
            return mapping.maskSuffix;
    }
    return std::nullopt;
}

inline bool usesAggregateOtherAttachedItems(const finale_mus_reader::ImportReport& report,
                                            musx::dom::Cmper cmper)
{
    const auto instance =
        finale_mus_reader::instanceKey<SurveyTarget>(musx::dom::SCORE_PARTID, cmper);
    const auto fields = report.fields.find(instance);
    if (fields == report.fields.end())
        return false;
    const auto articulations = fields->second.find("altHideOtherArtics");
    const auto smartShapes = fields->second.find("altHideOtherSmartShapes");
    return articulations != fields->second.end() && smartShapes != fields->second.end() &&
           articulations->second.blockOffset == smartShapes->second.blockOffset &&
           articulations->second.decodedOffset == smartShapes->second.decodedOffset &&
           articulations->second.sourceIdentity == smartShapes->second.sourceIdentity;
}

inline Value::Object observe(const SurveyTarget& style, const SurveyContext& context)
{
    auto object = staff_fields::observeStaffLike(style, context);
    staff_fields::addStaffLikeLeaf(object, style, context, "styleName", "style_name",
                                   style.styleName);
    staff_fields::addStaffLikeLeaf(object, style, context, "copyable", "copyable", style.copyable);
    staff_fields::addStaffLikeLeaf(object, style, context, "addToMenu", "add_to_menu",
                                   style.addToMenu);

    Value::Object masks;
#define STAFF_STYLE_SEMANTIC_MASK_LEAF(member, leaf)                                               \
    staff_fields::addStaffLikeLeaf(masks, style, context, "masks." #member, #member, #leaf,        \
                                   style.masks ? style.masks->member : false)
    STAFF_STYLE_SEMANTIC_MASK_LEAF(floatNoteheadFont, float_notehead_font);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(useNoteShapes, use_note_shapes);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(flatBeams, flat_beams);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(blankMeasureRest, blank_measure_rest);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(noOptimize, no_optimize);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(notationStyle, notation_style);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(defaultClef, default_clef);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(staffType, staff_type);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(transposition, transposition);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(blineBreak, bline_break);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(rbarBreak, rbar_break);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(negMnumb, neg_mnumb);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(negRepeat, neg_repeat);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(negNameScore, neg_name_score);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(hideBarlines, hide_barlines);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(fullName, full_name);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(abrvName, abrv_name);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(floatKeys, float_keys);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(floatTime, float_time);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(hideRptBars, hide_rpt_bars);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(negKey, neg_key);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(negTime, neg_time);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(negClef, neg_clef);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(hideStaff, hide_staff);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(noKey, no_key);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(fullNamePos, full_name_pos);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(abrvNamePos, abrv_name_pos);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(altNotation, alt_notation);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(showTies, show_ties);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(showDots, show_dots);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(showRests, show_rests);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(showStems, show_stems);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(hideChords, hide_chords);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(hideFretboards, hide_fretboards);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(hideLyrics, hide_lyrics);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(showNameParts, show_name_parts);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(showNoteColors, show_note_colors);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(hideStaffLines, hide_staff_lines);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(redisplayLayerAccis, redisplay_layer_accis);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(negTimeParts, neg_time_parts);
    STAFF_STYLE_SEMANTIC_MASK_LEAF(hideKeySigsShowAccis, hide_key_sigs_show_accis);
#undef STAFF_STYLE_SEMANTIC_MASK_LEAF
    object.emplace("masks", std::move(masks));
    return object;
}

inline bool activeMask(const Value::Object& style, std::string_view maskSuffix)
{
    constexpr std::string_view prefix = ".masks.";
    if (!maskSuffix.starts_with(prefix))
        return false;
    const auto* masks = [&]() -> const Value* {
        const auto found = style.find("masks");
        return found == style.end() ? nullptr : &found->second;
    }();
    if (!masks || !masks->isObject())
        return false;
    const auto* mask = masks->find(maskSuffix.substr(prefix.size()));
    return mask && mask->isBool() && mask->asBool();
}

inline void addCanonicalPatchLeaf(Value::Object& result, std::string path, const Value& value,
                                  const Value::Object& parent, std::string_view leaf)
{
    const auto key = path.substr(1);
    result.insert_or_assign(key, value);
    if (const auto origin = parent.find(originKeyForLeaf(leaf));
        origin != parent.end() && origin->second.isString()) {
        result.insert_or_assign(key + "_origin", origin->second);
    }
}

inline void collectActiveValues(const Value& value, std::string path, const Value::Object& style,
                                Value::Object& result, const Value::Object* parent = nullptr,
                                std::string_view leaf = {})
{
    if (value.isObject()) {
        for (const auto& [key, child] : value.asObject()) {
            if (key == "masks" || key == "style_name" || key == "copyable" ||
                key == "add_to_menu" || key == "cmper" || key == "part_id" || key == "share_mode" ||
                key == "origin" || key.starts_with("origin_")) {
                continue;
            }
            collectActiveValues(child, path + "." + key, style, result, &value.asObject(), key);
        }
        return;
    }
    const auto mask = maskForValue(path);
    if (mask && activeMask(style, *mask) && parent)
        addCanonicalPatchLeaf(result, std::move(path), value, *parent, leaf);
}

inline Value::Object canonicalPatch(const Value::Object& style,
                                    bool aggregateOtherAttachedItems = false)
{
    Value::Object result;
    const auto masks = style.find("masks");
    if (masks != style.end() && masks->second.isObject()) {
        for (const auto& [name, value] : masks->second.asObject()) {
            if (!name.starts_with("origin_") && value.isBool() && value.asBool())
                result.emplace(std::string(classifierMaskPrefix) + name, true);
        }
    }
    if (aggregateOtherAttachedItems)
        result.emplace(std::string(classifierAggregateOtherAttachedItems), true);
    collectActiveValues(Value(style), {}, style, result);
    return result;
}

} // namespace staff_style_semantics
} // namespace coverage
} // namespace finale_mus_reader
