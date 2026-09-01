// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <map>
#include <string>

#include "coverage/comparison.h"

namespace finale_mus_reader {
namespace coverage {
namespace comparison_text {

enum class ReferentComparison
{
    None,
    Matching,
    MatchingPageOnly,
    Renumbered
};

void realignCodaBlockTexts(SurveySnapshot& source, SurveySnapshot& companion,
                           const musx::dom::DocumentPtr& sourceDocument,
                           const musx::dom::DocumentPtr& companionDocument,
                           ComparisonResult& result);
std::map<std::string, ReferentComparison>
compareTextBlockReferents(const musx::dom::DocumentPtr& sourceDocument,
                          const musx::dom::DocumentPtr& companionDocument);
bool isPartNameText(const std::string& className, const std::string& path,
                    const SurveySnapshot& source, const SurveySnapshot& companion);
TextClassificationResult compareText(const std::string& className, const std::string& path,
                                     const std::string& source, const std::string& companion,
                                     const musx::dom::DocumentPtr& sourceDocument,
                                     const musx::dom::DocumentPtr& companionDocument,
                                     bool partNameText);
bool hasSynthesizedTextState(const SurveySnapshot& source, const std::string& className,
                             const std::string& path);

} // namespace comparison_text
} // namespace coverage
} // namespace finale_mus_reader
