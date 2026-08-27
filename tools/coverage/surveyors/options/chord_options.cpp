// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cmath>
#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/support/source_gate.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

bool equivalentChordDifference(const DifferenceContext& context)
{
    constexpr double scalingTolerance = 0.010001;
    return context.category == DifferenceCategory::Differs &&
           (context.path == "chord_options.fret_percent" ||
            context.path == "chord_options.chord_percent") &&
           context.sourceValue.isDouble() && context.companionValue.isDouble() &&
           std::abs(context.sourceValue.asDouble() - context.companionValue.asDouble()) <=
               scalingTolerance;
}

std::optional<DifferenceClassification> classifyChordDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category != Differs || context.origin != "finale27-default") {
        return std::nullopt;
    }
    if (context.path == "chord_options.use_fretboard_font" && context.sourceValue.isBool() &&
        !context.sourceValue.asBool() && context.companionValue.isBool() &&
        context.companionValue.asBool()) {
        return DifferenceClassification::DifferentDefaults;
    }
    if (context.path == "chord_options.use_simple_chord_spelling" &&
        sourceIsVersion(context.epoch, context.sourceVersion,
                        finale_mus_reader::FormatEpoch::UncompressedLegacy,
                        finale_mus_reader::versions::finale3_0) &&
        context.sourceValue.isBool() && context.sourceValue.asBool() &&
        context.companionValue.isBool() && !context.companionValue.asBool()) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

Value observeChordOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::ChordOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};
    auto result = observe(*options, ctx, field("fret_percent", &Target::fretPercent),
                          field("chord_percent", &Target::chordPercent),
                          field("chord_sharp_lift", &Target::chordSharpLift),
                          field("chord_flat_lift", &Target::chordFlatLift),
                          field("chord_natural_lift", &Target::chordNaturalLift),
                          field("show_fretboards", &Target::showFretboards),
                          field("fret_style_id", &Target::fretStyleId),
                          field("fret_inst_id", &Target::fretInstId),
                          field("multi_fret_items_per_string", &Target::multiFretItemsPerStr),
                          field("use_fretboard_font", &Target::useFretboardFont),
                          field("italicize_capo_chords", &Target::italicizeCapoChords),
                          field("chord_alignment", &Target::chordAlignment),
                          field("chord_style", &Target::chordStyle),
                          field("use_simple_chord_spelling", &Target::useSimpleChordSpelling),
                          field("chord_playback", &Target::chordPlayback));
    for (const auto* member :
         {"fretPercent", "chordPercent", "chordSharpLift", "chordFlatLift", "chordNaturalLift",
          "showFretboards", "fretStyleId", "fretInstId", "multiFretItemsPerStr", "useFretboardFont",
          "italicizeCapoChords", "chordAlignment", "chordStyle", "useSimpleChordSpelling",
          "chordPlayback"}) {
        result.asObject().emplace(std::string("origin_") + member,
                                  fieldOrigin<Target>(ctx, member));
    }
    return result;
}

COVERAGE_CLASS_WITH_EQUIVALENCE("options", "chord_options", observeChordOptions,
                                classifyChordDifference, equivalentChordDifference);

} // namespace
