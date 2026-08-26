// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options.h"

#include "import/options/test_access.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace options {
namespace {

using FontDefinition = musx::dom::others::FontDefinition;
using FontInfo = musx::dom::FontInfo;
using FontOptionsTarget = musx::dom::options::FontOptions;
using FontType = FontOptionsTarget::FontType;

constexpr std::uint16_t fontOptionsSelector = 24;
constexpr std::size_t tupleFieldCount = 3;
constexpr std::size_t wordSize = 2;
constexpr std::size_t tupleByteSize = tupleFieldCount * wordSize;
constexpr std::size_t fontTypeCount = static_cast<std::size_t>(FontType::TimePlusParts) + 1;

enum class TupleField : std::size_t
{
    FontId = 0,
    Size = 1,
    Effects = 2
};

struct FontOptionsLayout
{
    EpochMask epochs{};
    VersionRange versions{};
    RecordEncoding encoding{};
    records::LegacyTag identity{};
};

struct EarlyField
{
    records::LegacyTag identity{};
    std::uint32_t word{};
};

struct EarlyTuple
{
    FontType type{};
    std::array<EarlyField, tupleFieldCount> fields{};
};

// One row per variable-length physical representation. Tuple counts come from the file.
//
// The fixed-row row is gated on the epoch rather than on a version range. Selector 24 is the
// default-font array in every epoch that uses fixed rows except the Coda-banner era, where it
// holds one row of unrelated values, and `EpochMask::FixedRow` excludes exactly that era. A
// version range would add nothing here and could only cost files, since the era boundary and
// the epoch boundary are the same one.
const std::array<FontOptionsLayout, 2> layouts{{
    {EpochMask::FixedRow, versions::any(),
        RecordEncoding::FixedRow, records::packTag("24")},
    {EpochMask::Zlib, versions::any(), RecordEncoding::ClassRecord,
        numericGlobalClass(fontOptionsSelector)},
}};

constexpr EarlyField earlyField(std::string_view tag, std::uint32_t word)
{
    return {records::packTag(tag), word};
}

constexpr EarlyTuple contiguousEarlyTuple(
    FontType type, std::string_view tag, std::uint32_t firstWord)
{
    return {type, {earlyField(tag, firstWord), earlyField(tag, firstWord + 1),
                      earlyField(tag, firstWord + 2)}};
}

// Confirmed by controlled Finale 1.0.0 source edits and their exact Finale 27 upgrades,
// except StaffNames: Finale 1.0 calls the source preference "Name", and this era has exactly
// one of it where Finale 3.0 and later store four. Its continuation as StaffNames is strong
// under the additive-only early-version hypothesis. See @ref codaNameCompanionTypes.
const std::array<EarlyTuple, 13> earlyCodaTuples{{
    contiguousEarlyTuple(FontType::Music, "02", 0),
    contiguousEarlyTuple(FontType::Key, "03", 3),
    {FontType::Clef,
        {earlyField("04", 0), earlyField("39", 4), earlyField("39", 5)}},
    contiguousEarlyTuple(FontType::Time, "03", 0),
    contiguousEarlyTuple(FontType::Chord, "02", 3),
    contiguousEarlyTuple(FontType::ChordAcci, "37", 0),
    contiguousEarlyTuple(FontType::Ending, "05", 0),
    contiguousEarlyTuple(FontType::Tuplet, "36", 0),
    contiguousEarlyTuple(FontType::TextBlock, "26", 0),
    contiguousEarlyTuple(FontType::LyricVerse, "26", 3),
    contiguousEarlyTuple(FontType::LyricChorus, "27", 0),
    contiguousEarlyTuple(FontType::LyricSection, "27", 3),
    contiguousEarlyTuple(FontType::StaffNames, "04", 3),
}};

// The Coda-banner era exposes a single "Name" font preference. Finale 3.0 split it into the
// four modern name types, which that era stores as separate tuples at physical ordinals 31,
// 32, 33 and 39 -- so this fan-out belongs to the Coda epoch alone and must not be applied
// to any later one.
//
// Recovering only StaffNames would leave the other three at Finale 27 defaults and so emit a
// document whose staff names use one face and size while its group and abbreviated names use
// another. The source never had that split, and neither does the Finale 27 baseline, which
// sets all four identically. Propagating the one preference to all four is the reading that
// preserves the source's own behavior.
//
// This deliberately diverges from what Finale's own upgrade produces, and the divergence is
// recorded in research/FORMAT_NOTES.md and open to revision: that upgrade's output for these
// four types tracks the default file it ran under rather than the source document, so it does
// not settle the question. The three propagated types report as ValueOrigin::LegacyBehavior,
// not LegacyMus: the bytes are read from the source, but the assignment restores an era
// behavior rather than an option the source stored.
constexpr std::array<FontType, 3> codaNameCompanionTypes{
    FontType::AbbrvStaffNames,
    FontType::GroupNames,
    FontType::AbbrvGroupNames,
};

const FontOptionsLayout* layoutFor(const SourceProfile& profile)
{
    for (const auto& layout : layouts) {
        if (epochMatches(layout.epochs, profile.epoch)
            && layout.versions.includes(profile.version)) {
            return &layout;
        }
    }
    return nullptr;
}

std::size_t tupleCount(const FontOptionsLayout& layout,
    const records::LegacyRecordIndex& index, ImportReport& report)
{
    if (layout.encoding == RecordEncoding::FixedRow) {
        const auto rows = index.getOthers().getArray(layout.identity, GLOBALS_CMPER);
        return rows.size() * records::otherWordCount / tupleFieldCount;
    }

    const auto* row = index.getClassOthers().get(layout.identity, GLOBALS_CMPER, 0, 0);
    if (!row) {
        return 0;
    }
    const auto bytes = index.getClassOthers().payloadOf(*row);
    if (const auto trailing = bytes.size() % tupleByteSize; trailing != 0) {
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Verbose,"Ignored " + std::to_string(trailing)
            + " trailing byte(s) after the last complete legacy font-options tuple."});
    }
    return bytes.size() / tupleByteSize;
}

SourceLocation sourceFor(
    const FontOptionsLayout& layout, std::size_t ordinal, TupleField field)
{
    const auto fieldIndex = static_cast<std::size_t>(field);
    SourceLocation result;
    result.identity = layout.identity;
    result.selector = GLOBALS_CMPER;
    result.width = ValueWidth::Word;
    if (layout.encoding == RecordEncoding::FixedRow) {
        const auto absoluteWord = ordinal * tupleFieldCount + fieldIndex;
        result.incidence = static_cast<std::uint32_t>(
            absoluteWord / records::otherWordCount);
        result.wordSlot = static_cast<std::uint32_t>(
            absoluteWord % records::otherWordCount);
    } else {
        result.incidence = 0;
        result.wordSlot = static_cast<std::uint32_t>(
            ordinal * tupleByteSize + fieldIndex * wordSize);
    }
    return result;
}

std::optional<ResolvedValue> readTupleField(const FontOptionsLayout& layout,
    const records::LegacyRecordIndex& index, const SourceProfile& profile,
    std::size_t ordinal, TupleField field)
{
    return readSourceValue(index, layout.encoding, GLOBALS_CMPER,
        sourceFor(layout, ordinal, field), profile.byteOrder);
}

std::optional<ResolvedValue> readEarlyTupleField(const records::LegacyRecordIndex& index,
    const EarlyTuple& tuple, TupleField field, ByteOrder byteOrder)
{
    const auto& fieldSource = tuple.fields[static_cast<std::size_t>(field)];
    SourceLocation source;
    source.identity = fieldSource.identity;
    source.selector = GLOBALS_CMPER;
    source.incidence = 0;
    source.wordSlot = fieldSource.word;
    source.width = ValueWidth::Word;
    return readSourceValue(index, RecordEncoding::FixedRow, GLOBALS_CMPER,
        source, byteOrder);
}

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
void reportField(ImportReport& report, FontType type, const char* member,
    ValueOrigin origin, std::int64_t rawValue,
    std::size_t blockOffset = 0, std::size_t decodedOffset = 0)
{
    FINALE_MUS_READER_REPORT_FIELD(report, instanceKey<FontOptionsTarget>(),
        "fonts[" + std::to_string(static_cast<std::size_t>(type)) + "]." + member,
        {origin, blockOffset, decodedOffset, rawValue});
}

void reportPhysicalField(ImportReport& report, std::size_t ordinal, const char* member,
    const ResolvedValue& source, std::int64_t rawValue)
{
    FINALE_MUS_READER_REPORT_FIELD(report, instanceKey<FontOptionsTarget>(),
        "physical[" + std::to_string(ordinal) + "]." + member,
        {ValueOrigin::LegacyMus, source.blockOffset, source.decodedOffset, rawValue});
}

void reportPhysicalTuple(ImportReport& report, std::size_t ordinal,
    const ResolvedValue& fontId, const ResolvedValue& size, const ResolvedValue& effects)
{
    reportPhysicalField(report, ordinal, "fontId", fontId,
        static_cast<std::uint16_t>(fontId.value));
    reportPhysicalField(report, ordinal, "fontSize", size,
        static_cast<std::int16_t>(size.value));
    reportPhysicalField(report, ordinal, "effects", effects,
        static_cast<std::uint16_t>(effects.value));
}
#else
#define reportField(...) ((void)0)
#define reportPhysicalTuple(...) ((void)0)
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

/// @brief First major version whose physical font ordinals match the modern enum.
/// @details Finale 2012 renumbered the tail of the array: through Finale 2011 physical 13 is
/// a holding slot and physical 28 carries the default tablature font, and from Finale 2012
/// physical 13 is tablature and 28 is percussion. The boundary is named rather than written
/// as a literal because it is the whole content of the rule.
///
/// This is deliberately NOT shared with @ref versions::firstUnicodeMajorVersion even though
/// both fall at major 17. Nothing establishes a common cause -- this is an array
/// renumbering, not a text-encoding change -- and merging them would assert one.
constexpr std::uint8_t firstModernOrdinalMajorVersion = 17; // Finale 2012

/// @brief Maps a stored tuple position to the modern FontType it means.
/// @details Two layouts exist, and which applies is decided by epoch first. The uncompressed
/// and DCL epochs are entirely pre-2012, so they need no version test at all and keep working
/// on a file whose header version cannot be recovered. Only the zlib epoch spans the boundary,
/// so only it carries a version gate, framed inside that epoch.
///
/// Epoch has to lead here rather than version: major 12 occurs in both the DCL epoch
/// (Finale 2006) and the zlib epoch (Finale 2007), so no version range separates them.
std::optional<FontType> semanticType(
    const SourceProfile& profile, std::size_t physicalOrdinal)
{
    // **This boundary is Finale 2012 by measurement, NOT by documentation.** Finale's own
    // documentation places the 13/28 renumbering at Finale 2003. Documents say otherwise:
    // through Finale 2011 tablature is at physical ordinal 28, and from Finale 2012 it is at 13
    // with percussion at 28. If the 2003 claim turns up in some MakeMusic source, it does not
    // override this -- check a file. Taking the boundary at Finale 2003 costs every 2003-2011
    // document its tablature font and gives it a bogus percussion font.
    const bool modernOrdinals = profile.epoch == FormatEpoch::ZlibLegacy
        && profile.version
        && profile.version->major >= firstModernOrdinalMajorVersion;
    if (modernOrdinals) {
        if (physicalOrdinal < fontTypeCount) {
            return static_cast<FontType>(physicalOrdinal);
        }
        return std::nullopt;
    }

    // A zlib file whose version will not parse falls here, which is the safe direction: the
    // pre-2012 layout is what every epoch before it uses, and the epochs that cannot reach
    // 2012 at all never consult the version.
    if (physicalOrdinal == 13) {
        return std::nullopt;
    }
    if (physicalOrdinal == 28) {
        return FontType::Tablature;
    }
    if (physicalOrdinal < fontTypeCount) {
        return static_cast<FontType>(physicalOrdinal);
    }
    return std::nullopt;
}

bool isZeroTuple(const ResolvedValue& fontId,
    const ResolvedValue& size, const ResolvedValue& effects)
{
    return fontId.value == 0 && size.value == 0 && effects.value == 0;
}

bool isStructuralTupleFill(std::size_t tupleCount, std::size_t physicalOrdinal,
    const ResolvedValue& fontId,
    const ResolvedValue& size, const ResolvedValue& effects)
{
    // A fixed 16-byte other row has a four-byte header and room for exactly two
    // six-byte tuples. The zlib representation preserves the same 12-byte tuple-pair
    // grouping, consistent with Finale's plug-in-facing Enigma record model. When the
    // logical collection has an odd size, Finale zero-fills the second tuple of the final
    // pair. This is structural completion, not a terminator or a semantic tuple limit.
    return physicalOrdinal + 1 == tupleCount
        && physicalOrdinal % 2 == 1
        && isZeroTuple(fontId, size, effects);
}

void insertRecoveredTuple(const musx::dom::DocumentPtr& document,
    const std::shared_ptr<FontOptionsTarget>& target, FontType type,
    const ResolvedValue& fontId, const ResolvedValue& size,
    const ResolvedValue& effects, ImportReport& report
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    , ValueOrigin origin = ValueOrigin::LegacyMus
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    )
{
    auto font = std::make_shared<FontInfo>(document);
    font->fontId = musx::dom::Cmper(static_cast<std::uint16_t>(fontId.value));
    font->fontSize = static_cast<std::int16_t>(size.value);
    font->setEnigmaStyles(static_cast<std::uint16_t>(effects.value));
    target->fontOptions.insert_or_assign(type, std::move(font));

    reportField(report, type, "fontId", origin,
        static_cast<std::uint16_t>(fontId.value), fontId.blockOffset, fontId.decodedOffset);
    reportField(report, type, "fontSize", origin,
        static_cast<std::int16_t>(size.value), size.blockOffset, size.decodedOffset);
    reportField(report, type, "effects", origin,
        static_cast<std::uint16_t>(effects.value), effects.blockOffset, effects.decodedOffset);
}

/// @brief Resolves a reference font into this document, reporting when it cannot be done.
/// @details musxdom owns the rule: match by normalized name, never by comparator, and treat 0 as
/// the default-music-font sentinel. This only adapts the result to the importer's reporting, which
/// is why the legacy side keeps no font-matching logic of its own.
musx::dom::Cmper resolveReferenceFont(const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument, musx::dom::Cmper referenceId,
    ImportReport& report)
{
    const auto referenceFont = referenceDocument->getOthers()->get<FontDefinition>(
        musx::dom::SCORE_PARTID, referenceId);
    if (!referenceFont) {
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
            "Finale 27 FontOptions referenced missing font definition "
            + std::to_string(referenceId) + "; substituted font id 0."});
        return 0;
    }
    if (const auto resolved = musx::dom::importFontDefinitionInto(
            document, referenceFont, baselineObjectReporter(report))) {
        return *resolved;
    }
    report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,
        "No free font comparator remained for Finale 27 font \"" + referenceFont->name
        + "\"; substituted font id 0."});
    return 0;
}

/// @brief Restates an already-reported FontOptions field as coming from the reference.
/// @details The capture pass reports each recovered tuple as @ref ValueOrigin::LegacyMus.
/// When the substitution below replaces one, that report becomes untrue, so the existing
/// entry is rewritten in place rather than appended to: a second entry for the same target
/// would leave the diagnostics claiming both origins at once.
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
void retargetReportedOrigin(ImportReport& report, FontType type,
    const char* member, std::int64_t rawValue)
{
    const auto target = "fonts[" + std::to_string(static_cast<std::size_t>(type))
        + "]." + member;
    if (auto* info = report.findField(instanceKey<FontOptionsTarget>(), target)) {
        info->origin = ValueOrigin::Finale27Default;
        info->rawValue = rawValue;
        // The value no longer comes from anywhere in the source file.
        info->blockOffset = 0;
        info->decodedOffset = 0;
    }
}
#else
#define retargetReportedOrigin(...) ((void)0)
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

void repairMissingRecoveredFontDefinitionsImpl(const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument,
    const std::shared_ptr<FontOptionsTarget>& target, ImportReport& report)
{
    const auto reference = referenceDocument->getOptions()->get<FontOptionsTarget>();
    if (!reference) {
        throw std::logic_error("FontOptions reference document is incomplete");
    }

    for (const auto& [type, font] : target->fontOptions) {
        const auto missingId = font->fontId;
        if (document->getOthers()->get<FontDefinition>(
                musx::dom::SCORE_PARTID, missingId)) {
            continue;
        }

        // The missing MUS definition leaves no name that can be reconciled. Select the
        // reference definition by semantic FontOptions type instead. That selected face
        // may reuse an existing target definition by normalized name; otherwise it is
        // cloned into the next sequential target comparator.
        //
        // The whole tuple is taken, not just the face. Point size is not independent of
        // the face it was chosen for -- the same number renders at visibly different sizes
        // across faces, and the effects mask can be equally face-specific -- so pairing a
        // substituted face with the source's size produces a combination that existed in
        // neither document. The source tuple has already been judged untrustworthy by the
        // time this runs, since its font id names nothing.
        //
        // It also removes the era's worst recovered values. Sixteen documents carry a
        // negative fretboard point size, and every one of them reaches it through a
        // dangling font id; Finale 27 rewrites those to zero, which is no more renderable
        // than the negative and is a guard for its own renderer rather than a judgment
        // about the value. Taking the reference size yields the only usable result.
        const auto referenceFont = reference->getFontInfo(type);
        const auto resolvedId = resolveReferenceFont(
            document, referenceDocument, referenceFont->fontId, report);
        auto replacement = std::make_shared<FontInfo>(document);
        replacement->fontId = resolvedId;
        replacement->fontSize = referenceFont->fontSize;
        replacement->setEnigmaStyles(referenceFont->getEnigmaStyles());
        target->fontOptions.insert_or_assign(type, std::move(replacement));

        // Nothing in this tuple came from the source any more, so the report must stop saying
        // it did: leaving these as LegacyMus would make a substituted value look recovered.
        retargetReportedOrigin(report, type, "fontId", resolvedId);
        retargetReportedOrigin(report, type, "fontSize", referenceFont->fontSize);
        retargetReportedOrigin(report, type, "effects", referenceFont->getEnigmaStyles());

        // Verbose, not a warning: a designed-in fallback that leaves the document usable.
        // The three retargeted entries above already say Finale27Default, which is the
        // machine-readable form of the same fact, so this exists only for someone reading
        // a log to understand why.
        report.diagnostics.push_back({musx::util::Logger::LogLevel::Verbose,
            "Legacy FontOptions type "
            + std::to_string(static_cast<std::size_t>(type))
            + " referenced missing font definition " + std::to_string(missingId)
            + "; used the same-type Finale 27 reference font, size and effects as target"
              " font id " + std::to_string(resolvedId) + '.'});
    }
}

void completeFromReference(const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument,
    const std::shared_ptr<FontOptionsTarget>& target, ImportReport& report)
{
    const auto reference = referenceDocument->getOptions()->get<FontOptionsTarget>();
    if (!reference) {
        throw std::logic_error("FontOptions reference document is incomplete");
    }

    for (std::size_t ordinal = 0; ordinal < fontTypeCount; ++ordinal) {
        const auto type = static_cast<FontType>(ordinal);
        if (target->fontOptions.contains(type)) {
            continue;
        }
        const auto source = reference->getFontInfo(type);
        auto font = std::make_shared<FontInfo>(document);
        font->fontId = resolveReferenceFont(
            document, referenceDocument, source->fontId, report);
        font->fontSize = source->fontSize;
        font->setEnigmaStyles(source->getEnigmaStyles());
        target->fontOptions.emplace(type, font);

        reportField(report, type, "fontId", ValueOrigin::Finale27Default, font->fontId);
        reportField(report, type, "fontSize", ValueOrigin::Finale27Default, font->fontSize);
        reportField(report, type, "effects", ValueOrigin::Finale27Default,
            font->getEnigmaStyles());
    }
}

} // namespace

void repairMissingRecoveredFontDefinitions(const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument,
    const std::shared_ptr<FontOptionsTarget>& target, ImportReport& report)
{
    repairMissingRecoveredFontDefinitionsImpl(
        document, referenceDocument, target, report);
}

void captureFontOptions(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument, ImportReport& report)
{
    auto target = std::make_shared<FontOptionsTarget>(document);

    // The epoch alone, with no version test. The range this used to carry, 1.0 through 2.ff,
    // is simply a restatement of the Coda-banner era, so it could never exclude a Mac document
    // -- but it excluded every Windows one, because that era states a platform where it states
    // a version and `PC 1.0+` yields none. The tuple locations below hold for both platforms.
    if (profile.epoch == FormatEpoch::CodaBanner) {
        for (const auto& tuple : earlyCodaTuples) {
            const auto fontId = readEarlyTupleField(
                index, tuple, TupleField::FontId, profile.byteOrder);
            const auto size = readEarlyTupleField(
                index, tuple, TupleField::Size, profile.byteOrder);
            const auto effects = readEarlyTupleField(
                index, tuple, TupleField::Effects, profile.byteOrder);
            if (fontId && size && effects) {
                insertRecoveredTuple(document, target, tuple.type,
                    *fontId, *size, *effects, report);
                if (tuple.type == FontType::StaffNames) {
                    for (const auto companion : codaNameCompanionTypes) {
                        insertRecoveredTuple(document, target, companion,
                            *fontId, *size, *effects, report
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
                            , ValueOrigin::LegacyBehavior
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
                        );
                    }
                }
            }
        }
    } else if (const auto* layout = layoutFor(profile)) {
        const auto count = tupleCount(*layout, index, report);
        for (std::size_t physicalOrdinal = 0; physicalOrdinal < count; ++physicalOrdinal) {
            const auto fontId = readTupleField(
                *layout, index, profile, physicalOrdinal, TupleField::FontId);
            const auto size = readTupleField(
                *layout, index, profile, physicalOrdinal, TupleField::Size);
            const auto effects = readTupleField(
                *layout, index, profile, physicalOrdinal, TupleField::Effects);
            if (!fontId || !size || !effects) {
                report.diagnostics.push_back({musx::util::Logger::LogLevel::Warning,"Ignored an incomplete legacy font-options tuple at ordinal "
                    + std::to_string(physicalOrdinal) + '.'});
                continue;
            }

            if (isStructuralTupleFill(count, physicalOrdinal,
                    *fontId, *size, *effects)) {
                reportPhysicalTuple(report, physicalOrdinal, *fontId, *size, *effects);
            } else if (const auto type = semanticType(profile, physicalOrdinal)) {
                insertRecoveredTuple(document, target, *type, *fontId, *size, *effects, report);
            } else {
                reportPhysicalTuple(report, physicalOrdinal, *fontId, *size, *effects);
                if (!isZeroTuple(*fontId, *size, *effects) && profile.epoch == FormatEpoch::ZlibLegacy) {
                    report.diagnostics.push_back({musx::util::Logger::LogLevel::Info,"Captured nonzero legacy font-options tuple "
                        + std::to_string(physicalOrdinal)
                        + " beyond musxdom's current FontType range."});
                }
            }
        }
    }

    repairMissingRecoveredFontDefinitions(
        document, referenceDocument, target, report);
    completeFromReference(document, referenceDocument, target, report);
    document->getOptions()->add(FontOptionsTarget::XmlNodeName, std::move(target));
}

/// @brief Registers the font comparator every FontOptions type finally holds.
/// @details This must run after @ref repairMissingRecoveredFontDefinitions, not alongside the
/// capture that first sets a comparator. A recovered tuple whose font id names nothing is
/// replaced wholesale by the same-type reference tuple, so the comparator capture wrote is not
/// the one the document keeps. Registering during capture would mint a placeholder definition
/// for a discarded id -- a font named `Missing Font (n)` that nothing in the document refers
/// to, which is worse than the throw it was meant to prevent because it looks like data.
///
/// The repair leaves no dangling comparator behind, so in practice this registers ids that are
/// already defined and nothing is minted. It is here because that is a property of the repair
/// pass rather than of this class, and the two are free to change independently.
void registerFontOptionFonts(const musx::dom::DocumentPtr& document,
    musx::factory::ConstructionContext& construction)
{
    const auto target = document->getOptions()->get<FontOptionsTarget>();
    if (!target) {
        return;
    }
    for (const auto& [type, font] : target->fontOptions) {
        if (font) {
            construction.registerFontId(font->fontId);
        }
    }
}

void importFontOptions(const ImportContext& context)
{
    // No mapping table: every type is either a recovered physical tuple translated through a
    // versioned semantic map or a baseline entry completed with a remapped font id, and
    // neither is a fixed source location a table row could name.
    captureFontOptions(context.index, context.profile, context.document,
        context.referenceDocument, context.report);
    registerFontOptionFonts(context.document, context.construction);
}

} // namespace options
} // namespace finale_mus_reader

#if !defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
#undef reportField
#undef reportPhysicalTuple
#undef retargetReportedOrigin
#endif // !defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
