// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <map>
#include <ostream>
#include <string>
#include <utility>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeFontDefinitions(std::ostream& out, const SurveyContext& ctx)
{
    const auto fonts = ctx.document->getOthers()
        ->getArray<musx::dom::others::FontDefinition>(musx::dom::SCORE_PARTID);

    // A duplicate normalized name is not itself a defect -- Finale legitimately clones font
    // definitions -- but a duplicate that the source did not already contain (introduced
    // during import, e.g. by a fallback default) is worth flagging: it is new ambiguity the
    // source never had.
    std::map<std::string, std::pair<std::size_t, std::size_t>> nonzeroNameCounts;
    for (const auto& font : fonts) {
        if (font->getCmper() == 0) continue;
        const auto name = musx::dom::normalizeFontName(font->name);
        if (name.empty()) continue;
        const auto target = "others.fontName[" + std::to_string(font->getCmper()) + "].name";
        auto& [total, introduced] = nonzeroNameCounts[name];
        ++total;
        if (std::string(ctx.fields.originOf(target)) != "legacy-mus") ++introduced;
    }
    std::size_t duplicateCount = 0;
    std::size_t introducedDuplicateCount = 0;
    for (const auto& [name, counts] : nonzeroNameCounts) {
        static_cast<void>(name);
        if (counts.first <= 1) continue;
        ++duplicateCount;
        if (counts.second != 0) ++introducedDuplicateCount;
    }

    out << "{\"definitions\":[";
    bool first = true;
    for (const auto& font : fonts) {
        const auto target = "others.fontName[" + std::to_string(font->getCmper()) + "].name";
        out << (first ? "" : ",") << '{'
            << "\"cmper\":" << font->getCmper()
            << ",\"name\":" << jsonString(font->name)
            << ",\"normalized_name\":" << jsonString(musx::dom::normalizeFontName(font->name))
            << ",\"charset_bank\":" << static_cast<int>(font->charsetBank)
            << ",\"charset_value\":" << font->charsetVal
            << ",\"pitch\":" << font->pitch
            << ",\"family\":" << font->family
            << ",\"origin\":" << jsonString(ctx.fields.originOf(target))
            << '}';
        first = false;
    }
    out << "],\"duplicate_nonzero_name_count\":" << duplicateCount
        << ",\"introduced_duplicate_nonzero_name_count\":" << introducedDuplicateCount
        << '}';
}

COVERAGE_SURVEYOR("font_definitions", writeFontDefinitions);

} // namespace
