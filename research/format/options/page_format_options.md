# PageFormatOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Page format options

**Implemented.** `PageFormatOptions` contains score and parts page formats with 25 persisted
leaves apiece, plus the outer `adjustPageScope` and `avoidSystemMarginCollisions` fields. The
adjustment scope is document-editor UI state rather than a page-format setting and intentionally
retains `Finale27Default`; collision avoidance is recovered or supplied from established legacy
behavior in every represented epoch.

The public `FCPageFormatPrefs` documentation defines separate `LoadScore` and `LoadParts`
instances and confirms the dimensions, percentages, EVPU/EVPU16 units, margins, first-page
switches, and sign conventions used by the destination class. This is
**public-PDK-derived** semantic evidence from
[`class_f_c_page_format_prefs.html`](https://github.com/finale-lua/pdk-framework-docs/blob/c3c5ebf0335432812b286e79c9757ad023eb48f1/html/class_f_c_page_format_prefs.html)
(accessed 2026-09-01). An independent format reference supplied the initial score and
parts selector tables. Those locations remain **reference-derived** except where the
controlled and tracked evidence below independently corroborates them.

The later fixed-row score layout uses selectors `14` and `15` for high-word-first page
dimensions, `39` for page scaling, `76` and `93` for system scaling and staff height, `16`
and `02` for left and right page margins, `17` and `01` for system geometry, `02` and `03`
for first-page/system geometry, and `13` for the two different-first switches. Selector `77`
stores the parts format, while its system scaling and staff height remain at selector `76`
word 4 and selector `93` word 4. System scaling begins with Finale 2002: controlled Finale
2001 and tracked Finale 2000 documents contain zero placeholders in selector `76` words 3 and
4, while Finale 2002 contains the 100-percent values. Earlier sources therefore retain the
pinned Finale 27 system percentages, which are 100 on both platforms, and report
`Finale27Default`. In the DCL layout, selector `77` stores the parts system
distance at word 17 and the four first-page/system values and switches at words 19–24. The
zlib layout preserves those logical selectors as classes `selector + 0x000e` and coalesces
selector `77` into one payload. The two parts dimensions follow the source byte order in both
fixed-row and zlib layouts; the remaining fields remain ordinary signed words. Collision avoidance
uses `FI(10)` word 2: its scalar bit 0 before the expanded Finale 3.5 layout, and bit 15 in that
layout, the DCL epoch, and the corresponding class `0x008d` payload in zlib. Controlled synthetic tests exercise every
leaf in fixed rows and class records in both byte orders. The tracked F2002 and F2008 empty
fixtures independently compare all 50 contained values exactly with their Finale 27 companions.

**Confirmed in Finale 1.0.0:** the UI's single page-format set becomes both modern contained
objects. Selectors `14` and `15` and selector `39` word 2 supply dimensions and page scaling.
Selector `16` supplies
both page-side margin sets, `IU` comparator 0 supplies the system top, selector `17` supplies
the other system geometry, and selector `01` word 5 supplies first-system distance. The
first-page top copies the left-page top, the first-system left copies the system left, and the
first-system top adds selector `17` word 0 to the stored `IU` top. `F100-pageformat` recovers
all edited values exactly, including dimensions 3167 by 2449, 91 percent page scaling, and
first-system top -73. Finale 1.0.0 predates Avoid Margin Collisions and
therefore supplies false as `LegacyBehavior`.

Finale 3.5 has the expanded score page-format dialog, including facing pages, First Page Drop,
and First Staff System Drop and Indent, but no independent parts settings. The earlier records
already preserve realized first-page and first-system geometry, so those values remain recoverable
before the controls became user-facing. Before Finale 3.5, the recovered left-page margins also
supply all four right-page margins, and the recovered ordinary system-left margin supplies the
first-system left margin. The resulting score format is then copied in full to parts. These derived
values report `LegacyBehavior`. Finale's Windows 2002 Read Me lists
“Absolute Staff Sizing” among the new features of Finale 2002 and describes direct control of the
resulting staff height; the 2001d Read Me discusses system resizing and Page Format changes but
does not contain that feature. The controlled Finale 2001 UI likewise has no system-percentage
option, and its stored zeros at the later selector locations are placeholders rather than
percentages. The system-percentage and absolute-staff-height option boundary is therefore Finale
2002. Earlier selector
values are readable but do not represent musxdom's absolute `rawStaffHeight` preference, so the
importer supplies the fixed 96-EVPU height (1536 sixteenth-EVPU units) as `LegacyBehavior` before
that release; the pinned Finale 27 baseline is 1312 and does not represent the earlier behavior.
Two companion-backed Finale 3.5 documents omit selector `77`;
the importer copies the recovered score format to parts and all 52 class leaves compare equal. The
copies report `LegacyBehavior`. Finale 3.7 introduces independent parts settings and selector `77`,
so record presence is the direct layout marker and also guards the later score first-page-margin
switch. This works for unversioned sources; a damaged later document that loses selector `77` is
indistinguishable and receives the single-set behavior.

Before Finale 3.5, `IU(0)`/`Iu(0)` word 2 is a signed 16-bit score system-top margin and the
first-system top is that value plus selector `17` word 0. In the expanded Finale 3.5 layout,
words 4--5 instead form one signed 32-bit system-top margin in the source platform's word order,
and selector `03` word 0 supplies the additional first-system offset. Selector `75` presence
continues to identify the expanded layout structurally. Across the 17 affected Finale 3.0/3.2
documents, word 2 exactly reproduces every companion system top, including the nonstandard values
-220, -251, -71, and -101. Big-endian Finale 3.5, 97, and 2000 sources place the high word first;
five little-endian Finale 98 sources place it second. The DCL boundary ends this representation:
Finale 2001 and later fixed-row sources recover the signed 16-bit value from selector `17` word 0,
and zlib sources retain that logical location in class `0x001f`. **Strong.**

When present in the uncompressed layout, selector `77` stores the independent parts format. Its
system distance is word 13 and its facing-pages switch is word 17, while words 19 and 20 are first-system top and left offsets
rather than the absolute values introduced by the DCL layout. The parts first-system top is
therefore the parts system top plus distance plus word 19, and the first-system left is its
system left plus word 20. Its raw staff height and system top follow the score, and its
first-page top follows its own left-page top. For the score, selector `39` word 3 still uses
the earlier staff-height units, selector `17` word 0 is the system distance, selector `03`
holds the expanded layout's first-system offsets, and the current-system row supplies the system
top. Both observed current-system tag spellings, `IU` and `Iu`, follow the same versioned word
layout. These transformations are **strong**: all 43 previously tracked uncompressed sources
reproduce their companions, including distinct Finale 3.7, 97, 98, and 2000 values.

A controlled Finale 3.7.2 parts-facing-pages edit changes selector `77` word 17 from 0 to 1;
word 18 remains zero. Finale 2000 preserves the complete selector payload unchanged when it
resaves that document, and both Finale 27 companions retain facing pages for parts while leaving
the score switch off. The uncompressed location is therefore word 17, distinct from word 18 in
the DCL and zlib layouts. **Confirmed.**

Avoid Margin Collisions exists by Finale 2.6.3. Its Coda storage is `fi`, comparator 65534,
incidence 51, word 5 bit 0; the incidence is absent in Finale 1.0.0, making presence the capability
marker. Controlled Finale 2.6.3 and Finale 3.7.2 edits each clear their era's stored flag from one
to zero and produce companions with collision avoidance disabled. In the early uncompressed layout,
`FI(10)` word 2 retains the scalar representation: four Finale 3.0/3.2 sources store 1 and upgrade
to true, while four contrasting sources store 0 and upgrade to false. Reading the later bit 15
therefore loses every stored 1. Selector `75` selects the later packed representation. **Strong.**

The refreshed tracked-evidence comparison has 211 source occurrences and companions, representing
209 distinct `corpus_id` values: 73 Coda-banner, 48 uncompressed, 59 DCL, and 31 zlib. All 10,972
`PageFormatOptions` leaves compare equal, with no unexpected differences or pre-comparison
failures. A companion disagreement on `adjustPageScope` is classified as `different_defaults`
only while the source retains `Finale27Default`; recovered page-format values remain strict.

The subsequent all-corpus comparison exposed the fixed-row parts dimensions' platform-dependent
word order. A Windows Finale 2001 source (`mus-46c4619dfdc99ae6`) stores height 3168 and width
2448 as word pairs `[3168, 0]` and `[2448, 0]`; the tracked Mac Finale 2002 fixture stores the
same values as `[0, 3168]` and `[0, 2448]`. Selecting the long-word order from the document byte
order corrects 572 unexpected leaves across 286 occurrences representing 145 distinct contents.

An ad hoc recapture selected the 418 occurrences with any PageFormatOptions difference in that
earlier all-corpus snapshot, representing 277 distinct contents. The dimension correction and
pre-3.5 shared-margin behavior remove 718 of the earlier 1,008 unexpected leaves: 572 dimensions
and 146 derived right-page/first-system-left values. All 418 sources and companions imported. The
remaining 290 unexpected leaves occur in 132 distinct contents and concern staff height,
first-page top, system top and derived first-system top, collision avoidance, and one facing-pages
value. Collision avoidance remains a separate investigation.

Repeating that same 418-occurrence cohort after gating absolute staff height at Finale 2002 removes
all 196 score/parts `rawStaffHeight` differences without introducing another difference. Copying
each uncompressed parts first-page top from its recovered page top removes another 21 leaves.
Selecting the current-system word and first-system formula from the structural Finale 3.5 marker
then removes all 68 system-top and derived first-system-top leaves. All 418 sources and companions
again import successfully. Five unexpected `PageFormatOptions` leaves remain across five distinct
contents: four collision-avoidance flags and one parts facing-pages value.

Reading the scalar pre-expanded collision flag and selector `77` word 17 for uncompressed parts
facing pages removes those final five leaves. The refreshed 418-occurrence cohort, still
representing 277 distinct contents, has 21,720 equal `PageFormatOptions` leaves, 16 expected
`adjustPageScope` default differences, and no unexpected differences or failed imports. A fresh
all-corpus capture selected 16,308 occurrences representing 7,272 distinct contents; 16,219
sources imported, all 4,619 available companions imported, and the 89 pre-comparison failures
were 58 Finale library files plus 31 inputs that are not Finale MUS documents. Across those
companions, `PageFormatOptions` has 239,770 equal leaves, 418 expected `adjustPageScope` default
differences, and no unexpected differences.

After new Finale 98 Windows companions were added, the next all-corpus capture contained 250,392
equal `PageFormatOptions` leaves, 436 expected default differences, and 20 unexpected leaves. All
20 are the score system top and the three values derived or copied from it in each of five
little-endian sources. Their raw word pairs are the little-endian reversal of the already observed
big-endian signed long; treating the pair as a single platform-ordered value accounts for every
one. A broad recapture after that correction remains pending. **Strong.**
