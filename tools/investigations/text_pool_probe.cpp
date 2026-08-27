// Dump raw decoded container blocks, with an eye on the text pool.
//
// Works below the record layer: prints every block's type, stored and decoded
// size, then hex/ASCII for the blocks selected with --type=0xNNNN (repeatable)
// or --all. Written while locating the text pool across epochs, where the
// framing is not the generic record frame and record_dump therefore sees
// nothing.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "container/mus_container.h"

using namespace finale_mus_reader;

namespace {

std::vector<std::uint8_t> readFile(const char* path)
{
    std::ifstream stream(path, std::ios::binary);
    return std::vector<std::uint8_t>(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

void hexDump(const std::vector<std::uint8_t>& data)
{
    for (std::size_t offset = 0; offset < data.size(); offset += 16) {
        std::printf("%06zx  ", offset);
        for (std::size_t i = 0; i < 16; ++i) {
            if (offset + i < data.size()) std::printf("%02x ", data[offset + i]);
            else std::printf("   ");
            if (i == 7) std::printf(" ");
        }
        std::printf(" |");
        for (std::size_t i = 0; i < 16 && offset + i < data.size(); ++i) {
            const auto byte = data[offset + i];
            std::printf("%c", (byte >= 0x20 && byte < 0x7f) ? char(byte) : '.');
        }
        std::printf("|\n");
    }
}

const char* epochName(FormatEpoch epoch)
{
    switch (epoch) {
    case FormatEpoch::CodaBanner: return "coda";
    case FormatEpoch::UncompressedLegacy: return "uncompressed";
    case FormatEpoch::DclLegacy: return "dcl";
    case FormatEpoch::ZlibLegacy: return "zlib";
    }
    return "unknown";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: text_pool_probe <file> [--type=0xNNNN]... [--all]\n");
        return 2;
    }
    std::set<std::uint16_t> wanted;
    bool all = false;
    bool raw = false;
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--all") == 0) all = true;
        else if (std::strcmp(argv[i], "--raw") == 0) raw = true;
        else if (std::strncmp(argv[i], "--type=", 7) == 0)
            wanted.insert(static_cast<std::uint16_t>(std::strtoul(argv[i] + 7, nullptr, 0)));
    }

    const auto bytes = readFile(argv[1]);
    const auto parsed = container::parse(bytes.data(), bytes.size());
    if (raw) {
        for (const auto& block : parsed.blocks) {
            if (!wanted.count(block.info.type)) continue;
            std::fwrite(block.data.data(), 1, block.data.size(), stdout);
        }
        return 0;
    }
    std::printf("%s: epoch=%s order=%s blocks=%zu\n", argv[1], epochName(parsed.formatEpoch),
        parsed.byteOrder == ByteOrder::BigEndian ? "big"
            : parsed.byteOrder == ByteOrder::LittleEndian ? "little" : "unknown",
        parsed.blocks.size());
    for (const auto& block : parsed.blocks) {
        std::printf("  type=0x%04x source=0x%zx stored=%zu decoded=%zu%s\n", block.info.type,
            block.info.sourceOffset, block.info.storedSize, block.info.decodedSize,
            block.info.stored ? " (stored)" : "");
    }
    for (const auto& block : parsed.blocks) {
        if (!all && !wanted.count(block.info.type)) continue;
        std::printf("=== block 0x%04x (%zu bytes) ===\n", block.info.type, block.data.size());
        hexDump(block.data);
    }
    return 0;
}
