// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "import/support/legacy_mapping.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using ChordSuffixPlaybackTarget = musx::dom::others::ChordSuffixPlayback;

constexpr auto chordSuffixPlaybackTag = records::packTag("IK");
constexpr records::LegacyTag chordSuffixPlaybackClass = 0x007e;

} // namespace

void importChordSuffixPlayback(const ImportContext& context)
{
    const auto source = selectRecordFamilySource(context, context.index.getOthers(),
        context.index.getClassOthers(), chordSuffixPlaybackTag, chordSuffixPlaybackClass);
    if (!source) return;
    for (const auto [partId, cmper] : recordKeys(*source)) {
        const auto rows = source->pool->getArray(source->identity, cmper, 0, partId);
        if (rows.empty()) continue;
        auto values = collectRecordWords(*source, rows, context.profile.byteOrder);

        auto target = createOthersRecordTarget<ChordSuffixPlaybackTarget>(
            context.document, *source, rows.front(), cmper);
        if (!target) continue;
        target->values = std::move(values);
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        const auto key = instanceKey<ChordSuffixPlaybackTarget>(partId, cmper);
        context.report.setInstanceOrigin(key, ValueOrigin::LegacyMus);
        for (std::size_t index = 0; index < target->values.size(); ++index) {
            const auto rowIndex = source->classRecords
                ? std::size_t{0}
                : index / records::otherWordCount;
            const auto wordIndex = source->classRecords
                ? index
                : index % records::otherWordCount;
            const auto& row = rows[rowIndex];
            FINALE_MUS_READER_REPORT_FIELD(context.report, key,
                "values[" + std::to_string(index) + "]",
                {ValueOrigin::LegacyMus, row.blockOffset,
                    row.decodedOffset + wordIndex * 2, target->values[index],
                    source->identity});
        }
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        context.document->getOthers()->add(
            ChordSuffixPlaybackTarget::XmlNodeName, std::move(target));
    }
}

} // namespace others
} // namespace finale_mus_reader
