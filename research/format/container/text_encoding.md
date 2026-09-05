# Legacy text encoding

**Covers:** Per-font charset selection, the platform fallback, and the rule that a stored byte is decoded through its own font.
**Read when:** Touching any text, symbol, or character conversion.
**Confidence:** confirmed.

## Encoding

Text is encoded per font, not per document: the font in force at that point in the string
names the code page through its own `charsetBank` and `charsetVal`. A font whose character set
says symbol, which musxdom's `calcIsSymbolFont` decides, has no code page and preserves the byte
as the code point of the same value. `MacSymbolFonts.txt` is an external override for character
interpretation: Finale treats a listed face's characters as symbol values, but does not
necessarily change the persisted `FontDefinition` charset to match.

Comparator zero and definitions sharing its normalized name do not independently confer symbol
semantics. **Weak:** one Finale 2.6 source uses the unlisted custom face `PattersonSonata` at
comparator zero with the synthesized Mac Roman charset; Finale 27 converts its high bytes through
Mac Roman. The earlier apparent counterexample uses `Pmusic`, which was present in the converting
machine's `MacSymbolFonts.txt`, so that observation establishes the configured name override rather
than a comparator rule. See `research/investigations/music_symbol_options.md`.

The Finale 2011 source `mus-94547730f50a1e38` defines `FinaleAlphaNotes` with Mac
charset 0 and uses it as the Noteheads font. Both occurrences of the identical source upgrade
to companions that store the same definition as Mac symbol charset 4095, and
`FinaleAlphaNotes` is explicitly present in the supplied `MacSymbolFonts.txt`. The reader now
applies that configuration while importing font definitions: a matching definition retains
its bank and receives musxdom's symbol charset for that bank. The returned document therefore
carries the semantic adjustment, while `ImportReport` retains the stored charset as the raw
value and labels the result `LegacyMusAdjusted`. The name-based rule applies to both banks,
matching the prior decoding behavior; it does not reinterpret a Windows font as a Mac font.
**Weak:** a rebuilt companion for `mus-d155ea3bad6a0a6f` preserves the configured
`PattersonSonata` characters as symbol values while leaving its `FontDefinition` charset
unchanged. Thus the `FinaleAlphaNotes` rewrite is not a general conversion rule. When the reader's
self-describing symbol charset differs from such a companion's retained text charset, the
font-definition difference is conversion loss rather than a character-decoding disagreement.

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
as Mac Roman. `text::codePageForDocumentFont` answers the question once, and
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
