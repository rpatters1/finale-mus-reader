// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"
#include "import/support/enigma_text.h"
#include "import/support/expression_alignment.h"
#include "import/support/legacy_font.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using TextExpressionTarget = musx::dom::others::TextExpressionDef;
constexpr auto textExpressionTag = records::packTag("DT");
constexpr records::LegacyTag textExpressionClass = 0x00f1;
constexpr std::size_t primeExpressionHeaderSize = 36;

enum class LegacyNoteHorizontal : std::int16_t
{
    Left = 0,
    HorizontalClickPosition = 1,
    Stem = 2,
    CenterOfPrimaryNotehead = 3,
    CenterOfAllNoteheads = 4,
    Right = 5,
    LeftOfPrimaryNotehead = 6
};

enum class LegacyNoteVertical : std::int16_t
{
    VerticalClickPosition = 0,
    AboveStaffBaseline = 1,
    BelowStaffBaseline = 2,
    TopNote = 3,
    BottomNote = 4,
    AboveEntry = 5,
    BelowEntry = 6,
    AboveStaffBaselineOrEntry = 7,
    BelowStaffBaselineOrEntry = 8
};

std::optional<musx::dom::others::HorizontalMeasExprAlign> expressionNoteHorizontalAlignment(
    std::int16_t stored)
{
    using A = musx::dom::others::HorizontalMeasExprAlign;
    switch (static_cast<LegacyNoteHorizontal>(stored)) {
    case LegacyNoteHorizontal::Left:
        return A::LeftOfAllNoteheads;
    case LegacyNoteHorizontal::HorizontalClickPosition:
        return A::Manual;
    case LegacyNoteHorizontal::Stem:
        return A::Stem;
    case LegacyNoteHorizontal::CenterOfPrimaryNotehead:
        return A::CenterPrimaryNotehead;
    case LegacyNoteHorizontal::CenterOfAllNoteheads:
        return A::CenterAllNoteheads;
    case LegacyNoteHorizontal::Right:
        return A::RightOfAllNoteheads;
    case LegacyNoteHorizontal::LeftOfPrimaryNotehead:
        return A::LeftOfPrimaryNotehead;
    }
    return std::nullopt;
}

std::optional<musx::dom::others::VerticalMeasExprAlign> expressionNoteVerticalAlignment(
    std::int16_t stored)
{
    using A = musx::dom::others::VerticalMeasExprAlign;
    switch (static_cast<LegacyNoteVertical>(stored)) {
    case LegacyNoteVertical::VerticalClickPosition:
        return A::Manual;
    case LegacyNoteVertical::AboveStaffBaseline:
        return A::AboveStaff;
    case LegacyNoteVertical::BelowStaffBaseline:
        return A::BelowStaff;
    case LegacyNoteVertical::TopNote:
        return A::TopNote;
    case LegacyNoteVertical::BottomNote:
        return A::BottomNote;
    case LegacyNoteVertical::AboveEntry:
        return A::AboveEntry;
    case LegacyNoteVertical::BelowEntry:
        return A::BelowEntry;
    case LegacyNoteVertical::AboveStaffBaselineOrEntry:
        return A::AboveStaffOrEntry;
    case LegacyNoteVertical::BelowStaffBaselineOrEntry:
        return A::BelowStaffOrEntry;
    }
    return std::nullopt;
}

void synthesizeExpressionText(const ImportContext& context,
                              const std::shared_ptr<TextExpressionTarget>& target,
                              const records::LegacyRow& row, std::span<const std::uint8_t> payload)
{
    using Block = musx::dom::others::TextBlock;
    using RawText = musx::dom::texts::ExpressionText;
    const auto textNumber = context.document->getTexts()->nextFreeCmper<RawText>();
    const auto blockNumber =
        context.document->getOthers()->nextFreeCmper<Block>(target->getSourcePartId());
    if (!textNumber || !blockNumber) {
        context.report.diagnostics.push_back(
            {musx::util::Logger::LogLevel::Warning,
             "Text expression synthesis exhausted the text identifier space."});
        return;
    }
    const auto packed = payloadWord(payload, 0, context.profile.byteOrder);
    musx::dom::FontInfo font(context.document);
    const bool coda = context.profile.epoch == FormatEpoch::CodaBanner;
    assignPackedFont(font, context.construction, packed, coda);
    font.setEnigmaStyles(payloadWord(payload, 2, context.profile.byteOrder));
    const auto flags = payloadWord(payload, 10, context.profile.byteOrder);
    const bool hideText = bool(flags & 0x0200U);
    // Inline expressions in F2002–2003 use '<' and '>' as hide/show controls.
    // Earlier inline expressions apply the flag to the entire text.
    const bool partialHidden =
        hideText && context.profile.epoch == FormatEpoch::DclLegacy &&
        sourceAtOrAfter(context.profile, FormatEpoch::DclLegacy, versions::finale2002);
    font.hidden = font.hidden || (hideText && !partialHidden);
    const auto baseEffects = font.getEnigmaStyles();
    auto hiddenFont = font;
    hiddenFont.hidden = true;
    const auto hiddenEffects = hiddenFont.getEnigmaStyles();
    auto plain = text::normalizeLineBreaks(
        text::toUtf8(payloadString(payload, 12, payload.size() - 12), context.document, font.fontId,
                     text::UnresolvedFontFallback::Text));
    std::string enigma;
    for (const auto ch : plain) {
        if (partialHidden && (ch == '<' || ch == '>')) {
            enigma += "^nfx(" + std::to_string(ch == '<' ? hiddenEffects : baseEffects) + ")";
            continue;
        }
        if (ch == '^') enigma += "^^";
        else if (ch == '#' && (flags & 0xc000U) != 0xc000U) {
            switch (flags & 0xc000U) {
            case 0:
                enigma += "^value()";
                break;
            case 0x4000:
                enigma += "^pass()";
                break;
            case 0x8000:
                enigma += "^control()";
                break;
            }
        } else enigma += ch;
    }
    auto raw = std::make_shared<RawText>(context.document, musx::dom::SCORE_PARTID,
                                         musx::dom::EnigmaBase::ShareMode::All, *textNumber);
    raw->text = text::initializeEnigmaTextFontState(std::move(enigma), font);
    auto block = std::make_shared<Block>(context.document, target->getSourcePartId(),
                                         musx::dom::EnigmaBase::ShareMode::All, *blockNumber);
    block->textId = *textNumber;
    block->textType = Block::TextType::Expression;
    block->lineSpacingPercentage = 100;
    block->newPos36 = true;
    block->showShape = true;
    block->wordWrap = true;
    target->textIdKey = *blockNumber;
    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
        const auto rawKey =
            reporting.template instanceKey<RawText>(musx::dom::SCORE_PARTID, *textNumber);
        const auto blockKey =
            reporting.template instanceKey<Block>(target->getSourcePartId(), *blockNumber);
        reporting.report().setInstanceOrigin(rawKey, Reporting::Origin::LegacyMus);
        reporting.report().setInstanceOrigin(blockKey, Reporting::Origin::LegacyBehavior);
        reporting.report().setField(rawKey, "text",
            typename Reporting::FieldInfo{Reporting::Origin::LegacyMus, row.blockOffset,
                row.decodedOffset, 0, textExpressionTag});
        reporting.report().setField(reporting.template instanceKey<TextExpressionTarget>(
                                        target->getSourcePartId(), target->getCmper()),
            "textIdKey",
            typename Reporting::FieldInfo{Reporting::Origin::LegacyBehavior, row.blockOffset,
                row.decodedOffset, *blockNumber, textExpressionTag});
    });
    context.document->getTexts()->add(RawText::XmlNodeName, std::move(raw));
    context.document->getOthers()->add(Block::XmlNodeName, std::move(block));
}

std::optional<musx::dom::others::PlaybackType> expressionPlayback(std::uint16_t stored)
{
    using P = musx::dom::others::PlaybackType;
    switch (stored) {
    case 0:
        return P::None;
    case 1:
        return P::Tempo;
    case 2:
        return P::KeyVelocity;
    case 3:
        return P::Transpose;
    case 4:
        return P::Dump;
    case 5:
        return P::Channel;
    case 6:
        return P::RestrikeKeys;
    case 7:
        return P::PlayTempoToolChanges;
    case 8:
        return P::IgnoreTempoToolChanges;
    case 0xb0:
        return P::MidiController;
    case 0xc0:
        return P::MidiPatchChange;
    case 0xd0:
        return P::ChannelPressure;
    case 0xe0:
        return P::MidiPitchWheel;
    default:
        return std::nullopt;
    }
}

std::optional<musx::dom::others::RehearsalMarkStyle> expressionRehearsalStyle(std::int16_t stored)
{
    using Style = musx::dom::others::RehearsalMarkStyle;
    switch (stored) {
    case 0:
        return Style::None;
    case 1:
        return Style::Letters;
    case 2:
        return Style::LetterNumbers;
    case 3:
        return Style::LettersLowerCase;
    case 4:
        return Style::LettersNumbersLowerCase;
    case 5:
        return Style::Numbers;
    case 6:
        return Style::MeasureNumber;
    default:
        return std::nullopt;
    }
}

constexpr const char* textExpressionFields[] = {"textIdKey",
                                                "categoryId",
                                                "rehearsalMarkStyle",
                                                "value",
                                                "execShape",
                                                "auxData1",
                                                "playPass",
                                                "hideMeasureNum",
                                                "matchPlayback",
                                                "useAuxData",
                                                "hasEnclosure",
                                                "breakMmRest",
                                                "createdByHp",
                                                "playbackType",
                                                "horzMeasExprAlign",
                                                "vertMeasExprAlign",
                                                "horzExprJustification",
                                                "measXAdjust",
                                                "yAdjustEntry",
                                                "yAdjustBaseline",
                                                "useCategoryFonts",
                                                "useCategoryPos",
                                                "description"};

void reportAbsentTextExpressionFields(ImportReport& report, const ReportInstance& reportInstance,
    bool prime, bool hasCategory, bool hasRehearsalStyle)
{
    withReporting(report, [&]<typename Reporting>(Reporting& reporting) {
        if (!prime) {
            for (const auto* member :
                {"description", "createdByHp", "horzMeasExprAlign", "vertMeasExprAlign"}) {
                reporting.report().setField(reporting.instanceKey(reportInstance), member,
                    typename Reporting::FieldInfo{Reporting::Origin::LegacyBehavior, 0, 0, 0});
            }
        }
        if (!hasCategory) {
            for (const auto* member : {"useCategoryFonts", "useCategoryPos"}) {
                reporting.report().setField(reporting.instanceKey(reportInstance), member,
                    typename Reporting::FieldInfo{Reporting::Origin::LegacyBehavior, 0, 0, 0});
            }
        }
        if (!hasRehearsalStyle) {
            for (const auto* member : {"rehearsalMarkStyle", "hideMeasureNum", "matchPlayback"}) {
                reporting.report().setField(reporting.instanceKey(reportInstance), member,
                    typename Reporting::FieldInfo{Reporting::Origin::LegacyBehavior, 0, 0, 0});
            }
        }
    });
}

} // namespace

void importTextExpressionDefs(const ImportContext& context)
{
    const bool prime =
        sourceAtOrAfter(context.profile, FormatEpoch::DclLegacy, versions::finale2004);
    const auto source =
        selectRecordFamilySource(context, context.index.getOthers(), context.index.getClassOthers(),
                                 textExpressionTag, textExpressionClass);
    if (!source) return;
    for (const auto [partId, cmper] : recordKeys(*source)) {
        const auto rows = source->pool->getArray(source->identity, cmper, 0, partId);
        bool completeRows = !rows.empty();
        for (std::size_t index = 0; index < rows.size() && !source->classRecords; ++index) {
            completeRows = completeRows && rows[index].inci == index;
        }
        const auto payload = collectRecordPayload(*source, rows);
        const auto words = payloadWords(payload, context.profile.byteOrder);
        if (!completeRows || payload.size() < (prime ? primeExpressionHeaderSize : 12)) {
            context.report.diagnostics.push_back(
                {musx::util::Logger::LogLevel::Warning,
                 "Text expression " + std::to_string(cmper) + " has an incomplete header."});
            continue;
        }
        auto target = createOthersRecordTarget<TextExpressionTarget>(context.document, *source,
                                                                     rows.front(), cmper);
        if (!target) continue;
        const auto reportInstance = ReportInstance::of<TextExpressionTarget>(partId, cmper);
        withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
            reporting.report().setInstanceOrigin(
                reporting.instanceKey(reportInstance), Reporting::Origin::LegacyMus);
            for (const auto* field : textExpressionFields) {
                reporting.report().setField(reporting.instanceKey(reportInstance), field,
                    typename Reporting::FieldInfo{Reporting::Origin::Unmapped, 0, 0, 0});
            }
        });
        const auto assign = [&](auto member, const char* name, auto value, std::size_t slot,
                                bool adjusted = false) {
            target.get()->*member = value;
            withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                const auto& row = rows[source->classRecords ? 0 : slot / records::otherWordCount];
                const auto offset =
                    source->classRecords ? slot * 2 : (slot % records::otherWordCount) * 2;
                reporting.report().setField(reporting.instanceKey(reportInstance), name,
                    typename Reporting::FieldInfo{adjusted ? Reporting::Origin::LegacyMusAdjusted
                                                           : Reporting::Origin::LegacyMus,
                        row.blockOffset, row.decodedOffset + offset, words[slot],
                        source->identity});
            });
        };
        const auto flags = static_cast<std::uint16_t>(words[5]);
        const bool smartMusic = (flags & 0xffU) == 0x0fU;
        const bool hasCategory =
            sourceAtOrAfter(context.profile, FormatEpoch::ZlibLegacy, versions::finale2009);
        // Believed: automatic rehearsal styles and the hide-number/match-playback
        // controls share the Finale 2010 introduction boundary.
        const bool hasRehearsalStyle =
            sourceAtOrAfter(context.profile, FormatEpoch::ZlibLegacy, versions::finale2010);
        if (!prime) {
            target->horzMeasExprAlign = musx::dom::others::HorizontalMeasExprAlign::Manual;
            target->vertMeasExprAlign = musx::dom::others::VerticalMeasExprAlign::Manual;
        }
        reportAbsentTextExpressionFields(
            context.report, reportInstance, prime, hasCategory, hasRehearsalStyle);
        if (hasRehearsalStyle) {
            assign(&TextExpressionTarget::hideMeasureNum, "hideMeasureNum", bool(flags & 0x8000U),
                   5);
            assign(&TextExpressionTarget::matchPlayback, "matchPlayback", bool(flags & 0x4000U), 5);
            if (auto style = expressionRehearsalStyle(words[1])) {
                assign(&TextExpressionTarget::rehearsalMarkStyle, "rehearsalMarkStyle", *style, 1);
            }
        }
        if (prime)
            assign(&TextExpressionTarget::textIdKey, "textIdKey",
                   static_cast<musx::dom::Cmper>(words[0]), 0);
        assign(&TextExpressionTarget::value, "value", (flags & 0x2000U) ? 0 : words[2], 2);
        assign(&TextExpressionTarget::execShape, "execShape",
               static_cast<musx::dom::Cmper>((flags & 0x2000U) ? words[2] : 0), 2);
        // Normalize removed SmartMusic playback to None and clear its selector,
        // retaining unrelated flags and the original words in field provenance.
        assign(&TextExpressionTarget::auxData1, "auxData1", smartMusic ? 0 : words[3], 3,
               smartMusic);
        assign(&TextExpressionTarget::playPass, "playPass", words[4], 4);
        assign(&TextExpressionTarget::hasEnclosure, "hasEnclosure", bool(flags & 0x0800U), 5);
        assign(&TextExpressionTarget::useAuxData, "useAuxData", bool(flags & 0x1000U), 5);
        if (prime)
            assign(&TextExpressionTarget::breakMmRest, "breakMmRest", bool(flags & 0x0400U), 5);
        if (auto playback = smartMusic ? musx::dom::others::PlaybackType::None
                                       : expressionPlayback(flags & 0xffU))
            assign(&TextExpressionTarget::playbackType, "playbackType", *playback, 5, smartMusic);
        if (prime) {
            // Unverified: pre-category definitions use their separate note anchors as
            // the modern positioning selectors. Other positioning fields remain
            // unchanged.
            const std::size_t horizontalSlot = hasCategory ? 6 : 9;
            if (auto align = hasCategory ? expressionHorizontalAlignment(words[horizontalSlot])
                                         : expressionNoteHorizontalAlignment(words[horizontalSlot]))
                assign(&TextExpressionTarget::horzMeasExprAlign, "horzMeasExprAlign", *align,
                       horizontalSlot);
            if (auto justify = expressionJustification(words[7]))
                assign(&TextExpressionTarget::horzExprJustification, "horzExprJustification",
                       *justify, 7);
            assign(&TextExpressionTarget::measXAdjust, "measXAdjust", words[8], 8);
            const std::size_t verticalSlot = hasCategory ? 12 : 14;
            if (auto align = hasCategory ? expressionVerticalAlignment(words[verticalSlot])
                                         : expressionNoteVerticalAlignment(words[verticalSlot]))
                assign(&TextExpressionTarget::vertMeasExprAlign, "vertMeasExprAlign", *align,
                       verticalSlot);
            const std::size_t baselineSlot = hasCategory ? 15 : 13;
            assign(&TextExpressionTarget::yAdjustBaseline, "yAdjustBaseline", words[baselineSlot],
                   baselineSlot);
            assign(&TextExpressionTarget::yAdjustEntry, "yAdjustEntry", words[16], 16);
            if (hasCategory) {
                const auto category = static_cast<std::uint16_t>(words[13]);
                assign(&TextExpressionTarget::categoryId, "categoryId",
                       static_cast<musx::dom::Cmper>(category & 0x3fffU), 13);
                assign(&TextExpressionTarget::useCategoryFonts, "useCategoryFonts",
                       bool(category & 0x8000U), 13);
                assign(&TextExpressionTarget::useCategoryPos, "useCategoryPos",
                       bool(category & 0x4000U), 13);
            }
            const auto trailer =
                std::span<const std::uint8_t>(payload).subspan(primeExpressionHeaderSize);
            target->description =
                versions::storesUnicodeCodepoints(context.profile.version)
                    ? text::utf16ToUtf8(payloadWords(trailer, context.profile.byteOrder))
                    : text::toUtf8(payloadString(trailer, 0, trailer.size()),
                                   context.profile.platform);
            withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                const auto& descriptionRow = rows[source->classRecords || trailer.empty()
                        ? 0
                        : primeExpressionHeaderSize / (2 * records::otherWordCount)];
                reporting.report().setField(reporting.instanceKey(reportInstance), "description",
                    typename Reporting::FieldInfo{Reporting::Origin::LegacyMus,
                        descriptionRow.blockOffset,
                        descriptionRow.decodedOffset +
                            (source->classRecords ? primeExpressionHeaderSize : 0),
                        static_cast<std::int64_t>(trailer.size()), source->identity});
            });
        } else {
            context.pending.checks.push_back([&context, target, row = rows.front(), payload] {
                synthesizeExpressionText(context, target, row, payload);
            });
        }
        if (!hasCategory) {
            context.pending.checks.push_back([&context, target] {
                using Category = musx::dom::others::MarkingCategory;
                for (const auto& category :
                     context.document->getOthers()->getArray<Category>(musx::dom::SCORE_PARTID)) {
                    if (category->categoryType != Category::CategoryType::Misc) continue;
                    target->categoryId = category->getCmper();
                    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                        reporting.report().setField(
                            reporting.template instanceKey<TextExpressionTarget>(
                                target->getSourcePartId(), target->getCmper()),
                            "categoryId",
                            typename Reporting::FieldInfo{
                                Reporting::Origin::LegacyBehavior, 0, 0, target->categoryId});
                    });
                    break;
                }
            });
        }
        context.document->getOthers()->add(TextExpressionTarget::XmlNodeName, std::move(target));
    }
}

} // namespace others
} // namespace finale_mus_reader
