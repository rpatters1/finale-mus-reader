// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using Target = musx::dom::others::TextBlock;

constexpr auto textBlockTag = records::packTag("TX");
constexpr auto codaTextStyleTag = records::packTag("HS");
constexpr records::LegacyTag textBlockClass = 0x00b7;
constexpr std::size_t textBlockWordCount = 12;
constexpr std::size_t textTypeWord = 12;

// The optional trailing word identifies the texts-pool family. Its two observed encodings
// carry the same structural meaning, so the stored value is a more reliable selector than a
// document-version boundary. A zero or absent word predates the discriminator and retains the
// musxdom Block default.
std::optional<Target::TextType> textTypeFromLegacy(std::int16_t value)
{
    switch (static_cast<std::uint16_t>(value)) {
    case records::packTag("bl"):
    case 2004:
        return Target::TextType::Block;
    case records::packTag("xp"):
    case 2006:
        return Target::TextType::Expression;
    default:
        return std::nullopt;
    }
}

std::int32_t highFirstLong(std::int16_t high, std::int16_t low)
{
    return static_cast<std::int32_t>(
        (static_cast<std::uint32_t>(static_cast<std::uint16_t>(high)) << 16U) |
        static_cast<std::uint16_t>(low));
}

const records::LegacyRow& textBlockRow(const RecordFamilySource& source,
                                       std::span<const records::LegacyRow> rows,
                                       std::size_t wordIndex)
{
    return rows[source.classRecords ? 0 : wordIndex / records::otherWordCount];
}

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
void reportValue(const ImportContext& context, std::uint16_t partId, musx::dom::Cmper cmper,
                 const char* field, std::int64_t rawValue, const records::LegacyRow& row)
{
    FINALE_MUS_READER_REPORT_FIELD(context.report,
        instanceKey<Target>(partId, cmper), field,
        {ValueOrigin::LegacyMus, row.blockOffset, row.decodedOffset, rawValue});
}

void reportBehavior(const ImportContext& context, std::uint16_t partId, musx::dom::Cmper cmper,
                    const char* field, std::int64_t value)
{
    FINALE_MUS_READER_REPORT_FIELD(context.report,
        instanceKey<Target>(partId, cmper), field,
        {ValueOrigin::LegacyBehavior, 0, 0, value});
}
#else
#define reportValue(...) ((void)0)
#define reportBehavior(...) ((void)0)
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

void applyLegacyTextBlockCorners(
    const ImportContext& context, std::uint16_t partId, musx::dom::Cmper cmper, Target& target)
{
    // Legacy MUS TextBlocks predate rounded frames and therefore use square corners with a
    // zero radius.
    target.roundCorners = false;
    target.cornerRadius = 0;
    reportBehavior(context, partId, cmper, "roundCorners", 0);
    reportBehavior(context, partId, cmper, "cornerRadius", 0);
}

void populateStoredTextBlock(const ImportContext& context, musx::dom::Cmper cmper,
                             std::span<const records::LegacyRow> rows,
                             const RecordFamilySource& source)
{
    const auto words = collectRecordWords(source, rows, context.profile.byteOrder);
    if (words.size() < textBlockWordCount) {
        context.report.diagnostics.push_back(
            {musx::util::Logger::LogLevel::Info,
             "Text block " + std::to_string(cmper) + " has an incomplete record."});
        return;
    }

    const auto partId = rows.front().partId;
    auto target = createOthersRecordTarget<Target>(context.document, source, rows.front(), cmper);
    if (!target) return;
    target->textId = static_cast<musx::dom::Cmper>(words[0]);
    target->width = words[1];
    target->height = words[2];
    target->shapeId = static_cast<musx::dom::Cmper>(words[3]);
    const auto flags = static_cast<std::uint16_t>(words[7]);
    const bool hasPercentageLineSpacing = (flags & 0x1000U) != 0;
    const bool upgradesZeroPercentToEvpu = hasPercentageLineSpacing && words[4] == 0;
    if (upgradesZeroPercentToEvpu)
        target->lineSpacingEvpu = 0;
    else if (hasPercentageLineSpacing)
        target->lineSpacingPercentage = words[4];
    else
        target->lineSpacingEvpu = words[4];
    target->xAdd = words[5];
    target->yAdd = words[6];
    target->justify = static_cast<Target::TextJustify>(legacyCenterOppositeOrder(flags & 0x0007U));
    target->newPos36 = (flags & 0x0008U) != 0;
    target->showShape = (flags & 0x0200U) != 0;
    target->noExpandSingleWord = (flags & 0x0400U) != 0;
    target->wordWrap = (flags & 0x0800U) != 0;
    target->inset = highFirstLong(words[8], words[9]);
    target->stdLineThickness = highFirstLong(words[10], words[11]);
    applyLegacyTextBlockCorners(context, partId, cmper, *target);
    if (words.size() > textTypeWord) {
        if (const auto textType = textTypeFromLegacy(words[textTypeWord])) {
            target->textType = *textType;
            reportValue(context, partId, cmper, "textType", words[textTypeWord],
                        textBlockRow(source, rows, textTypeWord));
        } else if (words[textTypeWord] != 0) {
            context.report.diagnostics.push_back(
                {musx::util::Logger::LogLevel::Info,
                 "Text block " + std::to_string(cmper) +
                     " has an unrecognized text-family discriminator " +
                     std::to_string(words[textTypeWord]) + "."});
        }
    }

    constexpr const char* directNames[] = {"textId", "width", "height", "shapeId"};
    for (std::size_t slot = 0; slot < std::size(directNames); ++slot) {
        reportValue(context, partId, cmper, directNames[slot], words[slot],
                    textBlockRow(source, rows, slot));
    }
    reportValue(context, partId, cmper, hasPercentageLineSpacing ? "lineSpacingPercentage"
                                                                 : "lineSpacingEvpu",
        words[4], textBlockRow(source, rows, 4));
    if (upgradesZeroPercentToEvpu)
        reportBehavior(context, partId, cmper, "lineSpacingEvpu", 0);
    reportValue(context, partId, cmper, "xAdd", words[5], textBlockRow(source, rows, 5));
    reportValue(context, partId, cmper, "yAdd", words[6], textBlockRow(source, rows, 6));
    constexpr const char* flagNames[] = {"justify", "newPos36", "showShape", "noExpandSingleWord",
                                         "wordWrap"};
    const std::int64_t flagValues[] = {flags & 0x0007U, (flags >> 3U) & 1U, (flags >> 9U) & 1U,
                                       (flags >> 10U) & 1U, (flags >> 11U) & 1U};
    for (std::size_t index = 0; index < std::size(flagNames); ++index) {
        reportValue(context, partId, cmper, flagNames[index], flagValues[index],
                    textBlockRow(source, rows, 7));
    }
    reportValue(context, partId, cmper, "inset", highFirstLong(words[8], words[9]),
                textBlockRow(source, rows, 8));
    reportValue(context, partId, cmper, "stdLineThickness", highFirstLong(words[10], words[11]),
                textBlockRow(source, rows, 10));
    context.document->getOthers()->add(Target::XmlNodeName, std::move(target));
}

void importStoredTextBlocks(const ImportContext& context)
{
    const auto source = selectRecordFamilySource(context, context.index.getOthers(),
        context.index.getClassOthers(), textBlockTag, textBlockClass);
    if (!source) return;
    for (const auto [partId, cmper] : recordKeys(*source)) {
        populateStoredTextBlock(context, cmper,
            source->pool->getArray(source->identity, cmper, 0, partId), *source);
    }
}

void importCodaTextBlocks(const ImportContext& context)
{
    const auto blockTexts = context.document->getTexts()->getArray<musx::dom::texts::BlockText>();
    std::size_t textIndex = 0;
    for (const auto cmper : context.index.getOthers().cmpersForTag(codaTextStyleTag)) {
        for (const auto& row : context.index.getOthers().getArray(codaTextStyleTag, cmper)) {
            if (textIndex >= blockTexts.size())
                return;
            const auto textId = blockTexts[textIndex++]->getTextNumber();
            auto target = std::make_shared<Target>(context.document, musx::dom::SCORE_PARTID,
                                                   musx::dom::EnigmaBase::ShareMode::All, textId);
            target->textId = textId;
            target->lineSpacingPercentage = 100;
            const auto flags = static_cast<std::uint16_t>(row.words[5]);
            target->justify =
                static_cast<Target::TextJustify>(legacyCenterOppositeOrder(flags & 0x0003U));
            target->shapeId = 0;
            target->newPos36 = false;
            target->showShape = false;
            target->noExpandSingleWord = false;
            target->wordWrap = true;
            applyLegacyTextBlockCorners(context, musx::dom::SCORE_PARTID, textId, *target);

            reportValue(context, musx::dom::SCORE_PARTID, textId, "textId", textId, row);
            reportValue(context, musx::dom::SCORE_PARTID, textId, "justify", flags & 0x0003U, row);
            reportBehavior(context, musx::dom::SCORE_PARTID, textId, "lineSpacingPercentage", 100);
            reportBehavior(context, musx::dom::SCORE_PARTID, textId, "shapeId", 0);
            reportBehavior(context, musx::dom::SCORE_PARTID, textId, "newPos36", 0);
            reportBehavior(context, musx::dom::SCORE_PARTID, textId, "showShape", 0);
            reportBehavior(context, musx::dom::SCORE_PARTID, textId, "noExpandSingleWord", 0);
            reportBehavior(context, musx::dom::SCORE_PARTID, textId, "wordWrap", 1);
            context.document->getOthers()->add(Target::XmlNodeName, std::move(target));
        }
    }
}

} // namespace

void importTextBlocks(const ImportContext& context)
{
    if (context.profile.epoch != FormatEpoch::CodaBanner) {
        importStoredTextBlocks(context);
        return;
    }
    // A Coda-banner document names no text from its style rows: the two are paired by position
    // against the block texts, so the text pool has to be complete first. Deferring the pass keeps
    // that out of the registry's line order -- see @ref PendingReferences::checks.
    context.pending.checks.push_back([&context] { importCodaTextBlocks(context); });
}

} // namespace others
} // namespace finale_mus_reader

#if !defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
#undef reportValue
#undef reportBehavior
#endif // !defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
