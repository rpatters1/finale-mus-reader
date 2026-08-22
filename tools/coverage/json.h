// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// JSON-emission and label helpers shared by every surveyor, so a class's coverage file
// only ever spells out its own fields.

#pragma once

#include <string>
#include <string_view>

#include "finale_mus_reader/reader.h"

namespace finale_mus_reader::coverage {

std::string jsonString(std::string_view value);
inline const char* jsonBool(bool value) { return value ? "true" : "false"; }

const char* originName(ValueOrigin origin);
const char* epochName(FormatEpoch epoch);
const char* byteOrderName(ByteOrder byteOrder);
const char* diagnosticLevelName(musx::util::Logger::LogLevel level);
std::string versionName(const ImportReport& report);

// The reader returns failure rather than throwing: a null document means the import
// failed, and the reason is the Error-level diagnostic in the report.
std::string importError(const ImportReport& report);

} // namespace finale_mus_reader::coverage
