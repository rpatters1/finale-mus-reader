// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options.h"

#include "import/options/test_access.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "import/support/text_encoding.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace options {
namespace {

using StemOptionsTarget = musx::dom::options::StemOptions;
using StemConnection = StemOptionsTarget::StemConnection;

// The stem-connection table is a numeric global like any other option: the two decimal
// characters of its selector through Finale 2006, and from 2007 the class id the shared
// numericGlobalClass rule derives from that same selector.
constexpr std::uint16_t stemConnectionSelector = 40;

// One connection occupies one fixed row, so the element is exactly the six-word payload of
// one incidence: font, symbol, and the four adjustments. From Finale 2012 the symbol is a
// long, which adds one word and shifts every adjustment after it.
constexpr std::size_t narrowElementWords = 6;
constexpr std::size_t wideElementWords = 7;

// musxdom's own conversion, composed rather than restated, so the size of an Evpu keeps one
// definition.
constexpr double efixPerEvpu = musx::dom::EFIX_PER_EVPU;

// The highest Unicode codepoint. A symbol above it did not come from the widened field, and
// in the zlib era that is how a record still holding pre-Unicode bytes announces itself: the
// old table's first two words read as one implausible long. See @ref captureStemOptions.
constexpr std::uint32_t maximumCodepoint = 0x10FFFF;

/// @brief One stem connection as the source stores it, before any musxdom semantics.
struct PhysicalConnection
{
    std::uint16_t fontId{};
    /// @brief Whether the source stored the symbol as a code point rather than as a byte in
    /// the connection's own font encoding.
    bool symbolIsCodepoint{};
    std::uint32_t symbol{};
    std::int16_t upStemVert{};
    std::int16_t downStemVert{};
    std::int16_t upStemHorz{};
    std::int16_t downStemHorz{};
};

/// @brief Reads one connection out of a flat word stream, in either element width.
PhysicalConnection decodeElement(
    const std::vector<std::int16_t>& words, std::size_t first, std::size_t elementWords)
{
    // The wide element is the narrow one with the symbol promoted to a long, so every field
    // after the symbol shifts by exactly one word. Expressing it as an offset keeps the two
    // layouts from becoming two independently maintained field lists.
    const std::size_t shift = elementWords == wideElementWords ? 1 : 0;
    PhysicalConnection result;
    result.fontId = static_cast<std::uint16_t>(wordAt(words, first));
    result.symbol = shift == 0
        ? narrowCodepoint(wordAt(words, first + 1))
        : wideCodepoint(wordAt(words, first + 1), wordAt(words, first + 2));
    result.symbolIsCodepoint = shift != 0;
    result.upStemVert = wordAt(words, first + 2 + shift);
    result.downStemVert = wordAt(words, first + 3 + shift);
    result.upStemHorz = wordAt(words, first + 4 + shift);
    result.downStemHorz = wordAt(words, first + 5 + shift);
    return result;
}

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
void reportConnectionField(ImportReport& report, std::size_t index, const char* member,
    std::int64_t rawValue, std::size_t blockOffset, std::size_t decodedOffset)
{
    FINALE_MUS_READER_REPORT_FIELD(report, instanceKey<StemOptionsTarget>(),
        "stemConnections[" + std::to_string(index) + "]." + member,
        {ValueOrigin::LegacyMus, blockOffset, decodedOffset, rawValue});
}
#else
#define reportConnectionField(...) ((void)0)
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

/// @brief Turns one stored connection into a musxdom StemConnection and reports its values.
/// @details Before Finale 2012 the symbol is a byte in the encoding of the font the connection
/// names, not a code point, so it is decoded through that font exactly as legacy text is. A
/// music font is a symbol font and its byte is a glyph number that survives unchanged, which is
/// what every connection in every sampled document uses; a connection pointed at a text font is
/// the case that makes the difference visible.
void insertRecoveredConnection(const musx::dom::DocumentPtr& document,
    const std::shared_ptr<StemOptionsTarget>& target,
    const PhysicalConnection& stored, bool adjustmentsAreEvpu,
    std::size_t blockOffset, std::size_t decodedOffset, ImportReport& report,
    const text::SymbolFontNames* symbolFontNames)
{
    const auto index = target->stemConnections.size();
    const auto toEfix = [adjustmentsAreEvpu](std::int16_t stored) {
        return adjustmentsAreEvpu
            ? musx::dom::Efix(stored * efixPerEvpu) : musx::dom::Efix(stored);
    };
    auto connection = std::make_shared<StemConnection>();
    connection->fontId = musx::dom::Cmper(stored.fontId);
    connection->symbol = stored.symbolIsCodepoint
        ? static_cast<char32_t>(stored.symbol)
        : text::codepointFromByte(static_cast<std::uint8_t>(stored.symbol),
            document, connection->fontId, text::UnresolvedFontFallback::Symbol,
            symbolFontNames);
    connection->upStemVert = toEfix(stored.upStemVert);
    connection->downStemVert = toEfix(stored.downStemVert);
    connection->upStemHorz = toEfix(stored.upStemHorz);
    connection->downStemHorz = toEfix(stored.downStemHorz);
    target->stemConnections.push_back(std::move(connection));

    reportConnectionField(report, index, "fontId", stored.fontId, blockOffset, decodedOffset);
    reportConnectionField(report, index, "symbol",
        static_cast<std::int64_t>(stored.symbol), blockOffset, decodedOffset);
    // The raw stored words, not the assigned Efix. A pre-Finale-3.5 value is scaled on the
    // way in, and the report is the only place the original Evpu number survives.
    reportConnectionField(report, index, "upStemVert", stored.upStemVert,
        blockOffset, decodedOffset);
    reportConnectionField(report, index, "downStemVert", stored.downStemVert,
        blockOffset, decodedOffset);
    reportConnectionField(report, index, "upStemHorz", stored.upStemHorz,
        blockOffset, decodedOffset);
    reportConnectionField(report, index, "downStemHorz", stored.downStemHorz,
        blockOffset, decodedOffset);
}

// How many connection slots the table held before Finale 3.5.
constexpr std::size_t earlySlotCount = 32;

/// @brief Whether this source predates the Finale 3.5 units for the whole stem family.
/// @details Finale 3.5 changed every unit in this option family at once: the connection
/// collection grew from 32 slots to 128, its four adjustments became Efix where they had
/// been Evpu, and the scalar lengths became Evpu where they had been staff positions. One
/// marker therefore decides all of them, and it is the collection's own size.
///
/// A version range would also work, so this is a preference rather than a necessity, and it is
/// the same preference that decides the clef tuple width from its payload size. Three things
/// recommend it. **The boundary version is unobserved**: no Finale 3.3 or 3.4 document is
/// available to place the cut, while the slot count is something each file states outright.
/// **One fact decides three questions**, so the collection size, the connection unit and the
/// scalar unit cannot drift out of step. And **the Coda era states no version at all in its
/// Windows documents**, so a version range would have to be split across two tables by epoch to
/// reach them, where one marker spans both.
///
/// A source with no connection family at all reads as modern, which is the common case and
/// the one that leaves values alone rather than scaling them.
bool statesPreFinale35Units(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    if (profile.epoch == FormatEpoch::ZlibLegacy) {
        return false;
    }
    const auto family = readNumericGlobalWords(index, stemConnectionSelector);
    return family.present
        && family.words.size() <= earlySlotCount * narrowElementWords;
}

bool statesFinale35Units(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    return !statesPreFinale35Units(index, profile);
}

// The selector and word that carry the reverse-stemming flag in every era.
constexpr std::uint16_t beamFlagsSelector = 41;
constexpr std::uint32_t beamFlagsSlot = 1;

/// @brief Whether this source keeps the reverse-stemming flag alone in its word.
/// @details The flag's own boundary is not the Finale 3.5 unit boundary. Selector 41 word 1 is
/// exactly zero through Finale 97 and carries packed beam options from Finale 2000 on, so the
/// word acquires other tenants at that release. The file states which layout it is in: any bit
/// above bit 0 means the packed one, whose flag sits at bit 2.
///
/// A content test rather than a version range, because what a file shows directly is whether
/// its word is packed, not which release began packing it. The test costs nothing when the word
/// is zero, where both spellings answer false, and it can only mislead on a packed-era file
/// whose beam options are exactly bit 0 alone -- a value not seen. The compressed epochs are
/// all Finale 2001 or later and are settled by their epoch without consulting the word.
bool statesLoneStemFlag(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    if (profile.epoch != FormatEpoch::CodaBanner
        && profile.epoch != FormatEpoch::UncompressedLegacy) {
        return false;
    }
    const auto* row = index.getOthers().get(
        numericGlobalTag(beamFlagsSelector), GLOBALS_CMPER, 0, 0);
    if (!row || row->wordCount <= beamFlagsSlot) {
        return true;
    }
    constexpr std::uint16_t loneFlagBit = 0x0001;
    return (static_cast<std::uint16_t>(row->words[beamFlagsSlot]) & ~loneFlagBit) == 0;
}

bool statesPackedBeamFlags(
    const records::LegacyRecordIndex& index, const SourceProfile& profile)
{
    return !statesLoneStemFlag(index, profile);
}

// One staff position is one half space, so the pre-3.5 lengths convert through musxdom's own
// constant rather than through a 12 written here.
constexpr int evpuPerStaffPosition = musx::dom::EVPU_PER_STAFF_POSITION;

// Finale 3.5 and later, and the DCL epoch entire. Every location is the framework's.
const FieldMapping stemScalarFields[] = {
    MUS_WORD(StemOptionsTarget, "03", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 2, halfStemLength),
    MUS_WORD(StemOptionsTarget, "20", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 4, stemLength),
    MUS_WORD(StemOptionsTarget, "20", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 5, shortStemLength),
    MUS_WORD(StemOptionsTarget, "21", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 2, revStemAdj),
    MUS_WORD(StemOptionsTarget, "64", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 5, stemWidth),
    MUS_LONG(StemOptionsTarget, "65", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 0,
        LongWordOrder::HighFirst, stemOffset),
    MUS_WORD(StemOptionsTarget, "31", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 5,
        useStemConnections),
};

// The packed spelling of the flag, for the fixed-row eras that have one.
const FieldMapping packedStemFlagFields[] = {
    MUS_BIT(StemOptionsTarget, "41", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 1,
        /*bit*/ 2, noReverseStems),
};

// Before Finale 3.5, in both the Coda-banner and uncompressed epochs. The three lengths sit in
// the same slots but are stated in staff positions rather than Evpu, so each is scaled by
// @ref evpuPerStaffPosition on the way out.
//
// The half-stem length is absent rather than scaled. Selector 03 carries no row at all in
// Finale 3.0 and 3.2, and in the Coda era its word 2 is zero, so reading it would assign a zero
// the source does not state.
//
// The connection switch keeps its later location and needs no scaling.
const FieldMapping earlyStemScalarFields[] = {
    MUS_BITS_AS(StemOptionsTarget, "20", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 4,
        /*firstBit*/ 0, /*bitCount*/ 0, stemLength,
        static_cast<musx::dom::Evpu>(value * evpuPerStaffPosition)),
    MUS_BITS_AS(StemOptionsTarget, "20", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 5,
        /*firstBit*/ 0, /*bitCount*/ 0, shortStemLength,
        static_cast<musx::dom::Evpu>(value * evpuPerStaffPosition)),
    MUS_BITS_AS(StemOptionsTarget, "21", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 2,
        /*firstBit*/ 0, /*bitCount*/ 0, revStemAdj,
        static_cast<musx::dom::Evpu>(value * evpuPerStaffPosition)),
    MUS_WORD(StemOptionsTarget, "31", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 5,
        useStemConnections),
};

// Selector 41 word 1 holds the reverse-stemming flag in every era, but not in the same bit --
// the only stem field known to move. Before the packed era it is **bit 0**, the word holding
// nothing else. From the packed era it is bit 2, bit 0 there being another beam option
// entirely; reading bit 0 in that era would report reverse stemming as off for documents that
// leave it on. See @ref statesLoneStemFlag for which spelling a document is in.
//
// The sense is the same on both sides: set means reverse stemming is not displayed.
const FieldMapping loneStemFlagFields[] = {
    MUS_BIT(StemOptionsTarget, "41", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 1,
        /*bit*/ 0, noReverseStems),
};

// The stem thickness and offset hold their later locations from Finale 3.0, but not before it:
// in the Coda era both slots hold 5000 against an effective 128, on both platforms, and the
// earliest release carries neither selector at all. The epoch alone excludes that era, so this
// needs no version test -- which is what makes it safe for Coda-banner Windows documents, none
// of which state a version.
//
const FieldMapping earlyStemSizeFields[] = {
    MUS_WORD(StemOptionsTarget, "64", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 5, stemWidth),
    MUS_LONG(StemOptionsTarget, "65", GLOBALS_CMPER, /*incidence*/ 0, /*slot*/ 0,
        LongWordOrder::HighFirst, stemOffset),
};

// Finale 2007 and later: the same eight logical options, reached through the shared
// numericGlobalClass rule and addressed by byte offset. Both byte orders occur in this era.
const FieldMapping classStemScalarFields[] = {
    MUS_CLASS_WORD(StemOptionsTarget, numericGlobalClass(3), GLOBALS_CMPER, classWordOffset(2), halfStemLength),
    MUS_CLASS_WORD(StemOptionsTarget, numericGlobalClass(20), GLOBALS_CMPER, classWordOffset(4), stemLength),
    MUS_CLASS_WORD(StemOptionsTarget, numericGlobalClass(20), GLOBALS_CMPER, classWordOffset(5), shortStemLength),
    MUS_CLASS_WORD(StemOptionsTarget, numericGlobalClass(21), GLOBALS_CMPER, classWordOffset(2), revStemAdj),
    MUS_CLASS_WORD(StemOptionsTarget, numericGlobalClass(64), GLOBALS_CMPER, classWordOffset(5), stemWidth),
    MUS_CLASS_LONG(StemOptionsTarget, numericGlobalClass(65), GLOBALS_CMPER, classWordOffset(0),
        LongWordOrder::HighFirst, stemOffset),
    MUS_CLASS_WORD(StemOptionsTarget, numericGlobalClass(31), GLOBALS_CMPER, classWordOffset(5),
        useStemConnections),
    MUS_CLASS_BIT(StemOptionsTarget, numericGlobalClass(41), GLOBALS_CMPER, classWordOffset(1),
        /*bit*/ 2, noReverseStems),
};

const MappingTable& stemScalarsTable()
{
    static const MappingTable table{
        .reportPrefix = "options.stemOptions",
        .epochs = EpochMask::FixedRow,
        .applies = &statesFinale35Units,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<StemOptionsTarget>,
        .fields = stemScalarFields,
        .fieldCount = std::size(stemScalarFields)};
    return table;
}

const MappingTable& earlyStemScalarsTable()
{
    static const MappingTable table{
        .reportPrefix = "options.stemOptions",
        .epochs = EpochMask::CodaBanner | EpochMask::Uncompressed,
        .applies = &statesPreFinale35Units,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<StemOptionsTarget>,
        .fields = earlyStemScalarFields,
        .fieldCount = std::size(earlyStemScalarFields)};
    return table;
}

const MappingTable& loneStemFlagTable()
{
    static const MappingTable table{
        .reportPrefix = "options.stemOptions",
        .epochs = EpochMask::CodaBanner | EpochMask::Uncompressed,
        .applies = &statesLoneStemFlag,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<StemOptionsTarget>,
        .fields = loneStemFlagFields,
        .fieldCount = std::size(loneStemFlagFields)};
    return table;
}

const MappingTable& packedStemFlagTable()
{
    static const MappingTable table{
        .reportPrefix = "options.stemOptions",
        .epochs = EpochMask::FixedRow,
        .applies = &statesPackedBeamFlags,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<StemOptionsTarget>,
        .fields = packedStemFlagFields,
        .fieldCount = std::size(packedStemFlagFields)};
    return table;
}

const MappingTable& earlyStemSizesTable()
{
    static const MappingTable table{
        .reportPrefix = "options.stemOptions",
        .epochs = EpochMask::Uncompressed,
        .applies = &statesPreFinale35Units,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<StemOptionsTarget>,
        .fields = earlyStemSizeFields,
        .fieldCount = std::size(earlyStemSizeFields)};
    return table;
}

const MappingTable& classStemScalarsTable()
{
    static const MappingTable table{
        .reportPrefix = "options.stemOptions",
        .epochs = EpochMask::Zlib,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<StemOptionsTarget>,
        .fields = classStemScalarFields,
        .fieldCount = std::size(classStemScalarFields)};
    return table;
}

} // namespace

void captureStemOptions(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document, ImportReport& report)
{
    const auto pooled = document->getOptions()->get<StemOptionsTarget>();
    if (!pooled) {
        return;
    }
    // Pool instances are handed out const, and this is the one place the collection is
    // written. The scalars around it stay seeded from the pinned baseline.
    auto target = std::const_pointer_cast<StemOptionsTarget>(pooled);

    // The connections a document uses are its own, and the baseline's are a different
    // document's table naming a different document's fonts. They are dropped before anything
    // is read, so the collection holds exactly what this source turns out to carry: an empty
    // collection is the honest result for a source that stores none.
    target->stemConnections.clear();

    const bool wide = sourceMatches(profile, EpochMask::Zlib)
        && versions::storesUnicodeCodepoints(profile.version);
    const std::size_t elementWords = wide ? wideElementWords : narrowElementWords;

    const auto family = readGlobalWords(index, profile, stemConnectionSelector);
    const auto& words = family.words;
    const auto blockOffset = family.blockOffset;
    const auto decodedOffset = family.decodedOffset;
    if (family.present && wide && profile.byteOrder == ByteOrder::BigEndian) {
        // **Unverified: the widened symbol's word order in big-endian containers.** No such
        // document is known; see @ref wideCodepoint. Rather than rely on that absence, the case
        // announces itself when it arrives -- if the order is the other way the symbols are not
        // slightly off but nonsense.
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
            "This document uses the Finale 2012 stem-connection layout in big-endian "
            "order, an untested combination; the symbol's word order is unverified "
            "for it."});
    }

    if (!family.present) {
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Verbose,
            "This document stores no stem-connection table, so it has no stem connections."});
        return;
    }

    // The table is a run of connections terminated by the first element with no symbol, and
    // whatever follows that element is not part of it. In the zlib era the bytes after the
    // terminator are frequently a stale copy of the pre-Unicode default table left in the
    // record, which decodes through the widened element as the out-of-range symbols Finale
    // itself ignores.
    //
    // Finale's own upgrade does not stop at the terminator and carries those trailing elements
    // through, so it produces more connections than this does. The difference is intended: an
    // element the source has terminated states nothing.
    for (std::size_t first = 0; first + elementWords <= words.size(); first += elementWords) {
        const auto stored = decodeElement(words, first, elementWords);
        if (stored.symbol == 0) {
            break;
        }
        if (stored.symbol > maximumCodepoint) {
            // Only reachable in the Unicode era, where the pre-Unicode default table left in
            // the record reads as one implausible long. Recovering nothing is what Finale
            // sees in such a document; recovering the old table instead would assert a
            // layout this era does not use. The offending element is reported raw so the
            // case stays visible rather than looking like a document with no connections.
            //
            // Info, not a warning: the out-of-range symbol is the format's own leftover
            // default-table bytes, not content the user entered, and the connections found
            // before it are already kept. Warning would report a fully usable document as
            // though something in it were broken.
            const auto recoveredCount = target->stemConnections.size();
            report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,
                "Stem connections stopped at index " + std::to_string(recoveredCount)
                + ": symbol " + std::to_string(stored.symbol) + " is not a Unicode codepoint "
                  "(stale default-table bytes); " + std::to_string(recoveredCount)
                + " kept."});
            reportConnectionField(report, target->stemConnections.size(), "symbol",
                static_cast<std::int64_t>(stored.symbol), blockOffset, decodedOffset);
            break;
        }
        insertRecoveredConnection(document, target, stored,
            statesPreFinale35Units(index, profile), blockOffset, decodedOffset, report,
            profile.symbolFontNames);
    }
}

void validateStemOptions(const musx::dom::DocumentPtr& document,
    musx::factory::ConstructionContext& construction)
{
    const auto target = document->getOptions()->get<StemOptionsTarget>();
    if (!target) {
        return;
    }
    // A connection names a font by comparator, and one that no recovered definition answers is
    // preserved exactly as stored: it is what the source says, and replacing it with a default
    // would invent a typeface the document never named. The test is whether the document
    // defines the comparator, not whether the comparator is zero -- what zero means belongs to
    // musxdom, which resolves it to the default music font and guarantees a definition for it.
    //
    // Registering is what keeps "preserved as stored" from meaning "unusable". The comparator
    // stays exactly as the source wrote it, and musxdom mints a placeholder definition for it
    // at the end of construction, logging the substitution itself -- captured into this same
    // report -- so nothing here needs to duplicate that check.
    for (const auto& connection : target->stemConnections) {
        construction.registerFontId(connection->fontId);
    }
}

void importStemOptions(const ImportContext& context)
{
    // The connection collection is source-owned, so nothing is overlaid onto it and no table
    // applies to it. The scalars around it are seeded from the pinned baseline and overlaid
    // by whichever of the four tables this file's era matches; the other three still report
    // their fields, so a document from an era a table does not cover shows its supported
    // fields sitting at their synthesized defaults rather than not at all.
    captureStemOptions(context.index, context.profile, context.document, context.report);
    applyMappingTables({&earlyStemScalarsTable(), &earlyStemSizesTable(), &stemScalarsTable(),
                           &loneStemFlagTable(), &packedStemFlagTable(),
                           &classStemScalarsTable()},
        context.index, context.profile, context.document, context.report);
    validateStemOptions(context.document, context.construction);
}

} // namespace options
} // namespace finale_mus_reader

#if !defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
#undef reportConnectionField
#endif // !defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
