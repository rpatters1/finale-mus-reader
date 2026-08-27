// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/support/source_gate.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
omittedSlurConnectionDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    constexpr std::string_view prefix = "smart_shape_options.slur_connect_styles[type=";
    if (context.category != ReaderOnly || !comparisonPathStartsWith(context.path, prefix)) {
        return std::nullopt;
    }
    const auto close = context.path.find(']');
    if (close == std::string_view::npos) return std::nullopt;
    const auto itemPrefix = std::string(context.path.substr(0, close + 1));
    const auto type = comparisonIntegerLeaf(context.source, itemPrefix + ".type");
    std::set<std::int64_t> sourceTypes;
    std::set<std::int64_t> companionTypes;
    for (const auto& [path, value] : context.source) {
        if (comparisonPathStartsWith(path, prefix) && comparisonPathEndsWith(path, "].type") &&
            value.first.isInteger()) {
            sourceTypes.insert(value.first.asInteger());
        }
    }
    for (const auto& [path, value] : context.companion) {
        if (comparisonPathStartsWith(path, prefix) && comparisonPathEndsWith(path, "].type") &&
            value.first.isInteger()) {
            companionTypes.insert(value.first.asInteger());
        }
    }
    constexpr auto completeCount =
        static_cast<std::size_t>(
            musx::dom::options::SmartShapeOptions::SlurConnectStyleType::UnderTabNumEnd) +
        1;
    bool sourceComplete = sourceTypes.size() == completeCount;
    for (std::size_t index = 0; sourceComplete && index < completeCount; ++index) {
        sourceComplete = sourceTypes.contains(static_cast<std::int64_t>(index));
    }
    if (!type || !sourceComplete || (companionTypes.size() != 4 && companionTypes.size() != 25) ||
        companionTypes.contains(*type)) {
        return std::nullopt;
    }
    bool nonzeroLegacy = false;
    for (const auto suffix : {".connect_index", ".x", ".y"}) {
        const auto found = context.source.find(itemPrefix + suffix);
        if (found == context.source.end() || !found->second.first.isInteger() ||
            (found->second.second != "legacy-mus" && found->second.second != "finale27-default")) {
            return std::nullopt;
        }
        nonzeroLegacy = nonzeroLegacy || (found->second.second == "legacy-mus" &&
                                          found->second.first.asInteger() != 0);
    }
    return nonzeroLegacy ? DifferenceClassification::FinaleUpgradeLoss
                         : DifferenceClassification::ReaderCompletedConnectionArray;
}

std::optional<DifferenceClassification>
classifySmartShapeDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (const auto omitted = omittedSlurConnectionDifference(context)) return omitted;
    if (context.category != Differs) return std::nullopt;
    if (context.epoch == finale_mus_reader::FormatEpoch::CodaBanner &&
        std::set<std::string_view>{"smart_shape_options.slur_thickness_cp1_x",
                                   "smart_shape_options.slur_thickness_cp2_x",
                                   "smart_shape_options.slur_thickness_cp2_y"}
            .contains(context.path)) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    if (sourceIsVersion(context.epoch, context.sourceVersion,
                        finale_mus_reader::FormatEpoch::DclLegacy,
                        finale_mus_reader::versions::finale2003) &&
        comparisonPathStartsWith(context.path,
                                 "smart_shape_options.bend_curve_connect_styles[type=") &&
        (comparisonPathEndsWith(context.path, ".x") ||
         comparisonPathEndsWith(context.path, ".y"))) {
        return DifferenceClassification::FinaleUpgradeLoss;
    }
    if (context.origin != "finale27-default") return std::nullopt;
    if (std::set<std::string_view>{
            "smart_shape_options.cresc_horizontal", "smart_shape_options.cresc_line_width",
            "smart_shape_options.slur_avoid_staff_lines",
            "smart_shape_options.slur_left_break_horz_adj", "smart_shape_options.smart_line_width",
            "smart_shape_options.use_engraver_slurs"}
            .contains(context.path) ||
        (comparisonPathStartsWith(context.path, "smart_shape_options.") &&
         context.path.find("_connect_styles[type=") != std::string_view::npos &&
         (comparisonPathEndsWith(context.path, ".x") ||
          comparisonPathEndsWith(context.path, ".y")))) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

Value observeSmartShapeOptions(const SurveyContext& ctx)
{
    using Target = musx::dom::options::SmartShapeOptions;
    const auto options = ctx.document->getOptions()->get<Target>();
    if (!options) return {};

    auto result = observe(
        *options, ctx, field("short_hairpin_opening_width", &Target::shortHairpinOpeningWidth),
        field("cresc_height", &Target::crescHeight),
        field("maximum_short_hairpin_length", &Target::maximumShortHairpinLength),
        field("cresc_line_width", &Target::crescLineWidth),
        field("hook_length", &Target::hookLength),
        field("smart_line_width", &Target::smartLineWidth),
        field("show_octava_as_text", &Target::showOctavaAsText),
        field("smart_dash_on", &Target::smartDashOn),
        field("smart_dash_off", &Target::smartDashOff),
        field("cresc_horizontal", &Target::crescHorizontal), field("direction", &Target::direction),
        field("slur_thickness_cp1_x", &Target::slurThicknessCp1X),
        field("slur_thickness_cp1_y", &Target::slurThicknessCp1Y),
        field("slur_thickness_cp2_x", &Target::slurThicknessCp2X),
        field("slur_thickness_cp2_y", &Target::slurThicknessCp2Y),
        field("slur_avoid_accidentals", &Target::slurAvoidAccidentals),
        field("slur_avoid_staff_lines_amt", &Target::slurAvoidStaffLinesAmt),
        field("max_slur_stretch", &Target::maxSlurStretch),
        field("max_slur_lift", &Target::maxSlurLift), field("slur_symmetry", &Target::slurSymmetry),
        field("use_engraver_slurs", &Target::useEngraverSlurs),
        field("slur_left_break_horz_adj", &Target::slurLeftBreakHorzAdj),
        field("slur_right_break_horz_adj", &Target::slurRightBreakHorzAdj),
        field("slur_break_vert_adj", &Target::slurBreakVertAdj),
        field("slur_avoid_staff_lines", &Target::slurAvoidStaffLines),
        field("slur_padding", &Target::slurPadding), field("max_slur_angle", &Target::maxSlurAngle),
        field("slur_acci_padding", &Target::slurAcciPadding),
        field("slur_do_stretch_first", &Target::slurDoStretchFirst),
        field("slur_stretch_by_percent", &Target::slurStretchByPercent),
        field("max_slur_stretch_percent", &Target::maxSlurStretchPercent),
        field("artic_avoid_slur_amt", &Target::articAvoidSlurAmt),
        field("line_style_custom", &Target::ssLineStyleCmpCustom),
        field("line_style_glissando", &Target::ssLineStyleCmpGlissando),
        field("line_style_tab_slide", &Target::ssLineStyleCmpTabSlide),
        field("line_style_tab_bend_curve", &Target::ssLineStyleCmpTabBendCurve),
        field("smart_slur_tip_width", &Target::smartSlurTipWidth),
        field("guitar_bend_use_parens", &Target::guitarBendUseParens),
        field("guitar_bend_hide_bend_to", &Target::guitarBendHideBendTo),
        field("guitar_bend_gen_text", &Target::guitarBendGenText),
        field("guitar_bend_use_full", &Target::guitarBendUseFull));

    for (const auto* member : {"shortHairpinOpeningWidth",
                               "crescHeight",
                               "maximumShortHairpinLength",
                               "crescLineWidth",
                               "hookLength",
                               "smartLineWidth",
                               "showOctavaAsText",
                               "smartDashOn",
                               "smartDashOff",
                               "crescHorizontal",
                               "direction",
                               "slurThicknessCp1X",
                               "slurThicknessCp1Y",
                               "slurThicknessCp2X",
                               "slurThicknessCp2Y",
                               "slurAvoidAccidentals",
                               "slurAvoidStaffLinesAmt",
                               "maxSlurStretch",
                               "maxSlurLift",
                               "slurSymmetry",
                               "useEngraverSlurs",
                               "slurLeftBreakHorzAdj",
                               "slurRightBreakHorzAdj",
                               "slurBreakVertAdj",
                               "slurAvoidStaffLines",
                               "slurPadding",
                               "maxSlurAngle",
                               "slurAcciPadding",
                               "slurDoStretchFirst",
                               "slurStretchByPercent",
                               "maxSlurStretchPercent",
                               "articAvoidSlurAmt",
                               "ssLineStyleCmpCustom",
                               "ssLineStyleCmpGlissando",
                               "ssLineStyleCmpTabSlide",
                               "ssLineStyleCmpTabBendCurve",
                               "smartSlurTipWidth",
                               "guitarBendUseParens",
                               "guitarBendHideBendTo",
                               "guitarBendGenText",
                               "guitarBendUseFull"}) {
        result.asObject().emplace(std::string("origin_") + member,
                                  fieldOrigin<Target>(ctx, member));
    }

    std::vector<std::pair<Target::SlurControlStyleType, std::shared_ptr<Target::ControlStyle>>>
        controlTypes(options->slurControlStyles.begin(), options->slurControlStyles.end());
    std::ranges::sort(controlTypes, {}, [](const auto& item) { return item.first; });
    Value::Array controls;
    for (std::size_t index = 0; index < controlTypes.size(); ++index) {
        const auto& [type, item] = controlTypes[index];
        const auto& style = *item;
        const auto prefix = "slurControlStyles[" + std::to_string(index) + "].";
        Value::Object observed{{"type", static_cast<std::int64_t>(type)},
                               {"span", style.span},
                               {"inset", style.inset},
                               {"height", style.height}};
        for (const auto* member : {"span", "inset", "height"}) {
            observed.emplace(std::string("origin_") + member,
                             fieldOrigin<Target>(ctx, prefix + member));
        }
        controls.emplace_back(std::move(observed));
    }
    result.asObject().emplace("slur_control_styles", std::move(controls));

    const auto addConnectionStyles = [&ctx, &result]<typename Map>(std::string_view outputName,
                                                                   std::string_view reportName,
                                                                   const Map& styles) {
        std::vector<std::pair<typename Map::key_type, typename Map::mapped_type>> ordered(
            styles.begin(), styles.end());
        std::ranges::sort(ordered, {}, [](const auto& item) { return item.first; });
        Value::Array observedStyles;
        for (std::size_t index = 0; index < ordered.size(); ++index) {
            const auto& [type, item] = ordered[index];
            const auto prefix = std::string(reportName) + "[" + std::to_string(index) + "].";
            Value::Object observed{{"type", static_cast<std::int64_t>(type)},
                                   {"connect_index", static_cast<std::int64_t>(item->connectIndex)},
                                   {"x", item->xOffset},
                                   {"y", item->yOffset}};
            observed.emplace("origin_connectIndex",
                             fieldOrigin<Target>(ctx, prefix + "connectIndex"));
            observed.emplace("origin_x", fieldOrigin<Target>(ctx, prefix + "xOffset"));
            observed.emplace("origin_y", fieldOrigin<Target>(ctx, prefix + "yOffset"));
            observedStyles.emplace_back(std::move(observed));
        }
        result.asObject().emplace(std::string(outputName), std::move(observedStyles));
    };
    addConnectionStyles("slur_connect_styles", "slurConnectStyles", options->slurConnectStyles);
    addConnectionStyles("tab_slide_connect_styles", "tabSlideConnectStyles",
                        options->tabSlideConnectStyles);
    addConnectionStyles("glissando_connect_styles", "glissandoConnectStyles",
                        options->glissandoConnectStyles);
    addConnectionStyles("bend_curve_connect_styles", "bendCurveConnectStyles",
                        options->bendCurveConnectStyles);
    return result;
}

COVERAGE_CLASS("options", "smart_shape_options", observeSmartShapeOptions,
               classifySmartShapeDifference);

} // namespace
