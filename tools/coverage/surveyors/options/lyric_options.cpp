// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// LyricOptions. Its two collections are fixed-length maps in musxdom rather
// than sequences, so each element is emitted under its own type name and a
// comparison never depends on iteration order. `alt_hyphen_font_name` is
// resolved rather than reported as a comparator, because the legacy pool and
// the companion renumber font definitions independently.

#include <exception>
#include <string>
#include <utility>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/support/source_gate.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification> classifyLyricDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category != Differs) return std::nullopt;
    if ((context.path == "lyric_options.use_smart_hyphens" ||
         context.path == "lyric_options.use_smart_word_extensions") &&
        context.origin == "legacy-behavior" && context.sourceValue.isBool() &&
        context.companionValue.isBool() && !context.sourceValue.asBool() &&
        context.companionValue.asBool()) {
        return DifferenceClassification::SmartLyricsEnabled;
    }
    if (context.path == "lyric_options.word_ext_connect_styles.oneEntryEnd.x" &&
        context.sourceValue.isInteger() && context.companionValue.isInteger() &&
        context.sourceValue.asInteger() == 42 && context.companionValue.asInteger() == 44 &&
        sourcePredatesVersion(context.epoch, context.sourceVersion,
                              finale_mus_reader::FormatEpoch::DclLegacy,
                              finale_mus_reader::versions::finale2004) &&
        comparisonEqualSurrounding(context.source, context.companion,
                                   "lyric_options.word_ext_connect_styles.", context.path)) {
        return DifferenceClassification::PreConnectionEndpoint;
    }
    return std::nullopt;
}

Value observeLyricOptions(const SurveyContext& ctx)
{
    using Lyrics = musx::dom::options::LyricOptions;
    const auto options = ctx.document->getOptions()->get<Lyrics>();
    if (!options) return {};
    auto result = observe(*options, ctx, field("hyphen_char", &Lyrics::hyphenChar),
                          field("max_hyphen_separation", &Lyrics::maxHyphenSeparation),
                          field("word_ext_vert_offset", &Lyrics::wordExtVertOffset),
                          field("word_ext_horz_offset", &Lyrics::wordExtHorzOffset),
                          field("word_ext_line_width", &Lyrics::wordExtLineWidth),
                          field("word_ext_min_length", &Lyrics::wordExtMinLength),
                          field("smart_hyphen_start", &Lyrics::smartHyphenStart),
                          field("lyric_auto_num_type", &Lyrics::lyricAutoNumType),
                          field("use_smart_word_extensions", &Lyrics::useSmartWordExtensions),
                          field("use_smart_hyphens", &Lyrics::useSmartHyphens),
                          field("use_alt_hyphen_font", &Lyrics::useAltHyphenFont),
                          field("word_ext_need_underscore", &Lyrics::wordExtNeedUnderscore),
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
            // An unregistered comparator throws here rather than resolving. That is
            // exactly the failure this field is worth watching, so it is recorded
            // rather than swallowed.
            altHyphenName = "<unresolved>";
        }
    }
    result.asObject().emplace("alt_hyphen_font_name", altHyphenName);
    result.asObject().emplace("alt_hyphen_font_size",
                              options->altHyphenFont ? options->altHyphenFont->fontSize : 0);
    result.asObject().emplace("alt_hyphen_font_bold",
                              options->altHyphenFont && options->altHyphenFont->bold);
    result.asObject().emplace("alt_hyphen_font_italic",
                              options->altHyphenFont && options->altHyphenFont->italic);
    result.asObject().emplace("alt_hyphen_font_underline",
                              options->altHyphenFont && options->altHyphenFont->underline);
    result.asObject().emplace("alt_hyphen_font_strikeout",
                              options->altHyphenFont && options->altHyphenFont->strikeout);
    result.asObject().emplace("alt_hyphen_font_absolute",
                              options->altHyphenFont && options->altHyphenFont->absolute);
    result.asObject().emplace("alt_hyphen_font_hidden",
                              options->altHyphenFont && options->altHyphenFont->hidden);
    result.asObject().emplace("origin_altHyphenFontName",
                              fieldOrigin<Lyrics>(ctx, "altHyphenFont.fontId"));
    result.asObject().emplace("origin_altHyphenFontSize",
                              fieldOrigin<Lyrics>(ctx, "altHyphenFont.fontSize"));
    for (const auto* member : {"Bold", "Italic", "Underline", "Strikeout", "Absolute", "Hidden"}) {
        result.asObject().emplace(std::string("origin_altHyphenFont") + member,
                                  fieldOrigin<Lyrics>(ctx, "altHyphenFont.effects"));
    }

    Value::Object syllableStyles;
    for (const auto& [name, type] :
         {std::pair{"default", Lyrics::SyllablePosStyleType::Default},
          std::pair{"wordExt", Lyrics::SyllablePosStyleType::WordExt},
          std::pair{"first", Lyrics::SyllablePosStyleType::First},
          std::pair{"systemStart", Lyrics::SyllablePosStyleType::SystemStart}}) {
        const auto found = options->syllablePosStyles.find(type);
        if (found == options->syllablePosStyles.end() || !found->second) {
            syllableStyles.emplace(name, Value{});
            continue;
        }
        const auto prefix = std::string("syllablePosStyles[") + name + "].";
        syllableStyles.emplace(
            name, Value::Object{{"align", static_cast<int>(found->second->align)},
                                {"justify", static_cast<int>(found->second->justify)},
                                {"on", found->second->on},
                                {"origin_align", fieldOrigin<Lyrics>(ctx, prefix + "align")},
                                {"origin_justify", fieldOrigin<Lyrics>(ctx, prefix + "justify")},
                                {"origin_on", fieldOrigin<Lyrics>(ctx, prefix + "on")}});
    }
    result.asObject().emplace("syllable_pos_styles", std::move(syllableStyles));

    Value::Object connectStyles;
    for (const auto& [name, type] :
         {std::pair{"defaultStart", Lyrics::WordExtConnectStyleType::DefaultStart},
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
        const auto prefix = std::string("wordExtConnectStyles[") + name + "].";
        connectStyles.emplace(
            name, Value::Object{
                      {"connect_index", static_cast<int>(found->second->connectIndex)},
                      {"x", found->second->xOffset},
                      {"y", found->second->yOffset},
                      {"origin_connectIndex", fieldOrigin<Lyrics>(ctx, prefix + "connectIndex")},
                      {"origin_x", fieldOrigin<Lyrics>(ctx, prefix + "xOffset")},
                      {"origin_y", fieldOrigin<Lyrics>(ctx, prefix + "yOffset")}});
    }
    result.asObject().emplace("word_ext_connect_styles", std::move(connectStyles));

    for (const auto* member :
         {"maxHyphenSeparation", "wordExtVertOffset", "wordExtHorzOffset", "wordExtLineWidth",
          "wordExtMinLength", "smartHyphenStart", "lyricAutoNumType", "useSmartWordExtensions",
          "useSmartHyphens", "useAltHyphenFont", "wordExtNeedUnderscore", "wordExtOffsetToNotehead",
          "lyricUseEdgePunctuation", "showAutoNumbersOnVerses", "showAutoNumbersOnChoruses",
          "showAutoNumbersOnSections", "hyphenChar", "lyricPunctuationToIgnore"}) {
        result.asObject().emplace(std::string("origin_") + member,
                                  fieldOrigin<Lyrics>(ctx, member));
    }
    return result;
}

COVERAGE_CLASS("options", "lyric_options", observeLyricOptions, classifyLyricDifference);

} // namespace
