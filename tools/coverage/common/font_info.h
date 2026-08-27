// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <string_view>

#include "coverage/classification.h"

namespace finale_mus_reader {
namespace coverage {

std::string canonicalFontName(std::string_view value);
bool sameFontName(std::string_view left, std::string_view right);
std::set<std::string> comparisonFontReferencePaths(const SurveySnapshot& snapshot);
std::string comparisonFontIdentity(const SurveySnapshot& snapshot, std::int64_t id);
bool isComparisonFontReference(std::string_view path,
                               const std::set<std::string>& shapeFontPaths);

} // namespace coverage
} // namespace finale_mus_reader
