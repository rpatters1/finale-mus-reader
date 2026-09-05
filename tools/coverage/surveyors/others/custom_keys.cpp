// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <string>
#include <utility>

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

template <typename Target> Value observeCustomKeyArrays(const SurveyContext& ctx)
{
    Value::Array result;
    for (const auto& item : sourceInstances<Target>(ctx)) {
        Value::Array values;
        for (std::size_t index = 0; index < item->values.size(); ++index) {
            const auto member = "values[" + std::to_string(index) + "]";
            values.emplace_back(Value::Object{
                {"value", item->values[index]},
                {"origin", fieldOrigin<Target>(ctx, member, *item)},
            });
        }
        result.emplace_back(observe(*item, ctx,
            field("cmper", [](const Target& value) { return value.getCmper(); }),
            field(
                "values", [values = std::move(values)](const Target&) { return Value(values); })));
    }
    return result;
}

Value observeAcciAmountFlats(const SurveyContext& ctx)
{
    return observeCustomKeyArrays<musx::dom::others::AcciAmountFlats>(ctx);
}

Value observeAcciAmountSharps(const SurveyContext& ctx)
{
    return observeCustomKeyArrays<musx::dom::others::AcciAmountSharps>(ctx);
}

Value observeAcciOrderFlats(const SurveyContext& ctx)
{
    return observeCustomKeyArrays<musx::dom::others::AcciOrderFlats>(ctx);
}

Value observeAcciOrderSharps(const SurveyContext& ctx)
{
    return observeCustomKeyArrays<musx::dom::others::AcciOrderSharps>(ctx);
}

Value observeKeyAttributes(const SurveyContext& ctx)
{
    using Target = musx::dom::others::KeyAttributes;
    Value::Array result;
    for (const auto& value : sourceInstances<Target>(ctx)) {
        result.emplace_back(observe(*value, ctx,
            field("cmper", [](const Target& item) { return item.getCmper(); }),
            field("harm_refer", &Target::harmRefer), field("middle_c_key", &Target::middleCKey),
            field("font_sym", &Target::fontSym), field("goto_key", &Target::gotoKey),
            field("symbol_list", &Target::symbolList), field("has_clef_octv", &Target::hasClefOctv),
            field("origin_harmRefer",
                [&ctx](const Target& item) { return fieldOrigin<Target>(ctx, "harmRefer", item); }),
            field("origin_middleCKey",
                [&ctx](
                    const Target& item) { return fieldOrigin<Target>(ctx, "middleCKey", item); }),
            field("origin_fontSym",
                [&ctx](const Target& item) { return fieldOrigin<Target>(ctx, "fontSym", item); }),
            field("origin_gotoKey",
                [&ctx](const Target& item) { return fieldOrigin<Target>(ctx, "gotoKey", item); }),
            field("origin_symbolList",
                [&ctx](
                    const Target& item) { return fieldOrigin<Target>(ctx, "symbolList", item); }),
            field("origin_hasClefOctv", [&ctx](const Target& item) {
                return fieldOrigin<Target>(ctx, "hasClefOctv", item);
            })));
    }
    return result;
}

Value observeKeyFormats(const SurveyContext& ctx)
{
    using Target = musx::dom::others::KeyFormat;
    Value::Array result;
    for (const auto& value : sourceInstances<Target>(ctx)) {
        result.emplace_back(observe(*value, ctx,
            field("cmper", [](const Target& item) { return item.getCmper(); }),
            field("semitones", &Target::semitones), field("scale_tones", &Target::scaleTones),
            field("origin_semitones",
                [&ctx](const Target& item) { return fieldOrigin<Target>(ctx, "semitones", item); }),
            field("origin_scaleTones", [&ctx](const Target& item) {
                return fieldOrigin<Target>(ctx, "scaleTones", item);
            })));
    }
    return result;
}

Value observeKeyMapArrays(const SurveyContext& ctx)
{
    using Target = musx::dom::others::KeyMapArray;
    Value::Array result;
    for (const auto& value : sourceInstances<Target>(ctx)) {
        Value::Array steps;
        for (std::size_t index = 0; index < value->steps.size(); ++index) {
            const auto prefix = "steps[" + std::to_string(index) + "].";
            steps.emplace_back(Value::Object{
                {"diatonic", value->steps[index]->diatonic},
                {"hlevel", value->steps[index]->hlevel},
                {"origin_diatonic", fieldOrigin<Target>(ctx, prefix + "diatonic", *value)},
                {"origin_hlevel", fieldOrigin<Target>(ctx, prefix + "hlevel", *value)},
            });
        }
        result.emplace_back(observe(*value, ctx,
            field("cmper", [](const Target& item) { return item.getCmper(); }),
            field("steps", [steps = std::move(steps)](const Target&) { return Value(steps); })));
    }
    return result;
}

Value observeTonalCenterFlats(const SurveyContext& ctx)
{
    return observeCustomKeyArrays<musx::dom::others::TonalCenterFlats>(ctx);
}

Value observeTonalCenterSharps(const SurveyContext& ctx)
{
    return observeCustomKeyArrays<musx::dom::others::TonalCenterSharps>(ctx);
}

COVERAGE_SURVEYOR("others", "acci_amount_flats", observeAcciAmountFlats);
COVERAGE_SURVEYOR("others", "acci_amount_sharps", observeAcciAmountSharps);
COVERAGE_SURVEYOR("others", "acci_order_flats", observeAcciOrderFlats);
COVERAGE_SURVEYOR("others", "acci_order_sharps", observeAcciOrderSharps);
COVERAGE_SURVEYOR("others", "key_attributes", observeKeyAttributes);
COVERAGE_SURVEYOR("others", "key_formats", observeKeyFormats);
COVERAGE_SURVEYOR("others", "key_map_arrays", observeKeyMapArrays);
COVERAGE_SURVEYOR("others", "tonal_center_flats", observeTonalCenterFlats);
COVERAGE_SURVEYOR("others", "tonal_center_sharps", observeTonalCenterSharps);

} // namespace
