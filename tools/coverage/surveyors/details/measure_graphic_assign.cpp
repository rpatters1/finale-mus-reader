// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// The measure graphic assignments, which are details rather than others and so carry a second
// comparator. The fields are the ones both sides spell as plain numbers.

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;
using finale_mus_reader::instanceKey;

Value observeMeasureGraphicAssignments(const SurveyContext& ctx)
{
    using Target = musx::dom::details::MeasureGraphicAssign;
    Value::Array result;
    for (const auto& assign : ctx.document->getDetails()
            ->getArray<Target>(musx::dom::SCORE_PARTID)) {
        result.push_back(observe(*assign, ctx,
            field("cmper1", [](const Target& value) { return value.getCmper1(); }),
            field("cmper2", [](const Target& value) { return value.getCmper2(); }),
            field("inci", [](const Target& value) { return value.getInci().value_or(0); }),
            field("version", &Target::version), field("left", &Target::left),
            field("bottom", &Target::bottom),
            field("width", &Target::width), field("height", &Target::height),
            field("f_desc_id", &Target::fDescId), field("hidden", &Target::hidden),
            field("h_align", &Target::hAlign), field("v_align", &Target::vAlign),
            field("pos_from", &Target::posFrom), field("fixed_perc", &Target::fixedPerc),
            field("saved_record", &Target::savedRecord),
            field("orig_width", &Target::origWidth), field("orig_height", &Target::origHeight),
            field("graphic_cmper", &Target::graphicCmper),
            field("origin_version", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "version",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_left", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "left",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_bottom", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "bottom",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_width", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "width",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_height", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "height",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_fDescId", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "fDescId",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_hidden", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "hidden",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_hAlign", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "hAlign",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_vAlign", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "vAlign",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_posFrom", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "posFrom",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_fixedPerc", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "fixedPerc",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_savedRecord", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "savedRecord",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_origWidth", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "origWidth",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_origHeight", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "origHeight",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            }),
            field("origin_graphicCmper", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "graphicCmper",
                    instanceKey<Target>(value.getSourcePartId(), value.getCmper1(),
                        value.getInci(), value.getCmper2()));
            })));
    }
    return Value(std::move(result));
}

COVERAGE_SURVEYOR("details", "meas_graphic_assigns", observeMeasureGraphicAssignments);

} // namespace
