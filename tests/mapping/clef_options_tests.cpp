// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "mapping_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace mapping;

void testClefTupleDecoding()
{
    using ClefOptions = musx::dom::options::ClefOptions;
    const auto captured = [](const finale_mus_reader::container::ParsedContainer& parsed,
                              const SourceProfile& profile) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        ImportReport report(FormatEpoch::UncompressedLegacy);
        finale_mus_reader::PendingReferences pending;
        finale_mus_reader::options::captureClefOptions(LegacyRecordIndex::build(parsed),
            profile, document, makeClefReferenceDocument(), report, pending);
        return document->getOptions()->get<ClefOptions>();
    };

    // One nine-word tuple whose character is the sign-extended spelling of 139, and whose
    // flags claim an own font. Padded out to a whole sixteen-definition table.
    std::vector<SyntheticRow> rows;
    std::vector<std::int16_t> stream{-10, static_cast<std::int16_t>(0xff8b), -6, 7, 0,
        11, 24, 1, 0x0002};
    stream.resize(16 * 9, 0);
    for (std::size_t first = 0; first < stream.size(); first += 6) {
        SyntheticRow row;
        row.cmper = GLOBALS_CMPER;
        row.tag = "95";
        for (std::size_t slot = 0; slot < 6; ++slot) {
            row.words[slot] = stream[first + slot];
        }
        rows.push_back(row);
    }
    const auto narrow = captured(makeContainer(rows), profileFor(7));
    expectMapping(narrow && narrow->clefDefs.size() == 18,
        "The synthetic sixteen-clef table was not completed from the reference");
    const auto first = narrow->getClefDef(0);
    expectMapping(first->clefChar == 139,
        "A sign-extended clef character was not narrowed to its stored byte");
    expectMapping(first->useOwnFont && first->font && first->font->fontId == 11
            && first->font->fontSize == 24 && first->font->bold,
        "The own-font flag bit did not bring across the tuple's font triple");
    expectMapping(!first->isShape && !first->scaleToStaffHeight,
        "Unset clef flag bits were treated as set");
    expectMapping(first->baselineAdjust == 7,
        "The 2001-and-later baseline word was not assigned as Efix");
    expectMapping(narrow->clefFrontSepar == 24,
        "The reference scalars were not copied onto the rebuilt clef options");

    // A DCL file with no clef table must not fall back on the pre-2001 selectors. Those
    // selectors still exist in that era and hold unrelated option words, so reading them
    // would fabricate eight clef definitions and report them as recovered from the file.
    {
        std::vector<SyntheticRow> dclRows;
        std::array<std::array<char, 3>, 8> dclTags{};
        // The values a real Finale 2002 file holds at selectors 28 through 35.
        const std::array<std::array<std::int16_t, 6>, 8> notClefs{{{0, 0, 0, 0, 0, 24},
            {0, 0, 0, 0, 0, 1}, {0, 0, 0, 0, 0, 4}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 128}, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}}};
        for (int selector = 28; selector <= 35; ++selector) {
            auto& tag = dclTags[static_cast<std::size_t>(selector - 28)];
            tag = {static_cast<char>('0' + selector / 10),
                static_cast<char>('0' + selector % 10), '\0'};
            SyntheticRow row;
            row.cmper = GLOBALS_CMPER;
            row.tag = tag.data();
            row.words = notClefs[static_cast<std::size_t>(selector - 28)];
            dclRows.push_back(row);
        }
        auto profile = profileFor(7);
        profile.epoch = FormatEpoch::DclLegacy;
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        ImportReport report(FormatEpoch::UncompressedLegacy);
        finale_mus_reader::PendingReferences pending;
        finale_mus_reader::options::captureClefOptions(
            LegacyRecordIndex::build(makeContainer(dclRows)), profile, document,
            makeClefReferenceDocument(), report, pending);
        std::size_t recovered = 0;
        anyMappingReportedField(report, [&](const auto& member, const auto& info) {
            if (info.origin == ValueOrigin::LegacyMus
                    && member.find("clefDefs") != std::string::npos) ++recovered;
            return false;
        });
        expectMapping(recovered == 0,
            "A DCL file with no clef table read the pre-2001 selectors as clefs");
        expectMapping(document->getOptions()->get<ClefOptions>()->clefDefs.size() == 18,
            "A DCL file with no clef table was not completed from the reference");
    }

    // An out-of-range default clef index is warned about and left alone. Clamping it would
    // turn a damaged file into a plausible document and hide the fact worth knowing.
    {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto options = std::make_shared<ClefOptions>(document);
        options->defaultClef = 99;
        for (int i = 0; i < 18; ++i) {
            options->clefDefs.push_back(std::make_shared<ClefOptions::ClefDef>(options));
        }
        document->getOptions()->add(ClefOptions::XmlNodeName, options);
        ImportReport report(FormatEpoch::UncompressedLegacy);
        musx::factory::ConstructionContext construction;
        finale_mus_reader::options::validateClefOptions(document, report, construction);
        expectMapping(options->defaultClef == 99,
            "An out-of-range default clef index was silently corrected");
        expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                          [](const finale_mus_reader::Diagnostic& entry) {
                              return entry.message.find("default clef index 99") != std::string::npos;
                          }),
            "An out-of-range default clef index was accepted without a warning");
        ImportReport clean(FormatEpoch::UncompressedLegacy);
        options->defaultClef = 17;
        finale_mus_reader::options::validateClefOptions(document, clean, construction);
        expectMapping(clean.diagnostics.empty(), "A valid default clef index warned");
    }

    // Pre-2001 clefs are eight separate globals whose baseline word is in harmonic levels,
    // so it is scaled into Efix on the way in while the report keeps the stored number.
    std::vector<SyntheticRow> earlyRows;
    // Fixed storage: SyntheticRow keeps a pointer, so the tag text must outlive the vector
    // and must not move when rows are appended.
    std::array<std::array<char, 3>, 8> earlyTags{};
    for (int selector = 28; selector <= 35; ++selector) {
        auto& tag = earlyTags[static_cast<std::size_t>(selector - 28)];
        tag = {static_cast<char>('0' + selector / 10),
            static_cast<char>('0' + selector % 10), '\0'};
        SyntheticRow row;
        row.cmper = GLOBALS_CMPER;
        row.tag = tag.data();
        // Slot 4 is the baseline adjustment in this era; slot 1 holds something else.
        row.words = {static_cast<std::int16_t>(-selector), 7,
            static_cast<std::int16_t>(0xff8b), -6, 2, 0};
        earlyRows.push_back(row);
    }
    const auto early = captured(makeContainer(earlyRows), profileFor(5));
    expectMapping(early && early->clefDefs.size() == 18,
        "The eight pre-2001 clef globals were not completed from the reference");
    expectMapping(early->getClefDef(0)->middleCPos == -28
            && early->getClefDef(0)->clefChar == 139
            && early->getClefDef(0)->staffPosition == -6,
        "The pre-2001 per-clef record slots were misread");
    expectMapping(early->getClefDef(0)->baselineAdjust == 2 * 768,
        "The pre-2001 harmonic-level baseline was not converted to Efix");
    expectMapping(early->getClefDef(7)->middleCPos == -35
            && early->getClefDef(8)->middleCPos == -1,
        "The boundary between eight stored and ten synthesized clefs moved");

    // 360 bytes divides by both tuple widths. Only the version separates them, and only
    // the wide reading yields the eighteen definitions Finale actually stores.
    std::vector<std::int16_t> wide(18 * 10, 0);
    wide[0] = -10;
    wide[1] = 38;   // low half of the long character
    wide[2] = 0;    // high half
    wide[3] = -6;
    wide[9] = 0x0005;
    const auto unicodeEra = captured(
        makeClassContainer(0x006d, wide, ByteOrder::LittleEndian), [] {
            auto profile = profileFor(17);
            profile.epoch = FormatEpoch::ZlibLegacy;
            profile.byteOrder = ByteOrder::LittleEndian;
            return profile;
        }());
    expectMapping(unicodeEra && unicodeEra->clefDefs.size() == 18,
        "A 360-byte payload was not read as eighteen wide clef definitions");
    expectMapping(unicodeEra->getClefDef(0)->clefChar == 38
            && unicodeEra->getClefDef(0)->staffPosition == -6,
        "The wide tuple's long character or shifted slots were misread");
    expectMapping(unicodeEra->getClefDef(0)->isShape
            && unicodeEra->getClefDef(0)->scaleToStaffHeight,
        "The wide tuple's flags word was not read at its shifted slot");

    // A whole class-record field is signed, like the fixed-row word that carries the same
    // logical option. Read unsigned, this Evpu of -12 arrives as 65524.
    {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto profile = profileFor(13);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::BigEndian;
        // Class 0x1b is globals selector 13; word 2 is the percent and word 3 the offset.
        auto options = std::make_shared<ClefOptions>(document);
        document->getOptions()->add(ClefOptions::XmlNodeName, options);
        ImportReport report(FormatEpoch::UncompressedLegacy);
        finale_mus_reader::applyMappingTables(
            {&finale_mus_reader::options::classClefOptionsTable()},
            LegacyRecordIndex::build(makeClassContainer(
                0x001b, {4, 1024, 75, -12, 0, 1}, ByteOrder::BigEndian)),
            profile, document, report);
        expectMapping(options->clefChangeOffset == -12,
            "A negative class-record word was read as an unsigned value");
        expectMapping(options->clefChangePercent == 75,
            "A positive class-record word was not read");
    }

    // A big-endian file using the Finale 2012 layout announces itself, because no surveyed
    // file is one and the long character's word order has never been checked against a
    // specimen. The values still decode as the symmetric counterpart of the verified case.
    {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto profile = profileFor(17);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::BigEndian;
        ImportReport report(FormatEpoch::UncompressedLegacy);
        finale_mus_reader::PendingReferences pending;
        finale_mus_reader::options::captureClefOptions(
            LegacyRecordIndex::build(makeClassContainer(0x006d, wide, ByteOrder::BigEndian)),
            profile, document, makeClefReferenceDocument(), report, pending);
        expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                          [](const finale_mus_reader::Diagnostic& entry) {
                              return entry.message.find("unverified") != std::string::npos;
                          }),
            "A big-endian Finale 2012 clef layout was decoded without saying it is unverified");
        expectMapping(document->getOptions()->get<ClefOptions>()->getClefDef(0)->clefChar == 38,
            "The big-endian wide tuple did not decode symmetrically");
    }

    // The same payload from a pre-Unicode file is the narrow tuple, and twenty definitions
    // is more than Finale stores, so the reader must say so rather than pass it off.
    ImportReport report(FormatEpoch::UncompressedLegacy);
    {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto profile = profileFor(13);
        profile.epoch = FormatEpoch::ZlibLegacy;
        profile.byteOrder = ByteOrder::LittleEndian;
        finale_mus_reader::PendingReferences pending;
        finale_mus_reader::options::captureClefOptions(
            LegacyRecordIndex::build(makeClassContainer(0x006d, wide, ByteOrder::LittleEndian)),
            profile, document, makeClefReferenceDocument(), report, pending);
        expectMapping(document->getOptions()->get<ClefOptions>()->clefDefs.size() == 20,
            "The pre-Unicode reading of an ambiguous payload did not use the narrow tuple");
    }
    expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                      [](const finale_mus_reader::Diagnostic& entry) {
                          return entry.message.find("more than the 18") != std::string::npos;
                      }),
        "An over-long clef collection was accepted without a warning");
}


TEST_CASE("Clef tuple decoding", "[mapping]") { testClefTupleDecoding(); }

} // namespace
} // namespace finale_mus_reader_tests
