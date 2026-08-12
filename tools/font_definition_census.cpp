// Count font definitions per file, across a corpus.
//
// Backs the claim in research/FORMAT_NOTES.md that every pre-zlib file yields
// font definitions with no empty name. Reads a newline-separated list of file
// paths on argv[1] and writes one TSV row per file:
//
//     status  epoch  saving_product  fonts  named
//
// A file that fails to read reports ERR rather than aborting the run, because a
// corpus always contains something unreadable and the point is the rate.
//
// The path list names real files, so it is local evidence: build it from
// private/generated/, keep it out of the repository, and publish only totals.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "finale_mus_reader/reader.h"
#include "musx/musx.h"
#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#endif
#include "musx/xml/PugiXmlImpl.h"

// The reader returns failure rather than throwing: a null document means the import failed,
// and the reason is the Error-level diagnostic in the report.
std::string importError(const finale_mus_reader::ImportReport& report)
{
    for (const auto& entry : report.diagnostics) {
        if (entry.level == musx::util::Logger::LogLevel::Error) {
            return entry.message;
        }
    }
    return "import failed without a reported reason";
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: font_definition_census <file-list>\n");
        return 2;
    }
    musx::util::Logger::setCallback([](musx::util::Logger::LogLevel, const std::string&) {});

    std::ifstream list(argv[1]);
    if (!list) {
        std::fprintf(stderr, "cannot open file list: %s\n", argv[1]);
        return 2;
    }

    std::printf("status\tepoch\tsaving_product\tfonts\tnamed\n");
    std::string path;
    while (std::getline(list, path)) {
        if (path.empty()) continue;
        try {
            const auto r = finale_mus_reader::Reader::read<musx::xml::pugi::Document>(
                std::filesystem::path(path));
            if (!r.document) {
                throw std::runtime_error(importError(r.report));
            }
            const auto fonts = r.document->getOthers()
                ->getArray<musx::dom::others::FontDefinition>(musx::dom::SCORE_PARTID);
            std::size_t named = 0;
            for (const auto& f : fonts) if (!f->name.empty()) ++named;
            std::printf("OK\t%d\t%s\t%zu\t%zu\n", static_cast<int>(r.report.formatEpoch),
                r.report.savingProduct.c_str(), fonts.size(), named);
        } catch (const std::exception&) {
            std::printf("ERR\t-1\t?\t0\t0\n");
        }
    }
    return 0;
}
