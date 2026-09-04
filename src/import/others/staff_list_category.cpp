// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "import/support/text_encoding.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using CategoryNameTarget = musx::dom::others::StaffListCategoryName;
using CategoryPartsTarget = musx::dom::others::StaffListCategoryParts;
using CategoryScoreTarget = musx::dom::others::StaffListCategoryScore;

constexpr records::LegacyTag categoryNameClass = 0x012f;
constexpr records::LegacyTag categoryScoreClass = 0x0130;
constexpr records::LegacyTag categoryScoreOverrideClass = 0x0131;
constexpr records::LegacyTag categoryPartsClass = 0x0132;
constexpr records::LegacyTag categoryPartsOverrideClass = 0x0133;
constexpr musx::dom::Cmper categoryListCount = 8;

bool hasCategoryRecords(const ImportContext& context)
{
    const auto& pool = context.index.getClassOthers();
    return !pool.cmpersForTag(categoryNameClass).empty()
        || !pool.cmpersForTag(categoryScoreClass).empty()
        || !pool.cmpersForTag(categoryScoreOverrideClass).empty()
        || !pool.cmpersForTag(categoryPartsClass).empty()
        || !pool.cmpersForTag(categoryPartsOverrideClass).empty();
}

bool sourceStoresCategoryLists(const ImportContext& context)
{
    if (context.profile.epoch != FormatEpoch::ZlibLegacy) return false;
    if (hasCategoryRecords(context)) return true;
    return sourceAtOrAfter(context.profile, FormatEpoch::ZlibLegacy,
        versions::finale2009);
}

void reportUnsupportedOverrides(const ImportContext& context, records::LegacyTag identity,
                                std::string_view component)
{
    const RecordFamilySource source{
        .pool = &context.index.getClassOthers(),
        .identity = identity,
        .classRecords = true};
    for (const auto [partId, cmper] : recordKeys(source)) {
        context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
            "Category " + std::string(component) + " staff-list override "
                + std::to_string(cmper) + " for part " + std::to_string(partId)
                + " is unsupported and was ignored."});
    }
}

template <typename Target>
void importCategoryLists(const ImportContext& context, records::LegacyTag identity,
                         std::set<musx::dom::Cmper>& importedCmpers)
{
    const RecordFamilySource source{
        .pool = &context.index.getClassOthers(),
        .identity = identity,
        .classRecords = true};
    for (const auto [partId, cmper] : recordKeys(source)) {
        const auto rows = source.pool->getArray(identity, cmper, 0, partId);
        if (rows.empty()) continue;
        const auto payload = collectRecordPayload(source, rows);
        if (payload.size() % 2 != 0) {
            context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                "Category staff list " + std::to_string(cmper)
                    + " has an incomplete trailing staff value."});
        }
        auto target = createOthersRecordTarget<Target>(
            context.document, source, rows.front(), cmper);
        for (std::size_t offset = 0; offset + 2 <= payload.size(); offset += 2) {
            const auto value =
                static_cast<std::int16_t>(payloadWord(payload, offset, context.profile.byteOrder));
            if (value == 0) break;
            target->values.push_back(value);
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
            const auto key = instanceKey<Target>(partId, cmper);
            FINALE_MUS_READER_REPORT_FIELD(
                context.report, key, "values[" + std::to_string(target->values.size() - 1) + "]",
                {ValueOrigin::LegacyMus, rows.front().blockOffset,
                    rows.front().decodedOffset + offset, value, identity});
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        }
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        context.report.setInstanceOrigin(
            instanceKey<Target>(partId, cmper), ValueOrigin::LegacyMus);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        context.document->getOthers()->add(Target::XmlNodeName, std::move(target));
        importedCmpers.insert(cmper);
    }
}

std::string categoryNameBytes(std::span<const std::uint8_t> payload, ByteOrder byteOrder)
{
    std::string result(reinterpret_cast<const char*>(payload.data()), payload.size());
    if (byteOrder == ByteOrder::BigEndian) {
        for (std::size_t offset = 0; offset + 1 < result.size(); offset += 2) {
            std::swap(result[offset], result[offset + 1]);
        }
    }
    if (const auto end = result.find('\0'); end != std::string::npos) {
        result.resize(end);
    }
    return result;
}

void importCategoryNames(const ImportContext& context,
                         std::set<musx::dom::Cmper>& importedCmpers)
{
    const RecordFamilySource source{
        .pool = &context.index.getClassOthers(),
        .identity = categoryNameClass,
        .classRecords = true};
    for (const auto [partId, cmper] : recordKeys(source)) {
        const auto rows = source.pool->getArray(categoryNameClass, cmper, 0, partId);
        if (rows.empty()) continue;
        const auto payload = collectRecordPayload(source, rows);
        auto target = createOthersRecordTarget<CategoryNameTarget>(
            context.document, source, rows.front(), cmper);
        auto stored = categoryNameBytes(payload, context.profile.byteOrder);
        target->name = text::toUtf8(stored, context.profile.platform);
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        context.report.setInstanceOrigin(instanceKey<CategoryNameTarget>(partId, cmper),
            ValueOrigin::LegacyMus);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        context.document->getOthers()->add(CategoryNameTarget::XmlNodeName, std::move(target));
        importedCmpers.insert(cmper);
    }
}

} // namespace

void importStaffListCategories(const ImportContext& context)
{
    std::set<musx::dom::Cmper> importedCmpers;
    if (sourceStoresCategoryLists(context)) {
        reportUnsupportedOverrides(context, categoryScoreOverrideClass, "score");
        reportUnsupportedOverrides(context, categoryPartsOverrideClass, "parts");
        importCategoryNames(context, importedCmpers);
        importCategoryLists<CategoryPartsTarget>(context, categoryScoreClass, importedCmpers);
        importCategoryLists<CategoryScoreTarget>(context, categoryPartsClass, importedCmpers);
    }
    const auto reportBaselineObject = baselineObjectReporter(context.report);
    for (musx::dom::Cmper cmper = 1; cmper <= categoryListCount; ++cmper) {
        if (importedCmpers.contains(cmper)) continue;
        static_cast<void>(musx::dom::others::importCategoryStaffListInto(context.document,
            context.referenceDocument, cmper, reportBaselineObject));
    }
}

} // namespace others
} // namespace finale_mus_reader
