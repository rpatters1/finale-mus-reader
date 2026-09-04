# Chord suffix elements

**Covers:** Legacy recovery of `musx::dom::others::ChordSuffixElement`.
**Read when:** Changing `src/import/others/chord_suffix_elements.cpp` or interpreting `IV`/class `0x007d`.
**Confidence:** confirmed across all four physical epochs.

## Record family and identity

`ChordSuffixElement` is source-owned. Its cmper identifies one suffix definition and its zero-based
incidence identifies an element within that suffix. Through Finale 2006 the family uses special-other
selector `IV`; zlib files use class `0x007d` and coalesce the incidences into one payload. The evidence
and public-PDK provenance are in [the investigation](../../investigations/chord_suffix_elements.md).

## Layouts

Before Finale 2012, an element is six words: stored symbol, signed horizontal and vertical EVPU
offsets, a word with the font id in its low byte and point size in its high byte, Enigma font effects,
and flags. The symbol is one font-encoded byte even though its physical slot is a word; conversion
therefore uses the element's own font under the project-wide
[text-encoding rule](../container/text_encoding.md).

Finale 2012 widens the logical element to eight words: a high-word-first 32-bit symbol, the two signed
offsets, font id, point size, effects, and flags. Class `0x007d` stores that 16-byte logical value in a
24-byte stride whose final four words are zero fill. The version gate is confined to the zlib epoch;
all earlier epochs use the narrow layout.

The effects word is passed intact to `musx::dom::FontInfo::setEnigmaStyles`. Flag `0x0800` selects
numeric representation. The mutually exclusive prefix values are flat `0x0080`, sharp `0x0040`, plus
`0x0020`, and minus `0x0010`; no set prefix bit maps to `Prefix::None`.

## Coverage boundary

Every musxdom field is recovered: the symbol, offsets, numeric representation, prefix, and every leaf
of the contained `FontInfo`. The controlled Finale 1.0 fixture confirms the same `IV` selector,
six-word layout, comparator and incidence identity, packed font word, and numeric/flat flag bits in
the Coda-banner epoch.
