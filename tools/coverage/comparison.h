// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "coverage/registry.h"
#include "import/support/text_encoding.h"
#include "musx/dom/Document.h"

namespace finale_mus_reader {
namespace coverage {

struct ClassComparison
{
    std::uint64_t same{};
    std::uint64_t expected{};
    std::uint64_t unexpected{};
    std::uint64_t sourceOnly{};
    std::uint64_t companionOnly{};
};

struct DifferenceExample
{
    std::string path;
    Value source;
    Value companion;
    std::string kind;
    std::string origin;
};

struct ComparisonResult
{
    std::map<std::string, std::map<std::string, ClassComparison>> classes;
    std::map<std::string, std::uint64_t> expected;
    std::map<std::string, std::uint64_t> transformations;
    std::map<std::string, std::map<std::string, std::uint64_t>> textDifferences;
    std::map<std::string, std::uint64_t> fontSubstitutions;
    std::vector<DifferenceExample> unexpectedExamples;
    std::vector<DifferenceExample> textExamples;
};

ComparisonResult compareSnapshots(SurveySnapshot source, SurveySnapshot companion,
    const musx::dom::DocumentPtr& sourceDocument,
    const musx::dom::DocumentPtr& companionDocument,
    FormatEpoch sourceEpoch, ByteOrder sourceByteOrder, const SourceVersion* sourceVersion,
    const text::SymbolFontNames* symbolFontNames);

void writeCompactComparison(std::ostream& out, const ComparisonResult& comparison);

} // namespace coverage
} // namespace finale_mus_reader
