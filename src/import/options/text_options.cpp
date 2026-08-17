// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options.h"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <iterator>
#include <memory>
#include <string>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace options {
namespace {

using TextTarget = musx::dom::options::TextOptions;

// This class is spread across five numeric globals. Two of them are as old as the format and
// three arrive with Finale 97; see @ref storesTextLayoutOptions for why the split needs no
// gate. The accidental symbol inserts are a direct block on a sixth selector, handled by
// @ref captureSymbolInserts rather than by the tables.
constexpr std::uint16_t dateSelector = 5;
constexpr std::uint16_t tabSelector = 13;
constexpr std::uint16_t metricsSelector = 81;
constexpr std::uint16_t layoutSelector = 82;
constexpr std::uint16_t alignSelector = 83;

// Word slots of selector 82. Slot 1 is the line-spacing units and is handled outside the
// tables, because it selects which musxdom member slot 0 populates rather than carrying a
// value of its own.
constexpr std::uint32_t lineSpacingValueSlot = 0;
constexpr std::uint32_t lineSpacingIsPercentSlot = 1;

/// @brief Finale orders its alignment lists first, opposite, centre.
/// @details musxdom's `TextJustify` and `VerticalAlignment` both put centre second instead,
/// so the two spellings agree everywhere except at 1 and 2, which are exchanged. Only these
/// two need it: `AlignJustify`, which `textHorzAlign` uses, already has Finale's order, and a
/// Finale 2001 document storing 2 against a converted `center` proves it passes through.
///
/// A Finale 2003 document settles `textJustify`: it stores 2 and Finale 27 converts it to
/// `center`, which would be 1 in musxdom's order. `tests/evidence/F2005/F2005-textvert-center`
/// settles `textVertAlign` the same way by moving `83` word 1 alone from 0 to 2 against a
/// converted `center`, and the earlier scalars fixtures read `bottom` from a stored 1.
[[nodiscard]] constexpr std::int64_t exchangeCenterAndOpposite(std::int64_t value)
{
    if (value == 1) {
        return 2;
    }
    if (value == 2) {
        return 1;
    }
    return value;
}

/// @brief Whether this source stores selector 82 at all.
/// @details Selectors 81, 82 and 83 arrive together with Finale 97, which is the same boundary
/// the multimeasure-rest defaults find for selector 83. No document of Finale 2.6, 3.0, 3.2,
/// 3.5 or 3.7 in the reference corpus carries any of the three and every Finale 97 and later
/// one carries all three.
///
/// **The pre-97 epochs are covered and the exclusion is intended.** Reading presence rather
/// than dating the file keeps the boundary out of a version range whose lower end would have
/// to be guessed somewhere between 3.7 and 97, and it means a document from before the
/// selectors existed recovers nothing from them and correctly reports the eleven fields as
/// synthesized Finale 27 defaults. That is the fallback strategy working: the era had no such
/// settings to store, and the Coda-era report that its UI exposes only tab spacing and date
/// format agrees. Those two fields are on selectors 5 and 13, which every era carries, so they
/// are recovered from the same words in a Finale 1.0.0 document as in a Finale 2012 one.
///
/// Only the line-spacing pair consults this. The other tables need no gate of their own: an
/// absent record simply resolves no value, and the field is then reported as the synthesized
/// default it already holds. The pair is different because its two tables must stay
/// complementary -- musxdom rejects a TextOptions with both members engaged or neither -- so
/// the absent case has to be named rather than left to fall through both.
bool storesTextLayoutOptions(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    if (profile.epoch == FormatEpoch::ZlibLegacy) {
        return index.getClassOthers().get(
                   numericGlobalClass(layoutSelector), GLOBALS_CMPER, 0, 0)
            != nullptr;
    }
    return readNumericGlobalWords(index, layoutSelector).present;
}

// Selectors 5 and 13, which every epoch carries. Both locations are the distilled framework's
// and both are corpus-confirmed on all 1,189 adjacent-exact companion pairs of the reference
// corpus, Coda-banner included. The controlled Finale 1.0.0 and 2.6.3 fixtures move word 5 of
// selector 5 to 1 and 2 and word 0 of selector 13 to 7, against companions reading `long`,
// `abbrev` and 7, which is what shows the earliest era uses the same words as the latest.
//
// `dateFormat` needs no translation: the framework's DATEFORMAT_SHORT, _LONG and _MACLONG are
// 0, 1 and 2, and musxdom's DateFormat has the same three in the same order.
const FieldMapping textStampFields[] = {
    MUS_WORD(TextTarget, "05", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 4, showTimeSeconds),
    MUS_WORD(TextTarget, "05", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 5, dateFormat),
    MUS_WORD(TextTarget, "13", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 0, tabSpaces),
};

// Selector 81, three 32-bit values in the framework's MACFOURBYTE order: two 16-bit words with
// the high word first, each word in the container's byte order. That is one rule for both byte
// orders, and the controlled fixtures show it in each -- a big-endian Finale 2005 file stores
// -6 as (-1, -6) and a little-endian Finale 2012 file stores 42 as (0, 42).
//
// All 1,108 companion-backed documents that carry the selector agree on all three.
const FieldMapping textMetricsFields[] = {
    MUS_LONG(TextTarget, "81", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 0,
        LongWordOrder::HighFirst, textTracking),
    MUS_LONG(TextTarget, "81", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 2,
        LongWordOrder::HighFirst, textBaselineShift),
    MUS_LONG(TextTarget, "81", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 4,
        LongWordOrder::HighFirst, textSuperscript),
};

// Selectors 82 and 83, less the two words that need more than a slot each: word 0 of 82, whose
// destination depends on word 1, and word 1 of 82 itself.
//
// Word 2 of selector 83 is deliberately unmapped. It is 0 in every pre-2007 document and 1 in
// every 2007-and-later one, and a controlled Finale 2012 text-options save clears it, so it is
// something that dialog owns -- but it is not any field of this class, and the multimeasure-
// rest note records the same word as set in 468 companion-backed documents whose conversions
// have no <autoUpdateMmRests/>. Those 468 are exactly the zlib-era documents with word 4
// clear, so the two observations are one fact. It stays **open**.
const FieldMapping textLayoutFields[] = {
    MUS_WORD(TextTarget, "82", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 2, textWordWrap),
    MUS_WORD(TextTarget, "82", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 3, textPageOffset),
    MUS_BITS_AS(TextTarget, "82", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 4,
        /*firstBit*/ 0, /*bitCount*/ 0, textJustify,
        static_cast<TextTarget::TextJustify>(exchangeCenterAndOpposite(value))),
    MUS_WORD(TextTarget, "82", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 5,
        textExpandSingleWord),
    MUS_WORD(TextTarget, "83", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 0, textHorzAlign),
    MUS_BITS_AS(TextTarget, "83", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 1,
        /*firstBit*/ 0, /*bitCount*/ 0, textVertAlign,
        static_cast<TextTarget::VerticalAlignment>(exchangeCenterAndOpposite(value))),
    MUS_WORD(TextTarget, "83", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 3, textIsEdgeAligned),
};

// Finale 2007 and later reach the same words through the shared numericGlobalClass rule,
// addressed by byte offset in the coalesced payload.
const FieldMapping classTextStampFields[] = {
    MUS_CLASS_WORD(TextTarget, numericGlobalClass(dateSelector), GLOBALS_CMPER,
        classWordOffset(4), showTimeSeconds),
    MUS_CLASS_WORD(TextTarget, numericGlobalClass(dateSelector), GLOBALS_CMPER,
        classWordOffset(5), dateFormat),
    MUS_CLASS_WORD(TextTarget, numericGlobalClass(tabSelector), GLOBALS_CMPER,
        classWordOffset(0), tabSpaces),
};

const FieldMapping classTextMetricsFields[] = {
    MUS_CLASS_LONG(TextTarget, numericGlobalClass(metricsSelector), GLOBALS_CMPER,
        classWordOffset(0), LongWordOrder::HighFirst, textTracking),
    MUS_CLASS_LONG(TextTarget, numericGlobalClass(metricsSelector), GLOBALS_CMPER,
        classWordOffset(2), LongWordOrder::HighFirst, textBaselineShift),
    MUS_CLASS_LONG(TextTarget, numericGlobalClass(metricsSelector), GLOBALS_CMPER,
        classWordOffset(4), LongWordOrder::HighFirst, textSuperscript),
};

const FieldMapping classTextLayoutFields[] = {
    MUS_CLASS_WORD(TextTarget, numericGlobalClass(layoutSelector), GLOBALS_CMPER,
        classWordOffset(2), textWordWrap),
    MUS_CLASS_WORD(TextTarget, numericGlobalClass(layoutSelector), GLOBALS_CMPER,
        classWordOffset(3), textPageOffset),
    MUS_CLASS_SELECTED_BITS_AS(TextTarget, numericGlobalClass(layoutSelector), GLOBALS_CMPER,
        classWordOffset(4), /*firstBit*/ 0, /*bitCount*/ 0, textJustify,
        static_cast<TextTarget::TextJustify>(exchangeCenterAndOpposite(value))),
    MUS_CLASS_WORD(TextTarget, numericGlobalClass(layoutSelector), GLOBALS_CMPER,
        classWordOffset(5), textExpandSingleWord),
    MUS_CLASS_WORD(TextTarget, numericGlobalClass(alignSelector), GLOBALS_CMPER,
        classWordOffset(0), textHorzAlign),
    MUS_CLASS_SELECTED_BITS_AS(TextTarget, numericGlobalClass(alignSelector), GLOBALS_CMPER,
        classWordOffset(1), /*firstBit*/ 0, /*bitCount*/ 0, textVertAlign,
        static_cast<TextTarget::VerticalAlignment>(exchangeCenterAndOpposite(value))),
    MUS_CLASS_WORD(TextTarget, numericGlobalClass(alignSelector), GLOBALS_CMPER,
        classWordOffset(3), textIsEdgeAligned),
};

constexpr const char* textReportPrefix = "options.textOptions";

// Every epoch whose records are 16-byte rows. The Coda-banner epoch is included deliberately:
// it carries selectors 5 and 13, and the presence test keeps it out of the other three.
constexpr EpochMask fixedRowTextEpochs = EpochMask::CodaBanner | EpochMask::FixedRow;

const MappingTable& textStampTable()
{
    static const MappingTable table{
        .reportPrefix = textReportPrefix,
        .epochs = fixedRowTextEpochs,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<TextTarget>,
        .fields = textStampFields,
        .fieldCount = std::size(textStampFields)};
    return table;
}

const MappingTable& textMetricsTable()
{
    static const MappingTable table{
        .reportPrefix = textReportPrefix,
        .epochs = fixedRowTextEpochs,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<TextTarget>,
        .fields = textMetricsFields,
        .fieldCount = std::size(textMetricsFields)};
    return table;
}

const MappingTable& textLayoutTable()
{
    static const MappingTable table{
        .reportPrefix = textReportPrefix,
        .epochs = fixedRowTextEpochs,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<TextTarget>,
        .fields = textLayoutFields,
        .fieldCount = std::size(textLayoutFields)};
    return table;
}

const MappingTable& classTextStampTable()
{
    static const MappingTable table{
        .reportPrefix = textReportPrefix,
        .epochs = EpochMask::Zlib,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<TextTarget>,
        .fields = classTextStampFields,
        .fieldCount = std::size(classTextStampFields)};
    return table;
}

const MappingTable& classTextMetricsTable()
{
    static const MappingTable table{
        .reportPrefix = textReportPrefix,
        .epochs = EpochMask::Zlib,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<TextTarget>,
        .fields = classTextMetricsFields,
        .fieldCount = std::size(classTextMetricsFields)};
    return table;
}

const MappingTable& classTextLayoutTable()
{
    static const MappingTable table{
        .reportPrefix = textReportPrefix,
        .epochs = EpochMask::Zlib,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<TextTarget>,
        .fields = classTextLayoutFields,
        .fieldCount = std::size(classTextLayoutFields)};
    return table;
}

/// @brief Whether selector 82 states its line spacing as a percent rather than a distance.
/// @details Word 1 selects which musxdom member word 0 belongs in: set means percent, clear
/// means an absolute distance. That is why the value needs two tables rather than one row --
/// the destination depends on a second word, not on the value read.
///
/// `tests/evidence/F2005/F2005-linespace-to-evpu` establishes it directly. It moves words 0
/// and 1 alone, 100 -> 72 and 1 -> 0, and its companion replaces
/// <textLineSpacingPercent>100</> with <textLineSpacingEvpu>72</> while keeping
/// <textExpandSingleWord/>. Finale 27 has no boolean of its own for the mode -- it writes one
/// spelling or the other and never both -- so this word is read to route the value and is
/// never itself reported as a recovered field. All 1,108 companion-backed documents that carry
/// the selector have it set, against companions reporting the percent spelling.
///
/// A source with no selector 82 answers false, which costs nothing: the tables that depend on
/// this one are gated on the same record's presence.
bool statesLineSpacingAsPercent(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    const auto encoding = profile.epoch == FormatEpoch::ZlibLegacy
        ? RecordEncoding::ClassRecord
        : RecordEncoding::FixedRow;
    SourceLocation source;
    source.identity = encoding == RecordEncoding::ClassRecord
        ? numericGlobalClass(layoutSelector)
        : records::packTag("82");
    source.selector = GLOBALS_CMPER;
    source.incidence = 0;
    source.wordSlot = encoding == RecordEncoding::ClassRecord
        ? classWordOffset(lineSpacingIsPercentSlot)
        : lineSpacingIsPercentSlot;
    const auto resolved
        = readSourceValue(index, encoding, GLOBALS_CMPER, source, profile.byteOrder);
    return resolved && resolved->value != 0;
}

bool statesLineSpacingAsEvpu(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    return storesTextLayoutOptions(index, profile)
        && !statesLineSpacingAsPercent(index, profile);
}

// The two halves of selector 82 word 0. Exactly one table applies to any document that carries
// the selector, and musxdom's own integrity check rejects a TextOptions with both members
// engaged or neither, so the pair must stay complementary.
const FieldMapping lineSpacingPercentFields[] = {
    MUS_WORD(TextTarget, "82", GLOBALS_CMPER, /*incidence*/ 0, lineSpacingValueSlot,
        textLineSpacingPercent),
};

const FieldMapping lineSpacingEvpuFields[] = {
    MUS_WORD(TextTarget, "82", GLOBALS_CMPER, /*incidence*/ 0, lineSpacingValueSlot,
        textLineSpacingEvpu),
};

const FieldMapping classLineSpacingPercentFields[] = {
    MUS_CLASS_WORD(TextTarget, numericGlobalClass(layoutSelector), GLOBALS_CMPER,
        classWordOffset(lineSpacingValueSlot), textLineSpacingPercent),
};

const FieldMapping classLineSpacingEvpuFields[] = {
    MUS_CLASS_WORD(TextTarget, numericGlobalClass(layoutSelector), GLOBALS_CMPER,
        classWordOffset(lineSpacingValueSlot), textLineSpacingEvpu),
};

const MappingTable& lineSpacingPercentTable()
{
    static const MappingTable table{
        .reportPrefix = textReportPrefix,
        .epochs = fixedRowTextEpochs,
        .applies = &statesLineSpacingAsPercent,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<TextTarget>,
        .fields = lineSpacingPercentFields,
        .fieldCount = std::size(lineSpacingPercentFields)};
    return table;
}

const MappingTable& lineSpacingEvpuTable()
{
    static const MappingTable table{
        .reportPrefix = textReportPrefix,
        .epochs = fixedRowTextEpochs,
        .applies = &statesLineSpacingAsEvpu,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<TextTarget>,
        .fields = lineSpacingEvpuFields,
        .fieldCount = std::size(lineSpacingEvpuFields)};
    return table;
}

const MappingTable& classLineSpacingPercentTable()
{
    static const MappingTable table{
        .reportPrefix = textReportPrefix,
        .epochs = EpochMask::Zlib,
        .applies = &statesLineSpacingAsPercent,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<TextTarget>,
        .fields = classLineSpacingPercentFields,
        .fieldCount = std::size(classLineSpacingPercentFields)};
    return table;
}

const MappingTable& classLineSpacingEvpuTable()
{
    static const MappingTable table{
        .reportPrefix = textReportPrefix,
        .epochs = EpochMask::Zlib,
        .applies = &statesLineSpacingAsEvpu,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<TextTarget>,
        .fields = classLineSpacingEvpuFields,
        .fieldCount = std::size(classLineSpacingEvpuFields)};
    return table;
}

/// @brief Clears the baseline's line spacing before either table engages the right member.
/// @details The pinned baseline seeds the percent spelling. A source that states the absolute
/// one would otherwise leave that percent engaged beside it, which is exactly the shape
/// musxdom's `TextOptions::integrityCheck` rejects. Runs only when the source has the record
/// to replace it with, so a document from before Finale 97 keeps the baseline value.
void clearSeededLineSpacing(const records::LegacyRecordIndex& index,
    const SourceProfile& profile, const musx::dom::DocumentPtr& document)
{
    if (!storesTextLayoutOptions(index, profile)) {
        return;
    }
    const auto pooled = document->getOptions()->get<TextTarget>();
    if (!pooled) {
        return;
    }
    // Pool instances are handed out const. Overlaying legacy values is the one reason this
    // library writes to them.
    auto target = std::const_pointer_cast<TextTarget>(pooled);
    target->textLineSpacingPercent.reset();
    target->textLineSpacingEvpu.reset();
}

// ---------------------------------------------------------------------------------------
// Accidental symbol inserts
// ---------------------------------------------------------------------------------------

// The five inserts are a direct block rather than a field map: one element per accidental, in
// musxdom's own AccidentalInsertSymbolType order. The block is at selector 78 in the fixed-row
// epochs and at the class id the numericGlobalClass rule derives from it in the zlib epoch.
constexpr std::uint16_t insertSelector = 78;

// Element field offsets, which are the same in every layout. Only the width of the character
// changes, and with it the element size.
constexpr std::size_t insertTrackingBeforeOffset = 0;
constexpr std::size_t insertTrackingAfterOffset = 4;
constexpr std::size_t insertBaselineShiftOffset = 8;
constexpr std::size_t insertFontIdOffset = 10;
constexpr std::size_t insertFontSizeOffset = 12;
constexpr std::size_t insertFontEffectsOffset = 14;
constexpr std::size_t insertSymCharOffset = 16;

/// @brief Which physical shape of the insert block a source carries.
enum class InsertLayout
{
    /// @brief Finale 3.7-2000. 17 bytes, a one-byte character, little-endian throughout.
    EarlyByteChar,
    /// @brief Finale 2001-2010. 18 bytes, a two-byte character of which only the low byte counts.
    NarrowChar,
    /// @brief Finale 2012. 20 bytes, the character widened to a long for Unicode.
    WideChar,
};

[[nodiscard]] constexpr std::size_t elementSize(InsertLayout layout)
{
    switch (layout) {
    case InsertLayout::EarlyByteChar: return 17;
    case InsertLayout::NarrowChar: return 18;
    case InsertLayout::WideChar: return 20;
    }
    return 0;
}

// The order musxdom names them in, which is also the order they are stored in.
constexpr musx::dom::options::AccidentalInsertSymbolType insertOrder[] = {
    musx::dom::options::AccidentalInsertSymbolType::Sharp,
    musx::dom::options::AccidentalInsertSymbolType::Flat,
    musx::dom::options::AccidentalInsertSymbolType::Natural,
    musx::dom::options::AccidentalInsertSymbolType::DblSharp,
    musx::dom::options::AccidentalInsertSymbolType::DblFlat,
};

constexpr std::size_t insertCount = std::size(insertOrder);

const char* const insertNames[insertCount] = {
    "sharp", "flat", "natural", "dblSharp", "dblFlat"};

/// @brief The insert block's payload bytes, in the order its own struct is laid out.
struct InsertBlock
{
    bool present{};
    std::vector<std::uint8_t> bytes;
    InsertLayout layout = InsertLayout::NarrowChar;
    std::size_t blockOffset{};
    std::size_t decodedOffset{};
};

/// @brief Serializes a fixed-row word stream back to the bytes the file holds.
/// @details The row decoder hands out words rather than bytes, and this block is a byte
/// structure with an odd-sized element, so the bytes have to be reassembled to address it.
void appendWords(std::vector<std::uint8_t>& bytes, const std::vector<std::int16_t>& words,
    ByteOrder order)
{
    for (const auto word : words) {
        const auto raw = static_cast<std::uint16_t>(word);
        if (order == ByteOrder::BigEndian) {
            bytes.push_back(static_cast<std::uint8_t>(raw >> 8U));
            bytes.push_back(static_cast<std::uint8_t>(raw));
        } else {
            bytes.push_back(static_cast<std::uint8_t>(raw));
            bytes.push_back(static_cast<std::uint8_t>(raw >> 8U));
        }
    }
}

/// @brief Locates the insert block and decides which layout the source carries.
/// @details The layout follows from the epoch and, inside the zlib epoch, from the payload's
/// own length, so no version gate is needed anywhere.
///
/// **The Coda-banner epoch carries no such block and the omission is intended.** No document
/// of Finale 1.x-2.6 in any survey has selector 78, the era's UI is reported to expose no
/// accidental-insert settings, and Finale 27 synthesizes the pre-2001 defaults when converting
/// one. Finale 3.0-3.5 is the same case for a different reason: the record has not appeared
/// yet. Both keep the pinned baseline's five inserts, which is what the fallback strategy is
/// for. That rests on 61 documents of the reference corpus, only eight of them Finale 3.0-3.2,
/// so exactly where between 3.5 and 3.7 the block appears is thin and the installs survey
/// would firm it up.
///
/// **Finale 3.7-2000 is a byte structure read little-endian**, whatever the container says.
/// Its element is 17 bytes because the character is a single byte, and an odd stride cannot be
/// addressed by 16-bit words at all: the fields land at alternating parities. Reassembling the
/// row words little-endian recovers the structure on a big-endian file and is a no-op on a
/// little-endian one, so one rule serves both. Every observed document of the era is
/// big-endian, so whether the rule is "opposite to the container" or "always little-endian"
/// cannot be told apart; they agree on every file this reader can be given, and a Windows
/// Finale 3.x-2000 document would be needed to separate them.
///
/// **Finale 27 mis-converts that era, and this reader deliberately disagrees with it.** Finale
/// 27 uses the right 17-byte stride but reads the fields as though the element were the later
/// 18-byte one, reporting the sharp insert's tracking as 2293760 -- the bytes 00 23 00 00 read
/// as a big-endian long -- and its character as 50, which is the first byte of the next
/// element. Read as this reader reads it, the same records give 35, 50, 0, 40, 60 and
/// characters 35, 98, 110, 220, 186: what every other era stores, on all 179 companion-backed
/// documents of the era and all thirteen tracked fixtures. A companion comparison will
/// therefore show this era disagreeing on nearly every field, and that is the intended result.
/// Eight later documents -- six Finale 2012 and two Finale 2009 -- store the mis-converted
/// values permanently, evidently from an old file re-saved in a later Finale; this reader
/// reproduces Finale 27 exactly on those, because there the bytes really do say that.
InsertBlock readInsertBlock(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    InsertBlock result;
    if (profile.epoch == FormatEpoch::ZlibLegacy) {
        const auto* row = index.getClassOthers().get(
            numericGlobalClass(insertSelector), GLOBALS_CMPER, 0, 0);
        if (!row) {
            return result;
        }
        const auto payload = index.getClassOthers().payloadOf(*row);
        result.present = true;
        result.bytes.assign(payload.begin(), payload.end());
        // The payload's own length states the layout. Finale 2007-2010 write 96 bytes and
        // Finale 2012 writes 108, and no document of the reference corpus carries any other
        // length: 293 on the narrow side and 248 on the wide one.
        result.layout = result.bytes.size() >= insertCount * elementSize(InsertLayout::WideChar)
            ? InsertLayout::WideChar
            : InsertLayout::NarrowChar;
        result.blockOffset = row->blockOffset;
        result.decodedOffset = row->decodedOffset;
        return result;
    }
    const auto family = readNumericGlobalWords(index, insertSelector);
    if (!family.present) {
        return result;
    }
    result.present = true;
    result.layout = profile.epoch == FormatEpoch::DclLegacy ? InsertLayout::NarrowChar
                                                            : InsertLayout::EarlyByteChar;
    appendWords(result.bytes, family.words,
        result.layout == InsertLayout::EarlyByteChar ? ByteOrder::LittleEndian
                                                     : profile.byteOrder);
    result.blockOffset = family.blockOffset;
    result.decodedOffset = family.decodedOffset;
    return result;
}

/// @brief Reads one unsigned value of the given width from an element.
[[nodiscard]] std::uint32_t readInsertField(const InsertBlock& block, std::size_t elementBase,
    std::size_t offset, std::size_t width, ByteOrder order)
{
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < width; ++i) {
        const auto byte = static_cast<std::uint32_t>(block.bytes[elementBase + offset + i]);
        // The early layout is little-endian throughout; the later two follow the container,
        // except that a 32-bit value is two 16-bit words with the high word first.
        const bool littleEndian = block.layout == InsertLayout::EarlyByteChar
            || order == ByteOrder::LittleEndian;
        value |= byte << (8U * (littleEndian ? i : width - 1 - i));
    }
    return value;
}

/// @brief Reads a 32-bit field stored as two 16-bit words, high word first.
/// @details This is the framework's MACFOURBYTE order, and it is one rule for both byte
/// orders: a big-endian Finale 2005 file stores 1000 as 00 00 03 e8 and a little-endian Finale
/// 2012 file stores it as 00 00 e8 03. The early layout is a plain little-endian long instead,
/// which reads the same way once its bytes have been reassembled.
[[nodiscard]] std::int32_t readInsertLong(const InsertBlock& block, std::size_t elementBase,
    std::size_t offset, ByteOrder order)
{
    if (block.layout == InsertLayout::EarlyByteChar) {
        return static_cast<std::int32_t>(readInsertField(block, elementBase, offset, 4, order));
    }
    const auto high = readInsertField(block, elementBase, offset, 2, order);
    const auto low = readInsertField(block, elementBase, offset + 2, 2, order);
    return static_cast<std::int32_t>((high << 16U) | low);
}

/// @brief Reads the symbol character, whose width is what separates the three layouts.
/// @details Before Finale 2012 the character is a byte. The 18-byte layout gives it a whole
/// word and some sources sign-extend it: four Finale 2006 fixtures store ff dc for 220 and
/// ff ba for 186 while a fifth of the same version stores 00 dc, and only characters above 127
/// are affected. Finale 27 keeps the low byte, so this does too. Finale 2012 widened the field
/// to a long for Unicode and must not be masked; it reads as a plain little-endian long, and
/// no observed value exceeds 0xFFFF, so whether it is that or a low-word-first pair cannot be
/// distinguished by anything in evidence.
[[nodiscard]] char32_t readInsertChar(const InsertBlock& block, std::size_t elementBase,
    ByteOrder order)
{
    switch (block.layout) {
    case InsertLayout::EarlyByteChar:
        return static_cast<char32_t>(
            readInsertField(block, elementBase, insertSymCharOffset, 1, order));
    case InsertLayout::NarrowChar:
        return static_cast<char32_t>(
            readInsertField(block, elementBase, insertSymCharOffset, 2, order) & 0xFFU);
    case InsertLayout::WideChar:
        break;
    }
    const auto low = readInsertField(block, elementBase, insertSymCharOffset, 2, order);
    const auto high = readInsertField(block, elementBase, insertSymCharOffset + 2, 2, order);
    return static_cast<char32_t>((high << 16U) | low);
}

void reportInsertField(ImportReport& report, const char* insertName, const char* member,
    ValueOrigin origin, std::int64_t rawValue, const InsertBlock& block)
{
    FieldInfo info;
    info.target = std::string(textReportPrefix) + ".symbolInserts[" + insertName + "]." + member;
    info.origin = origin;
    info.rawValue = rawValue;
    if (origin == ValueOrigin::LegacyMus) {
        info.blockOffset = block.blockOffset;
        info.decodedOffset = block.decodedOffset;
    }
    report.fields.push_back(std::move(info));
}

/// @brief Rebuilds the five accidental inserts from the source's own block.
/// @details Every element is replaced rather than overlaid, because the block states all five
/// together: a source that carries it has its own values for each, and leaving a baseline
/// element in place beside four recovered ones would mix two documents' settings.
///
/// The font comparator is the source's own, and font definitions are recovered before any
/// options class, so it needs no remapping. A nonzero comparator the source does not define is
/// kept exactly as stored and noted at Info, the same policy the multimeasure-rest H-bar shape
/// uses: substituting the baseline's font would name a different document's typeface, and
/// hundreds of ordinary corpus documents name resources their own file never carries. The note
/// asks the document whether it defines the comparator rather than testing the comparator
/// against zero: what zero means is musxdom's business, and a document that defines it has
/// nothing to report.
bool captureSymbolInserts(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document, ImportReport& report)
{
    const auto block = readInsertBlock(index, profile);
    if (!block.present) {
        return false;
    }
    const auto pooled = document->getOptions()->get<TextTarget>();
    if (!pooled) {
        return false;
    }
    const auto stride = elementSize(block.layout);
    if (block.bytes.size() < insertCount * stride) {
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
            "The text options symbol-insert block holds " + std::to_string(block.bytes.size())
                + " bytes, too few for five elements; the Finale 27 defaults are kept."});
        return false;
    }
    // Pool instances are handed out const. Overlaying legacy values is the one reason this
    // library writes to them.
    auto target = std::const_pointer_cast<TextTarget>(pooled);

    for (std::size_t ordinal = 0; ordinal < insertCount; ++ordinal) {
        const auto base = ordinal * stride;
        const auto* name = insertNames[ordinal];
        auto insert = std::make_shared<TextTarget::InsertSymbolInfo>(pooled);
        insert->trackingBefore
            = readInsertLong(block, base, insertTrackingBeforeOffset, profile.byteOrder);
        insert->trackingAfter
            = readInsertLong(block, base, insertTrackingAfterOffset, profile.byteOrder);
        insert->baselineShiftPerc = static_cast<std::int16_t>(
            readInsertField(block, base, insertBaselineShiftOffset, 2, profile.byteOrder));
        insert->symChar = readInsertChar(block, base, profile.byteOrder);

        // The font is the ordinary Enigma tuple: comparator, size, effects bitmask. musxdom
        // owns the bitmask, and it drops the same two bits Finale 27 drops -- a stored 56
        // becomes strikeout alone in both -- so the mask is handed to it whole rather than
        // decoded here. The size is a percent of the preceding font size in an Enigma string,
        // which is what the second constructor argument states.
        const auto fontId = static_cast<musx::dom::Cmper>(
            readInsertField(block, base, insertFontIdOffset, 2, profile.byteOrder));
        const auto fontSize = static_cast<std::int16_t>(
            readInsertField(block, base, insertFontSizeOffset, 2, profile.byteOrder));
        const auto effects = static_cast<std::uint16_t>(
            readInsertField(block, base, insertFontEffectsOffset, 2, profile.byteOrder));
        auto font = std::make_shared<musx::dom::FontInfo>(document, /*sizeIsPercent*/ true);
        font->fontId = fontId;
        font->fontSize = fontSize;
        font->setEnigmaStyles(effects);
        insert->symFont = std::move(font);

        if (!document->getOthers()->get<musx::dom::others::FontDefinition>(
                musx::dom::SCORE_PARTID, fontId)) {
            report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                "The text options " + std::string(name) + " insert names font definition "
                    + std::to_string(fontId)
                    + ", which this document does not define; it is kept as stored."});
        }

        target->symbolInserts[insertOrder[ordinal]] = std::move(insert);

        reportInsertField(report, name, "trackingBefore", ValueOrigin::LegacyMus,
            target->symbolInserts[insertOrder[ordinal]]->trackingBefore, block);
        reportInsertField(report, name, "trackingAfter", ValueOrigin::LegacyMus,
            target->symbolInserts[insertOrder[ordinal]]->trackingAfter, block);
        reportInsertField(report, name, "baselineShiftPerc", ValueOrigin::LegacyMus,
            target->symbolInserts[insertOrder[ordinal]]->baselineShiftPerc, block);
        reportInsertField(report, name, "symFont.fontId", ValueOrigin::LegacyMus, fontId, block);
        reportInsertField(report, name, "symFont.fontSize", ValueOrigin::LegacyMus, fontSize, block);
        reportInsertField(report, name, "symFont.effects", ValueOrigin::LegacyMus, effects, block);
        reportInsertField(report, name, "symChar", ValueOrigin::LegacyMus,
            static_cast<std::int64_t>(target->symbolInserts[insertOrder[ordinal]]->symChar),
            block);
    }
    return true;
}

/// @brief Reports the baseline's own inserts where the source carries no block of its own.
/// @details The options pool is seeded from the pinned baseline, so a document with no
/// selector 78 already holds five inserts before this reader touches them -- the Coda-banner
/// era and Finale 3.0-3.5, which store none. Nothing needs copying; what needs doing is
/// saying so, and repairing the one thing seeding cannot get right.
///
/// **A seeded font comparator belongs to the baseline's numbering, not to this document's.**
/// The two documents number their font definitions independently, so a comparator carried
/// across unchanged would name whatever font happens to sit at that number here. musxdom owns
/// the rule for fixing it -- match by normalized name, treat zero as the default music font,
/// and add a definition only when no name matches -- and `importFontDefinitionInto` is its one
/// implementation, the same one `FontOptions` calls. It is called for every comparator
/// including zero: zero is the default-music-font sentinel, and the helper both returns it
/// unchanged and guarantees a definition exists at zero in this document, which is exactly the
/// part a caller that skipped the call for zero would leave undone.
void reportSeededSymbolInserts(const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument, ImportReport& report)
{
    const auto pooled = document->getOptions()->get<TextTarget>();
    if (!pooled) {
        return;
    }
    auto target = std::const_pointer_cast<TextTarget>(pooled);

    for (std::size_t ordinal = 0; ordinal < insertCount; ++ordinal) {
        const auto type = insertOrder[ordinal];
        const auto found = target->symbolInserts.find(type);
        if (found == target->symbolInserts.end() || !found->second) {
            continue;
        }
        auto& insert = *found->second;
        const auto* name = insertNames[ordinal];

        musx::dom::Cmper fontId = 0;
        if (insert.symFont) {
            const auto referenceFont
                = referenceDocument->getOthers()->get<musx::dom::others::FontDefinition>(
                    musx::dom::SCORE_PARTID, insert.symFont->fontId);
            if (!referenceFont) {
                report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
                    "The Finale 27 " + std::string(name)
                        + " insert names a font definition the baseline does not carry;"
                          " substituted font id 0."});
            } else if (const auto resolved
                = musx::dom::importFontDefinitionInto(document, referenceFont)) {
                fontId = *resolved;
            } else {
                report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
                    "No free font comparator remained for Finale 27 font \""
                        + referenceFont->name + "\"; substituted font id 0."});
            }
            insert.symFont->fontId = fontId;
        }

        const InsertBlock absent;
        reportInsertField(report, name, "trackingBefore", ValueOrigin::Finale27Default,
            insert.trackingBefore, absent);
        reportInsertField(report, name, "trackingAfter", ValueOrigin::Finale27Default,
            insert.trackingAfter, absent);
        reportInsertField(report, name, "baselineShiftPerc", ValueOrigin::Finale27Default,
            insert.baselineShiftPerc, absent);
        reportInsertField(report, name, "symFont.fontId", ValueOrigin::Finale27Default,
            fontId, absent);
        reportInsertField(report, name, "symFont.fontSize", ValueOrigin::Finale27Default,
            insert.symFont ? insert.symFont->fontSize : 0, absent);
        reportInsertField(report, name, "symFont.effects", ValueOrigin::Finale27Default,
            insert.symFont ? insert.symFont->getEnigmaStyles() : 0, absent);
        reportInsertField(report, name, "symChar", ValueOrigin::Finale27Default,
            static_cast<std::int64_t>(insert.symChar), absent);
    }
}

} // namespace

void importTextOptions(const ImportContext& context)
{
    clearSeededLineSpacing(context.index, context.profile, context.document);
    // The options pool is already seeded from the pinned baseline, so the inserts exist
    // either way. A source that carries the block replaces all five together; one that does
    // not keeps the baseline's, which then need their font comparators translated into this
    // document's numbering and reporting as the synthesized defaults they are.
    const bool recovered = captureSymbolInserts(
        context.index, context.profile, context.document, context.report);
    applyMappingTables({&textStampTable(), &textMetricsTable(), &textLayoutTable(),
                           &lineSpacingPercentTable(), &lineSpacingEvpuTable(),
                           &classTextStampTable(), &classTextMetricsTable(),
                           &classTextLayoutTable(), &classLineSpacingPercentTable(),
                           &classLineSpacingEvpuTable()},
        context.index, context.profile, context.document, context.report);

    if (!recovered) {
        reportSeededSymbolInserts(
            context.document, context.referenceDocument, context.report);
    }
}

} // namespace options
} // namespace finale_mus_reader
