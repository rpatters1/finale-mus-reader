// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/context.h"

#include "coverage/json.h"
#include "musx/musx.h"

namespace finale_mus_reader::coverage {

namespace {

std::string coverageTarget(const InstanceKey& key, std::string_view member)
{
    using namespace musx::dom;
    const auto indexed = [&](std::string_view prefix) {
        auto result = std::string(prefix);
        if (key.cmper1) result += '[' + std::to_string(*key.cmper1) + ']';
        if (key.cmper2) result += '[' + std::to_string(*key.cmper2) + ']';
        if (key.inci) result += '[' + std::to_string(*key.inci) + ']';
        return result + '.' + std::string(member);
    };
    const auto singleton = [&](std::string_view prefix) {
        return std::string(prefix) + '.' + std::string(member);
    };

#define COVERAGE_INDEXED(Type, Prefix) \
    if (key.classType == typeid(Type)) return indexed(Prefix)
#define COVERAGE_SINGLETON(Type, Prefix) \
    if (key.classType == typeid(Type)) return singleton(Prefix)
    COVERAGE_INDEXED(others::TextBlock, "others.textBlock");
    COVERAGE_INDEXED(others::ShapeDef, "others.shapeDef");
    COVERAGE_INDEXED(others::ShapeData, "others.shapeData");
    COVERAGE_INDEXED(others::ShapeInstructionList, "others.shapeList");
    COVERAGE_INDEXED(others::PageGraphicAssign, "others.pageGraphicAssign");
    COVERAGE_INDEXED(others::ShapeGraphicAssign, "others.shapeGraphicAssign");
    COVERAGE_INDEXED(details::MeasureGraphicAssign, "details.measureGraphicAssign");
    COVERAGE_INDEXED(texts::BlockText, "texts.blockText");
    COVERAGE_INDEXED(texts::LyricsVerse, "texts.verse");
    COVERAGE_INDEXED(texts::LyricsChorus, "texts.chorus");
    COVERAGE_INDEXED(texts::LyricsSection, "texts.section");
    COVERAGE_INDEXED(texts::SmartShapeText, "texts.smartShape");
    COVERAGE_INDEXED(texts::BookmarkText, "texts.bookmark");
    COVERAGE_INDEXED(texts::ExpressionText, "texts.expression");
    COVERAGE_INDEXED(texts::FileInfoText, "texts.fileInfo");
    COVERAGE_SINGLETON(options::ClefOptions, "options.clefOptions");
    COVERAGE_SINGLETON(options::StemOptions, "options.stemOptions");
    COVERAGE_SINGLETON(options::TextOptions, "options.textOptions");
    COVERAGE_SINGLETON(options::LyricOptions, "options.lyricOptions");
    COVERAGE_SINGLETON(options::MultimeasureRestOptions, "options.multimeasureRestOptions");
    COVERAGE_SINGLETON(options::MusicSpacingOptions, "options.musicSpacing");
    if (key.classType == typeid(options::FontOptions)) {
        if (member.starts_with("fonts[")) {
            return "options.fontOptions[" + std::string(member.substr(6));
        }
        if (member.starts_with("physical[")) {
            return "options.fontOptionsPhysical[" + std::string(member.substr(9));
        }
        return singleton("options.fontOptions");
    }
#undef COVERAGE_SINGLETON
#undef COVERAGE_INDEXED
    return indexed(key.classType.name());
}

} // namespace

FieldIndex::FieldIndex(const ImportReport& report)
{
    for (const auto& [instance, fields] : report.fields) {
        for (const auto& [member, info] : fields) {
            byTarget_.emplace(coverageTarget(instance, member), info);
        }
    }
    for (const auto& [instance, fields] : report.textFields) {
        for (const auto& [member, info] : fields) {
            textByTarget_.emplace(coverageTarget(instance, member), info);
        }
    }
}

const char* FieldIndex::originOf(const std::string& target) const
{
    const auto found = byTarget_.find(target);
    return found == byTarget_.end() ? "absent" : originName(found->second.origin);
}

const TextFieldInfo* FieldIndex::textInfoOf(const std::string& target) const
{
    const auto found = textByTarget_.find(target);
    return found == textByTarget_.end() ? nullptr : &found->second;
}

} // namespace finale_mus_reader::coverage
