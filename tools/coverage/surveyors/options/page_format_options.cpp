// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "coverage/common/page_format_info.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;
using PageFormatOptionsTarget = musx::dom::options::PageFormatOptions;
using PageFormatTarget = PageFormatOptionsTarget::PageFormat;

Value observePageFormat(const PageFormatTarget& format, const SurveyContext& ctx,
    std::string_view memberPrefix)
{
    auto result = observe(format, ctx, field("page_height", &PageFormatTarget::pageHeight),
        field("page_width", &PageFormatTarget::pageWidth),
        field("page_percent", &PageFormatTarget::pagePercent),
        field("sys_percent", &PageFormatTarget::sysPercent),
        field("raw_staff_height", &PageFormatTarget::rawStaffHeight),
        field("left_page_margin_top", &PageFormatTarget::leftPageMarginTop),
        field("left_page_margin_left", &PageFormatTarget::leftPageMarginLeft),
        field("left_page_margin_bottom", &PageFormatTarget::leftPageMarginBottom),
        field("left_page_margin_right", &PageFormatTarget::leftPageMarginRight),
        field("right_page_margin_top", &PageFormatTarget::rightPageMarginTop),
        field("right_page_margin_left", &PageFormatTarget::rightPageMarginLeft),
        field("right_page_margin_bottom", &PageFormatTarget::rightPageMarginBottom),
        field("right_page_margin_right", &PageFormatTarget::rightPageMarginRight),
        field("sys_margin_top", &PageFormatTarget::sysMarginTop),
        field("sys_margin_left", &PageFormatTarget::sysMarginLeft),
        field("sys_margin_bottom", &PageFormatTarget::sysMarginBottom),
        field("sys_margin_right", &PageFormatTarget::sysMarginRight),
        field("sys_distance_between", &PageFormatTarget::sysDistanceBetween),
        field("first_page_margin_top", &PageFormatTarget::firstPageMarginTop),
        field("first_sys_margin_top", &PageFormatTarget::firstSysMarginTop),
        field("first_sys_margin_left", &PageFormatTarget::firstSysMarginLeft),
        field("first_sys_margin_distance", &PageFormatTarget::firstSysMarginDistance),
        field("facing_pages", &PageFormatTarget::facingPages),
        field("different_first_sys_margin", &PageFormatTarget::differentFirstSysMargin),
        field("different_first_page_margin", &PageFormatTarget::differentFirstPageMargin));

    auto& fields = result.asObject();
#define ADD_PAGE_FORMAT_ORIGIN(member) \
    fields.emplace("origin_" #member, fieldOrigin<PageFormatOptionsTarget>( \
        ctx, std::string(memberPrefix).append(#member)))
    ADD_PAGE_FORMAT_ORIGIN(pageHeight);
    ADD_PAGE_FORMAT_ORIGIN(pageWidth);
    ADD_PAGE_FORMAT_ORIGIN(pagePercent);
    ADD_PAGE_FORMAT_ORIGIN(sysPercent);
    ADD_PAGE_FORMAT_ORIGIN(rawStaffHeight);
    ADD_PAGE_FORMAT_ORIGIN(leftPageMarginTop);
    ADD_PAGE_FORMAT_ORIGIN(leftPageMarginLeft);
    ADD_PAGE_FORMAT_ORIGIN(leftPageMarginBottom);
    ADD_PAGE_FORMAT_ORIGIN(leftPageMarginRight);
    ADD_PAGE_FORMAT_ORIGIN(rightPageMarginTop);
    ADD_PAGE_FORMAT_ORIGIN(rightPageMarginLeft);
    ADD_PAGE_FORMAT_ORIGIN(rightPageMarginBottom);
    ADD_PAGE_FORMAT_ORIGIN(rightPageMarginRight);
    ADD_PAGE_FORMAT_ORIGIN(sysMarginTop);
    ADD_PAGE_FORMAT_ORIGIN(sysMarginLeft);
    ADD_PAGE_FORMAT_ORIGIN(sysMarginBottom);
    ADD_PAGE_FORMAT_ORIGIN(sysMarginRight);
    ADD_PAGE_FORMAT_ORIGIN(sysDistanceBetween);
    ADD_PAGE_FORMAT_ORIGIN(firstPageMarginTop);
    ADD_PAGE_FORMAT_ORIGIN(firstSysMarginTop);
    ADD_PAGE_FORMAT_ORIGIN(firstSysMarginLeft);
    ADD_PAGE_FORMAT_ORIGIN(firstSysMarginDistance);
    ADD_PAGE_FORMAT_ORIGIN(facingPages);
    ADD_PAGE_FORMAT_ORIGIN(differentFirstSysMargin);
    ADD_PAGE_FORMAT_ORIGIN(differentFirstPageMargin);
#undef ADD_PAGE_FORMAT_ORIGIN
    return result;
}

Value observePageFormatOptions(const SurveyContext& ctx)
{
    const auto options = ctx.document->getOptions()->get<PageFormatOptionsTarget>();
    if (!options || !options->pageFormatScore || !options->pageFormatParts) return {};

    auto result = observe(*options, ctx,
        field("adjust_page_scope", &PageFormatOptionsTarget::adjustPageScope),
        field("avoid_system_margin_collisions",
            &PageFormatOptionsTarget::avoidSystemMarginCollisions),
        originField<PageFormatOptionsTarget>("origin_adjustPageScope", "adjustPageScope"),
        originField<PageFormatOptionsTarget>("origin_avoidSystemMarginCollisions",
            "avoidSystemMarginCollisions"));
    auto& fields = result.asObject();
    fields.emplace("page_format_score",
        observePageFormat(*options->pageFormatScore, ctx, "pageFormatScore."));
    fields.emplace("page_format_parts",
        observePageFormat(*options->pageFormatParts, ctx, "pageFormatParts."));
    return result;
}

COVERAGE_CLASS("options", "page_format_options", observePageFormatOptions,
    classifyPageFormatOptionsDifference);

} // namespace
