// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "finale_mus_reader/reader.h"

#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "container/mus_container.h"
#include "import/document_factory.h"
#include "musx/util/Logger.h"

namespace finale_mus_reader {
namespace {

ImportResult readImpl(const std::uint8_t* data, std::size_t size,
    const std::optional<std::filesystem::path>& sourcePath, XmlParser parseXml)
{
    if (!data || size == 0) {
        throw std::invalid_argument("MUS input is empty");
    }

    const auto parsed = container::parse(data, size);
    if (parsed.formatEpoch == FormatEpoch::Unknown && !hasBanner(data, size)) {
        throw std::invalid_argument("Input is not a recognized legacy Finale MUS file");
    }

    ImportResult result;
    result.report.formatEpoch = parsed.formatEpoch;
    result.report.byteOrder = parsed.byteOrder;
    result.report.sourceSize = size;
    describeSourceIdentity(data, size, result.report);
    for (const auto& block : parsed.blocks) {
        result.report.blocks.push_back(block.info);
    }
    if (parsed.trailingByteCount != 0) {
        result.report.warnings.push_back(
            "Preserved classification after a terminal block with "
            + std::to_string(parsed.trailingByteCount) + " trailing bytes.");
    }

    result.document = createDocument(
        parsed, data, size, sourcePath, parseXml, result.report);

    if (parsed.formatEpoch == FormatEpoch::PreBanner) {
        result.report.warnings.emplace_back(
            "Pre-banner pool directories are unresolved; options remain at Finale 27 defaults.");
    } else if (parsed.formatEpoch == FormatEpoch::ZlibLegacy) {
        result.report.warnings.emplace_back(
            "Later variable logical records are not overlaid yet; options remain at Finale 27 defaults.");
    } else if (parsed.formatEpoch == FormatEpoch::Unknown) {
        result.report.warnings.emplace_back(
            "The banner header was recovered, but the body framing was not recognized.");
    }
    for (const auto& warning : result.report.warnings) {
        musx::util::Logger::log(musx::util::Logger::LogLevel::Warning, warning);
    }
    return result;
}

} // namespace

ImportResult Reader::readWithParser(
    const std::filesystem::path& path, XmlParser parseXml)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        throw std::runtime_error("Unable to open MUS input: " + path.string());
    }
    const auto end = input.tellg();
    if (end <= 0) {
        throw std::invalid_argument("MUS input is empty: " + path.string());
    }
    const auto unsignedSize = static_cast<std::uintmax_t>(end);
    if (unsignedSize > (std::numeric_limits<std::size_t>::max)()
        || unsignedSize > static_cast<std::uintmax_t>(
            (std::numeric_limits<std::streamsize>::max)())) {
        throw std::length_error("MUS input is too large for this platform");
    }
    std::vector<std::uint8_t> data(static_cast<std::size_t>(unsignedSize));
    input.seekg(0);
    input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!input) {
        throw std::runtime_error("Unable to read complete MUS input: " + path.string());
    }
    return readImpl(data.data(), data.size(), path, parseXml);
}

ImportResult Reader::readWithParser(
    const std::uint8_t* data, std::size_t size, XmlParser parseXml)
{
    return readImpl(data, size, std::nullopt, parseXml);
}

} // namespace finale_mus_reader
