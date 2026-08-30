// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/comparison.h"

#include <map>
#include <stdexcept>
#include <string_view>

#include "coverage/json.h"

namespace finale_mus_reader {
namespace coverage {
namespace {

std::string_view differenceName(DifferenceClassification classification)
{
    using enum DifferenceClassification;
    switch (classification) {
    case Unexpected:
        return "unexpected";
    case AccidentalInsert17Byte:
        return "17-byte-accidental-insert";
    case BaselineFont:
        return "baseline-font";
    case CodaTextBlockUpgrade:
        return "coda-text-block-upgrade";
    case DefaultShapeId:
        return "default-shape-id";
    case DifferentDefaults:
        return "different_defaults";
    case EnigmaTextDifference:
        return "enigma-text-difference";
    case FinaleTextBlockRenumbering:
        return "finale-text-block-renumbering";
    case FinaleUpgradeLoss:
        return "finale-upgrade-loss";
    case LegacyPageParityText:
        return "legacy-page-parity-text";
    case MissingAccidentalInsertDefault:
        return "missing-accidental-insert-default";
    case MissingSelector:
        return "missing-selector";
    case PreConnectionEndpoint:
        return "pre-connection-endpoint";
    case ReaderCompletedConnectionArray:
        return "reader-completed-connection-array";
    case SetFontSubstitution:
        return "setfont-font-substitution";
    case ShapeReclassifiedOther:
        return "shape-reclassified-other";
    case SmartLyricsEnabled:
        return "smart-lyrics-enabled";
    case StemConnectionPastTerminator:
        return "stem-connection-past-terminator";
    case StemHorizontalCorrection:
        return "stem-horizontal-correction";
    case TransientTextBlock:
        return "transient-text-block";
    }
    throw std::logic_error("unhandled difference classification");
}

std::string_view textDifferenceName(TextDifferenceClassification classification)
{
    using enum TextDifferenceClassification;
    switch (classification) {
    case AddedFontInfo:
        return "added font info";
    case Effects:
        return "effects";
    case EmptyPartNameTemplate:
        return "empty part-name template";
    case Font:
        return "font";
    case KnownEncodingGlitch:
        return "known encoding glitch";
    case MissingRun:
        return "missing run";
    case Other:
        return "other";
    case Size:
        return "size";
    case UnresolvedFont:
        return "unresolved font";
    case Whitespace:
        return "whitespace";
    }
    throw std::logic_error("unhandled text difference classification");
}

std::string_view transformationName(ComparisonTransformation transformation)
{
    using enum ComparisonTransformation;
    switch (transformation) {
    case EquivalentEnigmaFontState:
        return "Equivalent Enigma font-state serialization";
    case EquivalentTextBlockReferent:
        return "Equivalent TextBlock raw-text referent";
    case FinaleAddedStartObjectWrapper:
        return "Finale-added StartObject wrapper";
    case FinaleDroppedTimeInsert:
        return "Finale-dropped ^time insert";
    case FinaleReformattedPartName:
        return "Finale-reformatted part-name text";
    case SemanticallyPairedCodaBlockText:
        return "Semantically paired Coda block text";
    }
    throw std::logic_error("unhandled comparison transformation");
}

template <typename Key, typename Name>
void writeCounts(std::ostream& out, const std::map<Key, std::uint64_t>& counts, Name name)
{
    out << '{';
    bool first = true;
    for (const auto& [key, count] : counts) {
        out << (first ? "" : ",") << jsonString(name(key)) << ':' << count;
        first = false;
    }
    out << '}';
}

void writeStringCounts(std::ostream& out, const std::map<std::string, std::uint64_t>& counts)
{
    writeCounts(out, counts, [](const std::string& value) -> std::string_view { return value; });
}

} // namespace

void writeCompactComparison(std::ostream& out, const ComparisonResult& comparison)
{
    out << "\"classes\":{";
    bool firstPool = true;
    for (const auto& [pool, classes] : comparison.classes) {
        out << (firstPool ? "" : ",") << jsonString(pool) << ":{";
        bool firstClass = true;
        for (const auto& [name, counts] : classes) {
            out << (firstClass ? "" : ",") << jsonString(name) << ":[" << counts.same << ','
                << counts.expected << ',' << counts.unexpected << ',' << counts.sourceOnly << ','
                << counts.companionOnly << ']';
            firstClass = false;
        }
        out << '}';
        firstPool = false;
    }
    out << "},\"expected\":";
    writeCounts(out, comparison.expected, differenceName);
    out << ",\"transformations\":";
    writeCounts(out, comparison.transformations, transformationName);
    out << ",\"font_substitutions\":";
    writeStringCounts(out, comparison.fontSubstitutions);
    out << ",\"text\":{";
    bool first = true;
    for (const auto& [className, counts] : comparison.textDifferences) {
        out << (first ? "" : ",") << jsonString(className) << ':';
        writeCounts(out, counts, textDifferenceName);
        first = false;
    }
    out << '}';
    const auto writeExamples = [&](std::string_view key,
                                   const std::vector<DifferenceExample>& examples,
                                   bool writeOrigin) {
        out << ",\"" << key << "\":[";
        bool firstExample = true;
        for (const auto& example : examples) {
            out << (firstExample ? "" : ",") << '[' << jsonString(example.path) << ','
                << example.source.toJson() << ',' << example.companion.toJson();
            if (writeOrigin && !example.origin.empty()) {
                out << ',' << jsonString(example.origin);
            } else if (!writeOrigin && example.kind) {
                out << ',' << jsonString(textDifferenceName(*example.kind));
            }
            out << ']';
            firstExample = false;
        }
        out << ']';
    };
    writeExamples("unexpected", comparison.unexpectedExamples, true);
    writeExamples("text_examples", comparison.textExamples, false);
}

} // namespace coverage
} // namespace finale_mus_reader
