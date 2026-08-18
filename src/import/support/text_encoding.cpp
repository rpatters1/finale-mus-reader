// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/support/text_encoding.h"

#include <string_view>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <vector>
#else
#include <cerrno>
#include <cstring>
#include <iconv.h>
#include <vector>
#endif

namespace finale_mus_reader {
namespace text {

namespace {

using Bank = musx::dom::others::FontDefinition::CharacterSetBank;

// Windows LOGFONT charset values, from wingdi.h.
constexpr int windowsAnsiCharset = 0;
constexpr int windowsDefaultCharset = 1;
constexpr int windowsShiftJisCharset = 128;
constexpr int windowsHangeulCharset = 129;
constexpr int windowsGb2312Charset = 134;
constexpr int windowsBig5Charset = 136;
constexpr int windowsGreekCharset = 161;
constexpr int windowsTurkishCharset = 162;
constexpr int windowsVietnameseCharset = 163;
constexpr int windowsHebrewCharset = 177;
constexpr int windowsArabicCharset = 178;
constexpr int windowsBalticCharset = 186;
constexpr int windowsCyrillicCharset = 204;
constexpr int windowsThaiCharset = 222;
constexpr int windowsEastEuropeCharset = 238;
constexpr int windowsOemCharset = 255;

// Classic Mac Script Manager codes, from Apple's Script.h. The surveyed corpora confirm
// several directly: names stored under 1, 2, and 25 decode to correct Japanese, Traditional
// Chinese, and Simplified Chinese and to nothing legible otherwise, and fonts whose own
// names end `CY` and `CE` are stored under 7 and 29 respectively.
constexpr int macRomanScript = 0;
constexpr int macJapaneseScript = 1;
constexpr int macTradChineseScript = 2;
constexpr int macKoreanScript = 3;
constexpr int macArabicScript = 4;
constexpr int macHebrewScript = 5;
constexpr int macGreekScript = 6;
constexpr int macCyrillicScript = 7;
constexpr int macThaiScript = 21;
constexpr int macSimpChineseScript = 25;
constexpr int macCentralEuroScript = 29;
constexpr int macTurkishScript = 81;
// smUninterp, the last of the script codes. Apple glosses it as uninterpreted symbols: the
// character codes are not text in any script and are not to be given a linguistic reading.
// It is the Script Manager's way of saying "symbol font", and Finale does use it — two
// fonts in the surveyed corpora carry it — even though Finale's usual Mac symbol marker is
// the separate 0xfff sentinel. A font's *name* is still ordinary text whatever its content
// is, so this resolves to the bank default like any other non-script value.
constexpr int macUninterpretedScript = 32;

#if !defined(_WIN32) && !defined(__APPLE__)

// Single-byte classic Mac encodings, embedded rather than delegated.
//
// iconv cannot be relied on to carry these. musl's iconv — and therefore Emscripten's —
// rejects MACINTOSH, MACGREEK, MACCYRILLIC and MACCENTRALEUROPE outright, while accepting
// every Windows and CJK encoding this project uses. Mac Roman is the single most common
// encoding in the corpora (80,226 fonts carry the Mac bank with charset 0), so leaving it
// to a converter that may not exist would silently return raw bytes for the majority case.
//
// Each table maps the 128 bytes from 0x80 upward; everything below that is ASCII and passes
// through. The values were generated from Python's stdlib codecs and checked entry by entry
// against macOS CoreFoundation: all 512 agree, so two independent sources back every row.

constexpr char16_t macRomanTable[] = {
    0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1,   // 0x80
    0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8,   // 0x88
    0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3,   // 0x90
    0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc,   // 0x98
    0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df,   // 0xA0
    0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8,   // 0xA8
    0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211,   // 0xB0
    0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8,   // 0xB8
    0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab,   // 0xC0
    0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153,   // 0xC8
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca,   // 0xD0
    0x00ff, 0x0178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02,   // 0xD8
    0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1,   // 0xE0
    0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4,   // 0xE8
    0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc,   // 0xF0
    0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7,   // 0xF8
};

constexpr char16_t macGreekTable[] = {
    0x00c4, 0x00b9, 0x00b2, 0x00c9, 0x00b3, 0x00d6, 0x00dc, 0x0385,   // 0x80
    0x00e0, 0x00e2, 0x00e4, 0x0384, 0x00a8, 0x00e7, 0x00e9, 0x00e8,   // 0x88
    0x00ea, 0x00eb, 0x00a3, 0x2122, 0x00ee, 0x00ef, 0x2022, 0x00bd,   // 0x90
    0x2030, 0x00f4, 0x00f6, 0x00a6, 0x20ac, 0x00f9, 0x00fb, 0x00fc,   // 0x98
    0x2020, 0x0393, 0x0394, 0x0398, 0x039b, 0x039e, 0x03a0, 0x00df,   // 0xA0
    0x00ae, 0x00a9, 0x03a3, 0x03aa, 0x00a7, 0x2260, 0x00b0, 0x00b7,   // 0xA8
    0x0391, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x0392, 0x0395, 0x0396,   // 0xB0
    0x0397, 0x0399, 0x039a, 0x039c, 0x03a6, 0x03ab, 0x03a8, 0x03a9,   // 0xB8
    0x03ac, 0x039d, 0x00ac, 0x039f, 0x03a1, 0x2248, 0x03a4, 0x00ab,   // 0xC0
    0x00bb, 0x2026, 0x00a0, 0x03a5, 0x03a7, 0x0386, 0x0388, 0x0153,   // 0xC8
    0x2013, 0x2015, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x0389,   // 0xD0
    0x038a, 0x038c, 0x038e, 0x03ad, 0x03ae, 0x03af, 0x03cc, 0x038f,   // 0xD8
    0x03cd, 0x03b1, 0x03b2, 0x03c8, 0x03b4, 0x03b5, 0x03c6, 0x03b3,   // 0xE0
    0x03b7, 0x03b9, 0x03be, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03bf,   // 0xE8
    0x03c0, 0x03ce, 0x03c1, 0x03c3, 0x03c4, 0x03b8, 0x03c9, 0x03c2,   // 0xF0
    0x03c7, 0x03c5, 0x03b6, 0x03ca, 0x03cb, 0x0390, 0x03b0, 0x00ad,   // 0xF8
};

constexpr char16_t macCyrillicTable[] = {
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,   // 0x80
    0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f,   // 0x88
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,   // 0x90
    0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e, 0x042f,   // 0x98
    0x2020, 0x00b0, 0x0490, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x0406,   // 0xA0
    0x00ae, 0x00a9, 0x2122, 0x0402, 0x0452, 0x2260, 0x0403, 0x0453,   // 0xA8
    0x221e, 0x00b1, 0x2264, 0x2265, 0x0456, 0x00b5, 0x0491, 0x0408,   // 0xB0
    0x0404, 0x0454, 0x0407, 0x0457, 0x0409, 0x0459, 0x040a, 0x045a,   // 0xB8
    0x0458, 0x0405, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab,   // 0xC0
    0x00bb, 0x2026, 0x00a0, 0x040b, 0x045b, 0x040c, 0x045c, 0x0455,   // 0xC8
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x201e,   // 0xD0
    0x040e, 0x045e, 0x040f, 0x045f, 0x2116, 0x0401, 0x0451, 0x044f,   // 0xD8
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,   // 0xE0
    0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,   // 0xE8
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,   // 0xF0
    0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x20ac,   // 0xF8
};

constexpr char16_t macCentralEuropeTable[] = {
    0x00c4, 0x0100, 0x0101, 0x00c9, 0x0104, 0x00d6, 0x00dc, 0x00e1,   // 0x80
    0x0105, 0x010c, 0x00e4, 0x010d, 0x0106, 0x0107, 0x00e9, 0x0179,   // 0x88
    0x017a, 0x010e, 0x00ed, 0x010f, 0x0112, 0x0113, 0x0116, 0x00f3,   // 0x90
    0x0117, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x011a, 0x011b, 0x00fc,   // 0x98
    0x2020, 0x00b0, 0x0118, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df,   // 0xA0
    0x00ae, 0x00a9, 0x2122, 0x0119, 0x00a8, 0x2260, 0x0123, 0x012e,   // 0xA8
    0x012f, 0x012a, 0x2264, 0x2265, 0x012b, 0x0136, 0x2202, 0x2211,   // 0xB0
    0x0142, 0x013b, 0x013c, 0x013d, 0x013e, 0x0139, 0x013a, 0x0145,   // 0xB8
    0x0146, 0x0143, 0x00ac, 0x221a, 0x0144, 0x0147, 0x2206, 0x00ab,   // 0xC0
    0x00bb, 0x2026, 0x00a0, 0x0148, 0x0150, 0x00d5, 0x0151, 0x014c,   // 0xC8
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca,   // 0xD0
    0x014d, 0x0154, 0x0155, 0x0158, 0x2039, 0x203a, 0x0159, 0x0156,   // 0xD8
    0x0157, 0x0160, 0x201a, 0x201e, 0x0161, 0x015a, 0x015b, 0x00c1,   // 0xE0
    0x0164, 0x0165, 0x00cd, 0x017d, 0x017e, 0x016a, 0x00d3, 0x00d4,   // 0xE8
    0x016b, 0x016e, 0x00da, 0x016f, 0x0170, 0x0171, 0x0172, 0x0173,   // 0xF0
    0x00dd, 0x00fd, 0x0137, 0x017b, 0x0141, 0x017c, 0x0122, 0x02c7,   // 0xF8
};

/// @brief The embedded table for a code page, or nullptr if it has none.
const char16_t* macTableFor(CodePage codePage)
{
    switch (codePage) {
    case CodePage::MacRoman:         return macRomanTable;
    case CodePage::MacGreek:         return macGreekTable;
    case CodePage::MacCyrillic:      return macCyrillicTable;
    case CodePage::MacCentralEurope: return macCentralEuropeTable;
    default:                         return nullptr;
    }
}

/// @brief Decodes a single-byte encoding through an embedded table, emitting UTF-8.
/// @details Cannot fail: every one of the 256 inputs has a defined code point, which is
/// what makes these tables preferable to a converter that may or may not be installed.
std::string convertWithTable(const char16_t* table, const std::string& source)
{
    std::string out;
    out.reserve(source.size());
    for (const char raw : source) {
        const auto byte = static_cast<unsigned char>(raw);
        const char32_t code = byte < 0x80 ? byte : table[byte - 0x80];
        if (code < 0x80) {
            out.push_back(static_cast<char>(code));
        } else if (code < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (code >> 6)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        } else {
            // Every entry is in the basic multilingual plane, so three bytes always suffice.
            out.push_back(static_cast<char>(0xE0 | (code >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
    }
    return out;
}

/// @brief The iconv name for a code page, or nullptr when there is no equivalent.
const char* iconvNameFor(CodePage codePage)
{
    switch (codePage) {
    case CodePage::Utf8:              return "UTF-8";
    case CodePage::Windows1250:       return "CP1250";
    case CodePage::Windows1251:       return "CP1251";
    case CodePage::Windows1252:       return "CP1252";
    case CodePage::Windows1253:       return "CP1253";
    case CodePage::Windows1254:       return "CP1254";
    case CodePage::Windows1255:       return "CP1255";
    case CodePage::Windows1256:       return "CP1256";
    case CodePage::Windows1257:       return "CP1257";
    case CodePage::Thai874:           return "CP874";
    case CodePage::ShiftJis:          return "SHIFT-JIS";
    case CodePage::Gb2312:            return "GB2312";
    case CodePage::Korean:            return "EUC-KR";
    case CodePage::Big5:              return "BIG5";
    case CodePage::MacRoman:          return "MACINTOSH";
    // No Mac-specific iconv name exists for these four. The corpus confirms the fallbacks
    // decode Mac-stored CJK font names correctly; Windows uses the exact Mac code page.
    case CodePage::MacJapanese:       return "SHIFT-JIS";
    case CodePage::MacTradChinese:    return "BIG5";
    case CodePage::MacKorean:         return "EUC-KR";
    case CodePage::MacSimpChinese:    return "GB2312";
    case CodePage::MacArabic:         return "MACARABIC";
    case CodePage::MacHebrew:         return "MACHEBREW";
    case CodePage::MacGreek:          return "MACGREEK";
    case CodePage::MacCyrillic:       return "MACCYRILLIC";
    case CodePage::MacThai:           return "MACTHAI";
    case CodePage::MacCentralEurope:  return "MACCENTRALEUROPE";
    case CodePage::MacTurkish:        return "MACTURKISH";
    case CodePage::Platform:
        // Deliberately unmapped: see the enumerator's documentation.
        return nullptr;
    }
    return nullptr;
}

/// @brief Converts with iconv, growing the output buffer until the input is consumed.
std::optional<std::string> convertWithIconv(const char* fromEncoding, const std::string& source)
{
    const iconv_t converter = iconv_open("UTF-8", fromEncoding);
    if (converter == reinterpret_cast<iconv_t>(-1)) {
        return std::nullopt;
    }

    // UTF-8 needs at most four bytes per input byte for any encoding handled here, so this
    // is sized once and only grown if a converter proves otherwise.
    std::vector<char> output(source.size() * 4 + 4);
    // iconv takes a non-const input pointer even though it does not write through it.
    std::string input = source;
    char* inputCursor = input.data();
    std::size_t inputLeft = input.size();
    char* outputCursor = output.data();
    std::size_t outputLeft = output.size();

    while (inputLeft > 0) {
        const std::size_t result =
            iconv(converter, &inputCursor, &inputLeft, &outputCursor, &outputLeft);
        if (result != static_cast<std::size_t>(-1)) {
            break;
        }
        if (errno == E2BIG) {
            const std::size_t used = static_cast<std::size_t>(outputCursor - output.data());
            output.resize(output.size() * 2);
            outputCursor = output.data() + used;
            outputLeft = output.size() - used;
            continue;
        }
        // EILSEQ or EINVAL: the bytes are not what the charset field claimed.
        iconv_close(converter);
        return std::nullopt;
    }

    iconv_close(converter);
    return std::string(output.data(), static_cast<std::size_t>(outputCursor - output.data()));
}

#endif // !defined(_WIN32) && !defined(__APPLE__)

/// @brief Converts @p source from @p codePage to UTF-8, or nullopt if it cannot.
/// @details Each platform is given the most accurate converter it has rather than a common
/// subset, per the design goal in the header.
///
/// Windows drives its own APIs with the code page number directly. macOS uses
/// CoreFoundation, which resolves every one of these numbers natively — including the
/// classic Mac encodings, where it is the only one of the three that can decode the true
/// Mac variants of the CJK scripts rather than their plain Shift-JIS, Big5 and GB2312
/// approximations. Other platforms use iconv, which has no name for those variants.
///
/// CoreFoundation is used rather than Foundation or Cocoa deliberately: it is pure C, so it
/// compiles in this translation unit with no Objective-C toolchain, and `NSString` would add
/// nothing here.
std::optional<std::string> convert(CodePage codePage, const std::string& source)
{
    if (source.empty()) {
        return std::string{};
    }

#if defined(_WIN32)
    const auto page = static_cast<UINT>(codePage);
    const int wideLength = MultiByteToWideChar(
        page, MB_ERR_INVALID_CHARS, source.data(), static_cast<int>(source.size()), nullptr, 0);
    if (wideLength <= 0) {
        return std::nullopt;
    }
    std::wstring wide(static_cast<std::size_t>(wideLength), L'\0');
    if (MultiByteToWideChar(page, MB_ERR_INVALID_CHARS, source.data(),
            static_cast<int>(source.size()), wide.data(), wideLength)
        != wideLength) {
        return std::nullopt;
    }

    const int utf8Length = WideCharToMultiByte(
        CP_UTF8, 0, wide.data(), wideLength, nullptr, 0, nullptr, nullptr);
    if (utf8Length <= 0) {
        return std::nullopt;
    }
    std::string utf8(static_cast<std::size_t>(utf8Length), '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideLength, utf8.data(), utf8Length,
            nullptr, nullptr)
        != utf8Length) {
        return std::nullopt;
    }
    return utf8;

#elif defined(__APPLE__)
    // CFStringConvertWindowsCodepageToEncoding maps the whole range this project uses,
    // including the two places where the Windows number and the Mac script code differ:
    // code page 10008 is Simplified Chinese, whose script code is 25, and 10081 is Turkish,
    // whose encoding is 35. Deriving the encoding by subtracting 10000 would be wrong for
    // both, so the documented conversion is used instead of arithmetic.
    const CFStringEncoding encoding =
        CFStringConvertWindowsCodepageToEncoding(static_cast<UInt32>(codePage));
    if (encoding == kCFStringEncodingInvalidId || !CFStringIsEncodingAvailable(encoding)) {
        return std::nullopt;
    }

    const CFStringRef decoded = CFStringCreateWithBytes(nullptr,
        reinterpret_cast<const UInt8*>(source.data()), static_cast<CFIndex>(source.size()),
        encoding, /*isExternalRepresentation*/ false);
    if (decoded == nullptr) {
        // The bytes are not valid in the encoding the charset field claimed.
        return std::nullopt;
    }

    // Two passes: the first sizes the buffer, the second fills it. CFStringGetBytes is used
    // rather than CFStringGetCString because a converted name may legitimately contain an
    // embedded NUL, which a C-string interface would silently truncate.
    const CFRange range = CFRangeMake(0, CFStringGetLength(decoded));
    CFIndex needed = 0;
    CFStringGetBytes(decoded, range, kCFStringEncodingUTF8, 0, false, nullptr, 0, &needed);
    std::vector<UInt8> buffer(static_cast<std::size_t>(needed));
    CFIndex written = 0;
    CFStringGetBytes(decoded, range, kCFStringEncodingUTF8, 0, false,
        buffer.data(), needed, &written);
    CFRelease(decoded);
    return std::string(reinterpret_cast<const char*>(buffer.data()),
        static_cast<std::size_t>(written));

#else
    // The embedded Mac tables come first: they are exact, always present, and cover the
    // encodings iconv is least likely to have.
    if (const char16_t* const table = macTableFor(codePage)) {
        return convertWithTable(table, source);
    }
    const char* const fromEncoding = iconvNameFor(codePage);
    if (fromEncoding == nullptr) {
        return std::nullopt;
    }
    return convertWithIconv(fromEncoding, source);
#endif
}

} // namespace

CodePage codePageForCharset(Bank bank, int charsetVal)
{
    switch (bank) {
    case Bank::Windows:
        switch (charsetVal) {
        case windowsShiftJisCharset:    return CodePage::ShiftJis;
        case windowsHangeulCharset:     return CodePage::Korean;
        case windowsGb2312Charset:      return CodePage::Gb2312;
        case windowsBig5Charset:        return CodePage::Big5;
        case windowsGreekCharset:       return CodePage::Windows1253;
        case windowsTurkishCharset:     return CodePage::Windows1254;
        case windowsHebrewCharset:      return CodePage::Windows1255;
        case windowsArabicCharset:      return CodePage::Windows1256;
        case windowsBalticCharset:      return CodePage::Windows1257;
        case windowsCyrillicCharset:    return CodePage::Windows1251;
        case windowsThaiCharset:        return CodePage::Thai874;
        case windowsEastEuropeCharset:  return CodePage::Windows1250;
        case windowsVietnameseCharset:
            // Windows-1258 is the Vietnamese page, but no surveyed file uses this charset
            // and the enum does not carry a value that has never been needed. It falls to
            // the bank default until one appears.
        case windowsAnsiCharset:
        case windowsDefaultCharset:
        case windowsOemCharset:
        default:
            // ANSI is pinned to Windows-1252 rather than resolved against the running
            // machine's active code page, so a file decodes the same way everywhere.
            // DEFAULT, OEM, the symbol charset and any unlisted value name no script at
            // all, so they fall to the same Windows default. Which values mean "symbol" is
            // musxdom's to say, through FontDefinition::calcIsSymbolFont, and restating it
            // here would only create a second answer to drift from the first.
            return CodePage::Windows1252;
        }

    case Bank::MacOS:
        switch (charsetVal) {
        case macJapaneseScript:     return CodePage::MacJapanese;
        case macTradChineseScript:  return CodePage::MacTradChinese;
        case macKoreanScript:       return CodePage::MacKorean;
        case macArabicScript:       return CodePage::MacArabic;
        case macHebrewScript:       return CodePage::MacHebrew;
        case macGreekScript:        return CodePage::MacGreek;
        case macCyrillicScript:     return CodePage::MacCyrillic;
        case macThaiScript:         return CodePage::MacThai;
        case macSimpChineseScript:  return CodePage::MacSimpChinese;
        case macCentralEuroScript:  return CodePage::MacCentralEurope;
        case macTurkishScript:      return CodePage::MacTurkish;
        case macUninterpretedScript:
        case macRomanScript:
        default:
            // Mac Roman, not Windows-1252. The two agree below 0x80 and disagree above it,
            // so treating Roman as ANSI turns every accented character into mojibake:
            // `C\x8elino` is `Célino` in Mac Roman and `CŽlino` in Windows-1252. Finale's
            // Mac symbol marker lands here too: whatever a symbol font's *content* means,
            // its name is ordinary Mac text, so no symbol test is needed.
            return CodePage::MacRoman;
        }
    }

    return CodePage::MacRoman;
}

std::string toUtf8(const std::string& source, CodePage codePage)
{
    if (auto converted = convert(codePage, source)) {
        return *converted;
    }
    return source;
}

std::string normalizeLineBreaks(std::string source)
{
    std::string result;
    result.reserve(source.size());
    for (std::size_t at = 0; at < source.size(); ++at) {
        if (source[at] != '\r') {
            result.push_back(source[at]);
            continue;
        }
        result.push_back('\n');
        if (at + 1 < source.size() && source[at + 1] == '\n') {
            ++at;
        }
    }
    return result;
}

CodePage platformCodePage(SourcePlatform platform)
{
    using Bank = musx::dom::others::FontDefinition::CharacterSetBank;
    return codePageForCharset(
        platform == SourcePlatform::Windows ? Bank::Windows : Bank::MacOS, 0);
}

std::string symbolBytesToUtf8(std::string_view source)
{
    std::string out;
    out.reserve(source.size());
    for (const char raw : source) {
        const auto byte = static_cast<unsigned char>(raw);
        if (byte < 0x80) {
            out.push_back(static_cast<char>(byte));
        } else {
            // Every value from 0x80 to 0xff is two UTF-8 bytes, so no third case exists.
            out.push_back(static_cast<char>(0xc0U | (byte >> 6U)));
            out.push_back(static_cast<char>(0x80U | (byte & 0x3fU)));
        }
    }
    return out;
}

} // namespace text
} // namespace finale_mus_reader
