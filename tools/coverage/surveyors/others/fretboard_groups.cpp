// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;
using finale_mus_reader::instanceKey;

Value observeFretboardGroups(const SurveyContext& ctx)
{
    using Target = musx::dom::others::FretboardGroup;
    Value::Array result;
    for (const auto& group : ctx.document->getOthers()
            ->getArray<Target>(musx::dom::SCORE_PARTID)) {
        result.emplace_back(observe(*group, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("inci", [](const Target& value) { return value.getInci().value_or(0); }),
            field("fret_inst_id", &Target::fretInstId), field("name", &Target::name),
            field("origin_fret_inst_id", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "fretInstId",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper(),
                        value.getInci()));
            })));
    }
    return result;
}

COVERAGE_SURVEYOR("others", "fretboard_groups", observeFretboardGroups);

} // namespace
