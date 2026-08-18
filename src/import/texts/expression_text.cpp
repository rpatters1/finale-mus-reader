// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/texts.h"

#include <memory>
#include <string>
#include <vector>

#include "import/support/enigma_text.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace texts {
namespace {

using ExpressionTextTarget = musx::dom::texts::ExpressionText;

// Before the text pool learned to spell expression text out for itself, Finale kept it inside
// the text expression definition, as a plain character string with its font, size, and style
// packed into the record's first incidence. musxdom has no such arrangement: it expects an
// `expression` object in the texts pool carrying a complete Enigma string. So this
// reconstructs one -- the same object the later eras' text pool supplies directly.
//
// The `DT` family holds one expression per comparator:
//
//   incidence 0, byte 0   point size
//   incidence 0, byte 1   font definition comparator
//   incidence 0, word 1   the `nfx` style bits
//   incidence 1 onward    the display text, twelve bytes per row, ending at the first NUL
//
constexpr records::LegacyTag expressionRecord = records::packTag("DT");
constexpr std::size_t pointSizeByte = 0;
constexpr std::size_t fontByte = 1;
constexpr std::size_t effectsWord = 1;
constexpr std::uint32_t firstTextIncidence = 1;

/// @brief Reads the family's text incidences as one string, ending at the first NUL.
std::string readStoredText(const records::LegacyRowPool& pool,
    std::span<const records::LegacyRow> family)
{
    std::string result;
    for (const auto& row : family) {
        if (row.inci < firstTextIncidence) {
            continue;
        }
        for (const auto byte : pool.payloadOf(row)) {
            if (byte == 0) {
                return result;
            }
            result.push_back(static_cast<char>(byte));
        }
    }
    return result;
}

} // namespace

void importExpressionTexts(const ImportContext& context)
{
    // Uncompressed only, and the narrowness is the point.
    //
    // The Coda-banner epoch stores the same class in the same family under a different layout:
    // its size is a whole word rather than a packed byte and its text incidence carries further
    // fields after the string. That layout is not decoded.
    //
    // The DCL and zlib epochs are excluded for the opposite reason: they carry expression text
    // in the text pool, where `importTextPool` already reads it, and by then the string
    // embedded in `DT` is the expression's *description* instead. Reading that here would fill
    // the texts pool with category descriptions presented as expression text, which is worse
    // than recovering nothing.
    //
    // Where inside the DCL era the move happens is not established, so a document from the
    // early part of that era still using the old layout recovers no expression text.
    if (context.profile.epoch != FormatEpoch::UncompressedLegacy) {
        return;
    }

    const auto& pool = context.index.getOthers();
    const text::EnigmaTextSource source{context.document,
        versions::storesUnicodeCodepoints(context.profile.version),
        text::platformCodePage(context.profile.platform)};

    for (const auto cmper : pool.cmpersForTag(expressionRecord)) {
        const auto family = pool.getArray(expressionRecord, cmper);
        if (family.empty()) {
            continue;
        }
        const auto header = pool.payloadOf(family.front());
        if (header.size() <= fontByte) {
            continue;
        }
        const auto stored = readStoredText(pool, family);
        if (stored.empty()) {
            // An empty string here is indistinguishable from a comparator the file merely
            // reserved, so no object is created for one.
            continue;
        }

        // Restated as an Enigma string and then read back through the ordinary converter, so
        // that the font decides the encoding by exactly the rule the text pool uses. A caret
        // in the stored text is escaped first: this string is characters, not commands, and
        // the escape is how Enigma spells a literal caret.
        std::string spelled = "^fontid(" + std::to_string(header[fontByte]) + ")^size("
            + std::to_string(header[pointSizeByte]) + ")^nfx("
            + std::to_string(static_cast<std::uint16_t>(family.front().words[effectsWord])) + ")";
        for (const char character : stored) {
            if (character == '^') {
                spelled.push_back('^');
            }
            spelled.push_back(character);
        }
        const std::span<const std::uint8_t> bytes(
            reinterpret_cast<const std::uint8_t*>(spelled.data()), spelled.size());

        auto instance = std::make_shared<ExpressionTextTarget>(context.document,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All,
            static_cast<musx::dom::Cmper>(cmper));
        instance->text = text::toModernEnigmaText(bytes, source).text;
        context.document->getTexts()->add(ExpressionTextTarget::XmlNodeName, instance);

        FieldInfo info;
        info.target = "texts." + std::string(ExpressionTextTarget::XmlNodeName) + '['
            + std::to_string(cmper) + "].text";
        info.origin = ValueOrigin::LegacyMus;
        info.blockOffset = family.front().blockOffset;
        info.decodedOffset = family.front().decodedOffset;
        info.rawValue = static_cast<std::int64_t>(instance->text.size());
        context.report.fields.push_back(std::move(info));
    }
}

} // namespace texts
} // namespace finale_mus_reader
