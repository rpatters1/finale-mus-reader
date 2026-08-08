// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "container/mus_container.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

#include <zlib.h>

extern "C" {
#include "blast.h"
}

#include "musx/util/Logger.h"

namespace finale_mus_reader {
namespace container {
namespace {

constexpr std::size_t bodyOffset = 0x200;
constexpr std::size_t maximumDecodedBlockSize = 64U * 1024U * 1024U;

std::uint16_t read16(const std::uint8_t* data, ByteOrder byteOrder)
{
    if (byteOrder == ByteOrder::BigEndian) {
        return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[0]) << 8U) | data[1]);
    }
    return static_cast<std::uint16_t>(data[0] | (static_cast<std::uint16_t>(data[1]) << 8U));
}

std::uint32_t read32(const std::uint8_t* data, ByteOrder byteOrder)
{
    if (byteOrder == ByteOrder::BigEndian) {
        return (static_cast<std::uint32_t>(data[0]) << 24U)
            | (static_cast<std::uint32_t>(data[1]) << 16U)
            | (static_cast<std::uint32_t>(data[2]) << 8U)
            | data[3];
    }
    return data[0]
        | (static_cast<std::uint32_t>(data[1]) << 8U)
        | (static_cast<std::uint32_t>(data[2]) << 16U)
        | (static_cast<std::uint32_t>(data[3]) << 24U);
}

struct BlastOutput
{
    std::vector<std::uint8_t> bytes;
    bool exceededLimit{};
};

unsigned noMoreBlastInput(void*, unsigned char**)
{
    return 0;
}

int appendBlastOutput(void* context, unsigned char* data, unsigned size)
{
    auto& output = *static_cast<BlastOutput*>(context);
    if (size > maximumDecodedBlockSize - output.bytes.size()) {
        output.exceededLimit = true;
        return 1;
    }
    output.bytes.insert(output.bytes.end(), data, data + size);
    return 0;
}

std::optional<std::vector<std::uint8_t>> inflateDcl(
    const std::uint8_t* data, std::size_t size)
{
    if (size > UINT_MAX) {
        return std::nullopt;
    }

    BlastOutput output;
    unsigned remaining = static_cast<unsigned>(size);
    auto* input = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(data));

    // blast 1.3 lazily initializes shared decode tables without synchronization.
    static std::mutex blastMutex;
    std::lock_guard<std::mutex> lock(blastMutex);
    const int result = blast(noMoreBlastInput, nullptr, appendBlastOutput, &output,
        &remaining, &input);
    if (result != 0 || output.exceededLimit) {
        return std::nullopt;
    }
    return output.bytes;
}

std::optional<std::vector<std::uint8_t>> inflateZlib(
    const std::uint8_t* data, std::size_t size)
{
    if (size > UINT_MAX) {
        return std::nullopt;
    }

    z_stream stream{};
    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data));
    stream.avail_in = static_cast<uInt>(size);
    if (inflateInit(&stream) != Z_OK) {
        return std::nullopt;
    }

    std::vector<std::uint8_t> output;
    std::array<std::uint8_t, 16384> chunk{};
    int result = Z_OK;
    while (result == Z_OK) {
        stream.next_out = chunk.data();
        stream.avail_out = static_cast<uInt>(chunk.size());
        result = inflate(&stream, Z_NO_FLUSH);
        const auto produced = chunk.size() - stream.avail_out;
        if (produced > maximumDecodedBlockSize - output.size()) {
            inflateEnd(&stream);
            return std::nullopt;
        }
        output.insert(output.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(produced));
    }

    const bool valid = result == Z_STREAM_END && stream.avail_in == 0;
    inflateEnd(&stream);
    return valid ? std::optional<std::vector<std::uint8_t>>(std::move(output)) : std::nullopt;
}

bool crcMatches(const std::vector<std::uint8_t>& bytes, std::uint32_t expected)
{
    uLong actual = crc32(0L, Z_NULL, 0);
    if (!bytes.empty()) {
        actual = crc32(actual, bytes.data(), static_cast<uInt>(bytes.size()));
    }
    return static_cast<std::uint32_t>(actual) == expected;
}

std::optional<ParsedContainer> tryUncompressed(
    const std::uint8_t* data, std::size_t size, ByteOrder byteOrder)
{
    if (size < bodyOffset + 6) {
        return std::nullopt;
    }

    ParsedContainer parsed;
    parsed.formatEpoch = FormatEpoch::UncompressedLegacy;
    parsed.byteOrder = byteOrder;
    std::size_t offset = bodyOffset;
    while (offset < size) {
        if (size - offset < 6) {
            return std::nullopt;
        }
        const auto type = read16(data + offset, byteOrder);
        const auto storedSize32 = read32(data + offset + 2, byteOrder);
        const std::size_t storedSize = storedSize32;
        if (storedSize < 6 || storedSize > size - offset) {
            return std::nullopt;
        }
        DecodedBlock block;
        block.info = {type, offset, storedSize, storedSize - 6, false, false};
        block.data.assign(data + offset + 6, data + offset + storedSize);
        parsed.blocks.push_back(std::move(block));
        offset += storedSize;
    }

    constexpr std::array<std::uint16_t, 4> expectedTypes{1, 2, 3, 4};
    if (parsed.blocks.size() != expectedTypes.size()) {
        return std::nullopt;
    }
    for (std::size_t index = 0; index < expectedTypes.size(); ++index) {
        if (parsed.blocks[index].info.type != expectedTypes[index]) {
            return std::nullopt;
        }
    }
    return parsed;
}

std::optional<ParsedContainer> tryCompressed(
    const std::uint8_t* data, std::size_t size, ByteOrder byteOrder, bool dcl)
{
    if (size < bodyOffset + 6) {
        return std::nullopt;
    }

    ParsedContainer parsed;
    parsed.formatEpoch = dcl ? FormatEpoch::DclLegacy : FormatEpoch::ZlibLegacy;
    parsed.byteOrder = byteOrder;
    const auto expectedFirstType = dcl ? std::uint16_t{0x000f} : std::uint16_t{0x001a};
    if (read16(data + bodyOffset, byteOrder) != expectedFirstType) {
        return std::nullopt;
    }
    std::size_t offset = bodyOffset;
    while (offset < size) {
        if (size - offset < 6) {
            return std::nullopt;
        }
        const auto type = read16(data + offset, byteOrder);
        const std::size_t storedSize = read32(data + offset + 2, byteOrder);
        if (storedSize < 6 || storedSize > size - offset) {
            return std::nullopt;
        }

        DecodedBlock block;
        block.info.type = type;
        block.info.sourceOffset = offset;
        block.info.storedSize = storedSize;
        if (storedSize == 6) {
            parsed.blocks.push_back(std::move(block));
            offset += storedSize;
            const bool isTerminal = dcl
                ? (type == 0x0012 || type == 0x0013)
                : (type == 0x0013 || type == 0x001d);
            if (isTerminal) {
                parsed.trailingByteCount = size - offset;
                return parsed;
            }
            continue;
        }
        if (storedSize < 10) {
            return std::nullopt;
        }

        const auto expectedCrc = read32(data + offset + 6, byteOrder);
        const auto compressedSize = storedSize - 10;
        auto decoded = dcl
            ? inflateDcl(data + offset + 10, compressedSize)
            : inflateZlib(data + offset + 10, compressedSize);
        if (!decoded) {
            musx::util::Logger::log(musx::util::Logger::LogLevel::Error,
                std::string(dcl ? "DCL" : "zlib") + " decompression failed at MUS offset "
                + std::to_string(offset));
            return std::nullopt;
        }
        if (!crcMatches(*decoded, expectedCrc)) {
            musx::util::Logger::log(musx::util::Logger::LogLevel::Error,
                std::string(dcl ? "DCL" : "zlib") + " checksum failed at MUS offset "
                + std::to_string(offset));
            return std::nullopt;
        }
        block.info.decodedSize = decoded->size();
        block.info.checksumPresent = true;
        block.info.checksumValid = true;
        block.data = std::move(*decoded);
        parsed.blocks.push_back(std::move(block));
        offset += storedSize;
    }

    if (parsed.blocks.empty()) {
        return std::nullopt;
    }
    return parsed;
}

bool hasBannerSignature(const std::uint8_t* data, std::size_t size)
{
    constexpr char signature[] = "ENIGMA BINARY FILE";
    return size >= sizeof(signature)
        && std::memcmp(data, signature, sizeof(signature) - 1) == 0
        && data[sizeof(signature) - 1] == 0;
}

} // namespace

ParsedContainer parse(const std::uint8_t* data, std::size_t size)
{
    if (!data && size != 0) {
        throw std::invalid_argument("Null MUS input with nonzero size");
    }

    if (hasBannerSignature(data, size)) {
        for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            if (auto parsed = tryUncompressed(data, size, byteOrder)) {
                return std::move(*parsed);
            }
        }
        for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            if (auto parsed = tryCompressed(data, size, byteOrder, true)) {
                return std::move(*parsed);
            }
        }
        for (const auto byteOrder : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            if (auto parsed = tryCompressed(data, size, byteOrder, false)) {
                return std::move(*parsed);
            }
        }
        return {};
    }

    if (size >= bodyOffset + 10
        && read32(data + 4, ByteOrder::BigEndian) == bodyOffset) {
        ParsedContainer parsed;
        parsed.formatEpoch = FormatEpoch::PreBanner;
        return parsed;
    }

    return {};
}

} // namespace container
} // namespace finale_mus_reader
