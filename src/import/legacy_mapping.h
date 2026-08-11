// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
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

/// @brief Applies every registered mapping table to a seeded document.
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

/// @brief A bit range of a class-identified record, addressed by byte offset in its payload.
#define MUS_CLASS_BITS(Class, classId, byteOffset, firstBit, bitCount, member) \
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
