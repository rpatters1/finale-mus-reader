// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
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

Value observeVersion(const FinaleVersion& version, const SurveyContext& context)
{
    return observe(version, context,
        field("major", &FinaleVersion::major), field("minor", &FinaleVersion::minor),
        field("maint", &FinaleVersion::maint), field("dev_status", &FinaleVersion::devStatus),
        field("build", &FinaleVersion::build));
}

Value observeFileInfo(const FileInfo& info, const SurveyContext& context)
{
    // Dates and modifier initials are document identity rather than format evidence. The
    // three version tuples and their application/platform context are the header fields that
    // answer which implementation created or last saved the representation being surveyed.
    return Value::Object{
        {"app_version", observeVersion(info.appVersion, context)},
        {"application", Value(info.application)},
        {"file_version", observeVersion(info.fileVersion, context)},
        {"finale_version", observeVersion(info.finaleVersion, context)},
        {"platform", Value(std::string(platformName(info.platform)))}};
}

Value observeHeader(const SurveyContext& ctx)
{
    const auto header = ctx.document->getHeader();
    if (!header) return {};
    return Value::Object{{"created", observeFileInfo(header->created, ctx)},
        {"modified", observeFileInfo(header->modified, ctx)}};
}

COVERAGE_SURVEYOR("metadata", "header", observeHeader);

} // namespace
