// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Every text class the reader recovers, as the records themselves. A text is compared by its
// characters rather than by a field list, so the whole string is emitted and the comparison is
// equality. The number goes with it because a recovered record claims a comparator as much as
// it claims characters, and musxdom keys the pool by it.

#include <algorithm>
#include <string>
#include <vector>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

template <typename Target>
Value observeTextClass(const SurveyContext& ctx)
{
    Value::Array result;
    for (const auto& item : ctx.document->getTexts()->getArray<Target>()) {
        result.push_back(observe(*item, ctx,
            field("number", [](const Target& value) { return value.getTextNumber(); }),
            field("text", &Target::text)));
    }
    return Value(std::move(result));
}

Value observeTextMetadata(const SurveyContext& ctx)
{
    std::vector<std::string> targets;
    targets.reserve(ctx.fields.textFields().size());
    for (const auto& entry : ctx.fields.textFields()) {
        targets.push_back(entry.first);
    }
    std::sort(targets.begin(), targets.end());

    Value::Object result;
    for (const auto& target : targets) {
        const auto& info = ctx.fields.textFields().at(target);
        result.emplace(target, Value::Object{
            {"effects_synthesized", Value(info.effectsWereSynthesized)},
            {"font_synthesized", Value(info.fontWasSynthesized)},
            {"size_synthesized", Value(info.sizeWasSynthesized)}});
    }
    return Value(std::move(result));
}

COVERAGE_SURVEYOR("text_metadata", observeTextMetadata)

#define TEXT_CLASS_SURVEYOR(key, Target) \
    Value observe_##Target(const SurveyContext& ctx) { \
        return observeTextClass<musx::dom::texts::Target>(ctx); \
    } \
    COVERAGE_SURVEYOR(key, observe_##Target)

TEXT_CLASS_SURVEYOR("block_texts", BlockText);
TEXT_CLASS_SURVEYOR("bookmark_texts", BookmarkText);
TEXT_CLASS_SURVEYOR("expression_texts", ExpressionText);
TEXT_CLASS_SURVEYOR("file_info_texts", FileInfoText);
TEXT_CLASS_SURVEYOR("lyrics_choruses", LyricsChorus);
TEXT_CLASS_SURVEYOR("lyrics_sections", LyricsSection);
TEXT_CLASS_SURVEYOR("lyrics_verses", LyricsVerse);
TEXT_CLASS_SURVEYOR("smart_shape_texts", SmartShapeText);

} // namespace
