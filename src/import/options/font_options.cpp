// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options/options.h"

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
// version range would add nothing and would cost real files: it previously read `Dcl` only,
// so every Finale 3.0 through 2000 document reported all 45 font options as Finale 27
// defaults while its source held 40 of them. Verified against the exact Finale 27 companions
// of the Finale 97 and Finale 2000 fixtures, which agree tuple for tuple.
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
// This is a deliberate divergence from the Finale 27 companions, which is recorded in
// research/FORMAT_NOTES.md and is open to revision. Those companions disagree among
// themselves about the four name types in a way that tracks the default file the upgrade was
// performed under rather than the source document, so they do not settle the question. The
// three propagated types report as ValueOrigin::LegacyBehavior, not LegacyMus: the bytes are
// read from the source, but the assignment restores an era behavior rather than an option
// the source stored.
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

    const auto* row = index.getClassRecords().get(layout.identity, GLOBALS_CMPER, 0, 0);
    if (!row) {
        return 0;
    }
    const auto bytes = index.getClassRecords().payloadOf(*row);
    if (const auto trailing = bytes.size() % tupleByteSize; trailing != 0) {
        report.warnings.push_back("Ignored " + std::to_string(trailing)
            + " trailing byte(s) after the last complete legacy font-options tuple.");
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

void reportField(ImportReport& report, FontType type, const char* member,
    ValueOrigin origin, std::int64_t rawValue,
    std::size_t blockOffset = 0, std::size_t decodedOffset = 0)
{
    FieldInfo info;
    info.target = "options.fontOptions["
        + std::to_string(static_cast<std::size_t>(type)) + "]." + member;
    info.origin = origin;
    info.blockOffset = blockOffset;
    info.decodedOffset = decodedOffset;
    info.rawValue = rawValue;
    report.fields.push_back(std::move(info));
}

void reportPhysicalField(ImportReport& report, std::size_t ordinal, const char* member,
    const ResolvedValue& source, std::int64_t rawValue)
{
    FieldInfo info;
    info.target = "options.fontOptionsPhysical[" + std::to_string(ordinal) + "]." + member;
    info.origin = ValueOrigin::LegacyMus;
    info.blockOffset = source.blockOffset;
    info.decodedOffset = source.decodedOffset;
    info.rawValue = rawValue;
    report.fields.push_back(std::move(info));
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

std::optional<FontType> semanticType(
    const SourceProfile& profile, std::size_t physicalOrdinal)
{
    if (profile.epoch == FormatEpoch::ZlibLegacy) {
        if (physicalOrdinal < fontTypeCount) {
            return static_cast<FontType>(physicalOrdinal);
        }
        return std::nullopt;
    }

    // The uncompressed epoch is entirely Finale 3.0 through 2000, so it always takes the
    // earlier semantic layout and needs no version at all. Deciding by epoch rather than by
    // version range is what keeps this true of any file the container classifies, including
    // one whose header version cannot be recovered.
    const bool earlyLayout = profile.epoch == FormatEpoch::UncompressedLegacy
        || (profile.version && profile.version->major >= 3 // Finale 3.0
            && profile.version->major <= 7); // Finale 2002
    if (earlyLayout) {
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
    if (!profile.version) {
        return std::nullopt;
    }
    if (profile.version->major >= 8 // Finale 2003
        && profile.version->major <= 11) { // Finale 2006
        if (physicalOrdinal < fontTypeCount) {
            return static_cast<FontType>(physicalOrdinal);
        }
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
    const ResolvedValue& effects, ImportReport& report,
    ValueOrigin origin = ValueOrigin::LegacyMus)
{
    auto font = std::make_shared<FontInfo>(document);
    font->fontId = static_cast<musx::dom::Cmper>(
        static_cast<std::uint16_t>(fontId.value));
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

musx::dom::Cmper cloneOrMatchFont(const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument, musx::dom::Cmper referenceId,
    std::unordered_map<std::string, musx::dom::Cmper>& targetFonts,
    std::uint32_t& nextCmper, ImportReport& report)
{
    if (referenceId == 0) {
        return 0;
    }
    const auto referenceFont = referenceDocument->getOthers()->get<FontDefinition>(
        musx::dom::SCORE_PARTID, referenceId);
    if (!referenceFont) {
        report.warnings.push_back("Finale 27 FontOptions referenced missing font definition "
            + std::to_string(referenceId) + "; substituted font id 0.");
        return 0;
    }

    const auto key = musx::dom::normalizeFontName(referenceFont->name);
    if (const auto found = targetFonts.find(key); found != targetFonts.end()) {
        return found->second;
    }
    if (nextCmper > (std::numeric_limits<musx::dom::Cmper>::max)()) {
        report.warnings.push_back("No free font comparator remained for Finale 27 font \""
            + referenceFont->name + "\"; substituted font id 0.");
        return 0;
    }

    // The comparator belongs to the target document's id space. Never carry the
    // reference comparator across documents: allocate directly after the target's
    // highest existing font comparator.
    const auto cmper = static_cast<musx::dom::Cmper>(nextCmper++);
    auto clone = std::make_shared<FontDefinition>(document, musx::dom::SCORE_PARTID,
        musx::dom::EnigmaBase::ShareMode::All, cmper);
    clone->charsetBank = referenceFont->charsetBank;
    clone->charsetVal = referenceFont->charsetVal;
    clone->pitch = referenceFont->pitch;
    clone->family = referenceFont->family;
    // Normalization is only a matching rule. A definition newly introduced from the
    // platform baseline retains that baseline's exact spelling in the output document.
    clone->name = referenceFont->name;
    document->getOthers()->add(FontDefinition::XmlNodeName, clone);
    targetFonts.emplace(key, cmper);
    return cmper;
}

struct TargetFontState
{
    std::unordered_map<std::string, musx::dom::Cmper> byName;
    std::uint32_t nextCmper = 1;
};

TargetFontState collectTargetFonts(const musx::dom::DocumentPtr& document)
{
    TargetFontState result;
    // Comparator zero has default-music semantics and is never a safe destination for a
    // concrete nonzero font copied from the reference document.
    for (const auto& font : document->getOthers()
            ->getArray<FontDefinition>(musx::dom::SCORE_PARTID)) {
        if (font->getCmper() != 0) {
            const auto key = musx::dom::normalizeFontName(font->name);
            const auto found = result.byName.find(key);
            if (found == result.byName.end() || font->getCmper() < found->second) {
                result.byName.insert_or_assign(key, font->getCmper());
            }
        }
        if (font->getCmper() >= result.nextCmper) {
            result.nextCmper = static_cast<std::uint32_t>(font->getCmper()) + 1;
        }
    }
    return result;
}

void repairMissingRecoveredFontDefinitionsImpl(const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument,
    const std::shared_ptr<FontOptionsTarget>& target, ImportReport& report)
{
    const auto reference = referenceDocument->getOptions()->get<FontOptionsTarget>();
    if (!reference) {
        throw std::logic_error("FontOptions reference document is incomplete");
    }

    auto targetFonts = collectTargetFonts(document);
    for (const auto& [type, font] : target->fontOptions) {
        const auto missingId = font->fontId;
        if (missingId == 0 || document->getOthers()->get<FontDefinition>(
                musx::dom::SCORE_PARTID, missingId)) {
            continue;
        }

        // The missing MUS definition leaves no name that can be reconciled. Select the
        // reference definition by semantic FontOptions type instead. That selected face
        // may reuse an existing target definition by normalized name; otherwise it is
        // cloned into the next sequential target comparator.
        const auto referenceFont = reference->getFontInfo(type);
        const auto resolvedId = cloneOrMatchFont(document, referenceDocument,
            referenceFont->fontId, targetFonts.byName, targetFonts.nextCmper, report);
        auto replacement = std::make_shared<FontInfo>(document);
        replacement->fontId = resolvedId;
        replacement->fontSize = font->fontSize;
        replacement->setEnigmaStyles(font->getEnigmaStyles());
        target->fontOptions.insert_or_assign(type, std::move(replacement));
        report.warnings.push_back("Legacy FontOptions type "
            + std::to_string(static_cast<std::size_t>(type))
            + " referenced missing font definition " + std::to_string(missingId)
            + "; used same-type Finale 27 reference font as target font id "
            + std::to_string(resolvedId) + '.');
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

    auto targetFonts = collectTargetFonts(document);

    for (std::size_t ordinal = 0; ordinal < fontTypeCount; ++ordinal) {
        const auto type = static_cast<FontType>(ordinal);
        if (target->fontOptions.contains(type)) {
            continue;
        }
        const auto source = reference->getFontInfo(type);
        auto font = std::make_shared<FontInfo>(document);
        font->fontId = cloneOrMatchFont(document, referenceDocument, source->fontId,
            targetFonts.byName, targetFonts.nextCmper, report);
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
    document->getOptions()->add(FontOptionsTarget::XmlNodeName, target);

    // The epoch alone, with no version test. The range this used to carry, 1.0 through 2.ff,
    // is simply a restatement of the Coda-banner era, so it could never exclude a Mac document
    // -- but it excluded every Windows one, because that era states a platform where it states
    // a version and `PC 1.0+` yields none. The tuple locations below hold for both: all 24
    // Windows documents agree with their exact Finale 27 companions on every recovered type.
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
                            *fontId, *size, *effects, report,
                            ValueOrigin::LegacyBehavior);
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
                report.warnings.push_back("Ignored an incomplete legacy font-options tuple at ordinal "
                    + std::to_string(physicalOrdinal) + '.');
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
                    report.warnings.push_back("Captured nonzero legacy font-options tuple "
                        + std::to_string(physicalOrdinal)
                        + " beyond musxdom's current FontType range.");
                }
            }
        }
    }

    repairMissingRecoveredFontDefinitions(
        document, referenceDocument, target, report);
    completeFromReference(document, referenceDocument, target, report);
}

} // namespace options
} // namespace finale_mus_reader
