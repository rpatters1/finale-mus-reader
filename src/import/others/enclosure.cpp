// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using Enclosure = musx::dom::others::TextRepeatEnclosure;
constexpr auto enclosureTag = records::packTag("Rx");
constexpr records::LegacyTag enclosureClass = 0x00f5;

} // namespace

void importTextRepeatEnclosures(const ImportContext& context)
{
    if (context.profile.epoch == FormatEpoch::CodaBanner) {
        return;
    }
    const auto source = selectRecordFamilySource(context, context.index.getOthers(), context.index.getClassOthers(), enclosureTag, enclosureClass);
    if (!source) {
        return;
    }
    for (const auto& [partId, cmper] : recordKeys(*source)) {
        const auto rows = source->pool->getArray(source->identity, cmper, 0, partId);
        if (rows.empty()) {
            continue;
        }
        const auto payload = collectRecordPayload(*source, rows);
        if (payload.size() < 12) {
            context.report.diagnostics.push_back(
                {musx::util::Logger::LogLevel::Info, "Text-repeat enclosure " + std::to_string(cmper) + " is shorter than its layout."});
            continue;
        }
        const auto word = [&](std::size_t slot) { return payloadWord(payload, slot * 2, context.profile.byteOrder); };
        const auto flags = word(5);
        const auto shape = flags & 0x000fU;
        if (shape > static_cast<std::uint16_t>(Enclosure::Shape::Octogon)) {
            context.report.diagnostics.push_back(
                {musx::util::Logger::LogLevel::Info, "Text-repeat enclosure " + std::to_string(cmper) + " has an unsupported shape."});
        }
        auto target = createOthersRecordTarget<Enclosure>(context.document, *source, rows.front(), cmper);
        target->xAdd = static_cast<std::int16_t>(word(0));
        target->yAdd = static_cast<std::int16_t>(word(1));
        target->xMargin = static_cast<std::int16_t>(word(2));
        target->yMargin = static_cast<std::int16_t>(word(3));
        target->lineWidth = (flags & 0x2000U) != 0 ? 0 : static_cast<std::int16_t>(word(4));
        if (shape <= static_cast<std::uint16_t>(Enclosure::Shape::Octogon)) {
            target->shape = static_cast<Enclosure::Shape>(shape);
        }
        target->fixedSize = (flags & 0x0800U) != 0;
        target->notTall = (flags & 0x1000U) != 0;
        target->equalAspect = (flags & 0x4000U) != 0;
        target->opaque = (flags & 0x8000U) != 0;
        target->roundCorners = false;
        target->cornerRadius = 0;
        withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
            const auto key = reporting.template instanceKey<Enclosure>(partId, cmper);
            reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
            const auto field = [&](const char* name, std::size_t slot, std::int64_t value) {
                reportLegacyField(reporting, key, *source, source->rowOfWord(rows, slot), name, source->byteOffsetInRow(slot * 2), value);
            };
            field("xAdd", 0, target->xAdd);
            field("yAdd", 1, target->yAdd);
            field("xMargin", 2, target->xMargin);
            field("yMargin", 3, target->yMargin);
            if ((flags & 0x2000U) == 0) {
                field("lineWidth", 4, target->lineWidth);
            } else {
                reportLegacyField(reporting, key, *source, source->rowOfWord(rows, 4), "lineWidth", source->byteOffsetInRow(8), word(4),
                    Reporting::Origin::Unmapped);
            }
            if (shape <= static_cast<std::uint16_t>(Enclosure::Shape::Octogon)) {
                field("shape", 5, shape);
            } else {
                reportLegacyField(
                    reporting, key, *source, source->rowOfWord(rows, 5), "shape", source->byteOffsetInRow(10), shape, Reporting::Origin::Unmapped);
            }
            field("fixedSize", 5, target->fixedSize);
            field("notTall", 5, target->notTall);
            field("equalAspect", 5, target->equalAspect);
            field("opaque", 5, target->opaque);
            reportFallbackField(reporting, key, "roundCorners", Reporting::Origin::MusxOnly, 0);
            reportFallbackField(reporting, key, "cornerRadius", Reporting::Origin::MusxOnly, 0);
        });
        context.document->getOthers()->add(Enclosure::XmlNodeName, std::move(target));
    }
}

} // namespace others
} // namespace finale_mus_reader
