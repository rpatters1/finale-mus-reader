// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace options {
namespace {

using LyricTarget = musx::dom::options::LyricOptions;
using LyricAlignJustify = musx::dom::AlignJustify;
using LyricSyllableStyle = LyricTarget::SyllablePosStyle;
using LyricSyllableType = LyricTarget::SyllablePosStyleType;
using LyricConnectIndex = LyricTarget::WordExtConnectIndex;
using LyricConnectStyle = LyricTarget::WordExtConnectStyle;
using LyricConnectType = LyricTarget::WordExtConnectStyleType;

// The lyric options are spread over six numeric globals rather than gathered into one record.
// Four of the six are named by the distilled framework study; selectors 55 and 57 are not in
// it at all and were located from the corpus. Each is an ordinary numeric global, so through
// Finale 2006 its identity is the two decimal characters of the selector and from Finale 2007
// it is the class id the shared numericGlobalClass rule derives.
constexpr std::uint16_t hyphenSeparationSelector = 15;
constexpr std::uint32_t hyphenSeparationSlot = 1;

constexpr std::uint16_t smartHyphenSelector = 35;
constexpr std::uint32_t smartHyphenSlot = 5;

constexpr std::uint16_t wordExtConnectSelector = 55;

constexpr std::uint16_t wordExtSelector = 57;
constexpr std::uint32_t smartHyphenStartSlot = 0;
constexpr std::uint32_t wordExtMinLengthSlot = 2;
constexpr std::uint32_t wordExtOffsetToNoteheadSlot = 3;
constexpr std::uint32_t edgePunctuationSlot = 4;
/// @brief The first word of selector 57's variable-length tail.
/// @details The record is six words when the document keeps the stock ignore list and grows to
/// hold a custom one, so the tail begins immediately after the six scalars.
constexpr std::size_t punctuationTailWord = 6;

constexpr std::uint16_t wordExtLineWidthSelector = 67;
constexpr std::uint32_t wordExtLineWidthSlot = 5;

constexpr std::uint16_t syllablePosSelector = 87;

/// @brief The word-extension line width of an era that has no record for it.
/// @details Every Coda-banner fixture converts to this, and the earliest documents that do
/// carry selector 67 store 118 rather than the pinned baseline's 115, so the baseline's value
/// is one Finale 27 document's setting rather than anything the older eras meant.
constexpr musx::dom::Efix unstatedWordExtLineWidth = 224;

/// @brief The first Enigma major version that stores the syllable edge punctuation setting.
/// @details "Ignore Syllable Edge Punctuation" arrives with Finale 2012. This is deliberately
/// its own constant rather than a shared "Finale 2012" one: several boundaries fall at major
/// 17 -- the Unicode codepoint widening and the font-ordinal renumbering among them -- and
/// nothing establishes a common cause that one constant would assert.
constexpr std::uint8_t firstEdgePunctuationMajorVersion = 17; // Finale 2012

constexpr const char* lyricReportPrefix = "options.lyricOptions";

// Every epoch whose records are 16-byte rows. Which of them can actually answer a given
// selector is left to that selector's own gate, because these six arrive at four different
// releases and none of the boundaries is the epoch boundary.
constexpr EpochMask fixedRowLyricEpochs = EpochMask::CodaBanner | EpochMask::FixedRow;

/// @brief Translates the legacy lyric alignment numbering into musxdom's @c AlignJustify.
/// @details The two orders disagree and neither is a rotation of the other: musxdom follows
/// Finale's general `Left, Right, Center` order from zero, while the lyric records number
/// their own list `1 = centre, 2 = left, 3 = right`. The numbering is the distilled
/// framework's, and the corpus confirms the two values it uses: every document that carries
/// selector 87 stores 1 where its Finale 27 companion says `center` and 2 where the companion
/// says `left`, for all four positions. **No surveyed document stores 3**, so `right` rests on
/// the framework alone; it is the only value of this field that is not corpus-confirmed.
///
/// Zero is not a member of the legacy list. A record storing it says nothing this reader can
/// translate, so the seeded default is kept rather than a fourth meaning invented for it.
std::optional<LyricAlignJustify> lyricAlignment(std::int16_t stored)
{
    switch (stored) {
    case 1:
        return LyricAlignJustify::Center;
    case 2:
        return LyricAlignJustify::Left;
    case 3:
        return LyricAlignJustify::Right;
    default:
        return std::nullopt;
    }
}

// Bit 15 of each position's flags word is "use this positioning", which musxdom spells as the
// style's own `on`. The framework names the bit as 0x8000 for the three optional positions and
// calls the fourth word a placeholder with no UI, and the corpus agrees on both counts: the
// Finale 2000 multilayer fixture is the one tracked document that clears the bit for the three
// optional positions, and its companion is the one whose conversion omits their <on/>.
constexpr std::uint8_t syllablePositionOnBit = 15;

// The four positions in the order the record stores them, which is musxdom's own enum order.
// The first is the fallback that always applies, so its stored flag decides nothing; musxdom
// documents `on` as meaningless for it.
constexpr LyricSyllableType syllablePositionOrder[] = {
    LyricSyllableType::Default,
    LyricSyllableType::WordExt,
    LyricSyllableType::First,
    LyricSyllableType::SystemStart,
};

constexpr const char* syllablePositionNames[] = {"default", "wordExt", "first", "systemStart"};

constexpr std::size_t syllablePositionWords = 3;

/// @brief The connection points a word extension can attach to, in the legacy numbering.
/// @details The legacy numbers are not this list's indices: they continue a wider entry
/// connection numbering that begins at note and stem attachments, and its lyric run starts at
/// 0x10. Two of the six are also in a different order from musxdom's enum, so the value cannot
/// be cast and has to be translated through this table.
///
/// The corpus fixes the whole mapping directly. The Finale 2006 fixture stores every one of
/// the six numbers with distinct offsets beside them, and its Finale 27 companion names each
/// connection point next to those same offsets. The private framework study corroborates the
/// starting number and the tail of the order from an unrelated place -- its smart-shape entry
/// connection enum reaches `lyric right bottom` at exactly 0x10 and ends with the two system
/// attachments -- with one difference: that enum has no entry for the dotted attachment, which
/// is why its `duration` sits one place earlier than the records put it. The records and the
/// companions agree with each other, so they govern.
constexpr LyricConnectIndex legacyConnectIndexOrder[] = {
    LyricConnectIndex::LyricRightBottom,
    LyricConnectIndex::HeadRightLyrBaseline,
    LyricConnectIndex::DotRightLyrBaseline,
    LyricConnectIndex::DurationLyrBaseline,
    LyricConnectIndex::SystemLeft,
    LyricConnectIndex::SystemRight,
};

/// @brief The legacy number of the first lyric connection point.
constexpr std::int16_t firstLegacyConnectIndex = 0x10;

// The nine connection styles in the order the record stores them, which is musxdom's own enum
// order. Every one is present in every document that carries the table.
constexpr LyricConnectType wordExtConnectOrder[] = {
    LyricConnectType::DefaultStart,
    LyricConnectType::DefaultEnd,
    LyricConnectType::SystemStart,
    LyricConnectType::SystemEnd,
    LyricConnectType::DottedEnd,
    LyricConnectType::DurationEnd,
    LyricConnectType::OneEntryEnd,
    LyricConnectType::ZeroLengthEnd,
    LyricConnectType::ZeroOffset,
};

constexpr const char* wordExtConnectNames[] = {"defaultStart", "defaultEnd", "systemStart",
    "systemEnd", "dottedEnd", "durationEnd", "oneEntryEnd", "zeroLengthEnd", "zeroOffset"};

// Each connection style is three words: the connection point, then the two offsets.
constexpr std::size_t wordExtConnectWords = 3;
constexpr std::size_t wordExtConnectCount = std::size(wordExtConnectOrder);

/// @brief Whether this source stores the word-extension connection table.
/// @details Selectors 55 and 57 arrive together with Finale 2004, inside the DCL epoch: no
/// Finale 2003 or earlier document in any survey carries either, and every Finale 2004 and
/// later one carries both. Their presence is what decides, rather than a version range, so a
/// document whose version cannot be recovered is read from what it actually carries.
///
/// The Coda-banner epoch is excluded outright, and it has to be: those documents do carry a
/// selector 55, and it is a different option that happens to reuse the number. The Finale 1.0.0
/// and 2.6.3 fixtures store values such as 16128 and 16448 in it, which are not connection
/// points at all, and one controlled Finale 1.0.0 stem-options edit moves its first two words.
/// Reading it as this table would fabricate nine connection styles out of another option's
/// bytes. The word count is a second, independent guard: that record is one six-word incidence
/// where this table needs twenty-seven words.
bool storesWordExtConnectTable(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    if (profile.epoch == FormatEpoch::CodaBanner) {
        return false;
    }
    const auto family = readGlobalWords(index, profile, wordExtConnectSelector);
    return family.present
        && family.words.size() >= wordExtConnectCount * wordExtConnectWords;
}

/// @brief Whether this source stores the Finale 2004 generation of smart-lyric settings.
/// @details Selector 57 is the marker for the whole group, including the smart-hyphen switch
/// that lives on selector 35. Selector 35 itself is present in every era, but its word 5 is
/// zero in every document before Finale 2004 and set in every one after, and every Finale 27
/// conversion of those earlier documents switches smart hyphens **on** regardless. That is what
/// an option arriving with a default of on looks like from before it existed, and it means the
/// word cannot be read on a document that predates it: doing so would turn smart hyphens off
/// for every Finale 2003-and-earlier file on the strength of a word that meant nothing yet.
///
/// The pinned baseline already switches smart hyphens on, which is exactly what those
/// conversions produce, so the earlier era needs no assertion of its own here -- only the gate.
bool storesSmartLyricOptions(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    return readGlobalWords(index, profile, wordExtSelector).present;
}

/// @brief Whether this source stores the word-extension line width.
/// @details Selector 67 is carried by every uncompressed, DCL and zlib document surveyed and by
/// no Coda-banner one. Its presence decides, because @ref captureLyricOptions asserts a value
/// when it is absent and the two must not be able to disagree.
bool storesWordExtLineWidth(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    return readGlobalWords(index, profile, wordExtLineWidthSelector).present;
}

/// @brief Whether this source's era stores the syllable-edge punctuation setting.
/// @details "Ignore Syllable Edge Punctuation" arrives with **Finale 2012**, and the reference
/// corpus settles both halves of that across all 1,189 of its adjacent-exact companion pairs.
/// Every document of every earlier release -- 454 through Finale 2003 that carry no selector 57
/// at all, and 487 from Finale 2004 through Finale 2010 that carry it with word 4 clear --
/// converts with <lyricUseEdgePunctuation/>, the reverse spelling meaning the punctuation is
/// *not* ignored. Only Finale 2012 documents vary, and there the word decides: all 198 with
/// word 4 clear convert without the element and all 50 with it set convert with it.
///
/// So the word exists before the setting does, and reading it on an earlier document would
/// switch edge punctuation off for every one of those 487. That is what the gate is for. The
/// pinned baseline omits the element too, so an earlier document left to the baseline would
/// claim a behavior its era did not have; the assertion is in @ref captureLyricOptions.
///
/// This is a version gate because nothing structural distinguishes the two cases: the record is
/// twelve bytes in Finale 2007 and in Finale 2012 alike, so its shape says nothing and only the
/// release does. The gate is framed inside the zlib epoch and bounded below, and a file whose
/// version cannot be recovered reads as the earlier behavior -- which is the era every other
/// epoch is in anyway, so the gate fails closed onto the answer that is right for all but one
/// release.
bool storesEdgePunctuationSetting(
    const records::LegacyRecordIndex&, const SourceProfile& profile)
{
    return profile.epoch == FormatEpoch::ZlibLegacy && profile.version
        && profile.version->major >= firstEdgePunctuationMajorVersion;
}

// The maximum hyphen separation, in the two epochs whose selector 15 states it. Its companion
// agreement is exact wherever it is read.
//
// The Coda-banner epoch is deliberately excluded. Those documents do carry selector 15, but
// word 1 is zero in every one of them while their Finale 27 companions all say 144, and the
// rest of that record differs from the later one as well. The field is not there to be read,
// and reading the zero would replace a correct default with a hyphen separation of nothing.
// The baseline's 144 is what the conversions produce, so the era needs no assertion either.
const FieldMapping lyricHyphenFields[] = {
    MUS_WORD(LyricTarget, "15", GLOBALS_CMPER, /*incidence*/ 0, hyphenSeparationSlot,
        maxHyphenSeparation),
};

// The word-extension line width, wherever selector 67 exists. This is the one scalar of the
// class that varies across the tracked fixtures, and it varies in all three epochs that carry
// it: 118 in the Finale 3.7.2, 97, 2000 and 2002 documents, 224 from Finale 2003, and 115 in
// one Finale 2012 document. Every companion agrees exactly.
const FieldMapping lyricLineWidthFields[] = {
    MUS_WORD(LyricTarget, "67", GLOBALS_CMPER, /*incidence*/ 0, wordExtLineWidthSlot,
        wordExtLineWidth),
};

// The Finale 2004 generation of smart-lyric settings. Every tracked document that carries them
// stores the same values, which are also the pinned baseline's, so these four agree with their
// companions everywhere and are discriminated by nothing: the corpus survey is what can still
// show them wrong.
//
// `smartHyphenStart` passes through as a number because musxdom's SmartHyphenStart is in the
// legacy order, `always, sometimes, never`. Only `sometimes` is witnessed, so the other two
// positions of that list rest on the enum's own documentation.
const FieldMapping smartLyricFields[] = {
    MUS_WORD(LyricTarget, "57", GLOBALS_CMPER, /*incidence*/ 0, smartHyphenStartSlot,
        smartHyphenStart),
    MUS_WORD(LyricTarget, "57", GLOBALS_CMPER, /*incidence*/ 0, wordExtMinLengthSlot,
        wordExtMinLength),
    MUS_WORD(LyricTarget, "57", GLOBALS_CMPER, /*incidence*/ 0, wordExtOffsetToNoteheadSlot,
        wordExtOffsetToNotehead),
    MUS_WORD(LyricTarget, "35", GLOBALS_CMPER, /*incidence*/ 0, smartHyphenSlot,
        useSmartHyphens),
};

// Finale 2007 and later. The same logical words, reached through the shared numericGlobalClass
// rule and addressed by byte offset in the coalesced payload. Both byte orders are exercised by
// the tracked fixtures: Finale 2007 is big-endian and Finale 2012 little-endian.
const FieldMapping classLyricHyphenFields[] = {
    MUS_CLASS_WORD(LyricTarget, numericGlobalClass(hyphenSeparationSelector), GLOBALS_CMPER,
        classWordOffset(hyphenSeparationSlot), maxHyphenSeparation),
};

const FieldMapping classLyricLineWidthFields[] = {
    MUS_CLASS_WORD(LyricTarget, numericGlobalClass(wordExtLineWidthSelector), GLOBALS_CMPER,
        classWordOffset(wordExtLineWidthSlot), wordExtLineWidth),
};

const FieldMapping classSmartLyricFields[] = {
    MUS_CLASS_WORD(LyricTarget, numericGlobalClass(wordExtSelector), GLOBALS_CMPER,
        classWordOffset(smartHyphenStartSlot), smartHyphenStart),
    MUS_CLASS_WORD(LyricTarget, numericGlobalClass(wordExtSelector), GLOBALS_CMPER,
        classWordOffset(wordExtMinLengthSlot), wordExtMinLength),
    MUS_CLASS_WORD(LyricTarget, numericGlobalClass(wordExtSelector), GLOBALS_CMPER,
        classWordOffset(wordExtOffsetToNoteheadSlot), wordExtOffsetToNotehead),
    MUS_CLASS_WORD(LyricTarget, numericGlobalClass(smartHyphenSelector), GLOBALS_CMPER,
        classWordOffset(smartHyphenSlot), useSmartHyphens),
};

const MappingTable& lyricHyphenTable()
{
    static const MappingTable table{
        .reportPrefix = lyricReportPrefix,
        .epochs = EpochMask::FixedRow,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<LyricTarget>,
        .fields = lyricHyphenFields,
        .fieldCount = std::size(lyricHyphenFields)};
    return table;
}

const MappingTable& lyricLineWidthTable()
{
    // The presence test alone decides this one, as it must: the capture pass asserts a value
    // when the same test says the record is absent, so a mask that could disagree with it
    // would leave a document with neither a read value nor an assertion.
    static const MappingTable table{
        .reportPrefix = lyricReportPrefix,
        .epochs = fixedRowLyricEpochs,
        .applies = &storesWordExtLineWidth,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<LyricTarget>,
        .fields = lyricLineWidthFields,
        .fieldCount = std::size(lyricLineWidthFields)};
    return table;
}

const MappingTable& smartLyricTable()
{
    static const MappingTable table{
        .reportPrefix = lyricReportPrefix,
        .epochs = fixedRowLyricEpochs,
        .applies = &storesSmartLyricOptions,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<LyricTarget>,
        .fields = smartLyricFields,
        .fieldCount = std::size(smartLyricFields)};
    return table;
}

const MappingTable& classLyricHyphenTable()
{
    static const MappingTable table{
        .reportPrefix = lyricReportPrefix,
        .epochs = EpochMask::Zlib,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<LyricTarget>,
        .fields = classLyricHyphenFields,
        .fieldCount = std::size(classLyricHyphenFields)};
    return table;
}

const MappingTable& classLyricLineWidthTable()
{
    static const MappingTable table{
        .reportPrefix = lyricReportPrefix,
        .epochs = EpochMask::Zlib,
        .applies = &storesWordExtLineWidth,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<LyricTarget>,
        .fields = classLyricLineWidthFields,
        .fieldCount = std::size(classLyricLineWidthFields)};
    return table;
}

// Finale 2012 and later only; see @ref storesEdgePunctuationSetting. musxdom spells the
// setting as the reverse of Finale's dialog, and so does the record: the word is set when
// punctuation is *used* rather than ignored, which is why it needs no inversion on the way in.
const FieldMapping classEdgePunctuationFields[] = {
    MUS_CLASS_WORD(LyricTarget, numericGlobalClass(wordExtSelector), GLOBALS_CMPER,
        classWordOffset(edgePunctuationSlot), lyricUseEdgePunctuation),
};

const MappingTable& classEdgePunctuationTable()
{
    // The gate alone decides this one, and it must: the capture pass asserts the opposite when
    // the same test says the era has no setting, so a mask that could disagree with it would
    // leave a document with neither a read value nor an assertion.
    static const MappingTable table{
        .reportPrefix = lyricReportPrefix,
        .epochs = EpochMask::Zlib,
        .applies = &storesEdgePunctuationSetting,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<LyricTarget>,
        .fields = classEdgePunctuationFields,
        .fieldCount = std::size(classEdgePunctuationFields)};
    return table;
}

const MappingTable& classSmartLyricTable()
{
    static const MappingTable table{
        .reportPrefix = lyricReportPrefix,
        .epochs = EpochMask::Zlib,
        .applies = &storesSmartLyricOptions,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<LyricTarget>,
        .fields = classSmartLyricFields,
        .fieldCount = std::size(classSmartLyricFields)};
    return table;
}

void reportLyricField(ImportReport& report, const std::string& member, ValueOrigin origin,
    std::int64_t rawValue, std::size_t blockOffset = 0, std::size_t decodedOffset = 0)
{
    FieldInfo info;
    info.target = std::string(lyricReportPrefix) + '.' + member;
    info.origin = origin;
    info.blockOffset = blockOffset;
    info.decodedOffset = decodedOffset;
    info.rawValue = rawValue;
    report.fields.push_back(std::move(info));
}

/// @brief The seeded style for one syllable position, created if the baseline lacked it.
std::shared_ptr<LyricSyllableStyle> syllableStyleFor(
    const std::shared_ptr<LyricTarget>& target, LyricSyllableType type)
{
    auto& slot = target->syllablePosStyles[type];
    if (!slot) {
        slot = std::make_shared<LyricSyllableStyle>();
    }
    return slot;
}

/// @brief The seeded style for one word-extension connection, created if the baseline lacked it.
std::shared_ptr<LyricConnectStyle> connectStyleFor(
    const std::shared_ptr<LyricTarget>& target, LyricConnectType type)
{
    auto& slot = target->wordExtConnectStyles[type];
    if (!slot) {
        slot = std::make_shared<LyricConnectStyle>();
    }
    return slot;
}

/// @brief Recovers the two collections and asserts what an era fixed rather than stored.
/// @details Both collections are fixed-length maps in musxdom rather than scalars, so neither
/// can be expressed as field mappings; both are read here from the numeric global's whole word
/// stream, which is one incidence family before Finale 2007 and one coalesced payload after.
/// Runs before the scalar tables, whose gates then leave the asserted fields alone.
void captureLyricOptions(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document, ImportReport& report)
{
    const auto pooled = document->getOptions()->get<LyricTarget>();
    if (!pooled) {
        return;
    }
    // Pool instances are handed out const. Overlaying legacy values is the one reason this
    // library writes to them.
    auto target = std::const_pointer_cast<LyricTarget>(pooled);

    // Selector 87: four syllable positions, three words each, spread over two fixed rows. The
    // whole group arrives with Finale 2000 -- no Coda-banner, 3.7.2 or Finale 97 document in
    // any survey carries the selector, and every later one does -- so its presence rather than
    // a version decides whether it can be read.
    const auto syllable = readGlobalWords(index, profile, syllablePosSelector);
    for (std::size_t position = 0; position < std::size(syllablePositionOrder); ++position) {
        const auto type = syllablePositionOrder[position];
        const auto style = syllableStyleFor(target, type);
        const std::string name =
            std::string("syllablePosStyles[") + syllablePositionNames[position] + ']';
        if (syllable.present) {
            const auto first = position * syllablePositionWords;
            if (const auto align = lyricAlignment(wordAt(syllable.words, first))) {
                style->align = *align;
                reportLyricField(report, name + ".align",
                    ValueOrigin::LegacyMus, wordAt(syllable.words, first),
                    syllable.blockOffset, syllable.decodedOffset);
            }
            if (const auto justify = lyricAlignment(wordAt(syllable.words, first + 1))) {
                style->justify = *justify;
                reportLyricField(report, name + ".justify",
                    ValueOrigin::LegacyMus, wordAt(syllable.words, first + 1),
                    syllable.blockOffset, syllable.decodedOffset);
            }
            const auto flags =
                static_cast<std::uint16_t>(wordAt(syllable.words, first + 2));
            style->on = (flags & (1U << syllablePositionOnBit)) != 0;
            reportLyricField(report, name + ".on", ValueOrigin::LegacyMus, style->on ? 1 : 0,
                syllable.blockOffset, syllable.decodedOffset);
        } else if (type != LyricSyllableType::Default) {
            // Before the selector exists the three optional positions are simply not applied,
            // and the pinned baseline says the opposite: it switches all three on. Every
            // companion of a document without the selector omits their <on/>, so leaving them
            // to the baseline would claim positioning the source never asked for. The
            // alignments are left alone, because the baseline already carries what those same
            // conversions produce.
            style->on = false;
            reportLyricField(report, name + ".on", ValueOrigin::LegacyBehavior, 0);
        }
    }

    // Selector 55: nine word-extension connection styles, three words each, spread over five
    // fixed rows whose last three words are padding. The gate is the record itself; see
    // @ref storesWordExtConnectTable for why the Coda-banner epoch is excluded from it.
    if (storesWordExtConnectTable(index, profile)) {
        const auto connect = readGlobalWords(index, profile, wordExtConnectSelector);
        for (std::size_t element = 0; element < wordExtConnectCount; ++element) {
            const auto type = wordExtConnectOrder[element];
            const auto style = connectStyleFor(target, type);
            const auto first = element * wordExtConnectWords;
            const std::string name =
                std::string("wordExtConnectStyles[") + wordExtConnectNames[element] + ']';
            const auto stored = wordAt(connect.words, first);
            const auto ordinal = stored - firstLegacyConnectIndex;
            if (ordinal >= 0
                && static_cast<std::size_t>(ordinal) < std::size(legacyConnectIndexOrder)) {
                style->connectIndex = legacyConnectIndexOrder[ordinal];
                reportLyricField(report, name + ".connectIndex", ValueOrigin::LegacyMus,
                    stored, connect.blockOffset, connect.decodedOffset);
            }
            style->xOffset = wordAt(connect.words, first + 1);
            style->yOffset = wordAt(connect.words, first + 2);
            reportLyricField(report, name + ".xOffset", ValueOrigin::LegacyMus, style->xOffset,
                connect.blockOffset, connect.decodedOffset);
            reportLyricField(report, name + ".yOffset", ValueOrigin::LegacyMus, style->yOffset,
                connect.blockOffset, connect.decodedOffset);
        }

        // musxdom keeps the starting connection's two offsets twice: once as that connection's
        // own, and once as the class-level word-extension offsets the Lyric Options dialog
        // shows. The record states them once. The Finale 2006 fixture is what shows the two are
        // the same value rather than merely equal by default: it stores 8 where every other
        // tracked document stores 4, and its companion moves both spellings together.
        const auto starting = connectStyleFor(target, LyricConnectType::DefaultStart);
        target->wordExtHorzOffset = starting->xOffset;
        target->wordExtVertOffset = starting->yOffset;
        reportLyricField(report, "wordExtHorzOffset", ValueOrigin::LegacyMus,
            target->wordExtHorzOffset, connect.blockOffset, connect.decodedOffset);
        reportLyricField(report, "wordExtVertOffset", ValueOrigin::LegacyMus,
            target->wordExtVertOffset, connect.blockOffset, connect.decodedOffset);
    }

    // The word-extension line width predates its own record. A Coda-banner document has no
    // selector 67, and the pinned baseline's 115 is not what those documents mean: all 25
    // tracked Finale 1.0.0 and 2.6.3 fixtures convert to 224, which is also what the earliest
    // document that does carry the selector stores. This is the one value of the class the
    // baseline supplies and supplies wrongly for an era, so it is asserted rather than left.
    if (!storesWordExtLineWidth(index, profile)) {
        target->wordExtLineWidth = unstatedWordExtLineWidth;
        reportLyricField(report, "wordExtLineWidth", ValueOrigin::LegacyBehavior,
            unstatedWordExtLineWidth);
    }

    // Syllable edge punctuation is ignored by default in a Finale 27 document and was not
    // ignored at all before Finale 2012, so the baseline states the opposite of every earlier
    // era's behavior. See @ref storesEdgePunctuationSetting for the boundary and for what a
    // Finale 2012 document is left with.
    if (!storesEdgePunctuationSetting(index, profile)) {
        target->lyricUseEdgePunctuation = true;
        reportLyricField(report, "lyricUseEdgePunctuation", ValueOrigin::LegacyBehavior, 1);
    }

    // The list of punctuation to ignore is stored as a variable-length tail on selector 57
    // rather than in a record of its own, which is why searching for the stock set found
    // nothing: **Finale writes the tail only when the list is not the stock one**. The record
    // is twelve bytes in every corpus document and in five of the six tracked Finale 2012
    // fixtures; `tests/evidence/F2012/F2012-lyric-punct.mus` sets the list to `#@%&` and its
    // record grows to twenty-four bytes, the six scalars unchanged and the four characters
    // appended as 16-bit code units followed by a zero.
    //
    // A document with no tail therefore means the stock list, and the right thing to do is
    // nothing: the pinned baseline states no <lyricPunctuationToIgnore> either, and musxdom's
    // LyricOptions::integrityCheck supplies exactly that set for an empty one. Writing it here
    // would be a second copy of a default musxdom already owns.
    if (storesEdgePunctuationSetting(index, profile)) {
        const auto lyric = readGlobalWords(index, profile, wordExtSelector);
        std::string ignored;
        std::size_t units = 0;
        for (std::size_t word = punctuationTailWord; word < lyric.words.size(); ++word) {
            const auto unit = static_cast<std::uint16_t>(lyric.words[word]);
            if (unit == 0) {
                break;
            }
            ++units;
            char32_t codepoint = unit;
            // UTF-16, so an astral character arrives as a surrogate pair. No specimen
            // exercises this -- every character of the stock list and of the controlled
            // fixture is in the basic multilingual plane -- but the era is the Unicode one
            // and a user can type anything into the field. A half pair passes through as
            // itself rather than being dropped, so a malformed tail stays visible.
            if (unit >= 0xd800 && unit <= 0xdbff && word + 1 < lyric.words.size()) {
                const auto low = static_cast<std::uint16_t>(lyric.words[word + 1]);
                if (low >= 0xdc00 && low <= 0xdfff) {
                    codepoint = static_cast<char32_t>(
                        0x10000 + ((unit - 0xd800) << 10) + (low - 0xdc00));
                    ++word;
                    ++units;
                }
            }
            ignored += musx::util::EnigmaString::toU8(codepoint);
        }
        if (!ignored.empty()) {
            target->lyricPunctuationToIgnore = ignored;
        }
        // The reported value is the number of code units the tail supplied, because the field
        // is text and the report carries numbers. Zero says the document kept the stock list.
        reportLyricField(report, "lyricPunctuationToIgnore",
            ignored.empty() ? ValueOrigin::Finale27Default : ValueOrigin::LegacyMus,
            static_cast<std::int64_t>(units), lyric.blockOffset, lyric.decodedOffset);
    }

    // The alternate hyphen font is a setting that postdates Finale 2012, the last release this
    // reader opens, so no legacy format has anywhere to put it and no legacy document uses a
    // second font for its hyphens. The switch is therefore known false for every file this
    // reader will ever open rather than guessed at.
    //
    // The pinned baseline also leaves it false, so the repository's usual rule -- leave a field
    // to the baseline where the baseline already agrees, rather than keeping a second copy of
    // one fact -- would say to omit this. It is asserted anyway, for the same reason
    // MultimeasureRestOptions::noHorizontalStretch is: the two statements are not the same
    // statement. The baseline saying false is one Finale 27 document's setting, which a later
    // pinned baseline could legitimately change; the setting postdating every legacy format is
    // a fact about the formats, and it is what makes the value known rather than synthesized.
    target->useAltHyphenFont = false;
    reportLyricField(report, "useAltHyphenFont", ValueOrigin::LegacyBehavior, 0);

    // The hyphen character postdates Finale 2012 in the same way, and is handled differently
    // on purpose. A boolean that is false because its feature does not exist can be stated in
    // code without restating anything; the character a legacy document drew cannot, because
    // writing U+002D here beside a pinned resource that already says 45 would be a second copy
    // of one fact -- the case the repository's rule is actually aimed at. The seeded value
    // stands and is reported as what it is, a Finale 27 default.
    reportLyricField(report, "hyphenChar", ValueOrigin::Finale27Default,
        static_cast<std::int64_t>(target->hyphenChar));

    // `altHyphenFont` itself is a different case from the switch above, and is deliberately
    // left alone. Which typeface an alternate hyphen would have used is not a fact about any
    // legacy era -- the feature did not exist, so there is no such typeface -- and the pinned
    // baseline carries no <altHyphenFont> element either. musxdom populates that member only
    // from such an element and synthesizes one in `integrityCheck` otherwise, and
    // `integrityCheck` runs at the end of construction, after every importer. A null pointer
    // here therefore means exactly that the baseline omitted the element, and it means it
    // without reading the baseline's XML, which no importer has access to in any case.
    //
    // So nothing is imported and nothing is reported for it. The obvious-looking alternative is
    // wrong: a FontInfo the baseline *had* seeded would carry the baseline's font numbering
    // rather than this document's and would need `musx::dom::importFontDefinitionInto` before
    // it named anything here, but a member the baseline never filled in is absent rather than
    // wrong. Copying the reference document's own synthesized placeholder into it would put a
    // value in the document that no document ever stated.
}

} // namespace

void importLyricOptions(const ImportContext& context)
{
    // The capture pass runs first because it establishes the fields no era-appropriate table
    // will read, and the table machinery leaves an already-reported field alone rather than
    // downgrading it to a synthesized default.
    captureLyricOptions(context.index, context.profile, context.document, context.report);
    applyMappingTables({&lyricHyphenTable(), &lyricLineWidthTable(), &smartLyricTable(),
                           &classLyricHyphenTable(), &classLyricLineWidthTable(),
                           &classSmartLyricTable(), &classEdgePunctuationTable()},
        context.index, context.profile, context.document, context.report);
}

} // namespace options
} // namespace finale_mus_reader
