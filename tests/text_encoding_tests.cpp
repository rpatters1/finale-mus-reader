// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>

#include <string>

#include "import/text_encoding.h"

using finale_mus_reader::text::CodePage;
using finale_mus_reader::text::codePageForCharset;
using finale_mus_reader::text::toUtf8;

using Bank = musx::dom::others::FontDefinition::CharacterSetBank;

namespace {

// Font-name bytes taken verbatim from surveyed Finale 2007 and 2008 documents. Each is a
// real macOS font of the era, and each decodes to legible text under exactly one encoding,
// which is what makes the charset value worth trusting rather than guessing.
const std::string kHiraginoMincho = "\x83\x71\x83\x89\x83\x4d\x83\x6d\x96\xbe\x92\xa9 Pro W3";
const std::string kHuawenFangsong = "\xbb\xaa\xce\xc4\xb7\xc2\xcb\xce";
const std::string kLiSong = "\xc4\xd7\xa7\xba Pro";
// MacRoman 0x8e is e-acute; the same byte is a capital Z-caron in Windows-1252.
const std::string kMacAccented = "C\x8elino";

} // namespace

TEST_CASE("Mac script codes select the encoding", "[text]")
{
    CHECK(codePageForCharset(Bank::MacOS, 0) == CodePage::MacRoman);
    CHECK(codePageForCharset(Bank::MacOS, 1) == CodePage::MacJapanese);
    CHECK(codePageForCharset(Bank::MacOS, 2) == CodePage::MacTradChinese);
    CHECK(codePageForCharset(Bank::MacOS, 3) == CodePage::MacKorean);
    CHECK(codePageForCharset(Bank::MacOS, 25) == CodePage::MacSimpChinese);
    // Single-byte scripts where the Mac encoding differs from the Windows one. The corpus
    // corroborates two of these by name: fonts stored under 7 and 29 are themselves called
    // "... CY" and "... CE".
    CHECK(codePageForCharset(Bank::MacOS, 6) == CodePage::MacGreek);
    CHECK(codePageForCharset(Bank::MacOS, 7) == CodePage::MacCyrillic);
    CHECK(codePageForCharset(Bank::MacOS, 29) == CodePage::MacCentralEurope);
}

TEST_CASE("Windows charset values select the encoding", "[text]")
{
    // ANSI is pinned rather than resolved against the running machine's code page, so the
    // same file decodes identically everywhere.
    CHECK(codePageForCharset(Bank::Windows, 0) == CodePage::Windows1252);
    CHECK(codePageForCharset(Bank::Windows, 128) == CodePage::ShiftJis);
    CHECK(codePageForCharset(Bank::Windows, 129) == CodePage::Korean);
    CHECK(codePageForCharset(Bank::Windows, 136) == CodePage::Big5);
    CHECK(codePageForCharset(Bank::Windows, 186) == CodePage::Windows1257);
    CHECK(codePageForCharset(Bank::Windows, 238) == CodePage::Windows1250);
}

TEST_CASE("A charset that names no script falls to the bank default", "[text]")
{
    // Symbol fonts are not special-cased. musxdom's calcIsSymbolFont describes how
    // character codes in text *set in* the font map to glyphs; the font's own name is
    // ordinary platform text and is converted like any other.
    CHECK(codePageForCharset(Bank::MacOS, 0xfff) == CodePage::MacRoman);
    CHECK(codePageForCharset(Bank::Windows, 2) == CodePage::Windows1252);
    // Windows DEFAULT and OEM name no encoding either.
    CHECK(codePageForCharset(Bank::Windows, 1) == CodePage::Windows1252);
    CHECK(codePageForCharset(Bank::Windows, 255) == CodePage::Windows1252);
    // smUninterp (32) is the Script Manager's own "not text in any script" code, and
    // Finale's separate 0xfff sentinel means the same thing. A font's name is ordinary
    // platform text whatever its content is, so both convert as Mac Roman.
    CHECK(codePageForCharset(Bank::MacOS, 32) == CodePage::MacRoman);
    // Nor does any value no survey has yet produced. This is the starting position, to be
    // revised when a file demands it, rather than a claim about what Finale wrote.
    CHECK(codePageForCharset(Bank::MacOS, 99) == CodePage::MacRoman);
    CHECK(codePageForCharset(Bank::Windows, 77) == CodePage::Windows1252);
}

TEST_CASE("Seven-bit names survive conversion unchanged", "[text]")
{
    // These go through a converter like everything else rather than short-circuiting on
    // ASCII, so this also asserts the conversion path is right for the bulk of real input:
    // every encoding here agrees with ASCII below 0x80.
    const std::string ascii = "Petrucci";
    CHECK(toUtf8(ascii, CodePage::MacRoman) == ascii);
    CHECK(toUtf8(ascii, CodePage::ShiftJis) == ascii);
    CHECK(toUtf8(ascii, CodePage::Windows1252) == ascii);
    CHECK(toUtf8(ascii, CodePage::Big5) == ascii);
    CHECK(toUtf8(ascii, CodePage::Gb2312) == ascii);
    CHECK(toUtf8(ascii, CodePage::Utf8) == ascii);
    CHECK(toUtf8("", CodePage::Big5).empty());
}

TEST_CASE("Legacy font names convert to UTF-8", "[text]")
{
    CHECK(toUtf8(kHiraginoMincho, codePageForCharset(Bank::MacOS, 1))
        == "ヒラギノ明朝 Pro W3");
    CHECK(toUtf8(kHuawenFangsong, codePageForCharset(Bank::MacOS, 25))
        == "华文仿宋");
    CHECK(toUtf8(kLiSong, codePageForCharset(Bank::MacOS, 2)) == "儷宋 Pro");
    CHECK(toUtf8(kMacAccented, codePageForCharset(Bank::MacOS, 0)) == "Célino");
}

TEST_CASE("Mac Roman is not Windows-1252", "[text]")
{
    // The two agree below 0x80 and disagree above it. Treating a Mac Roman name as ANSI is
    // the specific mistake this mapping exists to prevent, and it is why a pre-3.2 file
    // synthesizes its bank from the document's own operating system.
    CHECK(toUtf8(kMacAccented, CodePage::MacRoman) == "Célino");
    CHECK(toUtf8(kMacAccented, CodePage::Windows1252) == "CŽlino");
    CHECK(toUtf8(kMacAccented, CodePage::MacRoman) != toUtf8(kMacAccented, CodePage::Windows1252));
}

TEST_CASE("The single-byte Mac encodings agree on every platform", "[text]")
{
    // Each platform reaches these through a different route: CoreFoundation on macOS,
    // MultiByteToWideChar on Windows, and the embedded tables everywhere else, since musl's
    // iconv — and therefore Emscripten's — rejects all four outright. Asserting concrete
    // results is what keeps those three routes from drifting apart.
    CHECK(toUtf8("C\x8elino", CodePage::MacRoman) == "Célino");
    CHECK(toUtf8("\xa5\xc7", CodePage::MacRoman) == "•«");
    CHECK(toUtf8("\x81\x82", CodePage::MacGreek) == "¹²");
    CHECK(toUtf8("\x80\x81\x82", CodePage::MacCyrillic) == "АБВ");
    CHECK(toUtf8("\x81\x82", CodePage::MacCentralEurope) == "Āā");
    // Below 0x80 every one of them is ASCII.
    CHECK(toUtf8("Petrucci", CodePage::MacGreek) == "Petrucci");
    CHECK(toUtf8("Petrucci", CodePage::MacCyrillic) == "Petrucci");
    CHECK(toUtf8("Petrucci", CodePage::MacCentralEurope) == "Petrucci");
}

TEST_CASE("A name that contradicts its charset keeps its bytes", "[text]")
{
    // Preserved mojibake can be re-decoded once the encoding is understood; discarded bytes
    // cannot, and a thrown exception mid-import loses the whole document.
    const std::string notShiftJis = "\xff\xfe\xff\xfe";
    CHECK(toUtf8(notShiftJis, CodePage::ShiftJis) == notShiftJis);
}
