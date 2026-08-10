// Tally the FN header incidence's words across a corpus.
//
// Backs the claims in research/FORMAT_NOTES.md about the font definition
// header: that its second word carries pitch in the low nibble and family in
// the high nibble and is non-zero only for Windows-bank fonts, and that the
// remaining four header words are unused everywhere in the corpus.
//
// That last one is a universal claim, so this exists to keep testing it as the
// corpus grows: one non-zero row anywhere falsifies it.
//
// Reads a newline-separated list of file paths on argv[1]. Works at the record
// layer rather than through the reader, so it observes what is stored rather
// than what the importer makes of it.
//
// The path list names real files, so it is local evidence: build it from
// private/generated/, keep it out of the repository, and publish only totals.

#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "container/mus_container.h"
#include "records/legacy_record_index.h"

using namespace finale_mus_reader;

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: font_header_words <file-list>\n");
        return 2;
    }
    std::ifstream list(argv[1]);
    if (!list) {
        std::fprintf(stderr, "cannot open file list: %s\n", argv[1]);
        return 2;
    }

    std::map<std::uint16_t, long> word1;
    std::map<int, long> nonzeroSlot;
    std::map<std::string, std::map<std::uint16_t, long>> byBank;
    long headers = 0;
    long unreadable = 0;

    std::string path;
    while (std::getline(list, path)) {
        if (path.empty()) continue;
        std::ifstream in(path, std::ios::binary);
        std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (data.empty()) { ++unreadable; continue; }
        try {
            const auto parsed = container::parse(data.data(), data.size());
            const auto index = records::LegacyRecordIndex::build(parsed);
            const auto tag = records::packTag("FN");
            for (const auto cmper : index.getOthers().cmpersForTag(tag)) {
                const auto fam = index.getOthers().getArray(tag, cmper);
                if (fam.empty()) continue;
                const auto w0 = static_cast<std::uint16_t>(fam[0].words[0]);
                const int nibble = w0 >> 12;
                // 1 is the Mac bank and 2 the Windows bank; anything else is a
                // name row rather than the header incidence.
                if (nibble != 1 && nibble != 2) continue;
                ++headers;
                const auto w1 = static_cast<std::uint16_t>(fam[0].words[1]);
                ++word1[w1];
                byBank[nibble == 1 ? "Mac" : "Win"][w1]++;
                for (int i = 2; i < 6; ++i)
                    if (fam[0].words[i]) ++nonzeroSlot[i];
            }
        } catch (const std::exception&) {
            ++unreadable;
        }
    }

    std::printf("header incidences examined: %ld  (files skipped: %ld)\n\n", headers, unreadable);
    std::printf("word1 (pitch/family) distribution:\n");
    for (const auto& [v, n] : word1)
        std::printf("   0x%04x (%6u)  %8ld  %5.2f%%\n", v, v, n, 100.0 * double(n) / double(headers));
    std::printf("\nby bank:\n");
    for (const auto& [bank, m] : byBank) {
        std::printf("   %s:", bank.c_str());
        for (const auto& [v, n] : m) std::printf(" 0x%04x=%ld", v, n);
        std::printf("\n");
    }
    std::printf("\nnon-zero counts in words 2-5:\n");
    if (nonzeroSlot.empty()) std::printf("   none  (the unused-words claim still holds)\n");
    for (const auto& [slot, n] : nonzeroSlot)
        std::printf("   word %d: %ld   <- falsifies the unused-words claim\n", slot, n);
    return 0;
}
