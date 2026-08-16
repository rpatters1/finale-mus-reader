// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace options {
namespace {

using MmRestTarget = musx::dom::options::MultimeasureRestOptions;

// The multimeasure-rest defaults are a numeric global like any other option. They are not a
// record type of their own: through Finale 2006 the identity is the two decimal characters of
// the selector, and from Finale 2007 it is the class id the shared numericGlobalClass rule
// derives from that same selector.
constexpr std::uint16_t mmRestSelector = 25;

// "Automatically update multimeasure rests" is the one field of this class that lives
// somewhere else, on a selector of its own. See @ref storesMmRestAutoUpdate for why its
// presence rather than a version decides whether it can be read.
constexpr std::uint16_t mmRestUpdateSelector = 83;
constexpr std::uint32_t mmRestUpdateSlot = 4;

/// @brief An absolute word index into the family's second fixed row.
/// @details The zlib eras coalesce the whole incidence family into one payload and address it
/// by byte, where the fixed rows address the same option by incidence and slot. Deriving the
/// class offsets from the row size keeps the two encodings describing one word stream instead
/// of two independently maintained offset lists.
constexpr std::uint32_t secondRowWord(std::uint32_t slot)
{
    return records::otherWordCount + slot;
}

// Slots of the second fixed row, so that the fixed-row and class tables name the same field
// by the same number.
constexpr std::uint32_t symSpacingSlot = 0;
constexpr std::uint32_t numAdjXSlot = 1;
constexpr std::uint32_t startAdjustSlot = 2;
constexpr std::uint32_t endAdjustSlot = 3;
constexpr std::uint32_t mmRestFlagsSlot = 5;

// Bit 0 of the flags word is "use character rest style". All 917 companion-backed corpus
// documents that carry the bit have <useCharRestStyle/> in their Finale 27 conversion, and
// none of the 213 that do not, do.
//
// It is the only bit of this word that any legacy document uses, and the word is exactly 0 or
// 1 in all 3,458 later-layout documents of the reference corpus. The class's other boolean,
// "noHorizontalStretch", has no bit here to find: the option arrived with Finale 27, so no
// legacy format has a place to put it. It is asserted rather than mapped -- see
// @ref captureMultimeasureRestOptions.
constexpr std::uint8_t useSymbolsBit = 0;

// The word count of the whole selector-25 family in its early layout: one incidence, where
// Finale 3.5 and later write two.
constexpr std::size_t earlyMmRestFamilyWords = records::otherWordCount;

/// @brief Whether this source stores the six-word multimeasure-rest record.
/// @details Finale 3.5 rewrote this option's record, and the boundary is what makes a marker
/// necessary rather than merely preferable here: it falls **inside** the uncompressed epoch,
/// so an epoch gate cannot express it, and the versions on either side of it are 3.2 and 3.5
/// because no Finale 3.3 or 3.4 document exists in either surveyed corpus. A version range
/// would therefore have to guess its own cut point, and would in addition fail closed on the
/// Coda era's Windows documents, which state a platform where its Mac documents state a
/// version and so have no version at all. The family's own size states the layout outright.
///
/// The marker splits the reference corpus cleanly. All 264 of its 1.8.7, 2.0.1, 2.6, 3.0 and
/// 3.2 documents carry one incidence of six words, and all 3,458 of its 3.5-and-later
/// documents carry twelve, counting the zlib era's coalesced payload. No file carries any
/// other count and none crosses that line; the three that carry no selector 25 at all are
/// containers this reader cannot classify to an epoch either. The tracked-evidence survey
/// extends the early side below that corpus's earliest version: its 19 Finale 1.0.0 fixtures
/// all carry the six-word record, and unlike the reference corpus's earliest documents each
/// has an exact Finale 27 companion.
///
/// The installs survey adds the releases neither of the others contains, and every one falls
/// where the marker says: 11 Finale 3.8 and 43 Finale 98 documents on the later side, 1,295
/// Finale 2011 documents likewise, and 22 more Finale 1.0.0 documents on the early side. It
/// also holds the 24 Coda-era Windows documents, which state a platform where their Mac
/// contemporaries state a version and so carry no version at all. The marker recovers all 24;
/// a version range would have skipped every one of them without a word.
///
/// The two layouts share only the measure width, in slot 0. The number adjustment and the
/// shape moved from slots 4 and 5 down to 2 and 3 when the record grew, so reading an early
/// document through the later table would report its number adjustment as "start number at"
/// and its shape comparator as the symbol threshold, while taking the number adjustment and
/// shape from two words that mean something else entirely. That is the cost of an ambiguous
/// file, and it is why a source with no selector 25 at all reads as the later layout:
/// recovering nothing is better than recovering three fields from the wrong slots.
bool statesEarlyMmRestLayout(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    if (profile.epoch == FormatEpoch::ZlibLegacy) {
        return false;
    }
    const auto family = readNumericGlobalWords(index, mmRestSelector);
    return family.present && family.words.size() <= earlyMmRestFamilyWords;
}

bool statesLaterMmRestLayout(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    return !statesEarlyMmRestLayout(index, profile);
}

/// @brief Whether this source stores the automatic-update selector at all.
/// @details Selector 83 arrives with Finale 97 and is carried by every later document of the
/// reference corpus; none of its 1.8.7, 2.0.1, 2.6, 3.0, 3.2, 3.5 or 3.7 documents has it, and
/// this is the one boundary of this class that does not coincide with the layout marker above.
/// Reading the record's presence rather than dating the file keeps it out of a version range
/// whose lower end would sit somewhere between 3.7 and 97, and makes the absent case explicit
/// instead of silently defaulting: all 22 companion-backed documents that lack the selector
/// convert without <autoUpdateMmRests/>. What the absence means is therefore asserted in
/// @ref captureMultimeasureRestOptions rather than left to the pinned baseline, which would
/// say the opposite.
bool storesMmRestAutoUpdate(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    if (profile.epoch == FormatEpoch::ZlibLegacy) {
        return index.getClassOthers().get(
                   numericGlobalClass(mmRestUpdateSelector), GLOBALS_CMPER, 0, 0)
            != nullptr;
    }
    return readNumericGlobalWords(index, mmRestUpdateSelector).present;
}

// Finale 3.5 through Finale 2006, in both fixed-row epochs. Every location is the distilled
// framework's, and all 1,130 companion-backed later-layout documents of the reference corpus
// agree with their exact Finale 27 conversions on all nine values and on the flag, with no
// disagreement anywhere. The controlled Finale 97 fixture additionally separates the two rows:
// its second incidence carries the symbol spacing and the flag while its first carries the
// measure width, so a table that read them from one row would have to be wrong about one.
//
// Slot 1 of the first row and slot 4 of the second are the two words the framework leaves
// unnamed. Both are zero in all 3,458 later-layout documents, so nothing here can say what
// they are; they are **open** and deliberately unmapped.
const FieldMapping mmRestFields[] = {
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 0, measWidth),
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 2, numAdjY),
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 3, shapeDef),
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 4, numStart),
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 5, useSymsThreshold),
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 1, symSpacingSlot, symSpacing),
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 1, numAdjXSlot, numAdjX),
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 1, startAdjustSlot, startAdjust),
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 1, endAdjustSlot, endAdjust),
    MUS_BIT(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 1, mmRestFlagsSlot,
        useSymbolsBit, useSymbols),
};

// Finale 1.0.0 through Finale 3.2, spanning the whole Coda-banner epoch and the first
// uncompressed releases. Three fields, and the two that moved are what prove the layout: the
// Finale 3.0 and 3.2 documents carry -8 in slot 4 and 1 in slot 5 where their companions show
// a number adjustment of -8 and shape 1, and the controlled Finale 1.0.0 and 2.6.3 fixtures
// carry (12, 0) and (-28, 1) in those slots where their companions show exactly those pairs.
// The era's lower bound comes from the fixtures rather than from the reference corpus, whose
// earliest document is 1.8.7; all 19 tracked Finale 1.0.0 fixtures carry the six-word record.
// Slot 5
// is a shape comparator in this era as in every later one, and this reader keeps the source's
// own comparator because shape definitions are recovered from the source.
//
// All 59 companion-backed early documents agree with their conversions on all three fields.
//
// Slots 1 to 3 hold something this era stores and Finale 3.5 stopped writing: slot 1 varies
// per document across 0, 1, 2, 4, 5, 14, 16, 21, 22 and 25, and slots 2 and 3 move together
// as (24, 0) or (14, 1) in the Coda era and are (0, 0) in Finale 3.0 and 3.2. Finale 27's own
// conversion of these documents carries nothing from them, so no companion can name them, and
// no controlled edit has isolated one. They are **open** and unmapped. The rest of the class
// keeps the pinned baseline's values, which is what those same conversions produce for all 59:
// "start number at" 2, threshold 9, symbol spacing 48, horizontal number adjustment 0 and both
// booleans false. The two H-bar adjustments are the exception, because the baseline does not
// agree there; they are asserted in @ref captureMultimeasureRestOptions.
const FieldMapping earlyMmRestFields[] = {
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 0, measWidth),
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 4, numAdjY),
    MUS_WORD(MmRestTarget, "25", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 5, shapeDef),
};

// Selector 83 word 4, for the documents that carry the selector. Word 2 of the same record is
// also set in most of them and is not this flag: 468 companion-backed documents carry word 2
// set and word 4 clear and none of their conversions has <autoUpdateMmRests/>, while all 73
// that carry word 4 do. Reading word 2 instead would switch automatic updating on for roughly
// half the corpus.
const FieldMapping mmRestAutoUpdateFields[] = {
    MUS_WORD(MmRestTarget, "83", GLOBALS_CMPER, /*incidence*/ 0, mmRestUpdateSlot,
        autoUpdateMmRests),
};

// Finale 2007 and later: the same ten logical values, reached through the shared
// numericGlobalClass rule and addressed by byte offset in the coalesced payload. Both byte
// orders agree with their companions across the whole zlib cohort. The Finale 2012 fixture is
// what exercises the second row through this encoding: it carries 30 and -30 in the two H-bar
// adjustments where every other tracked fixture leaves them at zero.
const FieldMapping classMmRestFields[] = {
    MUS_CLASS_WORD(MmRestTarget, numericGlobalClass(mmRestSelector), GLOBALS_CMPER,
        classWordOffset(0), measWidth),
    MUS_CLASS_WORD(MmRestTarget, numericGlobalClass(mmRestSelector), GLOBALS_CMPER,
        classWordOffset(2), numAdjY),
    MUS_CLASS_WORD(MmRestTarget, numericGlobalClass(mmRestSelector), GLOBALS_CMPER,
        classWordOffset(3), shapeDef),
    MUS_CLASS_WORD(MmRestTarget, numericGlobalClass(mmRestSelector), GLOBALS_CMPER,
        classWordOffset(4), numStart),
    MUS_CLASS_WORD(MmRestTarget, numericGlobalClass(mmRestSelector), GLOBALS_CMPER,
        classWordOffset(5), useSymsThreshold),
    MUS_CLASS_WORD(MmRestTarget, numericGlobalClass(mmRestSelector), GLOBALS_CMPER,
        classWordOffset(secondRowWord(symSpacingSlot)), symSpacing),
    MUS_CLASS_WORD(MmRestTarget, numericGlobalClass(mmRestSelector), GLOBALS_CMPER,
        classWordOffset(secondRowWord(numAdjXSlot)), numAdjX),
    MUS_CLASS_WORD(MmRestTarget, numericGlobalClass(mmRestSelector), GLOBALS_CMPER,
        classWordOffset(secondRowWord(startAdjustSlot)), startAdjust),
    MUS_CLASS_WORD(MmRestTarget, numericGlobalClass(mmRestSelector), GLOBALS_CMPER,
        classWordOffset(secondRowWord(endAdjustSlot)), endAdjust),
    MUS_CLASS_BIT(MmRestTarget, numericGlobalClass(mmRestSelector), GLOBALS_CMPER,
        classWordOffset(secondRowWord(mmRestFlagsSlot)), useSymbolsBit, useSymbols),
};

const FieldMapping classMmRestAutoUpdateFields[] = {
    MUS_CLASS_WORD(MmRestTarget, numericGlobalClass(mmRestUpdateSelector), GLOBALS_CMPER,
        classWordOffset(mmRestUpdateSlot), autoUpdateMmRests),
};

constexpr const char* mmRestReportPrefix = "options.multimeasureRestOptions";

// Every epoch whose records are 16-byte rows, which is all three that are not zlib. The three
// tables below share this mask deliberately: the epoch says only how a record is addressed,
// and which of the two layouts a file uses is left entirely to the markers, so there is no
// epoch-shaped hole for a marker and a mask to disagree in. The Coda-banner epoch is covered
// because @ref statesEarlyMmRestLayout answers true for every document in it.
constexpr EpochMask fixedRowMmRestEpochs = EpochMask::CodaBanner | EpochMask::FixedRow;

const MappingTable& earlyMmRestTable()
{
    static const MappingTable table{
        .reportPrefix = mmRestReportPrefix,
        .epochs = fixedRowMmRestEpochs,
        .applies = &statesEarlyMmRestLayout,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<MmRestTarget>,
        .fields = earlyMmRestFields,
        .fieldCount = std::size(earlyMmRestFields)};
    return table;
}

const MappingTable& mmRestTable()
{
    static const MappingTable table{
        .reportPrefix = mmRestReportPrefix,
        .epochs = fixedRowMmRestEpochs,
        .applies = &statesLaterMmRestLayout,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<MmRestTarget>,
        .fields = mmRestFields,
        .fieldCount = std::size(mmRestFields)};
    return table;
}

const MappingTable& mmRestAutoUpdateTable()
{
    // The presence test alone decides this one, and it must: the capture pass asserts the
    // opposite when the same test says the record is absent, so a mask that could disagree
    // with it would leave a document with neither a read value nor an assertion, sitting at a
    // baseline default that says the opposite of both.
    static const MappingTable table{
        .reportPrefix = mmRestReportPrefix,
        .epochs = fixedRowMmRestEpochs,
        .applies = &storesMmRestAutoUpdate,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<MmRestTarget>,
        .fields = mmRestAutoUpdateFields,
        .fieldCount = std::size(mmRestAutoUpdateFields)};
    return table;
}

const MappingTable& classMmRestTable()
{
    static const MappingTable table{
        .reportPrefix = mmRestReportPrefix,
        .epochs = EpochMask::Zlib,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<MmRestTarget>,
        .fields = classMmRestFields,
        .fieldCount = std::size(classMmRestFields)};
    return table;
}

const MappingTable& classMmRestAutoUpdateTable()
{
    static const MappingTable table{
        .reportPrefix = mmRestReportPrefix,
        .epochs = EpochMask::Zlib,
        .applies = &storesMmRestAutoUpdate,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<MmRestTarget>,
        .fields = classMmRestAutoUpdateFields,
        .fieldCount = std::size(classMmRestAutoUpdateFields)};
    return table;
}

/// @brief Records one value the era fixed rather than stored.
void reportEraBehavior(ImportReport& report, const char* member, std::int64_t value)
{
    FieldInfo info;
    info.target = std::string(mmRestReportPrefix) + '.' + member;
    info.origin = ValueOrigin::LegacyBehavior;
    info.rawValue = value;
    report.fields.push_back(std::move(info));
}

/// @brief Asserts the values a legacy source fixed rather than stored.
/// @details The two H-bar adjustments and automatic updating arrive with Finale 3.5 and
/// Finale 97 respectively, and the pinned baseline supplies all three with values a document
/// from before them cannot have meant. "Stretch Horizontally" arrived with Finale 27 and is
/// false for every legacy document, whatever its era. Runs before the scalar tables, whose
/// gates then leave the asserted fields alone.
void captureMultimeasureRestOptions(const records::LegacyRecordIndex& index,
    const SourceProfile& profile, const musx::dom::DocumentPtr& document, ImportReport& report)
{
    const auto pooled = document->getOptions()->get<MmRestTarget>();
    if (!pooled) {
        return;
    }
    // Pool instances are handed out const. Overlaying legacy values is the one reason this
    // library writes to them.
    auto target = std::const_pointer_cast<MmRestTarget>(pooled);

    // Two of this class's fields are not merely absent from the early record but *wrong* in
    // the pinned baseline, so leaving them to it would assert the opposite of what the source
    // means. Finale 27's "New Document Without Libraries" starts the H-bar 30 Evpu in and ends
    // it 30 Evpu out; a document written before Finale 3.5 has no such adjustment, because the
    // whole second row that carries it does not exist. Finale 27's own conversions agree:
    // every early-layout companion in the reference corpus, Coda-banner and Finale 3.0/3.2
    // alike, writes neither <startAdjust> nor <endAdjust>.
    //
    // The rest of the class is left to the baseline deliberately, because the baseline already
    // carries what those conversions produce -- 2, 9 and 48 for the start number, threshold and
    // symbol spacing, and zero for the horizontal number adjustment -- and repeating a value
    // the pinned resource already states would be a second copy of one fact.
    if (statesEarlyMmRestLayout(index, profile)) {
        target->startAdjust = 0;
        target->endAdjust = 0;
        reportEraBehavior(report, "startAdjust", 0);
        reportEraBehavior(report, "endAdjust", 0);
    }

    // Likewise for automatic updating, which is a Finale 97 arrival. The baseline switches it
    // on, and a document from a version that had no such setting never updated its
    // multimeasure rests, so inheriting the baseline would claim a behavior the source cannot
    // have had. Every early companion agrees: none carries <autoUpdateMmRests/>. **An absent
    // selector 83 means off, with no further qualification** -- including for a document whose
    // epoch could not be classified, which is exactly the document that must not be left
    // claiming a Finale 27 setting.
    if (!storesMmRestAutoUpdate(index, profile)) {
        target->autoUpdateMmRests = false;
        reportEraBehavior(report, "autoUpdateMmRests", 0);
    }

    // "Stretch Horizontally" is a Finale 27 feature. No legacy document of any era can state
    // it, so its value is known exactly for every file this reader will ever open, and it is
    // false. This is the one field the reader asserts unconditionally.
    //
    // The pinned baseline also leaves it false, so the repository's usual rule -- leave a field
    // to the baseline where the baseline already agrees, rather than keeping a second copy of
    // one fact -- would say to omit this. It is asserted anyway, because the two statements are
    // not the same statement. The baseline saying false is one Finale 27 document's setting,
    // which a later pinned baseline could legitimately change; the feature postdating every
    // legacy format is a fact about the formats, and it makes the value known rather than
    // synthesized. Reporting it as a Finale 27 default would understate that, and would leave
    // the field looking like an unanswered question in a coverage survey.
    target->noHorizontalStretch = false;
    reportEraBehavior(report, "noHorizontalStretch", 0);
}

/// @brief Checks the recovered H-bar shape reference against the shape pool.
/// @details Runs after the shape definitions are decoded, which the registry guarantees by
/// filling the others pool before any options class.
void validateMultimeasureRestOptions(
    const musx::dom::DocumentPtr& document, ImportReport& report)
{
    const auto target = document->getOptions()->get<MmRestTarget>();
    if (!target || target->shapeDef == 0) {
        return;
    }
    // Comparator zero means the document draws no H-bar shape rather than naming a pooled
    // object. A nonzero one that no recovered definition answers is kept exactly as stored: it
    // is what the source says, and substituting the baseline's own shape would name a
    // different document's drawing.
    //
    // Info rather than Warning, because this is ordinary Finale behavior and not a sign that
    // anything went wrong. 319 zlib documents of the installs survey name an H-bar shape their
    // own file never defines -- they carry no shape records at all -- and Finale supplies the
    // drawing from its own resources. Raising it as a warning would report a normal document as
    // suspect several hundred times over, which is how a diagnostic channel stops being read.
    if (!document->getOthers()->get<musx::dom::others::ShapeDef>(
            musx::dom::SCORE_PARTID, target->shapeDef)) {
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
            "The multimeasure rest H-bar names shape definition "
            + std::to_string(target->shapeDef)
            + ", which this document does not define; it is kept as stored."});
    }
}

} // namespace

void importMultimeasureRestOptions(const ImportContext& context)
{
    // The capture pass runs first because it establishes the fields no era-appropriate table
    // will read, and the table machinery leaves an already-reported field alone rather than
    // downgrading it to a synthesized default.
    captureMultimeasureRestOptions(
        context.index, context.profile, context.document, context.report);
    applyMappingTables({&earlyMmRestTable(), &mmRestTable(), &mmRestAutoUpdateTable(),
                           &classMmRestTable(), &classMmRestAutoUpdateTable()},
        context.index, context.profile, context.document, context.report);
    validateMultimeasureRestOptions(context.document, context.report);
}

} // namespace options
} // namespace finale_mus_reader
