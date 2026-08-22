// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeStemOptions(std::ostream& out, const SurveyContext& ctx)
{
    const auto options = ctx.document->getOptions()->get<musx::dom::options::StemOptions>();
    if (!options) {
        out << "null";
        return;
    }
    out << '{'
        << "\"half_stem_length\":" << options->halfStemLength
        << ",\"stem_length\":" << options->stemLength
        << ",\"short_stem_length\":" << options->shortStemLength
        << ",\"rev_stem_adj\":" << options->revStemAdj
        << ",\"stem_width\":" << options->stemWidth
        << ",\"stem_offset\":" << options->stemOffset
        << ",\"use_stem_connections\":" << jsonBool(options->useStemConnections)
        << ",\"no_reverse_stems\":" << jsonBool(options->noReverseStems);
    for (const auto* member : {"halfStemLength", "stemLength", "shortStemLength", "revStemAdj",
             "stemWidth", "stemOffset", "useStemConnections", "noReverseStems"}) {
        out << ",\"origin_" << member << "\":"
            << jsonString(ctx.fields.originOf(std::string("options.stemOptions.") + member));
    }
    out << ",\"stem_connections\":[";
    for (std::size_t index = 0; index < options->stemConnections.size(); ++index) {
        const auto& connection = options->stemConnections[index];
        const auto prefix = "options.stemOptions.stemConnections[" + std::to_string(index) + "].";
        if (index) out << ',';
        // The face as well as the comparator. Finale 27 matches fonts against those installed
        // on the upgrading machine and renumbers its own table accordingly, so comparing
        // comparators against a companion manufactures disagreements; the face is what both
        // sides agree about.
        std::string connectionFace;
        if (const auto definition = ctx.document->getOthers()
                ->get<musx::dom::others::FontDefinition>(
                    musx::dom::SCORE_PARTID, connection->fontId)) {
            connectionFace = definition->name;
        }
        out << "{\"index\":" << index
            << ",\"font_name\":" << jsonString(connectionFace)
            << ",\"font_id\":" << connection->fontId
            << ",\"symbol\":" << static_cast<std::uint32_t>(connection->symbol)
            << ",\"up_stem_vert\":" << connection->upStemVert
            << ",\"down_stem_vert\":" << connection->downStemVert
            << ",\"up_stem_horz\":" << connection->upStemHorz
            << ",\"down_stem_horz\":" << connection->downStemHorz;
        for (const auto* member : {"fontId", "symbol", "upStemVert", "downStemVert",
                 "upStemHorz", "downStemHorz"}) {
            out << ",\"origin_" << member << "\":" << jsonString(ctx.fields.originOf(prefix + member));
        }
        out << '}';
    }
    out << "]}";
}

COVERAGE_SURVEYOR("stem_options", writeStemOptions);

} // namespace
