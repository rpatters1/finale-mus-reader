// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "finale_mus_reader/reader.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace text {

/// @brief Windows code page numbers, used as the portable name for an encoding.
/// @details The numbers are Windows code pages even on platforms that have no concept of
/// one, because that is the only identifier all three platforms can be driven from:
/// Windows consumes them directly through `MultiByteToWideChar`, and elsewhere they select
/// an iconv encoding name.
///
/// musxdom has no equivalent and needs none. EnigmaXML is always UTF-8, so the document
/// model never sees legacy bytes. Converting them is this library's job.
///
/// **Design goal: the best result obtainable on the machine that is running.** Legacy text
/// conversion is not required to be bit-identical across platforms, and pursuing that would
/// mean discarding real accuracy. Windows can name encodings that iconv cannot, so it is
/// given the more faithful code page rather than held back to a common subset; the other
/// platforms fall back to the nearest converter they have. Where the fallback is known to
/// be adequate, that is recorded next to it.
enum class CodePage : int
{
    /// @brief No encoding named; the platform's own default applies.
    /// @details Zero is what Windows calls `CP_ACP`, the machine's active code page, so the
    /// Windows path resolves this natively. Nothing else can: a code page that means
    /// "whatever this machine is set to" cannot be given a fixed iconv name, and a file
    /// that decoded differently on two machines would not be reproducible anyway. Reading
    /// legacy MUS therefore never selects this; it exists so the enum can express what a
    /// Windows charset field is actually saying.
    Platform = 0,
    Utf8 = 65001,         ///< Already UTF-8; nothing to convert.

    /// @name Windows code pages
    /// @{
    Windows1250 = 1250,   ///< Central European.
    Windows1251 = 1251,   ///< Cyrillic.
    Windows1252 = 1252,   ///< Western European, the "ANSI" code page.
    Windows1253 = 1253,   ///< Greek.
    Windows1254 = 1254,   ///< Turkish.
    Windows1255 = 1255,   ///< Hebrew.
    Windows1256 = 1256,   ///< Arabic.
    Windows1257 = 1257,   ///< Baltic.
    Thai874 = 874,        ///< Thai.
    ShiftJis = 932,       ///< Japanese.
    Gb2312 = 936,         ///< Simplified Chinese.
    Korean = 949,         ///< Korean.
    Big5 = 950,           ///< Traditional Chinese.
    /// @}

    /// @name Classic Mac encodings
    /// @details Numbered as Windows numbers them: 10000 plus the Mac script code.
    ///
    /// Windows resolves every one of these natively and exactly. iconv has no Mac-specific
    /// name for the CJK four, so there they fall back to Shift-JIS, Big5, EUC-KR and
    /// GB2312, which decode Mac-stored CJK font names correctly. That
    /// asymmetry is intended: Windows gets the more faithful answer rather than being held
    /// to what iconv can express.
    /// @{
    MacRoman = 10000,
    MacJapanese = 10001,
    MacTradChinese = 10002,
    MacKorean = 10003,
    MacArabic = 10004,
    MacHebrew = 10005,
    MacGreek = 10006,
    MacCyrillic = 10007,
    MacSimpChinese = 10008,
    MacThai = 10021,
    MacCentralEurope = 10029,
    MacTurkish = 10081,
    /// @}
};

/// @brief The code page a font's text is stored in, from the font's own charset fields.
/// @details Legacy MUS stores text in whatever encoding the saving machine used and does
/// not record that encoding globally. What it does record is per font: `charsetBank` names
/// the platform whose charset numbering applies, and `charsetVal` selects within it. That
/// is enough to decode, and it is why conversion is driven from the font rather than from
/// the document's platform — a Mac font can appear in a document saved on Windows.
///
/// A concrete code page is always returned. Where `charsetVal` names no script — a symbol
/// font, Windows `DEFAULT` or `OEM`, or a value not yet seen — the bank's own
/// default applies: Mac Roman for the Mac bank, Windows-1252 for the Windows bank. That is
/// a starting position rather than a discovery, and it is meant to be revised the moment a
/// file contradicts it.
///
/// Symbol fonts are not special-cased. `calcIsSymbolFont` describes how character codes in
/// text *set in* that font map to glyphs; the font's own name is ordinary platform text.
CodePage codePageForCharset(
    musx::dom::others::FontDefinition::CharacterSetBank bank, int charsetVal);

/// @brief Converts legacy bytes to UTF-8, or returns them unchanged when it cannot.
/// @details Every name goes through a converter, including a name that is entirely 7-bit.
/// Skipping ASCII would give the same answer for every encoding here, since all of them
/// agree with ASCII below 0x80, but it would also mean the conversion path went untested
/// against the overwhelming majority of real input.
///
/// A failed conversion returns @p source unchanged. Mojibake that preserves the original
/// bytes is recoverable by a later, better-informed pass; a thrown exception in the middle
/// of importing a document is not, and an empty string silently destroys evidence.
std::string toUtf8(const std::string& source, CodePage codePage);

/// @brief Restates legacy line breaks in the convention EnigmaXML uses.
/// @details Legacy MUS separates the lines of a text block with a carriage return, which is
/// the classic Mac convention Finale was built on and which it kept on both platforms.
/// EnigmaXML uses a line feed, and carries no `&#xD;`. A carriage return left in place would
/// not survive an XML round trip either, since XML parsers normalize a literal one to a line
/// feed regardless.
///
/// A `\r\n` pair becomes one `\n`, so a file written by a Windows Finale is not turned into
/// double-spaced text.
std::string normalizeLineBreaks(std::string source);

/// @brief The code page for legacy text that no font names an encoding for.
/// @details Some text belongs to the document rather than to a font: the File Info strings in
/// the header, and the name inside a font command. Nothing records a character set for those,
/// so the document's own platform decides. Resolved through @ref codePageForCharset with no
/// character set value, so the platform default is stated in exactly one place: Mac Roman for
/// the Mac bank, Windows-1252 for the Windows bank.
///
/// An unclassified platform is treated as Mac, matching what the font importer does with a
/// pre-Finale-3.2 document that records no bank of its own.
CodePage platformCodePage(SourcePlatform platform);

/// @brief Widens every byte to the code point of the same value and encodes that as UTF-8.
/// @details The conversion for text set in a symbol font, where a byte is a glyph number
/// rather than a character. Decoding such a byte through a code page would name a letter the
/// document never contained and, worse, would pick a different glyph: Mac Roman reads `0xb0`
/// as an infinity sign, so a metronome mark set in a music font would come out as one.
/// Preserving the number is what keeps the glyph, and it is what Finale's own conversion does
/// with a font whose character set it cannot name.
///
/// Cannot fail and needs no converter, which is why it is spelled out here rather than added
/// to @ref CodePage as Latin-1: every input byte has exactly one answer.
std::string symbolBytesToUtf8(std::string_view source);

} // namespace text
} // namespace finale_mus_reader
