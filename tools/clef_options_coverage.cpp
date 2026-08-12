// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Extract the ClefOptions the reader currently recovers, so a corpus run can be compared
// against the exact Finale 27 companions. Input is a private TSV containing one
// `corpus_id<TAB>source_path` row per distinct source. Output is private JSON Lines and
// deliberately contains no source path.

#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "finale_mus_reader/reader.h"
#include "musx/musx.h"
#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#endif
#include "musx/xml/PugiXmlImpl.h"

namespace {

std::string jsonString(std::string_view value)
{
    std::string result = "\"";
    for (const auto character : value) {
        if (character == '"' || character == '\\') {
            result += '\\';
            result += character;
        } else if (static_cast<unsigned char>(character) < 0x20) {
            result += ' ';
        } else {
            result += character;
        }
    }
    result += '"';
    return result;
}

/// @brief Whether a clef definition's values came from the source or from the baseline.
const char* originOf(const finale_mus_reader::ImportReport& report, std::size_t index)
{
    const auto target = "options.clefOptions.clefDefs[" + std::to_string(index)
        + "].middleCPos";
    for (const auto& field : report.fields) {
        if (field.target == target) {
            return field.origin == finale_mus_reader::ValueOrigin::LegacyMus
                ? "source" : "default";
        }
    }
    return "absent";
}

} // namespace

// The reader returns failure rather than throwing: a null document means the import failed,
// and the reason is the Error-level diagnostic in the report.
std::string importError(const finale_mus_reader::ImportReport& report)
{
    for (const auto& entry : report.diagnostics) {
        if (entry.level == musx::util::Logger::LogLevel::Error) {
            return entry.message;
        }
    }
    return "import failed without a reported reason";
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: clef_options_coverage <corpus-tsv>\n");
        return 2;
    }
    std::ifstream list(argv[1]);
    if (!list) {
        std::fprintf(stderr, "cannot open corpus list: %s\n", argv[1]);
        return 2;
    }

    std::string line;
    while (std::getline(list, line)) {
        if (line.empty()) continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const auto corpusId = line.substr(0, tab);
        const auto path = line.substr(tab + 1);

        std::ostringstream out;
        out << '{' << "\"corpus_id\":" << jsonString(corpusId);
        try {
            const auto result =
                finale_mus_reader::Reader::read<musx::xml::pugi::Document>(
                    std::filesystem::path(path));
            if (!result.document) {
                out << ",\"error\":" << jsonString(importError(result.report)) << '}';
                std::cout << out.str() << '\n';
                continue;
            }
            const auto options =
                result.document->getOptions()->get<musx::dom::options::ClefOptions>();
            out << ",\"product\":" << jsonString(result.report.savingProduct);
            if (!options) {
                out << ",\"error\":\"no clef options\"}";
                std::cout << out.str() << '\n';
                continue;
            }
            out << ",\"defaultClef\":" << options->defaultClef
                << ",\"clefChangePercent\":" << options->clefChangePercent
                << ",\"clefChangeOffset\":" << options->clefChangeOffset
                << ",\"clefFrontSepar\":" << options->clefFrontSepar
                << ",\"clefBackSepar\":" << options->clefBackSepar
                << ",\"clefKeySepar\":" << options->clefKeySepar
                << ",\"clefTimeSepar\":" << options->clefTimeSepar
                << ",\"showClefFirstSystemOnly\":"
                << (options->showClefFirstSystemOnly ? "true" : "false")
                << ",\"clefDefs\":[";
            for (std::size_t index = 0; index < options->clefDefs.size(); ++index) {
                const auto& def = options->clefDefs[index];
                out << (index ? "," : "") << '{'
                    << "\"origin\":" << jsonString(originOf(result.report, index))
                    << ",\"adjust\":" << def->middleCPos
                    << ",\"clefChar\":" << static_cast<std::uint32_t>(def->clefChar)
                    << ",\"clefYDisp\":" << def->staffPosition
                    << ",\"baseAdjust\":" << def->baselineAdjust
                    << ",\"shapeID\":" << def->shapeId
                    << ",\"isShape\":" << (def->isShape ? "true" : "false")
                    << ",\"scaleToStaffHeight\":"
                    << (def->scaleToStaffHeight ? "true" : "false")
                    << ",\"useOwnFont\":" << (def->useOwnFont ? "true" : "false");
                if (def->useOwnFont && def->font) {
                    out << ",\"fontID\":" << def->font->fontId
                        << ",\"fontSize\":" << def->font->fontSize;
                }
                out << '}';
            }
            out << "]}";
        } catch (const std::exception& e) {
            out << ",\"error\":" << jsonString(e.what()) << '}';
        }
        std::cout << out.str() << '\n';
    }
    return 0;
}
