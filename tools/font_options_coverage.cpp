// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Extract the FontOptions tuples currently recovered by the reader, together
// with their source-side font-name referents. Input is a private TSV containing
// one `corpus_id<TAB>source_path` row per distinct source. Output is private
// JSON Lines and deliberately contains no source path.

#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <optional>
#include <ostream>
#include <set>
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

struct Tuple
{
    std::optional<finale_mus_reader::FieldInfo> fontId;
    std::optional<finale_mus_reader::FieldInfo> fontSize;
    std::optional<finale_mus_reader::FieldInfo> effects;
};

std::string jsonString(std::string_view value)
{
    std::ostringstream result;
    result << '"';
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': result << "\\\""; break;
        case '\\': result << "\\\\"; break;
        case '\b': result << "\\b"; break;
        case '\f': result << "\\f"; break;
        case '\n': result << "\\n"; break;
        case '\r': result << "\\r"; break;
        case '\t': result << "\\t"; break;
        default:
            if (ch < 0x20) {
                result << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<unsigned>(ch) << std::dec;
            } else {
                result << static_cast<char>(ch);
            }
        }
    }
    result << '"';
    return result.str();
}

std::string epochName(finale_mus_reader::FormatEpoch epoch)
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

std::string versionName(const finale_mus_reader::ImportReport& report)
{
    if (!report.sourceVersion) return {};
    const auto& value = *report.sourceVersion;
    return std::to_string(value.major) + '.' + std::to_string(value.minor)
        + '.' + std::to_string(value.maint) + '.' + std::to_string(value.build);
}

std::string byteOrderName(finale_mus_reader::ByteOrder byteOrder)
{
    switch (byteOrder) {
    case finale_mus_reader::ByteOrder::LittleEndian: return "little-endian";
    case finale_mus_reader::ByteOrder::BigEndian: return "big-endian";
    case finale_mus_reader::ByteOrder::Unknown: return "unknown";
    }
    return "unknown";
}

std::string originName(finale_mus_reader::ValueOrigin origin)
{
    switch (origin) {
    case finale_mus_reader::ValueOrigin::LegacyMus: return "legacy-mus";
    case finale_mus_reader::ValueOrigin::Finale27Default: return "finale27-default";
    }
    return "unknown";
}

std::map<std::size_t, Tuple> collectTuples(const finale_mus_reader::ImportReport& report)
{
    constexpr std::string_view prefix = "options.fontOptions[";
    std::map<std::size_t, Tuple> result;
    for (const auto& field : report.fields) {
        if (!std::string_view(field.target).starts_with(prefix)) continue;
        const auto close = field.target.find(']', prefix.size());
        if (close == std::string::npos || close + 2 > field.target.size()) continue;
        const auto ordinal = static_cast<std::size_t>(
            std::stoul(field.target.substr(prefix.size(), close - prefix.size())));
        const auto member = std::string_view(field.target).substr(close + 2);
        auto& tuple = result[ordinal];
        if (member == "fontId") tuple.fontId = field;
        else if (member == "fontSize") tuple.fontSize = field;
        else if (member == "effects") tuple.effects = field;
    }
    return result;
}

std::set<musx::dom::Cmper> collectSourceFontIds(
    const finale_mus_reader::ImportReport& report)
{
    constexpr std::string_view prefix = "others.fontName[";
    std::set<musx::dom::Cmper> result;
    for (const auto& field : report.fields) {
        if (field.origin != finale_mus_reader::ValueOrigin::LegacyMus
                || !std::string_view(field.target).starts_with(prefix)) {
            continue;
        }
        const auto close = field.target.find(']', prefix.size());
        if (close == std::string::npos) continue;
        result.insert(static_cast<musx::dom::Cmper>(
            std::stoul(field.target.substr(prefix.size(), close - prefix.size()))));
    }
    return result;
}

void writeSummary(std::ostream& output, std::string_view corpusId,
    const finale_mus_reader::ImportResult& imported,
    const std::map<std::size_t, Tuple>& tuples,
    const std::set<musx::dom::Cmper>& sourceFontIds)
{
    const auto fonts = imported.document->getOthers()
        ->getArray<musx::dom::others::FontDefinition>(musx::dom::SCORE_PARTID);
    const auto fontOptions = imported.document->getOptions()
        ->get<musx::dom::options::FontOptions>();
    std::size_t completeFieldCount = 0;
    std::size_t recoveredCount = 0;
    std::size_t defaultCount = 0;
    for (const auto& [ordinal, tuple] : tuples) {
        if (ordinal >= 45 || !tuple.fontId || !tuple.fontSize || !tuple.effects) continue;
        ++completeFieldCount;
        if (tuple.fontId->origin == finale_mus_reader::ValueOrigin::LegacyMus) {
            ++recoveredCount;
        } else {
            ++defaultCount;
        }
    }
    std::size_t danglingNonzeroCount = 0;
    if (fontOptions) {
        for (const auto& [type, font] : fontOptions->fontOptions) {
            static_cast<void>(type);
            if (font->fontId != 0
                    && !imported.document->getOthers()->get<musx::dom::others::FontDefinition>(
                    musx::dom::SCORE_PARTID, font->fontId)) {
                ++danglingNonzeroCount;
            }
        }
    }
    std::map<std::string, std::pair<std::size_t, std::size_t>> nonzeroNameCounts;
    for (const auto& font : fonts) {
        if (font->getCmper() == 0) continue;
        const auto name = musx::dom::normalizeFontName(font->name);
        if (name.empty()) continue;
        auto& [total, cloned] = nonzeroNameCounts[name];
        ++total;
        if (!sourceFontIds.contains(font->getCmper())) ++cloned;
    }
    std::size_t duplicateNonzeroNameCount = 0;
    std::size_t introducedDuplicateNonzeroNameCount = 0;
    for (const auto& [name, counts] : nonzeroNameCounts) {
        static_cast<void>(name);
        if (counts.first <= 1) continue;
        ++duplicateNonzeroNameCount;
        if (counts.second != 0) ++introducedDuplicateNonzeroNameCount;
    }
    output << "{\"kind\":\"summary\",\"corpus_id\":" << jsonString(corpusId)
        << ",\"status\":\"ok\",\"epoch\":" << jsonString(epochName(imported.report.formatEpoch))
        << ",\"byte_order\":" << jsonString(byteOrderName(imported.report.byteOrder))
        << ",\"saving_product\":" << jsonString(imported.report.savingProduct)
        << ",\"source_version\":" << jsonString(versionName(imported.report))
        << ",\"font_option_count\":" << (fontOptions ? fontOptions->fontOptions.size() : 0)
        << ",\"complete_font_option_field_count\":" << completeFieldCount
        << ",\"recovered_font_option_count\":" << recoveredCount
        << ",\"default_font_option_count\":" << defaultCount
        << ",\"font_definition_count\":" << fonts.size()
        << ",\"source_font_definition_count\":" << sourceFontIds.size()
        << ",\"cloned_font_definition_count\":" << (fonts.size() - sourceFontIds.size())
        << ",\"dangling_nonzero_font_option_count\":" << danglingNonzeroCount
        << ",\"duplicate_nonzero_font_name_count\":" << duplicateNonzeroNameCount
        << ",\"introduced_duplicate_nonzero_font_name_count\":"
        << introducedDuplicateNonzeroNameCount
        << ",\"warning_count\":" << imported.report.warnings.size()
        << "}\n";
}

void writeField(std::ostream& output, std::string_view name,
    const finale_mus_reader::FieldInfo& field, finale_mus_reader::FormatEpoch epoch,
    std::size_t ordinal, std::size_t fieldIndex)
{
    std::size_t fieldOffset = field.decodedOffset;
    if (epoch == finale_mus_reader::FormatEpoch::DclLegacy) {
        constexpr std::size_t fixedRowHeaderSize = 4;
        constexpr std::size_t wordsPerRow = 6;
        fieldOffset += fixedRowHeaderSize
            + ((ordinal * 3 + fieldIndex) % wordsPerRow) * 2;
    } else if (epoch == finale_mus_reader::FormatEpoch::ZlibLegacy) {
        constexpr std::size_t classRecordHeaderSize = 10;
        fieldOffset += classRecordHeaderSize + ordinal * 6 + fieldIndex * 2;
    }
    output << ",\"" << name << "\":" << field.rawValue
        << ",\"" << name << "_block_offset\":" << field.blockOffset
        << ",\"" << name << "_record_decoded_offset\":" << field.decodedOffset
        << ",\"" << name << "_decoded_field_offset\":" << fieldOffset;
}

void writeTuple(std::ostream& output, std::string_view corpusId, std::size_t ordinal,
    const Tuple& tuple, const musx::dom::DocumentPtr& document,
    finale_mus_reader::FormatEpoch epoch)
{
    if (!tuple.fontId || !tuple.fontSize || !tuple.effects) return;

    const auto options = document->getOptions()->get<musx::dom::options::FontOptions>();
    const auto actual = options->getFontInfo(
        static_cast<musx::dom::options::FontOptions::FontType>(ordinal));
    std::string fontStatus = "missing";
    std::string fontName;
    std::string normalizedFontName;
    const auto fontId = actual->fontId;
    if (fontId == 0) {
        fontStatus = "default";
    }
    if (const auto font = document->getOthers()
            ->get<musx::dom::others::FontDefinition>(musx::dom::SCORE_PARTID, fontId)) {
        if (fontId != 0) fontStatus = "resolved";
        fontName = font->name;
        normalizedFontName = musx::dom::normalizeFontName(fontName);
    }

    output << "{\"kind\":\"tuple\",\"corpus_id\":" << jsonString(corpusId)
        << ",\"ordinal\":" << ordinal;
    writeField(output, "source_font_id", *tuple.fontId, epoch, ordinal, 0);
    writeField(output, "source_font_size", *tuple.fontSize, epoch, ordinal, 1);
    writeField(output, "source_effects", *tuple.effects, epoch, ordinal, 2);
    output
        << ",\"font_id\":" << fontId
        << ",\"font_size\":" << actual->fontSize
        << ",\"effects\":" << actual->getEnigmaStyles()
        << ",\"font_id_origin\":" << jsonString(originName(tuple.fontId->origin))
        << ",\"font_size_origin\":" << jsonString(originName(tuple.fontSize->origin))
        << ",\"effects_origin\":" << jsonString(originName(tuple.effects->origin))
        << ",\"font_status\":" << jsonString(fontStatus)
        << ",\"font_name\":" << jsonString(fontName)
        << ",\"normalized_font_name\":" << jsonString(normalizedFontName)
        << "}\n";
}

void writeFontDefinition(std::ostream& output, std::string_view corpusId,
    const musx::dom::others::FontDefinition& font, bool fromSource)
{
    output << "{\"kind\":\"font_definition\",\"corpus_id\":" << jsonString(corpusId)
        << ",\"font_id\":" << font.getCmper()
        << ",\"font_name\":" << jsonString(font.name)
        << ",\"normalized_font_name\":" << jsonString(musx::dom::normalizeFontName(font.name))
        << ",\"origin\":" << jsonString(fromSource ? "legacy-mus" : "finale27-default")
        << ",\"charset_bank\":" << static_cast<int>(font.charsetBank)
        << ",\"charset_value\":" << font.charsetVal
        << ",\"pitch\":" << font.pitch
        << ",\"family\":" << font.family
        << "}\n";
}

void writeError(std::ostream& output, std::string_view corpusId, std::string_view error)
{
    output << "{\"kind\":\"summary\",\"corpus_id\":" << jsonString(corpusId)
        << ",\"status\":\"error\",\"error\":" << jsonString(error) << "}\n";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: font_options_coverage <private-input.tsv> <private-output.jsonl>\n");
        return 2;
    }
    musx::util::Logger::setCallback([](musx::util::Logger::LogLevel, const std::string&) {});

    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);
    if (!input || !output) {
        std::fprintf(stderr, "unable to open private input or output\n");
        return 2;
    }

    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('\t');
        if (separator == std::string::npos) continue;
        const auto corpusId = line.substr(0, separator);
        const auto path = line.substr(separator + 1);
        try {
            const auto imported = finale_mus_reader::Reader::read<musx::xml::pugi::Document>(
                std::filesystem::path(path));
            const auto tuples = collectTuples(imported.report);
            const auto sourceFontIds = collectSourceFontIds(imported.report);
            writeSummary(output, corpusId, imported, tuples, sourceFontIds);
            for (const auto& font : imported.document->getOthers()
                    ->getArray<musx::dom::others::FontDefinition>(musx::dom::SCORE_PARTID)) {
                writeFontDefinition(output, corpusId, *font,
                    sourceFontIds.contains(font->getCmper()));
            }
            for (const auto& [ordinal, tuple] : tuples) {
                writeTuple(output, corpusId, ordinal, tuple, imported.document,
                    imported.report.formatEpoch);
            }
        } catch (const std::exception& error) {
            writeError(output, corpusId, error.what());
        }
    }
    return 0;
}
