// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/support/legacy_mapping.h"

namespace finale_mus_reader {
namespace coverage {

using finale_mus_reader::sourceAtOrAfter;
using finale_mus_reader::sourcePredatesVersion;

[[nodiscard]] constexpr bool sourceIsVersion(FormatEpoch sourceEpoch,
    const SourceVersion* sourceVersion, FormatEpoch expectedEpoch, VersionBound expectedVersion)
{
    return sourceEpoch == expectedEpoch && sourceVersion
        && VersionBound{sourceVersion->major, sourceVersion->minor} == expectedVersion;
}

} // namespace coverage
} // namespace finale_mus_reader
