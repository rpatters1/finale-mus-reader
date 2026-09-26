// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using ClefListTarget = musx::dom::others::ClefList;

constexpr records::LegacyTag clefListTag = records::packTag("CE");
constexpr records::LegacyTag clefListClass = 0x007f;

// One clef list item is six words: clef index, Edu position, vertical adjustment, percent,
// horizontal adjustment, and flags. Each item is one incidence, so a fixed-row list stores one
// item per row and a zlib payload concatenates them.
constexpr std::size_t clefItemWords = 6;
constexpr std::size_t clefFlagsWord = 5;

constexpr std::uint16_t clefHiddenBit = 0x0002;
constexpr std::uint16_t clefUnlockVertBit = 0x0004;
constexpr std::uint16_t clefForcedBit = 0x0008;
constexpr std::uint16_t clefAfterBarlineBit = 0x0010;

/// Translates the two clef-display bits. **Unverified:** no list sets both, and hidden is
/// preferred when both are set.
musx::dom::ShowClefMode clefMode(std::uint16_t flags)
{
    if ((flags & clefHiddenBit) != 0) {
        return musx::dom::ShowClefMode::Never;
    }
    if ((flags & clefForcedBit) != 0) {
        return musx::dom::ShowClefMode::Always;
    }
    return musx::dom::ShowClefMode::WhenNeeded;
}

void importClefListFamily(const ImportContext& context, const RecordFamilySource& source, std::uint16_t partId, musx::dom::Cmper cmper)
{
    const auto rows = source.pool->getArray(source.identity, cmper, 0, partId);
    if (rows.empty()) {
        return;
    }
    const auto words = collectRecordWords(source, rows, context.profile.byteOrder);
    if (words.size() % clefItemWords != 0) {
        context.report.diagnostics.push_back(
            {musx::util::Logger::LogLevel::Info, "Clef list " + std::to_string(cmper) + " has an incomplete trailing item."});
    }
    for (std::size_t at = 0; at + clefItemWords <= words.size(); at += clefItemWords) {
        const auto inci = static_cast<musx::dom::Inci>(at / clefItemWords);
        auto target = createOthersRecordTarget<ClefListTarget>(context.document, source, rows.front(), cmper, inci);
        if (!target) {
            continue;
        }
        const std::span<const std::int16_t> item(words.data() + at, clefItemWords);
        const auto flags = static_cast<std::uint16_t>(item[clefFlagsWord]);
        target->clefIndex = static_cast<musx::dom::ClefIndex>(item[0]);
        target->xEduPos = item[1];
        target->yEvpuPos = item[2];
        target->percent = item[3];
        target->xEvpuOffset = item[4];
        target->clefMode = clefMode(flags);
        target->unlockVert = (flags & clefUnlockVertBit) != 0;
        target->afterBarline = (flags & clefAfterBarlineBit) != 0;

        withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
            const auto key = reporting.template instanceKey<ClefListTarget>(partId, cmper, inci);
            const auto reportField = [&](const char* member, std::size_t word, std::int64_t value) {
                const auto& row = source.rowOfWord(rows, at + word);
                reporting.report().setField(key, member,
                    {Reporting::Origin::LegacyMus, row.blockOffset, row.decodedOffset + source.byteOffsetInRow((at + word) * sizeof(std::uint16_t)),
                        value, source.identity});
            };
            reportField("clefIndex", 0, item[0]);
            reportField("xEduPos", 1, item[1]);
            reportField("yEvpuPos", 2, item[2]);
            reportField("percent", 3, item[3]);
            reportField("xEvpuOffset", 4, item[4]);
            reportField("clefMode", clefFlagsWord, flags);
            reportField("unlockVert", clefFlagsWord, flags);
            reportField("afterBarline", clefFlagsWord, flags);
            reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
        });
        context.document->getOthers()->add(ClefListTarget::XmlNodeName, std::move(target));
    }
}

} // namespace

void importClefLists(const ImportContext& context)
{
    const auto source = selectRecordFamilySource(context, context.index.getOthers(), context.index.getClassOthers(), clefListTag, clefListClass);
    if (!source) {
        return;
    }
    // The Coda-banner list stores only mid-measure clefs, and positions each by EVPU rather than
    // Edu. Converting that position requires the measure's layout.
    const auto keys = recordKeys(*source);
    if (context.profile.epoch == FormatEpoch::CodaBanner) {
        if (!keys.empty()) {
            context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                "Mid-measure clefs in files before v3.0 are not imported. Only the clef at measure start is recovered."});
        }
        return;
    }
    for (const auto& [partId, cmper] : keys) {
        importClefListFamily(context, *source, partId, cmper);
    }
}

} // namespace others
} // namespace finale_mus_reader
