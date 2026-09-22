// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "records/legacy_record_index.h"

namespace finale_mus_reader {
namespace percussion_records {

inline constexpr records::LegacyTag drumStaffTag = records::packTag("DS");
inline constexpr records::LegacyTag drumStaffStyleTag = records::packTag("FY");
inline constexpr records::LegacyTag drumStaffClass = 0x0084;
inline constexpr records::LegacyTag drumStaffStyleClass = 0x0085;

} // namespace percussion_records
} // namespace finale_mus_reader
