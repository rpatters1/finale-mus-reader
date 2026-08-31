// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <string>

#include "coverage/common/music_symbol_info.h"
#include "coverage/registry.h"
#include "coverage/value.h"
#include "import/support/field_manifest.h"

namespace {

using namespace finale_mus_reader::coverage;

bool isMusicSymbolOptionsLeaf(std::string_view path,
    char32_t finale_mus_reader::options::MusicSymbolOptionsTarget::*member)
{
    constexpr std::string_view prefix = "music_symbol_options.";
    for (const auto& field : finale_mus_reader::options::musicSymbolOptionsFields()) {
        if (field.member == member && path.size() == prefix.size() + field.leafName.size()
            && path.starts_with(prefix) && path.ends_with(field.leafName)) {
            return true;
        }
    }
    return false;
}

std::optional<DifferenceClassification>
classifyMusicSymbolOptionsDifference(const DifferenceContext& context)
{
    using Target = finale_mus_reader::options::MusicSymbolOptionsTarget;
    if (const auto conversionLoss = classifyDoubleWholeSlashConversionLoss(context)) {
        return conversionLoss;
    }
    if (const auto slashDefault = classifyVersionlessCodaSlashDefault(context)) {
        return slashDefault;
    }
    if (context.category != DifferenceCategory::Differs) {
        return std::nullopt;
    }

    if (context.origin != "finale27-default") return std::nullopt;

    const bool predatesExpandedCharacters = sourcePredatesVersion(context.epoch,
        context.sourceVersion, finale_mus_reader::FormatEpoch::UncompressedLegacy,
        finale_mus_reader::versions::finale3_5);
    if (predatesExpandedCharacters
        && (isMusicSymbolOptionsLeaf(context.path, &Target::eightVbDown)
            || isMusicSymbolOptionsLeaf(context.path, &Target::oneBarRepeat)
            || isMusicSymbolOptionsLeaf(context.path, &Target::twoBarRepeat)
            || isMusicSymbolOptionsLeaf(context.path, &Target::flagStraightUp)
            || isMusicSymbolOptionsLeaf(context.path, &Target::flagStraightDown))) {
        return DifferenceClassification::DifferentDefaults;
    }

    const bool predatesSeparate16thFlags = sourcePredatesVersion(context.epoch,
        context.sourceVersion, finale_mus_reader::FormatEpoch::UncompressedLegacy,
        finale_mus_reader::versions::finale3_5_1);
    if (predatesSeparate16thFlags
        && (isMusicSymbolOptionsLeaf(context.path, &Target::flag16Up)
            || isMusicSymbolOptionsLeaf(context.path, &Target::flag16Down)
            || isMusicSymbolOptionsLeaf(context.path, &Target::fifteenMaUp)
            || isMusicSymbolOptionsLeaf(context.path, &Target::fifteenMbDown)
            || isMusicSymbolOptionsLeaf(context.path, &Target::trillChar)
            || isMusicSymbolOptionsLeaf(context.path, &Target::wiggleChar))) {
        return DifferenceClassification::DifferentDefaults;
    }

    if (context.epoch == finale_mus_reader::FormatEpoch::CodaBanner
        && isMusicSymbolOptionsLeaf(context.path, &Target::rest128th)) {
        return DifferenceClassification::DifferentDefaults;
    }
    return std::nullopt;
}

Value observeMusicSymbolOptions(const SurveyContext& context)
{
    using Target = finale_mus_reader::options::MusicSymbolOptionsTarget;
    const auto options = context.document->getOptions()->get<Target>();
    if (!options) return {};

    Value::Object result;
    for (const auto& field : finale_mus_reader::options::musicSymbolOptionsFields()) {
        result.emplace(field.leafName,
            static_cast<std::int64_t>(options.get()->*field.member));
        result.emplace(std::string("origin_") + std::string(field.memberName),
            fieldOrigin<Target>(context, field.memberName));
    }
    return result;
}

COVERAGE_CLASS("options", "music_symbol_options", observeMusicSymbolOptions,
    classifyMusicSymbolOptionsDifference);

} // namespace
