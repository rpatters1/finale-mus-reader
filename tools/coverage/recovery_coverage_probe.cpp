// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Emit one private observation per legacy document for every class the reader currently
// recovers. Input is a TSV of `corpus_id<TAB>source_path` rows; a line whose first
// character is `#`, like a blank line, is skipped rather than parsed, so a whole corpus's
// worth of rows in a shared corpus list can be enabled or disabled as a block by
// commenting/uncommenting it. A line with no tab instead names another corpus TSV to pull
// rows from, so the input can equally be a manifest of several corpora with the same
// comment-out-a-block convention selecting among them. A `.mus` path given directly in
// place of a corpus list surveys that one source on its own, labeled and identified by its
// own filename, without needing a one-row TSV written for it. Output is JSON Lines and
// deliberately contains no source path, so it may be aggregated into tracked findings.
// Import failures are printed to stderr with their path relative to the corpus's own root
// when one was declared for it (see the `#root:` line readCorpusRows() reads), or their
// bare filename otherwise -- never the full path, which is the survey policy's intentional
// console-only exception: enough is shown to locate the file within its corpus, and no more,
// so a terminal log or redirected run that ends up somewhere tracked leaks as little as
// possible.
//
// This probe reports what the reader produced and where each value came from. When a
// corpus declares a companion-naming convention (see the `#companion:` line
// readCorpusRows() reads) and the row's source imported successfully, it also surveys the
// matching Finale-27-written `.musx` companion -- loaded directly through musxdom's own
// DocumentFactory, not this reader. Both documents pass through the same registered
// surveyors, after which the probe aligns instances, compares leaves, classifies expected
// differences, and emits compact schema-3 counts and examples. Keeping those operations
// here gives semantic comparison access to both musxdom documents and its Enigma parser;
// the Python report only aggregates and renders the resulting classifications.
//
// On macOS, a failed row also carries a best-effort `finder_type` when the source file's
// classic Mac file type is still readable from its Finder Info (see macFinderFileType()):
// this probe reads real files from local disk, so it is not held to the byte-buffer-only
// constraint finale_mus_reader itself keeps for a WASM build.

#include <cctype>
#include <chrono>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(__APPLE__)
#include <sys/xattr.h>
#endif // defined(__APPLE__)

#include "coverage/context.h"
#include "coverage/comparison.h"
#include "coverage/json.h"
#include "coverage/registry.h"
#include "finale_mus_reader/reader.h"
#include "reader/timing.h"
#include "musx/musx.h"
#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#endif // !defined(MUSX_USE_PUGIXML)
#include "musx/factory/DocumentFactory.h"
#include "musx/xml/PugiXmlImpl.h"
#include "musx_companion.h"

namespace {

using LogLevel = musx::util::Logger::LogLevel;

struct Options
{
    std::string corpusListPath;
    std::string outputPath;
    std::string macSymbolFontsPath;
    // Verbose is the most permissive threshold, matching today's unfiltered default: no
    // flag means every diagnostic still prints, exactly as before this option existed.
    LogLevel minDiagnosticLevel = LogLevel::Verbose;
    bool showProgress = false;
    bool includeTimings = false;
};

// Printed every this many documents rather than a multiple of five or ten, so the printed
// count's last digit cycles through all ten values instead of only ever landing on 0 or 5.
constexpr std::size_t progressInterval = 29;

void writeTimingValue(std::ostream& out, double durationMs)
{
    out << std::fixed << std::setprecision(3) << durationMs;
}

void writeSurveyTimings(std::ostream& out,
    const finale_mus_reader::coverage::SurveyTimings& timings)
{
    out << "\"surveyors_ms\":";
    writeTimingValue(out, timings.durationMs);
    out << ",\"surveyors\":{";
    bool first = true;
    for (const auto& [key, durationMs] : timings.surveyors) {
        out << (first ? "" : ",") << finale_mus_reader::coverage::jsonString(key) << ':';
        writeTimingValue(out, durationMs);
        first = false;
    }
    out << '}';
}

void writeReaderPhaseTimings(std::ostream& out,
    const std::vector<finale_mus_reader::timing::Measurement>& measurements)
{
    out << "\"reader_phases\":{";
    bool first = true;
    for (const auto& measurement : measurements) {
        out << (first ? "" : ",")
            << finale_mus_reader::coverage::jsonString(
                   finale_mus_reader::timing::phaseName(measurement.phase))
            << ':';
        writeTimingValue(out, measurement.durationMs);
        first = false;
    }
    out << '}';
}

void writeReaderCounters(std::ostream& out,
    const std::vector<finale_mus_reader::timing::CounterMeasurement>& counters)
{
    out << "\"reader_counters\":{";
    bool first = true;
    for (const auto& counter : counters) {
        out << (first ? "" : ",")
            << finale_mus_reader::coverage::jsonString(
                   finale_mus_reader::timing::counterName(counter.counter))
            << ':' << counter.value;
        first = false;
    }
    out << '}';
}

void writeSurveyErrors(std::ostream& out,
    const std::map<std::string, std::string>& errors)
{
    if (errors.empty()) return;
    out << ",\"survey_errors\":{";
    bool first = true;
    for (const auto& [surveyor, message] : errors) {
        out << (first ? "" : ",") << finale_mus_reader::coverage::jsonString(surveyor)
            << ':' << finale_mus_reader::coverage::jsonString(message);
        first = false;
    }
    out << '}';
}

void writeContainerAttempts(std::ostream& out,
    const std::vector<finale_mus_reader::timing::ContainerAttemptMeasurement>& attempts)
{
    out << "\"container_attempts\":[";
    bool first = true;
    for (const auto& attempt : attempts) {
        out << (first ? "" : ",") << '{'
            << "\"candidate\":" << finale_mus_reader::coverage::jsonString(
                   finale_mus_reader::timing::containerCandidateName(attempt.candidate))
            << ",\"byte_order\":\"" << (attempt.bigEndian ? "big" : "little") << '"'
            << ",\"result\":" << finale_mus_reader::coverage::jsonString(
                   finale_mus_reader::timing::containerAttemptResultName(attempt.result))
            << ",\"duration_ms\":";
        writeTimingValue(out, attempt.durationMs);
        out << ",\"decompression_calls\":" << attempt.decompressionCalls
            << ",\"successful_blocks\":" << attempt.decompressedBlocks
            << ",\"compressed_input_bytes\":" << attempt.compressedInputBytes
            << ",\"decompressed_bytes\":" << attempt.decompressedBytes;
        if (attempt.decompressedBlocks != 0) {
            out << ",\"disposition\":\""
                << (attempt.result == finale_mus_reader::timing::ContainerAttemptResult::Accepted
                        ? "retained" : "discarded")
                << '"';
        }
        out << '}';
        first = false;
    }
    out << ']';
}

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
        "usage: recovery_coverage_probe [-h|--help] "
        "[--min-diagnostic-level=verbose|info|warning|error] "
        "[--mac-symbol-fonts=path] "
        "[--include-timings] [--progress] <corpus-tsv> <output-jsonl>\n");
}

// Keep this in sync with Options and parseOptions(): every flag accepted there must be
// listed here too. Nothing checks that automatically, so an option added to one and not
// the other silently drifts -- it still works, it just stops being discoverable.
void printHelp()
{
    std::fprintf(stdout,
        "recovery_coverage_probe -- survey what the reader recovers across a corpus\n"
        "\n"
        "usage: recovery_coverage_probe [options] <corpus-tsv> <output-jsonl>\n"
        "\n"
        "arguments:\n"
        "  <corpus-tsv>    TSV of corpus_id<TAB>source_path rows, one per document. A\n"
        "                  blank line or one starting with '#' is skipped, so a whole\n"
        "                  corpus can be commented out as a block. A line with no tab\n"
        "                  instead names another corpus TSV to pull rows from, so this\n"
        "                  can equally be a manifest selecting among several corpora. A\n"
        "                  '#root:' line declares that corpus's root directory, used to\n"
        "                  shorten a FAILED line's path; it is never printed itself. A\n"
        "                  '#companion:' line, formatted '#companion: <dir-name> <suffix>',\n"
        "                  declares one of that corpus's companion-naming conventions: a\n"
        "                  source dir/name.mus pairs with dir/<dir-name>/name<suffix>.\n"
        "                  Repeatable, for a corpus whose convention changed over time --\n"
        "                  each row tries them in the order declared and uses the first\n"
        "                  whose file actually exists. When one is found and the source\n"
        "                  imports, that companion is surveyed too and nested under\n"
        "                  \"companion\" in the same row.\n"
        "                  A path ending in .mus, or with no extension at all, is read\n"
        "                  directly as a single source instead of a corpus list.\n"
        "  <output-jsonl>  Path to write one JSON object per document.\n"
        "\n"
        "options:\n"
        "  --min-diagnostic-level=verbose|info|warning|error\n"
        "                  Drop reader diagnostics below this level on stderr instead\n"
        "                  of printing them. Default: verbose (nothing is dropped).\n"
        "  --mac-symbol-fonts=path\n"
        "                  Read Finale's MacSymbolFonts.txt from path and supply its\n"
        "                  contents to the reader for symbol-glyph decoding.\n"
        "  --include-timings\n"
        "                  Include detailed reader, container, and surveyor timings in\n"
        "                  each JSON row. They are omitted by default.\n"
        "  --progress      Print \"Processed X of T\" to stdout, updated in place, as\n"
        "                  documents are read.\n"
        "  -h, --help      Print this help and exit.\n");
}

// Parses argv into Options. An unrecognized value for a recognized option is reported and
// otherwise ignored -- whatever Options already holds for it, generally its declared
// default, stands -- rather than aborting the whole run over one bad flag. Missing the
// required positional arguments still fails outright: there is no sensible default for
// "which files to read" or "where to write results," so that prints usage and returns
// std::nullopt for main() to check. Adding a future option means adding one field to
// Options, one more branch here, and a line in printHelp() -- nothing else in this file
// has to change.
std::optional<Options> parseOptions(int argc, char** argv)
{
    Options options;
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        constexpr std::string_view levelFlag = "--min-diagnostic-level=";
        constexpr std::string_view symbolFontsFlag = "--mac-symbol-fonts=";
        if (arg == "--progress") {
            options.showProgress = true;
        } else if (arg == "--include-timings") {
            options.includeTimings = true;
        } else if (arg.substr(0, levelFlag.size()) == levelFlag) {
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
        } else if (arg.substr(0, symbolFontsFlag.size()) == symbolFontsFlag) {
            options.macSymbolFontsPath = std::string(arg.substr(symbolFontsFlag.size()));
            if (options.macSymbolFontsPath.empty()) {
                std::fprintf(stderr, "--mac-symbol-fonts requires a path\n");
                return std::nullopt;
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

struct CorpusRow
{
    std::string corpusId;
    std::string path;
    // The corpus TSV this row came from (its filename without extension), so a manifest run
    // can announce when processing moves from one corpus into the next.
    std::string corpusLabel;
};

// One of a corpus's companion-naming conventions (see private/corpora/*.conf's
// HAS_EXPORTS, EXPORT_DIR_NAME, EXPORT_SUFFIX, and their _FALLBACK counterparts): a source
// `dir/name.mus` pairs with `dir/companionDir/name<companionSuffix>`, or with nothing if
// that file does not exist. A corpus can declare more than one -- see the (repeatable)
// `#companion:` lines readCorpusRows() reads -- tried in the order declared, for a corpus
// whose convention changed partway through (rpatters1-main moved from `-exports`/
// `.fin27.musx` to `-finale27`/`.musx` for its more recent re-saves; a source with neither
// simply has no companion). Declared once per corpus rather than per row, the same
// reasoning `#root:` follows: it is a fixed property of the corpus, and a field repeated on
// every row is one more thing to keep in sync with the source of truth.
struct CompanionConvention
{
    std::string dirName;
    std::string suffix;
};

// Recognizes a single legacy MUS source given directly as the <corpus-tsv> argument, matched
// by extension alone since a .mus file is binary and can't be told apart from a corpus list
// by trying to parse it as one. An empty extension counts too: classic Mac Finale kept the
// file type in the resource fork, so pre-OS X documents and archive members routinely carry
// no extension at all.
bool isSingleMusFile(const std::filesystem::path& path)
{
    auto extension = path.extension().string();
    for (auto& ch : extension) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return extension == ".mus" || extension.empty();
}

// Reads every row up front rather than streaming line by line, so the total document count
// is known before processing starts; progress reporting needs that total to print "of T".
//
// A `#root:` line declares this corpus's root directory, recorded under `label` in `roots`;
// it is not printed anywhere, only used to shorten a FAILED line's path down to one relative
// to it (see displayPathFor()), so a corpus TSV -- itself private, under private/generated/
// -- is the one place that root needs to be written down at all.
//
// A `#companion:` line declares one of that corpus's companion-naming conventions, as
// "<dir-name> <suffix>", appended to the list recorded under `label` in `companions`; see
// CompanionConvention. Repeatable: a corpus with more than one such line gets a fallback
// chain, tried in the order the lines appear.
//
// Any other line with no tab is not a corpus_id/path row either: it names another corpus
// TSV to pull rows from, opened relative to the current working directory exactly as the
// top-level <corpus-tsv> argument is. That makes a manifest of several corpora just another
// corpus list, so the same blank-line/'#' comment-out-a-block convention selects among
// corpora here the same way it selects among rows within one of them. A referenced file
// that can't be opened is reported and skipped rather than aborting the run, since a
// manifest commonly names corpora that are regenerated independently of it and may not all
// exist yet. Rows pulled from a referenced file are labeled with its own name rather than
// the caller's, so nested corpora stay distinguishable from whichever manifest included
// them, and its own `#root:`/`#companion:` lines (if any) are recorded under that same label.
std::vector<CorpusRow> readCorpusRows(std::istream& list, const std::string& label,
    std::map<std::string, std::filesystem::path>& roots,
    std::map<std::string, std::vector<CompanionConvention>>& companions)
{
    constexpr std::string_view rootDirective = "#root:";
    constexpr std::string_view companionDirective = "#companion:";
    std::vector<CorpusRow> rows;
    std::string line;
    while (std::getline(list, line)) {
        if (line.empty()) continue;
        if (line.substr(0, rootDirective.size()) == rootDirective) {
            auto value = line.substr(rootDirective.size());
            while (!value.empty() && value.front() == ' ') {
                value.erase(value.begin());
            }
            roots[label] = value;
            continue;
        }
        if (line.substr(0, companionDirective.size()) == companionDirective) {
            auto value = line.substr(companionDirective.size());
            while (!value.empty() && value.front() == ' ') {
                value.erase(value.begin());
            }
            const auto space = value.find(' ');
            if (space != std::string::npos) {
                companions[label].push_back({value.substr(0, space), value.substr(space + 1)});
            } else {
                std::fprintf(stderr,
                    "malformed #companion: line for %s (want \"<dir-name> <suffix>\")\n",
                    label.c_str());
            }
            continue;
        }
        if (line[0] == '#') continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos) {
            std::ifstream nested(line);
            if (!nested) {
                std::fprintf(stderr, "cannot open referenced corpus list: %s\n", line.c_str());
                continue;
            }
            const auto nestedLabel = std::filesystem::path(line).stem().string();
            const auto nestedRows = readCorpusRows(nested, nestedLabel, roots, companions);
            rows.insert(rows.end(), nestedRows.begin(), nestedRows.end());
            continue;
        }
        rows.push_back({line.substr(0, tab), line.substr(tab + 1), label});
    }
    return rows;
}

struct CorpusSegment
{
    std::string label;
    std::size_t count;
    // From that corpus's `#root:` line, or empty when it did not declare one.
    std::filesystem::path root;
    // From that corpus's `#companion:` line(s), in the order declared; empty when it did
    // not declare any -- in which case no row in this segment is compared against a
    // companion at all.
    std::vector<CompanionConvention> companions;
};

// Rows pulled from the same corpus are always contiguous -- readCorpusRows() appends one
// referenced file's rows as one block -- so grouping consecutive equal labels recovers the
// per-corpus boundaries and counts without readCorpusRows() having to track them itself.
std::vector<CorpusSegment> segmentByCorpus(const std::vector<CorpusRow>& rows,
    const std::map<std::string, std::filesystem::path>& roots,
    const std::map<std::string, std::vector<CompanionConvention>>& companions)
{
    std::vector<CorpusSegment> segments;
    for (const auto& row : rows) {
        if (!segments.empty() && segments.back().label == row.corpusLabel) {
            ++segments.back().count;
        } else {
            const auto foundRoot = roots.find(row.corpusLabel);
            const auto foundCompanion = companions.find(row.corpusLabel);
            segments.push_back({row.corpusLabel, 1,
                foundRoot != roots.end() ? foundRoot->second : std::filesystem::path{},
                foundCompanion != companions.end() ? foundCompanion->second
                    : std::vector<CompanionConvention>{}});
        }
    }
    return segments;
}

// The path a FAILED line should show for `path`: relative to `root` when one is known and
// the file actually falls under it, the bare filename otherwise. Falling back rather than
// printing a `relative()` result that starts with `..` keeps a corpus whose declared root
// does not actually cover every row from leaking structure above that root by accident.
std::string displayPathFor(const std::filesystem::path& path, const std::filesystem::path& root)
{
    if (!root.empty()) {
        std::error_code error;
        const auto relative = std::filesystem::relative(path, root, error);
        if (!error && !relative.empty() && *relative.begin() != "..") {
            return relative.string();
        }
    }
    return path.filename().string();
}

// The companion path a source pairs with, tried under each of `conventions` in order (see
// CompanionConvention) and matching the rule scripts/inventory.py uses to build the
// corpus's own inventory: `source.parent / dirName / (source.stem() + suffix)`. Existence
// is checked here, not left to the caller, because a fallback chain only means something if
// the first convention that actually exists on disk wins -- a corpus with more than one
// convention (rpatters1-main: some sources still pair under the older `-exports`/
// `.fin27.musx`, more recent ones under `-finale27`/`.musx`) would otherwise always resolve
// to the first declared convention's path whether or not that file is really there. Returns
// nothing when no declared convention's candidate exists -- comparison against a companion
// is opt-in per corpus and best-effort per row, not attempted speculatively.
std::optional<std::filesystem::path> companionPathFor(
    const std::filesystem::path& source, const std::vector<CompanionConvention>& conventions)
{
    for (const auto& convention : conventions) {
        auto candidate =
            source.parent_path() / convention.dirName / (source.stem().string() + convention.suffix);
        std::error_code error;
        if (std::filesystem::exists(candidate, error) && !error) {
            return candidate;
        }
    }
    return std::nullopt;
}

// The width "Processed X of T" needs so a shorter update never leaves stale characters
// trailing from a longer one -- widest when X has as many digits as T, i.e. using T for X.
std::size_t progressLineWidth(std::size_t total)
{
    return ("Processed " + std::to_string(total) + " of " + std::to_string(total)).size();
}

// Rewrites one status line in place with a carriage return, padded to a fixed width so a
// shorter line never leaves stale characters trailing from a longer one. Leaves the cursor
// on that line with no trailing newline, so `dirty` is set to flag that whatever prints
// next -- a diagnostic, the next progress update, anything -- shares the line unless it is
// broken first with endProgressLine().
void printProgress(std::size_t processed, std::size_t total, std::size_t lineWidth, bool& dirty)
{
    std::ostringstream line;
    line << "Processed " << processed << " of " << total;
    std::string text = line.str();
    if (text.size() < lineWidth) {
        text.append(lineWidth - text.size(), ' ');
    }
    std::cout << '\r' << text << std::flush;
    dirty = true;
}

// Moves off a dirty progress line before something else prints, so a diagnostic never lands
// on the same line as "Processed X of T" instead of starting its own. A no-op when the line
// is already clean, so callers can call this unconditionally before every stderr write.
void endProgressLine(bool& dirty)
{
    if (dirty) {
        std::cout << '\n' << std::flush;
        dirty = false;
    }
}

// The classic Mac file type from a loose file's Finder Info, or nothing when this program
// is not running on macOS, the attribute is absent, or it holds fewer than the four bytes a
// file type occupies. A real Finale document reads type `NGMA`; a Finale library reads
// `LIB3` -- see private/corpora/rpatters1-installs.conf. That distinction lives in HFS
// volume metadata outside the file's data fork entirely, so it survives only on a loose
// file still sitting on the volume that wrote it: an archive member extracted during a
// survey, or any copy taken off that volume, will not carry it, and this returns nothing
// for either rather than guessing.
std::optional<std::string> macFinderFileType(const std::string& path)
{
#if defined(__APPLE__)
    unsigned char finderInfo[32];
    const auto size = getxattr(
        path.c_str(), "com.apple.FinderInfo", finderInfo, sizeof(finderInfo), 0, 0);
    if (size < 4) {
        return std::nullopt;
    }
    return std::string(reinterpret_cast<char*>(finderInfo), 4);
#else
    static_cast<void>(path);
    return std::nullopt;
#endif // defined(__APPLE__)
}

// Prints a corpus's final tally as a real, newline-terminated diagnostic line: the actual
// number of rows processed against the number segmentByCorpus() expected for it, not the
// expected count printed twice, so the two are genuinely independent and comparable rather
// than assumed equal. When a live progress line for that same corpus is still on screen,
// this overwrites it in place with a leading carriage return instead of leaving it behind
// followed by a duplicate-looking line -- stdout and stderr share one terminal cursor, so a
// carriage return written to either moves the same one. `dirty` is always false once this
// returns.
//
// A mismatch means a row was silently skipped or double-counted somewhere above, which
// nothing else here would otherwise surface, so it is reported regardless of --progress; it
// still respects --min-diagnostic-level like any other warning.
void printSegmentComplete(const std::string& label, std::size_t processedCount,
    std::size_t expectedTotal, bool& dirty, LogLevel minLevel)
{
    std::fprintf(stderr, "%sProcessed %zu of %zu\n", dirty ? "\r" : "", processedCount, expectedTotal);
    dirty = false;
    if (processedCount != expectedTotal && diagnosticRank(LogLevel::Warning) >= diagnosticRank(minLevel)) {
        std::fprintf(stderr,
            "[%s] Processed count does not match its expected total: processed %zu, expected %zu.\n",
            label.c_str(), processedCount, expectedTotal);
    }
}

} // namespace

int main(int argc, char** argv)
{
    // Checked ahead of parseOptions() so --help works even without the two required
    // positional arguments, matching ordinary CLI convention.
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        }
    }
    const auto options = parseOptions(argc, argv);
    if (!options) {
        return 2;
    }
    const std::filesystem::path corpusListPath = options->corpusListPath;
    const auto topLabel = corpusListPath.stem().string();

    std::vector<std::uint8_t> macSymbolFonts;
    if (!options->macSymbolFontsPath.empty()) {
        std::ifstream input(options->macSymbolFontsPath, std::ios::binary | std::ios::ate);
        if (!input) {
            std::fprintf(stderr, "cannot open MacSymbolFonts file: %s\n",
                options->macSymbolFontsPath.c_str());
            return 2;
        }
        const auto end = input.tellg();
        if (end < 0 || static_cast<std::uintmax_t>(end)
                > static_cast<std::uintmax_t>((std::numeric_limits<std::streamsize>::max)())) {
            std::fprintf(stderr, "MacSymbolFonts file is too large: %s\n",
                options->macSymbolFontsPath.c_str());
            return 2;
        }
        macSymbolFonts.resize(static_cast<std::size_t>(end));
        input.seekg(0);
        input.read(reinterpret_cast<char*>(macSymbolFonts.data()),
            static_cast<std::streamsize>(macSymbolFonts.size()));
        if (!input && !macSymbolFonts.empty()) {
            std::fprintf(stderr, "cannot read MacSymbolFonts file: %s\n",
                options->macSymbolFontsPath.c_str());
            return 2;
        }
    }
    const finale_mus_reader::ReaderOptions readerOptions{macSymbolFonts};
    const auto symbolFontNames = finale_mus_reader::text::parseMacSymbolFonts(macSymbolFonts);

    std::vector<CorpusRow> rows;
    std::map<std::string, std::filesystem::path> corpusRoots;
    std::map<std::string, std::vector<CompanionConvention>> corpusCompanions;
    if (isSingleMusFile(corpusListPath)) {
        // A source given directly rather than as a corpus list: one synthetic row, labeled
        // and identified by its own filename since there is no separate corpus_id for it.
        rows.push_back({topLabel, corpusListPath.string(), topLabel});
    } else {
        std::ifstream list(corpusListPath);
        if (!list) {
            std::fprintf(stderr, "cannot open corpus list: %s\n", options->corpusListPath.c_str());
            return 2;
        }
        rows = readCorpusRows(list, topLabel, corpusRoots, corpusCompanions);
    }
    std::ofstream output(options->outputPath);
    if (!output) {
        std::fprintf(stderr, "cannot write output: %s\n", options->outputPath.c_str());
        return 2;
    }

    using namespace finale_mus_reader;
    using namespace finale_mus_reader::coverage;

    std::cout << "Options:\n"
        << "  min diagnostic level: " << diagnosticLevelName(options->minDiagnosticLevel) << '\n'
        << "  include timings: " << (options->includeTimings ? "yes" : "no") << '\n'
        << "  progress: " << (options->showProgress ? "on" : "off") << '\n'
        << "  MacSymbolFonts: ";
    if (options->macSymbolFontsPath.empty()) {
        std::cout << "not supplied";
    } else {
        std::cout << std::quoted(options->macSymbolFontsPath);
    }
    std::cout << '\n';

    const auto total = rows.size();
    const auto segments = segmentByCorpus(rows, corpusRoots, corpusCompanions);

    // True exactly when a progress line is on screen with no trailing newline yet; every
    // stderr write below breaks it first via endProgressLine(), which is a no-op otherwise,
    // so nothing needs to separately check options->showProgress to do that.
    bool progressLineDirty = false;

    std::size_t failed = 0;
    std::size_t segmentIndex = 0;
    // Position within the current corpus, not across the whole run: crossing into a new
    // segment below resets this to 0, so "Processed X of T" and the progress interval both
    // read against that corpus's own count, the same as running it as a lone input would.
    std::size_t processedInSegment = 0;
    std::size_t segmentTotal = 0;
    std::size_t segmentProgressWidth = 0;
    std::string currentLabel;
    std::filesystem::path currentRoot;
    std::vector<CompanionConvention> currentCompanion;

    // Installed once, for the whole run, rather than reinstalled per row: Logger is a single
    // global callback (see musx/util/Logger.h), and there is exactly one thing here that
    // should ever be listening to it, so nothing is gained by tearing it down and putting it
    // back between documents. Reading `currentCorpusId` and `loggerCaptured` by reference
    // means it stays live and correctly labeled through every phase of every import,
    // including musxdom's own construction-completion pass, without needing to know when
    // that pass runs relative to the reader's own diagnostics.
    //
    // This callback is this row's *only* diagnostics source: everything below reads
    // `loggerCaptured`, never `result.report.diagnostics` directly. musxdom logs some
    // diagnostics (a placeholder it mints for an undefined font reference, say) only through
    // this single global hook, with no structured record a caller can otherwise reach --
    // that is the gap this is working around, not something to fix here. The tradeoff is
    // that routine, non-diagnostic Logger traffic (mus_container.cpp's speculative
    // byte-order/codec probe noise, at Verbose) rides along too, since this callback has no
    // way to tell that apart from a real diagnostic. Living with that noise is deliberate
    // for now: a real fix needs musxdom's own logging reworked into something structured
    // enough to filter by source, not patched around from outside it.
    std::string currentCorpusId;
    std::vector<finale_mus_reader::Diagnostic> loggerCaptured;
    const auto minLevel = options->minDiagnosticLevel;
    musx::util::Logger::setCallback(
        [&currentCorpusId, minLevel, &progressLineDirty, &loggerCaptured]
        (LogLevel level, const std::string& message) {
            loggerCaptured.push_back({level, message});
            if (diagnosticRank(level) < diagnosticRank(minLevel)) return;
            endProgressLine(progressLineDirty);
            std::fprintf(stderr, "[%s] %s\n", currentCorpusId.c_str(), message.c_str());
        });

    for (const auto& row : rows) {
        const auto& corpusId = row.corpusId;
        const auto& path = row.path;
        currentCorpusId = corpusId;
        loggerCaptured.clear();

        if (row.corpusLabel != currentLabel) {
            if (segmentIndex > 0) {
                printSegmentComplete(currentLabel, processedInSegment, segmentTotal,
                    progressLineDirty, options->minDiagnosticLevel);
            }
            currentLabel = row.corpusLabel;
            const auto& segment = segments.at(segmentIndex++);
            segmentTotal = segment.count;
            currentRoot = segment.root;
            currentCompanion = segment.companions;
            segmentProgressWidth = progressLineWidth(segmentTotal);
            processedInSegment = 0;
            std::cout << "== " << currentLabel << ": " << segmentTotal << " file"
                << (segmentTotal == 1 ? "" : "s") << " ==\n";
            if (options->showProgress) {
                printProgress(0, segmentTotal, segmentProgressWidth, progressLineDirty);
            }
        }
        ++processedInSegment;

        std::ostringstream out;
        out << '{' << "\"schema\":3,\"corpus_id\":" << jsonString(corpusId);
        // Wall-clock time for exactly the work this row does -- reading and importing the
        // source plus running every surveyor -- not the line's JSON assembly around it, so a
        // slow surveyor and a slow import are both visible in the same field. Measured only
        // over the source phase: the companion phase below (when there is one) gets its own
        // duration_ms nested under "companion", so a slow companion load never reads as a
        // slow reader import or vice versa.
        bool sourceOk = false;
        const auto started = std::chrono::steady_clock::now();
        std::optional<double> readerDurationMs;
        std::vector<timing::Measurement> readerPhaseTimings;
        std::vector<timing::CounterMeasurement> readerCounters;
        std::vector<timing::ContainerAttemptMeasurement> containerAttemptTimings;
        std::optional<SurveyTimings> sourceSurveyTimings;
        std::optional<SurveySnapshot> sourceSnapshot;
        musx::dom::DocumentPtr sourceDocument;
        std::unique_ptr<ImportReport> sourceReport;
        try {
            const auto readerStarted = std::chrono::steady_clock::now();
            timing::Session readerTimingSession;
            const auto result = Reader::readWithReport<musx::xml::pugi::Document>(
                std::filesystem::path(path), readerOptions);
            const std::chrono::duration<double, std::milli> readerElapsed =
                std::chrono::steady_clock::now() - readerStarted;
            readerDurationMs = readerElapsed.count();
            readerPhaseTimings = readerTimingSession.measurements();
            readerCounters = readerTimingSession.counters();
            containerAttemptTimings = readerTimingSession.containerAttempts();
            if (!result.document) {
                throw std::runtime_error(importError(result.report));
            }
            sourceDocument = result.document;
            sourceReport = std::make_unique<ImportReport>(result.report);
            out << ",\"status\":\"ok\""
                << ",\"epoch\":" << jsonString(epochName(result.report.formatEpoch))
                << ",\"saving_product\":" << jsonString(result.report.savingProduct)
                << ",\"source_version\":" << jsonString(versionName(result.report))
                << ",\"warning_count\":" << loggerCaptured.size();
            auto survey = runAllSurveyors(
                SurveyContext{result.document, result.report});
            sourceSurveyTimings = std::move(survey.timings);
            sourceSnapshot = std::move(survey.snapshot);
            writeSurveyErrors(out, survey.errors);
            sourceOk = true;
        } catch (const std::exception& error) {
            if (!readerDurationMs) {
                const std::chrono::duration<double, std::milli> readerElapsed =
                    std::chrono::steady_clock::now() - started;
                readerDurationMs = readerElapsed.count();
            }
            ++failed;
            const auto finderType = macFinderFileType(path);
            std::string message = error.what();
            // A file whose own Finder type identifies it as a Finale library gets a
            // friendlier message than the generic one below: we know specifically why this
            // one can't be read, not just that it can't be. Reworded only when the failure
            // is the exact case that identification actually explains -- a library file
            // that happened to fail some other way would be misdescribed by this one.
            if (finderType == "LIB3"
                    && message == "This file does not appear to be a Finale MUS document.") {
                message = "Unable to process Finale LIB file.";
            }
            // Console-only, as the survey policy requires for failures -- but relative to
            // the corpus's own root when it declared one, the bare filename otherwise, never
            // the full path: this still lets an operator locate which file failed without
            // printing the directory structure above the corpus, so a terminal log or a
            // redirected run that ends up somewhere tracked leaks a lot less if it does.
            endProgressLine(progressLineDirty);
            std::fprintf(stderr, "FAILED %s: %s\n    %s\n",
                corpusId.c_str(), message.c_str(),
                displayPathFor(path, currentRoot).c_str());
            out << ",\"status\":\"error\""
                << ",\"error\":" << jsonString(message);
            if (finderType) {
                out << ",\"finder_type\":" << jsonString(*finderType);
            }
        }
        const std::chrono::duration<double, std::milli> elapsed =
            std::chrono::steady_clock::now() - started;
        if (options->includeTimings) {
            out << ",\"duration_ms\":";
            writeTimingValue(out, elapsed.count());
            out << ",\"timings\":{\"reader_ms\":";
            writeTimingValue(out, *readerDurationMs);
            if (!readerPhaseTimings.empty()) {
                out << ',';
                writeReaderPhaseTimings(out, readerPhaseTimings);
            }
            if (!readerCounters.empty()) {
                out << ',';
                writeReaderCounters(out, readerCounters);
            }
            if (!containerAttemptTimings.empty()) {
                out << ',';
                writeContainerAttempts(out, containerAttemptTimings);
            }
            if (sourceSurveyTimings) {
                out << ',';
                writeSurveyTimings(out, *sourceSurveyTimings);
            }
            out << '}';
        }

        // A companion is only worth loading when there is a source result to compare it
        // against, and only attempted at all when this row's corpus declared a convention
        // for finding one (see companionPathFor()). "companion" is omitted entirely rather
        // than written with some "missing" status, so a row that was never compared looks
        // nothing like one that compared clean.
        if (sourceOk) {
            const auto companionPath = companionPathFor(path, currentCompanion);
            if (companionPath) {
                loggerCaptured.clear();
                std::ostringstream companionOut;
                const auto companionStarted = std::chrono::steady_clock::now();
                std::optional<double> documentLoadDurationMs;
                std::optional<double> archiveDurationMs;
                std::optional<double> documentFactoryDurationMs;
                std::optional<SurveyTimings> companionSurveyTimings;
                std::optional<SurveySnapshot> companionSnapshot;
                musx::dom::DocumentPtr companionDocument;
                try {
                    const auto documentLoadStarted = std::chrono::steady_clock::now();
                    const auto archiveStarted = std::chrono::steady_clock::now();
                    auto archive = readCompanionArchive(*companionPath);
                    const std::chrono::duration<double, std::milli> archiveElapsed =
                        std::chrono::steady_clock::now() - archiveStarted;
                    archiveDurationMs = archiveElapsed.count();
                    const auto documentFactoryStarted = std::chrono::steady_clock::now();
                    musx::factory::DocumentFactory::CreateOptions::EmbeddedGraphicFiles graphicFiles;
                    for (auto& [name, bytes] : archive.embeddedGraphics) {
                        graphicFiles.push_back({std::move(name), std::move(bytes)});
                    }
                    musx::factory::DocumentFactory::CreateOptions createOptions(*companionPath,
                        archive.notationMetadata.value_or(std::vector<char>{}),
                        std::move(graphicFiles));
                    companionDocument =
                        musx::factory::DocumentFactory::create<musx::xml::pugi::Document>(
                            archive.enigmaXml, std::move(createOptions));
                    const std::chrono::duration<double, std::milli> documentFactoryElapsed =
                        std::chrono::steady_clock::now() - documentFactoryStarted;
                    documentFactoryDurationMs = documentFactoryElapsed.count();
                    const std::chrono::duration<double, std::milli> documentLoadElapsed =
                        std::chrono::steady_clock::now() - documentLoadStarted;
                    documentLoadDurationMs = documentLoadElapsed.count();
                    const ImportReport emptyReport(sourceReport->formatEpoch);
                    companionOut << "\"status\":\"ok\""
                        << ",\"warning_count\":" << loggerCaptured.size();
                    auto survey = runAllSurveyors(
                        SurveyContext{companionDocument, emptyReport});
                    companionSurveyTimings = std::move(survey.timings);
                    companionSnapshot = std::move(survey.snapshot);
                    writeSurveyErrors(companionOut, survey.errors);
                } catch (const std::exception& error) {
                    companionOut << "\"status\":\"error\""
                        << ",\"error\":" << jsonString(error.what());
                }
                const std::chrono::duration<double, std::milli> companionElapsed =
                    std::chrono::steady_clock::now() - companionStarted;
                if (options->includeTimings) {
                    companionOut << ",\"duration_ms\":";
                    writeTimingValue(companionOut, companionElapsed.count());
                    companionOut << ",\"timings\":{";
                    bool firstTiming = true;
                    if (documentLoadDurationMs) {
                        companionOut << "\"document_load_ms\":";
                        writeTimingValue(companionOut, *documentLoadDurationMs);
                        firstTiming = false;
                    }
                    if (archiveDurationMs) {
                        companionOut << (firstTiming ? "" : ",") << "\"archive_ms\":";
                        writeTimingValue(companionOut, *archiveDurationMs);
                        firstTiming = false;
                    }
                    if (documentFactoryDurationMs) {
                        companionOut << (firstTiming ? "" : ",") << "\"document_factory_ms\":";
                        writeTimingValue(companionOut, *documentFactoryDurationMs);
                        firstTiming = false;
                    }
                    if (companionSurveyTimings) {
                        companionOut << (firstTiming ? "" : ",");
                        writeSurveyTimings(companionOut, *companionSurveyTimings);
                    }
                    companionOut << '}';
                }
                out << ",\"companion\":{" << companionOut.str() << "}";
                if (sourceSnapshot && companionSnapshot) {
                    out << ",\"comparison\":{";
                    const auto comparison = compareSnapshots(*sourceSnapshot, *companionSnapshot,
                        sourceDocument, companionDocument, sourceReport->formatEpoch,
                        sourceReport->byteOrder,
                        sourceReport->sourceVersion ? &*sourceReport->sourceVersion : nullptr,
                        *sourceReport,
                        &symbolFontNames);
                    writeCompactComparison(out, comparison);
                    out << '}';
                }
            }
        }

        out << '}';
        output << out.str() << '\n';

        if (options->showProgress
                && (processedInSegment % progressInterval == 0 || processedInSegment == segmentTotal)) {
            printProgress(processedInSegment, segmentTotal, segmentProgressWidth, progressLineDirty);
        }
    }
    if (!rows.empty()) {
        printSegmentComplete(currentLabel, processedInSegment, segmentTotal, progressLineDirty,
            options->minDiagnosticLevel);
    }
    std::fprintf(stderr, "read %zu documents, %zu failed\n", total, failed);
    return 0;
}
