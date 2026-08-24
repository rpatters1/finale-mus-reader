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
#include <string>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

Value observeShapeDefs(const SurveyContext& ctx)
{
    using Target = musx::dom::others::ShapeDef;
    Value::Array result;
    for (const auto& definition : ctx.document->getOthers()
            ->getArray<Target>(musx::dom::SCORE_PARTID)) {
        const auto prefix = "others.shapeDef[" + std::to_string(definition->getCmper()) + "].";
        result.emplace_back(observe(*definition, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("instruction_list", &Target::instructionList), field("data_list", &Target::dataList),
            field("shape_type", &Target::shapeType),
            field("origin_instructionList", [&ctx, &prefix](const Target&) { return ctx.fields.originOf(prefix + "instructionList"); }),
            field("origin_dataList", [&ctx, &prefix](const Target&) { return ctx.fields.originOf(prefix + "dataList"); }),
            field("origin_shapeType", [&ctx, &prefix](const Target&) { return ctx.fields.originOf(prefix + "shapeType"); })));
    }
    return result;
}

Value observeShapeInstructionLists(const SurveyContext& ctx)
{
    std::map<int, std::size_t> instructionTypes;
    std::size_t externalGraphicCount = 0;
    std::size_t undocumentedCount = 0;
    std::size_t instructionCount = 0;

    Value::Array lists;
    for (const auto& list : ctx.document->getOthers()
            ->getArray<musx::dom::others::ShapeInstructionList>(musx::dom::SCORE_PARTID)) {
        const auto prefix = "others.shapeList[" + std::to_string(list->getCmper()) + "].instructions[";
        Value::Array instructions;
        for (std::size_t index = 0; index < list->instructions.size(); ++index) {
            const auto& instruction = *list->instructions[index];
            const auto fieldPrefix = prefix + std::to_string(index) + "].";
            instructions.emplace_back(Value::Object{{"index", index}, {"num_data", instruction.numData},
                {"type", static_cast<std::int64_t>(instruction.type)},
                {"origin_numData", std::string(ctx.fields.originOf(fieldPrefix + "numData"))},
                {"origin_type", std::string(ctx.fields.originOf(fieldPrefix + "type"))}});
            ++instructionTypes[static_cast<int>(instruction.type)];
            ++instructionCount;
            if (instruction.type == musx::dom::ShapeDefInstructionType::ExternalGraphic) ++externalGraphicCount;
            if (instruction.type == musx::dom::ShapeDefInstructionType::Undocumented) ++undocumentedCount;
        }
        lists.emplace_back(Value::Object{{"cmper", list->getCmper()}, {"instructions", std::move(instructions)}});
    }
    Value::Object typeValues;
    for (const auto& [type, count] : instructionTypes) {
        typeValues.emplace(std::to_string(type), count);
    }
    return Value::Object{{"lists", std::move(lists)}, {"instruction_count", instructionCount},
        {"external_graphic_count", externalGraphicCount},
        {"undocumented_instruction_count", undocumentedCount},
        {"instruction_types", std::move(typeValues)}};
}

Value observeShapeData(const SurveyContext& ctx)
{
    std::size_t valueCount = 0;
    Value::Array buffers;
    for (const auto& data : ctx.document->getOthers()
            ->getArray<musx::dom::others::ShapeData>(musx::dom::SCORE_PARTID)) {
        const auto prefix = "others.shapeData[" + std::to_string(data->getCmper()) + "].values[";
        Value::Array values;
        for (std::size_t index = 0; index < data->values.size(); ++index) {
            values.emplace_back(Value::Object{{"index", index}, {"value", data->values[index]},
                {"origin", std::string(ctx.fields.originOf(prefix + std::to_string(index) + "]"))}});
            ++valueCount;
        }
        buffers.emplace_back(Value::Object{{"cmper", data->getCmper()}, {"values", std::move(values)}});
    }
    return Value::Object{{"buffers", std::move(buffers)}, {"value_count", valueCount}};
}

COVERAGE_SURVEYOR("shape_defs", observeShapeDefs);
COVERAGE_SURVEYOR("shape_instruction_lists", observeShapeInstructionLists);
COVERAGE_SURVEYOR("shape_data", observeShapeData);

} // namespace
