// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// Relationships derived from the fully constructed document. These are kept apart from
// the pool surveyors because they describe how instances in different pools are connected,
// not another property of either instance.

#include <ostream>
#include <set>

#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeRelationships(std::ostream& out, const SurveyContext& ctx)
{
    using musx::dom::others::PartDefinition;
    using musx::dom::others::TextBlock;

    const auto parts = ctx.document->getOthers()->getArray<PartDefinition>(musx::dom::SCORE_PARTID);
    std::set<musx::dom::Cmper> partNameTextIds;
    for (const auto& part : parts) {
        if (!part->nameId) continue;
        if (const auto textBlock = ctx.document->getOthers()->get<TextBlock>(
                part->getRequestedPartId(), part->nameId)) {
            partNameTextIds.insert(textBlock->textId);
        }
    }

    out << "{\"part_names\":{\"total_parts\":" << parts.size() << ",\"text_ids\":[";
    bool first = true;
    for (const auto textId : partNameTextIds) {
        if (!first) out << ',';
        first = false;
        out << textId;
    }
    out << "]}}";
}

COVERAGE_SURVEYOR("relationships", writeRelationships)

} // namespace
