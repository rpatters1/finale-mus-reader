// Dump the normalized record index of one MUS file.
//
// A general-purpose record-layer probe: it prints every others, details, and
// class row with its identity, comparators, incidence, words, and raw payload
// bytes, so a question about where a value lives can be answered without
// writing a new probe each time.
//
// Optional filters narrow the dump: --tag=cf (two characters), --class=0x0032,
// --cmper=65534, --pool=others|details|class.
//
// Reads one file path on argv[1]. Works at the record layer rather than through
// the reader, so it observes what is stored rather than what the importer makes
// of it.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "container/mus_container.h"
#include "records/legacy_record_index.h"

using namespace finale_mus_reader;

namespace {

struct Filters
{
    std::optional<std::uint16_t> identity;
    std::optional<std::uint16_t> cmper;
    std::string pool;
};

bool identityMatches(const Filters& filters, const records::LegacyRow& row)
{
    if (filters.identity && row.tag != *filters.identity) return false;
    if (filters.cmper && row.cmper1 != *filters.cmper) return false;
    return true;
}

void dumpPool(const char* name, const records::LegacyRowPool& pool, const Filters& filters)
{
    if (!filters.pool.empty() && filters.pool != name) return;
    std::printf("=== %s (%zu rows) ===\n", name, pool.size());
    // The pool has no public iteration, so walk the identities it reports.
    for (std::uint32_t identity = 0; identity <= 0xffffU; ++identity) {
        const auto tag = static_cast<records::LegacyTag>(identity);
        if (filters.identity && tag != *filters.identity) continue;
        for (const auto cmper : pool.cmpersForTag(tag)) {
            if (filters.cmper && cmper != *filters.cmper) continue;
            for (const auto cmper2 : pool.secondCmpersForTag(tag, cmper)) {
                const auto rows = pool.getArray(tag, cmper, cmper2);
                for (const auto& row : rows) {
                    if (!identityMatches(filters, row)) continue;
                    std::printf("%-4s 0x%04x cmper=%-6u cmper2=%-6u inci=%-4u words=[",
                        records::tagText(tag).c_str(), tag, row.cmper1, row.cmper2, row.inci);
                    for (std::uint8_t i = 0; i < row.wordCount; ++i) {
                        std::printf("%s%6d", i ? " " : "", row.words[i]);
                    }
                    std::printf("] bytes=");
                    const auto bytes = pool.payloadOf(row);
                    for (const auto byte : bytes) std::printf("%02x", byte);
                    std::printf(" block=0x%zx decoded=0x%zx\n", row.blockOffset, row.decodedOffset);
                }
            }
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr,
            "usage: record_dump <file> [--tag=XX|--class=0xNNNN] [--cmper=N] [--pool=others|details|class]\n");
        return 2;
    }

    Filters filters;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg.rfind("--tag=", 0) == 0 && arg.size() == 8) {
            filters.identity = records::packTag(arg.substr(6));
        } else if (arg.rfind("--class=", 0) == 0) {
            filters.identity = static_cast<std::uint16_t>(std::strtoul(arg.c_str() + 8, nullptr, 0));
        } else if (arg.rfind("--cmper=", 0) == 0) {
            filters.cmper = static_cast<std::uint16_t>(std::strtoul(arg.c_str() + 8, nullptr, 0));
        } else if (arg.rfind("--pool=", 0) == 0) {
            filters.pool = arg.substr(7);
        } else {
            std::fprintf(stderr, "unrecognized argument: %s\n", arg.c_str());
            return 2;
        }
    }

    std::ifstream in(argv[1], std::ios::binary);
    std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (data.empty()) {
        std::fprintf(stderr, "cannot read: %s\n", argv[1]);
        return 2;
    }

    const auto parsed = container::parse(data.data(), data.size());
    // A class-record payload is printed as raw bytes, so a caller decoding one needs the
    // order the container settled on. Guessing it from the header outside this tool is what
    // made an earlier probe read four little-endian files as big-endian.
    std::printf("epoch=%d byteOrder=%s\n", static_cast<int>(parsed.formatEpoch),
        parsed.byteOrder == ByteOrder::BigEndian ? "big"
            : parsed.byteOrder == ByteOrder::LittleEndian ? "little" : "unknown");
    const auto index = records::LegacyRecordIndex::build(parsed);
    dumpPool("others", index.getOthers(), filters);
    dumpPool("details", index.getDetails(), filters);
    dumpPool("class", index.getClassRecords(), filters);
    return 0;
}
