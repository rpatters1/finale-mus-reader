// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/mappings/tables.h"

#include <iterator>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace mapping {
namespace {

using Target = musx::dom::others::FontDefinition;

// A font definition is one `FN` family. Its first incidence is the header and every
// incidence after it carries the name.
//
// The header's first word packs the character set: the high nibble names the bank, 1 for a
// Mac font and 2 for a Windows font, and the remaining twelve bits are the character set
// value. This says where the *font* came from, not where the document was written: a Mac
// font can appear in a document saved on Windows.
//
// The values agree with musxdom's own documentation of the field, which records 0xfff as
// the macOS symbol character set and 2 as the Windows one. The controlled Finale 2002
// fixture stores 0x1fff for Maestro, a Mac symbol font, and 0x1000 for Times, a Mac text
// font, and the matching ETF prints those same headers as 8191 and 4096.
//
// The header's second word carries the pitch in its low nibble and the family in its high
// nibble. Across 7,622 header incidences in the corpus it is non-zero only for Windows-bank
// fonts: every one of the 7,355 Mac-bank fonts stores zero, while Windows-bank fonts take
// pitch values 1, 2, and 7 and family values 0, 1, 2, 4, and 5. That is consistent with the
// field describing a Windows font-selection attribute that has no Mac equivalent.
//
// The remaining four words are unused. Not one of them is non-zero anywhere in the corpus.
constexpr std::uint8_t charsetBankBit = 13;
constexpr std::uint8_t charsetValueBits = 12;
constexpr std::uint8_t nibbleBits = 4;

const FieldMapping fontFields[] = {
    // Bit 13 distinguishes the two banks: the nibble holds 1 for Mac and 2 for Windows, so
    // only a Windows font sets it. An explicit conversion keeps the enum honest rather than
    // relying on the legacy encoding happening to match musxdom's enumerators.
    MUS_BITS_AS(Target, "FN", CMPER_FROM_TARGET, /*incidence*/ 0, /*slot*/ 0,
        charsetBankBit, 1, charsetBank,
        value != 0 ? Target::CharacterSetBank::Windows : Target::CharacterSetBank::MacOS),
    MUS_BITS(Target, "FN", CMPER_FROM_TARGET, /*incidence*/ 0, /*slot*/ 0,
        /*firstBit*/ 0, charsetValueBits, charsetVal),
    MUS_BITS(Target, "FN", CMPER_FROM_TARGET, /*incidence*/ 0, /*slot*/ 1,
        /*firstBit*/ 0, nibbleBits, pitch),
    MUS_BITS(Target, "FN", CMPER_FROM_TARGET, /*incidence*/ 0, /*slot*/ 1,
        /*firstBit*/ nibbleBits, nibbleBits, family),
    // The name runs from the second incidence to the end of the family, so a long name
    // simply occupies more rows. It is read as bytes because character payloads are not
    // byte-order sensitive, and it ends at the first NUL.
    MUS_TEXT(Target, "FN", CMPER_FROM_TARGET, /*firstIncidence*/ 1, name),
};

// Before Finale 3.2 there is no header incidence at all: the family opens with the name.
// Observed in every Coda-banner file and in the Finale 3.0 files, against Finale 3.2 and
// later where the header is always present. The character set is therefore unavailable for
// these documents rather than merely unmapped, so this table carries only the name and
// leaves the bank and value at their constructed defaults.
//
// The exact boundary is unverified. Every corpus file without the header is either
// Coda-banner or Finale 3.0, and every file with one is Finale 3.2 or later, but the corpus
// holds no Finale 3.1 and its only 3.0 files are Windows-origin, so a platform explanation
// cannot be ruled out from this evidence alone.
constexpr std::uint8_t headerFirstVersion = 3;
constexpr std::uint8_t headerFirstMinor = 2;

const FieldMapping earlyFontFields[] = {
    MUS_TEXT(Target, "FN", CMPER_FROM_TARGET, /*firstIncidence*/ 0, name),
};

// From Finale 2007 the record is class-identified and length-governed rather than a fixed
// row, so the same fields are addressed by byte offset inside one record's payload. The
// character set encoding survived the change unchanged, which is why the bit positions below
// match the earlier table exactly; only the addressing differs.
//
// Verified against Finale 27's own conversion of the same document, which agrees on every
// comparator, every gap in the comparator sequence, every name, and every character set.
constexpr records::LegacyTag fontDefinitionClass = 0x0090;
constexpr std::uint8_t charsetOffset = 0;
constexpr std::uint8_t pitchFamilyOffset = 2;
constexpr std::uint8_t nameOffset = 12;

const FieldMapping classFontFields[] = {
    MUS_CLASS_BITS_AS(Target, fontDefinitionClass, charsetOffset, charsetBankBit, 1, charsetBank,
        value != 0 ? Target::CharacterSetBank::Windows : Target::CharacterSetBank::MacOS),
    MUS_CLASS_BITS(Target, fontDefinitionClass, charsetOffset, 0, charsetValueBits, charsetVal),
    MUS_CLASS_BITS(Target, fontDefinitionClass, pitchFamilyOffset, 0, nibbleBits, pitch),
    MUS_CLASS_BITS(Target, fontDefinitionClass, pitchFamilyOffset, nibbleBits, nibbleBits, family),
    // The payload is length-governed, so the name simply runs to its end. A longer name grows
    // the record rather than spilling into another incidence.
    MUS_CLASS_TEXT(Target, fontDefinitionClass, nameOffset, name),
};

} // namespace

const MappingTable& classFontDefinitionsTable()
{
    static const MappingTable table{
        .reportPrefix = "others.fontName",
        .epochs = EpochMask::Zlib,
        .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OthersFromRecords,
        .recordIdentity = fontDefinitionClass,
        .createTarget = &createOthersTarget<Target>,
        .fields = classFontFields,
        .fieldCount = std::size(classFontFields)};
    return table;
}

const MappingTable& fontDefinitionsTable()
{
    static const MappingTable table{
        .reportPrefix = "others.fontName",
        .epochs = EpochMask::CodaBanner | EpochMask::FixedRow,
        .versions = versions::from(headerFirstVersion, headerFirstMinor),
        .targetKind = TargetKind::OthersFromRecords,
        .recordIdentity = records::packTag("FN"),
        .createTarget = &createOthersTarget<Target>,
        .fields = fontFields,
        .fieldCount = std::size(fontFields)};
    return table;
}

const MappingTable& earlyFontDefinitionsTable()
{
    static const MappingTable table{
        .reportPrefix = "others.fontName",
        .epochs = EpochMask::CodaBanner | EpochMask::FixedRow,
        .versions = versions::upTo(headerFirstVersion, headerFirstMinor - 1),
        .targetKind = TargetKind::OthersFromRecords,
        .recordIdentity = records::packTag("FN"),
        .createTarget = &createOthersTarget<Target>,
        .fields = earlyFontFields,
        .fieldCount = std::size(earlyFontFields)};
    return table;
}

} // namespace mapping
} // namespace finale_mus_reader
