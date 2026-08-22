// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <ostream>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;
using musx::dom::header::FileInfo;
using musx::dom::header::FinaleVersion;
using musx::dom::header::Platform;

const char* platformName(Platform platform)
{
    switch (platform) {
    case Platform::Mac: return "mac";
    case Platform::Windows: return "windows";
    case Platform::Other: return "other";
    }
    return "other";
}

void writeVersion(std::ostream& out, const FinaleVersion& version)
{
    out << "{\"major\":" << version.major
        << ",\"minor\":" << version.minor
        << ",\"maint\":";
    if (version.maint) out << *version.maint; else out << "null";
    out << ",\"dev_status\":" << jsonString(version.devStatus)
        << ",\"build\":";
    if (version.build) out << *version.build; else out << "null";
    out << '}';
}

void writeFileInfo(std::ostream& out, const FileInfo& info)
{
    // Dates and modifier initials are document identity rather than format evidence. The
    // three version tuples and their application/platform context are the header fields that
    // answer which implementation created or last saved the representation being surveyed.
    out << "{\"finale_version\":";
    writeVersion(out, info.finaleVersion);
    out << ",\"application\":" << jsonString(info.application)
        << ",\"platform\":" << jsonString(platformName(info.platform))
        << ",\"app_version\":";
    writeVersion(out, info.appVersion);
    out << ",\"file_version\":";
    writeVersion(out, info.fileVersion);
    out << '}';
}

void writeHeader(std::ostream& out, const SurveyContext& ctx)
{
    const auto header = ctx.document->getHeader();
    if (!header) {
        out << "null";
        return;
    }
    out << "{\"created\":";
    writeFileInfo(out, header->created);
    out << ",\"modified\":";
    writeFileInfo(out, header->modified);
    out << '}';
}

COVERAGE_SURVEYOR("header", writeHeader);

} // namespace
