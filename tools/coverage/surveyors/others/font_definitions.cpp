// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <map>
#include <string>
#include <utility>

#include "coverage/common/font_info.h"
#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeFontDefinitions(const SurveyContext& ctx)
{
    const auto fonts = ctx.document->getOthers()->getArray<musx::dom::others::FontDefinition>(
        musx::dom::SCORE_PARTID);

    // A duplicate normalized name is not itself a defect -- Finale legitimately
    // clones font definitions -- but a duplicate that the source did not already
    // contain (introduced during import, e.g. by a fallback default) is worth
    // flagging: it is new ambiguity the source never had.
    std::map<std::string, std::pair<std::size_t, std::size_t>> nonzeroNameCounts;
    for (const auto& font : fonts) {
        if (font->getCmper() == 0) continue;
        const auto name = musx::dom::normalizeFontName(font->name);
        if (name.empty()) continue;
        auto& [total, introduced] = nonzeroNameCounts[name];
        ++total;
        if (fieldOrigin<musx::dom::others::FontDefinition>(ctx, "name", font->getCmper()) !=
            "legacy-mus")
            ++introduced;
    }
    std::size_t duplicateCount = 0;
    std::size_t introducedDuplicateCount = 0;
    for (const auto& [name, counts] : nonzeroNameCounts) {
        static_cast<void>(name);
        if (counts.first <= 1) continue;
        ++duplicateCount;
        if (counts.second != 0) ++introducedDuplicateCount;
    }

    Value::Array definitions;
    for (const auto& font : fonts) {
        definitions.emplace_back(observe(
            *font, ctx, field("cmper", [](const auto& value) { return value.getCmper(); }),
            field("name", &musx::dom::others::FontDefinition::name),
            field("normalized_name",
                  [](const auto& value) { return musx::dom::normalizeFontName(value.name); }),
            field("charset_bank", &musx::dom::others::FontDefinition::charsetBank),
            field("charset_val", &musx::dom::others::FontDefinition::charsetVal),
            field("pitch", &musx::dom::others::FontDefinition::pitch),
            field("family", &musx::dom::others::FontDefinition::family),
            field(fontDefinitionIsSymbolField,
                &musx::dom::others::FontDefinition::calcIsSymbolFont),
            field("origin_name",
                  [&ctx](const auto& value) {
                      return fieldOrigin<musx::dom::others::FontDefinition>(ctx, "name",
                                                                            value.getCmper());
                  }),
            field("origin_charsetBank",
                  [&ctx](const auto& value) {
                      return fieldOrigin<musx::dom::others::FontDefinition>(ctx, "charsetBank",
                                                                            value.getCmper());
                  }),
            field("origin_charsetVal",
                  [&ctx](const auto& value) {
                      return fieldOrigin<musx::dom::others::FontDefinition>(ctx, "charsetVal",
                                                                            value.getCmper());
                  }),
            field("origin_pitch",
                  [&ctx](const auto& value) {
                      return fieldOrigin<musx::dom::others::FontDefinition>(ctx, "pitch",
                                                                            value.getCmper());
                  }),
            field("origin_family", [&ctx](const auto& value) {
                return fieldOrigin<musx::dom::others::FontDefinition>(ctx, "family",
                                                                      value.getCmper());
            })));
    }
    return Value::Object{{"definitions", std::move(definitions)},
                         {"duplicate_nonzero_name_count", duplicateCount},
                         {"introduced_duplicate_nonzero_name_count", introducedDuplicateCount}};
}

COVERAGE_CLASS("others", "font_definitions", observeFontDefinitions,
    classifyFontDefinitionDifference);

} // namespace
