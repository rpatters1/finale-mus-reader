// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others/others.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "import/graphic_assignment.h"
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

struct AssignmentSource
{
    const records::LegacyRowPool* pool{};
    records::LegacyTag identity{};
    bool classRecords{};
};

AssignmentSource assignmentSource(const ImportContext& context,
    records::LegacyTag fixedTag, records::LegacyTag classId)
{
    if (context.profile.epoch == FormatEpoch::ZlibLegacy) {
        return {&context.index.getClassOthers(), classId, true};
    }
    return {&context.index.getOthers(), fixedTag, false};
}

std::vector<std::int16_t> assignmentWords(const AssignmentSource& source,
    std::span<const records::LegacyRow> rows, ByteOrder byteOrder)
{
    std::vector<std::int16_t> result;
    for (const auto& row : rows) {
        if (source.classRecords) {
            const auto words = payloadWords(source.pool->payloadOf(row), byteOrder);
            result.insert(result.end(), words.begin(), words.end());
        } else {
            result.insert(result.end(), row.words.begin(), row.words.begin() + row.wordCount);
        }
    }
    return result;
}

const records::LegacyRow& sourceRow(const AssignmentSource& source,
    std::span<const records::LegacyRow> rows, std::size_t wordIndex)
{
    return rows[source.classRecords ? 0 : wordIndex / records::otherWordCount];
}

void reportAssignmentValue(const ImportContext& context, std::string target,
    std::int64_t value, const records::LegacyRow& row)
{
    context.report.fields.push_back({std::move(target), ValueOrigin::LegacyMus,
        row.blockOffset, row.decodedOffset, value});
}

template <typename Target>
void populatePosition(Target& target, std::uint16_t packed)
{
    using H = typename Target::HorizontalAlignment;
    using V = typename Target::VerticalAlignment;
    switch (packed & 0x0007U) {
    case 0x0001: target.hAlign = H::Left; break;
    case 0x0002: target.hAlign = H::Right; break;
    case 0x0004: target.hAlign = H::Center; break;
    default: break;
    }
    switch (packed & 0x0038U) {
    case 0x0008: target.vAlign = V::Top; break;
    case 0x0010: target.vAlign = V::Bottom; break;
    case 0x0020: target.vAlign = V::Center; break;
    default: break;
    }
    target.fixedPerc = (packed & 0x0100U) != 0;
}

void populatePagePosition(PageTarget& target, std::uint16_t packed, bool right)
{
    if (!right) {
        populatePosition(target, packed);
        if ((packed & 0x0080U) != 0) target.posFrom = PageTarget::PositionFrom::PageEdge;
        else if ((packed & 0x0040U) != 0) target.posFrom = PageTarget::PositionFrom::Margins;
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
    // The source is a one-hot page-selection mask. Every surveyed assignment is One;
    // the other three values preserve the same bit spelling used by Finale's related
    // page-assignment records.
    switch (raw) {
    case 1: return PageTarget::PageAssignType::One;
    case 2: return PageTarget::PageAssignType::Odd;
    case 4: return PageTarget::PageAssignType::Even;
    default: return PageTarget::PageAssignType::AllPages;
    }
}

void importPageFamily(const ImportContext& context)
{
    const auto source = assignmentSource(context, pageGraphicTag, pageGraphicClass);
    for (const auto cmper : source.pool->cmpersForTag(source.identity)) {
        const auto rows = source.pool->getArray(source.identity, cmper);
        const auto words = assignmentWords(source, rows, context.profile.byteOrder);
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
            const auto prefix = "others.pageGraphicAssign[" + std::to_string(cmper)
                + "][" + std::to_string(inci) + "].";
            constexpr const char* names[] = {"version", "left", "bottom", "width", "height",
                "fDescId", "hidden", "displayType", "position", "startPage", "endPage",
                "savedRecord", "origWidth", "origHeight", "rightPgLeft", "rightPgBottom",
                "rightPgPosition", "graphicCmper"};
            for (std::size_t slot = 0; slot < graphicAssignmentWordCount; ++slot) {
                reportAssignmentValue(context, prefix + names[slot], tuple[slot],
                    sourceRow(source, rows, at + slot));
            }
            context.document->getOthers()->add(PageTarget::XmlNodeName, target);
        }
    }
}

void importShapeFamily(const ImportContext& context)
{
    const auto source = assignmentSource(context, shapeGraphicTag, shapeGraphicClass);
    for (const auto cmper : source.pool->cmpersForTag(source.identity)) {
        const auto rows = source.pool->getArray(source.identity, cmper);
        const auto words = assignmentWords(source, rows, context.profile.byteOrder);
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
            populatePosition(*target, static_cast<std::uint16_t>(tuple[8]));
            const auto prefix = "others.shapeGraphicAssign[" + std::to_string(cmper)
                + "][" + std::to_string(inci) + "].";
            constexpr std::size_t importedSlots[] = {0, 1, 2, 3, 4, 5, 6, 8, 11, 12, 13, 17};
            constexpr const char* names[] = {"version", "left", "bottom", "width", "height",
                "fDescId", "hidden", "position", "savedRecord", "origWidth", "origHeight",
                "graphicCmper"};
            for (std::size_t index = 0; index < std::size(importedSlots); ++index) {
                const auto slot = importedSlots[index];
                reportAssignmentValue(context, prefix + names[index], tuple[slot],
                    sourceRow(source, rows, at + slot));
            }
            context.document->getOthers()->add(ShapeTarget::XmlNodeName, target);
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
