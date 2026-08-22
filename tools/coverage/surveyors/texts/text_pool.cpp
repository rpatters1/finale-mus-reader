// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Every text class the reader recovers, as the records themselves. A text is compared by its
// characters rather than by a field list, so the whole string is emitted and the comparison is
// equality. The number goes with it because a recovered record claims a comparator as much as
// it claims characters, and musxdom keys the pool by it.

#include <ostream>
#include <algorithm>
#include <string>
#include <vector>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

template <typename Target>
void writeTextClass(std::ostream& out, const SurveyContext& ctx)
{
    out << '[';
    bool first = true;
    for (const auto& item : ctx.document->getTexts()->getArray<Target>()) {
        if (!first) out << ',';
        first = false;
        out << "{\"number\":" << item->getTextNumber() << ",\"text\":" << jsonString(item->text)
            << '}';
    }
    out << ']';
}

void writeTextMetadata(std::ostream& out, const SurveyContext& ctx)
{
    std::vector<std::string> targets;
    targets.reserve(ctx.report.textFields.size());
    for (const auto& entry : ctx.report.textFields) {
        targets.push_back(entry.first);
    }
    std::sort(targets.begin(), targets.end());

    out << '{';
    bool first = true;
    for (const auto& target : targets) {
        const auto& info = ctx.report.textFields.at(target);
        if (!first) out << ',';
        first = false;
        out << jsonString(target) << ":{\"font_synthesized\":"
            << (info.fontWasSynthesized ? "true" : "false")
            << ",\"size_synthesized\":" << (info.sizeWasSynthesized ? "true" : "false")
            << ",\"effects_synthesized\":"
            << (info.effectsWereSynthesized ? "true" : "false") << '}';
    }
    out << '}';
}

COVERAGE_SURVEYOR("text_metadata", writeTextMetadata)

#define TEXT_CLASS_SURVEYOR(key, Target) \
    void write_##Target(std::ostream& out, const SurveyContext& ctx) { \
        writeTextClass<musx::dom::texts::Target>(out, ctx); \
    } \
    COVERAGE_SURVEYOR(key, write_##Target)

TEXT_CLASS_SURVEYOR("block_texts", BlockText);
TEXT_CLASS_SURVEYOR("bookmark_texts", BookmarkText);
TEXT_CLASS_SURVEYOR("expression_texts", ExpressionText);
TEXT_CLASS_SURVEYOR("file_info_texts", FileInfoText);
TEXT_CLASS_SURVEYOR("lyrics_choruses", LyricsChorus);
TEXT_CLASS_SURVEYOR("lyrics_sections", LyricsSection);
TEXT_CLASS_SURVEYOR("lyrics_verses", LyricsVerse);
TEXT_CLASS_SURVEYOR("smart_shape_texts", SmartShapeText);

} // namespace
