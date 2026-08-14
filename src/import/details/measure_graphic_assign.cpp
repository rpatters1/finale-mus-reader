// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/details.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "import/support/graphic_assignment.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace details {
namespace {

using MeasureGraphicTarget = musx::dom::details::MeasureGraphicAssign;
constexpr auto measureGraphicAssignTag = records::packTag("mg");
constexpr records::LegacyTag measureGraphicAssignClass = 0x041d;
constexpr std::size_t measureGraphicAssignWordCount = 20;

void reportMeasureGraphicValue(const ImportContext& context, const std::string& prefix,
    std::string name, std::int64_t value, const records::LegacyRow& row)
{
    context.report.fields.push_back({prefix + std::move(name), ValueOrigin::LegacyMus,
        row.blockOffset, row.decodedOffset, value});
}

void importMeasureGraphicFamily(const ImportContext& context,
    const records::LegacyRowPool& pool, records::LegacyTag identity, bool classRecords)
{
    for (const auto staffId : pool.cmpersForTag(identity)) {
        for (const auto meas : pool.secondCmpersForTag(identity, staffId)) {
            const auto rows = pool.getArray(identity, staffId, meas);
            std::vector<std::int16_t> words;
            for (const auto& row : rows) {
                if (classRecords) {
                    const auto payload = payloadWords(pool.payloadOf(row), context.profile.byteOrder);
                    words.insert(words.end(), payload.begin(), payload.end());
                } else {
                    words.insert(words.end(), row.words.begin(), row.words.begin() + row.wordCount);
                }
            }
            if (words.size() % measureGraphicAssignWordCount != 0) {
                context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                    "Measure graphic assignment for staff " + std::to_string(staffId)
                        + ", measure " + std::to_string(meas)
                        + " has an incomplete trailing tuple."});
            }
            for (std::size_t at = 0; at + measureGraphicAssignWordCount <= words.size();
                    at += measureGraphicAssignWordCount) {
                const auto rowIndex = classRecords ? at / measureGraphicAssignWordCount : 0;
                const auto inci = classRecords ? rows[rowIndex].inci
                    : static_cast<musx::dom::Inci>(at / measureGraphicAssignWordCount);
                auto target = std::make_shared<MeasureGraphicTarget>(context.document,
                    musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All,
                    staffId, meas, inci);
                const std::span<const std::int16_t> tuple(
                    words.data() + at, measureGraphicAssignWordCount);
                populateGraphicAssignmentCommon(*target, tuple);
                const auto prefix = "details.measureGraphicAssign[" + std::to_string(staffId)
                    + "][" + std::to_string(meas) + "][" + std::to_string(inci) + "].";
                constexpr std::size_t slots[] = {0, 1, 2, 3, 4, 5, 6, 11, 12, 13, 17};
                constexpr const char* names[] = {"version", "left", "bottom", "width",
                    "height", "fDescId", "hidden", "savedRecord", "origWidth",
                    "origHeight", "graphicCmper"};
                for (std::size_t index = 0; index < std::size(slots); ++index) {
                    const auto slot = slots[index];
                    const auto& sourceRow = rows[classRecords ? rowIndex
                        : (at + slot) / records::detailWordCount];
                    reportMeasureGraphicValue(
                        context, prefix, names[index], tuple[slot], sourceRow);
                }
                context.document->getDetails()->add(MeasureGraphicTarget::XmlNodeName, target);
            }
        }
    }
}

} // namespace

void importMeasureGraphicAssignments(const ImportContext& context)
{
    switch (context.profile.epoch) {
    case FormatEpoch::CodaBanner:
        // No assignment was found in either surveyed Coda-banner corpus. The normalized
        // fixed detail pool remains accepted so a future specimen can use the same mg layout.
        [[fallthrough]];
    case FormatEpoch::UncompressedLegacy:
    case FormatEpoch::DclLegacy:
        importMeasureGraphicFamily(
            context, context.index.getDetails(), measureGraphicAssignTag, false);
        break;
    case FormatEpoch::ZlibLegacy:
        importMeasureGraphicFamily(
            context, context.index.getClassDetails(), measureGraphicAssignClass, true);
        break;
    case FormatEpoch::Unknown:
        // An unknown container has no trustworthy detail-pool framing.
        break;
    }
}

} // namespace details
} // namespace finale_mus_reader
