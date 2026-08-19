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
#include "reader/document_factory.h"
#include "musx/util/Logger.h"

namespace finale_mus_reader {
namespace {

ImportResult readImpl(const std::uint8_t* data, std::size_t size,
    const std::optional<std::filesystem::path>& sourcePath,
    XmlParser parseXml, DocumentParser parseDocument)
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
        result.report.diagnostics.push_back({musx::util::Logger::LogLevel::Verbose,
            "Preserved classification after a terminal block with "
            + std::to_string(parsed.trailingByteCount) + " trailing bytes."});
    }

    result.document = createDocument(
        parsed, data, size, sourcePath, parseXml, parseDocument, result.report);

    if (parsed.formatEpoch == FormatEpoch::Unknown) {
        result.report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
            "The banner header was recovered, but the body framing was not recognized."});
    }
    // Each diagnostic goes out at its own level. Forwarding them all as warnings was what
    // made a routine fallback indistinguishable from an unreadable document.
    for (const auto& diagnostic : result.report.diagnostics) {
        musx::util::Logger::log(diagnostic.level, diagnostic.message);
    }
    return result;
}

// Runs an import and converts a thrown failure into a returned one.
//
// Error is the only level that can be asserted rather than judged: inside this catch there
// is provably no document, because the sole path that produces one has already unwound. The
// caller sees that as a null `document`, so a failed import is checked the same way a
// successful one is used, and the reason is carried in the report alongside every other
// diagnostic instead of arriving through a separate channel.
//
// Failure is returned rather than propagated because this reader's whole purpose is
// rescuing damaged and obsolete documents: a caller sweeping thousands of files should not
// have to wrap each one to keep going, and a corpus run stopping on its first bad file is
// the wrong default for that job.
template <typename Body>
ImportResult runGuarded(Body&& body)
{
    try {
        return body();
    } catch (const std::exception& error) {
        musx::util::Logger::log(musx::util::Logger::LogLevel::Error, error.what());
        ImportResult failed;
        failed.report.diagnostics.push_back(
            {musx::util::Logger::LogLevel::Error, error.what()});
        return failed;
    } catch (...) {
        constexpr auto unknown = "MUS import failed with an unrecognized exception.";
        musx::util::Logger::log(musx::util::Logger::LogLevel::Error, unknown);
        ImportResult failed;
        failed.report.diagnostics.push_back(
            {musx::util::Logger::LogLevel::Error, unknown});
        return failed;
    }
}

} // namespace

ImportResult Reader::readWithParser(
    const std::filesystem::path& path,
    XmlParser parseXml, DocumentParser parseDocument)
{
    return runGuarded([&] {
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
    return readImpl(data.data(), data.size(), path, parseXml, parseDocument);
    });
}

ImportResult Reader::readWithParser(
    const std::uint8_t* data, std::size_t size,
    XmlParser parseXml, DocumentParser parseDocument)
{
    return runGuarded(
        [&] { return readImpl(data, size, std::nullopt, parseXml, parseDocument); });
}

} // namespace finale_mus_reader
