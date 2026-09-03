# Legacy text encoding

**Covers:** Per-font charset selection, the platform fallback, and the rule that a stored byte is decoded through its own font.
**Read when:** Touching any text, symbol, or character conversion.
**Confidence:** confirmed.

## Encoding

Text is encoded per font, not per document: the font in force at that point in the string
names the code page through its own `charsetBank` and `charsetVal`. Two cases are not a code
page at all and preserve the byte as the code point of the same value:

- a font whose character set says symbol, which musxdom's `calcIsSymbolFont` decides;
- font id 0, the default music font, whose bytes are glyph numbers whatever its record says.
  Finale 97 records Mac Roman for its own `Pmusic`, yet Finale 27 converts the same document's
  expression characters byte for byte and rewrites the font as a symbol font. The character
  set fields did not start carrying the symbol marker until the compressed eras.

**Believed, not confirmed:** the same rule extends to a nonzero definition with the same
normalized name as font 0.
`mus-bb73336c4c45f509` defines `Pmusic` at both 0 and 23 with Mac Roman character sets, and
four expression records explicitly select comparator 23 before bytes `0x82` or `0x8d`.
Finale 27 rewrites those runs under `Font0` and preserves the byte values, but that conversion
also had `Pmusic` in the machine's user-editable `MacSymbolFonts.txt`. The observation is
consistent with typeface identity deciding the runs; it cannot distinguish that rule from the
machine-local override.

The Finale 2011 source `mus-94547730f50a1e38` defines `FinaleAlphaNotes` with Mac
charset 0 and uses it as the Noteheads font. Both occurrences of the identical source upgrade
to companions that store the same definition as Mac symbol charset 4095, and
`FinaleAlphaNotes` is explicitly present in the supplied `MacSymbolFonts.txt`. The reader now
applies that configuration while importing font definitions: a matching definition retains
its bank and receives musxdom's symbol charset for that bank. The returned document therefore
carries the semantic adjustment, while `ImportReport` retains the stored charset as the raw
value and labels the result `LegacyMusAdjusted`. The name-based rule applies to both banks,
matching the prior decoding behavior; it does not reinterpret a Windows font as a Mac font.

Legacy line breaks are carriage returns and become line feeds: Finale 27 writes `\n` where
`F97-fileinfo-short.mus` has `\r`, and emits no `&#xD;` anywhere.

## Project rules for legacy text encoding

Moved from `AGENTS.md`; these are binding rules, not observations.

Legacy MUS stores text in whatever encoding the machine that saved it used; EnigmaXML and
musxdom are always UTF-8. Converting between the two is this project's job and not
musxdom's, which is why `src/import/support/text_encoding.*` exists here.

The encoding is named per font rather than per document: `charsetBank` selects the
platform's charset numbering and `charsetVal` selects within it, so a Mac font in a document
saved on Windows still decodes correctly. Before Finale 3.2 the font record carries no
charset at all, and the bank is synthesized from the document's own platform instead.

**Never re-encode pre-Finale-2012 text to Unicode without using that text's own font.** This
is the default position and it applies to a single stored character exactly as it applies to a
run of text: a clef character, a stem-connection symbol, a custom line style's character and a
text symbol insert are each a byte in the encoding of the font their own record names, not a
code point. Decoding one through the wrong encoding does not merely garble it -- it names a
different glyph, which is why a symbol font's byte must survive untouched rather than being read
as Mac Roman. `text::codePageForDocumentFont` answers the question once, including that font id
zero is the default music font whatever charset its record claims, and
`text::codepointFromByte` applies the answer to one character.

Where no font names an encoding, fall back to the platform default: Mac Roman on Mac and
Windows-1252 on Windows. That fallback is `text::platformCodePage` and belongs only to text
that genuinely has no font -- the File Info header strings, the name inside a font command,
literal text before any font command in a block, and the lyric punctuation string. Do not
restate it anywhere else.

**Aim for the best result obtainable on the machine that is running.** Conversion need not
be bit-identical across platforms, and insisting on that would mean giving up real accuracy:
Windows can name encodings iconv cannot, so it gets the more faithful code page rather than
being held to a common subset. Where a platform must fall back, say so next to the fallback
and record what evidence shows the fallback is adequate.

Choices that no observed file settles are starting positions, not findings. Label them as
such and revise them when a file demands it, rather than defending them.
