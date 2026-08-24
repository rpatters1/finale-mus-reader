// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// LyricOptions. Its two collections are fixed-length maps in musxdom rather than sequences, so
// each element is emitted under its own type name and a comparison never depends on iteration
// order. `alt_hyphen_font_name` is resolved rather than reported as a comparator, because the
// legacy pool and the companion renumber font definitions independently.

#include <exception>
#include <string>
#include <utility>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeLyricOptions(const SurveyContext& ctx)
{
    using Lyrics = musx::dom::options::LyricOptions;
    const auto options = ctx.document->getOptions()->get<Lyrics>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("hyphen_char", &Lyrics::hyphenChar), field("max_hyphen_separation", &Lyrics::maxHyphenSeparation),
        field("word_ext_vert_offset", &Lyrics::wordExtVertOffset), field("word_ext_horz_offset", &Lyrics::wordExtHorzOffset),
        field("word_ext_line_width", &Lyrics::wordExtLineWidth), field("word_ext_min_length", &Lyrics::wordExtMinLength),
        field("smart_hyphen_start", &Lyrics::smartHyphenStart), field("lyric_auto_num_type", &Lyrics::lyricAutoNumType),
        field("use_smart_word_extensions", &Lyrics::useSmartWordExtensions), field("use_smart_hyphens", &Lyrics::useSmartHyphens),
        field("use_alt_hyphen_font", &Lyrics::useAltHyphenFont), field("word_ext_need_underscore", &Lyrics::wordExtNeedUnderscore),
        field("word_ext_offset_to_notehead", &Lyrics::wordExtOffsetToNotehead),
        field("lyric_use_edge_punctuation", &Lyrics::lyricUseEdgePunctuation),
        field("show_auto_numbers_verses", &Lyrics::showAutoNumbersOnVerses),
        field("show_auto_numbers_choruses", &Lyrics::showAutoNumbersOnChoruses),
        field("show_auto_numbers_sections", &Lyrics::showAutoNumbersOnSections),
        field("punctuation_to_ignore", &Lyrics::lyricPunctuationToIgnore));

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
    result.asObject().emplace("alt_hyphen_font_name", altHyphenName);

    Value::Object syllableStyles;
    for (const auto& [name, type] : {
             std::pair{"default", Lyrics::SyllablePosStyleType::Default},
             std::pair{"wordExt", Lyrics::SyllablePosStyleType::WordExt},
             std::pair{"first", Lyrics::SyllablePosStyleType::First},
             std::pair{"systemStart", Lyrics::SyllablePosStyleType::SystemStart}}) {
        const auto found = options->syllablePosStyles.find(type);
        if (found == options->syllablePosStyles.end() || !found->second) {
            syllableStyles.emplace(name, Value{});
            continue;
        }
        syllableStyles.emplace(name, Value::Object{{"align", static_cast<int>(found->second->align)},
            {"justify", static_cast<int>(found->second->justify)}, {"on", found->second->on}});
    }
    result.asObject().emplace("syllable_pos_styles", std::move(syllableStyles));

    Value::Object connectStyles;
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
        if (found == options->wordExtConnectStyles.end() || !found->second) {
            connectStyles.emplace(name, Value{});
            continue;
        }
        connectStyles.emplace(name, Value::Object{{"connect_index", static_cast<int>(found->second->connectIndex)},
            {"x", found->second->xOffset}, {"y", found->second->yOffset}});
    }
    result.asObject().emplace("word_ext_connect_styles", std::move(connectStyles));

    for (const auto* member : {"maxHyphenSeparation", "wordExtVertOffset", "wordExtHorzOffset",
             "wordExtLineWidth", "wordExtMinLength", "smartHyphenStart", "lyricAutoNumType",
             "useSmartWordExtensions", "useSmartHyphens", "useAltHyphenFont",
             "wordExtNeedUnderscore", "wordExtOffsetToNotehead", "lyricUseEdgePunctuation",
             "showAutoNumbersOnVerses", "showAutoNumbersOnChoruses", "showAutoNumbersOnSections",
             "hyphenChar", "lyricPunctuationToIgnore"}) {
        result.asObject().emplace(std::string("origin_") + member,
            fieldOrigin<Lyrics>(ctx, member));
    }
    return result;
}

COVERAGE_SURVEYOR("options", "lyric_options", observeLyricOptions);

} // namespace
