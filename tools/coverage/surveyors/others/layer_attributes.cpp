// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <ostream>
#include <string>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeLayerAttributes(std::ostream& out, const SurveyContext& ctx)
{
    out << '[';
    bool first = true;
    for (const auto& layer : ctx.document->getOthers()
            ->getArray<musx::dom::others::LayerAttributes>(musx::dom::SCORE_PARTID)) {
        if (!first) out << ',';
        first = false;
        out << "{\"cmper\":" << layer->getCmper()
            << ",\"rest_offset\":" << layer->restOffset
            << ",\"origin_restOffset\":"
            << jsonString(ctx.fields.originOf(
                   "others.layerAtts[" + std::to_string(layer->getCmper()) + "].restOffset"))
            << '}';
    }
    out << ']';
}

COVERAGE_SURVEYOR("layer_atts", writeLayerAttributes);

} // namespace
