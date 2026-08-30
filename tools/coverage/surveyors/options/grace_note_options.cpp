// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "import/support/legacy_mapping.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyGraceNoteOptionsDifference(const DifferenceContext& context)
{
    using Target = musx::dom::options::GraceNoteOptions;
    if (context.category != DifferenceCategory::Differs ||
        context.epoch != finale_mus_reader::FormatEpoch::CodaBanner ||
        context.origin != "legacy-mus" ||
        context.path != "grace_note_options.grace_slash_width") {
        return std::nullopt;
    }
    const auto* source = context.sourceReport.findField<Target>("graceSlashWidth");
    if (source && source->sourceIdentity &&
        *source->sourceIdentity != finale_mus_reader::numericGlobalTag(
            finale_mus_reader::codaMigratedPointSizeSelector)) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    return std::nullopt;
}

Value observeGraceNoteOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::GraceNoteOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("tab_grace_perc", &Target::tabGracePerc),
        field("grace_perc", &Target::gracePerc),
        field("playback_duration", &Target::playbackDuration),
        field("entry_offset", &Target::entryOffset),
        field("slash_flagged_grace_notes", &Target::slashFlaggedGraceNotes),
        field("grace_slash_width", &Target::graceSlashWidth));
    for (const auto* member : {"tabGracePerc",
                               "gracePerc",
                               "playbackDuration",
                               "entryOffset",
                               "slashFlaggedGraceNotes",
                               "graceSlashWidth"}) {
        result.asObject().emplace(std::string("origin_") + member,
            fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_CLASS("options", "grace_note_options", observeGraceNoteOptions,
    classifyGraceNoteOptionsDifference);

} // namespace
