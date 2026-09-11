// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace finale_mus_reader {
namespace coverage {

inline bool hasMusExtension(const std::filesystem::path& source)
{
    auto extension = source.extension().string();
    std::ranges::transform(extension, extension.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return extension == ".mus";
}

/// @brief Returns the source filename used as the base of a companion filename.
/// @details A terminal `.mus` suffix is removed case-insensitively. Every other suffix is retained
/// because classic Mac documents can be extensionless even when their names contain dots.
inline std::string companionBaseNameFor(
    const std::filesystem::path& source, bool distinguishFromSibling = false)
{
    const auto hasExtension = hasMusExtension(source);
    auto result = hasExtension ? source.stem().string() : source.filename().string();
    if (distinguishFromSibling) {
        result += hasExtension ? ".from-mus" : ".from-no-extension";
    }
    return result;
}

/// @brief Returns the source path that would conflict for the same companion basename.
inline std::filesystem::path companionNameConflictFor(const std::filesystem::path& source)
{
    if (hasMusExtension(source)) {
        return source.parent_path() / source.stem();
    }
    auto result = source;
    result += ".mus";
    return result;
}

} // namespace coverage
} // namespace finale_mus_reader
