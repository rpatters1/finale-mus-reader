// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "container/mus_container.h"

#include "container/product_banner.h"

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

// The customary start of the record body, and the page size of a Coda-banner pool.
constexpr std::size_t defaultBodyOffset = 0x200;

// A banner-era header records where its record body actually begins, as a 16-bit value at
// 0x60 subject to the file's byte order. It reads 0x200 almost always, but the file-info
// strings that occupy 0x0b0 onwards can overrun that boundary and push the body later.
// Treating 0x200 as a constant leaves such a file unframed, and the failure looks like an
// unknown variant rather than a misread header.
constexpr std::size_t bodyOffsetField = 0x60;

std::size_t readBodyOffset(const std::uint8_t* data, std::size_t size, ByteOrder byteOrder);

// Defined below, beside the banner checks it belongs with; declared here because
// parseCodaBanner needs it and sits above them.
std::optional<ByteOrder> codaBannerByteOrder(const std::uint8_t* data, std::size_t size);
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

// Falls back to the customary offset when the field is absent or implausible, so a file
// that predates the field, or one whose header is truncated, behaves as it always did.
std::size_t readBodyOffset(const std::uint8_t* data, std::size_t size, ByteOrder byteOrder)
{
    if (size < bodyOffsetField + 2) {
        return defaultBodyOffset;
    }
    const std::size_t recorded = read16(data + bodyOffsetField, byteOrder);
    return recorded >= defaultBodyOffset && recorded < size ? recorded : defaultBodyOffset;
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
    const auto bodyOffset = readBodyOffset(data, size, byteOrder);
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

// Which block types actually carry a compressed member. This is an allowlist on purpose.
// Treating every non-empty block as compressed is the wrong default: a document may embed a
// graphic, and there is no reason to assume it cannot embed audio or something else later.
// An unknown type is then stored verbatim instead of aborting the whole file, which is what
// used to happen -- one embedded EPS cost a document its options, fonts, and layers, because
// the failed member discarded every good block decoded before it.
//
// The DCL era uses 0x000f through 0x0013 and the zlib era 0x0013 and 0x0016 through 0x001d.
// No type outside those ranges has been seen to decode.
bool isCompressedBlockType(std::uint16_t type, bool dcl)
{
    if (dcl) {
        // 0x0012 is compressed despite also being a terminal type number, so a rule keyed on
        // terminal types rather than on this list would discard a Finale 2006 block.
        return type >= 0x000f && type <= 0x0012;
    }
    return type == 0x0016 || type == 0x0017 || type == 0x001a || type == 0x001b;
}

std::optional<ParsedContainer> tryCompressed(
    const std::uint8_t* data, std::size_t size, ByteOrder byteOrder, bool dcl)
{
    const auto bodyOffset = readBodyOffset(data, size, byteOrder);
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
        const bool isTerminal = dcl
            ? (type == 0x0012 || type == 0x0013)
            : (type == 0x0013 || type == 0x001d);
        if (storedSize == 6) {
            parsed.blocks.push_back(std::move(block));
            offset += storedSize;
            if (isTerminal) {
                parsed.trailingByteCount = size - offset;
                return parsed;
            }
            continue;
        }
        // A block whose type is not known to be compressed is kept verbatim. It has no
        // checksum word, so its payload begins right after the six-byte header rather than
        // after ten. In practice these hold embedded graphics; the reader preserves the
        // bytes and reports them rather than interpreting them.
        if (!isCompressedBlockType(type, dcl)) {
            block.info.stored = true;
            block.info.decodedSize = storedSize - 6;
            block.data.assign(data + offset + 6, data + offset + storedSize);
            parsed.blocks.push_back(std::move(block));
            offset += storedSize;
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
            // Verbose, not Error. This runs inside a speculative attempt: parse() tries
            // both byte orders and both codecs and keeps the first that works, so a
            // rejection here is ordinary format detection and is routinely followed by a
            // successful parse of the same file. Error would say the import produced no
            // document, which this cannot know. When every attempt fails the epoch stays
            // Unknown and the reader raises that as a warning in its own right.
            musx::util::Logger::log(musx::util::Logger::LogLevel::Verbose,
                std::string(dcl ? "DCL" : "zlib") + " decompression failed at MUS offset "
                + std::to_string(offset));
            return std::nullopt;
        }
        if (!crcMatches(*decoded, expectedCrc)) {
            // Verbose for the same reason as the decompression rejection above: a wrong
            // speculative byte order can inflate to bytes that simply fail the checksum.
            musx::util::Logger::log(musx::util::Logger::LogLevel::Verbose,
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

// A Coda-banner file has no per-block framing. Its records live in a chain of pools, each
// an 8-byte prologue holding a 512-byte page count and the page size, followed by that many
// pages. The pools appear in the same order the later eras use for their typed blocks, so
// they are reported under the uncompressed-era type numbers and everything downstream can
// treat all pre-2007 epochs alike. A file of this era holds three pools; the text region that
// follows them uses different framing and is not decoded here.
ParsedContainer parseCodaBanner(const std::uint8_t* data, std::size_t size)
{
    ParsedContainer parsed;
    parsed.formatEpoch = FormatEpoch::CodaBanner;
    // The era states its platform in the banner product, and the platform gives the byte
    // order: its Windows documents say `PC` and are little-endian, its Mac documents carry a
    // bare version and are big-endian. This is read from the file rather than asserted about
    // it, which is what the note in research/FORMAT_NOTES.md was looking for.
    //
    // The pool prologue corroborates whichever order the banner selects: its page-size word
    // reads 0x200 in the right order and 0x0002 in the wrong one, and the record tags read as
    // text only in the right one. The loop below rejects a wrong order on the first pool.
    const auto byteOrder =
        codaBannerByteOrder(data, size).value_or(ByteOrder::BigEndian);
    parsed.byteOrder = byteOrder;

    std::size_t offset = defaultBodyOffset;
    std::uint16_t type = 1;
    while (offset + 8 <= size) {
        const auto pages = read32(data + offset, byteOrder);
        const auto pageSize = read32(data + offset + 4, byteOrder);
        if (pageSize != defaultBodyOffset || pages == 0) {
            break;
        }
        const auto body = offset + 8;
        const auto span = static_cast<std::size_t>(pages) * defaultBodyOffset;
        if (span > maximumDecodedBlockSize || body + span > size) {
            break;
        }

        DecodedBlock block;
        block.info.type = type;
        block.info.sourceOffset = body;
        block.info.storedSize = span + 8;
        block.data.assign(data + body, data + body + span);
        block.info.decodedSize = block.data.size();
        parsed.blocks.push_back(std::move(block));

        offset = body + span;
        ++type;
    }

    parsed.trailingByteCount = size - offset;
    return parsed;
}

bool hasBannerSignature(const std::uint8_t* data, std::size_t size)
{
    constexpr char signature[] = "ENIGMA BINARY FILE";
    return size >= sizeof(signature)
        && std::memcmp(data, signature, sizeof(signature) - 1) == 0
        && data[sizeof(signature) - 1] == 0;
}

// Files older than the signature open with a plain-text product banner instead, such as
// `Finale(TM) 2.6 Copyright 1987 by Coda.` or Finale 1.0.0's `Finale<0xAA> 1.0.0 ENIGA
// Structures ...`. A signature-bearing file always spells it `Finale(R)`, so a pre-signature
// spelling at offset 0 identifies the era on its own.
//
// The spellings themselves live in banner::parse, which is the only place any of them is
// recognized.
// The byte order a Coda-banner file is written in, or nothing when it is not one of these.
//
// This era states its platform in the banner product and its order follows from that: a `PC`
// product is a Windows document and little-endian, and every other product of the era is
// big-endian. A numeric product is the Mac form, so requiring a number here would reject every
// Windows document of the era outright.
//
// One function, so that the era's order is decided in one place.
std::optional<ByteOrder> codaBannerByteOrder(const std::uint8_t* data, std::size_t size)
{
    const auto parsed = banner::parse(data, size);
    if (!parsed.isPreSignature() || parsed.offset != 0) {
        return std::nullopt;
    }
    if (parsed.hasPcProduct()) {
        return ByteOrder::LittleEndian;
    }
    if (parsed.hasNumericProduct()) {
        return ByteOrder::BigEndian;
    }
    return std::nullopt;
}

// Whether a Coda-banner body begins where one should, read in the order its banner implies.
// The first pool's prologue is a page count followed by the page size, and that page size is
// what confirms the era: it reads 0x200 in the right order and 0x0002 in the wrong one.
bool hasCodaBannerBody(const std::uint8_t* data, std::size_t size, ByteOrder byteOrder)
{
    return size >= defaultBodyOffset + 10
        && read32(data + defaultBodyOffset + 4, byteOrder) == defaultBodyOffset;
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

    // The body of a Coda-banner file opens with a count followed by the body offset
    // itself. That second word, not anything at the top of the file, is what confirms the era.
    // A file that satisfies neither check is not a Coda-banner document at all -- an
    // AppleDouble metadata artifact, for instance.
    if (const auto byteOrder = codaBannerByteOrder(data, size);
        byteOrder && hasCodaBannerBody(data, size, *byteOrder)) {
        return parseCodaBanner(data, size);
    }

    return {};
}

} // namespace container
} // namespace finale_mus_reader
