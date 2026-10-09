// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/details.h"

#include <cstddef>
#include <cstdint>

#include "musx/musx.h"

namespace finale_mus_reader::details {
namespace {

using Target = musx::dom::details::SmartShapeEntryAssign;
constexpr auto assignmentTag = records::packTag("Ex");
constexpr records::LegacyTag assignmentClass = 0x041a;
constexpr records::LegacyTag laterAssignmentClass = 0x0428;
constexpr std::size_t wordsPerAssignment = 5;

void importSource(const ImportContext& context, const RecordFamilySource& source)
{
    for (const auto& [partId, entryHigh] : recordKeys(source)) {
        for (const auto entryLow : source.pool->secondCmpersForTag(source.identity, entryHigh, partId)) {
            const auto rows = source.pool->getArray(source.identity, entryHigh, entryLow, partId);
            const auto words = collectRecordWords(source, rows, context.profile.byteOrder);
            if (words.size() % wordsPerAssignment != 0) {
                context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info, "SmartShape entry assignment has an incomplete tuple."});
            }
            for (std::size_t at = 0; at + wordsPerAssignment <= words.size(); at += wordsPerAssignment) {
                const auto inci = static_cast<musx::dom::Inci>(at / wordsPerAssignment);
                auto target = createDetailsRecordTarget<Target>(context.document, source, source.rowOfWord(rows, at), entryHigh, entryLow, inci);
                target->shapeNum = static_cast<std::uint16_t>(words[at]);
                withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                    const auto key = reporting.template instanceKey<Target>(
                        partId, entryHigh, inci, static_cast<musx::dom::Cmper>((static_cast<std::uint32_t>(entryHigh) << 16U) | entryLow));
                    reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
                    reportLegacyField(
                        reporting, key, source, source.rowOfWord(rows, at), "shapeNum", source.byteOffsetInRow(at * 2), target->shapeNum);
                });
                context.document->getDetails()->add(Target::XmlNodeName, std::move(target));
            }
        }
    }
}

} // namespace

void importSmartShapeEntryAssigns(const ImportContext& context)
{
    const auto source =
        selectRecordFamilySource(context, context.index.getDetails(), context.index.getClassDetails(), assignmentTag, assignmentClass);
    if (source) {
        importSource(context, *source);
    }
    if (context.profile.epoch == FormatEpoch::ZlibLegacy) {
        importSource(context, RecordFamilySource{&context.index.getClassDetails(), laterAssignmentClass, true, true});
    }
}

} // namespace finale_mus_reader::details
