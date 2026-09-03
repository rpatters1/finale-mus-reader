# Graphic assignments

**Covers:** Embedded graphics, PageGraphicAssign, ShapeGraphicAssign, MeasureGraphicAssign, and the earliest controlled graphic placement.
**Read when:** Working on graphic placement or embedded image records.
**Confidence:** strong.

## Embedded graphics

**Confirmed across controlled Finale 2006 evidence, `rpatters1-main`, and
`rpatters1-installs`.** From Finale 2006 onward a stored `0x0013` block holds embedded graphics.
The three controlled Finale 2006 documents carry one EPS and four TIFF files; twenty-seven distinct zlib-era
corpus documents carry 66 EPS or PNG files. Each item is exactly:

`16-bit type, 32-bit byte length, raw graphic bytes, 32-bit footer version 1, 8-bit footer value`

Every numeric field follows the container byte order. Walking `6 + length + 5` bytes per item
consumes every observed block exactly, including blocks holding as many as nine graphics. The
final footer byte varies and remains **open**; it is not needed for delimiting or identifying the
raw file. Binary EPSF, `%!PS-Adobe` EPS, TIFF, and PNG signatures select the extension without
depending on the nested type.

The nested items carry no comparator. Their one-based encounter order is the graphic comparator:
for every adjacent-exact Finale 27 companion, item sizes in encounter order exactly match
`graphics/1.<extension>`, `graphics/2.<extension>`, and so on, even where ZIP member enumeration
is shuffled. The reader supplies this map to musxdom before document construction finishes.

No stored embedded payload was found in the Coda-banner or uncompressed epochs, nor in the
uncontrolled DCL corpus. Controlled Finale 2006 linked and embedded saves prove that embedding
begins inside the DCL epoch: the linked file's `0x0013` block is empty, while otherwise comparable
embedded files carry one or two nested EPS/TIFF items. A controlled measure-assigned EPS followed
by a page-assigned TIFF becomes comparators 1 and 2, confirming that encounter order is independent
of assignment type. Their ETF exports retain the score structures
but cannot carry the binary attachments. Finale 2005 and earlier remain uncovered for embedded
payloads because the application did not offer the feature.

The controlled Finale 2012 `F2012-graphics-types` fixture confirms embedded GIF, JPEG, TIFF, and
PDF payloads in the zlib epoch. Its six stored items correspond one-for-one with six assignment
occurrences: repeated use of the same GIF and JPEG produces distinct, byte-identical items and
comparators, while the singly used TIFF and PDF each produce one. Measure assignments resolve
comparators 1 and 6, the page assignment resolves comparator 2, and the three ShapeDef graphic
assignments resolve comparators 3, 4, and 5. Finale 27 preserves the same order and duplication.

## Page graphic assignments

**Confirmed across `rpatters1-main` and `rpatters1-installs` for the uncompressed, DCL, and zlib
epochs; structurally supported but corpus-unverified for Coda-banner.** Page graphics use tag `pg`
through Finale 2006 and class `0x00bc` in the zlib era. All 56 observed assignments in 26 documents
are exact 18-word tuples: three six-word fixed rows per assignment, or successive 36-byte tuples
inside a zlib class payload. The assignment comparator remains the DOM cmper and tuple order is its
zero-based incidence.

Words 0-5 are `version`, `left`, `bottom`, `width`, `height`, and `fDescId`; word 6 has no mapped
musxdom leaf. Word 7 is a packed display-flags word: page selection uses one-hot values
`0x0001/0x0002/0x0004/0x0008` for one/all/odd/even, and `0x0010` independently marks the graphic
hidden. Words 8-17 are packed left/all-page positioning, `startPage`, `endPage`, `savedRecord`,
`origWidth`, `origHeight`, `rightPgLeft`, `rightPgBottom`, packed right-page positioning, and
`graphicCmper`. The positioning word uses one-hot bits: horizontal left/right/center are
`0x01/0x02/0x04`, vertical top/bottom/center are `0x08/0x10/0x20`, margins/page-edge are
`0x40/0x80`, and preserve-aspect is `0x100`.

For a partially shared part assignment, the physical part snapshot's `startPage` and `endPage`
may differ from the score when the score and part have different numbers of leading blank pages.
The continuation masks for the observed assignments leave those fields linked, so the effective
assignment uses the score range. The comparator, rather than those stored snapshot words, selects
the assignment's page.

Measure-attached graphics use that same field order and packed positioning word through word 17.
An other row has six payload words, so an 18-word page assignment occupies exactly three rows. A
detail row has five payload words after its second comparator, so the same assignment occupies four
rows and its last two slots are zero filler. Zlib class `0x041d` retains that padded 20-word stride.
The page-only words remain present but have no measure-placement meaning. Controlled
Finale 3.7.2, 2006, and 2012 assignments independently agree that word 8 supplies `hAlign`,
`vAlign`, `posFrom`, and `fixedPerc` with the page-assignment masks above. Their Finale 27
companions preserve those four values. Thus the common 18-word prefix is **confirmed** across the
uncompressed, DCL, and zlib epochs; a Coda-banner assignment remains corpus-unverified.

## Earliest controlled graphic placement

**Introduced in Finale 3.7; the assignment layout is confirmed in Finale 3.7.2.** The controlled
`F372-measure-graphic` fixture places a linked EPS on staff 1 at measure 3. Its MUS and ETF both
carry four `mg(1,3)` rows containing the 18-word assignment followed by two zero filler words,
including file-description cmper 1 and `graphicCmper` zero. No stored graphic block exists, as expected for
a linked file. The user who produced the fixture observed the Graphics Tool in Finale 3.7.2's Tool
menu and observed it absent in Finale 2.6.3 and earlier. The Finale 3.7 addendum independently
identifies the Graphics Tool and Place/Export Graphics as new features, with EPS, PICT, and TIFF
explicitly supported. It does not mention embedding; that feature remains at the observed Finale
2006 boundary.

The controlled `F372-page-graphic` fixture independently establishes `pg` at the same boundary:
its three rows
contain the standard 18-word page assignment and refer to the linked `Photo_tiff.tiff`. Finale 27
preserves both the assignment and path in `score.dat`, but does not put the TIFF in the MUSX ZIP.

The corresponding `0x001d` block is non-empty in 208 zlib files. The reader does not reach it,
because the walk stops at the first terminal marker, so those bytes are counted as trailing.
Its payload does not begin with an image signature and its content is **open**.
