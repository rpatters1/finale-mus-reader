# SmartShapeCustomLine

**Covers:** The `ls` record, which the public PDK does not model, and its recovered layout.
**Read when:** Working on custom smart-shape line styles.
**Confidence:** strong; independently derived because the public PDK is silent.

## SmartShapeCustomLine (`ls`) is absent from the public PDK

Neither `EDTCustomLineDefinition` nor a two-character tag `ls`/`sl` occurs anywhere in the consulted PDK
sources above, checked across every header, not just `edata.h`. `others::SmartShapeCustomLine` is
therefore not `public-PDK-derived`; the tag and layout below are corpus-correlated instead.

**Confirmed.** Fixed-row tag `ls` (packed logical order, i.e. `records::packTag("ls")`) is the pre-zlib
identity of `others::SmartShapeCustomLine`; zlib class `0x00de` already appears in the tag table above.
Established by exact one-for-one correlation of source comparators against Finale 27 `ssLineStyle`
elements in two controlled private fixtures: an 11-line-style Finale 2000 file (`mus-baf04c0390a2fd12`)
and a 30-line-style Finale 2003 DCL file (`mus-302c33d4dffb58dc`), both in `rpatters1-main`, plus the
tracked `tests/evidence/F2000/F2000-baseline.mus` (cmpers 1-2). Every incidence family is exactly six
16-byte rows (36 words / 72 bytes), matching the 72-byte payload already recorded for class `0x00de`.

The record is absent, not merely unread, before Finale 2000. **Confirmed** by an exhaustive
content-deduplicated census of every filesystem-origin pre-2000 fixed-row/CodaBanner file across all
three registered surveys (`rpatters1-main`, `rpatters1-installs`, `tracked-evidence`): 393 distinct
files spanning `1.0.0`, `PC 1.0+`, `1.8.7`, `2.0.1`, `2.6`, `3.0`, `3.2`, `3.5`, `3.7`, `3.8`, `97`, and
`98` (Enigma major 3 and earlier) carry zero `ls` rows between them, including 25 Finale 1.0.0 files and
5 CodaBanner files that are tracked, public fixtures. (Two further files of unrecovered version were
also checked and are likewise absent, but are excluded from this count since their era is not itself
confirmed.) The same census finds `ls` in 139 of 143 distinct
Finale 2000 (Enigma major 5) files; the four exceptions are shipped install templates ("Encore Import
Defaults", two Church templates, one General template) that carry thousands of other `others` rows but
none under this tag, so a real Finale 2000 document can still lack the class entirely. The boundary is
Enigma major 5, matching `research/reference/VERSION_MATRIX.md`'s version table.

A further deduplicated sample (up to 25 distinct files per product) across 2001-2012 shows `ls`/class
`0x00de` in the large majority of files at every product, with a handful of absences at every product
from 2006 onward and a notably higher absence rate at 2012 (11/25 sampled). Spot-checking those Finale
2012 absences shows large, ordinary user documents with thousands of unrelated `others` rows and no
line-style record at all -- the class is populated only when a document actually uses a custom line
style, not synthesized into every source file the way Finale 27's own export synthesizes a baseline set
on upgrade, so an absence at any post-boundary product is an ordinary and expected outcome rather than a
gap in the reader.

The source-record importer relies on tag/class presence as its structural marker and creates only
source-owned line definitions. That absence cannot, however, distinguish a pre-capability document
from a later document that merely has no custom lines. `SmartShapeOptions` therefore applies the
confirmed capability boundary separately: the whole Coda-banner epoch predates the class, and only
within the uncompressed epoch does internal major version less than 5 select the pre-capability
fallback. DCL and zlib files are wholly later. An unclassified epoch does not take the fallback.

For a document before that gate, deferred reference resolution copies the pinned baseline's glissando,
tab-slide, and guitar-bend definitions, in that semantic order, and assigns their new comparators to
the corresponding option fields. Finale 2000 through Finale 2002 already store the first two but
predate the bend-curve tool, so those versions copy only the baseline guitar-bend definition. The
whole uncompressed epoch is before that second boundary; within the DCL epoch the cutoff is internal
major version 8 (Finale 2003). Zlib is wholly later, and an unknown epoch is not guessed across either
gate. The imports use musxdom's custom-line dependency importer, so any referenced shape, font, or raw
text receives the same on-demand treatment. In an otherwise empty pool the three definitions receive
comparators 1, 2, and 3; where the source already has lines, the bend curve receives the next free
comparator. The general custom-line option is not synthesized.

Physical word layout (0-indexed, pre-Finale-2012):

| word | field | notes |
|---:|---|---|
| 0 | raw `lineStyle` | `0`=Solid, `1`=Dashed, `2`=Char -- raw order differs from the modern enum |
| 1 | char: `lineChar`; solid/dashed: `lineWidth` (Efix) | union by `lineStyle` |
| 2 | dashed: `dashOn`; char: `font.fontId` | union |
| 3 | dashed: `dashOff`; char: `font.fontSize` | union |
| 4 | char: `font` Enigma style mask | bit meanings are musxdom's, through `FontInfo::setEnigmaStyles` |
| 5 | unrecovered | `-1` for Char and zero otherwise; meaning unconfirmed, not assigned |
| 6 | char: `baselineShiftEms` | |
| 7 | raw `lineCapStartType` | `0`=None, `1`=ArrowheadPreset, `2`=ArrowheadCustom, `3`=Hook -- again a raw order distinct from the modern enum |
| 8 | raw `lineCapEndType` | same raw mapping |
| 9 | `lineCapStartArrowId` or `lineCapStartHookLength` | shared slot, selected by word 7 |
| 10 | unrecovered | zero in every sample |
| 11 | `lineCapEndArrowId` or `lineCapEndHookLength` | shared slot, selected by word 8 |
| 12 | unrecovered | zero in every sample |
| 13 | flags | bit0 `makeHorz`, bit1 `lineAfterLeftStartText`, bit2 `lineBeforeRightEndText`, bit3 `lineAfterLeftContText` |
| 14-18 | `leftStartRawTextId`, `leftContRawTextId`, `rightEndRawTextId`, `centerFullRawTextId`, `centerAbbrRawTextId` | one word each, in that order |
| 19-28 | `leftStartX`, `leftStartY`, `leftContX`, `leftContY`, `rightEndX`, `rightEndY`, `centerFullX`, `centerFullY`, `centerAbbrX`, `centerAbbrY` | X and Y interleave per anchor, not grouped by axis |
| 29-33 | `lineStartX`, `lineStartY`, `lineEndX`, `lineEndY`, `lineContX` | musxdom's own declaration order |
| 34, 35 | unrecovered | zero in every sample, including one that fills every word through 33 |

**Confirmed, the anchor and line-adjustment offsets.** Words 19-33 were established by
`tests/evidence/F2000/F2000-ssline-offsets.mus`, a controlled fixture built for the purpose: one
Solid line style with all five text anchors defined and every position box set to a distinct
prime, so no two of the fifteen slots can be confused with one another. Its Finale 27 companion
names all fifteen and agrees word for word, including the one negative value.

The fixture also settles what the earlier samples could not distinguish about the vertical line
adjustment. Finale's dialog offers a single "V" control for the whole line and writes that one
value into **both** word 30 and word 32, which is what musxdom's note that Finale syncs
`lineStartY` with `lineEndY` describes. Entering 61 there produced 61 in both words while "Start
H", "End H" and "Cont H" produced three distinct values in words 29, 31 and 33.

**Confirmed, the Char font tuple.** Words 2, 3 and 4 of a Char record are the ordinary legacy font
tuple of id, size and Enigma style mask, the same triple the clef and font-options tables carry. Words
5, 10, 12, 34 and 35 correspond to no musxdom member; word 5 is the only one of them ever nonzero,
holding `-1` for a Char line and zero otherwise, and nothing establishes what it means.

**Confirmed, a stored character is a byte in its font's encoding.** `lineChar` is not a code point
before Finale 2012. It is a single byte in whatever encoding the font named by word 2 uses, exactly as
a run of legacy text is, and it is decoded by the same rule: a symbol font's byte is a glyph number and
is its own code point, and any other font's byte goes through the code page its charset fields name.
Font id 0 is the document's default music font and is a symbol font whatever its charset claims.
`F2000-ssline-offsets.mus` holds Mac Roman 199 in an Arial line and its companion reads 171, which is
`U+00AB`. From Finale 2012 the record stores the code point outright and there is nothing to decode;
the same document back-saved to that era holds 171 directly.

**Confirmed, and the rule is not particular to this class.** Every record that stores a bare
character stores it the same way and is decoded the same way: `ClefDef::clefChar`, the
stem-connection symbol, and `TextOptions`'s accidental symbol inserts.
`tests/evidence/F2002/F2002-clef-stem-font.mus` settles all three at once -- a clef given a font of
its own, a stem connection naming the same font, and the flat insert moved to it each store 199, and
the companion reads all three as 171.

Each carries its own control. The document's first stem connection and its four remaining symbol
inserts still name font 0, storing 192 and 35/110/186/220, and every one of those must read back
unchanged because a music font's byte is a glyph number -- decoding 192 through Mac Roman would name
an infinity sign. Every clef, connection and insert in every other fixture is of that second kind,
which is why no earlier document showed the difference.

Finale 2002 is the earliest available release whose clef dialog allows a font override, so the
fixture is a Finale 2002 document rather than a Finale 2000 one.

**Confirmed, Finale 2012 boundary.** The zlib class payload is the pre-2012 layout with the character
slot (word 1) widened to a 32-bit codepoint occupying words 1-2. The record stays 36 words because the
old final word is dropped. This is the same boundary already generalized in
`versions::firstUnicodeMajorVersion`/`versions::storesUnicodeCodepoints` (`legacy_mapping.h`) for the
clef and stem-connection tables, applied here as a new instance rather than a new rule.

**The shift is not uniform, and reading it as uniform is wrong.** The character belongs to the Char
parameter block, so only that block's own later fields move with it: for a Char record, old words 2 to 6
become new words 3 to 7. A Solid or Dashed record has no character and keeps its width and dash lengths
in words 1 to 3 exactly where every earlier layout put them. The block occupies one more word in every
record regardless, so the common part from word 7 onward moves for all three line styles (old word *n*
&rarr; new word *n*+1 for *n* &ge; 7).

Established by `tests/evidence/F2012/F2012-ssline-offsets.mus`, the back-save of the fixed-row fixture
below, which states all three parameter layouts in one document. A rule that shifted every word alike
reads its dashed line as a 7 EVPU dash with no gap; the source and its companion both say 3 and 7. The
earlier `F2012-baseline.mus` could not distinguish the two rules: its only non-default line style is
Solid with a cap, and every word that record uses lies in the common part.

Both controlled correlation fixtures also show the modern Finale 27 export carrying one more
`ssLineStyle` object than the legacy source stores physically (comparator 11 of 11 in the Finale 2000
file, comparator 3 of 3 in `F2000-baseline`): a further built-in default Finale's upgrade synthesizes
rather than something the reader failed to recover. The importer does not fabricate it.
