// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include "import/support/text_encoding.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using CategoryTarget = musx::dom::others::MarkingCategory;
using NameTarget = musx::dom::others::MarkingCategoryName;

constexpr records::LegacyTag markingCategoryClass = 0x012d;
constexpr records::LegacyTag markingCategoryNameClass = 0x012e;
constexpr std::size_t markingCategoryPayloadSize = 36;
constexpr musx::dom::Cmper markingCategoryCannedCount = 7;

constexpr std::size_t markingCategoryTypeOffset = 0;
constexpr std::array<std::size_t, 3> markingCategoryFontOffsets{2, 8, 14};
constexpr std::size_t markingCategoryJustificationOffset = 20;
constexpr std::size_t markingCategoryHorzAlignOffset = 22;
constexpr std::size_t markingCategoryHorzOffsetOffset = 24;
constexpr std::size_t markingCategoryVertAlignOffset = 26;
constexpr std::size_t markingCategoryVertOffsetEntryOffset = 28;
constexpr std::size_t markingCategoryVertOffsetBaselineOffset = 30;
constexpr std::size_t markingCategoryFlagsOffset = 32;
constexpr std::size_t markingCategoryStaffListOffset = 34;

constexpr std::uint16_t usesTextFontMask = 0x0001;
constexpr std::uint16_t usesMusicFontMask = 0x0002;
constexpr std::uint16_t usesNumberFontMask = 0x0004;
constexpr std::uint16_t usesPositioningMask = 0x0008;
constexpr std::uint16_t usesStaffListMask = 0x0010;
constexpr std::uint16_t usesBreakMmRestsMask = 0x0040;
constexpr std::uint16_t userCreatedMask = 0x0080;
constexpr std::uint16_t breakMmRestMask = 0x0400;

std::optional<musx::dom::AlignJustify> markingCategoryJustification(std::uint16_t stored)
{
    using A = musx::dom::AlignJustify;
    switch (stored) {
    case 0: return A::Left;
    case 1: return A::Center;
    case 2: return A::Right;
    default: return std::nullopt;
    }
}

std::optional<musx::dom::others::HorizontalMeasExprAlign> markingCategoryHorizontalAlignment(
    std::uint16_t stored)
{
    using A = musx::dom::others::HorizontalMeasExprAlign;
    switch (stored) {
    case 0: return A::LeftBarline;
    case 1: return A::StartTimeSig;
    case 2: return A::AfterClefKeyTime;
    case 3: return A::Manual;
    case 4: return A::CenterOverBarlines;
    case 5: return A::CenterOverMusic;
    case 6: return A::RightBarline;
    case 7: return A::StartOfMusic;
    case 9: return A::LeftOfAllNoteheads;
    case 10: return A::Stem;
    case 11: return A::CenterPrimaryNotehead;
    case 12: return A::CenterAllNoteheads;
    case 13: return A::LeftOfPrimaryNotehead;
    case 14: return A::RightOfAllNoteheads;
    default: return std::nullopt;
    }
}

std::optional<musx::dom::others::VerticalMeasExprAlign> markingCategoryVerticalAlignment(
    std::uint16_t stored)
{
    using A = musx::dom::others::VerticalMeasExprAlign;
    switch (stored) {
    case 0: return A::AboveStaff;
    case 1: return A::BelowStaff;
    case 2: return A::Manual;
    case 3: return A::RefLine;
    case 4: return A::TopNote;
    case 5: return A::BottomNote;
    case 6: return A::AboveEntry;
    case 7: return A::BelowEntry;
    case 8: return A::AboveStaffOrEntry;
    case 9: return A::BelowStaffOrEntry;
    default: return std::nullopt;
    }
}

std::shared_ptr<musx::dom::FontInfo> readMarkingCategoryFont(
    const ImportContext& context, std::span<const std::uint8_t> payload, std::size_t offset)
{
    auto result = std::make_shared<musx::dom::FontInfo>(context.document);
    result->fontId = context.construction.assignFontId(
        payloadWord(payload, offset, context.profile.byteOrder));
    result->fontSize =
        static_cast<std::int16_t>(payloadWord(payload, offset + 2, context.profile.byteOrder));
    result->setEnigmaStyles(payloadWord(payload, offset + 4, context.profile.byteOrder));
    return result;
}

void reportMarkingCategoryFont([[maybe_unused]] const ImportContext& context,
    [[maybe_unused]] const InstanceKey& key,
    [[maybe_unused]] std::string prefix,
    [[maybe_unused]] const musx::dom::FontInfo& font,
    [[maybe_unused]] const records::LegacyRow& row,
    [[maybe_unused]] std::size_t offset)
{
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    const auto report = [&](std::string member, std::size_t memberOffset, std::int64_t value) {
        FINALE_MUS_READER_REPORT_FIELD(context.report, key, prefix + std::move(member),
            FieldInfo{ValueOrigin::LegacyMus, row.blockOffset,
                row.decodedOffset + offset + memberOffset, value, markingCategoryClass});
    };
    report("fontId", 0, font.fontId);
    report("fontSize", 2, font.fontSize);
    report("bold", 4, font.bold);
    report("italic", 4, font.italic);
    report("underline", 4, font.underline);
    report("strikeout", 4, font.strikeout);
    report("absolute", 4, font.absolute);
    report("hidden", 4, font.hidden);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

void reportMarkingCategory([[maybe_unused]] const ImportContext& context,
    [[maybe_unused]] const CategoryTarget& category,
    [[maybe_unused]] const records::LegacyRow& row)
{
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    const auto key = instanceKey<CategoryTarget>(category.getSourcePartId(), category.getCmper());
    context.report.setInstanceOrigin(key, ValueOrigin::LegacyMus);
    const auto report = [&](const char* member, std::size_t offset, std::int64_t value) {
        FINALE_MUS_READER_REPORT_FIELD(context.report, key, member,
            FieldInfo{ValueOrigin::LegacyMus, row.blockOffset, row.decodedOffset + offset, value,
                markingCategoryClass});
    };
    report("categoryType", markingCategoryTypeOffset,
        static_cast<std::int64_t>(category.categoryType));
    reportMarkingCategoryFont(
        context, key, "textFont.", *category.textFont, row, markingCategoryFontOffsets[0]);
    reportMarkingCategoryFont(
        context, key, "musicFont.", *category.musicFont, row, markingCategoryFontOffsets[1]);
    reportMarkingCategoryFont(
        context, key, "numberFont.", *category.numberFont, row, markingCategoryFontOffsets[2]);
    report("justification", markingCategoryJustificationOffset,
        static_cast<std::int64_t>(category.justification));
    report(
        "horzAlign", markingCategoryHorzAlignOffset, static_cast<std::int64_t>(category.horzAlign));
    report("horzOffset", markingCategoryHorzOffsetOffset, category.horzOffset);
    report(
        "vertAlign", markingCategoryVertAlignOffset, static_cast<std::int64_t>(category.vertAlign));
    report("vertOffsetEntry", markingCategoryVertOffsetEntryOffset, category.vertOffsetEntry);
    report(
        "vertOffsetBaseline", markingCategoryVertOffsetBaselineOffset, category.vertOffsetBaseline);
    report("usesTextFont", markingCategoryFlagsOffset, category.usesTextFont);
    report("usesMusicFont", markingCategoryFlagsOffset, category.usesMusicFont);
    report("usesNumberFont", markingCategoryFlagsOffset, category.usesNumberFont);
    report("usesPositioning", markingCategoryFlagsOffset, category.usesPositioning);
    report("usesStaffList", markingCategoryFlagsOffset, category.usesStaffList);
    report("usesBreakMmRests", markingCategoryFlagsOffset, category.usesBreakMmRests);
    report("breakMmRest", markingCategoryFlagsOffset, category.breakMmRest);
    report("userCreated", markingCategoryFlagsOffset, category.userCreated);
    report("staffList", markingCategoryStaffListOffset, category.staffList);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

void importMarkingCategoryName(const ImportContext& context,
    const RecordFamilySource& source,
    const records::LegacyRow& categoryRow,
    musx::dom::Cmper cmper)
{
    const auto* row = source.pool->get(source.identity, cmper, 0, 0, categoryRow.partId);
    if (!row)
        return;
    const auto payload = source.pool->effectivePayloadOf(*row);
    auto name = createOthersRecordTarget<NameTarget>(context.document, source, *row, cmper);
    if (versions::storesUnicodeCodepoints(context.profile.version)) {
        name->name = text::utf16ToUtf8(payloadWords(payload, context.profile.byteOrder));
    } else {
        name->name =
            text::toUtf8(payloadString(payload, 0, payload.size()), context.profile.platform);
    }
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    const auto key = instanceKey<NameTarget>(name->getSourcePartId(), cmper);
    context.report.setInstanceOrigin(key, ValueOrigin::LegacyMus);
    FINALE_MUS_READER_REPORT_FIELD(context.report, key, "name",
        FieldInfo{ValueOrigin::LegacyMus, row->blockOffset, row->decodedOffset,
            static_cast<std::int64_t>(payload.size()), markingCategoryNameClass});
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    context.document->getOthers()->add(NameTarget::XmlNodeName, std::move(name));
}

void importSourceMarkingCategories(const ImportContext& context)
{
    if (context.profile.epoch != FormatEpoch::ZlibLegacy) {
        // Coda-banner, uncompressed, and DCL files predate the class. The proposed
        // MC/mn fixed tags remain unevidenced, so interpreting an accidental match
        // would be unsafe.
        return;
    }
    const RecordFamilySource categorySource{.pool = &context.index.getClassOthers(),
        .identity = markingCategoryClass,
        .classRecords = true};
    const RecordFamilySource nameSource{.pool = &context.index.getClassOthers(),
        .identity = markingCategoryNameClass,
        .classRecords = true};
    for (const auto [partId, cmper] : recordKeys(categorySource)) {
        const auto rows = categorySource.pool->getArray(categorySource.identity, cmper, 0, partId);
        if (rows.empty())
            continue;
        const auto& row = rows.front();
        const auto payload = categorySource.pool->effectivePayloadOf(row);
        if (payload.size() < markingCategoryPayloadSize) {
            context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                "Marking category " + std::to_string(cmper) +
                    " is shorter than its 36-byte layout and was ignored."});
            continue;
        }
        auto category =
            createOthersRecordTarget<CategoryTarget>(context.document, categorySource, row, cmper);
        const auto word = [&](std::size_t offset) {
            return payloadWord(payload, offset, context.profile.byteOrder);
        };
        category->categoryType =
            static_cast<CategoryTarget::CategoryType>(word(markingCategoryTypeOffset));
        category->textFont =
            readMarkingCategoryFont(context, payload, markingCategoryFontOffsets[0]);
        category->musicFont =
            readMarkingCategoryFont(context, payload, markingCategoryFontOffsets[1]);
        category->numberFont =
            readMarkingCategoryFont(context, payload, markingCategoryFontOffsets[2]);
        if (const auto value =
                markingCategoryJustification(word(markingCategoryJustificationOffset))) {
            category->justification = *value;
        }
        if (const auto value =
                markingCategoryHorizontalAlignment(word(markingCategoryHorzAlignOffset))) {
            category->horzAlign = *value;
        }
        category->horzOffset = static_cast<std::int16_t>(word(markingCategoryHorzOffsetOffset));
        if (const auto value =
                markingCategoryVerticalAlignment(word(markingCategoryVertAlignOffset))) {
            category->vertAlign = *value;
        }
        category->vertOffsetEntry =
            static_cast<std::int16_t>(word(markingCategoryVertOffsetEntryOffset));
        category->vertOffsetBaseline =
            static_cast<std::int16_t>(word(markingCategoryVertOffsetBaselineOffset));
        const auto flags = word(markingCategoryFlagsOffset);
        category->usesTextFont = (flags & usesTextFontMask) != 0;
        category->usesMusicFont = (flags & usesMusicFontMask) != 0;
        category->usesNumberFont = (flags & usesNumberFontMask) != 0;
        category->usesPositioning = (flags & usesPositioningMask) != 0;
        category->usesStaffList = (flags & usesStaffListMask) != 0;
        category->usesBreakMmRests = (flags & usesBreakMmRestsMask) != 0;
        category->userCreated = (flags & userCreatedMask) != 0;
        category->breakMmRest = (flags & breakMmRestMask) != 0;
        category->staffList = word(markingCategoryStaffListOffset);
        reportMarkingCategory(context, *category, row);
        context.document->getOthers()->add(CategoryTarget::XmlNodeName, std::move(category));
        importMarkingCategoryName(context, nameSource, row, cmper);
    }
}

} // namespace

void importMarkingCategories(const ImportContext& context)
{
    importSourceMarkingCategories(context);
    if (!sourcePredatesVersion(context.profile, FormatEpoch::ZlibLegacy, versions::finale2009)) {
        return;
    }
    const auto reportBaselineObject = baselineObjectReporter(context.report);
    for (musx::dom::Cmper cmper = 1; cmper <= markingCategoryCannedCount; ++cmper) {
        static_cast<void>(musx::dom::others::importMarkingCategoryInto(
            context.document, context.referenceDocument, cmper, reportBaselineObject));
    }
}

} // namespace others
} // namespace finale_mus_reader
