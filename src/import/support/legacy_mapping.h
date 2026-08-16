// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <functional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "finale_mus_reader/reader.h"
#include "musx/dom/Document.h"
#include "musx/dom/Fundamentals.h"
#include "records/legacy_record_index.h"

namespace finale_mus_reader {


/// @brief Width of a mapped value in the source record.
enum class ValueWidth : std::uint8_t
{
    Byte = 1,
    Word = 2,
    Long = 4
};

/// @brief Which of a four-byte value's two payload words comes first.
/// @details The distilled framework mapping names these `MACFOURBYTE` and `WINFOURBYTE`.
/// This is independent of container byte order, which the record index has already
/// applied when reading each individual word.
enum class LongWordOrder : std::uint8_t
{
    HighFirst,
    LowFirst
};

/// @brief Selects a bit range within the source value. A zero count means the whole value.
struct BitRange
{
    std::uint8_t firstBit{};
    std::uint8_t bitCount{};
};

/// @brief Which record encoding a table reads, and therefore how its fields are addressed.
enum class RecordEncoding : std::uint8_t
{
    /// @brief Tagged 16-byte rows, every epoch through Finale 2006. Fields address a word
    /// slot in the family's incidence stream, so a value may straddle an incidence.
    FixedRow,
    /// @brief Class-identified variable-length records, Finale 2007 and later. Fields
    /// address a byte offset inside a single record's payload.
    ClassRecord
};

/// @brief Where one field lives in the legacy record stream.
struct SourceLocation
{
    /// @brief The record identity: a packed two-character tag, or a numeric class id.
    records::LegacyTag identity{};
    /// @brief The record comparator. Ignored for @ref TargetKind::OthersByCmper tables,
    /// where the comparator of the target object is used instead.
    std::uint16_t selector{};
    std::uint32_t incidence{};
    /// @brief Word slot for @ref RecordEncoding::FixedRow, byte offset for
    /// @ref RecordEncoding::ClassRecord.
    std::uint32_t wordSlot{};
    ValueWidth width = ValueWidth::Word;
    LongWordOrder longOrder = LongWordOrder::HighFirst;
    BitRange bits{};
};

/// @brief The comparator that legacy synthetic preference records select.
/// @details Aliased from musxdom so mapping tables read in one column width.
inline constexpr musx::dom::Cmper GLOBALS_CMPER = musx::dom::MUSX_GLOBALS_CMPER;

/// @brief Placeholder selector for rows whose comparator comes from the target object.
inline constexpr musx::dom::Cmper CMPER_FROM_TARGET = 0;

/// @brief Converts a fixed-row numeric global selector to its zlib-era class id.
/// @details The zlib serialization retained the logical option identities and added
/// `0x000e` while coalescing each incidence family into one class-record payload.
[[nodiscard]] constexpr records::LegacyTag numericGlobalClass(std::uint16_t selector)
{
    return static_cast<records::LegacyTag>(selector + 0x000eU);
}

/// @brief The fixed-row tag of a numeric global: the two decimal characters of its selector.
/// @details A numeric global is not a record type of its own. Through Finale 2006 it is
/// spelled as the digits ETF prints, so selector 40 is the tag `40`, and only from Finale
/// 2007 does it become the class id @ref numericGlobalClass derives. Deriving the spelling
/// keeps a selector stated once as a number rather than again as a two-character literal.
[[nodiscard]] records::LegacyTag numericGlobalTag(std::uint16_t selector);

/// @brief One numeric global's whole incidence family, read as a single word stream.
struct GlobalSelectorWords
{
    /// @brief Every payload word of every incidence, in incidence order.
    /// @details A logical element may therefore straddle an incidence boundary, and a
    /// collection is simply as many elements as the stream holds.
    std::vector<std::int16_t> words;
    /// @brief Whether the family is present at all, which zero words cannot express.
    bool present{};
    std::size_t blockOffset{};
    std::size_t decodedOffset{};
};

/// @brief Collects a numeric global's fixed-row incidence family into one word stream.
/// @details Offsets come from the first incidence, which is where the family begins.
[[nodiscard]] GlobalSelectorWords readNumericGlobalWords(
    const records::LegacyRecordIndex& index, std::uint16_t selector);

/// @brief The byte offset a class record keeps the given word slot at.
/// @details The zlib eras address the same option by byte where the fixed rows address it by
/// word slot, so a class table's offsets are its fixed-row slots doubled. Stating that once
/// keeps a table from spelling out byte numbers whose relationship to the slots is silent.
[[nodiscard]] constexpr std::uint32_t classWordOffset(std::uint32_t slot)
{
    return slot * 2U;
}

/// @brief One word of a stream, or zero past its end.
/// @details Every collection decoder reads a fixed shape out of a stream whose length is the
/// file's business, so running off the end is an ordinary event and yields the same zero a
/// short final element would carry.
[[nodiscard]] std::int16_t wordAt(const std::vector<std::int16_t>& words, std::size_t index);

/// @brief Reads a class-record payload as signed words in the file's own byte order.
/// @details The zlib eras keep the word stream the fixed rows carried and merely coalesce
/// it into one payload, so a collection decoder can work in words for every era rather than
/// once per encoding. A trailing odd byte cannot belong to a word and is dropped.
[[nodiscard]] std::vector<std::int16_t> payloadWords(
    std::span<const std::uint8_t> payload, ByteOrder byteOrder);

/// @brief The 8-bit character a pre-Unicode record stores in a 16-bit word.
/// @details A source may store one either zero-extended or sign-extended: the same
/// character 139 appears as `0x008b` in some files and `0xff8b` in others. Narrowing to the
/// low byte is what makes the two spellings agree. It must not be applied to a codepoint
/// that a Unicode-era record stores as a long.
[[nodiscard]] std::uint32_t narrowCodepoint(std::int16_t stored);

/// @brief The codepoint a Unicode-era record stores across two consecutive words.
/// @details The words are in the order the container has already normalized each one into,
/// and the low half comes first. No big-endian Finale 2012 specimen exists in any surveyed
/// corpus, so that half of the rule is untested and likely to stay that way: such a file
/// needs the 2012 release on a PowerPC Mac, which its stated requirements allow only by
/// implication and the operating systems mostly forbid. A caller that can reach the
/// big-endian case should say so rather than rely on the absence.
[[nodiscard]] std::uint32_t wideCodepoint(std::int16_t low, std::int16_t high);

/// @brief One end of a version gate, ordered by major then minor.
/// @details Minor participates because the major version alone does not order Finale's
/// whole history: Finale 97 and the Finale 3.x line both appear to carry major 3, so an
/// early gate needs the minor version to separate them. Later eras are expected to need
/// only the major, which is why minor defaults at both ends.
struct VersionBound
{
    std::uint8_t major{};
    std::uint8_t minor{};

    [[nodiscard]] std::uint16_t key() const
    {
        return static_cast<std::uint16_t>((static_cast<std::uint16_t>(major) << 8U) | minor);
    }
};

/// @brief An inclusive version range, both bounds optional.
/// @details Gating is on the version embedded in the file, never on a number synthesized
/// from the banner product text. A file whose version could not be recovered, which
/// includes any file whose header tuple could not be read, matches only an
/// unrestricted range.
struct VersionRange
{
    std::optional<VersionBound> minVersion;
    std::optional<VersionBound> maxVersion;

    [[nodiscard]] bool unrestricted() const { return !minVersion && !maxVersion; }

    [[nodiscard]] bool includes(const std::optional<SourceVersion>& version) const
    {
        if (unrestricted()) {
            return true;
        }
        if (!version) {
            return false;
        }
        const auto key = VersionBound{version->major, version->minor}.key();
        if (minVersion && key < minVersion->key()) {
            return false;
        }
        return !maxVersion || key <= maxVersion->key();
    }
};

/// @brief Convenience constructors for version gates.
namespace versions {
[[nodiscard]] inline VersionRange any() { return {}; }

/// @brief This version and later. Omitting the minor starts at the whole major version.
[[nodiscard]] inline VersionRange from(std::uint8_t major, std::uint8_t minor = 0)
{
    return {VersionBound{major, minor}, std::nullopt};
}

/// @brief This version and earlier. Omitting the minor includes every minor of the major.
[[nodiscard]] inline VersionRange upTo(std::uint8_t major, std::uint8_t minor = 0xff)
{
    return {std::nullopt, VersionBound{major, minor}};
}

/// @brief An inclusive span between two bounds.
[[nodiscard]] inline VersionRange between(VersionBound minVersion, VersionBound maxVersion)
{
    return {minVersion, maxVersion};
}

/// @brief The first Enigma major version that stores a symbol codepoint as a long.
/// @details Finale 2012 gained Unicode text, and every option that names a glyph widened
/// with it: the clef table's character and the stem-connection table's symbol were both
/// 8-bit characters stored in a 16-bit word, and both became full 32-bit codepoints
/// occupying two words, shifting every field after them in their tuple.
///
/// This is one boundary shared by several classes, not a general "Finale 2012" constant.
/// The font-ordinal renumbering in font_options.cpp also falls at major 17 and is
/// deliberately kept separate: it is an array renumbering rather than a text-encoding
/// change, and nothing establishes a common cause that a shared constant would assert.
inline constexpr std::uint8_t firstUnicodeMajorVersion = 17; // Finale 2012

/// @brief Whether a source stores symbol codepoints as longs rather than as words.
/// @details A version that could not be recovered reads as pre-Unicode. That is the safe
/// direction and costs nothing: only the zlib epoch reaches Finale 2012 at all, and every
/// earlier epoch stores the narrow form regardless of what its header says.
[[nodiscard]] inline bool storesUnicodeCodepoints(const std::optional<SourceVersion>& version)
{
    return version && version->major >= firstUnicodeMajorVersion;
}
} // namespace versions

/// @brief Which format epochs a table applies to.
enum class EpochMask : std::uint8_t
{
    None = 0,
    CodaBanner = 1U << 0,
    Uncompressed = 1U << 1,
    Dcl = 1U << 2,
    Zlib = 1U << 3,
    /// @brief The eras whose pools resolve to fixed 16-byte Enigma rows.
    FixedRow = Uncompressed | Dcl,
    Any = CodaBanner | Uncompressed | Dcl | Zlib
};

[[nodiscard]] constexpr EpochMask operator|(EpochMask left, EpochMask right)
{
    return static_cast<EpochMask>(
        static_cast<std::uint8_t>(left) | static_cast<std::uint8_t>(right));
}

[[nodiscard]] bool epochMatches(EpochMask mask, FormatEpoch epoch);

/// @brief The classified properties of the source file that gate mapping rows.
struct SourceProfile
{
    FormatEpoch epoch = FormatEpoch::Unknown;
    std::optional<SourceVersion> version;
    ByteOrder byteOrder = ByteOrder::Unknown;
    SourcePlatform platform = SourcePlatform::Unknown;
};

/// @brief A decoded mapping value together with the physical row that supplied it.
struct ResolvedValue
{
    std::int64_t value{};
    std::size_t blockOffset{};
    std::size_t decodedOffset{};
};

/// @brief Reads one numeric value through the common fixed-row/class-record machinery.
/// @details Dynamic collection mappings use this rather than duplicating bounds checks,
/// byte-order handling, long-word assembly, or provenance rules.
[[nodiscard]] std::optional<ResolvedValue> readSourceValue(
    const records::LegacyRecordIndex& index, RecordEncoding encoding,
    std::uint16_t cmper, const SourceLocation& source, ByteOrder byteOrder);

/// @brief Assigns a decoded value to a member, converting through the member's own type.
template <typename T>
void assignFrom(T& target, std::int64_t value)
{
    target = static_cast<T>(value);
}

/// @brief Reads a member as a report value.
template <typename T>
[[nodiscard]] std::int64_t readAs(const T& source)
{
    return static_cast<std::int64_t>(source);
}

/// @brief What a mapped field reads out of the record stream.
enum class FieldKind : std::uint8_t
{
    /// @brief One numeric value from a word slot, optionally a bit range of it.
    Number,
    /// @brief Text assembled from every incidence at or after @ref SourceLocation::incidence.
    /// @details Legacy strings run across incidences rather than living in one row: a font
    /// name occupies the rows after its header, and header text blocks span four rows each.
    /// The payload is read as bytes and truncated at the first NUL.
    Text
};

/// @brief One mapped field: where it comes from and where it goes.
struct FieldMapping
{
    const char* fieldName{};
    FieldKind kind = FieldKind::Number;
    SourceLocation source{};
    VersionRange versions{};
    void (*apply)(void* instance, std::int64_t value){};
    /// @brief Reads the seeded default, so the report can record a synthesized value
    /// without a separately maintained list of supported fields.
    std::int64_t (*read)(const void* instance){};
    void (*applyText)(void* instance, std::string_view value){};
};

/// @brief How a table finds the objects it writes to.
enum class TargetKind : std::uint8_t
{
    /// @brief A single options object; rows use their own selector.
    OptionsSingleton,
    /// @brief Pooled others objects that the pinned baseline already seeded. Each target's
    /// comparator is used as the selector, and a record with no seeded object is skipped.
    OthersByCmper,
    /// @brief Pooled others objects created from the records themselves, one per comparator
    /// carried by the table's tag. Used where the legacy file is the only source of the
    /// objects, so there is no seeded default to overlay and nothing to report as
    /// synthesized.
    OthersFromRecords
};

/// @brief One resolved destination object.
struct MappingTarget
{
    std::uint16_t cmper{};
    void* instance{};
};

/// @brief A mapping table: one musxdom class, one applicability gate, a set of fields.
/// @details Several tables may target the same class. Every table whose gate matches is
/// applied in registry order, and a later row supersedes an earlier row for the same
/// field name, so a field that moves in a later version costs a one-row override table
/// rather than a restatement of the whole table.
struct MappingTable
{
    /// @brief Report target prefix, also the identity used when layering tables.
    const char* reportPrefix{};
    EpochMask epochs = EpochMask::FixedRow;
    VersionRange versions{};
    /// @brief Optional extra test, for a boundary neither the epoch nor the version states.
    /// @details Some layouts change at a release that sits inside one epoch, at a version no
    /// surveyed file occupies, or in a way the record stream states more directly than the
    /// header does. Where the stream says which layout a file uses, a predicate reads that
    /// instead of dating the file, which is the same preference for self-description that
    /// decides the clef tuple width from its payload size. Prefer an epoch gate to this, and
    /// this to a version gate.
    bool (*applies)(const records::LegacyRecordIndex& index, const SourceProfile& profile){};
    RecordEncoding encoding = RecordEncoding::FixedRow;
    TargetKind targetKind = TargetKind::OptionsSingleton;
    std::vector<MappingTarget> (*enumerateTargets)(const musx::dom::DocumentPtr& document){};
    /// @brief Record identity whose comparators enumerate the objects, for @ref
    /// TargetKind::OthersFromRecords.
    records::LegacyTag recordIdentity{};
    /// @brief Creates and pools one object, for @ref TargetKind::OthersFromRecords.
    void* (*createTarget)(const musx::dom::DocumentPtr& document, std::uint16_t cmper){};
    const FieldMapping* fields{};
    std::size_t fieldCount{};
    /// @brief Optional pass over one target after every field of this table has been applied.
    /// @details For work that needs more than one recovered field at once and therefore
    /// cannot be expressed as a field mapping. Converting a legacy font name to UTF-8 is the
    /// motivating case: the encoding is named by the charset fields of the same record, so
    /// the name cannot be converted until both are in hand. Doing it here rather than inside
    /// the name's own apply keeps the reader from depending on the order fields happen to be
    /// declared in.
    void (*finalizeTarget)(void* instance, const SourceProfile& profile){};
};

/// @brief Creates one others object of type T and adds it to the document pool.
template <typename T>
[[nodiscard]] void* createOthersTarget(
    const musx::dom::DocumentPtr& document, std::uint16_t cmper)
{
    auto instance = std::make_shared<T>(
        document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
    auto* raw = instance.get();
    document->getOthers()->add(T::XmlNodeName, instance);
    return raw;
}

/// @brief Enumerates the single instance of an options class, if it was seeded.
template <typename T>
[[nodiscard]] std::vector<MappingTarget> enumerateOptionsTarget(
    const musx::dom::DocumentPtr& document)
{
    std::vector<MappingTarget> result;
    if (const auto instance = document->getOptions()->template get<T>()) {
        // Pool instances are handed out const. Overlaying legacy values is the one
        // reason this library writes to them, so the cast is confined to here.
        result.push_back({0, const_cast<T*>(instance.get())});
    }
    return result;
}

/// @brief Enumerates every seeded instance of an others class, in comparator order.
/// @details Targets come from the document rather than from the record stream, so a
/// file that lacks a record still reports the seeded default, and a record with no
/// seeded target is skipped rather than fabricated.
template <typename T>
[[nodiscard]] std::vector<MappingTarget> enumerateOthersTargets(
    const musx::dom::DocumentPtr& document)
{
    std::vector<MappingTarget> result;
    for (const auto& instance : document->getOthers()->template getArray<T>(musx::dom::SCORE_PARTID)) {
        result.push_back({instance->getCmper(), const_cast<T*>(instance.get())});
    }
    return result;
}

/// @brief Applies an explicit list of tables to a seeded document.
/// @details Tables are layered in list order. Every field of every listed table is
/// reported, whether or not this file can supply it; the gates decide only what is read.
/// Recovered values carry @ref ValueOrigin::LegacyMus with the offsets they came from,
/// and everything else carries the seeded default as @ref ValueOrigin::Finale27Default.
void applyMappingTables(const std::vector<const MappingTable*>& tables,
    const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document, ImportReport& report);

/// @brief A reference-document object a target field needs, resolved after the pools are complete.
/// @details Copying an object out of the reference allocates comparators in the target, so it
/// cannot run while the target's own pools are still being filled: a comparator handed out early
/// could collide with one the source is about to claim. Capture therefore records what it needs
/// and leaves the field at zero, and one phase at the end resolves every request at once.
///
/// @ref assign writes the resolved comparator wherever it belongs, so the queue stays ignorant of
/// what is waiting on it. Clef shapes are the only user today; nothing about it is clef-specific.
struct PendingShapeReference
{
    /// @brief The shape's comparator in the reference document, meaningless in the target.
    musx::dom::Cmper referenceShapeId{};
    /// @brief Writes the resolved target comparator into the field that needs it.
    std::function<void(musx::dom::Cmper)> assign;
    /// @brief The @ref FieldInfo::target whose reported value the resolution updates.
    std::string reportTarget;
};

/// @brief Requests accumulated during capture, drained by @ref resolveDeferredReferences.
using PendingReferences = std::vector<PendingShapeReference>;

/// @brief Everything the importer for one musxdom class is handed.
/// @details Passed by reference and outlived by nothing: it is built once per import and
/// exists only for the duration of the pass over the registered importers.
struct ImportContext
{
    const records::LegacyRecordIndex& index;
    const SourceProfile& profile;
    const musx::dom::DocumentPtr& document;
    /// @brief The separately owned pinned baseline, read-only. An object owned by it must
    /// never be inserted into @ref document; copy what is needed instead.
    const musx::dom::DocumentPtr& referenceDocument;
    ImportReport& report;
    /// @brief Reference objects a class needs copied, drained after every pool is filled.
    PendingReferences& pending;
};

/// @brief Recovers one musxdom class, from record identity to finished object.
/// @details One per class. An importer owns every decision its class needs -- which tables
/// apply to which epoch, which capture pass builds a collection, and what must be checked
/// afterwards -- so that the registry states only which classes are imported and in what
/// order. Nothing outside the class's own translation unit knows how many layouts it has.
using ClassImporter = void (*)(const ImportContext& context);

/// @brief Applies every registered mapping table to a seeded document.
/// @details Runs the registered importers in order, then drains @ref PendingReferences in one
/// final phase. That phase allocates `others` comparators, so nothing may allocate one after
/// this returns.
void applyLegacyMappings(const records::LegacyRecordIndex& index, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document,
    const musx::dom::DocumentPtr& referenceDocument, ImportReport& report);


} // namespace finale_mus_reader

/// @brief Declares a numeric field mapping with every column stated explicitly.
#define MUS_FIELD(Class, tagText, selectorValue, incidenceValue, slotValue, widthValue, \
                  orderValue, bitsValue, versionsValue, member) \
    ::finale_mus_reader::FieldMapping { \
        #member, \
        ::finale_mus_reader::FieldKind::Number, \
        ::finale_mus_reader::SourceLocation{ \
            ::finale_mus_reader::records::packTag(tagText), static_cast<std::uint16_t>(selectorValue), \
            static_cast<std::uint32_t>(incidenceValue), static_cast<std::uint32_t>(slotValue), \
            (widthValue), (orderValue), (bitsValue) }, \
        (versionsValue), \
        [](void* instance, std::int64_t value) { \
            ::finale_mus_reader::assignFrom( \
                static_cast<Class*>(instance)->member, value); }, \
        [](const void* instance) -> std::int64_t { \
            return ::finale_mus_reader::readAs( \
                static_cast<const Class*>(instance)->member); }, \
        nullptr \
    }

/// @brief A bit range of a class-identified record, addressed by byte offset in its payload,
/// in a record found under an explicit comparator.
#define MUS_CLASS_SELECTED_BITS(Class, classId, selectorValue, byteOffset, firstBit, \
                                bitCount, member) \
    ::finale_mus_reader::FieldMapping { \
        #member, \
        ::finale_mus_reader::FieldKind::Number, \
        ::finale_mus_reader::SourceLocation{ \
            (classId), static_cast<std::uint16_t>(selectorValue), 0, \
            static_cast<std::uint32_t>(byteOffset), \
            ::finale_mus_reader::ValueWidth::Word, \
            ::finale_mus_reader::LongWordOrder::HighFirst, \
            (::finale_mus_reader::BitRange{ \
                static_cast<std::uint8_t>(firstBit), static_cast<std::uint8_t>(bitCount)}) }, \
        ::finale_mus_reader::VersionRange{}, \
        [](void* instance, std::int64_t value) { \
            ::finale_mus_reader::assignFrom( \
                static_cast<Class*>(instance)->member, value); }, \
        [](const void* instance) -> std::int64_t { \
            return ::finale_mus_reader::readAs( \
                static_cast<const Class*>(instance)->member); }, \
        nullptr \
    }

/// @brief A bit range of a class-identified record, addressed by byte offset in its payload.
/// @details The comparator is left at zero because the tables that use this are others
/// tables, where the target object's own comparator selects the record and the field's
/// comparator is ignored. An options singleton has no such comparator and must name the
/// record's own, so it uses @ref MUS_CLASS_WORD or @ref MUS_CLASS_BIT instead.
#define MUS_CLASS_BITS(Class, classId, byteOffset, firstBit, bitCount, member) \
    MUS_CLASS_SELECTED_BITS(Class, classId, 0, byteOffset, firstBit, bitCount, member)

/// @brief A whole two-byte field of a class-identified record found under a comparator.
/// @details The zero bit count is what selects the whole value rather than a range of it,
/// and a whole value is read signed.
#define MUS_CLASS_WORD(Class, classId, selector, byteOffset, member) \
    MUS_CLASS_SELECTED_BITS(Class, classId, selector, byteOffset, 0, 0, member)

/// @brief A single bit of a class-identified record's payload word, under a comparator.
#define MUS_CLASS_BIT(Class, classId, selector, byteOffset, bitIndex, member) \
    MUS_CLASS_SELECTED_BITS(Class, classId, selector, byteOffset, bitIndex, 1, member)

/// @brief A four-byte field of a class-identified record, spanning two payload words.
/// @details The order is the mapping's own, not the container's: the zlib serialization kept
/// the word pair the fixed rows carried, so the same word-order rule applies to both.
#define MUS_CLASS_LONG(Class, classId, selector, byteOffset, order, member) \
    ::finale_mus_reader::FieldMapping { \
        #member, \
        ::finale_mus_reader::FieldKind::Number, \
        ::finale_mus_reader::SourceLocation{ \
            (classId), static_cast<std::uint16_t>(selector), 0, \
            static_cast<std::uint32_t>(byteOffset), \
            ::finale_mus_reader::ValueWidth::Long, (order), \
            ::finale_mus_reader::BitRange{} }, \
        ::finale_mus_reader::VersionRange{}, \
        [](void* instance, std::int64_t value) { \
            ::finale_mus_reader::assignFrom( \
                static_cast<Class*>(instance)->member, value); }, \
        [](const void* instance) -> std::int64_t { \
            return ::finale_mus_reader::readAs( \
                static_cast<const Class*>(instance)->member); }, \
        nullptr \
    }

/// @brief A bit range of a class-identified record, assigned through a conversion expression.
#define MUS_CLASS_BITS_AS(Class, classId, byteOffset, firstBit, bitCount, member, ...) \
    ::finale_mus_reader::FieldMapping { \
        #member, \
        ::finale_mus_reader::FieldKind::Number, \
        ::finale_mus_reader::SourceLocation{ \
            (classId), 0, 0, static_cast<std::uint32_t>(byteOffset), \
            ::finale_mus_reader::ValueWidth::Word, \
            ::finale_mus_reader::LongWordOrder::HighFirst, \
            (::finale_mus_reader::BitRange{ \
                static_cast<std::uint8_t>(firstBit), static_cast<std::uint8_t>(bitCount)}) }, \
        ::finale_mus_reader::VersionRange{}, \
        [](void* instance, std::int64_t value) { \
            static_cast<Class*>(instance)->member = (__VA_ARGS__); }, \
        [](const void* instance) -> std::int64_t { \
            return ::finale_mus_reader::readAs( \
                static_cast<const Class*>(instance)->member); }, \
        nullptr \
    }

/// @brief Text running from a byte offset to the end of a class-identified record's payload.
#define MUS_CLASS_TEXT(Class, classId, byteOffset, member) \
    ::finale_mus_reader::FieldMapping { \
        #member, \
        ::finale_mus_reader::FieldKind::Text, \
        ::finale_mus_reader::SourceLocation{ \
            (classId), 0, 0, static_cast<std::uint32_t>(byteOffset), \
            ::finale_mus_reader::ValueWidth::Word, \
            ::finale_mus_reader::LongWordOrder::HighFirst, \
            ::finale_mus_reader::BitRange{} }, \
        ::finale_mus_reader::VersionRange{}, \
        nullptr, \
        nullptr, \
        [](void* instance, std::string_view value) { \
            static_cast<Class*>(instance)->member.assign(value); } \
    }

/// @brief A bit range assigned through an explicit conversion expression.
/// @details Use where the stored encoding does not match the destination type, such as an
/// enum whose values differ from the legacy encoding. `value` names the extracted bits.
#define MUS_BITS_AS(Class, tagText, selectorValue, incidenceValue, slotValue, firstBit, \
                    bitCount, member, ...) \
    ::finale_mus_reader::FieldMapping { \
        #member, \
        ::finale_mus_reader::FieldKind::Number, \
        ::finale_mus_reader::SourceLocation{ \
            ::finale_mus_reader::records::packTag(tagText), static_cast<std::uint16_t>(selectorValue), \
            static_cast<std::uint32_t>(incidenceValue), static_cast<std::uint32_t>(slotValue), \
            ::finale_mus_reader::ValueWidth::Word, \
            ::finale_mus_reader::LongWordOrder::HighFirst, \
            (::finale_mus_reader::BitRange{ \
                static_cast<std::uint8_t>(firstBit), static_cast<std::uint8_t>(bitCount)}) }, \
        ::finale_mus_reader::VersionRange{}, \
        [](void* instance, std::int64_t value) { \
            static_cast<Class*>(instance)->member = (__VA_ARGS__); }, \
        [](const void* instance) -> std::int64_t { \
            return ::finale_mus_reader::readAs( \
                static_cast<const Class*>(instance)->member); }, \
        nullptr \
    }

/// @brief Text assembled from every incidence at or after `firstIncidence`.
#define MUS_TEXT(Class, tagText, selectorValue, firstIncidence, member) \
    ::finale_mus_reader::FieldMapping { \
        #member, \
        ::finale_mus_reader::FieldKind::Text, \
        ::finale_mus_reader::SourceLocation{ \
            ::finale_mus_reader::records::packTag(tagText), static_cast<std::uint16_t>(selectorValue), \
            static_cast<std::uint16_t>(firstIncidence), 0, \
            ::finale_mus_reader::ValueWidth::Word, \
            ::finale_mus_reader::LongWordOrder::HighFirst, \
            ::finale_mus_reader::BitRange{} }, \
        ::finale_mus_reader::VersionRange{}, \
        nullptr, \
        nullptr, \
        [](void* instance, std::string_view value) { \
            static_cast<Class*>(instance)->member.assign(value); } \
    }

/// @brief A two-byte field.
#define MUS_WORD(Class, tagText, selector, incidence, slot, member) \
    MUS_FIELD(Class, tagText, selector, incidence, slot, \
        ::finale_mus_reader::ValueWidth::Word, \
        ::finale_mus_reader::LongWordOrder::HighFirst, \
        ::finale_mus_reader::BitRange{}, \
        ::finale_mus_reader::VersionRange{}, member)

/// @brief A two-byte field restricted to a range of Finale major versions.
#define MUS_WORD_V(Class, tagText, selector, incidence, slot, versionRange, member) \
    MUS_FIELD(Class, tagText, selector, incidence, slot, \
        ::finale_mus_reader::ValueWidth::Word, \
        ::finale_mus_reader::LongWordOrder::HighFirst, \
        ::finale_mus_reader::BitRange{}, (versionRange), member)

/// @brief A one-byte field, narrowed from its payload word.
#define MUS_BYTE(Class, tagText, selector, incidence, slot, member) \
    MUS_FIELD(Class, tagText, selector, incidence, slot, \
        ::finale_mus_reader::ValueWidth::Byte, \
        ::finale_mus_reader::LongWordOrder::HighFirst, \
        ::finale_mus_reader::BitRange{}, \
        ::finale_mus_reader::VersionRange{}, member)

/// @brief A four-byte field spanning two consecutive payload words.
#define MUS_LONG(Class, tagText, selector, incidence, slot, order, member) \
    MUS_FIELD(Class, tagText, selector, incidence, slot, \
        ::finale_mus_reader::ValueWidth::Long, (order), \
        ::finale_mus_reader::BitRange{}, \
        ::finale_mus_reader::VersionRange{}, member)

/// @brief A single bit of a payload word.
#define MUS_BIT(Class, tagText, selector, incidence, slot, bitIndex, member) \
    MUS_FIELD(Class, tagText, selector, incidence, slot, \
        ::finale_mus_reader::ValueWidth::Word, \
        ::finale_mus_reader::LongWordOrder::HighFirst, \
        (::finale_mus_reader::BitRange{static_cast<std::uint8_t>(bitIndex), 1}), \
        ::finale_mus_reader::VersionRange{}, member)

/// @brief A contiguous bit range of a payload word.
#define MUS_BITS(Class, tagText, selector, incidence, slot, firstBit, bitCount, member) \
    MUS_FIELD(Class, tagText, selector, incidence, slot, \
        ::finale_mus_reader::ValueWidth::Word, \
        ::finale_mus_reader::LongWordOrder::HighFirst, \
        (::finale_mus_reader::BitRange{ \
            static_cast<std::uint8_t>(firstBit), static_cast<std::uint8_t>(bitCount)}), \
        ::finale_mus_reader::VersionRange{}, member)
