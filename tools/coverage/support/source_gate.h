// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "finale_mus_reader/reader.h"
#include "support/finale_version.h"

namespace finale_mus_reader {
namespace coverage {

[[nodiscard]] constexpr bool sourceIsVersion(FormatEpoch sourceEpoch,
    const SourceVersion* sourceVersion, FormatEpoch expectedEpoch)
{
    return sourceEpoch == expectedEpoch && sourceVersion;
}

[[nodiscard]] constexpr bool sourceIsVersion(FormatEpoch sourceEpoch,
    const SourceVersion* sourceVersion, FormatEpoch expectedEpoch, VersionBound expectedVersion)
{
    return sourceIsVersion(sourceEpoch, sourceVersion, expectedEpoch)
        && VersionBound{sourceVersion->major, sourceVersion->minor} == expectedVersion;
}

[[nodiscard]] constexpr bool sourcePredatesVersion(
    const SourceVersion* sourceVersion, VersionBound boundary)
{
    return sourceVersion
        && VersionBound{sourceVersion->major, sourceVersion->minor} < boundary;
}

} // namespace coverage
} // namespace finale_mus_reader
