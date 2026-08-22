// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "musx_companion.h"

#include <array>
#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

#include <zlib.h>

#include "score_encoder.h"

namespace finale_mus_reader::coverage {
namespace {

constexpr std::uint32_t endOfCentralDirectorySignature = 0x06054b50;
constexpr std::uint32_t centralDirectoryHeaderSignature = 0x02014b50;
constexpr std::uint32_t localFileHeaderSignature = 0x04034b50;
constexpr std::string_view scoreEntryName = "score.dat";
constexpr std::string_view notationMetadataEntryName = "NotationMetadata.xml";
constexpr std::string_view graphicsEntryPrefix = "graphics/";

std::uint16_t readU16(const std::uint8_t* data)
{
    return static_cast<std::uint16_t>(data[0] | (data[1] << 8));
}

std::uint32_t readU32(const std::uint8_t* data)
{
    return static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8)
        | (static_cast<std::uint32_t>(data[2]) << 16) | (static_cast<std::uint32_t>(data[3]) << 24);
}

std::vector<std::uint8_t> readWholeFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        throw std::runtime_error("Unable to open companion: " + path.string());
    }
    const auto size = input.tellg();
    if (size < 0) {
        throw std::runtime_error("Unable to read companion: " + path.string());
    }
    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    input.seekg(0);
    input.read(reinterpret_cast<char*>(data.data()), size);
    if (!input) {
        throw std::runtime_error("Unable to read complete companion: " + path.string());
    }
    return data;
}

// Scans backward for the end-of-central-directory record. A ZIP comment of up to 65535
// bytes can follow it, so the signature is not necessarily the file's last four bytes.
std::size_t findEndOfCentralDirectory(const std::vector<std::uint8_t>& data)
{
    constexpr std::size_t recordSize = 22;
    constexpr std::size_t maxCommentSize = 65535;
    if (data.size() < recordSize) {
        throw std::runtime_error("Companion is too small to be a ZIP archive");
    }
    const auto searchStart = data.size() >= recordSize + maxCommentSize
        ? data.size() - recordSize - maxCommentSize : 0;
    for (std::size_t at = data.size() - recordSize + 1; at-- > searchStart;) {
        if (readU32(data.data() + at) == endOfCentralDirectorySignature) {
            return at;
        }
    }
    throw std::runtime_error("Companion has no ZIP end-of-central-directory record");
}

struct DirectoryEntry
{
    std::uint16_t compressionMethod;
    std::uint32_t compressedSize;
    std::uint32_t uncompressedSize;
    std::uint32_t localHeaderOffset;
};

// One pass over the central directory, keyed by name, rather than one pass per member
// sought: score.dat, NotationMetadata.xml, and every graphics/ member are all read from a
// single companion, and nothing here needs to know in advance how many of the last kind
// there are.
std::map<std::string, DirectoryEntry> listEntries(const std::vector<std::uint8_t>& data)
{
    const auto eocd = findEndOfCentralDirectory(data);
    const auto entryCount = readU16(data.data() + eocd + 10);
    auto at = readU32(data.data() + eocd + 16);

    std::map<std::string, DirectoryEntry> entries;
    for (std::uint16_t index = 0; index < entryCount; ++index) {
        constexpr std::size_t headerSize = 46;
        if (at + headerSize > data.size()
            || readU32(data.data() + at) != centralDirectoryHeaderSignature) {
            throw std::runtime_error("Companion's ZIP central directory is malformed");
        }
        const auto method = readU16(data.data() + at + 10);
        const auto compressedSize = readU32(data.data() + at + 20);
        const auto uncompressedSize = readU32(data.data() + at + 24);
        const auto nameLength = readU16(data.data() + at + 28);
        const auto extraLength = readU16(data.data() + at + 30);
        const auto commentLength = readU16(data.data() + at + 32);
        const auto localHeaderOffset = readU32(data.data() + at + 42);
        if (at + headerSize + nameLength > data.size()) {
            throw std::runtime_error("Companion's ZIP central directory is malformed");
        }
        std::string name(
            reinterpret_cast<const char*>(data.data() + at + headerSize), nameLength);
        entries.emplace(std::move(name),
            DirectoryEntry{method, compressedSize, uncompressedSize, localHeaderOffset});
        at += headerSize + nameLength + extraLength + commentLength;
    }
    return entries;
}

std::vector<std::uint8_t> extractStoredOrDeflated(
    const std::vector<std::uint8_t>& data, const DirectoryEntry& entry, std::string_view name)
{
    constexpr std::size_t headerSize = 30;
    const auto at = entry.localHeaderOffset;
    if (at + headerSize > data.size() || readU32(data.data() + at) != localFileHeaderSignature) {
        throw std::runtime_error("Companion's " + std::string(name) + " local header is malformed");
    }
    const auto nameLength = readU16(data.data() + at + 26);
    const auto extraLength = readU16(data.data() + at + 28);
    const auto dataStart = at + headerSize + nameLength + extraLength;
    if (dataStart + entry.compressedSize > data.size()) {
        throw std::runtime_error(
            "Companion's " + std::string(name) + " data runs past the end of the file");
    }
    const auto* begin = data.data() + dataStart;

    if (entry.compressionMethod == 0) {
        return std::vector<std::uint8_t>(begin, begin + entry.compressedSize);
    }
    if (entry.compressionMethod != 8) {
        throw std::runtime_error(
            "Companion's " + std::string(name) + " uses an unsupported ZIP compression method");
    }

    z_stream stream{};
    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(begin));
    stream.avail_in = entry.compressedSize;
    // Negative windowBits selects raw deflate, the framing ZIP itself uses (no zlib header).
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
        throw std::runtime_error("Unable to start inflating companion's " + std::string(name));
    }
    std::vector<std::uint8_t> output(entry.uncompressedSize);
    stream.next_out = output.data();
    stream.avail_out = entry.uncompressedSize;
    const auto result = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    if (result != Z_STREAM_END) {
        throw std::runtime_error("Companion's " + std::string(name) + " failed to inflate");
    }
    return output;
}

std::vector<std::uint8_t> inflateGzip(const std::vector<std::uint8_t>& data)
{
    z_stream stream{};
    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data.data()));
    stream.avail_in = static_cast<uInt>(data.size());
    // 16 added to the window size selects gzip header parsing rather than a raw zlib stream.
    if (inflateInit2(&stream, MAX_WBITS + 16) != Z_OK) {
        throw std::runtime_error("Unable to start inflating companion's decoded score.dat");
    }
    std::vector<std::uint8_t> output;
    std::array<std::uint8_t, 65536> chunk{};
    int result = Z_OK;
    while (result == Z_OK) {
        stream.next_out = chunk.data();
        stream.avail_out = static_cast<uInt>(chunk.size());
        result = inflate(&stream, Z_NO_FLUSH);
        if (result != Z_OK && result != Z_STREAM_END) {
            inflateEnd(&stream);
            throw std::runtime_error("Companion's decoded score.dat failed to inflate as gzip");
        }
        const auto produced = chunk.size() - stream.avail_out;
        output.insert(output.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(produced));
    }
    inflateEnd(&stream);
    return output;
}

} // namespace

CompanionArchive readCompanionArchive(const std::filesystem::path& musxPath)
{
    const auto archive = readWholeFile(musxPath);
    const auto entries = listEntries(archive);

    const auto scoreEntry = entries.find(std::string(scoreEntryName));
    if (scoreEntry == entries.end()) {
        throw std::runtime_error("Companion's ZIP archive has no score.dat member");
    }
    auto compressed = extractStoredOrDeflated(archive, scoreEntry->second, scoreEntryName);
    musx::encoder::ScoreFileEncoder::recodeBuffer(compressed);

    CompanionArchive result;
    result.enigmaXml = inflateGzip(compressed);

    const auto metadataEntry = entries.find(std::string(notationMetadataEntryName));
    if (metadataEntry != entries.end()) {
        const auto bytes = extractStoredOrDeflated(
            archive, metadataEntry->second, notationMetadataEntryName);
        result.notationMetadata = std::vector<char>(
            reinterpret_cast<const char*>(bytes.data()),
            reinterpret_cast<const char*>(bytes.data() + bytes.size()));
    }

    for (const auto& [name, entry] : entries) {
        if (name.rfind(graphicsEntryPrefix, 0) != 0) {
            continue;
        }
        // A directory entry ("graphics/" itself, or a nested one) carries no filename after
        // the prefix, or carries another slash in what follows it -- neither is one of the
        // flat files this reader actually embeds a graphic under.
        const std::string_view filename(name.data() + graphicsEntryPrefix.size(),
            name.size() - graphicsEntryPrefix.size());
        if (filename.empty() || filename.find('/') != std::string_view::npos) {
            continue;
        }
        auto bytes = extractStoredOrDeflated(archive, entry, name);
        result.embeddedGraphics.emplace_back(std::string(filename), std::move(bytes));
    }

    return result;
}

} // namespace finale_mus_reader::coverage
