// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Targeted private extractor for ShapeDef, ShapeInstructionList, and ShapeData.
// Input is survey_id<TAB>corpus_id<TAB>source_path. Output omits the path; complete
// import failures print it to stderr as required by the corpus-survey policy.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "finale_mus_reader/reader.h"
#include "musx/musx.h"
#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#endif
#include "musx/xml/PugiXmlImpl.h"

namespace {

std::string quote(std::string_view text)
{
    std::string result{"\""};
    for (const char ch : text) {
        if (ch == '\\' || ch == '"') result += '\\';
        if (ch == '\n') result += "\\n";
        else if (ch != '\r') result += ch;
    }
    return result + '"';
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

std::string importError(const finale_mus_reader::ImportReport& report)
{
    for (const auto& diagnostic : report.diagnostics) {
        if (diagnostic.level == musx::util::Logger::LogLevel::Error) return diagnostic.message;
    }
    return "import failed without a reported reason";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: shape_coverage_probe <corpus-tsv> <output-jsonl>\n");
        return 2;
    }
    musx::util::Logger::setCallback([](musx::util::Logger::LogLevel, const std::string&) {});
    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    if (!input || !output) return 2;
    std::size_t total = 0;
    std::size_t failed = 0;
    std::string line;
    while (std::getline(input, line)) {
        const auto first = line.find('\t');
        const auto second = first == std::string::npos ? first : line.find('\t', first + 1);
        if (first == std::string::npos || second == std::string::npos) continue;
        const auto survey = line.substr(0, first);
        const auto corpus = line.substr(first + 1, second - first - 1);
        const auto path = line.substr(second + 1);
        ++total;
        output << "{\"survey_id\":" << quote(survey) << ",\"corpus_id\":" << quote(corpus);
        try {
            const auto result = finale_mus_reader::Reader::read<musx::xml::pugi::Document>(
                std::filesystem::path(path));
            if (!result.document) throw std::runtime_error(importError(result.report));
            const auto others = result.document->getOthers();
            const auto definitions = others->getArray<musx::dom::others::ShapeDef>(musx::dom::SCORE_PARTID);
            std::map<int, std::size_t> instructionTypes;
            std::size_t sourceDefinitions = 0;
            std::size_t blankDefinitions = 0;
            std::size_t recognizedBlankDefinitions = 0;
            std::size_t resolvedDefinitions = 0;
            std::size_t instructionCount = 0;
            std::size_t dataCount = 0;
            std::size_t insufficientData = 0;
            std::vector<std::string> insufficientIds;
            std::size_t externalGraphics = 0;
            std::size_t undocumented = 0;
            for (const auto& definition : definitions) {
                const auto prefix = "others.shapeDef[" + std::to_string(definition->getCmper())
                    + "].instructionList";
                bool source = false;
                for (const auto& field : result.report.fields) {
                    if (field.target == prefix
                            && field.origin != finale_mus_reader::ValueOrigin::Finale27Default) {
                        source = true;
                        break;
                    }
                }
                if (!source) continue;
                ++sourceDefinitions;
                if (definition->isBlank()) {
                    ++blankDefinitions;
                    if (definition->recognize() == musx::dom::KnownShapeDefType::Blank)
                        ++recognizedBlankDefinitions;
                    continue;
                }
                const auto list = others->get<musx::dom::others::ShapeInstructionList>(
                    musx::dom::SCORE_PARTID, definition->instructionList);
                const auto data = others->get<musx::dom::others::ShapeData>(
                    musx::dom::SCORE_PARTID, definition->dataList);
                if (!list || !data) continue;
                ++resolvedDefinitions;
                std::size_t required = 0;
                instructionCount += list->instructions.size();
                dataCount += data->values.size();
                for (const auto& instruction : list->instructions) {
                    required += static_cast<std::size_t>(instruction->numData);
                    const auto type = static_cast<int>(instruction->type);
                    ++instructionTypes[type];
                    if (instruction->type == musx::dom::ShapeDefInstructionType::ExternalGraphic)
                        ++externalGraphics;
                    if (instruction->type == musx::dom::ShapeDefInstructionType::Undocumented)
                        ++undocumented;
                }
                if (required > data->values.size()) {
                    ++insufficientData;
                    insufficientIds.push_back(std::to_string(definition->getCmper()) + ":"
                        + std::to_string(required) + ":" + std::to_string(data->values.size()));
                }
            }
            output << ",\"status\":\"ok\",\"epoch\":" << quote(epochName(result.report.formatEpoch))
                << ",\"saving_product\":" << quote(result.report.savingProduct)
                << ",\"source_definitions\":" << sourceDefinitions
                << ",\"blank_definitions\":" << blankDefinitions
                << ",\"recognized_blank_definitions\":" << recognizedBlankDefinitions
                << ",\"resolved_definitions\":" << resolvedDefinitions
                << ",\"instruction_count\":" << instructionCount
                << ",\"data_count\":" << dataCount
                << ",\"insufficient_data\":" << insufficientData
                << ",\"external_graphics\":" << externalGraphics
                << ",\"undocumented\":" << undocumented
                << ",\"insufficient_ids\":[";
            for (std::size_t i = 0; i < insufficientIds.size(); ++i)
                output << (i ? "," : "") << quote(insufficientIds[i]);
            output << ']'
                << ",\"instruction_types\":{";
            bool comma = false;
            for (const auto& [type, count] : instructionTypes) {
                output << (comma ? "," : "") << quote(std::to_string(type)) << ':' << count;
                comma = true;
            }
            output << "}}\n";
        } catch (const std::exception& error) {
            ++failed;
            std::fprintf(stderr, "FAILED %s: %s\n    %s\n", corpus.c_str(), error.what(), path.c_str());
            output << ",\"status\":\"error\",\"error\":" << quote(error.what()) << "}\n";
        }
    }
    std::fprintf(stderr, "read %zu documents, %zu failed\n", total, failed);
    return failed ? 1 : 0;
}
