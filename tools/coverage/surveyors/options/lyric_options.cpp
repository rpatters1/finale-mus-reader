// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// LyricOptions. Its two collections are fixed-length maps in musxdom rather than sequences, so
// each element is emitted under its own type name and a comparison never depends on iteration
// order. `alt_hyphen_font_name` is resolved rather than reported as a comparator, because the
// legacy pool and the companion renumber font definitions independently.

#include <exception>
#include <ostream>
#include <string>
#include <utility>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeLyricOptions(std::ostream& out, const SurveyContext& ctx)
{
    using Lyrics = musx::dom::options::LyricOptions;
    const auto options = ctx.document->getOptions()->get<Lyrics>();
    if (!options) {
        out << "null";
        return;
    }
    out << '{'
        << "\"hyphen_char\":" << static_cast<std::uint32_t>(options->hyphenChar)
        << ",\"max_hyphen_separation\":" << options->maxHyphenSeparation
        << ",\"word_ext_vert_offset\":" << options->wordExtVertOffset
        << ",\"word_ext_horz_offset\":" << options->wordExtHorzOffset
        << ",\"word_ext_line_width\":" << options->wordExtLineWidth
        << ",\"word_ext_min_length\":" << options->wordExtMinLength
        << ",\"smart_hyphen_start\":" << static_cast<int>(options->smartHyphenStart)
        << ",\"lyric_auto_num_type\":" << static_cast<int>(options->lyricAutoNumType)
        << ",\"use_smart_word_extensions\":" << jsonBool(options->useSmartWordExtensions)
        << ",\"use_smart_hyphens\":" << jsonBool(options->useSmartHyphens)
        << ",\"use_alt_hyphen_font\":" << jsonBool(options->useAltHyphenFont)
        << ",\"word_ext_need_underscore\":" << jsonBool(options->wordExtNeedUnderscore)
        << ",\"word_ext_offset_to_notehead\":" << jsonBool(options->wordExtOffsetToNotehead)
        << ",\"lyric_use_edge_punctuation\":" << jsonBool(options->lyricUseEdgePunctuation)
        << ",\"show_auto_numbers_verses\":" << jsonBool(options->showAutoNumbersOnVerses)
        << ",\"show_auto_numbers_choruses\":" << jsonBool(options->showAutoNumbersOnChoruses)
        << ",\"show_auto_numbers_sections\":" << jsonBool(options->showAutoNumbersOnSections)
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
        if (!firstStyle) out << ',';
        firstStyle = false;
        out << jsonString(name) << ':';
        if (found == options->syllablePosStyles.end() || !found->second) {
            out << "null";
            continue;
        }
        out << "{\"align\":" << static_cast<int>(found->second->align)
            << ",\"justify\":" << static_cast<int>(found->second->justify)
            << ",\"on\":" << jsonBool(found->second->on) << '}';
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
        if (!firstConnect) out << ',';
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
            << jsonString(ctx.fields.originOf(std::string("options.lyricOptions.") + member));
    }
    out << '}';
}

COVERAGE_SURVEYOR("lyric_options", writeLyricOptions);

} // namespace
