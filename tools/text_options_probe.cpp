// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT
//
// Prints the TextOptions this reader recovers from each named file, so a fixture's
// values can be compared against its Finale 27 companion by eye.

#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>

#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#endif

#include "finale_mus_reader/reader.h"
#include "musx/musx.h"
#include "musx/xml/PugiXmlImpl.h"

namespace {

std::string spacing(const musx::dom::options::TextOptions& options)
{
    if (options.textLineSpacingPercent) {
        return std::to_string(*options.textLineSpacingPercent) + "%";
    }
    if (options.textLineSpacingEvpu) {
        return std::to_string(*options.textLineSpacingEvpu) + "evpu";
    }
    return "(none)";
}

} // namespace

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        const auto result = finale_mus_reader::Reader::read<musx::xml::pugi::Document>(
            std::filesystem::path(argv[i]));
        if (!result.document) {
            std::printf("%s: not read\n", argv[i]);
            continue;
        }
        const auto options
            = result.document->getOptions()->get<musx::dom::options::TextOptions>();
        if (!options) {
            std::printf("%s: no textOptions\n", argv[i]);
            continue;
        }
        std::printf("%s\n", std::filesystem::path(argv[i]).filename().string().c_str());
        std::printf("  spacing=%-9s tab=%d secs=%d date=%d track=%d base=%d super=%d\n",
            spacing(*options).c_str(), options->tabSpaces, int(options->showTimeSeconds),
            int(options->dateFormat), options->textTracking, options->textBaselineShift,
            options->textSuperscript);
        std::printf("  wrap=%d page=%d just=%d expand=%d horz=%d vert=%d edge=%d\n",
            int(options->textWordWrap), options->textPageOffset, int(options->textJustify),
            int(options->textExpandSingleWord), int(options->textHorzAlign),
            int(options->textVertAlign), int(options->textIsEdgeAligned));
        using Insert = musx::dom::options::AccidentalInsertSymbolType;
        static const std::pair<Insert, const char*> order[] = {
            {Insert::Sharp, "sharp"}, {Insert::Flat, "flat"}, {Insert::Natural, "natural"},
            {Insert::DblSharp, "dblSharp"}, {Insert::DblFlat, "dblFlat"}};
        for (const auto& [type, name] : order) {
            const auto found = options->symbolInserts.find(type);
            if (found == options->symbolInserts.end() || !found->second) {
                std::printf("  %-9s (missing)\n", name);
                continue;
            }
            const auto& insert = *found->second;
            std::printf("  %-9s tb=%-6d ta=%-5d bsp=%-5d font=%-3d size=%-4d efx=%-3d char=%u\n",
                name, insert.trackingBefore, insert.trackingAfter, insert.baselineShiftPerc,
                insert.symFont ? int(insert.symFont->fontId) : -1,
                insert.symFont ? insert.symFont->fontSize : -1,
                insert.symFont ? int(insert.symFont->getEnigmaStyles()) : -1,
                unsigned(insert.symChar));
        }
    }
    return 0;
}
