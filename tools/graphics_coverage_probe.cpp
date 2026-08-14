// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Targeted private extractor for page-graphic records, embedded image signatures,
// and ShapeDef ExternalGraphic operands. Input is
// survey_id<TAB>corpus_id<TAB>source_path. Output deliberately omits paths.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "container/mus_container.h"
#include "finale_mus_reader/reader.h"
#include "musx/musx.h"
#include "records/legacy_record_index.h"
#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#endif
#include "musx/xml/PugiXmlImpl.h"

namespace {

using finale_mus_reader::ByteOrder;
using finale_mus_reader::FormatEpoch;

constexpr auto pageGraphicTag = finale_mus_reader::records::packTag("pg");
constexpr finale_mus_reader::records::LegacyTag pageGraphicClass = 0x00bc;
constexpr auto measureGraphicTag = finale_mus_reader::records::packTag("mg");
constexpr finale_mus_reader::records::LegacyTag measureGraphicClass = 0x041d;

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

const char* epochName(FormatEpoch epoch)
{
    switch (epoch) {
    case FormatEpoch::CodaBanner: return "coda-banner";
    case FormatEpoch::UncompressedLegacy: return "uncompressed";
    case FormatEpoch::DclLegacy: return "dcl";
    case FormatEpoch::ZlibLegacy: return "zlib";
    case FormatEpoch::Unknown: return "unknown";
    }
    return "unknown";
}

std::uint16_t readWord(const std::uint8_t* bytes, ByteOrder order)
{
    return order == ByteOrder::BigEndian
        ? static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[0]) << 8U) | bytes[1])
        : static_cast<std::uint16_t>(bytes[0]
            | (static_cast<std::uint16_t>(bytes[1]) << 8U));
}

std::vector<std::uint16_t> pageGraphicWords(
    const finale_mus_reader::records::LegacyRecordIndex& index,
    FormatEpoch epoch, ByteOrder order)
{
    std::vector<std::uint16_t> result;
    const auto& pool = epoch == FormatEpoch::ZlibLegacy
        ? index.getClassRecords() : index.getOthers();
    const auto identity = epoch == FormatEpoch::ZlibLegacy
        ? pageGraphicClass : pageGraphicTag;
    for (const auto cmper : pool.cmpersForTag(identity)) {
        const auto rows = pool.getArray(identity, cmper);
        if (epoch == FormatEpoch::ZlibLegacy) {
            if (rows.empty()) continue;
            const auto payload = pool.payloadOf(rows.front());
            for (std::size_t at = 0; at + 2 <= payload.size(); at += 2)
                result.push_back(readWord(payload.data() + at, order));
        } else {
            for (const auto& row : rows) {
                for (std::uint8_t word = 0; word < row.wordCount; ++word)
                    result.push_back(static_cast<std::uint16_t>(row.words[word]));
            }
        }
    }
    return result;
}

struct MeasureGraphicRecord
{
    std::uint16_t cmper1{};
    std::uint16_t cmper2{};
    std::vector<std::uint16_t> words;
};

std::vector<MeasureGraphicRecord> measureGraphicRecords(
    const finale_mus_reader::records::LegacyRecordIndex& index,
    const finale_mus_reader::container::ParsedContainer& parsed)
{
    std::vector<MeasureGraphicRecord> result;
    const auto epoch = parsed.formatEpoch;
    const auto order = parsed.byteOrder;
    if (epoch == FormatEpoch::ZlibLegacy) {
        const auto& pool = index.getClassRecords();
        for (const auto cmper1 : pool.cmpersForTag(measureGraphicClass)) {
            for (const auto cmper2 : pool.secondCmpersForTag(measureGraphicClass, cmper1)) {
                for (const auto& row : pool.getArray(measureGraphicClass, cmper1, cmper2)) {
                    const auto payload = pool.payloadOf(row);
                    MeasureGraphicRecord record{cmper1, cmper2, {}};
                    for (std::size_t at = 0; at + 2 <= payload.size(); at += 2)
                        record.words.push_back(readWord(payload.data() + at, order));
                    result.push_back(std::move(record));
                }
            }
        }
    } else {
        const auto detailBlock = epoch == FormatEpoch::DclLegacy ? 0x0010 : 0x0002;
        for (const auto& block : parsed.blocks) {
            if (block.info.type != detailBlock) continue;
            for (std::size_t at = 0; at + 16 <= block.data.size(); at += 16) {
                if (readWord(block.data.data() + at + 4, order) != measureGraphicTag) continue;
                const auto cmper1 = readWord(block.data.data() + at, order);
                const auto cmper2 = readWord(block.data.data() + at + 2, order);
                auto found = std::find_if(result.begin(), result.end(), [&](const auto& item) {
                    return item.cmper1 == cmper1 && item.cmper2 == cmper2;
                });
                if (found == result.end()) {
                    result.push_back({cmper1, cmper2, {}});
                    found = std::prev(result.end());
                }
                for (std::size_t word = 0; word < 5; ++word)
                    found->words.push_back(readWord(block.data.data() + at + 6 + word * 2, order));
            }
        }
    }
    return result;
}

struct Signature
{
    std::string_view name;
    std::span<const std::uint8_t> bytes;
};

std::vector<std::pair<std::string_view, std::size_t>> imageSignatures(
    const std::vector<std::uint8_t>& data)
{
    static constexpr std::array<std::uint8_t, 4> binaryEps{0xc5, 0xd0, 0xd3, 0xc6};
    static constexpr std::array<std::uint8_t, 8> png{0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    static constexpr std::array<std::uint8_t, 4> jpeg{0xff, 0xd8, 0xff, 0xe0};
    static constexpr std::string_view asciiEps = "%!PS-Adobe";
    const std::array signatures{
        Signature{"eps-binary", binaryEps},
        Signature{"png", png},
        Signature{"jpeg", jpeg},
        Signature{"eps-ascii", std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(asciiEps.data()), asciiEps.size())}
    };
    std::vector<std::pair<std::string_view, std::size_t>> result;
    for (const auto& signature : signatures) {
        if (signature.bytes.size() > data.size()) continue;
        for (std::size_t at = 0; at + signature.bytes.size() <= data.size(); ++at) {
            if (std::equal(signature.bytes.begin(), signature.bytes.end(), data.begin() + at))
                result.emplace_back(signature.name, at);
        }
    }
    return result;
}

struct ShapeGraphicReference
{
    int shapeCmper{};
    int graphicCmper{};
};

std::vector<ShapeGraphicReference> shapeGraphicReferences(
    const musx::dom::DocumentPtr& document)
{
    std::vector<ShapeGraphicReference> result;
    const auto others = document->getOthers();
    for (const auto& shape : others->getArray<musx::dom::others::ShapeDef>(musx::dom::SCORE_PARTID)) {
        const auto instructions = others->get<musx::dom::others::ShapeInstructionList>(
            musx::dom::SCORE_PARTID, shape->instructionList);
        const auto data = others->get<musx::dom::others::ShapeData>(
            musx::dom::SCORE_PARTID, shape->dataList);
        if (!instructions || !data) continue;
        std::size_t dataIndex = 0;
        for (const auto& instruction : instructions->instructions) {
            const auto count = static_cast<std::size_t>(instruction->numData);
            if (dataIndex + count > data->values.size()) break;
            if (instruction->type == musx::dom::ShapeDefInstructionType::ExternalGraphic) {
                const std::vector<int> values(data->values.begin() + dataIndex,
                    data->values.begin() + dataIndex + count);
                if (const auto graphic = musx::dom::ShapeDefInstruction::parseExternalGraphic(values))
                    result.push_back({shape->getCmper(), graphic->cmper});
            }
            dataIndex += count;
        }
    }
    return result;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: graphics_coverage_probe <corpus-tsv> <output-jsonl>\n");
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
            std::ifstream source(path, std::ios::binary);
            std::vector<std::uint8_t> bytes(
                (std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
            if (bytes.empty()) throw std::runtime_error("source is empty or unreadable");
            const auto parsed = finale_mus_reader::container::parse(bytes.data(), bytes.size());
            const auto index = finale_mus_reader::records::LegacyRecordIndex::build(parsed);
            const auto words = pageGraphicWords(index, parsed.formatEpoch, parsed.byteOrder);
            const auto measureRecords = measureGraphicRecords(index, parsed);
            const auto signatures = imageSignatures(bytes);
            const auto imported = finale_mus_reader::Reader::read<musx::xml::pugi::Document>(
                std::filesystem::path(path));
            if (!imported.document) throw std::runtime_error("reader import failed");
            const auto shapeReferences = shapeGraphicReferences(imported.document);
            const auto shapeAssignments = imported.document->getOthers()
                ->getArray<musx::dom::others::ShapeGraphicAssign>(musx::dom::SCORE_PARTID);
            const auto pageAssignments = imported.document->getOthers()
                ->getArray<musx::dom::others::PageGraphicAssign>(musx::dom::SCORE_PARTID);
            const auto unresolvedShapeRefs = std::count_if(
                shapeReferences.begin(), shapeReferences.end(), [&](const auto& reference) {
                    return !musx::dom::others::ShapeGraphicAssign::findForGraphic(
                        imported.document, musx::dom::SCORE_PARTID,
                        static_cast<musx::dom::Cmper>(reference.graphicCmper));
                });
            output << ",\"status\":\"ok\",\"epoch\":" << quote(epochName(parsed.formatEpoch))
                << ",\"saving_product\":" << quote(imported.report.savingProduct)
                << ",\"page_words\":[";
            for (std::size_t i = 0; i < words.size(); ++i)
                output << (i ? "," : "") << static_cast<std::int16_t>(words[i]);
            output << "],\"page_payload_remainder\":" << (words.size() % 18)
                << ",\"page_assignments\":" << pageAssignments.size()
                << ",\"measure_graphic_records\":[";
            for (std::size_t i = 0; i < measureRecords.size(); ++i) {
                const auto& record = measureRecords[i];
                output << (i ? "," : "") << "{\"cmper1\":" << record.cmper1
                    << ",\"cmper2\":" << record.cmper2 << ",\"words\":[";
                for (std::size_t j = 0; j < record.words.size(); ++j)
                    output << (j ? "," : "") << static_cast<std::int16_t>(record.words[j]);
                output << "]}";
            }
            output << "]"
                << ",\"shape_graphic_cmpers\":[";
            for (std::size_t i = 0; i < shapeReferences.size(); ++i)
                output << (i ? "," : "") << shapeReferences[i].graphicCmper;
            output << "],\"shape_graphic_references\":[";
            for (std::size_t i = 0; i < shapeReferences.size(); ++i) {
                output << (i ? "," : "") << "{\"shape_cmper\":"
                    << shapeReferences[i].shapeCmper << ",\"graphic_cmper\":"
                    << shapeReferences[i].graphicCmper << '}';
            }
            output << "],\"shape_assignment_graphic_cmpers\":[";
            for (std::size_t i = 0; i < shapeAssignments.size(); ++i)
                output << (i ? "," : "") << shapeAssignments[i]->graphicCmper;
            output << "],\"shape_graphic_assignments\":" << shapeAssignments.size()
                << ",\"unresolved_shape_graphic_refs\":" << unresolvedShapeRefs
                << ",\"embedded_graphics\":" << imported.document->getEmbeddedGraphics().size()
                << ",\"image_signatures\":[";
            for (std::size_t i = 0; i < signatures.size(); ++i) {
                output << (i ? "," : "") << "{\"kind\":" << quote(signatures[i].first)
                    << ",\"offset\":" << signatures[i].second << '}';
            }
            output << "],\"stored_blocks\":[";
            bool comma = false;
            for (const auto& block : parsed.blocks) {
                if (!block.info.stored) continue;
                output << (comma ? "," : "") << "{\"type\":" << block.info.type
                    << ",\"offset\":" << block.info.sourceOffset
                    << ",\"size\":" << block.data.size() << '}';
                comma = true;
            }
            output << "]}\n";
        } catch (const std::exception& error) {
            ++failed;
            std::fprintf(stderr, "FAILED %s: %s\n    %s\n", corpus.c_str(), error.what(), path.c_str());
            output << ",\"status\":\"error\",\"error\":" << quote(error.what()) << "}\n";
        }
    }
    std::fprintf(stderr, "read %zu documents, %zu failed\n", total, failed);
    return failed ? 1 : 0;
}
