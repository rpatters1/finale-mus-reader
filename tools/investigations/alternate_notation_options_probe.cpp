// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Report the stored numeric-global families used by AlternateNotationOptions.
//
// Reads a newline-separated list containing either paths or corpus_id<TAB>path rows and
// prints one TSV row per source. This stays below the importer so selector presence and
// stored words can be compared independently of the recovery hypothesis under test.

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "container/mus_container.h"
#include "import/header.h"
#include "import/support/legacy_mapping.h"
#include "records/legacy_record_index.h"

using namespace finale_mus_reader;

namespace {

constexpr std::uint16_t alternateNotationProbeSelectors[] = {22, 43, 46};

void printAlternateNotationProbeWords(const GlobalSelectorWords& family)
{
    if (!family.present) {
        std::printf("absent");
        return;
    }
    for (std::size_t i = 0; i < family.words.size(); ++i) {
        std::printf("%s%d", i ? "," : "", family.words[i]);
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: alternate_notation_options_probe <file-list>\n");
        return 2;
    }
    std::ifstream list(argv[1]);
    if (!list) {
        std::fprintf(stderr, "cannot open file list: %s\n", argv[1]);
        return 2;
    }

    std::printf("corpus_id\tepoch\tsaving_product\tsource_version\tselector22\tselector43\tselector46\tselectors\n");
    std::string line;
    while (std::getline(list, line)) {
        if (line.empty() || line.front() == '#') continue;
        const auto tab = line.find('\t');
        const std::string corpusId = tab == std::string::npos ? "" : line.substr(0, tab);
        const std::string path = tab == std::string::npos ? line : line.substr(tab + 1);
        std::ifstream in(path, std::ios::binary);
        std::vector<std::uint8_t> data(
            (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (data.empty()) {
            std::printf("%s\tUNREADABLE\t\t\t\t\t\n", corpusId.c_str());
            continue;
        }
        try {
            const auto parsed = container::parse(data.data(), data.size());
            const auto index = records::LegacyRecordIndex::build(parsed);
            ImportReport report(parsed.formatEpoch);
            report.byteOrder = parsed.byteOrder;
            header::describeSourceIdentity(data.data(), data.size(), report);
            SourceProfile profile(parsed.formatEpoch);
            profile.version = report.sourceVersion;
            profile.byteOrder = parsed.byteOrder;
            profile.platform = report.sourcePlatform;

            std::printf("%s\t%d\t%s\t", corpusId.c_str(),
                static_cast<int>(parsed.formatEpoch), report.savingProduct.c_str());
            if (report.sourceVersion) {
                const auto& version = *report.sourceVersion;
                std::printf("%u.%u.%u.%u", version.major, version.minor,
                    version.maint, version.build);
            }
            for (const auto selector : alternateNotationProbeSelectors) {
                std::printf("\t");
                printAlternateNotationProbeWords(readGlobalWords(index, profile, selector));
            }
            std::printf("\t");
            bool firstSelector = true;
            for (std::uint16_t selector = 0; selector < 100; ++selector) {
                if (!readGlobalWords(index, profile, selector).present) continue;
                std::printf("%s%u", firstSelector ? "" : ",", selector);
                firstSelector = false;
            }
            std::printf("\n");
        } catch (const std::exception& e) {
            std::printf("%s\tFAILED: %s\t\t\t\t\t\n", corpusId.c_str(), e.what());
        }
    }
    return 0;
}
