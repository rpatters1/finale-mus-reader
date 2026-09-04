// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Relationships derived from the fully constructed document. These are kept apart from
// the pool surveyors because they describe how instances in different pools are connected,
// not another property of either instance.

#include <set>

#include "coverage/registry.h"
#include "coverage/value.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeRelationships(const SurveyContext& ctx)
{
    using musx::dom::others::PartDefinition;
    using musx::dom::others::TextBlock;

    std::set<musx::dom::Cmper> partNameTextIds;
    const auto score = ctx.document->getOthers()->get<PartDefinition>(
        musx::dom::SCORE_PARTID, musx::dom::SCORE_PARTID);
    if (score && score->nameId) {
        if (const auto textBlock = ctx.document->getOthers()->get<TextBlock>(
                score->getRequestedPartId(), score->nameId)) {
            partNameTextIds.insert(textBlock->textId);
        }
    }

    Value::Array textIds;
    for (const auto textId : partNameTextIds) {
        textIds.emplace_back(textId);
    }
    // The ids alone, deliberately. A part count cannot stand in for "this side names a part":
    // every imported document has a score part, including the eras that store no part definition,
    // and that part carries no name. See isPartNameText in coverage/comparison_text.cpp.
    return Value::Object{{"part_names", Value::Object{{"text_ids", std::move(textIds)}}}};
}

COVERAGE_SURVEYOR("metadata", "relationships", observeRelationships)

} // namespace
