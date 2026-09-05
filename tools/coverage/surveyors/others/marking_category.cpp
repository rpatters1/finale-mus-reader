// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <array>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/support/source_gate.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

using CategorySurveyTarget = musx::dom::others::MarkingCategory;
using NameSurveyTarget = musx::dom::others::MarkingCategoryName;

std::optional<DifferenceClassification>
classifyMarkingCategoryDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    constexpr std::array fontPaths{"].text_font.", "].music_font.", "].number_font."};
    if (context.category != Differs ||
        !comparisonPathStartsWith(context.path, "marking_categories[")) {
        return std::nullopt;
    }
    if (context.origin == "finale27-default") {
        for (const auto path : fontPaths) {
            if (context.path.find(path) != std::string_view::npos) {
                return DifferenceClassification::DifferentDefaults;
            }
        }
    }
    if (context.origin == "legacy-mus" &&
        comparisonPathEndsWith(context.path, ".font_id") && context.sourceValue.isInteger() &&
        context.companionValue.isInteger() &&
        context.sourceValue == context.companionValue &&
        !context.companionFontIdentity.empty()) {
        const auto fontId = context.sourceValue.asInteger();
        if (fontId > 0 && fontId <= (std::numeric_limits<musx::dom::Cmper>::max)()) {
            const auto* sourceName =
                context.sourceReport.findField<musx::dom::others::FontDefinition>(
                    "name", musx::dom::SCORE_PARTID, musx::dom::Cmper(fontId));
            if (!sourceName || sourceName->origin != finale_mus_reader::ValueOrigin::LegacyMus) {
                return DifferenceClassification::FinaleUpgradeLoss;
            }
        }
    }
    if (context.origin == "legacy-mus" && sourceIsBeta(context.sourceVersion) &&
        sourceIsVersion(context.epoch, context.sourceVersion,
            finale_mus_reader::FormatEpoch::ZlibLegacy,
            finale_mus_reader::versions::finale2009)) {
        return DifferenceClassification::BetaDiscrepancy;
    }
    return std::nullopt;
}

Value observeMarkingCategoryFont(const musx::dom::FontInfo& font,
    const SurveyContext& context,
    const CategorySurveyTarget& category,
    std::string_view member)
{
    const auto origin = [&](std::string_view leaf) {
        return fieldOrigin<CategorySurveyTarget>(
            context, std::string(member) + "." + std::string(leaf), category);
    };
    return Value::Object{{"font_id", font.fontId}, {"font_size", font.fontSize},
        {"bold", font.bold}, {"italic", font.italic}, {"underline", font.underline},
        {"strikeout", font.strikeout}, {"absolute", font.absolute}, {"hidden", font.hidden},
        {"origin_fontId", origin("fontId")}, {"origin_fontSize", origin("fontSize")},
        {"origin_bold", origin("bold")}, {"origin_italic", origin("italic")},
        {"origin_underline", origin("underline")}, {"origin_strikeout", origin("strikeout")},
        {"origin_absolute", origin("absolute")}, {"origin_hidden", origin("hidden")}};
}

auto markingCategoryOrigin(const char* member)
{
    return [member](const CategorySurveyTarget& value, const SurveyContext& context) {
        return fieldOrigin<CategorySurveyTarget>(context, member, value);
    };
}

Value observeMarkingCategories(const SurveyContext& context)
{
    Value::Array result;
    for (const auto& category : sourceInstances<CategorySurveyTarget>(context)) {
        result.emplace_back(observe(*category, context,
            field("cmper", [](const CategorySurveyTarget& value) { return value.getCmper(); }),
            field("category_type", &CategorySurveyTarget::categoryType),
            field("text_font",
                [&context](const CategorySurveyTarget& value) {
                    return observeMarkingCategoryFont(*value.textFont, context, value, "textFont");
                }),
            field("music_font",
                [&context](const CategorySurveyTarget& value) {
                    return observeMarkingCategoryFont(
                        *value.musicFont, context, value, "musicFont");
                }),
            field("number_font",
                [&context](const CategorySurveyTarget& value) {
                    return observeMarkingCategoryFont(
                        *value.numberFont, context, value, "numberFont");
                }),
            field("justification", &CategorySurveyTarget::justification),
            field("horz_align", &CategorySurveyTarget::horzAlign),
            field("vert_align", &CategorySurveyTarget::vertAlign),
            field("horz_offset", &CategorySurveyTarget::horzOffset),
            field("vert_offset_baseline", &CategorySurveyTarget::vertOffsetBaseline),
            field("vert_offset_entry", &CategorySurveyTarget::vertOffsetEntry),
            field("uses_text_font", &CategorySurveyTarget::usesTextFont),
            field("uses_music_font", &CategorySurveyTarget::usesMusicFont),
            field("uses_number_font", &CategorySurveyTarget::usesNumberFont),
            field("uses_positioning", &CategorySurveyTarget::usesPositioning),
            field("uses_staff_list", &CategorySurveyTarget::usesStaffList),
            field("uses_break_mm_rests", &CategorySurveyTarget::usesBreakMmRests),
            field("break_mm_rest", &CategorySurveyTarget::breakMmRest),
            field("user_created", &CategorySurveyTarget::userCreated),
            field("staff_list", &CategorySurveyTarget::staffList),
            field("origin_categoryType", markingCategoryOrigin("categoryType")),
            field("origin_justification", markingCategoryOrigin("justification")),
            field("origin_horzAlign", markingCategoryOrigin("horzAlign")),
            field("origin_vertAlign", markingCategoryOrigin("vertAlign")),
            field("origin_horzOffset", markingCategoryOrigin("horzOffset")),
            field("origin_vertOffsetBaseline", markingCategoryOrigin("vertOffsetBaseline")),
            field("origin_vertOffsetEntry", markingCategoryOrigin("vertOffsetEntry")),
            field("origin_usesTextFont", markingCategoryOrigin("usesTextFont")),
            field("origin_usesMusicFont", markingCategoryOrigin("usesMusicFont")),
            field("origin_usesNumberFont", markingCategoryOrigin("usesNumberFont")),
            field("origin_usesPositioning", markingCategoryOrigin("usesPositioning")),
            field("origin_usesStaffList", markingCategoryOrigin("usesStaffList")),
            field("origin_usesBreakMmRests", markingCategoryOrigin("usesBreakMmRests")),
            field("origin_breakMmRest", markingCategoryOrigin("breakMmRest")),
            field("origin_userCreated", markingCategoryOrigin("userCreated")),
            field("origin_staffList", markingCategoryOrigin("staffList"))));
    }
    return Value(std::move(result));
}

Value observeMarkingCategoryNames(const SurveyContext& context)
{
    Value::Array result;
    for (const auto& name : sourceInstances<NameSurveyTarget>(context)) {
        result.emplace_back(observe(*name, context,
            field("cmper", [](const NameSurveyTarget& value) { return value.getCmper(); }),
            field("name", &NameSurveyTarget::name),
            field("origin_name", [](const NameSurveyTarget& value, const SurveyContext& ctx) {
                return fieldOrigin<NameSurveyTarget>(ctx, "name", value);
            })));
    }
    return Value(std::move(result));
}

COVERAGE_CLASS("others", "marking_categories", observeMarkingCategories,
    classifyMarkingCategoryDifference);
COVERAGE_SURVEYOR("others", "marking_category_names", observeMarkingCategoryNames);

} // namespace
