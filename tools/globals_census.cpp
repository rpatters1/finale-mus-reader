// Census the globals (options) families of one or many MUS files.
//
// Every legacy option lives in a record whose comparator is the globals
// comparator, so the shape of that pool -- which identities exist, how many
// incidences each carries, and how many payload bytes that adds up to -- is the
// first thing to know when locating an option whose selector is unknown or
// whose collection changed size between versions.
//
// Reads a newline-separated list of file paths on argv[1] and prints one line
// per file listing `identity:incidences:bytes` for every globals family, so the
// output can be diffed or grouped by saving product.
//
// The path list names real files, so it is local evidence: build it from
// private/generated/, keep it out of the repository, and publish only totals.

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "container/mus_container.h"
#include "records/legacy_record_index.h"

using namespace finale_mus_reader;

namespace {

constexpr std::uint16_t globalsCmper = 65534;

void censusPool(const char* poolName, const records::LegacyRowPool& pool)
{
    for (std::uint32_t identity = 0; identity <= 0xffffU; ++identity) {
        const auto tag = static_cast<records::LegacyTag>(identity);
        const auto rows = pool.getArray(tag, globalsCmper);
        if (rows.empty()) continue;
        std::size_t bytes = 0;
        for (const auto& row : rows) bytes += row.payloadSize;
        // Print both spellings: a pre-2007 identity reads as two characters, a
        // 2007+ class id reads as a number, and neither alone is unambiguous.
        std::printf(" %s/%s(0x%04x)x%zu=%zub", poolName,
            records::tagText(tag).c_str(), tag, rows.size(), bytes);
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: globals_census <file-list>\n");
        return 2;
    }
    std::ifstream list(argv[1]);
    if (!list) {
        std::fprintf(stderr, "cannot open file list: %s\n", argv[1]);
        return 2;
    }

    std::string path;
    while (std::getline(list, path)) {
        if (path.empty()) continue;
        std::ifstream in(path, std::ios::binary);
        std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (data.empty()) {
            std::printf("%s\tUNREADABLE\n", path.c_str());
            continue;
        }
        try {
            const auto parsed = container::parse(data.data(), data.size());
            const auto index = records::LegacyRecordIndex::build(parsed);
            std::printf("%s\t", path.c_str());
            censusPool("o", index.getOthers());
            censusPool("d", index.getDetails());
            censusPool("c", index.getClassOthers());
            std::printf("\n");
        } catch (const std::exception& e) {
            std::printf("%s\tFAILED: %s\n", path.c_str(), e.what());
        }
    }
    return 0;
}
