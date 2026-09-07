// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/registry.h"
#include "coverage/schema.h"
#include "musx/musx.h"

namespace {

using namespace finale_mus_reader::coverage;

constexpr std::string_view filePathsCoverageKey = "file_paths";
constexpr std::string_view fileAliasesCoverageKey = "file_aliases";
constexpr std::string_view fileDescriptionsCoverageKey = "file_descriptions";
constexpr std::string_view fileUrlBookmarksCoverageKey = "file_url_bookmarks";

std::optional<DifferenceClassification> classifyEmbeddedLocator(const DifferenceContext& context,
    std::string_view locatorPath, std::string_view classKey, std::string_view member)
{
    const bool resolvePath = classKey == filePathsCoverageKey;
    const auto& source = context.sourceDocumentLeaves ? *context.sourceDocumentLeaves : context.source;
    bool embedded = false;
    constexpr std::string_view descMember = ".f_desc_id";
    for (const auto& [path, leaf] : source) {
        if (!path.ends_with(descMember) || !leaf.first.isInteger()
            || leaf.first.asInteger() == 0
            || !(path.starts_with("page_graphic_assigns[")
                || path.starts_with("shape_graphic_assigns[")
                || path.starts_with("meas_graphic_assigns["))) continue;
        const auto assignment = path.substr(0, path.size() - descMember.size());
        const auto identity = path.find('[') + 1;
        const auto cmper = path.find("cmper", identity);
        if (cmper == std::string::npos) continue;
        const auto part = path.substr(identity, cmper - identity);
        const auto descriptionKey = [&](std::string_view partPrefix) {
            return std::string(fileDescriptionsCoverageKey) + "[" + std::string(partPrefix) + "cmper="
                + std::to_string(leaf.first.asInteger()) + "].path_id";
        };
        auto pathId = !resolvePath ? std::optional{leaf.first.asInteger()}
                            : comparisonIntegerLeaf(source, descriptionKey(part));
        if (resolvePath && !pathId && !part.empty())
            pathId = comparisonIntegerLeaf(source, descriptionKey({}));
        if (!pathId || *pathId == 0) continue;
        const auto pathKey = [&](std::string_view partPrefix) {
            return std::string(classKey)
                + "[" + std::string(partPrefix) + "cmper=" + std::to_string(*pathId)
                + "]." + std::string(member);
        };
        auto targetPath = pathKey(part);
        if (!source.contains(targetPath) && !part.empty()) targetPath = pathKey({});
        if (targetPath != locatorPath) continue;

        // Resolve graphic locators on the source side. A locator also used
        // by a linked or unresolved assignment must still compare as an external locator.
        const auto graphic = comparisonIntegerLeaf(source, assignment + ".graphic_cmper");
        if (!graphic || *graphic <= 0) return std::nullopt;
        embedded = true;
    }
    return embedded ? std::optional{DifferenceClassification::FinaleUpgradeNormalization}
                    : std::nullopt;
}

std::optional<DifferenceClassification> classifyEmbeddedFilePath(const DifferenceContext& context)
{
    if (context.category != DifferenceCategory::Differs || context.origin != "legacy-mus"
        || !context.path.starts_with(std::string(filePathsCoverageKey) + "[")
        || !context.path.ends_with(".path") || !context.sourceValue.isString()
        || !context.companionValue.isString()) return std::nullopt;
    return classifyEmbeddedLocator(context, context.path, filePathsCoverageKey, "path");
}

bool hasLocatorBlob(const ComparisonLeaves& leaves, const std::string& object, std::string_view member)
{
    const auto length = comparisonIntegerLeaf(leaves, object + ".length");
    const auto blob = leaves.find(object + "." + std::string(member));
    return length && *length >= 0 && blob != leaves.end() && blob->second.first.isBlob()
        && static_cast<std::uint64_t>(*length) == blob->second.first.asBlob().size();
}

std::optional<DifferenceClassification> classifySynthesizedFileLocator(const DifferenceContext& context,
    std::string_view classKey, std::string_view blobMember)
{
    if (context.category != DifferenceCategory::CompanionOnly || !context.origin.empty()
        || !context.path.starts_with(std::string(classKey) + "[")) return std::nullopt;
    const auto end = context.path.find("].");
    if (end == std::string_view::npos) return std::nullopt;
    const auto member = context.path.substr(end + 2);
    if (member != "share_mode" && member != "length" && member != blobMember) return std::nullopt;
    const auto object = std::string(context.path.substr(0, end + 1));
    const auto& source = context.sourceDocumentLeaves ? *context.sourceDocumentLeaves : context.source;
    const auto& companion = context.companionDocumentLeaves ? *context.companionDocumentLeaves : context.companion;
    if (source.contains(object + ".length") || !hasLocatorBlob(companion, object, blobMember))
        return std::nullopt;
    const auto identity = std::string(context.path.substr(classKey.size(), end + 1 - classKey.size()));
    if (classKey == fileUrlBookmarksCoverageKey) {
        if (!hasLocatorBlob(companion, std::string(fileAliasesCoverageKey) + identity, "alias_handle"))
            return std::nullopt;
    } else {
        using PathType = musx::dom::others::FileDescription::PathType;
        const auto typePath = std::string(fileDescriptionsCoverageKey) + identity + ".path_type";
        if (comparisonIntegerLeaf(source, typePath) != static_cast<std::int64_t>(PathType::DosPath)
            || comparisonIntegerLeaf(companion, typePath) != static_cast<std::int64_t>(PathType::MacAlias)
            || source.at(typePath).second != "legacy-mus"
            || !classifyEmbeddedLocator(context, typePath, fileDescriptionsCoverageKey, "path_type"))
            return std::nullopt;
    }
    return DifferenceClassification::FinaleUpgradeSynthesis;
}

std::optional<DifferenceClassification> classifyFileUrlBookmark(const DifferenceContext& context)
{
    return classifySynthesizedFileLocator(context, fileUrlBookmarksCoverageKey, "url_bookmark_data");
}

std::optional<DifferenceClassification> classifyEmbeddedFileAlias(const DifferenceContext& context)
{
    if (context.category == DifferenceCategory::CompanionOnly)
        return classifySynthesizedFileLocator(context, fileAliasesCoverageKey, "alias_handle");
    if (context.category != DifferenceCategory::Differs
        || !context.path.starts_with(std::string(fileAliasesCoverageKey) + "["))
        return std::nullopt;
    const auto end = context.path.find("].");
    if (end == std::string_view::npos) return std::nullopt;
    const auto member = context.path.substr(end + 2);
    const bool length = member == "length";
    if (!length && member != "alias_handle")
        return std::nullopt;
    if (context.origin != (length ? "legacy-mus" : "legacy-mus-adjusted")
        || (!length && (!context.sourceValue.isBlob() || !context.companionValue.isBlob()))) {
        return std::nullopt;
    }
    const auto lengthPath = std::string(context.path.substr(0, end + 2)) + "length";
    // Only paired aliases qualify; missing whole objects remain recovery gaps.
    if (!comparisonIntegerLeaf(context.source, lengthPath)
        || !comparisonIntegerLeaf(context.companion, lengthPath)) return std::nullopt;
    return classifyEmbeddedLocator(context, lengthPath, fileAliasesCoverageKey, "length");
}

std::optional<DifferenceClassification> classifyEmbeddedFileDescription(const DifferenceContext& context)
{
    using PathType = musx::dom::others::FileDescription::PathType;
    if (context.category != DifferenceCategory::Differs || context.origin != "legacy-mus"
        || !context.path.starts_with(std::string(fileDescriptionsCoverageKey) + "[")
        || !context.sourceValue.isInteger() || !context.companionValue.isInteger())
        return std::nullopt;
    if (context.path.ends_with(".dir_id"))
        return classifyEmbeddedLocator(context, context.path, fileDescriptionsCoverageKey, "dir_id");
    if (!context.path.ends_with(".path_type")
        || (context.sourceValue.asInteger() != static_cast<std::int64_t>(PathType::MacFsSpec)
            && context.sourceValue.asInteger() != static_cast<std::int64_t>(PathType::DosPath))
        || context.companionValue.asInteger() != static_cast<std::int64_t>(PathType::MacAlias))
        return std::nullopt;
    return classifyEmbeddedLocator(context, context.path, fileDescriptionsCoverageKey, "path_type");
}

template <typename Target, typename Populate>
Value observeFileLocators(const SurveyContext& ctx, Populate populate)
{
    Value::Array result;
    for (const auto& target : sourceInstances<Target>(ctx)) {
        auto value = observe(*target, ctx,
            field("cmper", [](const Target& item) { return item.getCmper(); }));
        const auto member = [&](const char* name, const char* originName, auto accessor) {
            const auto& stored = (*target).*accessor;
            auto& object = value.asObject();
            if constexpr (std::is_same_v<std::decay_t<decltype(stored)>, Value::Blob>)
                object.emplace(name, Value(stored));
            else
                object.emplace(name, detail::schemaValue(stored));
            object.emplace(std::string("origin_") + originName,
                fieldOrigin<Target>(ctx, originName, *target));
        };
        populate(member);
        result.emplace_back(std::move(value));
    }
    return result;
}

Value observeFileAliases(const SurveyContext& ctx)
{
    using Target = musx::dom::others::FileAlias;
    return observeFileLocators<Target>(ctx, [](auto member) {
        member("length", "length", &Target::length);
        member("alias_handle", "aliasHandle", &Target::aliasHandle);
    });
}

Value observeFileDescriptions(const SurveyContext& ctx)
{
    using Target = musx::dom::others::FileDescription;
    return observeFileLocators<Target>(ctx, [](auto member) {
        member("version", "version", &Target::version);
        member("vol_ref_num", "volRefNum", &Target::volRefNum);
        member("dir_id", "dirId", &Target::dirId);
        member("path_type", "pathType", &Target::pathType);
        member("path_id", "pathId", &Target::pathId);
    });
}

Value observeFilePaths(const SurveyContext& ctx)
{
    using Target = musx::dom::others::FilePath;
    return observeFileLocators<Target>(ctx, [](auto member) {
        member("path", "path", &Target::path);
    });
}

Value observeFileUrlBookmarks(const SurveyContext& ctx)
{
    using Target = musx::dom::others::FileUrlBookmark;
    return observeFileLocators<Target>(ctx, [](auto member) {
        member("length", "length", &Target::length);
        member("url_bookmark_data", "urlBookmarkData", &Target::urlBookmarkData);
    });
}

COVERAGE_CLASS("others", fileAliasesCoverageKey, observeFileAliases, classifyEmbeddedFileAlias);
COVERAGE_CLASS("others", fileDescriptionsCoverageKey, observeFileDescriptions, classifyEmbeddedFileDescription);
COVERAGE_CLASS("others", filePathsCoverageKey, observeFilePaths, classifyEmbeddedFilePath);
COVERAGE_CLASS("others", fileUrlBookmarksCoverageKey, observeFileUrlBookmarks, classifyFileUrlBookmark);

} // namespace
