// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// ShapeDef, ShapeInstructionList, and ShapeData are recovered as three independent pools --
// the importer copies every comparator present in each pool regardless of whether any
// ShapeDef names it (see src/import/others/shape_definitions.cpp) -- so they are surveyed as
// three independent classes here too, each comparable against its own Finale 27 companion
// pool without assuming the others resolve. ShapeDef itself does nothing but report the two
// cmpers it stores; whether those cmpers resolve, whether a list is blank, and whether an
// ExternalGraphic instruction's operand resolves to a ShapeGraphicAssign are all downstream
// questions an aggregator can answer by joining these three surveyors' output on cmper,
// exactly as musxdom itself does when a caller resolves `instructionList`/`dataList`.

#include <cstddef>
#include <map>
#include <ostream>
#include <string>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

void writeShapeDefs(std::ostream& out, const SurveyContext& ctx)
{
    out << '[';
    bool first = true;
    for (const auto& definition : ctx.document->getOthers()
            ->getArray<musx::dom::others::ShapeDef>(musx::dom::SCORE_PARTID)) {
        const auto prefix = "others.shapeDef[" + std::to_string(definition->getCmper()) + "].";
        out << (first ? "" : ",") << '{'
            << "\"cmper\":" << definition->getCmper()
            << ",\"instruction_list\":" << definition->instructionList
            << ",\"data_list\":" << definition->dataList
            << ",\"shape_type\":" << static_cast<int>(definition->shapeType)
            << ",\"origin_instructionList\":"
            << jsonString(ctx.fields.originOf(prefix + "instructionList"))
            << ",\"origin_dataList\":" << jsonString(ctx.fields.originOf(prefix + "dataList"))
            << ",\"origin_shapeType\":" << jsonString(ctx.fields.originOf(prefix + "shapeType"))
            << '}';
        first = false;
    }
    out << ']';
}

void writeShapeInstructionLists(std::ostream& out, const SurveyContext& ctx)
{
    std::map<int, std::size_t> instructionTypes;
    std::size_t externalGraphicCount = 0;
    std::size_t undocumentedCount = 0;
    std::size_t instructionCount = 0;

    out << "{\"lists\":[";
    bool first = true;
    for (const auto& list : ctx.document->getOthers()
            ->getArray<musx::dom::others::ShapeInstructionList>(musx::dom::SCORE_PARTID)) {
        const auto prefix = "others.shapeList[" + std::to_string(list->getCmper()) + "].instructions[";
        out << (first ? "" : ",") << '{'
            << "\"cmper\":" << list->getCmper()
            << ",\"instructions\":[";
        for (std::size_t index = 0; index < list->instructions.size(); ++index) {
            const auto& instruction = *list->instructions[index];
            const auto fieldPrefix = prefix + std::to_string(index) + "].";
            out << (index ? "," : "") << '{'
                << "\"index\":" << index
                << ",\"num_data\":" << instruction.numData
                << ",\"type\":" << static_cast<int>(instruction.type)
                << ",\"origin_numData\":" << jsonString(ctx.fields.originOf(fieldPrefix + "numData"))
                << ",\"origin_type\":" << jsonString(ctx.fields.originOf(fieldPrefix + "type"))
                << '}';
            ++instructionTypes[static_cast<int>(instruction.type)];
            ++instructionCount;
            if (instruction.type == musx::dom::ShapeDefInstructionType::ExternalGraphic) ++externalGraphicCount;
            if (instruction.type == musx::dom::ShapeDefInstructionType::Undocumented) ++undocumentedCount;
        }
        out << "]}";
        first = false;
    }
    out << "],\"instruction_count\":" << instructionCount
        << ",\"external_graphic_count\":" << externalGraphicCount
        << ",\"undocumented_instruction_count\":" << undocumentedCount
        << ",\"instruction_types\":{";
    bool comma = false;
    for (const auto& [type, count] : instructionTypes) {
        out << (comma ? "," : "") << jsonString(std::to_string(type)) << ':' << count;
        comma = true;
    }
    out << "}}";
}

void writeShapeData(std::ostream& out, const SurveyContext& ctx)
{
    std::size_t valueCount = 0;
    out << "{\"buffers\":[";
    bool first = true;
    for (const auto& data : ctx.document->getOthers()
            ->getArray<musx::dom::others::ShapeData>(musx::dom::SCORE_PARTID)) {
        const auto prefix = "others.shapeData[" + std::to_string(data->getCmper()) + "].values[";
        out << (first ? "" : ",") << '{'
            << "\"cmper\":" << data->getCmper()
            << ",\"values\":[";
        for (std::size_t index = 0; index < data->values.size(); ++index) {
            out << (index ? "," : "") << '{'
                << "\"index\":" << index
                << ",\"value\":" << data->values[index]
                << ",\"origin\":"
                << jsonString(ctx.fields.originOf(prefix + std::to_string(index) + "]"))
                << '}';
            ++valueCount;
        }
        out << "]}";
        first = false;
    }
    out << "],\"value_count\":" << valueCount << '}';
}

COVERAGE_SURVEYOR("shape_defs", writeShapeDefs);
COVERAGE_SURVEYOR("shape_instruction_lists", writeShapeInstructionLists);
COVERAGE_SURVEYOR("shape_data", writeShapeData);

} // namespace
