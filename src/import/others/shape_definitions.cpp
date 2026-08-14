// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others/others.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using ShapeDataTarget = musx::dom::others::ShapeData;
using ShapeDefTarget = musx::dom::others::ShapeDef;
using ShapeInstructionTarget = musx::dom::others::ShapeInstructionList;
using ShapeInstructionType = musx::dom::ShapeDefInstructionType;

constexpr auto shapeDefinitionTag = records::packTag("SD");
constexpr auto earlyShapeInstructionTag = records::packTag("SL");
constexpr auto shapeInstructionTag = records::packTag("sL");
constexpr auto earlyShapeDataTag = records::packTag("SB");
constexpr auto shapeDataTag = records::packTag("sb");

// The zlib serialization gives the three former tagged families consecutive class ids.
// Exact Finale 2007 and 2012 source/conversion pairs establish which payload is which:
// 0x00d5 is the long-valued data stream, 0x00d6 is the six-word definition, and 0x00d7
// is the packed instruction stream.
constexpr records::LegacyTag shapeDataClass = 0x00d5;
constexpr records::LegacyTag shapeDefinitionClass = 0x00d6;
constexpr records::LegacyTag shapeInstructionClass = 0x00d7;

struct ShapeSourceFamily
{
    const records::LegacyRowPool* pool{};
    records::LegacyTag definition{};
    records::LegacyTag instructions{};
    records::LegacyTag data{};
    bool earlyData{};
};

ShapeSourceFamily sourceFamily(const ImportContext& context)
{
    if (context.profile.epoch == FormatEpoch::ZlibLegacy) {
        return {&context.index.getClassRecords(), shapeDefinitionClass,
            shapeInstructionClass, shapeDataClass, false};
    }
    if (context.profile.epoch == FormatEpoch::CodaBanner) {
        return {&context.index.getOthers(), shapeDefinitionTag,
            earlyShapeInstructionTag, earlyShapeDataTag, true};
    }
    // The uncompressed and DCL epochs deliberately share the same fixed-row spelling.
    return {&context.index.getOthers(), shapeDefinitionTag,
        shapeInstructionTag, shapeDataTag, false};
}

std::uint32_t combineShapeWords(std::int16_t first, std::int16_t second,
    SourcePlatform platform)
{
    const auto a = static_cast<std::uint16_t>(first);
    const auto b = static_cast<std::uint16_t>(second);
    // Fixed-row four-byte values use the platform's historical word order after the
    // record index has normalized each word. The Windows form is low-word first; the Mac
    // form is high-word first. Zlib payloads are read separately as ordinary four-byte
    // numbers in the container's byte order and never reach this branch.
    return platform == SourcePlatform::Windows
        ? (static_cast<std::uint32_t>(b) << 16U) | a
        : (static_cast<std::uint32_t>(a) << 16U) | b;
}

std::int32_t readShapeLong(const std::uint8_t* bytes, ByteOrder order)
{
    const auto value = order == ByteOrder::BigEndian
        ? (static_cast<std::uint32_t>(bytes[0]) << 24U)
            | (static_cast<std::uint32_t>(bytes[1]) << 16U)
            | (static_cast<std::uint32_t>(bytes[2]) << 8U) | bytes[3]
        : static_cast<std::uint32_t>(bytes[0])
            | (static_cast<std::uint32_t>(bytes[1]) << 8U)
            | (static_cast<std::uint32_t>(bytes[2]) << 16U)
            | (static_cast<std::uint32_t>(bytes[3]) << 24U);
    return static_cast<std::int32_t>(value);
}

std::vector<std::int32_t> fixedLongs(std::span<const records::LegacyRow> rows,
    SourcePlatform platform)
{
    std::vector<std::int32_t> result;
    result.reserve(rows.size() * 3);
    for (const auto& row : rows) {
        for (std::size_t i = 0; i + 1 < row.wordCount; i += 2) {
            result.push_back(static_cast<std::int32_t>(
                combineShapeWords(row.words[i], row.words[i + 1], platform)));
        }
    }
    return result;
}

std::vector<std::int32_t> classLongs(const records::LegacyRowPool& pool,
    const records::LegacyRow& row, ByteOrder order)
{
    const auto payload = pool.payloadOf(row);
    std::vector<std::int32_t> result;
    result.reserve(payload.size() / 4);
    for (std::size_t at = 0; at + 4 <= payload.size(); at += 4) {
        result.push_back(readShapeLong(payload.data() + at, order));
    }
    return result;
}

std::vector<std::int32_t> shapeLongs(const ShapeSourceFamily& source,
    records::LegacyTag identity, std::uint16_t cmper, const ImportContext& context)
{
    const auto rows = source.pool->getArray(identity, cmper);
    if (rows.empty()) {
        return {};
    }
    if (context.profile.epoch == FormatEpoch::ZlibLegacy) {
        return classLongs(*source.pool, rows.front(), context.profile.byteOrder);
    }
    return fixedLongs(rows, context.profile.platform);
}

void reportShapeValue(ImportReport& report, std::string target, std::int64_t value,
    const records::LegacyRow& row)
{
    report.fields.push_back({std::move(target), ValueOrigin::LegacyMus,
        row.blockOffset, row.decodedOffset, value});
}

ShapeInstructionType instructionType(records::LegacyTag tag, bool early)
{
    using records::packTag;
    switch (tag) {
    case packTag("br"): return ShapeInstructionType::Bracket;
    case packTag("cc"): return ShapeInstructionType::CloneChar;
    case packTag("cp"): return ShapeInstructionType::ClosePath;
    case packTag("cv"): return ShapeInstructionType::CurveTo;
    case packTag("dc"): return ShapeInstructionType::DrawChar;
    case packTag("el"): return ShapeInstructionType::Ellipse;
    case packTag("er"): return ShapeInstructionType::EndGroup;
    case packTag("xg"): return ShapeInstructionType::ExternalGraphic;
    case packTag("fl"): return ShapeInstructionType::FillAlt;
    case packTag("fs"): return ShapeInstructionType::FillSolid;
    case packTag("go"): return ShapeInstructionType::GoToOrigin;
    // Before Finale 3.0 this instruction meant origin, not the beginning of the path.
    case packTag("gs"): return early ? ShapeInstructionType::GoToOrigin
                                      : ShapeInstructionType::GoToStart;
    case packTag("lw"): return ShapeInstructionType::LineWidth;
    case packTag("sw"): return ShapeInstructionType::LineWidth;
    case packTag("re"): return ShapeInstructionType::Rectangle;
    case packTag("rl"): return ShapeInstructionType::RLineTo;
    case packTag("rm"): return ShapeInstructionType::RMoveTo;
    case packTag("sa"): return ShapeInstructionType::SetArrowhead;
    case packTag("bl"): return ShapeInstructionType::SetBlack;
    case packTag("sd"): return ShapeInstructionType::SetDash;
    case packTag("sf"): return ShapeInstructionType::SetFont;
    case packTag("sg"): return ShapeInstructionType::SetGray;
    case packTag("wo"): return ShapeInstructionType::SetWhite;
    case packTag("sl"):
    case packTag("ti"): return ShapeInstructionType::Slur;
    case packTag("sr"): return ShapeInstructionType::StartGroup;
    case packTag("so"): return ShapeInstructionType::StartObject;
    case packTag("st"): return ShapeInstructionType::Stroke;
    case packTag("vm"): return ShapeInstructionType::VerticalMode;
    default: return ShapeInstructionType::Undocumented;
    }
}

void importShapeData(const ShapeSourceFamily& source, const ImportContext& context)
{
    for (const auto cmper : source.pool->cmpersForTag(source.data)) {
        const auto rows = source.pool->getArray(source.data, cmper);
        auto target = std::make_shared<ShapeDataTarget>(context.document,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
        std::vector<std::size_t> earlyLineWidths;
        if (source.earlyData) {
            // The three pools have independent comparators. Find every definition that
            // names this data list instead of assuming its instruction-list id matches.
            for (const auto& shape : context.document->getOthers()
                    ->getArray<ShapeDefTarget>(musx::dom::SCORE_PARTID)) {
                if (shape->dataList != cmper) continue;
                if (const auto instructions = context.document->getOthers()
                        ->get<ShapeInstructionTarget>(
                            musx::dom::SCORE_PARTID, shape->instructionList)) {
                    std::size_t dataIndex = 0;
                    for (const auto& instruction : instructions->instructions) {
                        if (instruction->type == ShapeInstructionType::LineWidth
                                && instruction->numData == 1) {
                            earlyLineWidths.push_back(dataIndex);
                        }
                        dataIndex += static_cast<std::size_t>(instruction->numData);
                    }
                }
            }
        }
        const auto values = shapeLongs(source, source.data, cmper, context);
        for (std::size_t index = 0; index < values.size(); ++index) {
            auto value = values[index];
            if (source.earlyData
                    && std::find(earlyLineWidths.begin(), earlyLineWidths.end(), index)
                        != earlyLineWidths.end()) {
                // Revision 1's `sw` stored hundredths of a point. The modern LineWidth
                // instruction consumes Efix: one point becomes 256.
                value = (value * 256 + (value >= 0 ? 50 : -50)) / 100;
            }
            target->values.push_back(value);
            const auto& row = rows[context.profile.epoch == FormatEpoch::ZlibLegacy
                ? 0 : (index / 3)];
            reportShapeValue(context.report, "others.shapeData["
                + std::to_string(cmper) + "].values[" + std::to_string(index) + "]",
                values[index], row);
        }
        context.document->getOthers()->add(ShapeDataTarget::XmlNodeName, target);
    }
}

void importShapeInstructions(const ShapeSourceFamily& source, const ImportContext& context)
{
    for (const auto cmper : source.pool->cmpersForTag(source.instructions)) {
        const auto rows = source.pool->getArray(source.instructions, cmper);
        const auto packed = shapeLongs(source, source.instructions, cmper, context);
        auto target = std::make_shared<ShapeInstructionTarget>(context.document,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
        for (std::size_t index = 0; index < packed.size(); ++index) {
            const auto raw = static_cast<std::uint32_t>(packed[index]);
            if (raw == 0) {
                // Zero terminates the logical list. Real files retain stale packed
                // instructions after it; Finale's own upgrade omits that tail, and
                // consuming it makes the declared data counts exceed ShapeData.
                break;
            }
            const auto tag = static_cast<records::LegacyTag>(raw & 0xffffU);
            const auto numData = static_cast<int>((raw >> 16U) & 0xffU);
            const auto revision = static_cast<unsigned>((raw >> 24U) & 0xffU);
            auto instruction = std::make_shared<ShapeInstructionTarget::InstructionInfo>();
            instruction->numData = numData;
            instruction->type = instructionType(tag, source.earlyData);
            target->instructions.push_back(instruction);
            const auto& row = rows[context.profile.epoch == FormatEpoch::ZlibLegacy
                ? 0 : (index / 3)];
            const auto prefix = "others.shapeList[" + std::to_string(cmper)
                + "].instructions[" + std::to_string(target->instructions.size() - 1) + "].";
            reportShapeValue(context.report, prefix + "numData", numData, row);
            // The destination is an enum, but the report's raw value preserves the
            // two-byte source tag that selected it.
            reportShapeValue(context.report, prefix + "type", tag, row);

            if (instruction->type == ShapeInstructionType::Undocumented || revision > 2) {
                context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                    "Shape " + std::to_string(cmper) + " contains unsupported instruction tag "
                        + records::tagText(tag) + " revision " + std::to_string(revision) + "."});
            }
        }
        context.document->getOthers()->add(ShapeInstructionTarget::XmlNodeName, target);
    }
}

void importShapeDefs(const ShapeSourceFamily& source, const ImportContext& context)
{
    for (const auto cmper : source.pool->cmpersForTag(source.definition)) {
        const auto rows = source.pool->getArray(source.definition, cmper);
        if (rows.empty()) {
            continue;
        }
        const auto words = context.profile.epoch == FormatEpoch::ZlibLegacy
            ? payloadWords(source.pool->payloadOf(rows.front()), context.profile.byteOrder)
            : std::vector<std::int16_t>(rows.front().words.begin(),
                rows.front().words.begin() + rows.front().wordCount);
        if (words.size() < 2) {
            continue;
        }
        auto target = std::make_shared<ShapeDefTarget>(context.document,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
        target->instructionList = static_cast<musx::dom::Cmper>(words[0]);
        target->dataList = static_cast<musx::dom::Cmper>(words[1]);
        // Fixed-row Finale 3-2006 SD puts the semantic enum in word 2. Coda SD and
        // zlib class 0x00d6 instead carry a bounding rectangle after the two list ids;
        // even a small coordinate must not be mistaken for an enum.
        const bool hasStoredShapeType = context.profile.epoch == FormatEpoch::UncompressedLegacy
            || context.profile.epoch == FormatEpoch::DclLegacy;
        if (hasStoredShapeType && words.size() >= 3
                && words[2] >= static_cast<int>(ShapeDefTarget::ShapeType::Other)
                && words[2] <= static_cast<int>(ShapeDefTarget::ShapeType::Clef)) {
            target->shapeType = static_cast<ShapeDefTarget::ShapeType>(words[2]);
        }
        const auto prefix = "others.shapeDef[" + std::to_string(cmper) + "].";
        reportShapeValue(context.report, prefix + "instructionList",
            target->instructionList, rows.front());
        reportShapeValue(context.report, prefix + "dataList", target->dataList, rows.front());
        if (!hasStoredShapeType) {
            // These layouts carry a bounding rectangle in this position. `Other` is
            // the behavior represented by an absent modern type, not a recovered value.
            context.report.fields.push_back({"others.shapeDef[" + std::to_string(cmper)
                    + "].shapeType",
                ValueOrigin::LegacyBehavior, 0, 0,
                static_cast<int>(target->shapeType)});
        } else {
            reportShapeValue(context.report, "others.shapeDef[" + std::to_string(cmper)
                + "].shapeType", static_cast<int>(target->shapeType), rows.front());
        }
        context.document->getOthers()->add(ShapeDefTarget::XmlNodeName, target);
    }
}

void validateShapeDefinitions(const ShapeSourceFamily& source, const ImportContext& context)
{
    std::size_t unresolved = 0;
    std::size_t insufficient = 0;
    std::size_t externalGraphics = 0;
    for (const auto cmper : source.pool->cmpersForTag(source.definition)) {
        const auto shape = context.document->getOthers()->get<ShapeDefTarget>(
            musx::dom::SCORE_PARTID, cmper);
        if (!shape || shape->isBlank()) continue;
        const auto instructions = context.document->getOthers()->get<ShapeInstructionTarget>(
            musx::dom::SCORE_PARTID, shape->instructionList);
        const auto data = context.document->getOthers()->get<ShapeDataTarget>(
            musx::dom::SCORE_PARTID, shape->dataList);
        if (!instructions || !data) {
            ++unresolved;
            continue;
        }
        std::size_t required = 0;
        for (const auto& instruction : instructions->instructions) {
            required += static_cast<std::size_t>(instruction->numData);
            if (instruction->type == ShapeInstructionType::ExternalGraphic) {
                ++externalGraphics;
            }
        }
        if (required > data->values.size()) ++insufficient;
    }
    if (unresolved != 0 || insufficient != 0) {
        context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
            std::to_string(unresolved) + " nonblank shape definition(s) have a missing list, and "
                + std::to_string(insufficient)
                + " have less data than their instructions consume."});
    }
    if (externalGraphics != 0) {
        context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
            std::to_string(externalGraphics)
                + " shape instruction(s) reference embedded graphics, whose stored bytes are "
                  "preserved but are not represented by the imported shape DOM classes."});
    }
}

} // namespace

void importShapeDefinitions(const ImportContext& context)
{
    const auto source = sourceFamily(context);
    // Definitions precede data only because the Coda line-width conversion must follow
    // their independent instruction-list and data-list references.
    importShapeInstructions(source, context);
    importShapeDefs(source, context);
    importShapeData(source, context);
    validateShapeDefinitions(source, context);
}

} // namespace others
} // namespace finale_mus_reader
