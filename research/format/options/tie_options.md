# TieOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Tie options

**Partial.** `TieOptions` recovers its located scalar fields and both fixed collections when
their families are present in the uncompressed, DCL, and zlib epochs. The fixed-row
representation stores 24 scalar words on `^84(65534)`, twelve `(x, y)` connection pairs on
`^85(65534)`, and four seven-word control styles on `^86(65534)`. Each control style is
`span`, followed by each control point's `insetRatio`, `height`, and `insetFixed`. The zlib
representation coalesces those same streams into numeric classes `0x0062`, `0x0063`, and
`0x0064`. `^97(65534)` (class `0x006f`) supplies `tieTipWidth` as ten-thousandths of an
EVPU. The separately stored `frontTieSepar` is selector 41 incidence 2 word 3 in the
DCL-era fixed rows and flattened word 15 of numeric class `0x0037` in zlib.

Before selector 84 appears, the shared curve records retain a smaller tie representation.
Negated selector 51 words 1 and 3 supply `thicknessLeft` and `thicknessRight`. Selector 22
word 4 is the base contour height; selector 51 words 0 and 2 adjust its left and right values.
Medium, long, and tie-end contours use those two sums directly, while the short contour uses
integer half-height. Nonzero selector 50 words 0 and 2 supply the start and end horizontal
positions used by both inner ties and outer-stem ties. Zero in those two placement positions
selects the era's built-in value, so the importer leaves the pinned horizontal position.
Selector 50 words 1 and 3 add to selector 17 word 4 for the over-inner start and end Y
positions; negating those sums supplies the under-inner positions. For the short, medium, and
long contours, the first fixed inset is selector 50 word 4 minus word 0 and the second is word 2
minus word 5. The corresponding tie-end fixed-inset conversion remains open.

The scattered representation continues into Finale 3.7 when selector 84 is absent, with the same
connection, height, thickness, and non-tie-end fixed-inset formulas. Selector 22 word 5 stores the
third legacy Position from Notehead value, but its modern target or upgrade disposition remains
open.

Selector 22 word 5 is the early dialog's single inset value, but its modern transform remains
open. Controlled values 7 and -12 do not by themselves identify a modern target. The separately
stored PostScript positions determine the modern short-, medium-, and long-contour fixed insets,
while the tie-end inset conversion remains unresolved. In the two controlled Finale 1.0 files,
selector 17 word 4 follows the base height at integer one-third, but that relationship does not
hold in Finale 2.6 and is not needed to construct the modern contours. The remaining early
vertical-placement and tie-end inset conversions are not yet established.

The obsolete Tend XDisp and Tend YDisp controls are selector 51 word 4 and selector 35 word 5,
respectively. Controlled single-edit Finale 1.0 files and their ETFs establish both locations,
but Finale 27 discards both values and leaves its complete `TieOptions` object unchanged. They
therefore have no musxdom target. Selector 51 word 5 is a different early control whose meaning
and modern transformation remain open.

The field meanings on selector 84 and the tip-width conversion began as
`private-framework-derived`. The controlled Finale 2000 thickness and fixed-inset edits
independently confirm the relevant scalar and contour positions. Finale 97, 2000, 2005,
2008, and 2012 controlled files confirm the collection shapes and the zlib numeric-class
relationship in both byte orders. The published PDK API independently documents the
four contour types, six placement groups, units, and scalar semantics, but its wrapper is
not treated as the disk layout: [FCTiePrefs](https://pdk.finalelua.com/class_f_c_tie_prefs.html),
[FCTieContourPrefs](https://pdk.finalelua.com/class_f_c_tie_contour_prefs.html), and
[FCTiePlacementPrefs](https://pdk.finalelua.com/class_f_c_tie_placement_prefs.html), accessed
2026-09-02.

The Coda-banner epoch is partially covered through the early shared curve records. Its controlled
documents do not carry selectors 84 through 86, so all other scalars and the vertical and contour
members retain pinned values. The same scattered layout continues through the observed Finale 3.7
documents and is selected structurally by the absence of selector 84. A companion disagreement on
an unmapped `Finale27Default` leaf is classified as `different_defaults` when the companions do not
all agree; it is not evidence for synthesizing a value when the early UI supplies no corresponding
control and the converted value varies.

Invariant companion values that disagree with the baseline are applied as `LegacyBehavior`.
Across both scattered eras these are `specialPosMode::None`, zero `sysBreakLeftHAdj`, 512 for all
eight control-point inset ratios, and zero `frontTieSepar`. The observed pre-selector-84
uncompressed companions additionally agree on disabled time- and key-signature breaks, disabled
outer placement, `secondsPlacement::None`, `chordTieDirType::OutsideInside`, disabled opposing
seconds and single-note accidental handling, `mixedStemDirection::OppositeFirst`, enabled
tie-end control style, and 48-EVPU short, medium, and long spans. Other unmapped leaves remain at
the pinned Finale 27 values and any companion differences are `different_defaults`.

Selectors 84 through 86 are absent from all 24 tracked Finale 3.7 documents and first appear in
the tracked cohort at Finale 3.8; their introduction boundary is therefore **strong**, not
confirmed. Selector 41 incidence 2 is absent throughout the observed Coda and uncompressed
records and present throughout DCL; the implementation consequently uses the record's presence
as its structural gate. A layout without flattened word 15 uses zero separation as
`LegacyBehavior`; a layout carrying the word recovers it from the source.
That introduction boundary is **strong**, with its exact release still open. In the current
tracked-evidence capture, all 24 pre-selector-84 Finale 3.7 documents and all 73 Coda documents
have no unexpected `TieOptions` differences. Five Finale 2.6 occurrences have both decoded
thicknesses at 6 while their upgraded companions contain zero. A differing source-derived Coda
thickness is classified as Finale upgrade loss: the source value is retained instead of accepting
the companion's discarded value. This classification is not value- or version-specific.
Controlled Finale 1.0 evidence upgraded through Finale 2.6 independently preserves distinct
nondefault thicknesses 9 and 11 in both the reader and companion through the same selector-51
mapping. The all-corpus capture contains one Finale 2006 DCL conversion anomaly: a source-derived
`mixedStemDirection` 2 becomes 0 in the companion. That exact release, leaf, source origin, and
2-to-0 disagreement is classified as Finale upgrade loss.
