// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/support/legacy_mapping.h"

namespace finale_mus_reader::others {

/// @brief Whether a pre-3.7 page-text style selector is present in a source eligible to use it.
inline bool hasLegacyPageTextStyle(const ImportContext& context)
{
    const auto& profile = context.profile;
    if (profile.version && VersionBound{profile.version->major, profile.version->minor, profile.version->maint} >= versions::finale3_7) {
        return false;
    }
    if (!sourcePredatesVersion(profile, FormatEpoch::UncompressedLegacy, versions::finale3_7)) {
        return false;
    }
    return !context.index.getOthers().cmpersForTag(records::packTag("HS")).empty();
}

} // namespace finale_mus_reader::others
