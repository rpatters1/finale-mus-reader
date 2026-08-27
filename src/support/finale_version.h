// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <compare>
#include <cstdint>

namespace finale_mus_reader {

/// @brief A Finale version boundary, ordered by major then minor.
/// @details Minor versions participate because the Finale 3.x line and Finale 97 share
/// internal major version 3.
struct VersionBound
{
    constexpr VersionBound(std::uint8_t sourceMajor) : major(sourceMajor) {}
    constexpr VersionBound(std::uint8_t sourceMajor, std::uint8_t sourceMinor)
        : major(sourceMajor), minor(sourceMinor)
    {
    }

    std::uint8_t major;
    std::uint8_t minor{};

    constexpr auto operator<=>(const VersionBound&) const = default;
};

namespace versions {
inline constexpr VersionBound finale1_0{1, 0};
inline constexpr VersionBound finale2_6{2, 6};
inline constexpr VersionBound finale3_0{3, 0};
inline constexpr VersionBound finale3_2{3, 2};
inline constexpr VersionBound finale3_5{3, 5};
inline constexpr VersionBound finale3_7{3, 7};
inline constexpr VersionBound finale97{3, 8};
inline constexpr VersionBound finale98{4};
inline constexpr VersionBound finale2000{5};
inline constexpr VersionBound finale2001{6};
inline constexpr VersionBound finale2002{7};
inline constexpr VersionBound finale2003{8};
inline constexpr VersionBound finale2004{9};
inline constexpr VersionBound finale2005{10};
inline constexpr VersionBound finale2006{11};
inline constexpr VersionBound finale2007{12};
inline constexpr VersionBound finale2008{13};
inline constexpr VersionBound finale2009{14};
inline constexpr VersionBound finale2010{15};
inline constexpr VersionBound finale2011{16};
inline constexpr VersionBound finale2012{17};
} // namespace versions

} // namespace finale_mus_reader
