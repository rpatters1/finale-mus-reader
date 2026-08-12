// Extract the stored clef-definition table from a corpus of MUS files.
//
// Backs the claims in research/FORMAT_NOTES.md about where clef definitions
// live and how their physical tuple changed shape:
//
//   * pre-2001 files carry eight one-record globals selectors, 28 through 35;
//   * 2001-2006 files carry one globals selector 95 family whose incidences are
//     one continuous stream of nine-word tuples;
//   * 2007+ files carry the same stream as the single class record 0x006d;
//   * the tuple is 18 bytes until the clef character widens to 32 bits.
//
// It prints one line per file: the layout it selected, the tuple count, and
// every tuple's words, so unknown slots can be tallied and the ones that are
// never non-zero can be separated from the ones that carry real data.
//
// Reads a newline-separated list of file paths on argv[1]. Works at the record
// layer rather than through the reader, so it observes what is stored rather
// than what the importer makes of it.
//
// The path list names real files, so it is local evidence: build it from
// private/generated/, keep it out of the repository, and publish only totals.

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "container/mus_container.h"
#include "import/header/header.h"
#include "records/legacy_record_index.h"

using namespace finale_mus_reader;

namespace {

constexpr std::uint16_t globalsCmper = 65534;
constexpr std::uint16_t clefSelector = 95;
constexpr std::uint16_t clefClassId = clefSelector + 0x000e;
constexpr std::uint16_t earlyFirstSelector = 28;
constexpr std::uint16_t earlySelectorCount = 8;

std::int16_t readWord(std::span<const std::uint8_t> bytes, std::size_t offset, ByteOrder order)
{
    if (offset + 2 > bytes.size()) return 0;
    if (order == ByteOrder::BigEndian) {
        return static_cast<std::int16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8U) | bytes[offset + 1]);
    }
    return static_cast<std::int16_t>(bytes[offset] | (static_cast<std::uint16_t>(bytes[offset + 1]) << 8U));
}

// A globals selector is spelled as the two decimal characters of its number.
records::LegacyTag selectorTag(std::uint16_t selector)
{
    const char text[2] = {static_cast<char>('0' + selector / 10), static_cast<char>('0' + selector % 10)};
    return records::packTag(std::string_view(text, 2));
}

void printTuples(const std::vector<std::int16_t>& words, std::size_t wordsPerTuple)
{
    std::printf(" tuples=%zu", words.size() / wordsPerTuple);
    for (std::size_t i = 0; i + wordsPerTuple <= words.size(); i += wordsPerTuple) {
        std::printf(" [");
        for (std::size_t w = 0; w < wordsPerTuple; ++w) {
            std::printf("%s%d", w ? "," : "", words[i + w]);
        }
        std::printf("]");
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: clef_options_probe <file-list>\n");
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
        if (data.empty()) { std::printf("%s\tUNREADABLE\n", path.c_str()); continue; }
        try {
            const auto parsed = container::parse(data.data(), data.size());
            const auto index = records::LegacyRecordIndex::build(parsed);
            ImportReport report;
            header::describeSourceIdentity(data.data(), data.size(), report);
            const int major = report.sourceVersion ? report.sourceVersion->major : -1;
            std::printf("%s\tv%d ", path.c_str(), major);

            const auto* classRow = index.getClassRecords().get(clefClassId, globalsCmper, 0, 0);
            const auto selectorRows = index.getOthers().getArray(selectorTag(clefSelector), globalsCmper);
            if (classRow) {
                const auto bytes = index.getClassRecords().payloadOf(*classRow);
                // 18 bytes while the clef character is one word, 20 once it is a long.
                // A byte count alone cannot decide the stride -- 360 divides by both 18
                // and 20 -- so the observed sizes are named rather than divided blindly.
                // Anything else is reported as unexpected instead of being forced.
                const std::size_t wordsPerTuple = bytes.size() == 360 ? 10 : 9;
                std::vector<std::int16_t> words;
                for (std::size_t o = 0; o + 2 <= bytes.size(); o += 2) {
                    words.push_back(readWord(bytes, o, parsed.byteOrder));
                }
                std::printf("class0x%04x bytes=%zu", clefClassId, bytes.size());
                printTuples(words, wordsPerTuple);
            } else if (!selectorRows.empty()) {
                std::vector<std::int16_t> words;
                for (const auto& row : selectorRows) {
                    for (std::uint8_t w = 0; w < row.wordCount; ++w) words.push_back(row.words[w]);
                }
                std::printf("selector%u rows=%zu", clefSelector, selectorRows.size());
                printTuples(words, 9);
            } else {
                std::printf("early");
                std::size_t present = 0;
                for (std::uint16_t i = 0; i < earlySelectorCount; ++i) {
                    const auto rows = index.getOthers().getArray(
                        selectorTag(static_cast<std::uint16_t>(earlyFirstSelector + i)), globalsCmper);
                    if (rows.empty()) { std::printf(" [absent]"); continue; }
                    ++present;
                    std::printf(" x%zu[", rows.size());
                    for (std::uint8_t w = 0; w < rows[0].wordCount; ++w) {
                        std::printf("%s%d", w ? "," : "", rows[0].words[w]);
                    }
                    std::printf("]");
                }
                std::printf(" present=%zu", present);
            }
            std::printf("\n");
        } catch (const std::exception& e) {
            std::printf("%s\tFAILED: %s\n", path.c_str(), e.what());
        }
    }
    return 0;
}
