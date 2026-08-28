// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

void testClassRecordContinuationSegment()
{
    const auto verify = [](ByteOrder byteOrder) {
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
        const auto push32 = [&](std::uint32_t value) {
            if (byteOrder == ByteOrder::BigEndian) {
                push16(static_cast<std::uint16_t>(value >> 16U));
                push16(static_cast<std::uint16_t>(value));
            } else {
                push16(static_cast<std::uint16_t>(value));
                push16(static_cast<std::uint16_t>(value >> 16U));
            }
        };
        const auto appendHeader = [&](std::uint16_t classId, std::uint16_t cmper,
                                      std::uint16_t partId, std::uint32_t length) {
            push16(classId);
            push16(cmper);
            push16(partId);
            push32(length);
        };

        appendHeader(0x00b1, 1, 1, 12);
        block.data.insert(block.data.end(), {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
        // The continuation occupies the declared byte count including its repeated-size
        // prefix. It is framing, not another searchable incidence.
        push32(12);
        block.data.insert(block.data.end(), {21, 22, 23, 24, 25, 26, 27, 28});
        push16(0);
        push16(0x1234);

        appendHeader(0x00d6, 3, 0, 4);
        block.data.insert(block.data.end(), {31, 32, 33, 34});
        block.data.insert(block.data.end(), 4, 0);
        block.info.decodedSize = block.data.size();
        parsed.blocks.push_back(std::move(block));

        const auto index = LegacyRecordIndex::build(parsed);
        expectMapping(index.getClassOthers().cmpersForTag(0x00b1).empty()
                && index.getClassOthers().cmpersForTag(0x00b1, 1)
                    == std::vector<std::uint16_t>{1},
            "A part-owned class record leaked into the score comparator list");
        const auto first = index.getClassOthers().get(0x00b1, 1, 0, 0, 1);
        const auto firstPayload = first
            ? index.getClassOthers().payloadOf(*first) : std::span<const std::uint8_t>{};
        expectMapping(firstPayload.size() == 12 && firstPayload.front() == 1
                && firstPayload.back() == 12,
            "A class-record continuation replaced the primary payload");
        const auto following = index.getClassOthers().get(0x00d6, 3, 0, 0);
        const auto followingPayload = following
            ? index.getClassOthers().payloadOf(*following) : std::span<const std::uint8_t>{};
        expectMapping(followingPayload.size() == 4 && followingPayload.front() == 31
                && followingPayload.back() == 34,
            "A class-record continuation stopped the remaining pool");
    };

    verify(ByteOrder::BigEndian);
    verify(ByteOrder::LittleEndian);
}
void testDetailRowShape()
{
    finale_mus_reader::container::ParsedContainer parsed(FormatEpoch::UncompressedLegacy);
    parsed.byteOrder = ByteOrder::BigEndian;

    finale_mus_reader::container::DecodedBlock block;
    block.info.type = 0x0002;
    const std::vector<std::array<std::int16_t, 8>> detailRows{
        // cmper1, cmper2, packed tag halves, then five payload words
        {7, 9, 'C', 'L', 11, 22, 33, 44},
        {7, 9, 'C', 'L', 55, 66, 77, 88},
        {7, 8, 'C', 'L', 99, 0, 0, 0},
    };
    for (const auto& row : detailRows) {
        const auto push16 = [&](std::uint16_t v) {
            block.data.push_back(static_cast<std::uint8_t>(v >> 8U));
            block.data.push_back(static_cast<std::uint8_t>(v));
        };
        push16(static_cast<std::uint16_t>(row[0]));
        push16(static_cast<std::uint16_t>(row[1]));
        block.data.push_back(static_cast<std::uint8_t>(row[2]));
        block.data.push_back(static_cast<std::uint8_t>(row[3]));
        for (int i = 4; i < 8; ++i) push16(static_cast<std::uint16_t>(row[i]));
        push16(0);
    }
    parsed.blocks.push_back(std::move(block));

    const auto index = LegacyRecordIndex::build(parsed);
    const auto tag = finale_mus_reader::records::packTag("CL");
    const auto family = index.getDetails().getArray(tag, 7, 9);
    expectMapping(family.size() == 2, "Detail family did not group by both comparators");
    expectMapping(family[0].inci == 0 && family[1].inci == 1,
        "Detail incidences were not assigned in encounter order");
    expectMapping(family[0].wordCount == finale_mus_reader::records::detailWordCount,
        "Detail rows should carry five payload words");
    expectMapping(family[0].words[0] == 11 && family[1].words[0] == 55,
        "Detail payload was read from the wrong offset");

    const auto other = index.getDetails().getArray(tag, 7, 8);
    expectMapping(other.size() == 1 && other[0].words[0] == 99,
        "A different second comparator was not treated as a separate family");
    expectMapping(index.getDetails().get(tag, 7, 9, 1) != nullptr
        && index.getDetails().get(tag, 7, 9, 2) == nullptr,
        "Detail incidence lookup did not bound correctly");
    expectMapping(index.getDetails().getArray(tag, 1, 1).empty(),
        "An absent detail family returned rows");
    expectMapping(index.getOthers().empty(), "A details block produced others rows");
}

// The others pool keeps working through the same normalized index, and the word stream is
// still addressed across incidences.
void testOtherRowsRemainSearchable()
{
    const auto parsed = makeContainer({
        {GLOBALS_CMPER, "94", {1, 2, 3, 4, 5, 6}},
        {GLOBALS_CMPER, "94", {7, 8, 9, 10, 11, 12}},
        {3, "LA", {-14, 0, 0, 0, 0, 0}},
    });
    const auto index = LegacyRecordIndex::build(parsed);
    const auto spacing = finale_mus_reader::records::packTag("94");
    expectMapping(index.getOthers().getArray(spacing, GLOBALS_CMPER).size() == 2,
        "Others family did not group by comparator");
    expectMapping(index.getOthers().cmpersForTag(finale_mus_reader::records::packTag("LA"))
        == std::vector<std::uint16_t>{3}, "cmpersForTag did not report the layer comparator");
    const auto straddle = index.word(spacing, GLOBALS_CMPER, 6);
    expectMapping(straddle && straddle->value == 7,
        "Word addressing did not continue into the next incidence");
    expectMapping(!index.word(spacing, GLOBALS_CMPER, 12),
        "Word addressing ran past the last incidence");
}

TEST_CASE("Class-record continuation segment", "[class]") { testClassRecordContinuationSegment(); }
TEST_CASE("Detail row shape", "[class]") { testDetailRowShape(); }
TEST_CASE("Other rows remain searchable", "[class]") { testOtherRowsRemainSearchable(); }

} // namespace
} // namespace finale_mus_reader_tests
