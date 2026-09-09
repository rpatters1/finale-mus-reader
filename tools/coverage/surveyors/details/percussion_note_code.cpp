// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyPercussionNoteCodeDifference(const DifferenceContext &context) {
    if (context.category != DifferenceCategory::CompanionOnly) {
        return std::nullopt;
    }
    return deferredRecoveryClassified() ? DifferenceClassification::AwaitsDependentRecovery
                                        : DifferenceClassification::Unexpected;
}

Value observePercussionNoteCodes(const SurveyContext &ctx) {
    using Target = musx::dom::details::PercussionNoteCode;
    Value::Array result;
    for (const auto &note : sourceInstances<Target>(ctx)) {
        result.emplace_back(observe(*note, ctx, field("entry_number", &Target::getEntryNumber),
                                    field("note_code", &Target::noteCode),
                                    field("note_id", &Target::noteId),
                                    field("origin_noteCode",
                                          [&ctx](const Target &value) {
                                              return fieldOrigin<Target>(ctx, "noteCode", value);
                                          }),
                                    field("origin_noteId", [&ctx](const Target &value) {
                                        return fieldOrigin<Target>(ctx, "noteId", value);
                                    })));
    }
    return result;
}

COVERAGE_CLASS("details", "percussion_note_code", observePercussionNoteCodes,
               classifyPercussionNoteCodeDifference);

} // namespace
