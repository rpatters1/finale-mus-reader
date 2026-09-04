// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Every text class the reader recovers, as the records themselves. A text is
// compared by its characters rather than by a field list, so the whole string
// is emitted and the comparison is equality. The number goes with it because a
// recovered record claims a comparator as much as it claims characters, and
// musxdom keys the pool by it.

#include <regex>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<TextClassificationResult>
classifyBlockTextDifference(const TextDifferenceContext& context)
{
    if (!context.sourcePlain && context.normalizedCompanion.empty()) {
        static const std::regex emptyPartNameTemplate(
            R"(^(?:\^(?:font|fontid|Font|fontMus|fontTxt|fontNum|size|nfx)\([^)]*\))*\^partname\(\)$)");
        if (std::regex_match(context.normalizedSource.begin(), context.normalizedSource.end(),
                             emptyPartNameTemplate)) {
            return TextClassificationResult{
                false, {TextDifferenceClassification::EmptyPartNameTemplate}, {}};
        }
    }
    // A score name Finale wrote into an empty block the source already carries. The source's score
    // part names no text, so this is not a part name recovered wrongly: it is the same synthesis
    // that `synthesized-score-name` records on `partDef.nameId`, reaching the text pool because
    // the block existed rather than being allocated fresh.
    if (context.synthesizedScoreName && context.sourcePlain && context.companionPlain
        && context.sourcePlain->empty() && *context.companionPlain == "Score") {
        return TextClassificationResult{
            false, {TextDifferenceClassification::SynthesizedScoreName}, {}};
    }
    if (!context.sourcePlain || !context.companionPlain || !context.partNameText) {
        return std::nullopt;
    }
    if (*context.sourcePlain == *context.companionPlain) {
        if (context.removedWhitespaceControl) {
            return TextClassificationResult{false, {TextDifferenceClassification::Whitespace}, {}};
        }
        return TextClassificationResult{
            true, {}, ComparisonTransformation::FinaleReformattedPartName};
    }
    if (context.sourcePlain->empty() && *context.companionPlain == "Score") {
        return TextClassificationResult{
            false, {TextDifferenceClassification::EmptyPartNameTemplate}, {}};
    }
    return std::nullopt;
}

std::optional<TextClassificationResult>
classifyFileInfoTextDifference(const TextDifferenceContext& context)
{
    if (!context.sourcePlain || !context.companionPlain ||
        *context.sourcePlain != *context.companionPlain) {
        return std::nullopt;
    }
    std::set<TextDifferenceClassification> differences{TextDifferenceClassification::AddedFontInfo};
    if (context.removedWhitespaceControl) {
        differences.insert(TextDifferenceClassification::Whitespace);
    }
    return TextClassificationResult{false, std::move(differences), {}};
}

template <typename Target> Value observeTextClass(const SurveyContext& ctx)
{
    Value::Array result;
    for (const auto& item : ctx.document->getTexts()->getArray<Target>()) {
        auto observed = observe(
            *item, ctx, field("number", [](const Target& value) { return value.getTextNumber(); }),
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

#define TEXT_CLASS_SURVEYOR(key, Target)                                                           \
    Value observe_##Target(const SurveyContext& ctx)                                               \
    {                                                                                              \
        return observeTextClass<musx::dom::texts::Target>(ctx);                                    \
    }                                                                                              \
    COVERAGE_SURVEYOR("texts", key, observe_##Target)

Value observe_BlockText(const SurveyContext& ctx)
{
    return observeTextClass<musx::dom::texts::BlockText>(ctx);
}
COVERAGE_TEXT_CLASS("texts", "block_texts", observe_BlockText, classifyBlockTextDifference);
TEXT_CLASS_SURVEYOR("bookmark_texts", BookmarkText);
TEXT_CLASS_SURVEYOR("expression_texts", ExpressionText);
Value observe_FileInfoText(const SurveyContext& ctx)
{
    return observeTextClass<musx::dom::texts::FileInfoText>(ctx);
}
COVERAGE_TEXT_CLASS("texts", "file_info_texts", observe_FileInfoText,
                    classifyFileInfoTextDifference);
TEXT_CLASS_SURVEYOR("lyrics_choruses", LyricsChorus);
TEXT_CLASS_SURVEYOR("lyrics_sections", LyricsSection);
TEXT_CLASS_SURVEYOR("lyrics_verses", LyricsVerse);
TEXT_CLASS_SURVEYOR("smart_shape_texts", SmartShapeText);

} // namespace
