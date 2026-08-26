// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/json.h"

#include <cstdio>

namespace finale_mus_reader::coverage {

std::string jsonString(std::string_view value)
{
    std::string result = "\"";
    for (const char ch : value) {
        switch (ch) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20) {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x",
                    static_cast<unsigned>(static_cast<unsigned char>(ch)));
                result += buffer;
            } else {
                result += ch;
            }
        }
    }
    return result + '"';
}

const char* originName(ValueOrigin origin)
{
    switch (origin) {
    case ValueOrigin::Unmapped: return "unmapped";
    case ValueOrigin::MusxOnly: return "musx-only";
    case ValueOrigin::LegacyMus: return "legacy-mus";
    case ValueOrigin::LegacyBehavior: return "legacy-behavior";
    case ValueOrigin::Finale27Default: return "finale27-default";
    }
    return "unknown";
}

const char* epochName(FormatEpoch epoch)
{
    switch (epoch) {
    case FormatEpoch::CodaBanner: return "coda-banner";
    case FormatEpoch::UncompressedLegacy: return "uncompressed";
    case FormatEpoch::DclLegacy: return "dcl";
    case FormatEpoch::ZlibLegacy: return "zlib";
    case FormatEpoch::Unknown: return "unknown";
    }
    return "unknown";
}

const char* byteOrderName(ByteOrder byteOrder)
{
    switch (byteOrder) {
    case ByteOrder::LittleEndian: return "little-endian";
    case ByteOrder::BigEndian: return "big-endian";
    case ByteOrder::Unknown: return "unknown";
    }
    return "unknown";
}

const char* diagnosticLevelName(musx::util::Logger::LogLevel level)
{
    switch (level) {
    case musx::util::Logger::LogLevel::Verbose: return "verbose";
    case musx::util::Logger::LogLevel::Info: return "info";
    case musx::util::Logger::LogLevel::Warning: return "warning";
    case musx::util::Logger::LogLevel::Error: return "error";
    }
    return "unknown";
}

std::string versionName(const ImportReport& report)
{
    if (!report.sourceVersion) {
        return {};
    }
    const auto& version = *report.sourceVersion;
    return std::to_string(version.major) + '.' + std::to_string(version.minor) + '.'
        + std::to_string(version.maint) + '.' + std::to_string(version.build);
}

std::string importError(const ImportReport& report)
{
    for (const auto& entry : report.diagnostics) {
        if (entry.level == musx::util::Logger::LogLevel::Error) {
            return entry.message;
        }
    }
    return "import failed without a reported reason";
}

} // namespace finale_mus_reader::coverage
