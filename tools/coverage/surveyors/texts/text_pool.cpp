// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Every text class the reader recovers, as the records themselves. A text is compared by its
// characters rather than by a field list, so the whole string is emitted and the comparison is
// equality. The number goes with it because a recovered record claims a comparator as much as
// it claims characters, and musxdom keys the pool by it.

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
        auto observed = observe(*item, ctx,
            field("number", [](const Target& value) { return value.getTextNumber(); }),
            field("text", &Target::text));
        if (const auto* info = textFieldInfo<Target>(ctx, "text", item->getTextNumber())) {
            observed.asObject().emplace("effects_synthesized", info->effectsWereSynthesized);
            observed.asObject().emplace("font_synthesized", info->fontWasSynthesized);
            observed.asObject().emplace("size_synthesized", info->sizeWasSynthesized);
        }
        result.push_back(std::move(observed));
    }
    return Value(std::move(result));
}

#define TEXT_CLASS_SURVEYOR(key, Target) \
    Value observe_##Target(const SurveyContext& ctx) { \
        return observeTextClass<musx::dom::texts::Target>(ctx); \
    } \
    COVERAGE_SURVEYOR("texts", key, observe_##Target)

TEXT_CLASS_SURVEYOR("block_texts", BlockText);
TEXT_CLASS_SURVEYOR("bookmark_texts", BookmarkText);
TEXT_CLASS_SURVEYOR("expression_texts", ExpressionText);
TEXT_CLASS_SURVEYOR("file_info_texts", FileInfoText);
TEXT_CLASS_SURVEYOR("lyrics_choruses", LyricsChorus);
TEXT_CLASS_SURVEYOR("lyrics_sections", LyricsSection);
TEXT_CLASS_SURVEYOR("lyrics_verses", LyricsVerse);
TEXT_CLASS_SURVEYOR("smart_shape_texts", SmartShapeText);

} // namespace
