// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

#include <algorithm>
#include <cstdint>

namespace {
using namespace finale_mus_reader::coverage;

void prepareTextExpressionDefComparison(ComparisonPreparationContext& context)
{
    const auto omit = [](SurveySnapshot& snapshot) {
        snapshot.erase("text_expression_defs");
        snapshot.erase("expression_texts");
        const auto blocks = snapshot.find("text_blocks");
        if (blocks == snapshot.end() || !blocks->second.isArray()) return;
        std::erase_if(blocks->second.asArray(), [](const Value& item) {
            const auto* type = item.find("text_type");
            return type && type->isInteger() &&
                   type->asInteger() == static_cast<std::int64_t>(
                                            musx::dom::others::TextBlock::TextType::Expression);
        });
    };
    omit(context.source);
    omit(context.companion);
}

Value observeTextExpressionDefs(const SurveyContext& ctx)
{
    using Expression = musx::dom::others::TextExpressionDef;
    Value::Array result;
    for (const auto& expression : sourceInstances<Expression>(ctx)) {
        result.emplace_back(observe(*expression, ctx,
            field("cmper", [](const Expression& value) { return value.getCmper(); }),
            field("text_id_key", &Expression::textIdKey),
            field("origin_textIdKey",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "textIdKey", value);
                }),
            field("category_id", &Expression::categoryId),
            field("origin_categoryId",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "categoryId", value);
                }),
            field("rehearsal_mark_style", &Expression::rehearsalMarkStyle),
            field("origin_rehearsalMarkStyle",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "rehearsalMarkStyle", value);
                }),
            field("value", &Expression::value),
            field("origin_value",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "value", value);
                }),
            field("exec_shape", &Expression::execShape),
            field("origin_execShape",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "execShape", value);
                }),
            field("aux_data1", &Expression::auxData1),
            field("origin_auxData1",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "auxData1", value);
                }),
            field("play_pass", &Expression::playPass),
            field("origin_playPass",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "playPass", value);
                }),
            field("hide_measure_num", &Expression::hideMeasureNum),
            field("origin_hideMeasureNum",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "hideMeasureNum", value);
                }),
            field("match_playback", &Expression::matchPlayback),
            field("origin_matchPlayback",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "matchPlayback", value);
                }),
            field("use_aux_data", &Expression::useAuxData),
            field("origin_useAuxData",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "useAuxData", value);
                }),
            field("has_enclosure", &Expression::hasEnclosure),
            field("origin_hasEnclosure",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "hasEnclosure", value);
                }),
            field("break_mm_rest", &Expression::breakMmRest),
            field("origin_breakMmRest",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "breakMmRest", value);
                }),
            field("created_by_hp", &Expression::createdByHp),
            field("origin_createdByHp",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "createdByHp", value);
                }),
            field("playback_type", &Expression::playbackType),
            field("origin_playbackType",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "playbackType", value);
                }),
            // Position anchors, justification, and offsets are excluded until
            // assignment-dependent positioning conversion is supported.
            field("use_category_fonts", &Expression::useCategoryFonts),
            field("origin_useCategoryFonts",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "useCategoryFonts", value);
                }),
            field("use_category_pos", &Expression::useCategoryPos),
            field("origin_useCategoryPos",
                [&ctx](const Expression& value) {
                    return fieldOrigin<Expression>(ctx, "useCategoryPos", value);
                }),
            field("description", &Expression::description),
            field("origin_description", [&ctx](const Expression& value) {
                return fieldOrigin<Expression>(ctx, "description", value);
            })));
    }
    return result;
}
COVERAGE_CLASS_WITH_PREPARATION("others", "text_expression_defs", observeTextExpressionDefs,
                                nullptr, prepareTextExpressionDefComparison);
} // namespace
