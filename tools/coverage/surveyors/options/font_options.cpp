// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <map>
#include <optional>
#include <string>

#include "coverage/json.h"
#include "coverage/registry.h"
#include "coverage/value.h"
#include "finale_mus_reader/reader.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;
using finale_mus_reader::FieldInfo;

struct Tuple
{
    std::optional<FieldInfo> fontId;
    std::optional<FieldInfo> fontSize;
    std::optional<FieldInfo> effects;
};

std::map<std::size_t, Tuple> collectTuples(const finale_mus_reader::ImportReport& report)
{
    std::map<std::size_t, Tuple> result;
    const auto foundInstance = report.fields.find(
        finale_mus_reader::instanceKey<musx::dom::options::FontOptions>());
    if (foundInstance == report.fields.end()) return result;
    constexpr std::string_view prefix = "fonts[";
    for (const auto& [name, field] : foundInstance->second) {
        if (!std::string_view(name).starts_with(prefix)) continue;
        const auto close = name.find(']', prefix.size());
        if (close == std::string::npos || close + 2 > name.size()) continue;
        const auto ordinal = static_cast<std::size_t>(
            std::stoul(name.substr(prefix.size(), close - prefix.size())));
        const auto member = std::string_view(name).substr(close + 2);
        auto& tuple = result[ordinal];
        if (member == "fontId") tuple.fontId = field;
        else if (member == "fontSize") tuple.fontSize = field;
        else if (member == "effects") tuple.effects = field;
    }
    return result;
}

// The decoded field offset a source tuple's word occupied, so a disagreement can be
// chased back to its exact bytes without re-deriving the epoch's record layout by hand.
std::size_t decodedFieldOffset(const FieldInfo& field, finale_mus_reader::FormatEpoch epoch,
    std::size_t ordinal, std::size_t fieldIndex)
{
    std::size_t offset = field.decodedOffset;
    if (epoch == finale_mus_reader::FormatEpoch::DclLegacy) {
        constexpr std::size_t fixedRowHeaderSize = 4;
        constexpr std::size_t wordsPerRow = 6;
        offset += fixedRowHeaderSize + ((ordinal * 3 + fieldIndex) % wordsPerRow) * 2;
    } else if (epoch == finale_mus_reader::FormatEpoch::ZlibLegacy) {
        constexpr std::size_t classRecordHeaderSize = 10;
        offset += classRecordHeaderSize + ordinal * 6 + fieldIndex * 2;
    }
    return offset;
}

void addSourceField(Value::Object& result, std::string_view name, const FieldInfo& field,
    finale_mus_reader::FormatEpoch epoch, std::size_t ordinal, std::size_t fieldIndex)
{
    result.emplace(name, field.rawValue);
    result.emplace(std::string(name) + "_block_offset", field.blockOffset);
    result.emplace(std::string(name) + "_decoded_field_offset",
        decodedFieldOffset(field, epoch, ordinal, fieldIndex));
}

Value observeTuple(std::size_t ordinal, const Tuple& tuple,
    const SurveyContext& ctx)
{
    if (!tuple.fontId || !tuple.fontSize || !tuple.effects) return {};

    const auto options = ctx.document->getOptions()->get<musx::dom::options::FontOptions>();
    const auto actual = options->getFontInfo(
        static_cast<musx::dom::options::FontOptions::FontType>(ordinal));
    std::string fontStatus = "missing";
    std::string fontName;
    const auto fontId = actual->fontId;
    if (fontId == 0) fontStatus = "default";
    if (const auto font = ctx.document->getOthers()
            ->get<musx::dom::others::FontDefinition>(musx::dom::SCORE_PARTID, fontId)) {
        if (fontId != 0) fontStatus = "resolved";
        fontName = font->name;
    }

    Value::Object result{{"ordinal", ordinal}};
    addSourceField(result, "source_font_id", *tuple.fontId, ctx.report.formatEpoch, ordinal, 0);
    addSourceField(result, "source_font_size", *tuple.fontSize, ctx.report.formatEpoch, ordinal, 1);
    addSourceField(result, "source_effects", *tuple.effects, ctx.report.formatEpoch, ordinal, 2);
    result.insert({{"font_id", fontId}, {"font_size", actual->fontSize},
        {"effects", actual->getEnigmaStyles()}, {"bold", actual->bold},
        {"italic", actual->italic}, {"underline", actual->underline},
        {"strikeout", actual->strikeout}, {"absolute", actual->absolute},
        {"hidden", actual->hidden},
        {"origin_bold", std::string(originName(tuple.effects->origin))},
        {"origin_italic", std::string(originName(tuple.effects->origin))},
        {"origin_underline", std::string(originName(tuple.effects->origin))},
        {"origin_strikeout", std::string(originName(tuple.effects->origin))},
        {"origin_absolute", std::string(originName(tuple.effects->origin))},
        {"origin_hidden", std::string(originName(tuple.effects->origin))},
        {"font_id_origin", std::string(originName(tuple.fontId->origin))},
        {"font_size_origin", std::string(originName(tuple.fontSize->origin))},
        {"effects_origin", std::string(originName(tuple.effects->origin))},
        {"font_status", fontStatus}, {"font_name", fontName},
        {"normalized_font_name", musx::dom::normalizeFontName(fontName)}});
    return result;
}

Value observeFontOptions(const SurveyContext& ctx)
{
    const auto options = ctx.document->getOptions()->get<musx::dom::options::FontOptions>();
    if (!options) return {};
    const auto tuples = collectTuples(ctx.report);

    std::size_t danglingNonzeroCount = 0;
    std::size_t recoveredCount = 0;
    std::size_t behaviorCount = 0;
    std::size_t defaultCount = 0;
    std::size_t unmappedCount = 0;
    std::size_t musxOnlyCount = 0;
    for (const auto& [type, font] : options->fontOptions) {
        static_cast<void>(type);
        if (font->fontId != 0
                && !ctx.document->getOthers()->get<musx::dom::others::FontDefinition>(
                musx::dom::SCORE_PARTID, font->fontId)) {
            ++danglingNonzeroCount;
        }
    }
    for (const auto& [ordinal, tuple] : tuples) {
        if (!tuple.fontId) continue;
        switch (tuple.fontId->origin) {
        case finale_mus_reader::ValueOrigin::Unmapped: ++unmappedCount; break;
        case finale_mus_reader::ValueOrigin::MusxOnly: ++musxOnlyCount; break;
        case finale_mus_reader::ValueOrigin::LegacyMus: ++recoveredCount; break;
        case finale_mus_reader::ValueOrigin::LegacyMusAdjusted: ++recoveredCount; break;
        case finale_mus_reader::ValueOrigin::LegacyBehavior: ++behaviorCount; break;
        case finale_mus_reader::ValueOrigin::Finale27Default: ++defaultCount; break;
        }
    }

    Value::Array observedTuples;
    for (const auto& [ordinal, tuple] : tuples) {
        if (!tuple.fontId || !tuple.fontSize || !tuple.effects) continue;
        observedTuples.emplace_back(observeTuple(ordinal, tuple, ctx));
    }
    return Value::Object{{"option_count", options->fontOptions.size()}, {"recovered_count", recoveredCount},
        {"legacy_behavior_count", behaviorCount}, {"default_count", defaultCount},
        {"unmapped_count", unmappedCount}, {"musx_only_count", musxOnlyCount},
        {"dangling_nonzero_font_id_count", danglingNonzeroCount}, {"tuples", std::move(observedTuples)}};
}

COVERAGE_SURVEYOR("options", "font_options", observeFontOptions);

} // namespace
