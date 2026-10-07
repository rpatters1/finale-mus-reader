// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"
#include "import/shared/expression_common.h"
#include "import/support/enigma_text.h"
#include "import/support/legacy_font.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using TextExpressionTarget = musx::dom::others::TextExpressionDef;
constexpr auto textExpressionTag = records::packTag("DT");
constexpr records::LegacyTag textExpressionClass = 0x00f1;
constexpr std::size_t primeExpressionHeaderSize = 36;

void synthesizeExpressionText(const ImportContext& context, const std::shared_ptr<TextExpressionTarget>& target, const records::LegacyRow& row,
    std::span<const std::uint8_t> payload)
{
    using Block = musx::dom::others::TextBlock;
    using RawText = musx::dom::texts::ExpressionText;
    const auto textNumber = context.document->getTexts()->nextFreeCmper<RawText>();
    const auto blockNumber = context.document->getOthers()->nextFreeCmper<Block>(target->getSourcePartId());
    if (!textNumber || !blockNumber) {
        context.report.diagnostics.push_back(
            {musx::util::Logger::LogLevel::Warning, "Text expression synthesis exhausted the text identifier space."});
        return;
    }
    const auto packed = payloadWord(payload, 0, context.profile.byteOrder);
    musx::dom::FontInfo font(context.document);
    const bool coda = context.profile.epoch == FormatEpoch::CodaBanner;
    assignPackedFont(font, context.construction, packed, coda);
    font.setEnigmaStyles(payloadWord(payload, 2, context.profile.byteOrder));
    const auto flags = payloadWord(payload, 10, context.profile.byteOrder);
    const bool hideText = sourceAtOrAfter(context.profile, FormatEpoch::UncompressedLegacy, versions::finale97) && bool(flags & 0x0200U);
    // Inline expressions in F2002–2003 use '<' and '>' as hide/show controls.
    // F97–F2001 inline expressions apply the flag to the entire text.
    const bool partialHidden =
        hideText && context.profile.epoch == FormatEpoch::DclLegacy && sourceAtOrAfter(context.profile, FormatEpoch::DclLegacy, versions::finale2002);
    font.hidden = font.hidden || (hideText && !partialHidden);
    const auto baseEffects = font.getEnigmaStyles();
    auto hiddenFont = font;
    hiddenFont.hidden = true;
    const auto hiddenEffects = hiddenFont.getEnigmaStyles();
    auto plain = text::normalizeLineBreaks(
        text::toUtf8(payloadString(payload, 12, payload.size() - 12), context.document, font.fontId, text::UnresolvedFontFallback::Text));
    std::string enigma;
    for (const auto ch : plain) {
        if (partialHidden && (ch == '<' || ch == '>')) {
            enigma += "^nfx(" + std::to_string(ch == '<' ? hiddenEffects : baseEffects) + ")";
            continue;
        }
        if (ch == '^') {
            enigma += "^^";
        } else if (ch == '#' && (flags & 0xc000U) != 0xc000U) {
            switch (flags & 0xc000U) {
            case 0: enigma += "^value()"; break;
            case 0x4000: enigma += "^pass()"; break;
            case 0x8000: enigma += "^control()"; break;
            }
        } else {
            enigma += ch;
        }
    }
    auto raw = std::make_shared<RawText>(context.document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, *textNumber);
    raw->text = text::initializeEnigmaTextFontState(std::move(enigma), font);
    auto block = std::make_shared<Block>(context.document, target->getSourcePartId(), musx::dom::EnigmaBase::ShareMode::All, *blockNumber);
    block->textId = *textNumber;
    block->textType = Block::TextType::Expression;
    block->lineSpacingPercentage = 100;
    block->newPos36 = true;
    block->showShape = true;
    block->wordWrap = true;
    target->textIdKey = *blockNumber;
    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
        const auto rawKey = reporting.template instanceKey<RawText>(musx::dom::SCORE_PARTID, *textNumber);
        const auto blockKey = reporting.template instanceKey<Block>(target->getSourcePartId(), *blockNumber);
        reporting.report().setInstanceOrigin(rawKey, Reporting::Origin::LegacyMus);
        reporting.report().setInstanceOrigin(blockKey, Reporting::Origin::LegacyBehavior);
        reporting.report().setField(
            rawKey, "text", typename Reporting::FieldInfo{Reporting::Origin::LegacyMus, row.blockOffset, row.decodedOffset, 0, textExpressionTag});
        reporting.report().setField(reporting.template instanceKey<TextExpressionTarget>(target->getSourcePartId(), target->getCmper()), "textIdKey",
            typename Reporting::FieldInfo{Reporting::Origin::LegacyBehavior, row.blockOffset, row.decodedOffset, *blockNumber, textExpressionTag});
    });
    context.document->getTexts()->add(RawText::XmlNodeName, std::move(raw));
    context.document->getOthers()->add(Block::XmlNodeName, std::move(block));
}

constexpr const char* textExpressionFields[] = {"textIdKey", "categoryId", "rehearsalMarkStyle", "value", "execShape", "auxData1", "playPass",
    "hideMeasureNum", "matchPlayback", "useAuxData", "hasEnclosure", "breakMmRest", "createdByHp", "playbackType", "horzMeasExprAlign",
    "vertMeasExprAlign", "horzExprJustification", "measXAdjust", "yAdjustEntry", "yAdjustBaseline", "useCategoryFonts", "useCategoryPos",
    "description"};

void reportAbsentTextExpressionFields(
    ImportReport& report, const ReportInstance& reportInstance, bool prime, bool hasCategory, bool hasRehearsalStyle, bool hasBreakMmRest)
{
    withReporting(report, [&]<typename Reporting>(Reporting& reporting) {
        if (!prime) {
            for (const auto* member : {"description", "createdByHp", "horzMeasExprAlign", "vertMeasExprAlign"}) {
                reporting.report().setField(
                    reporting.instanceKey(reportInstance), member, typename Reporting::FieldInfo{Reporting::Origin::LegacyBehavior, 0, 0, 0});
            }
        }
        if (!hasCategory) {
            for (const auto* member : {"useCategoryFonts", "useCategoryPos"}) {
                reporting.report().setField(
                    reporting.instanceKey(reportInstance), member, typename Reporting::FieldInfo{Reporting::Origin::LegacyBehavior, 0, 0, 0});
            }
        }
        if (!hasRehearsalStyle) {
            for (const auto* member : {"rehearsalMarkStyle", "hideMeasureNum", "matchPlayback"}) {
                reporting.report().setField(
                    reporting.instanceKey(reportInstance), member, typename Reporting::FieldInfo{Reporting::Origin::LegacyBehavior, 0, 0, 0});
            }
        }
        if (!hasBreakMmRest) {
            reporting.report().setField(
                reporting.instanceKey(reportInstance), "breakMmRest", typename Reporting::FieldInfo{Reporting::Origin::LegacyBehavior, 0, 0, 0});
        }
    });
}

} // namespace

void importTextExpressionDefs(const ImportContext& context)
{
    const bool prime = sourceAtOrAfter(context.profile, FormatEpoch::DclLegacy, versions::finale2004);
    const auto source =
        selectRecordFamilySource(context, context.index.getOthers(), context.index.getClassOthers(), textExpressionTag, textExpressionClass);
    if (!source) {
        return;
    }
    std::vector<std::function<void()>> synthesizedTexts;
    for (const auto& [partId, cmper] : recordKeys(*source)) {
        const auto rows = source->pool->getArray(source->identity, cmper, 0, partId);
        bool completeRows = !rows.empty();
        for (std::size_t index = 0; index < rows.size() && !source->classRecords; ++index) {
            completeRows = completeRows && rows[index].inci == index;
        }
        const auto payload = collectRecordPayload(*source, rows);
        const auto words = payloadWords(payload, context.profile.byteOrder);
        if (!completeRows || payload.size() < (prime ? primeExpressionHeaderSize : 12)) {
            context.report.diagnostics.push_back(
                {musx::util::Logger::LogLevel::Warning, "Text expression " + std::to_string(cmper) + " has an incomplete header."});
            continue;
        }
        auto target = createOthersRecordTarget<TextExpressionTarget>(context.document, *source, rows.front(), cmper);
        if (!target) {
            continue;
        }
        const auto reportInstance = ReportInstance::of<TextExpressionTarget>(partId, cmper);
        withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
            reporting.report().setInstanceOrigin(reporting.instanceKey(reportInstance), Reporting::Origin::LegacyMus);
            for (const auto* field : textExpressionFields) {
                reporting.report().setField(
                    reporting.instanceKey(reportInstance), field, typename Reporting::FieldInfo{Reporting::Origin::Unmapped, 0, 0, 0});
            }
        });
        const auto assign = [&](auto member, const char* name, auto value, std::size_t slot, bool adjusted = false) {
            target.get()->*member = value;
            withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                const auto& row = source->rowOfWord(rows, slot);
                const auto offset = source->byteOffsetInRow(slot * 2);
                reporting.report().setField(reporting.instanceKey(reportInstance), name,
                    typename Reporting::FieldInfo{adjusted ? Reporting::Origin::LegacyMusAdjusted : Reporting::Origin::LegacyMus, row.blockOffset,
                        row.decodedOffset + offset, words[slot], source->identity});
            });
        };
        const auto flags = static_cast<std::uint16_t>(words[5]);
        const bool hasCategory = sourceAtOrAfter(context.profile, FormatEpoch::ZlibLegacy, versions::finale2009);
        // Believed: automatic rehearsal styles and the hide-number/match-playback
        // controls share the Finale 2010 introduction boundary.
        const bool hasRehearsalStyle = sourceAtOrAfter(context.profile, FormatEpoch::ZlibLegacy, versions::finale2010);
        const bool hasBreakMmRest = sourceAtOrAfter(context.profile, FormatEpoch::DclLegacy, versions::finale2002);
        if (!prime) {
            target->horzMeasExprAlign = musx::dom::others::HorizontalMeasExprAlign::Manual;
            target->vertMeasExprAlign = musx::dom::others::VerticalMeasExprAlign::Manual;
        }
        reportAbsentTextExpressionFields(context.report, reportInstance, prime, hasCategory, hasRehearsalStyle, hasBreakMmRest);
        if (hasRehearsalStyle) {
            assign(&TextExpressionTarget::hideMeasureNum, "hideMeasureNum", bool(flags & 0x8000U), 5);
            assign(&TextExpressionTarget::matchPlayback, "matchPlayback", bool(flags & 0x4000U), 5);
            if (auto style = expressionRehearsalStyle(words[1])) {
                assign(&TextExpressionTarget::rehearsalMarkStyle, "rehearsalMarkStyle", *style, 1);
            }
        }
        if (prime) {
            assign(&TextExpressionTarget::textIdKey, "textIdKey", static_cast<musx::dom::Cmper>(words[0]), 0);
        }
        assignExpressionPlayback<TextExpressionTarget>(assign, words);
        assign(&TextExpressionTarget::hasEnclosure, "hasEnclosure", bool(flags & 0x0800U), 5);
        if (hasBreakMmRest) {
            assign(&TextExpressionTarget::breakMmRest, "breakMmRest", bool(flags & 0x0400U), 5);
        }
        if (prime) {
            // Unverified: pre-category definitions use their separate note anchors as modern positioning selectors.
            assignExpressionPositioning<TextExpressionTarget>(*target, assign, words, hasCategory);
            recoverExpressionDescription(context, *source, rows, payload, target, reportInstance, primeExpressionHeaderSize);
        } else {
            synthesizedTexts.push_back([&context, target, row = rows.front(), payload] { synthesizeExpressionText(context, target, row, payload); });
        }
        if (!hasCategory) {
            scheduleExpressionMiscCategory(context, target);
        }
        context.document->getOthers()->add(TextExpressionTarget::XmlNodeName, std::move(target));
    }
    context.pending.defer(DeferredStage::Synthesize, std::move(synthesizedTexts));
}

} // namespace others
} // namespace finale_mus_reader
