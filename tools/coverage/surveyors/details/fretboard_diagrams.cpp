// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeFretboardDiagrams(const SurveyContext& ctx)
{
    using Target = musx::dom::details::FretboardDiagram;
    Value::Array result;
    for (const auto& diagram : sourceInstances<Target>(ctx)) {
        Value::Array cells;
        for (const auto& cell : diagram->cells) {
            cells.emplace_back(Value::Object{{"string", cell->string}, {"fret", cell->fret},
                {"shape", static_cast<std::int64_t>(cell->shape)},
                {"finger_num", cell->fingerNum}});
        }
        Value::Array barres;
        for (const auto& barre : diagram->barres) {
            barres.emplace_back(Value::Object{{"fret", barre->fret},
                {"start_string", barre->startString}, {"end_string", barre->endString}});
        }
        result.emplace_back(observe(*diagram, ctx,
            field("cmper1", [](const Target& value) { return value.getCmper1(); }),
            field("cmper2", [](const Target& value) { return value.getCmper2(); }),
            field("num_frets", &Target::numFrets), field("fretboard_num", &Target::fretboardNum),
            field("lock", &Target::lock), field("show_num", &Target::showNum),
            field("num_fret_cells", &Target::numFretCells),
            field("num_fret_barres", &Target::numFretBarres),
            field("cells", [cells = std::move(cells)](const Target&) { return Value(cells); }),
            field("barres", [barres = std::move(barres)](const Target&) {
                return Value(barres);
            })));
    }
    return result;
}

COVERAGE_SURVEYOR("details", "fretboard_diagrams", observeFretboardDiagrams);

} // namespace
