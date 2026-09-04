// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/classification_rules.h"
#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

using PartDefinitionSurveyTarget = musx::dom::others::PartDefinition;

// Every origin leaf names its C++ member exactly, so the comparison model pairs the two halves
// by spelling. One helper keeps that pairing from drifting a member at a time.
auto partDefinitionOrigin(const char* member)
{
    return [member](const PartDefinitionSurveyTarget& value, const SurveyContext& context) {
        return fieldOrigin<PartDefinitionSurveyTarget>(context, member, value);
    };
}

Value observePartDefinitions(const SurveyContext& ctx)
{
    using Target = PartDefinitionSurveyTarget;
    Value::Array result;
    for (const auto& part : sourceInstances<Target>(ctx)) {
        result.push_back(observe(*part, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field("name_id", &Target::nameId),
            field("part_order", &Target::partOrder),
            field("copies", &Target::copies),
            field("print_part", &Target::printPart),
            field("extract_part", &Target::extractPart),
            field("apply_format", &Target::applyFormat),
            field("needs_recalc", &Target::needsRecalc),
            field("use_as_smp_inst", &Target::useAsSmpInst),
            field("unlink_insts", &Target::unlinkInsts),
            field("smart_music_inst", &Target::smartMusicInst),
            field("default_name_staff", &Target::defaultNameStaff),
            field("default_name_group", &Target::defaultNameGroup),
            field("origin_nameId", partDefinitionOrigin("nameId")),
            field("origin_partOrder", partDefinitionOrigin("partOrder")),
            field("origin_copies", partDefinitionOrigin("copies")),
            field("origin_printPart", partDefinitionOrigin("printPart")),
            field("origin_extractPart", partDefinitionOrigin("extractPart")),
            field("origin_applyFormat", partDefinitionOrigin("applyFormat")),
            field("origin_needsRecalc", partDefinitionOrigin("needsRecalc")),
            field("origin_useAsSmpInst", partDefinitionOrigin("useAsSmpInst")),
            field("origin_unlinkInsts", partDefinitionOrigin("unlinkInsts")),
            field("origin_smartMusicInst", partDefinitionOrigin("smartMusicInst")),
            field("origin_defaultNameStaff", partDefinitionOrigin("defaultNameStaff")),
            field("origin_defaultNameGroup", partDefinitionOrigin("defaultNameGroup"))));
    }
    return Value(std::move(result));
}

COVERAGE_CLASS("others", "part_defs", observePartDefinitions, classifyPartDefinitionDifference);

} // namespace
