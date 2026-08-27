// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <set>
#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyTextBlockDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category != Differs) return std::nullopt;
    if ((comparisonPathEndsWith(context.path, ".new_pos_36") ||
         comparisonPathEndsWith(context.path, ".no_expand_single_word")) &&
        context.origin == "legacy-behavior" && context.sourceValue.isBool() &&
        context.companionValue.isBool() && !context.sourceValue.asBool() &&
        context.companionValue.asBool()) {
        return DifferenceClassification::CodaTextBlockUpgrade;
    }
    if (context.epoch == finale_mus_reader::FormatEpoch::CodaBanner &&
        comparisonPathEndsWith(context.path, ".shape_id") && context.sourceValue.isInteger() &&
        context.companionValue.isInteger() && context.sourceValue.asInteger() == 0 &&
        context.companionValue.asInteger() != 0) {
        return DifferenceClassification::CodaTextBlockUpgrade;
    }
    if (context.epoch == finale_mus_reader::FormatEpoch::CodaBanner &&
        comparisonPathEndsWith(context.path, ".show_shape") && context.sourceValue.isBool() &&
        context.companionValue.isBool() && !context.sourceValue.asBool() &&
        context.companionValue.asBool()) {
        return DifferenceClassification::CodaTextBlockUpgrade;
    }
    if (comparisonPathEndsWith(context.path, "shape_id") && context.origin == "finale27-default") {
        return DifferenceClassification::DefaultShapeId;
    }
    if (context.relatedDifference == RelatedDifference::MatchingPageOnlyTextBlockReferent &&
        comparisonPathEndsWith(context.path, ".justify") && context.sourceValue.isInteger() &&
        context.companionValue.isInteger() &&
        std::set<std::int64_t>{context.sourceValue.asInteger(),
                               context.companionValue.asInteger()} ==
            std::set<std::int64_t>{0, 2}) {
        return DifferenceClassification::LegacyPageParityText;
    }
    if (context.relatedDifference == RelatedDifference::RenumberedTextBlockReferent) {
        return DifferenceClassification::FinaleTextBlockRenumbering;
    }
    const auto equals = context.path.find("cmper=");
    const auto close = context.path.find(']', equals);
    if (equals != std::string_view::npos && close != std::string_view::npos &&
        std::stoll(std::string(context.path.substr(equals + 6, close - equals - 6))) > 65000) {
        return DifferenceClassification::TransientTextBlock;
    }
    return std::nullopt;
}

Value observeTextBlocks(const SurveyContext& ctx)
{
    using TextBlock = musx::dom::others::TextBlock;

    Value::Array result;
    for (const auto& block :
         ctx.document->getOthers()->getArray<TextBlock>(musx::dom::SCORE_PARTID)) {
        result.push_back(observe(
            *block, ctx, field("cmper", [](const TextBlock& value) { return value.getCmper(); }),
            field("text_id", &TextBlock::textId), field("shape_id", &TextBlock::shapeId),
            field("width", &TextBlock::width), field("height", &TextBlock::height),
            field("line_spacing_percent", &TextBlock::lineSpacingPercentage),
            field("line_spacing_evpu", &TextBlock::lineSpacingEvpu),
            field("x_add", &TextBlock::xAdd), field("y_add", &TextBlock::yAdd),
            field("justify", &TextBlock::justify), field("new_pos_36", &TextBlock::newPos36),
            field("show_shape", &TextBlock::showShape),
            field("no_expand_single_word", &TextBlock::noExpandSingleWord),
            field("word_wrap", &TextBlock::wordWrap), field("inset", &TextBlock::inset),
            field("standard_line", &TextBlock::stdLineThickness),
            field("round_corners", &TextBlock::roundCorners),
            field("corner_radius", &TextBlock::cornerRadius),
            field("text_type", &TextBlock::textType),
            field("origin_text_id",
                  [](const TextBlock& value, const SurveyContext& context) {
                      return fieldOrigin<TextBlock>(context, "textId", value.getCmper());
                  }),
            field("origin_justify",
                  [](const TextBlock& value, const SurveyContext& context) {
                      return fieldOrigin<TextBlock>(context, "justify", value.getCmper());
                  }),
            field("origin_newPos36",
                  [](const TextBlock& value, const SurveyContext& context) {
                      return fieldOrigin<TextBlock>(context, "newPos36", value.getCmper());
                  }),
            field("origin_noExpandSingleWord",
                  [](const TextBlock& value, const SurveyContext& context) {
                      return fieldOrigin<TextBlock>(context, "noExpandSingleWord",
                                                    value.getCmper());
                  }),
            field("origin_round_corners",
                  [](const TextBlock& value, const SurveyContext& context) {
                      return fieldOrigin<TextBlock>(context, "roundCorners", value.getCmper());
                  }),
            field("origin_corner_radius", [](const TextBlock& value, const SurveyContext& context) {
                return fieldOrigin<TextBlock>(context, "cornerRadius", value.getCmper());
            })));
    }
    return Value(std::move(result));
}

COVERAGE_CLASS("others", "text_blocks", observeTextBlocks, classifyTextBlockDifference);

} // namespace
