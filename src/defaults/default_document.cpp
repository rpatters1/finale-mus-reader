// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "defaults/default_document.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <zlib.h>

#include "embedded_default.h"
#include "musx/musx.h"

#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#define FINALE_MUS_READER_UNDEFINE_MUSX_USE_PUGIXML
#endif

#include "musx/xml/PugiXmlImpl.h"

#ifdef FINALE_MUS_READER_UNDEFINE_MUSX_USE_PUGIXML
#undef MUSX_USE_PUGIXML
#undef FINALE_MUS_READER_UNDEFINE_MUSX_USE_PUGIXML
#endif

namespace finale_mus_reader {
namespace defaults {
namespace {

constexpr std::size_t expectedMacOSXmlSize = 91059;
constexpr std::size_t maximumDefaultXmlSize = 1024U * 1024U;

std::string inflateDefault()
{
    if (generated::macosDefaultGzipSize > UINT_MAX) {
        throw std::runtime_error("Embedded default gzip exceeds zlib's input limit");
    }

    z_stream stream{};
    stream.next_in = const_cast<Bytef*>(
        reinterpret_cast<const Bytef*>(generated::macosDefaultGzip));
    stream.avail_in = static_cast<uInt>(generated::macosDefaultGzipSize);
    if (inflateInit2(&stream, 15 + 16) != Z_OK) {
        throw std::runtime_error("Unable to initialize gzip inflation for the embedded default");
    }

    std::string output;
    std::array<char, 16384> chunk{};
    int result = Z_OK;
    while (result == Z_OK) {
        stream.next_out = reinterpret_cast<Bytef*>(chunk.data());
        stream.avail_out = static_cast<uInt>(chunk.size());
        result = inflate(&stream, Z_NO_FLUSH);
        const auto produced = chunk.size() - stream.avail_out;
        if (produced > maximumDefaultXmlSize - output.size()) {
            inflateEnd(&stream);
            throw std::runtime_error("Embedded default expands beyond the configured limit");
        }
        output.append(chunk.data(), produced);
    }

    const bool valid = result == Z_STREAM_END && stream.avail_in == 0;
    inflateEnd(&stream);
    if (!valid || output.size() != expectedMacOSXmlSize) {
        throw std::runtime_error("Embedded Finale 27 macOS default failed integrity validation");
    }
    return output;
}

std::string_view extractElement(std::string_view xml, std::string_view nodeName)
{
    const std::string opening = "<" + std::string(nodeName) + ">";
    const std::string closing = "</" + std::string(nodeName) + ">";
    const auto begin = xml.find(opening);
    if (begin == std::string_view::npos) {
        throw std::runtime_error("Embedded default is missing <" + std::string(nodeName) + ">");
    }
    const auto close = xml.find(closing, begin + opening.size());
    if (close == std::string_view::npos) {
        throw std::runtime_error("Embedded default has an unterminated <" + std::string(nodeName) + ">");
    }
    return xml.substr(begin, close + closing.size() - begin);
}

std::vector<std::string_view> extractLayerAttributes(std::string_view xml)
{
    const auto others = extractElement(xml, "others");
    constexpr std::string_view opening = "<layerAtts ";
    constexpr std::string_view closing = "</layerAtts>";
    std::vector<std::string_view> result;
    std::size_t offset = 0;
    while (true) {
        const auto begin = others.find(opening, offset);
        if (begin == std::string_view::npos) {
            break;
        }
        const auto close = others.find(closing, begin + opening.size());
        if (close == std::string_view::npos) {
            throw std::runtime_error("Embedded default has an unterminated <layerAtts>");
        }
        result.push_back(others.substr(begin, close + closing.size() - begin));
        offset = close + closing.size();
    }
    if (result.size() != 4) {
        throw std::runtime_error("Embedded default must contain exactly four layer attributes");
    }
    return result;
}

const std::string& optionsOnlyXml()
{
    static const std::string result = [] {
        const auto source = inflateDefault();
        const auto options = extractElement(source, "options");
        const auto layers = extractLayerAttributes(source);

        std::string xml;
        xml.reserve(options.size() + 4096);
        xml += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        xml += "<finale version=\"27.4\" xmlns=\"http://www.makemusic.com/2012/finale\">\n";
        xml += "<header><headerData/></header>\n";
        xml.append(options);
        xml += "\n<others>\n";
        for (const auto layer : layers) {
            xml.append(layer);
            xml += '\n';
        }
        xml += "</others>\n</finale>\n";
        return xml;
    }();
    return result;
}

} // namespace

std::shared_ptr<musx::dom::Document> createMacOSOptionsDocument(
    const std::optional<std::filesystem::path>& sourcePath)
{
    if (sourcePath) {
        musx::factory::DocumentFactory::CreateOptions createOptions(
            *sourcePath, {}, {});
        return musx::factory::DocumentFactory::create<musx::xml::pugi::Document>(
            optionsOnlyXml(), std::move(createOptions));
    }
    return musx::factory::DocumentFactory::create<musx::xml::pugi::Document>(
        optionsOnlyXml());
}

} // namespace defaults
} // namespace finale_mus_reader
