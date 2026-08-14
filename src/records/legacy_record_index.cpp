// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "records/legacy_record_index.h"

#include <algorithm>
#include <tuple>
#include <utility>

namespace finale_mus_reader {
namespace records {
namespace {

constexpr std::size_t rowSize = 16;

auto familyKey(const LegacyRow& row)
{
    return std::tie(row.tag, row.cmper1, row.cmper2);
}

std::int16_t readWord(const std::uint8_t* data, ByteOrder byteOrder)
{
    if (byteOrder == ByteOrder::BigEndian) {
        return static_cast<std::int16_t>(
            (static_cast<std::uint16_t>(data[0]) << 8U) | data[1]);
    }
    return static_cast<std::int16_t>(data[0] | (static_cast<std::uint16_t>(data[1]) << 8U));
}

std::uint16_t readCmper(const std::uint8_t* data, ByteOrder byteOrder)
{
    return static_cast<std::uint16_t>(readWord(data, byteOrder));
}

std::uint32_t readLong(const std::uint8_t* data, ByteOrder byteOrder)
{
    if (byteOrder == ByteOrder::BigEndian) {
        return (static_cast<std::uint32_t>(data[0]) << 24U)
            | (static_cast<std::uint32_t>(data[1]) << 16U)
            | (static_cast<std::uint32_t>(data[2]) << 8U)
            | data[3];
    }
    return data[0]
        | (static_cast<std::uint32_t>(data[1]) << 8U)
        | (static_cast<std::uint32_t>(data[2]) << 16U)
        | (static_cast<std::uint32_t>(data[3]) << 24U);
}

// Through Finale 2006 the pools appear in a fixed order and only the framing differs, so a
// block type selects a pool the same way in every epoch. Coda-banner pools are reported
// under the uncompressed numbering by the container.
struct PoolTypes
{
    std::uint16_t others{};
    std::uint16_t details{};
};

std::optional<PoolTypes> poolTypesFor(FormatEpoch epoch)
{
    switch (epoch) {
    case FormatEpoch::CodaBanner:
    case FormatEpoch::UncompressedLegacy:
        return PoolTypes{0x0001, 0x0002};
    case FormatEpoch::DclLegacy:
        return PoolTypes{0x000f, 0x0010};
    case FormatEpoch::ZlibLegacy:
    case FormatEpoch::Unknown:
        break;
    }
    return std::nullopt;
}

// An other is cmper, tag, six words. A detail carries a second cmper, which pushes the tag
// two bytes along and leaves five words.
std::vector<LegacyRow> decodeRows(const container::ParsedContainer& parsed,
    std::uint16_t blockType, bool isDetail, std::vector<std::uint8_t>& payload)
{
    std::vector<LegacyRow> result;
    for (const auto& block : parsed.blocks) {
        if (block.info.type != blockType) {
            continue;
        }
        // Entry pools are not this shape and are frequently not a multiple of the row size,
        // so a partial trailing row is ignored rather than treated as an error.
        for (std::size_t offset = 0; offset + rowSize <= block.data.size(); offset += rowSize) {
            const auto* row = block.data.data() + offset;
            LegacyRow decoded;
            decoded.cmper1 = readCmper(row, parsed.byteOrder);
            const std::size_t tagOffset = isDetail ? 4 : 2;
            if (isDetail) {
                decoded.cmper2 = readCmper(row + 2, parsed.byteOrder);
            }
            // The tag is stored as a 16-bit value, so a little-endian file holds "FN" as
            // the bytes "NF". Reading the bytes literally mismatches every tag in such a
            // file and does so silently, because the rows still look structurally valid.
            // The two characters are taken in the file's order and repacked logically, so
            // everything above this point sees a tag that reads as text.
            const bool bigEndian = parsed.byteOrder == ByteOrder::BigEndian;
            const std::array<char, 2> tagBytes{
                static_cast<char>(row[tagOffset + (bigEndian ? 0 : 1)]),
                static_cast<char>(row[tagOffset + (bigEndian ? 1 : 0)])};
            decoded.tag = packTag(std::string_view(tagBytes.data(), tagBytes.size()));
            decoded.wordCount = isDetail ? detailWordCount : otherWordCount;
            const auto* source = row + tagOffset + 2;
            for (std::uint8_t i = 0; i < decoded.wordCount; ++i) {
                decoded.words[i] = readWord(source + i * 2, parsed.byteOrder);
            }
            decoded.payloadOffset = static_cast<std::uint32_t>(payload.size());
            decoded.payloadSize = static_cast<std::uint32_t>(decoded.wordCount * 2);
            payload.insert(payload.end(), source, source + decoded.payloadSize);
            decoded.blockOffset = block.info.sourceOffset;
            decoded.decodedOffset = offset;
            result.push_back(decoded);
        }
    }
    return result;
}

// The 2007 encoding replaces the fixed row with streams of self-describing records. Other
// records in block 0x001a carry class, cmper1, incidence, and a 32-bit payload length. Detail
// records in block 0x001b retain the pre-zlib second comparator between cmper1 and incidence;
// the endian-specific length geometry is handled below.
// This is strongly supported by six assignments in two Finale 2008 documents: the field
// equals the companion's measure comparator, and the remaining 20-word payload is identical
// to the older mg tuple. Keep this structural interpretation revisable if contrary data
// appears, but do not discard the field by treating it as an incidence.
std::vector<LegacyRow> decodeClassRecords(const container::ParsedContainer& parsed,
    std::vector<std::uint8_t>& payload)
{
    constexpr std::uint16_t otherRecordBlockType = 0x001a;
    constexpr std::uint16_t detailRecordBlockType = 0x001b;
    constexpr std::size_t otherHeaderSize = 10;
    constexpr std::size_t littleEndianDetailHeaderSize = 12;
    constexpr std::size_t paddingSize = 4;

    std::vector<LegacyRow> result;
    for (const auto& block : parsed.blocks) {
        const bool isDetail = block.info.type == detailRecordBlockType;
        if (block.info.type != otherRecordBlockType && !isDetail) {
            continue;
        }
        // The transition-era detail stream spells its length as the fifth 16-bit field in
        // big-endian files. Little-endian files widen that same location to a 32-bit value,
        // making the header two bytes longer. Complete-member consumption establishes both
        // geometries independently of the saving version.
        const auto headerSize = isDetail
            ? (parsed.byteOrder == ByteOrder::BigEndian
                    ? otherHeaderSize : littleEndianDetailHeaderSize)
            : otherHeaderSize;
        std::size_t offset = 0;
        while (offset + headerSize <= block.data.size()) {
            const auto* header = block.data.data() + offset;
            const auto classId = static_cast<std::uint16_t>(readWord(header, parsed.byteOrder));
            // The length is a 32-bit value, so its word order flips with the file's byte
            // order too. Composing it from two 16-bit reads in a fixed order yields an
            // enormous length on a big-endian file and aborts the walk at the first record,
            // which is what most Finale 2007 documents are.
            const auto length = isDetail && parsed.byteOrder == ByteOrder::BigEndian
                ? static_cast<std::uint32_t>(readCmper(header + 8, parsed.byteOrder))
                : readLong(header + (isDetail ? 8U : 6U), parsed.byteOrder);
            if (classId == 0 || length > block.data.size() - offset - headerSize) {
                break;
            }

            LegacyRow decoded;
            decoded.tag = classId;
            decoded.cmper1 = readCmper(header + 2, parsed.byteOrder);
            if (isDetail) {
                decoded.cmper2 = readCmper(header + 4, parsed.byteOrder);
                decoded.inci = readCmper(header + 6, parsed.byteOrder);
            } else {
                decoded.inci = readCmper(header + 4, parsed.byteOrder);
            }
            decoded.explicitIncidence = true;
            decoded.payloadOffset = static_cast<std::uint32_t>(payload.size());
            decoded.payloadSize = length;
            const auto* body = header + headerSize;
            payload.insert(payload.end(), body, body + length);
            decoded.blockOffset = block.info.sourceOffset;
            decoded.decodedOffset = offset;
            result.push_back(decoded);

            offset += headerSize + length + paddingSize;
        }
    }
    return result;
}

} // namespace

LegacyRowPool LegacyRowPool::build(std::vector<LegacyRow> rows, std::vector<std::uint8_t> payload)
{
    // Sorting by family keeps each family contiguous, so a lookup is a binary search rather
    // than a hashed allocation. Fixed rows have no incidence field and derive it from encounter
    // order; variable records carry one explicitly and must retain it.
    //
    // Incidence is defined by encounter order, so decode order is carried into the sort key
    // rather than left to the algorithm's stability. Relying on std::stable_sort would work
    // but would silently break if the comparator were ever reused with std::sort.
    for (std::size_t i = 0; i < rows.size(); ++i)
        if (!rows[i].explicitIncidence) rows[i].inci = static_cast<std::uint32_t>(i);
    std::sort(rows.begin(), rows.end(),
        [](const LegacyRow& left, const LegacyRow& right) {
            return std::tuple_cat(familyKey(left), std::tie(left.inci))
                < std::tuple_cat(familyKey(right), std::tie(right.inci));
        });
    std::uint32_t inci = 0;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (!rows[i].explicitIncidence) {
            if (i != 0 && familyKey(rows[i]) == familyKey(rows[i - 1])) ++inci;
            else inci = 0;
            rows[i].inci = inci;
        }
    }

    LegacyRowPool result;
    result.m_rows = std::move(rows);
    result.m_payload = std::move(payload);
    return result;
}

std::span<const LegacyRow> LegacyRowPool::getArray(
    LegacyTag tag, std::uint16_t cmper1, std::uint16_t cmper2) const
{
    const auto key = std::tie(tag, cmper1, cmper2);
    const auto range = std::equal_range(m_rows.begin(), m_rows.end(), key,
        [](const auto& left, const auto& right) {
            if constexpr (std::is_same_v<std::decay_t<decltype(left)>, LegacyRow>) {
                return familyKey(left) < right;
            } else {
                return left < familyKey(right);
            }
        });
    return std::span<const LegacyRow>(&*range.first, static_cast<std::size_t>(
        std::distance(range.first, range.second)));
}

const LegacyRow* LegacyRowPool::get(
    LegacyTag tag, std::uint16_t cmper1, std::uint16_t cmper2, std::uint32_t inci) const
{
    const auto family = getArray(tag, cmper1, cmper2);
    return inci < family.size() ? &family[inci] : nullptr;
}

std::vector<std::uint16_t> LegacyRowPool::cmpersForTag(LegacyTag tag) const
{
    std::vector<std::uint16_t> result;
    for (const auto& row : m_rows) {
        if (row.tag == tag && (result.empty() || result.back() != row.cmper1)) {
            result.push_back(row.cmper1);
        }
    }
    return result;
}

std::vector<std::uint16_t> LegacyRowPool::secondCmpersForTag(
    LegacyTag tag, std::uint16_t cmper1) const
{
    std::vector<std::uint16_t> result;
    for (const auto& row : m_rows) {
        if (row.tag == tag && row.cmper1 == cmper1
            && (result.empty() || result.back() != row.cmper2)) {
            result.push_back(row.cmper2);
        }
    }
    return result;
}

LegacyRecordIndex LegacyRecordIndex::build(const container::ParsedContainer& parsed)
{
    LegacyRecordIndex result;
    if (const auto types = poolTypesFor(parsed.formatEpoch)) {
        std::vector<std::uint8_t> othersPayload;
        auto othersRows = decodeRows(parsed, types->others, false, othersPayload);
        result.m_others = LegacyRowPool::build(std::move(othersRows), std::move(othersPayload));

        std::vector<std::uint8_t> detailsPayload;
        auto detailRows = decodeRows(parsed, types->details, true, detailsPayload);
        result.m_details = LegacyRowPool::build(std::move(detailRows), std::move(detailsPayload));
    } else if (parsed.formatEpoch == FormatEpoch::ZlibLegacy) {
        std::vector<std::uint8_t> recordPayload;
        auto records = decodeClassRecords(parsed, recordPayload);
        result.m_classRecords = LegacyRowPool::build(std::move(records), std::move(recordPayload));
    }
    return result;
}

std::optional<RecordWord> LegacyRecordIndex::word(
    LegacyTag tag, std::uint16_t cmper, std::size_t wordIndex) const
{
    const auto family = m_others.getArray(tag, cmper);
    if (family.empty()) {
        return std::nullopt;
    }
    const auto incidence = wordIndex / family.front().wordCount;
    const auto slot = wordIndex % family.front().wordCount;
    if (incidence >= family.size()) {
        return std::nullopt;
    }
    const auto& row = family[incidence];
    return RecordWord{row.words[slot], row.blockOffset, row.decodedOffset};
}

} // namespace records
} // namespace finale_mus_reader
