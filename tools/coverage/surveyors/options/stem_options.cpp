// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <cstdint>
#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeStemOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::StemOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx,
        field("half_stem_length", &Target::halfStemLength), field("stem_length", &Target::stemLength),
        field("short_stem_length", &Target::shortStemLength), field("rev_stem_adj", &Target::revStemAdj),
        field("stem_width", &Target::stemWidth), field("stem_offset", &Target::stemOffset),
        field("use_stem_connections", &Target::useStemConnections), field("no_reverse_stems", &Target::noReverseStems),
        originField<Target>("origin_halfStemLength", "halfStemLength"), originField<Target>("origin_stemLength", "stemLength"),
        originField<Target>("origin_shortStemLength", "shortStemLength"), originField<Target>("origin_revStemAdj", "revStemAdj"),
        originField<Target>("origin_stemWidth", "stemWidth"), originField<Target>("origin_stemOffset", "stemOffset"),
        originField<Target>("origin_useStemConnections", "useStemConnections"),
        originField<Target>("origin_noReverseStems", "noReverseStems"));
    Value::Array connections;
    for (std::size_t index = 0; index < options->stemConnections.size(); ++index) {
        const auto& connection = options->stemConnections[index];
        const auto prefix = "stemConnections[" + std::to_string(index) + "].";
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
        Value::Object observed{{"index", index}, {"font_name", connectionFace},
            {"font_id", connection->fontId}, {"symbol", static_cast<std::uint32_t>(connection->symbol)},
            {"up_stem_vert", connection->upStemVert}, {"down_stem_vert", connection->downStemVert},
            {"up_stem_horz", connection->upStemHorz}, {"down_stem_horz", connection->downStemHorz}};
        for (const auto* member : {"fontId", "symbol", "upStemVert", "downStemVert",
                 "upStemHorz", "downStemHorz"}) {
            observed.emplace(std::string("origin_") + member,
                fieldOrigin<Target>(ctx, prefix + member));
        }
        connections.emplace_back(std::move(observed));
    }
    result.asObject().emplace("stem_connections", std::move(connections));
    return result;
}

COVERAGE_SURVEYOR("options", "stem_options", observeStemOptions);

} // namespace
