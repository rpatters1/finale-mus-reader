// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <memory>
#include <string>

#include "import/support/fret_records.h"
#include "import/support/text_encoding.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using FretboardStyleTarget = musx::dom::others::FretboardStyle;
constexpr records::LegacyTag fretboardStyleClass = 0x0097;
constexpr records::LegacyTag fretboardStyleTag = records::packTag("ft");
constexpr std::size_t fretboardStyleRecordSize = 156;
constexpr std::size_t fretboardStyleNameOffset = 84;
constexpr std::size_t fretboardStyleNameSize = 48;
constexpr std::size_t fretboardStyleNumberTextOffset = 132;
constexpr std::size_t fretboardStyleNumberTextSize = 24;

void populateFretboardStyleFont(const ImportContext& context,
    const std::span<const std::uint8_t> payload, std::size_t at,
    std::shared_ptr<musx::dom::FontInfo>& target)
{
    target = std::make_shared<musx::dom::FontInfo>(context.document);
    target->fontId = fret_records::readWord(payload, at, context.profile.byteOrder);
    target->fontSize = static_cast<std::int16_t>(
        fret_records::readWord(payload, at + 2, context.profile.byteOrder));
    target->setEnigmaStyles(fret_records::readWord(
        payload, at + 4, context.profile.byteOrder));
}

} // namespace

void importFretboardStyles(const ImportContext& context)
{
    const auto source = fret_records::selectSource(context, context.index.getOthers(),
        context.index.getClassOthers(), fretboardStyleTag, fretboardStyleClass);
    if (!source) return;
    for (const auto cmper : source->pool->cmpersForTag(source->identity)) {
        const auto rows = source->pool->getArray(source->identity, cmper);
        if (rows.empty()) continue;
        const auto payload = fret_records::collectPayload(*source, rows);
        if (payload.size() < fretboardStyleRecordSize) continue;
        auto target = std::make_shared<FretboardStyleTarget>(context.document,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
        target->showLastFret = fret_records::readWord(payload, 0, context.profile.byteOrder);
        target->rotate = fret_records::readWord(payload, 2, context.profile.byteOrder);
        target->fingNumWhite = fret_records::readWord(payload, 4, context.profile.byteOrder);
        target->fingStrShapeId = fret_records::readWord(payload, 6, context.profile.byteOrder);
        target->openStrShapeId = fret_records::readWord(payload, 8, context.profile.byteOrder);
        target->muteStrShapeId = fret_records::readWord(payload, 10, context.profile.byteOrder);
        target->barreShapeId = fret_records::readWord(payload, 12, context.profile.byteOrder);
        target->customShapeId = fret_records::readWord(payload, 14, context.profile.byteOrder);
        target->defNumFrets = fret_records::readWord(payload, 16, context.profile.byteOrder);
        target->stringGap = fret_records::readHighFirstLong(payload, 18, context.profile.byteOrder);
        target->fretGap = fret_records::readHighFirstLong(payload, 22, context.profile.byteOrder);
        target->stringWidth = fret_records::readHighFirstLong(payload, 26, context.profile.byteOrder);
        target->fretWidth = fret_records::readHighFirstLong(payload, 30, context.profile.byteOrder);
        target->nutWidth = fret_records::readHighFirstLong(payload, 34, context.profile.byteOrder);
        target->vertTextOff = fret_records::readHighFirstLong(payload, 38, context.profile.byteOrder);
        target->horzTextOff = fret_records::readHighFirstLong(payload, 42, context.profile.byteOrder);
        target->horzHandleOff = fret_records::readHighFirstLong(payload, 46, context.profile.byteOrder);
        target->vertHandleOff = fret_records::readHighFirstLong(payload, 50, context.profile.byteOrder);
        target->whiteout = fret_records::readHighFirstLong(payload, 54, context.profile.byteOrder);
        populateFretboardStyleFont(context, payload, 58, target->fretNumFont);
        populateFretboardStyleFont(context, payload, 64, target->fingNumFont);
        target->horzFingNumOff = fret_records::readHighFirstLong(payload, 70, context.profile.byteOrder);
        target->vertFingNumOff = fret_records::readHighFirstLong(payload, 74, context.profile.byteOrder);
        target->name = text::toUtf8(fret_records::readString(payload,
            fretboardStyleNameOffset, fretboardStyleNameSize), context.profile.platform);
        target->fretNumText = text::toUtf8(fret_records::readString(payload,
            fretboardStyleNumberTextOffset, fretboardStyleNumberTextSize),
            context.profile.platform);
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        const auto key = instanceKey<FretboardStyleTarget>(musx::dom::SCORE_PARTID, cmper);
        context.report.setInstanceOrigin(key, ValueOrigin::LegacyMus);
        const auto report = [&](std::string member, auto value) {
            FINALE_MUS_READER_REPORT_FIELD(context.report, key, std::move(member),
                {ValueOrigin::LegacyMus, rows.front().blockOffset,
                    rows.front().decodedOffset, value});
        };
        report("showLastFret", target->showLastFret);
        report("rotate", target->rotate);
        report("fingNumWhite", target->fingNumWhite);
        report("fingStrShapeId", target->fingStrShapeId);
        report("openStrShapeId", target->openStrShapeId);
        report("muteStrShapeId", target->muteStrShapeId);
        report("barreShapeId", target->barreShapeId);
        report("customShapeId", target->customShapeId);
        report("defNumFrets", target->defNumFrets);
        report("stringGap", target->stringGap);
        report("fretGap", target->fretGap);
        report("stringWidth", target->stringWidth);
        report("fretWidth", target->fretWidth);
        report("nutWidth", target->nutWidth);
        report("vertTextOff", target->vertTextOff);
        report("horzTextOff", target->horzTextOff);
        report("horzHandleOff", target->horzHandleOff);
        report("vertHandleOff", target->vertHandleOff);
        report("whiteout", target->whiteout);
        report("fretNumFont.fontId", target->fretNumFont->fontId);
        report("fretNumFont.fontSize", target->fretNumFont->fontSize);
        report("fretNumFont.efx", target->fretNumFont->getEnigmaStyles());
        report("fingNumFont.fontId", target->fingNumFont->fontId);
        report("fingNumFont.fontSize", target->fingNumFont->fontSize);
        report("fingNumFont.efx", target->fingNumFont->getEnigmaStyles());
        report("horzFingNumOff", target->horzFingNumOff);
        report("vertFingNumOff", target->vertFingNumOff);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        context.document->getOthers()->add(FretboardStyleTarget::XmlNodeName,
            std::move(target));
    }
}

} // namespace others
} // namespace finale_mus_reader
