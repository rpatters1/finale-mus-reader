// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Emit one private observation per legacy document for every class the reader currently
// recovers. Input is a TSV of `corpus_id<TAB>source_path` rows; a line whose first
// character is `#`, like a blank line, is skipped rather than parsed, so a whole corpus's
// worth of rows in a shared corpus list can be enabled or disabled as a block by
// commenting/uncommenting it. Output is JSON Lines and deliberately contains no source
// path, so it may be aggregated into tracked findings. Import failures are printed to
// stderr WITH their path, which is the survey policy's intentional console-only exception.
//
// This probe reports what the reader produced and where each value came from. It does
// not compare against a companion; that is the aggregator's job, so that the source and
// the companion are extracted independently. Coverage is what makes it useful for
// regression detection: because every class goes through the same
// coverage::runAllSurveyors() harness (see registry.h), two runs' JSON Lines -- before and
// after a change -- diff cleanly, and a class can never silently drop out of that diff the
// way a hand-maintained list of writer calls could forget one.

#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "coverage/context.h"
#include "coverage/json.h"
#include "coverage/registry.h"
#include "finale_mus_reader/reader.h"
#include "musx/musx.h"
#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#endif
#include "musx/xml/PugiXmlImpl.h"

namespace {

using LogLevel = musx::util::Logger::LogLevel;

struct Options
{
    std::string corpusListPath;
    std::string outputPath;
    // Verbose is the most permissive threshold, matching today's unfiltered default: no
    // flag means every diagnostic still prints, exactly as before this option existed.
    LogLevel minDiagnosticLevel = LogLevel::Verbose;
};

// LogLevel's declaration order (Info, Warning, Error, Verbose) is not its severity order,
// so filtering compares this rank instead of the enum value directly.
int diagnosticRank(LogLevel level)
{
    switch (level) {
    case LogLevel::Verbose: return 0;
    case LogLevel::Info: return 1;
    case LogLevel::Warning: return 2;
    case LogLevel::Error: return 3;
    }
    return 1;
}

void printUsage()
{
    std::fprintf(stderr,
        "usage: recovery_coverage_probe [--min-diagnostic-level=verbose|info|warning|error] "
        "<corpus-tsv> <output-jsonl>\n");
}

// Parses argv into Options. An unrecognized value for a recognized option is reported and
// otherwise ignored -- whatever Options already holds for it, generally its declared
// default, stands -- rather than aborting the whole run over one bad flag. Missing the
// required positional arguments still fails outright: there is no sensible default for
// "which files to read" or "where to write results," so that prints usage and returns
// std::nullopt for main() to check. Adding a future option means adding one field to
// Options and one more branch here -- nothing else in this file has to change.
std::optional<Options> parseOptions(int argc, char** argv)
{
    Options options;
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        constexpr std::string_view levelFlag = "--min-diagnostic-level=";
        if (arg.substr(0, levelFlag.size()) == levelFlag) {
            const auto value = arg.substr(levelFlag.size());
            if (value == "verbose") options.minDiagnosticLevel = LogLevel::Verbose;
            else if (value == "info") options.minDiagnosticLevel = LogLevel::Info;
            else if (value == "warning") options.minDiagnosticLevel = LogLevel::Warning;
            else if (value == "error") options.minDiagnosticLevel = LogLevel::Error;
            else {
                std::fprintf(stderr,
                    "unrecognized --min-diagnostic-level value: %.*s (ignoring)\n",
                    static_cast<int>(value.size()), value.data());
            }
        } else {
            positional.emplace_back(arg);
        }
    }
    if (positional.size() != 2) {
        printUsage();
        return std::nullopt;
    }
    options.corpusListPath = positional[0];
    options.outputPath = positional[1];
    return options;
}

} // namespace

int main(int argc, char** argv)
{
    const auto options = parseOptions(argc, argv);
    if (!options) {
        return 2;
    }
    std::ifstream list(options->corpusListPath);
    if (!list) {
        std::fprintf(stderr, "cannot open corpus list: %s\n", options->corpusListPath.c_str());
        return 2;
    }
    std::ofstream output(options->outputPath);
    if (!output) {
        std::fprintf(stderr, "cannot write output: %s\n", options->outputPath.c_str());
        return 2;
    }

    using namespace finale_mus_reader;
    using namespace finale_mus_reader::coverage;

    std::size_t total = 0;
    std::size_t failed = 0;
    std::string line;
    while (std::getline(list, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const auto corpusId = line.substr(0, tab);
        const auto path = line.substr(tab + 1);
        ++total;

        // The reader logs every diagnostic it collects (see reader.cpp) through this
        // callback as it imports. With no callback installed it defaults to bare
        // std::cerr lines with no indication of which document they came from; prefixing
        // each with its corpus_id -- never its path, per survey policy -- makes a run
        // over hundreds of files legible, and --min-diagnostic-level lets a level below
        // the threshold be dropped instead of printed.
        const auto minLevel = options->minDiagnosticLevel;
        musx::util::Logger::setCallback(
            [corpusId, minLevel](LogLevel level, const std::string& message) {
                if (diagnosticRank(level) < diagnosticRank(minLevel)) return;
                std::fprintf(stderr, "[%s] %s\n", corpusId.c_str(), message.c_str());
            });

        std::ostringstream out;
        out << '{' << "\"corpus_id\":" << jsonString(corpusId);
        try {
            const auto result = Reader::read<musx::xml::pugi::Document>(std::filesystem::path(path));
            if (!result.document) {
                throw std::runtime_error(importError(result.report));
            }
            const FieldIndex fields(result.report);
            out << ",\"status\":\"ok\""
                << ",\"epoch\":" << jsonString(epochName(result.report.formatEpoch))
                << ",\"saving_product\":" << jsonString(result.report.savingProduct)
                << ",\"source_version\":" << jsonString(versionName(result.report))
                << ",\"warning_count\":" << result.report.diagnostics.size();
            runAllSurveyors(out, SurveyContext{result.document, result.report, fields});
            out << '}';
        } catch (const std::exception& error) {
            ++failed;
            // Console-only, with the path, as the survey policy requires for failures.
            std::fprintf(stderr, "FAILED %s: %s\n    %s\n",
                corpusId.c_str(), error.what(), path.c_str());
            out << ",\"status\":\"error\""
                << ",\"error\":" << jsonString(error.what()) << '}';
        }
        output << out.str() << '\n';
    }
    std::fprintf(stderr, "read %zu documents, %zu failed\n", total, failed);
    return 0;
}
