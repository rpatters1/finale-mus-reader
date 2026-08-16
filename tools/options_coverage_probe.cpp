// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Emit one private observation per legacy document for the option classes that recovery
// currently covers: options::ClefOptions, options::FontOptions,
// options::MultimeasureRestOptions, and the others::FontDefinition pool the first two
// reference.
//
// Input is a TSV of `corpus_id<TAB>source_path` rows. Output is JSON Lines and
// deliberately contains no source path, so it may be aggregated into tracked findings.
// Import failures are printed to stderr WITH their path, which is the survey policy's
// intentional console-only exception.
//
// This probe reports what the reader produced and where each value came from. It does
// not compare against a companion; that is the aggregator's job, so that the source and
// the companion are extracted independently.

#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
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

using finale_mus_reader::ValueOrigin;

std::string jsonString(std::string_view value)
{
    std::string result = "\"";
    for (const char ch : value) {
        switch (ch) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20) {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x",
                    static_cast<unsigned>(static_cast<unsigned char>(ch)));
                result += buffer;
            } else {
                result += ch;
            }
        }
    }
    return result + '"';
}

const char* originName(ValueOrigin origin)
{
    switch (origin) {
    case ValueOrigin::LegacyMus: return "legacy-mus";
    case ValueOrigin::LegacyBehavior: return "legacy-behavior";
    case ValueOrigin::Finale27Default: return "finale27-default";
    }
    return "unknown";
}

const char* epochName(finale_mus_reader::FormatEpoch epoch)
{
    switch (epoch) {
    case finale_mus_reader::FormatEpoch::CodaBanner: return "coda-banner";
    case finale_mus_reader::FormatEpoch::UncompressedLegacy: return "uncompressed";
    case finale_mus_reader::FormatEpoch::DclLegacy: return "dcl";
    case finale_mus_reader::FormatEpoch::ZlibLegacy: return "zlib";
    case finale_mus_reader::FormatEpoch::Unknown: return "unknown";
    }
    return "unknown";
}

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

// Every reported field keyed by target, so a value's origin can be looked up by the
// same name the importer used to report it.
std::map<std::string, finale_mus_reader::FieldInfo> fieldsByTarget(
    const finale_mus_reader::ImportReport& report)
{
    std::map<std::string, finale_mus_reader::FieldInfo> result;
    for (const auto& info : report.fields) {
        result.emplace(info.target, info);
    }
    return result;
}

std::string originOf(const std::map<std::string, finale_mus_reader::FieldInfo>& fields,
    const std::string& target)
{
    const auto found = fields.find(target);
    return found == fields.end() ? "absent" : originName(found->second.origin);
}

std::string versionName(const finale_mus_reader::ImportReport& report)
{
    if (!report.sourceVersion) {
        return {};
    }
    const auto& version = *report.sourceVersion;
    return std::to_string(version.major) + '.' + std::to_string(version.minor) + '.'
        + std::to_string(version.maint) + '.' + std::to_string(version.build);
}

void writeClefOptions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    const auto options = document->getOptions()->get<musx::dom::options::ClefOptions>();
    if (!options) {
        out << ",\"clef_options\":null";
        return;
    }
    out << ",\"clef_options\":{"
        << "\"default_clef\":" << options->defaultClef
        << ",\"clef_change_percent\":" << options->clefChangePercent
        << ",\"clef_change_offset\":" << options->clefChangeOffset
        << ",\"clef_front_separ\":" << options->clefFrontSepar
        << ",\"clef_back_separ\":" << options->clefBackSepar
        << ",\"clef_key_separ\":" << options->clefKeySepar
        << ",\"clef_time_separ\":" << options->clefTimeSepar
        << ",\"show_clef_first_system_only\":"
        << (options->showClefFirstSystemOnly ? "true" : "false")
        << ",\"cautionary_clef_changes\":"
        << (options->cautionaryClefChanges ? "true" : "false");

    for (const auto* member : {"defaultClef", "clefChangePercent", "clefChangeOffset",
             "clefFrontSepar", "clefBackSepar", "clefKeySepar", "clefTimeSepar",
             "showClefFirstSystemOnly", "cautionaryClefChanges"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(originOf(fields, std::string("options.clefOptions.") + member));
    }

    out << ",\"clef_defs\":[";
    for (std::size_t index = 0; index < options->clefDefs.size(); ++index) {
        const auto& def = options->clefDefs[index];
        const auto prefix = "options.clefOptions.clefDefs[" + std::to_string(index) + "].";
        std::string fontName;
        if (def->useOwnFont && def->font) {
            if (const auto font = document->getOthers()
                    ->get<musx::dom::others::FontDefinition>(
                        musx::dom::SCORE_PARTID, def->font->fontId)) {
                fontName = font->name;
            }
        }
        // A shape comparator that names no shape leaves the definition unrenderable.
        bool danglingShape = false;
        if (def->isShape && def->shapeId != 0) {
            danglingShape = !document->getOthers()->get<musx::dom::others::ShapeDef>(
                musx::dom::SCORE_PARTID, def->shapeId);
        }
        out << (index ? "," : "") << '{'
            << "\"index\":" << index
            << ",\"middle_c_pos\":" << def->middleCPos
            << ",\"clef_char\":" << static_cast<std::uint32_t>(def->clefChar)
            << ",\"staff_position\":" << def->staffPosition
            << ",\"baseline_adjust\":" << def->baselineAdjust
            << ",\"shape_id\":" << def->shapeId
            << ",\"is_shape\":" << (def->isShape ? "true" : "false")
            << ",\"scale_to_staff_height\":"
            << (def->scaleToStaffHeight ? "true" : "false")
            << ",\"use_own_font\":" << (def->useOwnFont ? "true" : "false")
            << ",\"font_id\":" << (def->font ? def->font->fontId : 0)
            << ",\"font_size\":" << (def->font ? def->font->fontSize : 0)
            << ",\"font_name\":" << jsonString(fontName)
            << ",\"dangling_shape\":" << (danglingShape ? "true" : "false")
            << ",\"origin_middleCPos\":" << jsonString(originOf(fields, prefix + "middleCPos"))
            << ",\"origin_clefChar\":" << jsonString(originOf(fields, prefix + "clefChar"))
            << ",\"origin_staffPosition\":"
            << jsonString(originOf(fields, prefix + "staffPosition"))
            << ",\"origin_baselineAdjust\":"
            << jsonString(originOf(fields, prefix + "baselineAdjust"))
            << ",\"origin_shapeId\":" << jsonString(originOf(fields, prefix + "shapeId"))
            << '}';
    }
    out << "]}";
}

void writeFontOptions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    const auto options = document->getOptions()->get<musx::dom::options::FontOptions>();
    if (!options) {
        out << ",\"font_options\":null";
        return;
    }
    out << ",\"font_options\":[";
    bool first = true;
    for (const auto& [type, font] : options->fontOptions) {
        const auto ordinal = static_cast<std::size_t>(type);
        const auto prefix = "options.fontOptions[" + std::to_string(ordinal) + "].";
        std::string fontName;
        bool dangling = false;
        if (const auto definition = document->getOthers()
                ->get<musx::dom::others::FontDefinition>(
                    musx::dom::SCORE_PARTID, font->fontId)) {
            fontName = definition->name;
        } else {
            dangling = font->fontId != 0;
        }
        // Both spellings are emitted. A comparison must normalize with musxdom's own
        // normalizer -- "EngraverTextT" and "Engraver Text T" are the same face, and
        // comparing the raw strings reports 324 false disagreements.
        out << (first ? "" : ",") << '{'
            << "\"ordinal\":" << ordinal
            << ",\"font_id\":" << font->fontId
            << ",\"font_size\":" << font->fontSize
            << ",\"font_name\":" << jsonString(fontName)
            << ",\"normalized_font_name\":"
            << jsonString(musx::dom::normalizeFontName(fontName))
            << ",\"dangling\":" << (dangling ? "true" : "false")
            << ",\"origin\":" << jsonString(originOf(fields, prefix + "fontId"))
            << '}';
        first = false;
    }
    out << ']';
}

void writeMultimeasureRestOptions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    const auto options =
        document->getOptions()->get<musx::dom::options::MultimeasureRestOptions>();
    if (!options) {
        out << ",\"mmrest_options\":null";
        return;
    }
    // A shape comparator that names no shape leaves the H-bar undrawable, and comparator zero
    // means no shape rather than a missing one.
    const bool danglingShape = options->shapeDef != 0
        && !document->getOthers()->get<musx::dom::others::ShapeDef>(
            musx::dom::SCORE_PARTID, options->shapeDef);
    out << ",\"mmrest_options\":{"
        << "\"meas_width\":" << options->measWidth
        << ",\"num_adj_y\":" << options->numAdjY
        << ",\"shape_def\":" << options->shapeDef
        << ",\"num_start\":" << options->numStart
        << ",\"use_syms_threshold\":" << options->useSymsThreshold
        << ",\"sym_spacing\":" << options->symSpacing
        << ",\"num_adj_x\":" << options->numAdjX
        << ",\"start_adjust\":" << options->startAdjust
        << ",\"end_adjust\":" << options->endAdjust
        << ",\"use_symbols\":" << (options->useSymbols ? "true" : "false")
        << ",\"no_horizontal_stretch\":"
        << (options->noHorizontalStretch ? "true" : "false")
        << ",\"auto_update_mm_rests\":"
        << (options->autoUpdateMmRests ? "true" : "false")
        << ",\"dangling_shape\":" << (danglingShape ? "true" : "false");
    for (const auto* member : {"measWidth", "numAdjY", "shapeDef", "numStart",
             "useSymsThreshold", "symSpacing", "numAdjX", "startAdjust", "endAdjust",
             "useSymbols", "noHorizontalStretch", "autoUpdateMmRests"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(
                   originOf(fields, std::string("options.multimeasureRestOptions.") + member));
    }
    out << '}';
}

void writeFontDefinitions(std::ostream& out, const musx::dom::DocumentPtr& document,
    const std::map<std::string, finale_mus_reader::FieldInfo>& fields)
{
    out << ",\"font_definitions\":[";
    bool first = true;
    for (const auto& font : document->getOthers()
            ->getArray<musx::dom::others::FontDefinition>(musx::dom::SCORE_PARTID)) {
        const auto target = "others.fontName[" + std::to_string(font->getCmper()) + "].name";
        out << (first ? "" : ",") << '{'
            << "\"cmper\":" << font->getCmper()
            << ",\"name\":" << jsonString(font->name)
            << ",\"origin\":" << jsonString(originOf(fields, target))
            << '}';
        first = false;
    }
    out << ']';
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::fprintf(stderr,
            "usage: options_coverage_probe <corpus-tsv> <output-jsonl>\n");
        return 2;
    }
    std::ifstream list(argv[1]);
    if (!list) {
        std::fprintf(stderr, "cannot open corpus list: %s\n", argv[1]);
        return 2;
    }
    std::ofstream output(argv[2]);
    if (!output) {
        std::fprintf(stderr, "cannot write output: %s\n", argv[2]);
        return 2;
    }

    std::size_t total = 0;
    std::size_t failed = 0;
    std::string line;
    while (std::getline(list, line)) {
        if (line.empty()) continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const auto corpusId = line.substr(0, tab);
        const auto path = line.substr(tab + 1);
        ++total;

        std::ostringstream out;
        out << '{' << "\"corpus_id\":" << jsonString(corpusId);
        try {
            const auto result =
                finale_mus_reader::Reader::read<musx::xml::pugi::Document>(
                    std::filesystem::path(path));
            if (!result.document) {
                throw std::runtime_error(importError(result.report));
            }
            const auto& report = result.report;
            const auto fields = fieldsByTarget(report);
            out << ",\"status\":\"ok\""
                << ",\"epoch\":" << jsonString(epochName(report.formatEpoch))
                << ",\"saving_product\":" << jsonString(report.savingProduct)
                << ",\"source_version\":" << jsonString(versionName(report))
                << ",\"warning_count\":" << report.diagnostics.size();
            writeClefOptions(out, result.document, fields);
            writeFontOptions(out, result.document, fields);
            writeMultimeasureRestOptions(out, result.document, fields);
            writeFontDefinitions(out, result.document, fields);
            out << '}';
        } catch (const std::exception& error) {
            ++failed;
            // Console-only, with the path, as the survey policy requires for failures.
            std::fprintf(stderr, "FAILED %s: %s\n    %s\n",
                corpusId.c_str(), error.what(), path.c_str());
            out << ",\"status\":\"error\""
                << ",\"error\":" << jsonString(error.what()) << '}';
        }
        output << out.str() << '\n';
    }
    std::fprintf(stderr, "read %zu documents, %zu failed\n", total, failed);
    return 0;
}
