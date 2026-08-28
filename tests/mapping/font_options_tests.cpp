// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "mapping_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace mapping;

void testMissingRecoveredFontDefinitionFallback()
{
    using FontDefinition = musx::dom::others::FontDefinition;
    using FontOptions = musx::dom::options::FontOptions;
    using FontType = FontOptions::FontType;

    auto targetSession = musx::factory::DocumentFactory::begin();
    const auto targetDocument = targetSession.getDocument();
    auto targetOptions = std::make_shared<FontOptions>(targetDocument);
    targetDocument->getOptions()->add(FontOptions::XmlNodeName, targetOptions);
    const auto addTargetFont = [&](musx::dom::Cmper cmper, const char* name) {
        auto font = std::make_shared<FontDefinition>(targetDocument, musx::dom::SCORE_PARTID,
            musx::dom::EnigmaBase::ShareMode::All, cmper);
        font->name = name;
        targetDocument->getOthers()->add(FontDefinition::XmlNodeName, font);
    };
    addTargetFont(0, "Seville");
    addTargetFont(5, "Arial");
    const auto addMissingOption = [&](FontType type, int size, std::uint16_t effects) {
        auto font = std::make_shared<musx::dom::FontInfo>(targetDocument);
        font->fontId = 99;
        font->fontSize = size;
        font->setEnigmaStyles(effects);
        targetOptions->fontOptions.emplace(type, font);
    };
    addMissingOption(FontType::Fretboard, 36, 1);
    addMissingOption(FontType::Tablature, 12, 2);

    auto referenceSession = musx::factory::DocumentFactory::begin();
    const auto referenceDocument = referenceSession.getDocument();
    auto referenceOptions = std::make_shared<FontOptions>(referenceDocument);
    referenceDocument->getOptions()->add(FontOptions::XmlNodeName, referenceOptions);
    // The reference carries a size and effects distinct from the source's, so the
    // assertions below can tell which document each part of the tuple came from.
    const auto addReference = [&](FontType type, musx::dom::Cmper cmper, const char* name,
                                  int size, std::uint16_t effects) {
        auto definition = std::make_shared<FontDefinition>(referenceDocument,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, cmper);
        definition->name = name;
        referenceDocument->getOthers()->add(FontDefinition::XmlNodeName, definition);
        auto font = std::make_shared<musx::dom::FontInfo>(referenceDocument);
        font->fontId = cmper;
        font->fontSize = size;
        font->setEnigmaStyles(effects);
        referenceOptions->fontOptions.emplace(type, font);
    };
    addReference(FontType::Fretboard, 3, "Seville", 24, 4);
    addReference(FontType::Tablature, 4, " arIAL ", 18, 5);

    ImportReport report(FormatEpoch::UncompressedLegacy);
    finale_mus_reader::options::repairMissingRecoveredFontDefinitions(
        targetDocument, referenceDocument, targetOptions, report);

    // The whole tuple comes from the reference, not just the face. A point size is not
    // independent of the face it was chosen for, so pairing a substituted face with the
    // source's size would produce a combination present in neither document. The source
    // values here are 36/1 and 12/2; both must be gone.
    const auto fretboard = targetOptions->getFontInfo(FontType::Fretboard);
    expectMapping(fretboard->fontId == 6 && fretboard->fontSize == 24
            && fretboard->getEnigmaStyles() == 4,
        "A same-type reference face was not cloned after the highest target comparator");
    expectMapping(targetDocument->getOthers()->get<FontDefinition>(
            musx::dom::SCORE_PARTID, 6)->name == "Seville",
        "The cloned same-type reference face did not retain its reference spelling");
    const auto tablature = targetOptions->getFontInfo(FontType::Tablature);
    expectMapping(tablature->fontId == 5 && tablature->fontSize == 18
            && tablature->getEnigmaStyles() == 5,
        "A normalized nonzero target face was not reused by the fallback");
    expectMapping(targetDocument->getOthers()->getArray<FontDefinition>(
            musx::dom::SCORE_PARTID).size() == 3,
        "The fallback introduced a duplicate nonzero font name");
    // The fallback is silent by design: it is a considered substitution that leaves the
    // document usable, and a warning would surface it in user interfaces as though
    // something had gone wrong. Callers distinguish substituted values from recovered ones
    // through the reported ValueOrigin, not through a message.
    expectMapping(std::none_of(report.diagnostics.begin(), report.diagnostics.end(),
                      [](const finale_mus_reader::Diagnostic& entry) {
                          return entry.level == musx::util::Logger::LogLevel::Warning;
                      }),
        "The designed-in font substitution emitted a user-facing warning");
    expectMapping(report.diagnostics.size() == 2,
        "The font substitution was not recorded at verbose level");
}

TEST_CASE("Missing recovered font definition fallback", "[mapping]") { testMissingRecoveredFontDefinitionFallback(); }

} // namespace
} // namespace finale_mus_reader_tests
