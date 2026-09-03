// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// ShapeDef, ShapeInstructionList, and ShapeData are recovered as three
// independent pools -- the importer copies every comparator present in each
// pool regardless of whether any ShapeDef names it (see
// src/import/others/shape_definitions.cpp) -- so they are surveyed as three
// independent classes here too, each comparable against its own Finale 27
// companion pool without assuming the others resolve. ShapeDef itself does
// nothing but report the two cmpers it stores; whether those cmpers resolve,
// whether a list is blank, and whether an ExternalGraphic instruction's operand
// resolves to a ShapeGraphicAssign are all downstream questions an aggregator
// can answer by joining these three surveyors' output on cmper, exactly as
// musxdom itself does when a caller resolves `instructionList`/`dataList`.

#include <cstddef>
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

std::int64_t integerMember(const Value& object, std::string_view key, std::int64_t fallback = 0)
{
    const auto* value = object.find(key);
    return value && value->isInteger() ? value->asInteger() : fallback;
}

std::string instructionSignature(const Value& list, std::size_t skip = 0)
{
    const auto* instructions = list.find("instructions");
    if (!instructions || !instructions->isArray()) return {};
    std::string result;
    for (std::size_t index = skip; index < instructions->asArray().size(); ++index) {
        const auto& instruction = instructions->asArray()[index];
        result += std::to_string(integerMember(instruction, "type")) + ':' +
                  std::to_string(integerMember(instruction, "num_data")) + ';';
    }
    return result;
}

std::map<std::int64_t, Value*> objectsByCmper(Value::Array& items)
{
    std::map<std::int64_t, Value*> result;
    for (auto& item : items)
        result.emplace(integerMember(item, "cmper"), &item);
    return result;
}

std::map<std::int64_t, const Value*> objectsByCmper(const Value::Array& items)
{
    std::map<std::int64_t, const Value*> result;
    for (const auto& item : items)
        result.emplace(integerMember(item, "cmper"), &item);
    return result;
}

Value::Array* nestedArray(SurveySnapshot& snapshot, std::string_view object, std::string_view array)
{
    auto found = snapshot.find(object);
    if (found == snapshot.end()) return nullptr;
    auto* value = found->second.find(array);
    return value && value->isArray() ? &value->asArray() : nullptr;
}

Value::Array* topArray(SurveySnapshot& snapshot, std::string_view key)
{
    auto found = snapshot.find(key);
    return found != snapshot.end() && found->second.isArray() ? &found->second.asArray() : nullptr;
}

std::map<std::int64_t, std::size_t>
consumedLengths(const Value::Array& shapes, const std::map<std::int64_t, const Value*>& lists)
{
    std::map<std::int64_t, std::size_t> result;
    for (const auto& shape : shapes) {
        const auto listFound = lists.find(integerMember(shape, "instruction_list"));
        if (listFound == lists.end()) continue;
        std::size_t consumed = 0;
        if (const auto* instructions = listFound->second->find("instructions");
            instructions && instructions->isArray()) {
            for (const auto& instruction : instructions->asArray()) {
                consumed += static_cast<std::size_t>(integerMember(instruction, "num_data"));
            }
        }
        result.try_emplace(integerMember(shape, "data_list"), consumed);
    }
    return result;
}

std::map<std::int64_t, std::set<std::size_t>>
setFontPositions(const Value::Array& shapes, const std::map<std::int64_t, const Value*>& lists)
{
    std::map<std::int64_t, std::set<std::size_t>> result;
    for (const auto& shape : shapes) {
        const auto listFound = lists.find(integerMember(shape, "instruction_list"));
        if (listFound == lists.end()) continue;
        std::size_t offset = 0;
        const auto* instructions = listFound->second->find("instructions");
        if (!instructions || !instructions->isArray()) continue;
        for (const auto& instruction : instructions->asArray()) {
            if (integerMember(instruction, "type") == 20) {
                result[integerMember(shape, "data_list")].insert(offset);
            }
            offset += static_cast<std::size_t>(integerMember(instruction, "num_data"));
        }
    }
    return result;
}

std::string dataSignature(const Value& buffer, const std::set<std::size_t>& masked,
                          std::optional<std::size_t> consumed, std::size_t skip = 0)
{
    const auto* values = buffer.find("values");
    if (!values || !values->isArray()) return {};
    const auto limit =
        consumed ? (std::min)(*consumed, values->asArray().size()) : values->asArray().size();
    std::string result;
    for (std::size_t index = skip; index < limit; ++index) {
        result += masked.contains(index)
                      ? "*;"
                      : std::to_string(integerMember(values->asArray()[index], "value")) + ';';
    }
    return result;
}

template <typename Signature>
std::map<std::int64_t, std::int64_t>
matchBySignature(const Value::Array& source, const Value::Array& companion, Signature signature)
{
    std::map<std::int64_t, std::int64_t> result;
    std::set<std::int64_t> consumedSource;
    const auto sourceByCmper = objectsByCmper(source);
    for (const auto& item : companion) {
        const auto cmper = integerMember(item, "cmper");
        const auto found = sourceByCmper.find(cmper);
        if (found != sourceByCmper.end() &&
            signature(*found->second, true) == signature(item, false)) {
            result[cmper] = cmper;
            consumedSource.insert(cmper);
        }
    }
    std::map<std::string, std::vector<std::int64_t>> available;
    for (const auto& item : source) {
        const auto cmper = integerMember(item, "cmper");
        if (!consumedSource.contains(cmper)) available[signature(item, true)].push_back(cmper);
    }
    for (const auto& item : companion) {
        const auto cmper = integerMember(item, "cmper");
        if (result.contains(cmper)) continue;
        auto& candidates = available[signature(item, false)];
        if (!candidates.empty()) {
            result[cmper] = candidates.front();
            candidates.erase(candidates.begin());
        }
    }
    return result;
}

std::map<std::int64_t, std::int64_t>
safeRenumbering(const Value::Array& source, const Value::Array& companion,
                const std::map<std::int64_t, std::int64_t>& matches)
{
    std::set<std::int64_t> sourceCmpers;
    std::set<std::int64_t> allCmpers;
    for (const auto& item : source)
        sourceCmpers.insert(integerMember(item, "cmper"));
    for (const auto& item : companion)
        allCmpers.insert(integerMember(item, "cmper"));
    allCmpers.insert(sourceCmpers.begin(), sourceCmpers.end());
    auto next = allCmpers.empty() ? 1 : *allCmpers.rbegin() + 1;
    std::map<std::int64_t, std::int64_t> result;
    for (const auto& item : companion) {
        const auto cmper = integerMember(item, "cmper");
        const auto match = matches.find(cmper);
        if (match != matches.end())
            result[cmper] = match->second;
        else if (sourceCmpers.contains(cmper))
            result[cmper] = next++;
        else
            result[cmper] = cmper;
    }
    return result;
}

std::size_t realignShapes(SurveySnapshot& source, SurveySnapshot& companion)
{
    auto* sourceShapes = topArray(source, "shape_defs");
    auto* companionShapes = topArray(companion, "shape_defs");
    auto* sourceLists = nestedArray(source, "shape_instruction_lists", "lists");
    auto* companionLists = nestedArray(companion, "shape_instruction_lists", "lists");
    auto* sourceBuffers = nestedArray(source, "shape_data", "buffers");
    auto* companionBuffers = nestedArray(companion, "shape_data", "buffers");
    if (!sourceShapes || !companionShapes || !sourceLists || !companionLists || !sourceBuffers ||
        !companionBuffers)
        return 0;

    auto instructionMatches =
        matchBySignature(*sourceLists, *companionLists,
                         [](const Value& item, bool) { return instructionSignature(item); });
    const auto sourceListsByCmper = objectsByCmper(std::as_const(*sourceLists));
    const auto companionListsByCmper = objectsByCmper(std::as_const(*companionLists));
    const auto sourceConsumed = consumedLengths(*sourceShapes, sourceListsByCmper);
    auto companionConsumed = consumedLengths(*companionShapes, companionListsByCmper);
    const auto sourceFontPositions = setFontPositions(*sourceShapes, sourceListsByCmper);
    const auto companionFontPositions = setFontPositions(*companionShapes, companionListsByCmper);
    const auto dataSignatureFor = [&](const Value& item, bool isSource) {
        const auto cmper = integerMember(item, "cmper");
        const auto& positions = isSource ? sourceFontPositions : companionFontPositions;
        const auto& consumed = isSource ? sourceConsumed : companionConsumed;
        const auto positionFound = positions.find(cmper);
        const auto consumedFound = consumed.find(cmper);
        return dataSignature(
            item,
            positionFound == positions.end() ? std::set<std::size_t>{} : positionFound->second,
            consumedFound == consumed.end() ? std::nullopt
                                            : std::optional<std::size_t>(consumedFound->second));
    };
    auto dataMatches = matchBySignature(*sourceBuffers, *companionBuffers, dataSignatureFor);
    const auto sourceBuffersByCmper = objectsByCmper(std::as_const(*sourceBuffers));
    const auto companionBuffersConstByCmper = objectsByCmper(std::as_const(*companionBuffers));
    auto companionBuffersByCmper = objectsByCmper(*companionBuffers);

    const auto shapeSignature = [&](const Value& shape, bool isSource) {
        const auto& lists = isSource ? sourceListsByCmper : companionListsByCmper;
        const auto& buffers = isSource ? sourceBuffersByCmper : companionBuffersConstByCmper;
        const auto list = lists.find(integerMember(shape, "instruction_list"));
        const auto buffer = buffers.find(integerMember(shape, "data_list"));
        return (list == lists.end() ? std::string{} : instructionSignature(*list->second)) + '|' +
               (buffer == buffers.end() ? std::string{}
                                        : dataSignatureFor(*buffer->second, isSource));
    };
    auto shapeMatches = matchBySignature(*sourceShapes, *companionShapes, shapeSignature);

    std::map<std::string, std::vector<std::int64_t>> sourceShapesBySignature;
    for (const auto& shape : *sourceShapes) {
        const auto cmper = integerMember(shape, "cmper");
        bool already = false;
        for (const auto& [unused, target] : shapeMatches)
            if (target == cmper) already = true;
        if (!already) sourceShapesBySignature[shapeSignature(shape, true)].push_back(cmper);
    }
    const auto sourceShapesByCmper = objectsByCmper(std::as_const(*sourceShapes));
    std::size_t wrapperCount = 0;
    for (auto& shape : *companionShapes) {
        const auto cmper = integerMember(shape, "cmper");
        if (shapeMatches.contains(cmper)) continue;
        const auto listCmper = integerMember(shape, "instruction_list");
        const auto dataCmper = integerMember(shape, "data_list");
        const auto listFound = companionListsByCmper.find(listCmper);
        const auto bufferFound = companionBuffersByCmper.find(dataCmper);
        if (listFound == companionListsByCmper.end() ||
            bufferFound == companionBuffersByCmper.end())
            continue;
        const auto* instructions = listFound->second->find("instructions");
        if (!instructions || !instructions->isArray() || instructions->asArray().empty() ||
            integerMember(instructions->asArray().front(), "type") != 25)
            continue;
        const auto numData =
            static_cast<std::size_t>(integerMember(instructions->asArray().front(), "num_data"));
        const auto stripped =
            instructionSignature(*listFound->second, 1) + '|' +
            dataSignature(*bufferFound->second,
                          companionFontPositions.contains(dataCmper)
                              ? companionFontPositions.at(dataCmper)
                              : std::set<std::size_t>{},
                          companionConsumed.contains(dataCmper)
                              ? std::optional<std::size_t>(companionConsumed.at(dataCmper))
                              : std::nullopt,
                          numData);
        auto& candidates = sourceShapesBySignature[stripped];
        if (candidates.empty()) continue;
        const auto sourceCmper = candidates.front();
        candidates.erase(candidates.begin());
        shapeMatches[cmper] = sourceCmper;
        const auto* sourceShape = sourceShapesByCmper.at(sourceCmper);
        instructionMatches[listCmper] = integerMember(*sourceShape, "instruction_list");
        dataMatches[dataCmper] = integerMember(*sourceShape, "data_list");
        auto listMutable = objectsByCmper(*companionLists).at(listCmper);
        listMutable->find("instructions")
            ->asArray()
            .erase(listMutable->find("instructions")->asArray().begin());
        auto& values = bufferFound->second->find("values")->asArray();
        values.erase(values.begin(), values.begin() + (std::min)(numData, values.size()));
        ++wrapperCount;
    }

    const auto shapeFinal = safeRenumbering(*sourceShapes, *companionShapes, shapeMatches);
    const auto instructionFinal =
        safeRenumbering(*sourceLists, *companionLists, instructionMatches);
    const auto dataFinal = safeRenumbering(*sourceBuffers, *companionBuffers, dataMatches);
    companionConsumed = consumedLengths(*companionShapes, companionListsByCmper);
    for (auto& buffer : *sourceBuffers) {
        const auto cmper = integerMember(buffer, "cmper");
        if (auto* values = buffer.find("values");
            values && sourceConsumed.contains(cmper) &&
            values->asArray().size() > sourceConsumed.at(cmper)) {
            values->asArray().resize(sourceConsumed.at(cmper));
        }
    }
    for (auto& shape : *companionShapes) {
        const auto cmper = integerMember(shape, "cmper");
        const auto list = integerMember(shape, "instruction_list");
        const auto data = integerMember(shape, "data_list");
        shape.asObject()["cmper"] = Value(shapeFinal.at(cmper));
        if (instructionFinal.contains(list))
            shape.asObject()["instruction_list"] = Value(instructionFinal.at(list));
        if (dataFinal.contains(data)) shape.asObject()["data_list"] = Value(dataFinal.at(data));
    }
    for (auto& list : *companionLists) {
        const auto cmper = integerMember(list, "cmper");
        list.asObject()["cmper"] = Value(instructionFinal.at(cmper));
        if (auto* instructions = list.find("instructions")) {
            for (std::size_t index = 0; index < instructions->asArray().size(); ++index) {
                instructions->asArray()[index].asObject()["index"] =
                    Value(static_cast<std::int64_t>(index));
            }
        }
    }
    for (auto& buffer : *companionBuffers) {
        const auto cmper = integerMember(buffer, "cmper");
        buffer.asObject()["cmper"] = Value(dataFinal.at(cmper));
        if (auto* values = buffer.find("values")) {
            const auto consumed = companionConsumed.find(cmper);
            if (consumed != companionConsumed.end() &&
                values->asArray().size() > consumed->second) {
                values->asArray().resize(consumed->second);
            }
            for (std::size_t index = 0; index < values->asArray().size(); ++index) {
                values->asArray()[index].asObject()["index"] =
                    Value(static_cast<std::int64_t>(index));
            }
        }
    }
    if (auto found = companion.find("clef_options"); found != companion.end()) {
        if (auto* clefs = found->second.find("clef_defs"); clefs && clefs->isArray()) {
            for (auto& clef : clefs->asArray()) {
                const auto id = integerMember(clef, "shape_id");
                if (shapeFinal.contains(id)) clef.asObject()["shape_id"] = Value(shapeFinal.at(id));
            }
        }
    }
    if (auto found = companion.find("mmrest_options"); found != companion.end()) {
        const auto id = integerMember(found->second, "shape_def");
        if (shapeFinal.contains(id))
            found->second.asObject()["shape_def"] = Value(shapeFinal.at(id));
    }
    return wrapperCount;
}


void prepareShapeComparison(ComparisonPreparationContext& context)
{
    const auto wrappers = realignShapes(context.source, context.companion);
    if (wrappers) {
        context.transformations[ComparisonTransformation::FinaleAddedStartObjectWrapper] +=
            wrappers;
    }
}

std::optional<DifferenceClassification>
classifyShapeDefinitionDifference(const DifferenceContext& context)
{
    using enum DifferenceCategory;
    if (context.category == Differs && comparisonPathEndsWith(context.path, ".shape_type") &&
        context.origin == "legacy-mus" && context.sourceValue.isInteger() &&
        context.companionValue.isInteger() && context.sourceValue.asInteger() != 0 &&
        context.companionValue.asInteger() == 0) {
        return DifferenceClassification::ShapeReclassifiedOther;
    }
    return std::nullopt;
}

Value observeShapeDefs(const SurveyContext& ctx)
{
    using Target = musx::dom::others::ShapeDef;
    Value::Array result;
    for (const auto& definition : sourceInstances<Target>(ctx)) {
        result.emplace_back(observe(
            *definition, ctx, field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("instruction_list", &Target::instructionList),
            field("data_list", &Target::dataList), field("shape_type", &Target::shapeType),
            field("origin_instructionList",
                  [&ctx](const Target& value) {
                      return fieldOrigin<Target>(ctx, "instructionList", value.getCmper());
                  }),
            field("origin_dataList",
                  [&ctx](const Target& value) {
                      return fieldOrigin<Target>(ctx, "dataList", value.getCmper());
                  }),
            field("origin_shapeType", [&ctx](const Target& value) {
                return fieldOrigin<Target>(ctx, "shapeType", value.getCmper());
            })));
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
    using Target = musx::dom::others::ShapeInstructionList;
    for (const auto& list : sourceInstances<Target>(ctx)) {
        Value::Array instructions;
        for (std::size_t index = 0; index < list->instructions.size(); ++index) {
            const auto& instruction = *list->instructions[index];
            const auto fieldPrefix = "instructions[" + std::to_string(index) + "].";
            instructions.emplace_back(Value::Object{
                {"index", index},
                {"num_data", instruction.numData},
                {"type", static_cast<std::int64_t>(instruction.type)},
                {"origin_numData",
                 fieldOrigin<Target>(ctx, fieldPrefix + "numData", list->getCmper())},
                {"origin_type", fieldOrigin<Target>(ctx, fieldPrefix + "type", list->getCmper())}});
            ++instructionTypes[static_cast<int>(instruction.type)];
            ++instructionCount;
            if (instruction.type == musx::dom::ShapeDefInstructionType::ExternalGraphic)
                ++externalGraphicCount;
            if (instruction.type == musx::dom::ShapeDefInstructionType::Undocumented)
                ++undocumentedCount;
        }
        lists.emplace_back(
            Value::Object{{"part_id", list->getSourcePartId()},
                          {"share_mode", static_cast<std::int64_t>(list->getShareMode())},
                          {"cmper", list->getCmper()},
                          {"instructions", std::move(instructions)}});
    }
    Value::Object typeValues;
    for (const auto& [type, count] : instructionTypes) {
        typeValues.emplace(std::to_string(type), count);
    }
    return Value::Object{{"lists", std::move(lists)},
                         {"instruction_count", instructionCount},
                         {"external_graphic_count", externalGraphicCount},
                         {"undocumented_instruction_count", undocumentedCount},
                         {"instruction_types", std::move(typeValues)}};
}

Value observeShapeData(const SurveyContext& ctx)
{
    std::size_t valueCount = 0;
    Value::Array buffers;
    using Target = musx::dom::others::ShapeData;
    for (const auto& data : sourceInstances<Target>(ctx)) {
        Value::Array values;
        for (std::size_t index = 0; index < data->values.size(); ++index) {
            values.emplace_back(Value::Object{
                {"index", index},
                {"value", data->values[index]},
                {"origin", fieldOrigin<Target>(ctx, "values[" + std::to_string(index) + "]",
                                               data->getCmper())}});
            ++valueCount;
        }
        buffers.emplace_back(
            Value::Object{{"part_id", data->getSourcePartId()},
                          {"share_mode", static_cast<std::int64_t>(data->getShareMode())},
                          {"cmper", data->getCmper()},
                          {"values", std::move(values)}});
    }
    return Value::Object{{"buffers", std::move(buffers)}, {"value_count", valueCount}};
}

COVERAGE_CLASS_WITH_PREPARATION("others", "shape_defs", observeShapeDefs,
                                classifyShapeDefinitionDifference, prepareShapeComparison);
COVERAGE_SURVEYOR("others", "shape_instruction_lists", observeShapeInstructionLists);
COVERAGE_SURVEYOR("others", "shape_data", observeShapeData);

} // namespace
