// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <array>
#include <span>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "container/mus_container.h"
#include "import/support/embedded_graphics.h"
#include "import/details.h"
#include "import/support/legacy_mapping.h"
#include "import/others.h"
#include "import/options.h"
#include "import/options/test_access.h"
#include "records/legacy_record_index.h"

#include "musx/musx.h"

#ifndef MUSX_USE_PUGIXML
#define MUSX_USE_PUGIXML
#define FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML
#endif // !defined(MUSX_USE_PUGIXML)

#include "musx/xml/PugiXmlImpl.h"

#ifdef FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML
#undef MUSX_USE_PUGIXML
#undef FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML
#endif // defined(FINALE_MUS_READER_MAPPING_UNDEFINE_MUSX_USE_PUGIXML)

namespace finale_mus_reader_tests {
namespace mapping {

using finale_mus_reader::ByteOrder;
using finale_mus_reader::FormatEpoch;
using finale_mus_reader::ImportReport;
using finale_mus_reader::SourceVersion;
using finale_mus_reader::ValueOrigin;
using finale_mus_reader::EpochMask;
using finale_mus_reader::FieldMapping;
using finale_mus_reader::GLOBALS_CMPER;
using finale_mus_reader::LongWordOrder;
using finale_mus_reader::MappingTable;
using finale_mus_reader::SourceProfile;
using finale_mus_reader::TargetKind;
using finale_mus_reader::SourceGate;
using finale_mus_reader::records::LegacyRecordIndex;
using Spacing = musx::dom::options::MusicSpacingOptions;

constexpr auto sourceMatchTestProfile = [] {
    SourceProfile result(FormatEpoch::ZlibLegacy);
    result.version = SourceVersion{.major = 15, .minor = 1};
    return result;
}();
static_assert(finale_mus_reader::sourceMatches(sourceMatchTestProfile, EpochMask::Zlib));
static_assert(!finale_mus_reader::sourceMatches(sourceMatchTestProfile, EpochMask::Dcl));
static_assert(finale_mus_reader::sourceAtOrAfter(
    sourceMatchTestProfile, FormatEpoch::DclLegacy));
constexpr SourceProfile earlierSourceMatchTestProfile(FormatEpoch::DclLegacy);
static_assert(!finale_mus_reader::sourceAtOrAfter(
    earlierSourceMatchTestProfile, FormatEpoch::ZlibLegacy));
static_assert(finale_mus_reader::sourceAtOrAfter(
    sourceMatchTestProfile, FormatEpoch::DclLegacy, finale_mus_reader::versions::finale2007));
static_assert(finale_mus_reader::sourceAtOrAfter(
    sourceMatchTestProfile, FormatEpoch::ZlibLegacy, {15, 1}));
static_assert(!finale_mus_reader::sourceAtOrAfter(
    sourceMatchTestProfile, FormatEpoch::ZlibLegacy, finale_mus_reader::versions::finale2011));
static_assert(finale_mus_reader::sourcePredatesVersion(FormatEpoch::CodaBanner, nullptr,
    FormatEpoch::UncompressedLegacy, finale_mus_reader::versions::finale97));
static_assert(!finale_mus_reader::sourcePredatesVersion(FormatEpoch::UncompressedLegacy, nullptr,
    FormatEpoch::UncompressedLegacy, finale_mus_reader::versions::finale97));
static_assert(finale_mus_reader::sourceAtOrAfter(FormatEpoch::DclLegacy, nullptr,
    FormatEpoch::UncompressedLegacy, finale_mus_reader::versions::finale2000));

inline bool sourceAtFinale2007(const SourceProfile& profile)
{
    return finale_mus_reader::sourceAtOrAfter(
        profile, FormatEpoch::UncompressedLegacy, finale_mus_reader::versions::finale2007);
}

inline bool sourceAtFinale35(const SourceProfile& profile)
{
    return finale_mus_reader::sourceAtOrAfter(
        profile, FormatEpoch::UncompressedLegacy, finale_mus_reader::versions::finale3_5);
}

// These tests drive one importer at a time against a synthesized record set, so none of them
// has a source file for the importer to read the header out of.
constexpr std::span<const std::uint8_t> noSource{};

inline void expectMapping(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

/// @brief One synthetic 16-byte legacy row.
struct SyntheticRow
{
    std::uint16_t cmper{};
    const char* tag{};
    std::array<std::int16_t, 6> words{};
};

/// @brief Builds a parsed container holding the given rows as an uncompressed other pool.
inline finale_mus_reader::container::ParsedContainer makeContainer(
    const std::vector<SyntheticRow>& rows,
    FormatEpoch epoch = FormatEpoch::UncompressedLegacy)
{
    finale_mus_reader::container::ParsedContainer parsed(epoch);
    parsed.byteOrder = ByteOrder::BigEndian;

    finale_mus_reader::container::DecodedBlock block;
    block.info.type = epoch == FormatEpoch::DclLegacy ? 0x000f : 0x0001;
    for (const auto& row : rows) {
        block.data.push_back(static_cast<std::uint8_t>(row.cmper >> 8U));
        block.data.push_back(static_cast<std::uint8_t>(row.cmper));
        block.data.push_back(static_cast<std::uint8_t>(row.tag[0]));
        block.data.push_back(static_cast<std::uint8_t>(row.tag[1]));
        for (const auto word : row.words) {
            const auto raw = static_cast<std::uint16_t>(word);
            block.data.push_back(static_cast<std::uint8_t>(raw >> 8U));
            block.data.push_back(static_cast<std::uint8_t>(raw));
        }
    }
    block.info.decodedSize = block.data.size();
    parsed.blocks.push_back(std::move(block));
    return parsed;
}

/// @brief A bare document carrying one music spacing options instance.
inline musx::dom::DocumentPtr makeDocument(Spacing** instanceOut)
{
    auto session = musx::factory::DocumentFactory::begin();
    auto document = session.getDocument();
    auto spacing = std::make_shared<Spacing>(
        document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All);
    spacing->minWidth = 111;
    spacing->maxWidth = 222;
    spacing->referenceDuration = 333;
    spacing->avoidColNotes = false;
    spacing->avoidColStems = true;
    spacing->avoidColUnisons = Spacing::ColUnisonsChoice::DiffNoteheads;
    spacing->ignoreHidden = false;
    *instanceOut = spacing.get();
    document->getOptions()->add(Spacing::XmlNodeName, spacing);
    return std::move(session).finish();
}

struct SyntheticClassRow
{
    std::uint16_t classId{};
    std::vector<std::int16_t> words;
    std::uint16_t cmper{GLOBALS_CMPER};
};

/// @brief Builds a parsed container holding class-identified records, the 2007+ framing.
inline finale_mus_reader::container::ParsedContainer makeClassContainer(
    const std::vector<SyntheticClassRow>& rows, ByteOrder byteOrder)
{
    finale_mus_reader::container::ParsedContainer parsed(FormatEpoch::ZlibLegacy);
    parsed.byteOrder = byteOrder;

    finale_mus_reader::container::DecodedBlock block;
    block.info.type = 0x001a;
    const auto push16 = [&](std::uint16_t value) {
        if (byteOrder == ByteOrder::BigEndian) {
            block.data.push_back(static_cast<std::uint8_t>(value >> 8U));
            block.data.push_back(static_cast<std::uint8_t>(value));
        } else {
            block.data.push_back(static_cast<std::uint8_t>(value));
            block.data.push_back(static_cast<std::uint8_t>(value >> 8U));
        }
    };
    for (const auto& row : rows) {
        push16(row.classId);
        push16(row.cmper);
        push16(0);
        const auto length = static_cast<std::uint32_t>(row.words.size() * 2);
        if (byteOrder == ByteOrder::BigEndian) {
            for (int shift = 24; shift >= 0; shift -= 8) {
                block.data.push_back(static_cast<std::uint8_t>(length >> shift));
            }
        } else {
            for (int shift = 0; shift <= 24; shift += 8) {
                block.data.push_back(static_cast<std::uint8_t>(length >> shift));
            }
        }
        for (const auto word : row.words) {
            push16(static_cast<std::uint16_t>(word));
        }
        block.data.insert(block.data.end(), 4, 0);
    }
    block.info.decodedSize = block.data.size();
    parsed.blocks.push_back(std::move(block));
    return parsed;
}

/// @brief Builds a parsed container holding one class-identified record.
inline finale_mus_reader::container::ParsedContainer makeClassContainer(
    std::uint16_t classId, const std::vector<std::int16_t>& words, ByteOrder byteOrder,
    std::uint16_t cmper = GLOBALS_CMPER)
{
    return makeClassContainer({SyntheticClassRow{classId, words, cmper}}, byteOrder);
}


inline finale_mus_reader::container::ParsedContainer makeDetailContainer(
    FormatEpoch epoch, std::uint16_t staffId, std::uint16_t meas,
    const std::vector<std::int16_t>& words, const char* tag = "mg")
{
    finale_mus_reader::container::ParsedContainer parsed(epoch);
    parsed.byteOrder = ByteOrder::BigEndian;
    finale_mus_reader::container::DecodedBlock block;
    block.info.type = epoch == FormatEpoch::DclLegacy ? 0x0010 : 0x0002;
    for (std::size_t at = 0; at < words.size(); at += finale_mus_reader::records::detailWordCount) {
        const auto push16 = [&](std::uint16_t value) {
            block.data.push_back(static_cast<std::uint8_t>(value >> 8U));
            block.data.push_back(static_cast<std::uint8_t>(value));
        };
        push16(staffId);
        push16(meas);
        push16(finale_mus_reader::records::packTag(tag));
        for (std::size_t slot = 0; slot < finale_mus_reader::records::detailWordCount; ++slot)
            push16(static_cast<std::uint16_t>(words[at + slot]));
    }
    block.info.decodedSize = block.data.size();
    parsed.blocks.push_back(std::move(block));
    return parsed;
}

inline finale_mus_reader::container::ParsedContainer makeDetailClassContainer(
    std::uint16_t staffId, std::uint16_t meas, std::uint16_t partId,
    const std::vector<std::int16_t>& words, ByteOrder byteOrder,
    std::uint16_t classId = 0x041d)
{
    auto parsed = makeClassContainer(classId, {}, byteOrder, staffId);
    auto& block = parsed.blocks.front();
    block.info.type = 0x001b;
    block.data.clear();
    const auto push16 = [&](std::uint16_t value) {
        if (byteOrder == ByteOrder::BigEndian) {
            block.data.push_back(static_cast<std::uint8_t>(value >> 8U));
            block.data.push_back(static_cast<std::uint8_t>(value));
        } else {
            block.data.push_back(static_cast<std::uint8_t>(value));
            block.data.push_back(static_cast<std::uint8_t>(value >> 8U));
        }
    };
    push16(classId);
    push16(staffId);
    push16(meas);
    push16(partId);
    const auto length = static_cast<std::uint32_t>(words.size() * 2);
    if (byteOrder == ByteOrder::BigEndian) {
        push16(static_cast<std::uint16_t>(length));
    } else {
        for (int shift = 0; shift <= 24; shift += 8)
            block.data.push_back(static_cast<std::uint8_t>(length >> shift));
    }
    for (const auto word : words) push16(static_cast<std::uint16_t>(word));
    block.data.insert(block.data.end(), 4, 0);
    block.info.decodedSize = block.data.size();
    return parsed;
}

/// @brief A reference document whose ClefOptions has the modern collection and no fonts.
inline musx::dom::DocumentPtr makeClefReferenceDocument()
{
    using ClefOptions = musx::dom::options::ClefOptions;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto options = std::make_shared<ClefOptions>(document);
    options->clefFrontSepar = 24;
    options->cautionaryClefChanges = true;
    for (int index = 0; index < 18; ++index) {
        auto def = std::make_shared<ClefOptions::ClefDef>(options);
        def->middleCPos = -1;
        def->clefChar = static_cast<char32_t>(900 + index);
        options->clefDefs.push_back(std::move(def));
    }
    document->getOptions()->add(ClefOptions::XmlNodeName, options);
    return std::move(session).finish();
}

inline SourceProfile profileFor(std::uint8_t major, std::uint8_t minor = 0)
{
    SourceProfile profile(FormatEpoch::UncompressedLegacy);
    profile.byteOrder = ByteOrder::BigEndian;
    SourceVersion version;
    version.major = major;
    version.minor = minor;
    profile.version = version;
    return profile;
}


// The target is taken by value rather than by const reference so that a literal call site
// does not create a temporary bound to a reference parameter. GCC cannot prove the returned
// reference does not alias such a temporary and rejects the binding under
// -Wdangling-reference.
struct ExpectedReportField
{
    std::string member;
    std::optional<musx::dom::Cmper> cmper;
};

inline ExpectedReportField expectedReportField(std::string_view target)
{
    ExpectedReportField result{std::string(target), std::nullopt};
    if (target.starts_with("options.fontOptions[")) {
        result.member = "fonts["
            + std::string(target.substr(std::string_view("options.fontOptions[").size()));
    } else if (target.starts_with("options.fontOptionsPhysical[")) {
        result.member = "physical[" + std::string(
            target.substr(std::string_view("options.fontOptionsPhysical[").size()));
    } else if (target.starts_with("options.")) {
        const auto dot = target.find('.', std::string_view("options.").size());
        if (dot != std::string_view::npos) result.member = target.substr(dot + 1);
    } else if (const auto open = target.find('['); open != std::string_view::npos) {
        const auto close = target.find(']', open);
        const auto dot = close == std::string_view::npos ? close : target.find('.', close);
        if (close != std::string_view::npos && dot != std::string_view::npos) {
            result.cmper = static_cast<musx::dom::Cmper>(std::stoul(
                std::string(target.substr(open + 1, close - open - 1))));
            result.member = target.substr(dot + 1);
        }
    }
    return result;
}

inline const finale_mus_reader::FieldInfo& field(const ImportReport& report, std::string_view target)
{
    const auto expected = expectedReportField(target);
    for (const auto& [instance, fields] : report.fields) {
        if (expected.cmper && instance.cmper1 != expected.cmper) continue;
        for (const auto& [member, info] : fields) {
            if (member == expected.member) return info;
        }
    }
    std::string message = std::string("Missing mapping report for ").append(target)
        .append("; available members:");
    for (const auto& [instance, fields] : report.fields) {
        static_cast<void>(instance);
        for (const auto& [member, info] : fields) {
            static_cast<void>(info);
            message.append(" ").append(member);
        }
    }
    throw std::runtime_error(std::move(message));
}

/// @brief Whether the report names a target at all, for a field a record is not expected to have.
inline bool fieldPresent(const ImportReport& report, std::string_view target)
{
    const auto expected = expectedReportField(target);
    for (const auto& [instance, fields] : report.fields) {
        if (expected.cmper && instance.cmper1 != expected.cmper) continue;
        for (const auto& [member, info] : fields) {
            (void)info;
            if (member == expected.member) return true;
        }
    }
    return false;
}

template <typename Predicate>
inline bool anyMappingReportedField(const ImportReport& report, Predicate predicate)
{
    for (const auto& [instance, fields] : report.fields) {
        (void)instance;
        for (const auto& [member, info] : fields) {
            if (predicate(member, info)) return true;
        }
    }
    return false;
}

inline std::size_t reportedFieldCount(const ImportReport& report)
{
    std::size_t result = 0;
    for (const auto& [instance, fields] : report.fields) {
        (void)instance;
        result += fields.size();
    }
    return result;
}

inline MappingTable makeTable(const char* prefix, const FieldMapping* fields, std::size_t count,
    SourceGate sourceApplies = nullptr, EpochMask epochs = EpochMask::FixedRow)
{
    return MappingTable{
        .reportPrefix = prefix,
        .epochs = epochs,
        .sourceApplies = sourceApplies,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &finale_mus_reader::enumerateOptionsTarget<Spacing>,
        .fields = fields,
        .fieldCount = count};
}


} // namespace mapping
} // namespace finale_mus_reader_tests
