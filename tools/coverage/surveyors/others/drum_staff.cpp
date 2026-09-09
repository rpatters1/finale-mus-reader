// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

#include <optional>

namespace {

using namespace finale_mus_reader::coverage;

std::optional<DifferenceClassification>
classifyDrumStaffMapDifference(const DifferenceContext &context) {
    if (context.category == DifferenceCategory::Differs &&
        comparisonPathEndsWith(context.path, ".which_drum_lib") && context.origin == "legacy-mus") {
        return DifferenceClassification::FinaleReplacedLegacyPercussionMap;
    }
    return std::nullopt;
}

Value observeDrumStaff(const SurveyContext &context) {
    using Target = musx::dom::others::DrumStaff;
    Value::Array result;
    for (const auto &staff : sourceInstances<Target>(context)) {
        result.push_back(observe(
            *staff, context, field("cmper", [](const Target &value) { return value.getCmper(); }),
            field("origin_whichDrumLib",
                  [](const Target &value, const SurveyContext &ctx) {
                      return fieldOrigin<Target>(ctx, "whichDrumLib", value);
                  }),
            field("which_drum_lib", &Target::whichDrumLib)));
    }
    return result;
}

COVERAGE_CLASS("others", "drum_staff", observeDrumStaff, classifyDrumStaffMapDifference);

} // namespace
