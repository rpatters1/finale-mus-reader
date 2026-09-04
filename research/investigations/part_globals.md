# PartGlobals investigations

**Covers:** How class `0x0120`, the pre-zlib numeric globals, Coda Scroll View cache behavior,
and missing linked-part defaults were established.
**Read when:** Revisiting a PartGlobals field location, the `PG` selector, or a source/companion
PartGlobals disagreement.
**Confidence:** `confirmed` where controlled fixtures independently expose the source and semantic
result; `strong` for the pre-zlib special-extraction location.

## 2026-09-04 — Locating the four fields across the physical epochs

**Question.** Did the plug-in-facing PartGlobals selector remain one physical record across every
legacy era, or did the settings move from numeric options into a part-scoped class record?

**Method.** The four musxdom members and XML mapping were inventoried first. Numeric global rows
from controlled Coda, uncompressed, and DCL fixtures were compared with their ETF and Finale 27
companions. Zlib class candidates were read in both byte orders and compared with controlled
Finale 2007–2012 companions, including the linked-part fixture
`tests/evidence/F2012/F2012-noteartexp.mus`.

**Result.** The pre-zlib representation is split: selector 12 word 0 carries
`showTransposed`, while selector 23 word 4 carries `specialPartExtractionIUList`. The controlled
concert-pitch edit `tests/evidence/F263/F263-concert-pitch.mus` changes the first from one to zero
and the companion clears `showTransposed`. The later physical representation is class `0x0120`, a
12-byte record whose first four words are the four PartGlobals members in order and whose last two
words are unused. Its comparator remains `65534`, while zlib record ownership distinguishes score
and linked-part instances.

The `PG` spelling in historical plug-in mappings is therefore a logical API selector, not evidence
that pre-zlib files contain a literal `PG` row. The exact layout and confidence labels are recorded
in [`../format/others/part_globals.md`](../format/others/part_globals.md).

## 2026-09-04 — Coda Scroll View state is a disposable UI cache

**Question.** Does the extra `IU(65531)` family in the new Coda quartet pair supply
`scrollViewIUlist`, and should the reader preserve it?

**Method.** `tests/evidence/F100/F100-quartet.mus` and
`tests/evidence/F100/F100-quartet-oboeview.mus` were compared at normalized record level and
against their ETF exports and Finale 27 companions. The baseline has no `IU(65531)` family; the
oboe-view edit adds one incidence selecting only staff 2. No other PartGlobals candidate changes.

**Result.** The family records the saved Scroll View subset, but both Finale 27 companions omit
`scrollViewIUlist`. The value is document-specific UI cache state, not score semantics. The reader
therefore ignores the family and supplies `BASE_SYSTEM_ID` throughout the pre-zlib epochs. The two
fixtures have public evidence tokens `mus-f38017dde5514645` and `mus-e62f183d5d078d2a`.

## 2026-09-04 — Missing linked-part records and corpus validation

**Question.** Three controlled Finale 2012 sources lack a physical linked-part PartGlobals record,
while their companions contain an unshared part instance with `showTransposed` true. Should the
reader inherit the score object or should musxdom synthesize the modern default?

**Method.** The missing records were confirmed independently of companion comparison. musxdom's
`resolvePartDefinitions` behavior was then aligned with the companion by setting
`showTransposed = true` on the unshared PartGlobals object it already creates for a missing part.
The musxdom factory test and the reader's linked-part fixture test cover the result.

**Result.** The three previously differing linked-part instances now match their companions. The
tracked capture compared 226 instances across 223 source/companion pairs with no PartGlobals
difference. The authorized all-corpus capture then compared 6,711 instances across 4,631
source/companion pairs, again with every PartGlobals leaf matching. Of 16,320 selected occurrences,
16,231 imported; the other 89 were 58 Finale libraries and 31 files that did not classify as MUS,
so none supplies contrary PartGlobals evidence.
