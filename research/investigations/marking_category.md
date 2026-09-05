# Marking category investigations

**Covers:** How the marking-category class identities, layout, name-width boundary, enum mapping,
flags, and pre-Finale-2009 fallback comparison were established.
**Read when:** Revisiting a category field, proposing fixed-row identities, or reviewing fallback
differences.
**Confidence:** `strong` within tracked evidence.

## 2026-09-04 — Class records, layout, and name width

**Method.** Class-record dumps were paired with Finale 27 companions. Finale 2009 source
`mus-76ef4d96d05bedff` supplied class `0x012d` category payloads and byte-string class `0x012e`
names. Finale 2012 source `mus-84761f30804afb36` supplied the same 36-byte numeric layout and
UTF-16 name payloads. Offsets were checked word by word against the companion category fields.

The complete alignment constant tables and the category API's field meanings were then checked
against public category-definition documentation
([public API reference](https://pdk.finalelua.com/class_f_c_category_def.html), accessed
2026-09-04). This is `public-PDK-derived`; the on-disk offsets and class IDs are independently
binary-verified.

**Result.** The identities, offsets, and translations are recorded in
[`../format/others/marking_category.md`](../format/others/marking_category.md). Direct enum casts
were disproved by the first coverage capture: stored horizontal values `13`, `1`, and `3` became
`LeftOfPrimaryNotehead`, `StartTimeSig`, and `Manual` in the companion, while stored vertical `2`
became `Manual`. Applying the published constant table removed all 216 source-owned alignment
differences.

The flag masks were checked against canned-category combinations and their companion leaves.
Custom-category source `mus-559cbb2d3a3ca516` additionally isolates `0x0080` on categories beyond
the canned range as `userCreated`.

## 2026-09-04 — Tracked-evidence coverage

**Selection funnel.** The instrumented Release probe read 225 of 225 `tracked-evidence` documents;
all 225 had readable companions. The source epochs were 77 Coda-banner, 49 uncompressed, 65 DCL,
and 34 zlib. No selected document failed before comparison.

**Source-owned result.** The 27 zlib documents saved by Finale 2011 or 2012 contribute 7,749
MarkingCategory leaves and 378 MarkingCategoryName leaves. After enum translation, all compare
exactly. Across source-owned and fallback categories together, 3,150 of 3,150 name leaves compare
exactly. This gives `strong` support within one survey; a public Finale 2009 fixture with a
non-ASCII name would make the byte-width boundary independently reproducible.

**Fallback result.** The same capture found 497 differing MarkingCategory leaves across 118
distinct Coda-banner or uncompressed documents, all with `finale27-default` origin. The
repeated population consists of baseline music-font id 0 on categories 4 and 5 versus concrete
companion font IDs, and baseline category 6 text at bold 14 points versus regular 12 points. One
document contributes most of the additional font substitutions and sizes; a second contributes
one additional music-font substitution. Names have no fallback difference.

These are not source-record decoder disagreements: the affected epochs contain no category
record, and the requested behavior deliberately imports the pinned baseline. User review approved
classifying fallback-origin font leaves as different defaults. A recapture classified all 497
(325 Coda-banner and 172 uncompressed) without classifying any non-font or source-owned leaf;
no marking-category or category-name difference remains unexpected in tracked evidence.

## 2026-09-04 — All-corpus coverage

**Selection funnel.** The instrumented Release probe selected 16,322 occurrences from all three
registered surveys, representing 7,286 distinct content IDs. It read 16,233 occurrences; 58 LIB
files and 31 inputs that did not identify as MUS failed before comparison. All 4,633 companion
occurrences were readable.

**Result.** The category population had 1,364,118 equal leaves and 7,824 expected differences:
7,766 pre-Finale-2009 baseline font leaves classified as different defaults, 44 source-owned
Finale 2009 beta leaves classified as beta discrepancies, and 14 missing-source-font leaves
classified as Finale upgrade loss. The upgrade-loss rule requires a positive source font ID with
no source-owned font definition and a concrete companion font identity; it does not depend on a
placeholder font name. No category leaf was unexpected or one-sided.

The category-name population had 66,924 equal leaves with no expected, unexpected, or one-sided
differences. With every persisted member accounted for and no unexpected difference in the final
all-corpus capture, both classes are complete.
