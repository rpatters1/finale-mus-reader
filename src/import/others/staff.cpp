// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/others.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "import/support/enigma_text.h"
#include "import/support/legacy_font.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace others {
namespace {

using StaffTarget = musx::dom::others::Staff;

struct StaffFinale27Defaults
{
    bool lineSpace{};
    bool restOffsets{};
    bool stemReversal{};
    bool repeatDotOffsets{};
};

struct StaffLegacySemantics
{
    bool hasAlternateNotationProperties{};
    bool usesAggregateAlternateNotationItems{};
    bool hideNoteAttachedItems{};
};

struct StaffBooleanLegacyDefault
{
    const char* name;
    bool StaffTarget::*member;
    bool value;
};

constexpr StaffBooleanLegacyDefault staffAlternateNotationBooleanDefaults[] = {
    {"altHideArtics", &StaffTarget::altHideArtics, false},
    {"altHideLyrics", &StaffTarget::altHideLyrics, false},
    {"altHideSmartShapes", &StaffTarget::altHideSmartShapes, false},
    {"altRhythmStemsUp", &StaffTarget::altRhythmStemsUp, false},
    {"altSlashDots", &StaffTarget::altSlashDots, true},
    {"altHideOtherNotes", &StaffTarget::altHideOtherNotes, false},
    {"altHideOtherArtics", &StaffTarget::altHideOtherArtics, false},
    {"altHideExpressions", &StaffTarget::altHideExpressions, false},
    {"altHideOtherLyrics", &StaffTarget::altHideOtherLyrics, false},
    {"altHideOtherSmartShapes", &StaffTarget::altHideOtherSmartShapes, false},
    {"altHideOtherExpressions", &StaffTarget::altHideOtherExpressions, false},
};

constexpr StaffBooleanLegacyDefault staffCodaTabBooleanDefaults[] = {
    {"showTabClefAllSys", &StaffTarget::showTabClefAllSys, true},
    {"hideRests", &StaffTarget::hideRests, true},
    {"hideDots", &StaffTarget::hideDots, true},
    {"hideStems", &StaffTarget::hideStems, true},
    {"hideTuplets", &StaffTarget::hideTuplets, true},
};

template <std::size_t N>
void applyStaffBooleanLegacyBehavior(
    StaffTarget& target, const StaffBooleanLegacyDefault (&fields)[N])
{
    for (const auto& field : fields) target.*field.member = field.value;
}

void applyStaffAlternateNotationLegacyBehavior(StaffTarget& target)
{
    target.altNotation = StaffTarget::AlternateNotation::Normal;
    target.altLayer = 0;
    applyStaffBooleanLegacyBehavior(target, staffAlternateNotationBooleanDefaults);
}

constexpr records::LegacyTag staffTag = records::packTag("IS");
constexpr records::LegacyTag staffClass = 0x00e7;
constexpr records::LegacyTag codaStaffAttributesTag = records::packTag("IA");
constexpr records::LegacyTag codaStaffFullNameTag = records::packTag("IN");
constexpr records::LegacyTag codaStaffAbbreviatedNameTag = records::packTag("in");

constexpr std::size_t staffBaseWords = 18;
constexpr std::size_t staffBaseBytes = staffBaseWords * 2;
constexpr std::size_t staffCodaWords = 6;

constexpr std::size_t codaDefaultClefSlot = 0;
constexpr std::size_t codaVerticalTabOffsetSlot = 1;
constexpr std::size_t codaStaffLinesSlot = 2;
constexpr std::size_t codaTranspositionSlot = 4;
constexpr std::size_t codaDisplayFlagsSlot = 5;
constexpr std::size_t codaCustomStaffMarkerSlot = 2;
constexpr std::size_t codaNoteFontIdSlot = 3;
constexpr std::size_t codaNoteFontSizeEffectsSlot = 4;
constexpr std::size_t codaPrimaryFlagsSlot = 5;

constexpr std::size_t botBarlineSlot = 0;
constexpr std::size_t tablaturePositionsSlot = 1;
constexpr std::size_t altFlagsSlot = 2;
constexpr std::size_t noteFontIdSlot = 3;
constexpr std::size_t noteFontSizeEffectsSlot = 4;
constexpr std::size_t primaryFlagsSlot = 5;
constexpr std::size_t clefsSlot = 6;
constexpr std::size_t topStaffLinesSlot = 7;
constexpr std::size_t bottomStaffLinesSlot = 8;
constexpr std::size_t topBarlineSlot = 9;
constexpr std::size_t transpositionSlot = 10;
constexpr std::size_t displayFlagsSlot = 11;
constexpr std::size_t doubleWholeWholeRestSlot = 12;
constexpr std::size_t halfOtherRestSlot = 13;
constexpr std::size_t stemReversalSlot = 14;
constexpr std::size_t fullNameSlot = 15;
constexpr std::size_t abbreviatedNameSlot = 16;
constexpr std::size_t repeatDotOffsetsSlot = 17;
constexpr std::size_t lineSpaceSlot = 18;
constexpr std::size_t verticalTabOffsetSlot = 20;
constexpr std::size_t extendedFlagsSlot = 22;
constexpr std::size_t fretInstrumentSlot = 23;
constexpr std::size_t horizontalStemUpSlot = 24;
constexpr std::size_t horizontalStemDownSlot = 26;
constexpr std::size_t verticalStemStartUpSlot = 28;
constexpr std::size_t verticalStemStartDownSlot = 30;
constexpr std::size_t verticalStemEndUpSlot = 32;
constexpr std::size_t verticalStemEndDownSlot = 34;
constexpr std::size_t alternateFlags2Slot = 36;
constexpr std::size_t autoNumberingSlot = 37;
constexpr std::size_t instrumentUuidSlot = 40;
constexpr std::size_t instrumentUuidBytes = 16;
constexpr std::size_t finale2012StaffBytes = instrumentUuidSlot * 2 + instrumentUuidBytes;

[[nodiscard]] bool hasSecondAlternateNotationFlags(std::span<const std::uint8_t> payload)
{
    // The second word supplies independent other-layer controls; without it,
    // the first word's controls retain their aggregate semantics. Payload shape
    // states this distinction even when a source version is unavailable.
    return payload.size() >= (alternateFlags2Slot + 1) * 2;
}

StaffFinale27Defaults selectStaffFinale27Defaults(
    const SourceProfile& profile, std::span<const std::uint8_t> payload)
{
    const auto coda = profile.epoch == FormatEpoch::CodaBanner;
    const auto wordCount = payload.size() / 2;
    return {.lineSpace = coda || wordCount < lineSpaceSlot + 2,
        .restOffsets = coda,
        .stemReversal = coda,
        .repeatDotOffsets = coda || wordCount <= staffBaseWords};
}

const StaffTarget& finale27StaffDefaults(const ImportContext& context)
{
    const auto result =
        context.referenceDocument->getOthers()->get<StaffTarget>(musx::dom::SCORE_PARTID, 1);
    if (!result) throw std::logic_error("Finale 27 reference is missing its standard Staff");
    return *result;
}

int staffFinale27NoteheadFontSize(const ImportContext& context)
{
    const auto font = musx::dom::options::FontOptions::getFontInfoOrNull(
        context.referenceDocument, musx::dom::options::FontOptions::FontType::Noteheads);
    if (!font) throw std::logic_error("Finale 27 reference is missing its notehead font default");
    return font->fontSize;
}

void applyStaffFinale27Defaults(
    StaffTarget& target, const StaffTarget& defaults, StaffFinale27Defaults selected)
{
    if (selected.lineSpace) target.lineSpace = defaults.lineSpace;
    if (selected.restOffsets) {
        target.dwRestOffset = defaults.dwRestOffset;
        target.wRestOffset = defaults.wRestOffset;
        target.hRestOffset = defaults.hRestOffset;
        target.otherRestOffset = defaults.otherRestOffset;
    }
    if (selected.stemReversal) target.stemReversal = defaults.stemReversal;
    if (selected.repeatDotOffsets) {
        target.botRepeatDotOff = defaults.botRepeatDotOff;
        target.topRepeatDotOff = defaults.topRepeatDotOff;
    }
}

void synthesizeCodaStaffName(const ImportContext& context, StaffTarget& staff,
    records::LegacyTag tag, musx::dom::options::FontOptions::FontType fontType,
    musx::dom::Cmper StaffTarget::*nameMember,
    text::EnigmaFontResolutionCache& fontResolutionCache)
{
    using BlockText = musx::dom::texts::BlockText;
    using TextBlock = musx::dom::others::TextBlock;

    const auto& pool = context.index.getOthers();
    const auto rows = pool.getArray(tag, staff.getCmper(), 0, staff.getSourcePartId());
    if (rows.empty()) return;
    const auto raw = readRowText(pool, rows);
    if (raw.empty()) return;

    const auto textNumber = context.document->getTexts()->nextFreeCmper<BlockText>();
    const auto blockNumber =
        context.document->getOthers()->nextFreeCmper<TextBlock>(staff.getSourcePartId());
    if (!textNumber || !blockNumber) {
        context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
            "Staff-name import exhausted the text identifier space."});
        return;
    }

    const text::EnigmaTextSource source{context.document,
        /*utf8*/ false, context.profile.platform,
        musx::dom::options::FontOptions::getFontInfoOrNull(context.document, fontType),
        &fontResolutionCache};
    auto converted =
        text::toModernEnigmaText(std::span<const std::uint8_t>(
                                     reinterpret_cast<const std::uint8_t*>(raw.data()), raw.size()),
            source);
    auto rawText = std::make_shared<BlockText>(context.document, musx::dom::SCORE_PARTID,
        musx::dom::EnigmaBase::ShareMode::All, *textNumber);
    rawText->text = std::move(converted.text);

    auto block = std::make_shared<TextBlock>(context.document, staff.getSourcePartId(),
        musx::dom::EnigmaBase::ShareMode::All, *blockNumber);
    block->textId = *textNumber;
    block->textType = TextBlock::TextType::Block;
    block->lineSpacingPercentage = 100;
    block->newPos36 = true;
    block->showShape = true;
    block->wordWrap = true;
    staff.*nameMember = *blockNumber;

    withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
        const auto& row = rows.front();
        const auto rawKey =
            reporting.template instanceKey<BlockText>(musx::dom::SCORE_PARTID, *textNumber);
        const auto blockKey =
            reporting.template instanceKey<TextBlock>(staff.getSourcePartId(), *blockNumber);
        const auto staffKey =
            reporting.template instanceKey<StaffTarget>(staff.getSourcePartId(), staff.getCmper());
        reporting.report().setInstanceOrigin(rawKey, Reporting::Origin::LegacyMus);
        reporting.report().setInstanceOrigin(blockKey, Reporting::Origin::LegacyBehavior);
        reporting.textField(rawKey, "text", converted);
        reporting.report().setField(rawKey, "text",
            {Reporting::Origin::LegacyMus, row.blockOffset, row.decodedOffset,
                static_cast<std::int64_t>(rawText->text.size()), tag});
        reporting.report().setField(staffKey,
            nameMember == &StaffTarget::fullNameTextId ? "fullNameTextId" : "abbrvNameTextId",
            {Reporting::Origin::LegacyBehavior, row.blockOffset, row.decodedOffset, *blockNumber,
                tag});
        reporting.report().setField(blockKey, "textId",
            {Reporting::Origin::LegacyBehavior, row.blockOffset, row.decodedOffset, *textNumber,
                tag});
        const auto behavior = [&](const char* field, std::int64_t value) {
            reporting.report().setField(
                blockKey, field, {Reporting::Origin::LegacyBehavior, 0, 0, value});
        };
        behavior("lineSpacingPercentage", 100);
        behavior("shapeId", 0);
        behavior("newPos36", 1);
        behavior("showShape", 1);
        behavior("noExpandSingleWord", 0);
        behavior("wordWrap", 1);
        behavior("roundCorners", 0);
        behavior("cornerRadius", 0);
    });

    context.document->getTexts()->add(BlockText::XmlNodeName, std::move(rawText));
    context.document->getOthers()->add(TextBlock::XmlNodeName, std::move(block));
}

constexpr std::uint16_t altNotationMask = 0x000f;
constexpr std::uint16_t altLayerMask = 0x00f0;
constexpr std::uint16_t altShowArticulationsMask = 0x0100;
constexpr std::uint16_t altShowLyricsMask = 0x0200;
constexpr std::uint16_t altShowSmartShapesMask = 0x0400;
constexpr std::uint16_t altRhythmStemsUpMask = 0x0800;
constexpr std::uint16_t altSlashDotsMask = 0x1000;
constexpr std::uint16_t altShowOtherNotesMask = 0x2000;
constexpr std::uint16_t altShowOtherArticulationsMask = 0x4000;
constexpr std::uint16_t altShowExpressionsMask = 0x8000;

constexpr std::uint16_t hideRepeatBottomDotMask = 0x8000;
constexpr std::uint16_t flatBeamsMask = 0x4000;
constexpr std::uint16_t percussionNotationMask = 0x2000;
constexpr std::uint16_t hideFretboardsMask = 0x0800;
constexpr std::uint16_t tablatureNotationMask = 0x0200;
constexpr std::uint16_t blankMeasureMask = 0x0100;
constexpr std::uint16_t useNoteShapesMask = 0x0080;
constexpr std::uint16_t hideRepeatTopDotMask = 0x0040;
constexpr std::uint16_t useNoteFontMask = 0x0020;
constexpr std::uint16_t codaUseNoteFontMask = 0x0040;
constexpr std::uint16_t hideLyricsMask = 0x0010;
constexpr std::uint16_t showNameInPartsMask = 0x0008;
constexpr std::uint16_t showNoteColorsMask = 0x0004;
constexpr std::uint16_t noOptimizeMask = 0x0001;

constexpr std::uint16_t setToClefMask = 0x8000;
constexpr std::uint16_t noSimplifyKeyMask = 0x4000;
constexpr std::uint16_t chromaticTranspositionMask = 0x2000;

constexpr std::uint16_t floatKeysMask = 0x8000;
constexpr std::uint16_t floatTimeMask = 0x4000;
constexpr std::uint16_t breakBarlinesMask = 0x2000;
constexpr std::uint16_t breakRepeatBarlinesMask = 0x1000;
constexpr std::uint16_t hideMeasureNumbersMask = 0x0400;
constexpr std::uint16_t hideRepeatsMask = 0x0200;
constexpr std::uint16_t hideNameMask = 0x0100;
constexpr std::uint16_t hideBarlinesMask = 0x0080;
constexpr std::uint16_t hideRepeatBarsMask = 0x0040;
constexpr std::uint16_t hideKeySignaturesMask = 0x0020;
constexpr std::uint16_t hideTimeSignaturesMask = 0x0010;
constexpr std::uint16_t hideClefsMask = 0x0008;
constexpr std::uint16_t hideStaffLinesMask = 0x0004;
constexpr std::uint16_t hideChordsMask = 0x0002;
constexpr std::uint16_t noKeyMask = 0x0001;

constexpr std::uint16_t clefFirstMeasureOnlyMask = 0x0001;
constexpr std::uint16_t showRestsMask = 0x0002;
constexpr std::uint16_t showTiesMask = 0x0004;
constexpr std::uint16_t showDotsMask = 0x0008;
constexpr std::uint16_t showStemsMask = 0x0010;
constexpr std::uint16_t stemDirectionMask = 0x0060;
constexpr std::uint16_t stemsAlwaysUpMask = 0x0040;
constexpr std::uint16_t stemsAlwaysDownMask = 0x0020;
constexpr std::uint16_t stemStartFromStaffMask = 0x0080;
constexpr std::uint16_t stemsFixedEndMask = 0x0100;
constexpr std::uint16_t useTabLettersMask = 0x0200;
constexpr std::uint16_t showBeamsMask = 0x0400;
constexpr std::uint16_t breakTabLinesMask = 0x0800;
constexpr std::uint16_t stemsFixedStartMask = 0x1000;
constexpr std::uint16_t showTupletsMask = 0x2000;
constexpr std::uint16_t hideModeMask = 0xc000;
constexpr std::uint16_t hideScoreMask = 0x4000;
constexpr std::uint16_t hideScorePartsMask = 0x8000;

constexpr std::uint16_t altShowOtherLyricsMask = 0x0001;
constexpr std::uint16_t altShowOtherSmartShapesMask = 0x0002;
constexpr std::uint16_t altShowOtherExpressionsMask = 0x0004;

constexpr std::uint16_t autoNumberingEnabledMask = 0x8000;
constexpr std::uint16_t autoNumberingStyleMask = 0x7fff;

[[nodiscard]] bool sourceHasStaffAlternateNotationProperties(const SourceProfile& profile)
{
    // Before Finale 2000, alternate notation is applied by range records rather
    // than Staff properties.
    return sourceAtOrAfter(profile, FormatEpoch::UncompressedLegacy, versions::finale2000);
}

[[nodiscard]] std::int8_t signedByte(std::uint16_t value, bool high)
{
    return static_cast<std::int8_t>(high ? value >> 8U : value & 0xffU);
}

[[nodiscard]] int signExtend(std::uint16_t value, unsigned bits)
{
    const auto mask = (std::uint16_t{1} << bits) - 1U;
    value &= mask;
    const auto sign = std::uint16_t{1} << (bits - 1U);
    return static_cast<int>((value ^ sign) - sign);
}

void decodeStaffTransposition(const std::shared_ptr<StaffTarget>& target, std::uint16_t value)
{
    if (!value) return;
    target->transposition = std::make_shared<StaffTarget::Transposition>(target);
    target->transposition->setToClef = value & setToClefMask;
    target->transposition->noSimplifyKey = value & noSimplifyKeyMask;
    if (value & chromaticTranspositionMask) {
        target->transposition->chromatic = std::make_shared<StaffTarget::ChromaticTransposition>();
        target->transposition->chromatic->alteration = signExtend(value >> 8U, 4);
        target->transposition->chromatic->diatonic = signExtend(value, 8);
    } else {
        target->transposition->keysig = std::make_shared<StaffTarget::KeySigTransposition>();
        target->transposition->keysig->interval = signExtend(value >> 6U, 6);
        target->transposition->keysig->adjust = signExtend(value, 6);
    }
}

[[nodiscard]] std::string instrumentUuid(std::span<const std::uint8_t> payload)
{
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(36);
    for (std::size_t index = 0; index < instrumentUuidBytes; ++index) {
        if (index == 4 || index == 6 || index == 8 || index == 10) result.push_back('-');
        const auto value = payload[instrumentUuidSlot * 2 + index];
        result.push_back(hex[value >> 4U]);
        result.push_back(hex[value & 0x0fU]);
    }
    return result;
}

[[nodiscard]] bool hasFinale2012StaffLayout(std::span<const std::uint8_t> payload)
{
    return payload.size() >= finale2012StaffBytes;
}

[[nodiscard]] std::vector<int> customStaffLines(std::uint16_t top, std::uint16_t bottom)
{
    std::vector<int> result;
    for (unsigned bit = 0; bit < 16; ++bit) {
        if (bottom & (std::uint16_t{1} << bit)) result.push_back(15 - static_cast<int>(bit));
    }
    // The low top bit is the custom-layout marker; the remaining bits name lines
    // above the reference line. musxdom's representable range ends at line 26.
    for (unsigned bit = 1; bit <= 11; ++bit) {
        if (top & (std::uint16_t{1} << bit)) result.push_back(15 + static_cast<int>(bit));
    }
    return result;
}

[[nodiscard]] StaffTarget::NotationStyle notationStyle(std::uint16_t flags)
{
    if (flags & percussionNotationMask) return StaffTarget::NotationStyle::Percussion;
    if (flags & tablatureNotationMask) return StaffTarget::NotationStyle::Tablature;
    return StaffTarget::NotationStyle::Standard;
}

[[nodiscard]] musx::dom::StemDirection stemDirection(std::uint16_t flags)
{
    switch (flags & stemDirectionMask) {
    case stemsAlwaysUpMask:
        return musx::dom::StemDirection::AlwaysUp;
    case stemsAlwaysDownMask:
        return musx::dom::StemDirection::AlwaysDown;
    default:
        return musx::dom::StemDirection::Default;
    }
}

[[nodiscard]] StaffTarget::HideMode hideMode(std::uint16_t flags)
{
    switch (flags & hideModeMask) {
    case hideScoreMask:
        return StaffTarget::HideMode::Score;
    case hideScorePartsMask:
        return StaffTarget::HideMode::ScoreParts;
    case hideModeMask:
        return StaffTarget::HideMode::Cutaway;
    default:
        return StaffTarget::HideMode::None;
    }
}

template <typename Reporting, typename Value>
void reportStaffField(Reporting& reporting, const typename Reporting::InstanceKey& key,
    const RecordFamilySource& source, std::span<const records::LegacyRow> rows, const char* member,
    std::size_t slot, Value value)
{
    const auto byteOffset = slot * 2;
    const auto& row = source.classRecords
                          ? rows.front()
                          : rows[(std::min)(slot / records::otherWordCount, rows.size() - 1)];
    const auto rowByteOffset =
        source.classRecords ? byteOffset : (slot % records::otherWordCount) * 2;
    reporting.report().setField(key, Reporting::memberName(member),
        typename Reporting::FieldInfo{Reporting::Origin::LegacyMus, row.blockOffset,
            row.decodedOffset + rowByteOffset, static_cast<std::int64_t>(value), source.identity});
}

template <typename Reporting>
void reportStaffBooleanLegacyBehavior(Reporting& reporting, const StaffTarget& target,
    const typename Reporting::InstanceKey& key, std::span<const StaffBooleanLegacyDefault> fields)
{
    for (const auto& field : fields) {
        reporting.report().setField(
            key, field.name, {Reporting::Origin::LegacyBehavior, 0, 0, target.*field.member});
    }
}

template <typename Reporting>
void reportStaffAlternateNotationLegacyBehavior(
    Reporting& reporting, const StaffTarget& target, const typename Reporting::InstanceKey& key)
{
    reporting.report().setField(key, "altNotation",
        {Reporting::Origin::LegacyBehavior, 0, 0, static_cast<std::int64_t>(target.altNotation)});
    reporting.report().setField(
        key, "altLayer", {Reporting::Origin::LegacyBehavior, 0, 0, target.altLayer});
    reportStaffBooleanLegacyBehavior(reporting, target, key, staffAlternateNotationBooleanDefaults);
}

template <typename Reporting>
void reportStaffFont(Reporting& reporting, const StaffTarget& target,
    const typename Reporting::InstanceKey& key, const RecordFamilySource& source,
    std::span<const records::LegacyRow> rows, std::size_t fontIdSlot, std::size_t sizeEffectsSlot)
{
    if (!target.noteFont) return;
    reportStaffField(
        reporting, key, source, rows, "noteFont.fontId", fontIdSlot, target.noteFont->fontId);
    reportStaffField(reporting, key, source, rows, "noteFont.fontSize", sizeEffectsSlot,
        target.noteFont->fontSize);
    reportStaffField(
        reporting, key, source, rows, "noteFont.bold", sizeEffectsSlot, target.noteFont->bold);
    reportStaffField(
        reporting, key, source, rows, "noteFont.italic", sizeEffectsSlot, target.noteFont->italic);
    reportStaffField(reporting, key, source, rows, "noteFont.underline", sizeEffectsSlot,
        target.noteFont->underline);
    reportStaffField(reporting, key, source, rows, "noteFont.strikeout", sizeEffectsSlot,
        target.noteFont->strikeout);
    reportStaffField(reporting, key, source, rows, "noteFont.absolute", sizeEffectsSlot,
        target.noteFont->absolute);
    reportStaffField(
        reporting, key, source, rows, "noteFont.hidden", sizeEffectsSlot, target.noteFont->hidden);
}

template <typename Reporting>
void reportStaffTransposition(Reporting& reporting, const StaffTarget& target,
    const typename Reporting::InstanceKey& key, const RecordFamilySource& source,
    std::span<const records::LegacyRow> rows, std::size_t slot)
{
    if (!target.transposition) return;
    reportStaffField(reporting, key, source, rows, "transposition.setToClef", slot,
        target.transposition->setToClef);
    reportStaffField(reporting, key, source, rows, "transposition.noSimplifyKey", slot,
        target.transposition->noSimplifyKey);
    if (target.transposition->keysig) {
        reportStaffField(reporting, key, source, rows, "transposition.keysig.interval", slot,
            target.transposition->keysig->interval);
        reportStaffField(reporting, key, source, rows, "transposition.keysig.adjust", slot,
            target.transposition->keysig->adjust);
    }
    if (target.transposition->chromatic) {
        reportStaffField(reporting, key, source, rows, "transposition.chromatic.alteration", slot,
            target.transposition->chromatic->alteration);
        reportStaffField(reporting, key, source, rows, "transposition.chromatic.diatonic", slot,
            target.transposition->chromatic->diatonic);
    }
}

template <typename Reporting>
void reportMappedStaff(Reporting& reporting, const StaffTarget& target,
    const RecordFamilySource& source, std::span<const records::LegacyRow> rows,
    std::span<const std::uint8_t> payload, ByteOrder byteOrder,
    StaffLegacySemantics legacySemantics)
{
    const auto key =
        reporting.template instanceKey<StaffTarget>(target.getSourcePartId(), target.getCmper());
    reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
#define STAFF_MAPPED(member, slot)                                                                 \
    reportStaffField(reporting, key, source, rows, #member, slot, target.member)
    STAFF_MAPPED(botBarlineOffset, botBarlineSlot);
    STAFF_MAPPED(capoPos, tablaturePositionsSlot);
    STAFF_MAPPED(lowestFret, tablaturePositionsSlot);
    STAFF_MAPPED(notationStyle, primaryFlagsSlot);
    STAFF_MAPPED(useNoteShapes, primaryFlagsSlot);
    STAFF_MAPPED(useNoteFont, primaryFlagsSlot);
    STAFF_MAPPED(defaultClef, clefsSlot);
    STAFF_MAPPED(transposedClef, clefsSlot);
    reportStaffField(reporting, key, source, rows, "staffLines", bottomStaffLinesSlot,
        target.staffLines.value_or(0));
    reportStaffField(reporting, key, source, rows, "customStaff", topStaffLinesSlot,
        target.customStaff ? target.customStaff->size() : 0);
    STAFF_MAPPED(floatKeys, displayFlagsSlot);
    STAFF_MAPPED(floatTime, displayFlagsSlot);
    STAFF_MAPPED(blineBreak, displayFlagsSlot);
    STAFF_MAPPED(rbarBreak, displayFlagsSlot);
    STAFF_MAPPED(showNameInParts, primaryFlagsSlot);
    STAFF_MAPPED(showNoteColors, primaryFlagsSlot);
    STAFF_MAPPED(hideNameInScore, displayFlagsSlot);
    if (legacySemantics.hasAlternateNotationProperties) {
        STAFF_MAPPED(altNotation, altFlagsSlot);
        STAFF_MAPPED(altLayer, altFlagsSlot);
        STAFF_MAPPED(altHideArtics, altFlagsSlot);
        STAFF_MAPPED(altHideLyrics, altFlagsSlot);
        STAFF_MAPPED(altHideSmartShapes, altFlagsSlot);
        STAFF_MAPPED(altRhythmStemsUp, altFlagsSlot);
        STAFF_MAPPED(altSlashDots, altFlagsSlot);
        STAFF_MAPPED(altHideExpressions, altFlagsSlot);
        STAFF_MAPPED(altHideOtherNotes, altFlagsSlot);
        STAFF_MAPPED(altHideOtherArtics, altFlagsSlot);
    } else {
        reportStaffAlternateNotationLegacyBehavior(reporting, target, key);
    }
    STAFF_MAPPED(hideRepeatBottomDot, primaryFlagsSlot);
    STAFF_MAPPED(flatBeams, primaryFlagsSlot);
    STAFF_MAPPED(
        hideFretboards, legacySemantics.hideNoteAttachedItems ? altFlagsSlot : primaryFlagsSlot);
    STAFF_MAPPED(blankMeasure, primaryFlagsSlot);
    STAFF_MAPPED(hideRepeatTopDot, primaryFlagsSlot);
    STAFF_MAPPED(hideLyrics, primaryFlagsSlot);
    STAFF_MAPPED(noOptimize, primaryFlagsSlot);
    STAFF_MAPPED(topBarlineOffset, topBarlineSlot);
    STAFF_MAPPED(hideMeasNums, displayFlagsSlot);
    STAFF_MAPPED(hideRepeats, displayFlagsSlot);
    STAFF_MAPPED(hideBarlines, displayFlagsSlot);
    STAFF_MAPPED(hideRptBars, displayFlagsSlot);
    STAFF_MAPPED(hideKeySigs, displayFlagsSlot);
    STAFF_MAPPED(hideTimeSigs, displayFlagsSlot);
    STAFF_MAPPED(hideClefs, displayFlagsSlot);
    if (hasFinale2012StaffLayout(payload)) STAFF_MAPPED(hideStaffLines, displayFlagsSlot);
    STAFF_MAPPED(
        hideChords, legacySemantics.hideNoteAttachedItems ? altFlagsSlot : displayFlagsSlot);
    STAFF_MAPPED(noKey, displayFlagsSlot);
    STAFF_MAPPED(dwRestOffset, doubleWholeWholeRestSlot);
    STAFF_MAPPED(wRestOffset, doubleWholeWholeRestSlot);
    STAFF_MAPPED(hRestOffset, halfOtherRestSlot);
    STAFF_MAPPED(otherRestOffset, halfOtherRestSlot);
    STAFF_MAPPED(stemReversal, stemReversalSlot);
    STAFF_MAPPED(fullNameTextId, fullNameSlot);
    STAFF_MAPPED(abbrvNameTextId, abbreviatedNameSlot);
    reportStaffFont(reporting, target, key, source, rows, noteFontIdSlot, noteFontSizeEffectsSlot);
    reportStaffTransposition(reporting, target, key, source, rows, transpositionSlot);
    const auto wordCount = payload.size() / 2;
    if (wordCount > staffBaseWords) {
        STAFF_MAPPED(botRepeatDotOff, repeatDotOffsetsSlot);
        STAFF_MAPPED(topRepeatDotOff, repeatDotOffsetsSlot);
    }
    const auto longOrder =
        byteOrder == ByteOrder::BigEndian ? LongWordOrder::HighFirst : LongWordOrder::LowFirst;
    if (wordCount >= lineSpaceSlot + 2) {
        reportStaffField(reporting, key, source, rows, "lineSpace", lineSpaceSlot,
            payloadLong(payload, lineSpaceSlot * 2, byteOrder, longOrder));
    }
    if (wordCount >= verticalTabOffsetSlot + 2) {
        reportStaffField(reporting, key, source, rows, "vertTabNumOff", verticalTabOffsetSlot,
            payloadLong(payload, verticalTabOffsetSlot * 2, byteOrder, longOrder));
    }
    if (wordCount > extendedFlagsSlot) {
        STAFF_MAPPED(showTabClefAllSys, extendedFlagsSlot);
        STAFF_MAPPED(hideRests, extendedFlagsSlot);
        STAFF_MAPPED(hideTies, extendedFlagsSlot);
        STAFF_MAPPED(hideDots, extendedFlagsSlot);
        STAFF_MAPPED(hideStems, extendedFlagsSlot);
        STAFF_MAPPED(stemDirection, extendedFlagsSlot);
        STAFF_MAPPED(stemStartFromStaff, extendedFlagsSlot);
        STAFF_MAPPED(stemsFixedEnd, extendedFlagsSlot);
        STAFF_MAPPED(useTabLetters, extendedFlagsSlot);
        STAFF_MAPPED(hideBeams, extendedFlagsSlot);
        STAFF_MAPPED(breakTabLinesAtNotes, extendedFlagsSlot);
        STAFF_MAPPED(stemsFixedStart, extendedFlagsSlot);
        STAFF_MAPPED(hideTuplets, extendedFlagsSlot);
        STAFF_MAPPED(hideMode, extendedFlagsSlot);
    }
    if (wordCount > fretInstrumentSlot) {
        STAFF_MAPPED(fretInstId, fretInstrumentSlot);
    }
    const auto reportLong = [&](const char* member, std::size_t slot) {
        if (wordCount >= slot + 2) {
            reportStaffField(reporting, key, source, rows, member, slot,
                payloadLong(payload, slot * 2, byteOrder, longOrder));
        }
    };
    reportLong("horzStemOffUp", horizontalStemUpSlot);
    reportLong("horzStemOffDown", horizontalStemDownSlot);
    reportLong("vertStemStartOffUp", verticalStemStartUpSlot);
    reportLong("vertStemStartOffDown", verticalStemStartDownSlot);
    reportLong("vertStemEndOffUp", verticalStemEndUpSlot);
    reportLong("vertStemEndOffDown", verticalStemEndDownSlot);
    if (hasSecondAlternateNotationFlags(payload)) {
        STAFF_MAPPED(altHideOtherLyrics, alternateFlags2Slot);
        STAFF_MAPPED(altHideOtherSmartShapes, alternateFlags2Slot);
        STAFF_MAPPED(altHideOtherExpressions, alternateFlags2Slot);
    } else if (legacySemantics.usesAggregateAlternateNotationItems) {
        STAFF_MAPPED(altHideOtherLyrics, altFlagsSlot);
        STAFF_MAPPED(altHideOtherSmartShapes, altFlagsSlot);
        STAFF_MAPPED(altHideOtherExpressions, altFlagsSlot);
    }
    if (hasFinale2012StaffLayout(payload)) {
        STAFF_MAPPED(autoNumbering, autoNumberingSlot);
        STAFF_MAPPED(useAutoNumbering, autoNumberingSlot);
        reportStaffField(
            reporting, key, source, rows, "instUuid", instrumentUuidSlot, instrumentUuidBytes);
    } else {
        reporting.report().setField(key, "autoNumbering",
            {Reporting::Origin::LegacyBehavior, 0, 0,
                static_cast<std::int64_t>(target.autoNumbering)});
        reporting.report().setField(key, "useAutoNumbering",
            {Reporting::Origin::LegacyBehavior, 0, 0, target.useAutoNumbering});
    }
#undef STAFF_MAPPED
}

template <typename Reporting>
void reportRemainingStaffFields(Reporting& reporting, const StaffTarget& target)
{
    const auto key =
        reporting.template instanceKey<StaffTarget>(target.getSourcePartId(), target.getCmper());
#define STAFF_UNMAPPED(member)                                                                     \
    reporting.unmappedField(key, #member, static_cast<std::int64_t>(target.member))
    STAFF_UNMAPPED(useNoteShapes);
    STAFF_UNMAPPED(useNoteFont);
    STAFF_UNMAPPED(notationStyle);
    STAFF_UNMAPPED(defaultClef);
    STAFF_UNMAPPED(transposedClef);
    STAFF_UNMAPPED(lineSpace);
    STAFF_UNMAPPED(capoPos);
    STAFF_UNMAPPED(lowestFret);
    STAFF_UNMAPPED(floatKeys);
    STAFF_UNMAPPED(floatTime);
    STAFF_UNMAPPED(blineBreak);
    STAFF_UNMAPPED(rbarBreak);
    STAFF_UNMAPPED(hasStyles);
    STAFF_UNMAPPED(showNameInParts);
    STAFF_UNMAPPED(showNoteColors);
    STAFF_UNMAPPED(hideNameInScore);
    STAFF_UNMAPPED(botBarlineOffset);
    STAFF_UNMAPPED(altNotation);
    STAFF_UNMAPPED(altLayer);
    STAFF_UNMAPPED(altHideArtics);
    STAFF_UNMAPPED(altHideLyrics);
    STAFF_UNMAPPED(altHideSmartShapes);
    STAFF_UNMAPPED(altRhythmStemsUp);
    STAFF_UNMAPPED(altSlashDots);
    STAFF_UNMAPPED(altHideOtherNotes);
    STAFF_UNMAPPED(altHideOtherArtics);
    STAFF_UNMAPPED(altHideExpressions);
    STAFF_UNMAPPED(altHideOtherLyrics);
    STAFF_UNMAPPED(altHideOtherSmartShapes);
    STAFF_UNMAPPED(altHideOtherExpressions);
    STAFF_UNMAPPED(hideRepeatBottomDot);
    STAFF_UNMAPPED(flatBeams);
    STAFF_UNMAPPED(hideFretboards);
    STAFF_UNMAPPED(blankMeasure);
    STAFF_UNMAPPED(hideRepeatTopDot);
    STAFF_UNMAPPED(hideLyrics);
    STAFF_UNMAPPED(noOptimize);
    STAFF_UNMAPPED(topBarlineOffset);
    STAFF_UNMAPPED(hideMeasNums);
    STAFF_UNMAPPED(hideRepeats);
    STAFF_UNMAPPED(hideBarlines);
    STAFF_UNMAPPED(hideRptBars);
    STAFF_UNMAPPED(hideKeySigs);
    STAFF_UNMAPPED(hideTimeSigs);
    STAFF_UNMAPPED(hideClefs);
    STAFF_UNMAPPED(hideStaffLines);
    STAFF_UNMAPPED(hideChords);
    STAFF_UNMAPPED(noKey);
    STAFF_UNMAPPED(dwRestOffset);
    STAFF_UNMAPPED(wRestOffset);
    STAFF_UNMAPPED(hRestOffset);
    STAFF_UNMAPPED(otherRestOffset);
    STAFF_UNMAPPED(hideRests);
    STAFF_UNMAPPED(hideTies);
    STAFF_UNMAPPED(hideDots);
    STAFF_UNMAPPED(stemReversal);
    STAFF_UNMAPPED(fullNameTextId);
    STAFF_UNMAPPED(abbrvNameTextId);
    STAFF_UNMAPPED(botRepeatDotOff);
    STAFF_UNMAPPED(topRepeatDotOff);
    STAFF_UNMAPPED(vertTabNumOff);
    STAFF_UNMAPPED(showTabClefAllSys);
    STAFF_UNMAPPED(useTabLetters);
    STAFF_UNMAPPED(breakTabLinesAtNotes);
    STAFF_UNMAPPED(hideTuplets);
    STAFF_UNMAPPED(fretInstId);
    STAFF_UNMAPPED(hideStems);
    STAFF_UNMAPPED(stemDirection);
    STAFF_UNMAPPED(hideBeams);
    STAFF_UNMAPPED(stemStartFromStaff);
    STAFF_UNMAPPED(stemsFixedEnd);
    STAFF_UNMAPPED(stemsFixedStart);
    STAFF_UNMAPPED(horzStemOffUp);
    STAFF_UNMAPPED(horzStemOffDown);
    STAFF_UNMAPPED(vertStemStartOffUp);
    STAFF_UNMAPPED(vertStemStartOffDown);
    STAFF_UNMAPPED(vertStemEndOffUp);
    STAFF_UNMAPPED(vertStemEndOffDown);
    STAFF_UNMAPPED(hideMode);
#undef STAFF_UNMAPPED
    reporting.report().setField(key, "redisplayLayerAccis",
        {Reporting::Origin::LegacyBehavior, 0, 0, target.redisplayLayerAccis});
    reporting.report().setField(key, "hideTimeSigsInParts",
        {Reporting::Origin::LegacyBehavior, 0, 0, target.hideTimeSigsInParts});
    reporting.report().setField(key, "hideKeySigsShowAccis",
        {Reporting::Origin::LegacyBehavior, 0, 0, target.hideKeySigsShowAccis});
    reporting.unmappedField(key, "staffLines", target.staffLines.value_or(0));
    reporting.unmappedField(
        key, "customStaff", target.customStaff ? target.customStaff->size() : 0);
    reporting.unmappedField(key, "instUuid", 0);
    const auto font = target.noteFont;
    reporting.unmappedField(key, "noteFont.fontId", font ? font->fontId : 0);
    reporting.unmappedField(key, "noteFont.fontSize", font ? font->fontSize : 0);
    reporting.unmappedField(key, "noteFont.bold", font ? font->bold : 0);
    reporting.unmappedField(key, "noteFont.italic", font ? font->italic : 0);
    reporting.unmappedField(key, "noteFont.underline", font ? font->underline : 0);
    reporting.unmappedField(key, "noteFont.strikeout", font ? font->strikeout : 0);
    reporting.unmappedField(key, "noteFont.absolute", font ? font->absolute : 0);
    reporting.unmappedField(key, "noteFont.hidden", font ? font->hidden : 0);
    const auto transposition = target.transposition;
    reporting.unmappedField(
        key, "transposition.setToClef", transposition ? transposition->setToClef : 0);
    reporting.unmappedField(
        key, "transposition.noSimplifyKey", transposition ? transposition->noSimplifyKey : 0);
    reporting.unmappedField(key, "transposition.keysig.interval",
        transposition && transposition->keysig ? transposition->keysig->interval : 0);
    reporting.unmappedField(key, "transposition.keysig.adjust",
        transposition && transposition->keysig ? transposition->keysig->adjust : 0);
    reporting.unmappedField(key, "transposition.chromatic.alteration",
        transposition && transposition->chromatic ? transposition->chromatic->alteration : 0);
    reporting.unmappedField(key, "transposition.chromatic.diatonic",
        transposition && transposition->chromatic ? transposition->chromatic->diatonic : 0);
}

template <typename Reporting>
void reportStaffFinale27Defaults(
    Reporting& reporting, const StaffTarget& target, StaffFinale27Defaults selected)
{
    const auto key =
        reporting.template instanceKey<StaffTarget>(target.getSourcePartId(), target.getCmper());
    if (selected.lineSpace) {
        reporting.report().setField(
            key, "lineSpace", {Reporting::Origin::Finale27Default, 0, 0, target.lineSpace});
    }
    if (selected.restOffsets) {
        reporting.report().setField(
            key, "dwRestOffset", {Reporting::Origin::Finale27Default, 0, 0, target.dwRestOffset});
        reporting.report().setField(
            key, "wRestOffset", {Reporting::Origin::Finale27Default, 0, 0, target.wRestOffset});
        reporting.report().setField(
            key, "hRestOffset", {Reporting::Origin::Finale27Default, 0, 0, target.hRestOffset});
        reporting.report().setField(key, "otherRestOffset",
            {Reporting::Origin::Finale27Default, 0, 0, target.otherRestOffset});
    }
    if (selected.stemReversal) {
        reporting.report().setField(
            key, "stemReversal", {Reporting::Origin::Finale27Default, 0, 0, target.stemReversal});
    }
    if (selected.repeatDotOffsets) {
        reporting.report().setField(key, "botRepeatDotOff",
            {Reporting::Origin::Finale27Default, 0, 0, target.botRepeatDotOff});
        reporting.report().setField(key, "topRepeatDotOff",
            {Reporting::Origin::Finale27Default, 0, 0, target.topRepeatDotOff});
    }
}

StaffLegacySemantics decodeStaffBase(const std::shared_ptr<StaffTarget>& targetPtr,
    std::span<const std::uint8_t> payload, const SourceProfile& profile,
    StaffLegacySemantics legacySemantics, musx::factory::ConstructionContext& construction)
{
    auto& target = *targetPtr;
    const auto byteOrder = profile.byteOrder;
    const auto word = [&](std::size_t slot) { return payloadWord(payload, slot * 2, byteOrder); };
    const auto signedWord = [&](std::size_t slot) { return static_cast<std::int16_t>(word(slot)); };
    target.botBarlineOffset = signedWord(botBarlineSlot);
    const auto tablaturePositions = word(tablaturePositionsSlot);
    target.capoPos = tablaturePositions & 0xffU;
    target.lowestFret = tablaturePositions >> 8U;
    const auto alt = word(altFlagsSlot);
    if (legacySemantics.hasAlternateNotationProperties) {
        const auto altNotation = alt & altNotationMask;
        if (altNotation <=
            static_cast<std::uint16_t>(StaffTarget::AlternateNotation::BlankWithRests)) {
            target.altNotation = static_cast<StaffTarget::AlternateNotation>(altNotation);
        }
        target.altLayer = (alt & altLayerMask) >> 4U;
        target.altRhythmStemsUp = alt & altRhythmStemsUpMask;
        target.altSlashDots = alt & altSlashDotsMask;
        if (legacySemantics.usesAggregateAlternateNotationItems) {
            legacySemantics.hideNoteAttachedItems = !(alt & altShowArticulationsMask);
            const auto hideOtherAttachedItems = !(alt & altShowSmartShapesMask);
            target.altHideArtics = legacySemantics.hideNoteAttachedItems;
            target.altHideLyrics = legacySemantics.hideNoteAttachedItems;
            target.altHideSmartShapes = legacySemantics.hideNoteAttachedItems;
            target.altHideExpressions = legacySemantics.hideNoteAttachedItems;
            target.altHideOtherNotes = !(alt & altShowLyricsMask);
            target.altHideOtherArtics = hideOtherAttachedItems;
            target.altHideOtherLyrics = hideOtherAttachedItems;
            target.altHideOtherSmartShapes = hideOtherAttachedItems;
            target.altHideOtherExpressions = hideOtherAttachedItems;
        } else {
            target.altHideArtics = !(alt & altShowArticulationsMask);
            target.altHideLyrics = !(alt & altShowLyricsMask);
            target.altHideSmartShapes = !(alt & altShowSmartShapesMask);
            target.altHideOtherNotes = !(alt & altShowOtherNotesMask);
            target.altHideOtherArtics = !(alt & altShowOtherArticulationsMask);
            target.altHideExpressions = !(alt & altShowExpressionsMask);
        }
    }

    target.noteFont = std::make_shared<musx::dom::FontInfo>(target.getDocument());
    target.noteFont->fontId = construction.assignFontId(word(noteFontIdSlot));
    const auto sizeEffects = word(noteFontSizeEffectsSlot);
    target.noteFont->fontSize = sizeEffects >> 8U;
    target.noteFont->setEnigmaStyles(sizeEffects & 0xffU);

    const auto primary = word(primaryFlagsSlot);
    target.notationStyle = notationStyle(primary);
    target.hideRepeatBottomDot = primary & hideRepeatBottomDotMask;
    target.flatBeams = primary & flatBeamsMask;
    target.hideFretboards = legacySemantics.hideNoteAttachedItems || (primary & hideFretboardsMask);
    target.blankMeasure = primary & blankMeasureMask;
    target.useNoteShapes = primary & useNoteShapesMask;
    target.hideRepeatTopDot = primary & hideRepeatTopDotMask;
    target.useNoteFont = primary & useNoteFontMask;
    target.hideLyrics = primary & hideLyricsMask;
    target.showNameInParts = primary & showNameInPartsMask;
    target.showNoteColors = primary & showNoteColorsMask;
    target.noOptimize = primary & noOptimizeMask;
    const auto clefs = word(clefsSlot);
    target.defaultClef = clefs & 0xffU;
    target.transposedClef = clefs >> 8U;
    const auto topLines = word(topStaffLinesSlot);
    const auto bottomLines = word(bottomStaffLinesSlot);
    if (topLines & 1U) {
        target.staffLines.reset();
        target.customStaff = customStaffLines(topLines, bottomLines);
    } else {
        target.staffLines = bottomLines;
        target.customStaff.reset();
    }
    target.topBarlineOffset = signedWord(topBarlineSlot);

    decodeStaffTransposition(targetPtr, word(transpositionSlot));

    const auto display = word(displayFlagsSlot);
    target.floatKeys = display & floatKeysMask;
    target.floatTime = display & floatTimeMask;
    target.blineBreak = display & breakBarlinesMask;
    target.rbarBreak = display & breakRepeatBarlinesMask;
    target.hideMeasNums = display & hideMeasureNumbersMask;
    target.hideRepeats = display & hideRepeatsMask;
    target.hideNameInScore = display & hideNameMask;
    target.hideBarlines = display & hideBarlinesMask;
    target.hideRptBars = display & hideRepeatBarsMask;
    target.hideKeySigs = display & hideKeySignaturesMask;
    target.hideTimeSigs = display & hideTimeSignaturesMask;
    target.hideTimeSigsInParts = target.hideTimeSigs;
    target.hideClefs = display & hideClefsMask;
    if (hasFinale2012StaffLayout(payload)) target.hideStaffLines = display & hideStaffLinesMask;
    target.hideChords = legacySemantics.hideNoteAttachedItems || (display & hideChordsMask);
    target.noKey = display & noKeyMask;

    const auto doubleWholeWhole = word(doubleWholeWholeRestSlot);
    target.dwRestOffset = signedByte(doubleWholeWhole, false);
    target.wRestOffset = signedByte(doubleWholeWhole, true);
    const auto halfOther = word(halfOtherRestSlot);
    target.hRestOffset = signedByte(halfOther, false);
    target.otherRestOffset = signedByte(halfOther, true);
    target.stemReversal = signedWord(stemReversalSlot);
    target.fullNameTextId = word(fullNameSlot);
    target.abbrvNameTextId = word(abbreviatedNameSlot);

    const auto wordCount = payload.size() / 2;
    if (wordCount > staffBaseWords) {
        const auto repeatDots = word(repeatDotOffsetsSlot);
        target.botRepeatDotOff = signedByte(repeatDots, false);
        target.topRepeatDotOff = signedByte(repeatDots, true);
    }
    const auto longOrder =
        byteOrder == ByteOrder::BigEndian ? LongWordOrder::HighFirst : LongWordOrder::LowFirst;
    if (wordCount >= lineSpaceSlot + 2) {
        target.lineSpace = payloadLong(payload, lineSpaceSlot * 2, byteOrder, longOrder) / 64;
    }
    if (wordCount >= verticalTabOffsetSlot + 2) {
        target.vertTabNumOff =
            payloadLong(payload, verticalTabOffsetSlot * 2, byteOrder, longOrder);
    }
    if (wordCount > extendedFlagsSlot) {
        const auto extended = word(extendedFlagsSlot);
        target.showTabClefAllSys = !(extended & clefFirstMeasureOnlyMask);
        target.hideRests = !(extended & showRestsMask);
        target.hideTies = !(extended & showTiesMask);
        target.hideDots = !(extended & showDotsMask);
        target.hideStems = !(extended & showStemsMask);
        target.stemDirection = stemDirection(extended);
        target.stemStartFromStaff = extended & stemStartFromStaffMask;
        target.stemsFixedEnd = extended & stemsFixedEndMask;
        target.useTabLetters = extended & useTabLettersMask;
        target.hideBeams = !(extended & showBeamsMask);
        target.breakTabLinesAtNotes = extended & breakTabLinesMask;
        target.stemsFixedStart = extended & stemsFixedStartMask;
        target.hideTuplets = !(extended & showTupletsMask);
        target.hideMode = hideMode(extended);
    }
    if (wordCount > fretInstrumentSlot) {
        target.fretInstId = word(fretInstrumentSlot);
    }
    const auto decodeLong = [&](std::size_t slot) {
        return payloadLong(payload, slot * 2, byteOrder, longOrder);
    };
    if (wordCount >= horizontalStemUpSlot + 2) {
        target.horzStemOffUp = decodeLong(horizontalStemUpSlot);
    }
    if (wordCount >= horizontalStemDownSlot + 2) {
        target.horzStemOffDown = decodeLong(horizontalStemDownSlot);
    }
    if (wordCount >= verticalStemStartUpSlot + 2) {
        target.vertStemStartOffUp = decodeLong(verticalStemStartUpSlot);
    }
    if (wordCount >= verticalStemStartDownSlot + 2) {
        target.vertStemStartOffDown = decodeLong(verticalStemStartDownSlot);
    }
    if (wordCount >= verticalStemEndUpSlot + 2) {
        target.vertStemEndOffUp = decodeLong(verticalStemEndUpSlot);
    }
    if (wordCount >= verticalStemEndDownSlot + 2) {
        target.vertStemEndOffDown = decodeLong(verticalStemEndDownSlot);
    }
    if (hasSecondAlternateNotationFlags(payload)) {
        const auto alt2 = word(alternateFlags2Slot);
        target.altHideOtherLyrics = !(alt2 & altShowOtherLyricsMask);
        target.altHideOtherSmartShapes = !(alt2 & altShowOtherSmartShapesMask);
        target.altHideOtherExpressions = !(alt2 & altShowOtherExpressionsMask);
    }
    if (hasFinale2012StaffLayout(payload)) {
        const auto autoNumbering = word(autoNumberingSlot);
        const auto autoNumberingStyle = autoNumbering & autoNumberingStyleMask;
        if (autoNumberingStyle <=
            static_cast<std::uint16_t>(StaffTarget::AutoNumberingStyle::ArabicPrefix)) {
            target.autoNumbering = static_cast<StaffTarget::AutoNumberingStyle>(autoNumberingStyle);
        }
        target.useAutoNumbering = autoNumbering & autoNumberingEnabledMask;
        target.instUuid = instrumentUuid(payload);
    }
    return legacySemantics;
}

} // namespace

void importStaff(const ImportContext& context)
{
    const auto selected = selectRecordFamilySource(
        context, context.index.getOthers(), context.index.getClassOthers(), staffTag, staffClass);
    if (!selected) return;
    const auto& source = *selected;
    for (const auto [partId, staffId] : recordKeys(source)) {
        const auto rows = source.pool->getArray(source.identity, staffId, 0, partId);
        if (rows.empty()) continue;
        const auto payload = collectRecordPayload(source, rows);
        auto target =
            createOthersRecordTarget<StaffTarget>(context.document, source, rows.front(), staffId);
        if (!target) continue;

        target->autoNumbering = static_cast<StaffTarget::AutoNumberingStyle>(0);
        target->useAutoNumbering = false;
        target->instUuid = std::string(musx::dom::uuid::Unknown);
        const auto hasAlternateNotationProperties =
            sourceHasStaffAlternateNotationProperties(context.profile);
        StaffLegacySemantics legacySemantics{
            .hasAlternateNotationProperties = hasAlternateNotationProperties,
            .usesAggregateAlternateNotationItems =
                hasAlternateNotationProperties && !hasSecondAlternateNotationFlags(payload)};
        if (!legacySemantics.hasAlternateNotationProperties)
            applyStaffAlternateNotationLegacyBehavior(*target);
        const auto hasStoredInstrumentUuid = hasFinale2012StaffLayout(payload);
        const auto selectedDefaults = selectStaffFinale27Defaults(context.profile, payload);

        if (context.profile.epoch == FormatEpoch::CodaBanner) {
            if (payload.size() < staffCodaWords * 2) continue;
            const auto codaWord = [&](std::size_t slot) {
                return payloadWord(payload, slot * 2, context.profile.byteOrder);
            };
            target->defaultClef = codaWord(codaDefaultClefSlot);
            decodeStaffTransposition(target, codaWord(codaTranspositionSlot));
            const auto display = codaWord(codaDisplayFlagsSlot);
            target->hideTimeSigs = display & hideTimeSignaturesMask;
            target->hideTimeSigsInParts = target->hideTimeSigs;

            const RecordFamilySource codaAttributesSource{
                &context.index.getOthers(), codaStaffAttributesTag, false, false};
            const auto codaAttributesRows =
                context.index.getOthers().getArray(codaStaffAttributesTag, staffId, 0, partId);
            const auto codaAttributes =
                collectRecordPayload(codaAttributesSource, codaAttributesRows);
            const auto hasCodaAttributes = codaAttributes.size() >= staffCodaWords * 2;
            if (hasCodaAttributes) {
                const auto attributeWord = [&](std::size_t slot) {
                    return payloadWord(codaAttributes, slot * 2, context.profile.byteOrder);
                };
                const auto customMarker = attributeWord(codaCustomStaffMarkerSlot);
                if (customMarker & 1U) {
                    target->staffLines.reset();
                    target->customStaff =
                        customStaffLines(customMarker, codaWord(codaStaffLinesSlot));
                } else {
                    target->staffLines = 5;
                }
                target->noteFont = std::make_shared<musx::dom::FontInfo>(target->getDocument());
                target->noteFont->fontId =
                    context.construction.assignFontId(attributeWord(codaNoteFontIdSlot));
                const auto sizeEffects = attributeWord(codaNoteFontSizeEffectsSlot);
                target->noteFont->fontSize = sizeEffects >> 8U;
                target->noteFont->setEnigmaStyles(sizeEffects & 0xffU);
                const auto primary = attributeWord(codaPrimaryFlagsSlot);
                // The low byte is the legacy Base Key control, which has no modern
                // Staff member; the signed high byte is the tablature-number offset.
                target->vertTabNumOff =
                    static_cast<std::int16_t>(attributeWord(codaVerticalTabOffsetSlot) & 0xff00U);
                target->fretInstId = primary >> 8U;
                target->notationStyle = notationStyle(primary);
                target->useNoteFont = primary & codaUseNoteFontMask;
                if (target->notationStyle == StaffTarget::NotationStyle::Tablature) {
                    applyStaffBooleanLegacyBehavior(*target, staffCodaTabBooleanDefaults);
                }
            } else {
                // Without the optional attributes row, this layout has neither
                // editable staff lines nor a Staff-local notehead-font tuple.
                target->staffLines = 5;
                target->noteFont = std::make_shared<musx::dom::FontInfo>(target->getDocument());
                target->noteFont->fontSize = staffFinale27NoteheadFontSize(context);
            }
            const auto hasOneStaffLine = target->customStaff && target->customStaff->size() == 1 &&
                                         target->customStaff->at(0) == 13;
            if (hasOneStaffLine) {
                target->botBarlineOffset = -48;
                target->topBarlineOffset = 48;
            }
            withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                const auto key = reporting.template instanceKey<StaffTarget>(partId, staffId);
                reporting.report().setInstanceOrigin(key, Reporting::Origin::LegacyMus);
                reportStaffField(reporting, key, source, rows, "defaultClef", codaDefaultClefSlot,
                    target->defaultClef);
                reportStaffTransposition(
                    reporting, *target, key, source, rows, codaTranspositionSlot);
                reportStaffField(reporting, key, source, rows, "hideTimeSigs", codaDisplayFlagsSlot,
                    target->hideTimeSigs);
                if (hasCodaAttributes && target->customStaff) {
                    reportStaffField(reporting, key, codaAttributesSource, codaAttributesRows,
                        "staffLines", codaCustomStaffMarkerSlot, 0);
                    reportStaffField(reporting, key, source, rows, "customStaff",
                        codaStaffLinesSlot, target->customStaff->size());
                } else {
                    reporting.report().setField(
                        key, "staffLines", {Reporting::Origin::LegacyBehavior, 0, 0, 5});
                }
                if (hasOneStaffLine) {
                    reporting.report().setField(key, "botBarlineOffset",
                        {Reporting::Origin::LegacyBehavior, 0, 0, target->botBarlineOffset});
                    reporting.report().setField(key, "topBarlineOffset",
                        {Reporting::Origin::LegacyBehavior, 0, 0, target->topBarlineOffset});
                }
                if (hasCodaAttributes) {
                    reportStaffField(reporting, key, codaAttributesSource, codaAttributesRows,
                        "notationStyle", codaPrimaryFlagsSlot, target->notationStyle);
                    reportStaffField(reporting, key, codaAttributesSource, codaAttributesRows,
                        "useNoteFont", codaPrimaryFlagsSlot, target->useNoteFont);
                    reportStaffField(reporting, key, codaAttributesSource, codaAttributesRows,
                        "vertTabNumOff", codaVerticalTabOffsetSlot, target->vertTabNumOff);
                    reportStaffField(reporting, key, codaAttributesSource, codaAttributesRows,
                        "fretInstId", codaPrimaryFlagsSlot, target->fretInstId);
                    reportStaffFont(reporting, *target, key, codaAttributesSource,
                        codaAttributesRows, codaNoteFontIdSlot, codaNoteFontSizeEffectsSlot);
                    if (target->notationStyle == StaffTarget::NotationStyle::Tablature) {
                        reportStaffBooleanLegacyBehavior(
                            reporting, *target, key, staffCodaTabBooleanDefaults);
                    }
                } else {
                    reporting.report().setField(key, "noteFont.fontSize",
                        {Reporting::Origin::Finale27Default, 0, 0, target->noteFont->fontSize});
                }
                reporting.report().setField(key, "autoNumbering",
                    {Reporting::Origin::LegacyBehavior, 0, 0,
                        static_cast<std::int64_t>(target->autoNumbering)});
                reporting.report().setField(key, "useAutoNumbering",
                    {Reporting::Origin::LegacyBehavior, 0, 0, target->useAutoNumbering});
                reportStaffAlternateNotationLegacyBehavior(reporting, *target, key);
            });
            context.pending.checks.push_back([&context, target] {
                text::EnigmaFontResolutionCache fontResolutionCache;
                synthesizeCodaStaffName(context, *target, codaStaffFullNameTag,
                    musx::dom::options::FontOptions::FontType::StaffNames,
                    &StaffTarget::fullNameTextId, fontResolutionCache);
                synthesizeCodaStaffName(context, *target, codaStaffAbbreviatedNameTag,
                    musx::dom::options::FontOptions::FontType::AbbrvStaffNames,
                    &StaffTarget::abbrvNameTextId, fontResolutionCache);
            });
        } else {
            if (payload.size() < staffBaseBytes) {
                context.report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                    "Staff " + std::to_string(staffId) + " is shorter than its base layout."});
                continue;
            }
            legacySemantics = decodeStaffBase(
                target, payload, context.profile, legacySemantics, context.construction);
            withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
                reportMappedStaff(reporting, *target, source, rows, payload,
                    context.profile.byteOrder, legacySemantics);
            });
        }
        if (selectedDefaults.lineSpace || selectedDefaults.restOffsets ||
            selectedDefaults.stemReversal || selectedDefaults.repeatDotOffsets) {
            applyStaffFinale27Defaults(*target, finale27StaffDefaults(context), selectedDefaults);
        }
        withReporting(context.report, [&]<typename Reporting>(Reporting& reporting) {
            reportStaffFinale27Defaults(reporting, *target, selectedDefaults);
            if (!hasStoredInstrumentUuid) {
                const auto key = reporting.template instanceKey<StaffTarget>(partId, staffId);
                reporting.report().setField(
                    key, "instUuid", {Reporting::Origin::LegacyBehavior, 0, 0, 0});
            }
            reportRemainingStaffFields(reporting, *target);
        });
        context.document->getOthers()->add(StaffTarget::XmlNodeName, std::move(target));
    }
}

} // namespace others
} // namespace finale_mus_reader
