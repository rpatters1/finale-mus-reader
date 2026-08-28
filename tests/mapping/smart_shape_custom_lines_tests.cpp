// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "mapping_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace mapping;

// The pre-Unicode word stream of one custom line style per line style, 36 words each.
// Spelled out rather than derived, so a change to the layout has to be restated here to pass.
const std::vector<std::int16_t> ssLineCharWords{
    2, 199, 6, 17, 3, -1, -83, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const std::vector<std::int16_t> ssLineDashedWords{
    1, 118, 192, 448, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const std::vector<std::int16_t> ssLineCappedWords{
    0, 118, 0, 0, 0, 0, 0, 3, 2, 1984, 0, 2,
    0, 5, 1, 2, 3, 4, 5, 11, 13, 17, 19, 23,
    -29, 31, 37, 41, 43, 47, 61, 59, 61, 53, 0, 0};

// The same three in the Finale 2012 layout. The character occupies words 1 and 2, so only the
// Char record's own later fields move; the Dashed record is untouched and everything from the
// old word 7 on moves for all three.
const std::vector<std::int16_t> ssLineWideCharWords{
    2, 171, 0, 6, 17, 3, -1, -83, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const std::vector<std::int16_t> ssLineWideDashedWords = ssLineDashedWords;
const std::vector<std::int16_t> ssLineWideCappedWords{
    0, 118, 0, 0, 0, 0, 0, 0, 3, 2, 1984, 0,
    2, 0, 5, 1, 2, 3, 4, 5, 11, 13, 17, 19,
    23, -29, 31, 37, 41, 43, 47, 61, 59, 61, 53, 0};

/// @brief Splits one logical word stream into the six fixed rows a family occupies.
std::vector<SyntheticRow> ssLineFixedRows(
    std::uint16_t cmper, const std::vector<std::int16_t>& words)
{
    std::vector<SyntheticRow> rows;
    for (std::size_t at = 0; at < words.size(); at += 6) {
        SyntheticRow row{cmper, "ls", {}};
        for (std::size_t slot = 0; slot < 6; ++slot) {
            row.words[slot] = words[at + slot];
        }
        rows.push_back(row);
    }
    return rows;
}

/// @brief A document carrying the two font definitions the character decoding needs.
/// @details Font 6 is an ordinary Mac text font, so a byte stored in it is Mac Roman. Font 0 is
/// the default music font, whose byte is a glyph number whatever its record claims.
musx::dom::DocumentPtr ssLineDocument(musx::factory::DocumentFactory::ConstructionSession& session)
{
    using FontDefinition = musx::dom::others::FontDefinition;
    const auto document = session.getDocument();
    const auto addFont = [&](musx::dom::Cmper cmper, int charsetVal) {
        auto font = std::make_shared<FontDefinition>(
            document, musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
        font->charsetBank = FontDefinition::CharacterSetBank::MacOS;
        font->charsetVal = charsetVal;
        document->getOthers()->add(FontDefinition::XmlNodeName, font);
    };
    addFont(0, 4095);
    addFont(6, 0);
    return document;
}

/// @brief Runs the custom-line importer over one synthesized container.
void ssLineImport(const finale_mus_reader::container::ParsedContainer& parsed,
    const SourceProfile& profile, const musx::dom::DocumentPtr& document, ImportReport& report)
{
    const auto index = LegacyRecordIndex::build(parsed);
    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto reference = std::move(referenceSession).finish();
    finale_mus_reader::PendingReferences pending;
    musx::factory::ConstructionContext construction;
    const finale_mus_reader::ImportContext context{
        index, profile, noSource, document, reference, report, pending, construction};
    finale_mus_reader::others::importSmartShapeCustomLines(context);
}

/// @brief Every field of every line style, in each epoch that carries the record.
/// @details The Coda-banner epoch is in the sweep to assert the opposite: the record does not
/// exist before Finale 2000, so rows carrying its tag must build nothing at all rather than
/// empty objects.
void testSmartShapeCustomLinesAcrossEpochs()
{
    using CustomLine = musx::dom::others::SmartShapeCustomLine;
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        const auto era = " in epoch " + std::to_string(static_cast<int>(epoch));
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = ssLineDocument(session);
        ImportReport report(FormatEpoch::UncompressedLegacy);
        SourceProfile profile(epoch);
        if (epoch == FormatEpoch::ZlibLegacy) {
            // One record per container, so each line style is imported into its own document
            // and then read back from a document of its own.
            for (const auto& record : {std::pair{std::uint16_t{1}, ssLineCharWords},
                     std::pair{std::uint16_t{2}, ssLineDashedWords},
                     std::pair{std::uint16_t{3}, ssLineCappedWords}}) {
                const auto parsed = makeClassContainer(
                    0x00de, record.second, ByteOrder::BigEndian, record.first);
                profile.byteOrder = parsed.byteOrder;
                ssLineImport(parsed, profile, document, report);
            }
        } else {
            std::vector<SyntheticRow> rows;
            for (const auto& record : {std::pair{std::uint16_t{1}, ssLineCharWords},
                     std::pair{std::uint16_t{2}, ssLineDashedWords},
                     std::pair{std::uint16_t{3}, ssLineCappedWords}}) {
                const auto family = ssLineFixedRows(record.first, record.second);
                rows.insert(rows.end(), family.begin(), family.end());
            }
            const auto parsed = makeContainer(rows, epoch);
            profile.byteOrder = parsed.byteOrder;
            ssLineImport(parsed, profile, document, report);
        }

        const auto lines = document->getOthers()->getArray<CustomLine>(musx::dom::SCORE_PARTID);
        if (epoch == FormatEpoch::CodaBanner) {
            expectMapping(lines.empty(),
                "A Coda-banner source built custom line styles from a record that era never"
                " wrote" + era);
            expectMapping(reportedFieldCount(report) == 0,
                "A Coda-banner source reported custom line style fields it cannot have" + era);
            continue;
        }
        expectMapping(lines.size() == 3, "Not every custom line style was built" + era);

        const auto charLine = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 1);
        expectMapping(charLine && charLine->lineStyle == CustomLine::LineStyle::Char,
            "The Char line style was not recovered" + era);
        expectMapping(charLine->charParams && !charLine->solidParams && !charLine->dashedParams,
            "A Char line kept a parameter block its line style does not select" + era);
        // 199 is Mac Roman in font 6, so the document must carry the code point rather than
        // the byte. The setter musxdom owns turns the stored mask into the two flags.
        expectMapping(charLine->charParams->lineChar == 0x00ab,
            "The Char line character was not decoded through its own font" + era);
        expectMapping(charLine->charParams->font->fontId == 6
                && charLine->charParams->font->fontSize == 17
                && charLine->charParams->font->bold && charLine->charParams->font->italic
                && !charLine->charParams->font->underline,
            "The Char line font tuple was not recovered" + era);
        expectMapping(charLine->charParams->baselineShiftEms == -83,
            "The Char line baseline shift was not recovered" + era);

        const auto dashed = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 2);
        expectMapping(dashed && dashed->dashedParams && !dashed->charParams
                && dashed->dashedParams->lineWidth == 118
                && dashed->dashedParams->dashOn == 192
                && dashed->dashedParams->dashOff == 448,
            "The Dashed line parameters were not recovered" + era);

        const auto capped = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 3);
        expectMapping(capped && capped->solidParams && capped->solidParams->lineWidth == 118,
            "The Solid line width was not recovered" + era);
        expectMapping(capped->lineCapStartType == CustomLine::LineCapType::Hook
                && capped->lineCapEndType == CustomLine::LineCapType::ArrowheadCustom
                && capped->lineCapStartHookLength == 1984 && capped->lineCapEndArrowId == 2
                && capped->lineCapStartArrowId == 0 && capped->lineCapEndHookLength == 0,
            "A line cap read its shared value slot as the wrong one of the two" + era);
        expectMapping(capped->makeHorz && !capped->lineAfterLeftStartText
                && capped->lineBeforeRightEndText && !capped->lineAfterLeftContText,
            "The line adjustment flags were not recovered" + era);
        expectMapping(capped->leftStartRawTextId == 1 && capped->leftContRawTextId == 2
                && capped->rightEndRawTextId == 3 && capped->centerFullRawTextId == 4
                && capped->centerAbbrRawTextId == 5,
            "The text anchor comparators were not recovered" + era);
        expectMapping(capped->leftStartX == 11 && capped->leftStartY == 13
                && capped->leftContX == 17 && capped->leftContY == 19
                && capped->rightEndX == 23 && capped->rightEndY == -29
                && capped->centerFullX == 31 && capped->centerFullY == 37
                && capped->centerAbbrX == 41 && capped->centerAbbrY == 43,
            "The anchor offsets did not interleave X with Y" + era);
        expectMapping(capped->lineStartX == 47 && capped->lineStartY == 61
                && capped->lineEndX == 59 && capped->lineEndY == 61 && capped->lineContX == 53,
            "The line adjustments were not recovered" + era);

        // A field the record does not carry is not reported at all, because the destination
        // does not exist for it. The nested destinations are named with dots, as every other
        // nested report target is.
        expectMapping(!fieldPresent(report, "others.smartShapeCustomLine[2].charParams.lineChar"),
            "A Dashed line reported a Char parameter it cannot have" + era);
        expectMapping(fieldPresent(report, "others.smartShapeCustomLine[3].lineCapStartHookLength")
                && !fieldPresent(report, "others.smartShapeCustomLine[3].lineCapEndHookLength"),
            "An arrowhead cap reported the hook length it does not use" + era);
        expectMapping(
            field(report, "others.smartShapeCustomLine[1].charParams.font.fontSize").origin
                == ValueOrigin::LegacyMus,
            "A nested destination was not reported under its dotted path" + era);
        // The report keeps the byte the source held, not the code point it decoded to.
        expectMapping(
            field(report, "others.smartShapeCustomLine[1].charParams.lineChar").rawValue == 199,
            "The report did not keep the stored character byte" + era);
    }
}

/// @brief The Finale 2012 layout, where only the Char parameter block moves with the character.
void testSmartShapeCustomLineUnicodeLayout()
{
    using CustomLine = musx::dom::others::SmartShapeCustomLine;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = ssLineDocument(session);
    ImportReport report(FormatEpoch::UncompressedLegacy);
    auto profile = profileFor(17);
    profile.epoch = FormatEpoch::ZlibLegacy;
    profile.byteOrder = ByteOrder::LittleEndian;
    for (const auto& record : {std::pair{std::uint16_t{1}, ssLineWideCharWords},
             std::pair{std::uint16_t{2}, ssLineWideDashedWords},
             std::pair{std::uint16_t{3}, ssLineWideCappedWords}}) {
        ssLineImport(makeClassContainer(0x00de, record.second, profile.byteOrder, record.first),
            profile, document, report);
    }

    const auto charLine = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 1);
    expectMapping(charLine && charLine->charParams,
        "The Unicode-era Char line was not recovered");
    // Already a code point in this era, so nothing is decoded and font 6 is not consulted.
    expectMapping(charLine->charParams->lineChar == 0x00ab,
        "The Unicode-era character was not read as a code point");
    expectMapping(charLine->charParams->font->fontId == 6
            && charLine->charParams->font->fontSize == 17
            && charLine->charParams->font->bold && charLine->charParams->font->italic
            && charLine->charParams->baselineShiftEms == -83,
        "The Char parameter block did not shift with the widened character");

    const auto dashed = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 2);
    expectMapping(dashed && dashed->dashedParams && dashed->dashedParams->dashOn == 192
            && dashed->dashedParams->dashOff == 448,
        "A Dashed record shifted with a character it does not have");

    const auto capped = document->getOthers()->get<CustomLine>(musx::dom::SCORE_PARTID, 3);
    expectMapping(capped && capped->lineCapStartType == CustomLine::LineCapType::Hook
            && capped->lineCapStartHookLength == 1984 && capped->lineCapEndArrowId == 2,
        "The common part of the record did not shift in the Unicode era");
    expectMapping(capped->leftStartX == 11 && capped->centerAbbrY == 43
            && capped->lineStartX == 47 && capped->lineContX == 53,
        "The positions did not shift in the Unicode era");
}


TEST_CASE("Smart shape custom lines span three epochs", "[mapping]") { testSmartShapeCustomLinesAcrossEpochs(); }
TEST_CASE("Smart shape custom line Unicode layout", "[mapping]") { testSmartShapeCustomLineUnicodeLayout(); }

} // namespace
} // namespace finale_mus_reader_tests
