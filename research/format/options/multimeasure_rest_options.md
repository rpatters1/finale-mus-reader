# MultimeasureRestOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Multimeasure rest defaults

**Confirmed for both physical layouts and for every field of the class.** The multimeasure-rest
defaults are the ordinary field-map kind of numeric global rather than a direct block: selector `25`,
comparator `65534`, and from Finale 2007 the class id `numericGlobalClass` derives, `25 + 0x0e = 0x0027`.
The automatic-update flag is the one field kept elsewhere, on selector `83` (class `0x0061`). Counts below
are distinct files of the reference corpus.

**Finale 3.5 rewrote the record**, and that boundary sits *inside* the uncompressed epoch, which is what makes
this a case for a structural marker rather than either kind of gate. The family's own size states which layout
a file uses:

| Era | Family | Files |
|---|---|---:|
| Finale 1.0.0–3.2 | selector `25`, **1 incidence**, 6 words | 264 (229 Coda-banner, 35 uncompressed), plus 19 Finale 1.0.0 fixtures |
| Finale 3.5–2006 | selector `25`, **2 incidences**, 12 words | 2,069 (767 uncompressed, 1,302 DCL) |
| Finale 2007–2012 | class `0x0027`, 24 bytes = the same 12 words coalesced | 1,389 |

No file in any survey carries any other word count, and none crosses the line: every 1.0.0, 1.8.7, 2.0.1, 2.6,
3.0 and 3.2 document is on the short side and every 3.5-and-later document on the long one. The files with no
selector `25` at all are containers the reader cannot classify to an epoch either.

The printed Finale 3.5 addendum independently describes enhanced multimeasure rests, symbol
representation, and a separate options dialog. It confirms the semantic and release boundary;
the record width remains authoritative for decoding.

All three surveys were run, and the two beyond the reference corpus are what make the boundary a measurement
rather than an interpolation:

| Survey | What it adds here |
|---|---|
| `rpatters1-main` | 3,725 documents, 1,130 later-layout and 59 early-layout companion comparisons |
| `tracked-evidence` | the only companion-backed Finale 1.0.0 documents anywhere: 19 fixtures, all six-word |
| `rpatters1-installs` | 12,116 documents, and the only 3.8, 98, 2011 and Coda-era-Windows material in any survey |

The installs survey settles three releases the reference corpus does not contain at all. **Finale 3.8 (11
documents) and Finale 98 (43) are both on the later side**, as are all 1,295 Finale 2011 documents; its 22
Finale 1.0.0 documents are on the early side, agreeing with the fixtures. It also supplies the population that
justifies reading the record instead of the header: **the 24 `PC 1.0+` Coda-era Windows documents state a
platform where their Mac contemporaries state a version, so they have no version at all**, and the marker
recovers all 24 where any version range would have skipped every one.

The later layout, addressed as absolute word slots across the two incidences:

| Word | Field | Word | Field |
|---:|---|---:|---|
| 0 | `measWidth` | 6 | `symSpacing` |
| 1 | *unnamed, zero in all 3,458 files* | 7 | `numAdjX` |
| 2 | `numAdjY` | 8 | `startAdjust` |
| 3 | `shapeDef` | 9 | `endAdjust` |
| 4 | `numStart` | 10 | *unnamed, zero in all 3,458 files* |
| 5 | `useSymsThreshold` | 11 | flags; bit 0 is `useSymbols` |

The early layout keeps only three of them, and two have moved: `measWidth` is still word 0, but `numAdjY` is
word **4** and `shapeDef` word **5**. Reading an early document through the later table would report its number
adjustment as a shape comparator. Words 1–3 hold something Finale 3.5 stopped writing — word 1 varies per
document across 0, 1, 2, 4, 5, 14, 16, 21, 22 and 25, and words 2–3 move together as `(24, 0)` or `(14, 1)` in
the Coda era and are `(0, 0)` in Finale 3.0 and 3.2. Finale 27's conversion carries nothing from them, so no
companion can name them; they are **open**.

Agreement with exact Finale 27 companions is complete: all 1,130 companion-backed later-layout documents match
on all nine scalars and on `useSymbols`, and all 59 companion-backed early-layout documents match on all three,
with no disagreement of any kind.

Two boundaries do not coincide with the layout marker:

- **Selector `83` arrives with Finale 97, internal 3.8.** No 1.0.0, `PC 1.0+`, 1.8.7, 2.0.1, 2.6, 3.0, 3.2, 3.5
  or 3.7 document in any survey carries it, and every 3.8, 97, 98 and later one does — the installs corpus
  confirms both spellings of that release and the Finale 98 that follows it, neither of which the reference
  corpus contains. Word 4 is `autoUpdateMmRests`; word **2** of the same record is also set in most documents
  and is *not* this flag — 468 companion-backed documents carry word 2 set with word 4 clear and none of their
  conversions has `<autoUpdateMmRests/>`, while all 73 that carry word 4 do. All 22 companion-backed documents
  that lack the selector entirely convert with the flag off. Across both large surveys the flag is only ever
  *set* in a zlib-era document, which is consistent with a feature added long after its record.
- **The H-bar adjustments and automatic updating are era behavior before their records exist.** The pinned
  Finale 27 baseline starts the H-bar 30 Evpu in, ends it 30 Evpu out, and switches automatic updating on; an
  early document states none of the three, and every early companion converts with all three at zero or false.
  The reader asserts them as `LegacyBehavior` rather than inheriting the baseline, and an absent selector `83`
  means automatic updating is off with no further qualification — including for a document whose epoch could not
  be classified, which is the document most at risk of being left claiming a Finale 27 setting. The rest of what
  the early era omits — `numStart` 2, `useSymsThreshold` 9, `symSpacing` 48, `numAdjX` 0 and `useSymbols` false —
  is left to the baseline, which already carries exactly what those conversions produce.

The `shapeDef` comparator is the source's own in every era and agrees with the companion in all 2,305 compared
documents, but 319 zlib documents name a shape their own file does not define. Those files carry no shape records at
all, while other zlib documents in the same corpus carry all three shape classes, so it is a property of those
sources rather than a decoding gap and the reader notes it at `Info` rather than warning; see [PRODUCTION_READINESS.md](../../state/PRODUCTION_READINESS.md#p22-dangling-shape-references-in-seeded-options).

`noHorizontalStretch` has no legacy spelling to find. **"Stretch Horizontally" is a Finale 27 feature**, so no
legacy format has anywhere to put it, and its value is known exactly — false — for every document this reader
will ever open. The corpus is consistent with that and cannot have shown it on its own: bit 0 is the only bit of
the flags word any document uses, the word is exactly 0 or 1 in all 3,458 later-layout documents, and no
companion sets `<noHorizontalStretch/>`. The reader asserts it as `LegacyBehavior` in every era rather than
leaving it to the pinned baseline, which also says false but says it as one Finale 27 document's setting rather
than as a fact about the formats.
