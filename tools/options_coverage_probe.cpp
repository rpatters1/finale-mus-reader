// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Emit one private observation per legacy document for the option classes that recovery
// currently covers: options::ClefOptions, options::FontOptions,
// options::LyricOptions, options::MultimeasureRestOptions, options::TextOptions, and the
// others::FontDefinition
// pool those reference.
//
// Input is a TSV of `corpus_id<TAB>source_path` rows. Output is JSON Lines and
// deliberately contains no source path, so it may be aggregated into tracked findings.
// Import failures are printed to stderr WITH their path, which is the survey policy's
// intentional console-only exception.
//
// This probe reports what the reader produced and where each value came from. It does
// not compare against a companion; that is the aggregator's job, so that the source and
// the companion are extracted independently.

#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "finale_mus_reader/reader.h"
#include "musx/musx.h"
#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#endif
#include "musx/xml/PugiXmlImpl.h"

namespace {

using finale_mus_reader::ValueOrigin;

std::string jsonString(std::string_view value)
{
    std::string result = "\"";
    for (const char ch : value) {
        switch (ch) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20) {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x",
                    static_cast<unsigned>(static_cast<unsigned char>(ch)));
                result += buffer;
            } else {
                result += ch;
            }
        }
    }
    return result + '"';
}

const char* originName(ValueOrigin origin)
{
    switch (origin) {
    case ValueOrigin::LegacyMus: return "legacy-mus";
    case ValueOrigin::LegacyBehavior: return "legacy-behavior";
    case ValueOrigin::Finale27Default: return "finale27-default";
    }
    return "unknown";
}

const char* epochName(finale_mus_reader::FormatEpoch epoch)
{
    switch (epoch) {
    case finale_mus_reader::FormatEpoch::CodaBanner: return "coda-banner";
    case finale_mus_reader::FormatEpoch::UncompressedLegacy: return "uncompressed";
    case finale_mus_reader::FormatEpoch::DclLegacy: return "dcl";
    case finale_mus_reader::FormatEpoch::ZlibLegacy: return "zlib";
    case finale_mus_reader::FormatEpoch::Unknown: return "unknown";
    }
    return "unknown";
}

// The reader returns failure rather than throwing: a null document means the import failed,
// and the reason is the Error-level diagnostic in the report.
std::string importError(const finale_mus_reader::ImportReport& report)
{
    for (const auto& entry : report.diagnostics) {
        if (entry.level == musx::util::Logger::LogLevel::Error) {
            return entry.message;
        }
    }
    return "import failed without a reported reason";
}

// Every reported field keyed by target, so a value's origin can be looked up by the
// same name the importer used to report it.
std::map<std::string, finale_mus_reader::FieldInfo> fieldsByTarget(
    const finale_mus_reader::ImportReport& report)
{
    std::map<std::string, finale_mus_reader::FieldInfo> result;
    for (const auto& info : report.fields) {
        result.emplace(info.target, info);
    }
    return result;
}

std::string originOf(const std::map<std::string, finale_mus_reader::FieldInfo>& fields,
    const std::string& target)
{
    const auto found = fields.find(target);
    return found == fields.end() ? "absent" : originName(found->second.origin);
}

std::string versionName(const finale_mus_reader::ImportReport& report)
{
    if (!report.sourceVersion) {
        return {};
    }
    const auto& version = *report.sourceVersion;
    return std::to_string(version.major) + '.' + std::to_string(version.minor) + '.'
        + std::to_string(version.maint) + '.' + std::to_string(version.build);
}

void writeClefOptions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    const auto options = document->getOptions()->get<musx::dom::options::ClefOptions>();
    if (!options) {
        out << ",\"clef_options\":null";
        return;
    }
    out << ",\"clef_options\":{"
        << "\"default_clef\":" << options->defaultClef
        << ",\"clef_change_percent\":" << options->clefChangePercent
        << ",\"clef_change_offset\":" << options->clefChangeOffset
        << ",\"clef_front_separ\":" << options->clefFrontSepar
        << ",\"clef_back_separ\":" << options->clefBackSepar
        << ",\"clef_key_separ\":" << options->clefKeySepar
        << ",\"clef_time_separ\":" << options->clefTimeSepar
        << ",\"show_clef_first_system_only\":"
        << (options->showClefFirstSystemOnly ? "true" : "false")
        << ",\"cautionary_clef_changes\":"
        << (options->cautionaryClefChanges ? "true" : "false");

    for (const auto* member : {"defaultClef", "clefChangePercent", "clefChangeOffset",
             "clefFrontSepar", "clefBackSepar", "clefKeySepar", "clefTimeSepar",
             "showClefFirstSystemOnly", "cautionaryClefChanges"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(originOf(fields, std::string("options.clefOptions.") + member));
    }

    out << ",\"clef_defs\":[";
    for (std::size_t index = 0; index < options->clefDefs.size(); ++index) {
        const auto& def = options->clefDefs[index];
        const auto prefix = "options.clefOptions.clefDefs[" + std::to_string(index) + "].";
        std::string fontName;
        if (def->useOwnFont && def->font) {
            if (const auto font = document->getOthers()
                    ->get<musx::dom::others::FontDefinition>(
                        musx::dom::SCORE_PARTID, def->font->fontId)) {
                fontName = font->name;
            }
        }
        // A shape comparator that names no shape leaves the definition unrenderable.
        bool danglingShape = false;
        if (def->isShape && def->shapeId != 0) {
            danglingShape = !document->getOthers()->get<musx::dom::others::ShapeDef>(
                musx::dom::SCORE_PARTID, def->shapeId);
        }
        out << (index ? "," : "") << '{'
            << "\"index\":" << index
            << ",\"middle_c_pos\":" << def->middleCPos
            << ",\"clef_char\":" << static_cast<std::uint32_t>(def->clefChar)
            << ",\"staff_position\":" << def->staffPosition
            << ",\"baseline_adjust\":" << def->baselineAdjust
            << ",\"shape_id\":" << def->shapeId
            << ",\"is_shape\":" << (def->isShape ? "true" : "false")
            << ",\"scale_to_staff_height\":"
            << (def->scaleToStaffHeight ? "true" : "false")
            << ",\"use_own_font\":" << (def->useOwnFont ? "true" : "false")
            << ",\"font_id\":" << (def->font ? def->font->fontId : 0)
            << ",\"font_size\":" << (def->font ? def->font->fontSize : 0)
            << ",\"font_name\":" << jsonString(fontName)
            << ",\"dangling_shape\":" << (danglingShape ? "true" : "false")
            << ",\"origin_middleCPos\":" << jsonString(originOf(fields, prefix + "middleCPos"))
            << ",\"origin_clefChar\":" << jsonString(originOf(fields, prefix + "clefChar"))
            << ",\"origin_staffPosition\":"
            << jsonString(originOf(fields, prefix + "staffPosition"))
            << ",\"origin_baselineAdjust\":"
            << jsonString(originOf(fields, prefix + "baselineAdjust"))
            << ",\"origin_shapeId\":" << jsonString(originOf(fields, prefix + "shapeId"))
            << '}';
    }
    out << "]}";
}

void writeFontOptions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    const auto options = document->getOptions()->get<musx::dom::options::FontOptions>();
    if (!options) {
        out << ",\"font_options\":null";
        return;
    }
    out << ",\"font_options\":[";
    bool first = true;
    for (const auto& [type, font] : options->fontOptions) {
        const auto ordinal = static_cast<std::size_t>(type);
        const auto prefix = "options.fontOptions[" + std::to_string(ordinal) + "].";
        std::string fontName;
        bool dangling = false;
        if (const auto definition = document->getOthers()
                ->get<musx::dom::others::FontDefinition>(
                    musx::dom::SCORE_PARTID, font->fontId)) {
            fontName = definition->name;
        } else {
            dangling = font->fontId != 0;
        }
        // Both spellings are emitted. A comparison must normalize with musxdom's own
        // normalizer -- "EngraverTextT" and "Engraver Text T" are the same face, and
        // comparing the raw strings reports 324 false disagreements.
        out << (first ? "" : ",") << '{'
            << "\"ordinal\":" << ordinal
            << ",\"font_id\":" << font->fontId
            << ",\"font_size\":" << font->fontSize
            << ",\"font_name\":" << jsonString(fontName)
            << ",\"normalized_font_name\":"
            << jsonString(musx::dom::normalizeFontName(fontName))
            << ",\"dangling\":" << (dangling ? "true" : "false")
            << ",\"origin\":" << jsonString(originOf(fields, prefix + "fontId"))
            << '}';
        first = false;
    }
    out << ']';
}

void writeMultimeasureRestOptions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    const auto options =
        document->getOptions()->get<musx::dom::options::MultimeasureRestOptions>();
    if (!options) {
        out << ",\"mmrest_options\":null";
        return;
    }
    // A shape comparator that names no shape leaves the H-bar undrawable, and comparator zero
    // means no shape rather than a missing one.
    const bool danglingShape = options->shapeDef != 0
        && !document->getOthers()->get<musx::dom::others::ShapeDef>(
            musx::dom::SCORE_PARTID, options->shapeDef);
    out << ",\"mmrest_options\":{"
        << "\"meas_width\":" << options->measWidth
        << ",\"num_adj_y\":" << options->numAdjY
        << ",\"shape_def\":" << options->shapeDef
        << ",\"num_start\":" << options->numStart
        << ",\"use_syms_threshold\":" << options->useSymsThreshold
        << ",\"sym_spacing\":" << options->symSpacing
        << ",\"num_adj_x\":" << options->numAdjX
        << ",\"start_adjust\":" << options->startAdjust
        << ",\"end_adjust\":" << options->endAdjust
        << ",\"use_symbols\":" << (options->useSymbols ? "true" : "false")
        << ",\"no_horizontal_stretch\":"
        << (options->noHorizontalStretch ? "true" : "false")
        << ",\"auto_update_mm_rests\":"
        << (options->autoUpdateMmRests ? "true" : "false")
        << ",\"dangling_shape\":" << (danglingShape ? "true" : "false");
    for (const auto* member : {"measWidth", "numAdjY", "shapeDef", "numStart",
             "useSymsThreshold", "symSpacing", "numAdjX", "startAdjust", "endAdjust",
             "useSymbols", "noHorizontalStretch", "autoUpdateMmRests"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(
                   originOf(fields, std::string("options.multimeasureRestOptions.") + member));
    }
    out << '}';
}

// LyricOptions. Its two collections are fixed-length maps in musxdom rather than sequences, so
// each element is emitted under its own type name and a comparison never depends on iteration
// order. `alt_hyphen_font_name` is resolved rather than reported as a comparator, because the
// legacy pool and the companion renumber font definitions independently.
void writeLyricOptions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    using Lyrics = musx::dom::options::LyricOptions;
    const auto options = document->getOptions()->get<Lyrics>();
    if (!options) {
        out << ",\"lyric_options\":null";
        return;
    }
    const auto boolOf = [](bool value) { return value ? "true" : "false"; };
    out << ",\"lyric_options\":{"
        << "\"hyphen_char\":" << static_cast<std::uint32_t>(options->hyphenChar)
        << ",\"max_hyphen_separation\":" << options->maxHyphenSeparation
        << ",\"word_ext_vert_offset\":" << options->wordExtVertOffset
        << ",\"word_ext_horz_offset\":" << options->wordExtHorzOffset
        << ",\"word_ext_line_width\":" << options->wordExtLineWidth
        << ",\"word_ext_min_length\":" << options->wordExtMinLength
        << ",\"smart_hyphen_start\":" << static_cast<int>(options->smartHyphenStart)
        << ",\"lyric_auto_num_type\":" << static_cast<int>(options->lyricAutoNumType)
        << ",\"use_smart_word_extensions\":" << boolOf(options->useSmartWordExtensions)
        << ",\"use_smart_hyphens\":" << boolOf(options->useSmartHyphens)
        << ",\"use_alt_hyphen_font\":" << boolOf(options->useAltHyphenFont)
        << ",\"word_ext_need_underscore\":" << boolOf(options->wordExtNeedUnderscore)
        << ",\"word_ext_offset_to_notehead\":" << boolOf(options->wordExtOffsetToNotehead)
        << ",\"lyric_use_edge_punctuation\":" << boolOf(options->lyricUseEdgePunctuation)
        << ",\"show_auto_numbers_verses\":" << boolOf(options->showAutoNumbersOnVerses)
        << ",\"show_auto_numbers_choruses\":" << boolOf(options->showAutoNumbersOnChoruses)
        << ",\"show_auto_numbers_sections\":" << boolOf(options->showAutoNumbersOnSections)
        << ",\"punctuation_to_ignore\":" << jsonString(options->lyricPunctuationToIgnore);

    std::string altHyphenName;
    if (options->altHyphenFont) {
        try {
            altHyphenName = options->altHyphenFont->getName();
        } catch (const std::exception&) {
            // An unregistered comparator throws here rather than resolving. That is exactly
            // the failure this field is worth watching, so it is recorded rather than swallowed.
            altHyphenName = "<unresolved>";
        }
    }
    out << ",\"alt_hyphen_font_name\":" << jsonString(altHyphenName);

    out << ",\"syllable_pos_styles\":{";
    bool firstStyle = true;
    for (const auto& [name, type] : {
             std::pair{"default", Lyrics::SyllablePosStyleType::Default},
             std::pair{"wordExt", Lyrics::SyllablePosStyleType::WordExt},
             std::pair{"first", Lyrics::SyllablePosStyleType::First},
             std::pair{"systemStart", Lyrics::SyllablePosStyleType::SystemStart}}) {
        const auto found = options->syllablePosStyles.find(type);
        if (!firstStyle) {
            out << ',';
        }
        firstStyle = false;
        out << jsonString(name) << ':';
        if (found == options->syllablePosStyles.end() || !found->second) {
            out << "null";
            continue;
        }
        out << "{\"align\":" << static_cast<int>(found->second->align)
            << ",\"justify\":" << static_cast<int>(found->second->justify)
            << ",\"on\":" << boolOf(found->second->on) << '}';
    }
    out << '}';

    out << ",\"word_ext_connect_styles\":{";
    bool firstConnect = true;
    for (const auto& [name, type] : {
             std::pair{"defaultStart", Lyrics::WordExtConnectStyleType::DefaultStart},
             std::pair{"defaultEnd", Lyrics::WordExtConnectStyleType::DefaultEnd},
             std::pair{"systemStart", Lyrics::WordExtConnectStyleType::SystemStart},
             std::pair{"systemEnd", Lyrics::WordExtConnectStyleType::SystemEnd},
             std::pair{"dottedEnd", Lyrics::WordExtConnectStyleType::DottedEnd},
             std::pair{"durationEnd", Lyrics::WordExtConnectStyleType::DurationEnd},
             std::pair{"oneEntryEnd", Lyrics::WordExtConnectStyleType::OneEntryEnd},
             std::pair{"zeroLengthEnd", Lyrics::WordExtConnectStyleType::ZeroLengthEnd},
             std::pair{"zeroOffset", Lyrics::WordExtConnectStyleType::ZeroOffset}}) {
        const auto found = options->wordExtConnectStyles.find(type);
        if (!firstConnect) {
            out << ',';
        }
        firstConnect = false;
        out << jsonString(name) << ':';
        if (found == options->wordExtConnectStyles.end() || !found->second) {
            out << "null";
            continue;
        }
        out << "{\"connect_index\":" << static_cast<int>(found->second->connectIndex)
            << ",\"x\":" << found->second->xOffset
            << ",\"y\":" << found->second->yOffset << '}';
    }
    out << '}';

    for (const auto* member : {"maxHyphenSeparation", "wordExtVertOffset", "wordExtHorzOffset",
             "wordExtLineWidth", "wordExtMinLength", "smartHyphenStart", "lyricAutoNumType",
             "useSmartWordExtensions", "useSmartHyphens", "useAltHyphenFont",
             "wordExtNeedUnderscore", "wordExtOffsetToNotehead", "lyricUseEdgePunctuation",
             "showAutoNumbersOnVerses", "showAutoNumbersOnChoruses", "showAutoNumbersOnSections",
             "hyphenChar", "lyricPunctuationToIgnore"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(originOf(fields, std::string("options.lyricOptions.") + member));
    }
    out << '}';
}

// TextOptions. The two line-spacing members are optional and mutually exclusive, so both are
// emitted and a null means the document did not state that spelling. Each symbol insert emits
// its font by comparator and by resolved name: comparators are renumbered between the legacy
// pool and the companion, so only the name is comparable across the two.
void writeTextOptions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    const auto options = document->getOptions()->get<musx::dom::options::TextOptions>();
    if (!options) {
        out << ",\"text_options\":null";
        return;
    }
    out << ",\"text_options\":{";
    if (options->textLineSpacingPercent) {
        out << "\"line_spacing_percent\":" << *options->textLineSpacingPercent;
    } else {
        out << "\"line_spacing_percent\":null";
    }
    if (options->textLineSpacingEvpu) {
        out << ",\"line_spacing_evpu\":" << *options->textLineSpacingEvpu;
    } else {
        out << ",\"line_spacing_evpu\":null";
    }
    out << ",\"show_time_seconds\":" << (options->showTimeSeconds ? "true" : "false")
        << ",\"date_format\":" << static_cast<int>(options->dateFormat)
        << ",\"tab_spaces\":" << options->tabSpaces
        << ",\"text_tracking\":" << options->textTracking
        << ",\"text_baseline_shift\":" << options->textBaselineShift
        << ",\"text_superscript\":" << options->textSuperscript
        << ",\"text_word_wrap\":" << (options->textWordWrap ? "true" : "false")
        << ",\"text_page_offset\":" << options->textPageOffset
        << ",\"text_justify\":" << static_cast<int>(options->textJustify)
        << ",\"text_expand_single_word\":"
        << (options->textExpandSingleWord ? "true" : "false")
        << ",\"text_horz_align\":" << static_cast<int>(options->textHorzAlign)
        << ",\"text_vert_align\":" << static_cast<int>(options->textVertAlign)
        << ",\"text_is_edge_aligned\":" << (options->textIsEdgeAligned ? "true" : "false");

    for (const auto* member : {"textLineSpacingPercent", "textLineSpacingEvpu",
             "showTimeSeconds", "dateFormat", "tabSpaces", "textTracking",
             "textBaselineShift", "textSuperscript", "textWordWrap", "textPageOffset",
             "textJustify", "textExpandSingleWord", "textHorzAlign", "textVertAlign",
             "textIsEdgeAligned"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(originOf(fields, std::string("options.textOptions.") + member));
    }

    using Insert = musx::dom::options::AccidentalInsertSymbolType;
    static const std::pair<Insert, const char*> insertOrder[] = {
        {Insert::Sharp, "sharp"}, {Insert::Flat, "flat"}, {Insert::Natural, "natural"},
        {Insert::DblSharp, "dblSharp"}, {Insert::DblFlat, "dblFlat"}};
    out << ",\"inserts\":[";
    bool first = true;
    for (const auto& [type, name] : insertOrder) {
        const auto found = options->symbolInserts.find(type);
        out << (first ? "" : ",") << "{\"type\":" << jsonString(name);
        first = false;
        if (found == options->symbolInserts.end() || !found->second) {
            out << ",\"present\":false}";
            continue;
        }
        const auto& insert = *found->second;
        std::string fontName;
        bool dangling = false;
        if (insert.symFont) {
            if (const auto definition = document->getOthers()
                    ->get<musx::dom::others::FontDefinition>(
                        musx::dom::SCORE_PARTID, insert.symFont->fontId)) {
                fontName = definition->name;
            } else {
                dangling = insert.symFont->fontId != 0;
            }
        }
        out << ",\"present\":true"
            << ",\"tracking_before\":" << insert.trackingBefore
            << ",\"tracking_after\":" << insert.trackingAfter
            << ",\"baseline_shift_perc\":" << insert.baselineShiftPerc
            << ",\"sym_char\":" << static_cast<std::uint32_t>(insert.symChar)
            << ",\"has_font\":" << (insert.symFont ? "true" : "false")
            << ",\"font_id\":" << (insert.symFont ? int(insert.symFont->fontId) : 0)
            << ",\"font_size\":" << (insert.symFont ? insert.symFont->fontSize : 0)
            << ",\"font_effects\":"
            << (insert.symFont ? int(insert.symFont->getEnigmaStyles()) : 0)
            << ",\"font_name\":" << jsonString(fontName)
            << ",\"normalized_font_name\":"
            << jsonString(musx::dom::normalizeFontName(fontName))
            << ",\"dangling_font\":" << (dangling ? "true" : "false");
        const auto prefix
            = std::string("options.textOptions.symbolInserts[") + name + "].";
        for (const auto* member : {"trackingBefore", "trackingAfter", "baselineShiftPerc",
                 "symChar", "symFont.fontId", "symFont.fontSize", "symFont.effects"}) {
            out << ",\"origin_" << member << "\":"
                << jsonString(originOf(fields, prefix + member));
        }
        out << '}';
    }
    out << "]}";
}

const char* jsonBool(bool value) { return value ? "true" : "false"; }

void writeStemOptions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    const auto options = document->getOptions()->get<musx::dom::options::StemOptions>();
    if (!options) {
        out << ",\"stem_options\":null";
        return;
    }
    out << ",\"stem_options\":{"
        << "\"half_stem_length\":" << options->halfStemLength
        << ",\"stem_length\":" << options->stemLength
        << ",\"short_stem_length\":" << options->shortStemLength
        << ",\"rev_stem_adj\":" << options->revStemAdj
        << ",\"stem_width\":" << options->stemWidth
        << ",\"stem_offset\":" << options->stemOffset
        << ",\"use_stem_connections\":" << jsonBool(options->useStemConnections)
        << ",\"no_reverse_stems\":" << jsonBool(options->noReverseStems);
    for (const auto* member : {"halfStemLength", "stemLength", "shortStemLength", "revStemAdj",
             "stemWidth", "stemOffset", "useStemConnections", "noReverseStems"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(originOf(fields, std::string("options.stemOptions.") + member));
    }
    out << ",\"stem_connections\":[";
    for (std::size_t index = 0; index < options->stemConnections.size(); ++index) {
        const auto& connection = options->stemConnections[index];
        const auto prefix
            = "options.stemOptions.stemConnections[" + std::to_string(index) + "].";
        if (index) {
            out << ',';
        }
        // The face as well as the comparator. Finale 27 matches fonts against those installed
        // on the upgrading machine and renumbers its own table accordingly, so comparing
        // comparators against a companion manufactures disagreements; the face is what both
        // sides agree about.
        std::string connectionFace;
        if (const auto definition = document->getOthers()
                ->get<musx::dom::others::FontDefinition>(
                    musx::dom::SCORE_PARTID, connection->fontId)) {
            connectionFace = definition->name;
        }
        out << "{\"index\":" << index
            << ",\"font_name\":" << jsonString(connectionFace)
            << ",\"font_id\":" << connection->fontId
            << ",\"symbol\":" << static_cast<std::uint32_t>(connection->symbol)
            << ",\"up_stem_vert\":" << connection->upStemVert
            << ",\"down_stem_vert\":" << connection->downStemVert
            << ",\"up_stem_horz\":" << connection->upStemHorz
            << ",\"down_stem_horz\":" << connection->downStemHorz;
        for (const auto* member : {"fontId", "symbol", "upStemVert", "downStemVert",
                 "upStemHorz", "downStemHorz"}) {
            out << ",\"origin_" << member << "\":"
                << jsonString(originOf(fields, prefix + member));
        }
        out << '}';
    }
    out << "]}";
}

void writeMusicSpacingOptions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    const auto options = document->getOptions()->get<musx::dom::options::MusicSpacingOptions>();
    if (!options) {
        out << ",\"spacing_options\":null";
        return;
    }
    out << ",\"spacing_options\":{"
        << "\"min_width\":" << options->minWidth
        << ",\"max_width\":" << options->maxWidth
        << ",\"min_distance\":" << options->minDistance
        << ",\"min_dist_tied_notes\":" << options->minDistTiedNotes;
    for (const auto* member : {"minWidth", "maxWidth", "minDistance", "minDistTiedNotes"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(originOf(fields, std::string("options.musicSpacing.") + member));
    }
    out << '}';
}

void writeLayerAttributes(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    out << ",\"layer_atts\":[";
    bool first = true;
    for (const auto& layer :
        document->getOthers()->getArray<musx::dom::others::LayerAttributes>(
            musx::dom::SCORE_PARTID)) {
        if (!first) {
            out << ',';
        }
        first = false;
        out << "{\"cmper\":" << layer->getCmper()
            << ",\"rest_offset\":" << layer->restOffset
            << ",\"origin_restOffset\":"
            << jsonString(originOf(fields,
                   "others.layerAtts[" + std::to_string(layer->getCmper()) + "].restOffset"))
            << '}';
    }
    out << ']';
}

// The graphic assignments, whose recovered values are the tuple the source stores rather than a
// named field list. Both page and shape assignments carry the same sixteen words plus the
// graphic they name, so one writer serves both.
template <typename Target>
void writeGraphicAssignments(std::ostream& out, const musx::dom::DocumentPtr& document,
    const char* key)
{
    out << ",\"" << key << "\":[";
    bool first = true;
    for (const auto& assign :
        document->getOthers()->getArray<Target>(musx::dom::SCORE_PARTID)) {
        if (!first) {
            out << ',';
        }
        first = false;
        out << "{\"cmper\":" << assign->getCmper()
            << ",\"inci\":" << assign->getInci().value_or(0)
            << ",\"left\":" << assign->left
            << ",\"bottom\":" << assign->bottom
            << ",\"width\":" << assign->width
            << ",\"height\":" << assign->height
            << ",\"f_desc_id\":" << assign->fDescId
            << '}';
    }
    out << ']';
}

// The measure graphic assignments, which are details rather than others and so carry a second
// comparator. The fields are the ones both sides spell as plain numbers.
void writeMeasureGraphicAssignments(std::ostream& out, const musx::dom::DocumentPtr& document)
{
    out << ",\"meas_graphic_assigns\":[";
    bool first = true;
    for (const auto& assign :
        document->getDetails()->getArray<musx::dom::details::MeasureGraphicAssign>(
            musx::dom::SCORE_PARTID)) {
        if (!first) {
            out << ',';
        }
        first = false;
        out << "{\"cmper1\":" << assign->getCmper1()
            << ",\"cmper2\":" << assign->getCmper2()
            << ",\"inci\":" << assign->getInci().value_or(0)
            << ",\"left\":" << assign->left
            << ",\"bottom\":" << assign->bottom
            << ",\"width\":" << assign->width
            << ",\"height\":" << assign->height
            << ",\"f_desc_id\":" << assign->fDescId
            << ",\"orig_width\":" << assign->origWidth
            << ",\"orig_height\":" << assign->origHeight
            << '}';
    }
    out << ']';
}

void writeShapeDefinitions(std::ostream& out, const musx::dom::DocumentPtr& document)
{
    out << ",\"shape_defs\":[";
    bool first = true;
    for (const auto& shape :
        document->getOthers()->getArray<musx::dom::others::ShapeDef>(musx::dom::SCORE_PARTID)) {
        if (!first) {
            out << ',';
        }
        first = false;
        out << "{\"cmper\":" << shape->getCmper()
            << ",\"instruction_list\":" << shape->instructionList
            << ",\"data_list\":" << shape->dataList
            << ",\"shape_type\":" << static_cast<int>(shape->shapeType)
            << ",\"blank\":" << jsonBool(shape->isBlank())
            << '}';
    }
    out << ']';
}

// The custom line styles. Which parameter block a record carries is the record's own
// statement, so the block is named rather than flattened: a comparison that flattened it could
// not tell a solid line's width from a dashed line's. The character is a code point on both
// sides by the time it is written here.
void writeSmartShapeCustomLines(std::ostream& out, const musx::dom::DocumentPtr& document)
{
    using CustomLine = musx::dom::others::SmartShapeCustomLine;
    out << ",\"ss_line_styles\":[";
    bool first = true;
    for (const auto& line :
        document->getOthers()->getArray<CustomLine>(musx::dom::SCORE_PARTID)) {
        if (!first) {
            out << ',';
        }
        first = false;
        out << "{\"cmper\":" << line->getCmper()
            << ",\"line_style\":" << static_cast<int>(line->lineStyle)
            << ",\"cap_start_type\":" << static_cast<int>(line->lineCapStartType)
            << ",\"cap_end_type\":" << static_cast<int>(line->lineCapEndType)
            << ",\"cap_start_arrow_id\":" << line->lineCapStartArrowId
            << ",\"cap_end_arrow_id\":" << line->lineCapEndArrowId
            << ",\"cap_start_hook_length\":" << line->lineCapStartHookLength
            << ",\"cap_end_hook_length\":" << line->lineCapEndHookLength
            << ",\"left_start_x\":" << line->leftStartX
            << ",\"left_start_y\":" << line->leftStartY
            << ",\"line_start_x\":" << line->lineStartX
            << ",\"line_end_x\":" << line->lineEndX
            << ",\"line_cont_x\":" << line->lineContX
            << ",\"line_char\":"
            << (line->charParams ? static_cast<std::uint32_t>(line->charParams->lineChar) : 0)
            << ",\"char_font_id\":"
            << (line->charParams ? line->charParams->font->fontId : 0)
            << ",\"char_font_size\":"
            << (line->charParams ? line->charParams->font->fontSize : 0)
            << ",\"solid_width\":" << (line->solidParams ? line->solidParams->lineWidth : 0)
            << ",\"dash_on\":" << (line->dashedParams ? line->dashedParams->dashOn : 0)
            << ",\"dash_off\":" << (line->dashedParams ? line->dashedParams->dashOff : 0)
            << '}';
    }
    out << ']';
}

// Every text class the reader recovers, as the records themselves. A text is compared by its
// characters rather than by a field list, so the whole string is emitted and the comparison is
// equality. The number goes with it because a recovered record claims a comparator as much as
// it claims characters, and musxdom keys the pool by it.
template <typename Target>
void writeTextClass(std::ostream& out, const musx::dom::DocumentPtr& document, const char* key)
{
    out << ",\"" << key << "\":[";
    bool first = true;
    for (const auto& item : document->getTexts()->getArray<Target>()) {
        if (!first) {
            out << ',';
        }
        first = false;
        out << "{\"number\":" << item->getTextNumber() << ",\"text\":" << jsonString(item->text)
            << '}';
    }
    out << ']';
}

void writeTexts(std::ostream& out, const musx::dom::DocumentPtr& document)
{
    using namespace musx::dom::texts;
    writeTextClass<BlockText>(out, document, "block_texts");
    writeTextClass<BookmarkText>(out, document, "bookmark_texts");
    writeTextClass<ExpressionText>(out, document, "expression_texts");
    writeTextClass<FileInfoText>(out, document, "file_info_texts");
    writeTextClass<LyricsChorus>(out, document, "lyrics_choruses");
    writeTextClass<LyricsSection>(out, document, "lyrics_sections");
    writeTextClass<LyricsVerse>(out, document, "lyrics_verses");
    writeTextClass<SmartShapeText>(out, document, "smart_shape_texts");
}

void writeFontDefinitions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    out << ",\"font_definitions\":[";
    bool first = true;
    for (const auto& font : document->getOthers()
            ->getArray<musx::dom::others::FontDefinition>(musx::dom::SCORE_PARTID)) {
        const auto target = "others.fontName[" + std::to_string(font->getCmper()) + "].name";
        out << (first ? "" : ",") << '{'
            << "\"cmper\":" << font->getCmper()
            << ",\"name\":" << jsonString(font->name)
            << ",\"origin\":" << jsonString(originOf(fields, target))
            << '}';
        first = false;
    }
    out << ']';
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::fprintf(stderr,
            "usage: options_coverage_probe <corpus-tsv> <output-jsonl>\n");
        return 2;
    }
    std::ifstream list(argv[1]);
    if (!list) {
        std::fprintf(stderr, "cannot open corpus list: %s\n", argv[1]);
        return 2;
    }
    std::ofstream output(argv[2]);
    if (!output) {
        std::fprintf(stderr, "cannot write output: %s\n", argv[2]);
        return 2;
    }

    std::size_t total = 0;
    std::size_t failed = 0;
    std::string line;
    while (std::getline(list, line)) {
        if (line.empty()) continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const auto corpusId = line.substr(0, tab);
        const auto path = line.substr(tab + 1);
        ++total;

        std::ostringstream out;
        out << '{' << "\"corpus_id\":" << jsonString(corpusId);
        try {
            const auto result =
                finale_mus_reader::Reader::read<musx::xml::pugi::Document>(
                    std::filesystem::path(path));
            if (!result.document) {
                throw std::runtime_error(importError(result.report));
            }
            const auto& report = result.report;
            const auto fields = fieldsByTarget(report);
            out << ",\"status\":\"ok\""
                << ",\"epoch\":" << jsonString(epochName(report.formatEpoch))
                << ",\"saving_product\":" << jsonString(report.savingProduct)
                << ",\"source_version\":" << jsonString(versionName(report))
                << ",\"warning_count\":" << report.diagnostics.size();
            writeClefOptions(out, result.document, fields);
            writeFontOptions(out, result.document, fields);
            writeMultimeasureRestOptions(out, result.document, fields);
            writeTextOptions(out, result.document, fields);
            writeLyricOptions(out, result.document, fields);
            writeFontDefinitions(out, result.document, fields);
            writeStemOptions(out, result.document, fields);
            writeMusicSpacingOptions(out, result.document, fields);
            writeLayerAttributes(out, result.document, fields);
            writeGraphicAssignments<musx::dom::others::PageGraphicAssign>(
                out, result.document, "page_graphic_assigns");
            writeGraphicAssignments<musx::dom::others::ShapeGraphicAssign>(
                out, result.document, "shape_graphic_assigns");
            writeShapeDefinitions(out, result.document);
            writeMeasureGraphicAssignments(out, result.document);
            writeSmartShapeCustomLines(out, result.document);
            writeTexts(out, result.document);
            out << '}';
        } catch (const std::exception& error) {
            ++failed;
            // Console-only, with the path, as the survey policy requires for failures.
            std::fprintf(stderr, "FAILED %s: %s\n    %s\n",
                corpusId.c_str(), error.what(), path.c_str());
            out << ",\"status\":\"error\""
                << ",\"error\":" << jsonString(error.what()) << '}';
        }
        output << out.str() << '\n';
    }
    std::fprintf(stderr, "read %zu documents, %zu failed\n", total, failed);
    return 0;
}
