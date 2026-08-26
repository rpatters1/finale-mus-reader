// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "import/shared/graphic_assignment.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using PageTarget = musx::dom::others::PageGraphicAssign;
using ShapeTarget = musx::dom::others::ShapeGraphicAssign;

constexpr auto pageGraphicTag = records::packTag("pg");
constexpr auto shapeGraphicTag = records::packTag("sg");
constexpr records::LegacyTag pageGraphicClass = 0x00bc;
constexpr records::LegacyTag shapeGraphicClass = 0x00d8;

const records::LegacyRow& sourceRow(const RecordFamilySource& source,
    std::span<const records::LegacyRow> rows, std::size_t wordIndex)
{
    return rows[source.classRecords ? 0 : wordIndex / records::otherWordCount];
}

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
template <typename Target>
void reportAssignmentValue(const ImportContext& context, musx::dom::Cmper cmper,
    musx::dom::Inci inci, std::string member, std::int64_t value,
    const records::LegacyRow& row)
{
    FINALE_MUS_READER_REPORT_FIELD(context.report,
        instanceKey<Target>(musx::dom::SCORE_PARTID, cmper, inci),
        std::move(member), {ValueOrigin::LegacyMus,
        row.blockOffset, row.decodedOffset, value});
}

template <typename Target>
void reportPositionValues(const ImportContext& context, musx::dom::Cmper cmper,
    musx::dom::Inci inci, std::string_view prefix, std::int64_t value,
    const records::LegacyRow& row, bool hasPositionFrom)
{
    const auto memberName = [prefix](std::string_view suffix) {
        if (!prefix.empty()) return std::string(prefix) + std::string(suffix);
        auto result = std::string(suffix);
        result.front() = static_cast<char>(result.front() - 'A' + 'a');
        return result;
    };
    reportAssignmentValue<Target>(context, cmper, inci,
        memberName("HAlign"), value, row);
    reportAssignmentValue<Target>(context, cmper, inci,
        memberName("VAlign"), value, row);
    if (hasPositionFrom) {
        reportAssignmentValue<Target>(context, cmper, inci,
            memberName("PosFrom"), value, row);
    }
    reportAssignmentValue<Target>(context, cmper, inci,
        memberName("FixedPerc"), value, row);
}
#define REPORT_ASSIGNMENT_VALUE(Target, ...) reportAssignmentValue<Target>(__VA_ARGS__)
#define REPORT_POSITION_VALUES(Target, ...) reportPositionValues<Target>(__VA_ARGS__)
#else
#define REPORT_ASSIGNMENT_VALUE(...) ((void)0)
#define REPORT_POSITION_VALUES(...) ((void)0)
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

void populatePagePosition(PageTarget& target, std::uint16_t packed, bool right)
{
    if (!right) {
        populateGraphicAssignmentPosition<true>(target, packed);
        return;
    }
    using H = PageTarget::HorizontalAlignment;
    using V = PageTarget::VerticalAlignment;
    switch (packed & 0x0007U) {
    case 0x0001: target.rightPgHAlign = H::Left; break;
    case 0x0002: target.rightPgHAlign = H::Right; break;
    case 0x0004: target.rightPgHAlign = H::Center; break;
    default: break;
    }
    switch (packed & 0x0038U) {
    case 0x0008: target.rightPgVAlign = V::Top; break;
    case 0x0010: target.rightPgVAlign = V::Bottom; break;
    case 0x0020: target.rightPgVAlign = V::Center; break;
    default: break;
    }
    target.rightPgFixedPerc = (packed & 0x0100U) != 0;
    if ((packed & 0x0080U) != 0) target.rightPgPosFrom = PageTarget::PositionFrom::PageEdge;
    else if ((packed & 0x0040U) != 0) target.rightPgPosFrom = PageTarget::PositionFrom::Margins;
}

PageTarget::PageAssignType pageAssignType(std::uint16_t raw)
{
    // Page selection occupies a one-hot low nibble; visibility is an independent bit in the
    // same display-flags word.
    switch (raw & 0x000fU) {
    case 1: return PageTarget::PageAssignType::One;
    case 2: return PageTarget::PageAssignType::AllPages;
    case 4: return PageTarget::PageAssignType::Odd;
    case 8: return PageTarget::PageAssignType::Even;
    default: return PageTarget::PageAssignType::AllPages;
    }
}

void importPageFamily(const ImportContext& context)
{
    const auto source = selectRecordFamilySource(context, context.index.getOthers(),
        context.index.getClassOthers(), pageGraphicTag, pageGraphicClass);
    if (!source) return;
    for (const auto cmper : source->pool->cmpersForTag(source->identity)) {
        const auto rows = source->pool->getArray(source->identity, cmper);
        const auto words = collectRecordWords(*source, rows, context.profile.byteOrder);
        if (words.size() % graphicAssignmentWordCount != 0) {
            context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                "Page graphic assignment " + std::to_string(cmper)
                    + " has an incomplete trailing tuple."});
        }
        for (std::size_t at = 0; at + graphicAssignmentWordCount <= words.size();
                at += graphicAssignmentWordCount) {
            const auto inci = static_cast<musx::dom::Inci>(at / graphicAssignmentWordCount);
            auto target = std::make_shared<PageTarget>(context.document,
                musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper, inci);
            const std::span<const std::int16_t> tuple(
                words.data() + at, graphicAssignmentWordCount);
            populateGraphicAssignmentCommon(*target, tuple);
            target->displayType = pageAssignType(static_cast<std::uint16_t>(tuple[7]));
            populatePagePosition(*target, static_cast<std::uint16_t>(tuple[8]), false);
            target->startPage = static_cast<musx::dom::PageCmper>(tuple[9]);
            target->endPage = static_cast<musx::dom::PageCmper>(tuple[10]);
            target->rightPgLeft = tuple[14];
            target->rightPgBottom = tuple[15];
            populatePagePosition(*target, static_cast<std::uint16_t>(tuple[16]), true);
            constexpr const char* names[] = {"version", "left", "bottom", "width", "height",
                "fDescId", nullptr, "displayType", nullptr, "startPage", "endPage",
                "savedRecord", "origWidth", "origHeight", "rightPgLeft", "rightPgBottom",
                nullptr, "graphicCmper"};
            for (std::size_t slot = 0; slot < graphicAssignmentWordCount; ++slot) {
                if (!names[slot]) continue;
                REPORT_ASSIGNMENT_VALUE(PageTarget, context, cmper, inci, names[slot], tuple[slot],
                    sourceRow(*source, rows, at + slot));
            }
            REPORT_ASSIGNMENT_VALUE(PageTarget, context, cmper, inci, "hidden", tuple[7],
                sourceRow(*source, rows, at + 7));
            REPORT_POSITION_VALUES(PageTarget, context, cmper, inci, "", tuple[8],
                sourceRow(*source, rows, at + 8), true);
            REPORT_POSITION_VALUES(PageTarget, context, cmper, inci, "rightPg", tuple[16],
                sourceRow(*source, rows, at + 16), true);
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
            context.report.setInstanceOrigin(
                instanceKey<PageTarget>(musx::dom::SCORE_PARTID, cmper, inci),
                ValueOrigin::LegacyMus);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
            context.document->getOthers()->add(PageTarget::XmlNodeName, std::move(target));
        }
    }
}

void importShapeFamily(const ImportContext& context)
{
    const auto source = selectRecordFamilySource(context, context.index.getOthers(),
        context.index.getClassOthers(), shapeGraphicTag, shapeGraphicClass);
    if (!source) return;
    for (const auto cmper : source->pool->cmpersForTag(source->identity)) {
        const auto rows = source->pool->getArray(source->identity, cmper);
        const auto words = collectRecordWords(*source, rows, context.profile.byteOrder);
        if (words.size() % graphicAssignmentWordCount != 0) {
            context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                "Shape graphic assignment " + std::to_string(cmper)
                    + " has an incomplete trailing tuple."});
        }
        for (std::size_t at = 0; at + graphicAssignmentWordCount <= words.size();
                at += graphicAssignmentWordCount) {
            const auto inci = static_cast<musx::dom::Inci>(at / graphicAssignmentWordCount);
            auto target = std::make_shared<ShapeTarget>(context.document,
                musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper, inci);
            const std::span<const std::int16_t> tuple(
                words.data() + at, graphicAssignmentWordCount);
            populateGraphicAssignmentCommon(*target, tuple);
            populateGraphicAssignmentPosition<false>(
                *target, static_cast<std::uint16_t>(tuple[8]));
            constexpr std::size_t importedSlots[] = {0, 1, 2, 3, 4, 5, 7, 11, 12, 13, 17};
            constexpr const char* names[] = {"version", "left", "bottom", "width", "height",
                "fDescId", "hidden", "savedRecord", "origWidth", "origHeight", "graphicCmper"};
            for (std::size_t index = 0; index < std::size(importedSlots); ++index) {
                const auto slot = importedSlots[index];
                REPORT_ASSIGNMENT_VALUE(ShapeTarget, context, cmper, inci, names[index], tuple[slot],
                    sourceRow(*source, rows, at + slot));
            }
            REPORT_POSITION_VALUES(ShapeTarget, context, cmper, inci, "", tuple[8],
                sourceRow(*source, rows, at + 8), false);
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
            context.report.setInstanceOrigin(
                instanceKey<ShapeTarget>(musx::dom::SCORE_PARTID, cmper, inci),
                ValueOrigin::LegacyMus);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
            context.document->getOthers()->add(ShapeTarget::XmlNodeName, std::move(target));
        }
    }
}

} // namespace

void importPageGraphicAssignments(const ImportContext& context)
{
    importPageFamily(context);
}

void importShapeGraphicAssignments(const ImportContext& context)
{
    importShapeFamily(context);
}

} // namespace others
} // namespace finale_mus_reader

#undef REPORT_ASSIGNMENT_VALUE
#undef REPORT_POSITION_VALUES
