# StaffSystem investigations

**Covers:** How the compact and expanded StaffSystem layouts, 2005 extension, unit conversions,
and current
companion differences were established.
**Read when:** Revisiting selector `SS`, class `0x00df`, Coda systems, or proposing a StaffSystem
comparison rule.
**Confidence:** `confirmed` for represented compact geometry, Coda system options, and expanded
bytes; `open` for the remaining compact flags, StaffUsed-dependent synthesis, and unreviewed upgrade
transformations.

## 2026-09-13 — Post-Coda field layout and structural extension

**Question.** Does the plug-in-facing `EDTStaffSystemSpec` layout directly describe the stored
`SS` rows, and where does the Finale 2005 extension begin?

**Method.** The 17 persisted musxdom members were inventoried before decoding. `SS` fixed rows and
zlib class `0x00df` were dumped from controlled Finale 97, 2005, 2006, 2007, and 2012 fixtures in
both represented byte orders, then compared independently with ETF or Finale 27 EnigmaXML. The
public [`FCStaffSystem`](https://pdk.finalelua.com/class_f_c_staff_system.html) documentation was
consulted on 2026-09-13 for the logical API surface. Synthetic tests exercised every field, both
byte orders and framings, both payload lengths, all active flag bits, the two ignored high bits,
zero staff height, truncation, and Coda exclusion.

**Result.** The post-Coda stored prefix follows the field order recorded in
[`../format/others/staff_system.md`](../format/others/staff_system.md), including a four-byte
horizontal percentage at byte 14 and staff height at byte 22. The long remains high-word-first in
little-endian class records. Staff height uses quarter-Efix legacy units; early zero values mean a
standard four-space staff. Finale 2005 adds `extraStartSystemSpace` and
`extraEndSystemSpace` at bytes 24 and 26. The meaningful extension is selected by a payload of at
least 28 bytes, while 25–27 bytes is rejected as neither recognized layout.

Controlled evidence includes `tests/evidence/F97/Fin97-baseline.mus`,
`tests/evidence/F2006/F2006-empty.mus`,
`tests/evidence/F2007/F2007-lyric-hyphens.mus`, and
`tests/evidence/F2012/F2012-baseline.mus`, with their tracked companions. The distinct Coda row is
present in `tests/evidence/F263/F263-baseline.mus`; its exclusion at this stage is superseded by
the Coda investigation below.

## 2026-09-13 — Coda geometry and deferred StaffUsed synthesis

**Question.** Which modern StaffSystem fields come from the six-word Coda `SS` row, and how does
Finale obtain system top and end measure values that the row does not directly store?

**Method.** One hundred bars were added independently to the controlled Finale 1.0 and 2.6.3
baselines, Update Layout was invoked, and each result was saved with an ETF export and Finale 27
companion. Systems 1 and 5 were then given distinct values in all four system-margin controls.
The resulting `SS` and `IU` rows were compared with the companion `staffSystemSpec` and `instUsed`
objects. Evidence is reproducible against `tests/evidence/F100/F100-update-layout.mus`,
`tests/evidence/F100/F100-chg-sys.mus`, `tests/evidence/F263/F263-update-layout.mus`, and
`tests/evidence/F263/F263-chg-sys.mus`. A second Finale 2.6.3 derivative added a staff and set
systems 1--3 to distinct percentages and combinations of Hold Margins and Close Up Space;
`tests/evidence/F263/F263-sysopts.mus` records that edit.

**Result.** In both Finale versions, `SS` bytes 2, 4, 6, and 8 map directly to `left`, `right`,
`bottom`, and `startMeas`. Byte 0 contributes a top adjustment on system 1 and maps to
`distanceToPrev` thereafter. Finale constructs a system's exclusive `endMeas` from the next
system's start, using one past the last recovered measure for the final system. The two versions
independently reproduce the same layout. **Confirmed.**

The raw Finale 1.0 `IU(0)` first distance is -80. Its edited system 1 has byte 0 = -257 and the
companion has `top = -337`; edited system 5 has byte 0 = -481, `top = -80`, and
`distanceToPrev = -481`. Finale 2.6.3 likewise uses base -188: its corresponding values are
-161/-349 for system 1 and -151/-188/-151 for system 5. In every ordinary upgraded system the
first `instUsed.distFromTop` is zero, with later positions shifted by the same first-distance
amount. This confirmed the ordinary-system formula but did not establish which legacy array
applies to an optimized system or how special extraction is synthesized. The deferred top
conclusion is **superseded** by the completed [`StaffUsed` investigation](staff_used.md);
`hasStaffScaling` remains separate work that depends on `StaffSize`.

The base Coda row contains no horizontal percentage, system percentage, staff height, end
measure, or extra-system-space words. Finale recalculates horizontal percentages; recovery
deliberately does not synthesize this value because it is not useful to musxdom, while comparison
classifies its differences as layout recalculation just as it does for a recovered value. In the
absence of a separate options record, Finale supplies system percentage 100, Hold Margins true,
and standard staff height, and derives end measures as above.

The three edited systems introduce `SP` rows whose first two words both contain 83, 85, and 87.
Their last words are respectively zero, `0x4000`, and `0x2000`, establishing word 0 as the
recovered `ssysPercent`, `0x4000 holdMargins`, and `0x2000 scaleVert` (Close Up Space). All three
`SS` rows change from `0x0080` to `0x0880`, which associates `0x0800` with the separate record but
does not distinguish its options. Actual `SP` presence is therefore used as the structural
selector. The controlled clearing of Hold Margins also refutes the provisional `SS 0x0080`
mapping: that common bit has no assigned meaning, and true is the no-`SP` default. **Confirmed.**

A refreshed tracked capture selected all 329 public controlled occurrences and six private ones;
all 335 sources and companions imported. Before the provisional flag mapping, the 168 paired Coda
StaffSystem instances contributed 168 identical `holdMargins` transformations from a set
`0x0080` source bit to a true companion value. That population is the observational basis for the
mapping; the refreshed post-implementation counts are recorded below.

## 2026-09-13 — Compact-to-expanded structural boundary

**Question.** Does the six-word `SS` representation end with the Coda-banner container, or does it
continue into early uncompressed files?

**Method.** Raw `SS` families were sampled in Finale 3.0, 3.2, 3.5, and 3.7 documents from the
authorized all-corpus snapshot and compared with a Finale 3.8/97 sample. The tracked
`tests/evidence/F372/F372-fileinfo-text.mus` row provides a public early-uncompressed check.

**Result.** Every sampled 3.0--3.7 system is a single 12-byte row with the compact geometry, while
the 3.8/97 sample has the 24-byte expanded prefix. The earlier epoch gate therefore left 1,388
StaffSystem instances companion-only across 135 paired early-uncompressed documents. Recovery now
selects the compact decoder from the exact 12-byte payload rather than from the container epoch.
The 12-byte public Finale 3.7.2 row independently reproduces the boundary's early side.
**Confirmed** for Finale 3.7.2; **strong** across represented Finale 3.0--3.7 corpus samples.

The structural-decoder tracked capture selected 330 public and six private occurrences,
representing 333
distinct contents; every source and companion imported. Four distinct Finale 3.7 sources now
recover seven source StaffSystem instances. Six paired instances contribute 95 equal and 13
classified leaves, and the seventh contributes 18 reader-only score leaves. Another 504 Finale
3.7 companion-only leaves remain where no source `SS` instance was recovered. StaffSystem as a
whole has 13,255 equal, 1,181 expected, no unexpected, 774 reader-only score, and 1,890
companion-only leaves in this snapshot.

All 774 reader-only score leaves are 43 trailing systems in seventeen distinct uncompressed
files. In sixteen expanded-layout files, 42 systems follow a source system whose stored
`endMeas` already passes one beyond the last recovered measure: fifteen Finale 97/2000 documents
account for the one-measure piece with system 1 before two trailing records, and one Finale 98
document accounts for its eight measures with system 3 before twelve trailing records. The
remaining compact Finale 3.7.2 document has systems 1--3 with strictly rising starts covering its
seven measures, followed by system 4 with start zero. The stricter structural rules therefore
identify all 43 source-only systems as stale layout data without relying on companion membership.
**Confirmed** for the compact case by the tracked fixture and **strong** across the expanded
tracked population.

The stored-end cutoff was subsequently superseded by applying the compact grid reconstruction to
every physical layout. Narrowing the `endMeas` classification first exposed nineteen terminal
differences: seventeen expanded stored ends exceeded the exclusive document end, while two compact
ends were already synthesized as the exclusive document end. Every difference belonged to the
final usable source system, and the two compact companions merely divided that source system into
additional systems. This supports deriving all retained ends from the validated start sequence and
classifying only a terminal derived end that agrees with the companion's terminal end. **Strong.**

The post-filter tracked capture used the same 336 occurrences; every source and companion
imported. StaffSystem now has 13,256 equal, 1,180 expected, no unexpected, no reader-only, and
1,890 companion-only leaves. All 774 leaves from the 43 trailing systems disappeared. One prior
expected difference became equal because the retained terminal compact system now receives the
recovered exclusive document end. **Confirmed** for the tracked population.

## 2026-09-13 — Tracked-evidence companion comparison

**Question.** Does direct source recovery equal the representation written by modern Finale, and
which discrepancies remain before StaffSystem can be considered complete?

**Method.** The instrumented Release probe selected the generated manifests for both controlled
surveys: 324 public occurrences (321 distinct contents) and six private occurrences (six distinct
contents). It received the installed Mac symbol-font list and all three installed non-General-MIDI
percussion annotation tables. All 330 sources had successful companions. Raw bytes and companion
XML from the controlled cross-epoch fixtures were checked separately from the comparison.

**Result.** Across the combined 330 occurrences (327 distinct contents), StaffSystem produced
9,887 equal leaves, 931 unexpected differing leaves in 88 distinct documents, 756 reader-only
score leaves, and 2,844 companion-only leaves. Unexpected differences occur only after Coda:
695 in the uncompressed epoch, 226 in DCL, and 10 in zlib. The retained examples cover five
members:

- `top` and `distanceToPrev` often redistribute vertical spacing during upgrade;
- `startMeas` and `endMeas` can change when Finale rebuilds system boundaries; and
- `horzPercent` can be recalculated from zero, changed substantially by relayout, or rounded by a
  small amount.

For example, `mus-e10296cf6fed3509` changes system 1 from end measure 4, horizontal percentage
111.99, and top -72 to 2, 376.17, and -260. `mus-1f36cbe6c26b50a3` contains later systems whose
source start/end measures are zero while the companion reconstructs ranges beginning at measure
28; the same document changes 114.56 to 114.44. These are observations, not approved
classifications. The compact snapshot retained 345 StaffSystem difference examples; documents
with larger populations hit its per-document example cap, while the class counter preserves all
931 leaves. No comparison rule was added.

The result did not settle the Coda six-word layout at capture time; the later controlled Coda
investigation above now establishes its geometry. The eventual `0x4000`/`StaffUsed` relationship
and whether every post-Coda difference is a Finale relayout transformation remain open.

## 2026-09-13 — Controlled Update Layout save

**Question.** Are the unexpected `top` and `horzPercent` conversions on an untouched Finale 2000
document consequences of stale source layout data?

**Method.** `tests/evidence/F2000/F2000-empty.mus` was opened in Finale 2000, Update Layout was
invoked, and the result was saved as `tests/evidence/F2000/F2000-update-layout.mus` with an ETF
export and Finale 27 companion. The source `SS` words, ETF rows, and raw companion EnigmaXML were
inspected independently. A one-document instrumented Release probe then compared all persisted
StaffSystem leaves under public ID `mus-ce7a8a997ec2b772`.

**Result.** Update Layout changed the source horizontal percentage from zero to 33150, represented
by words `0x0000 0x817e`; the ETF repeats those words and the companion stores the same 33150.
The source and ETF both retain `top = 0`, while the companion explicitly stores `top = -80`.
The focused comparison therefore has 17 equal StaffSystem leaves and one unexpected difference,
`staff_systems[cmper=1].top: 0 -> -80`, with no unmatched StaffSystem instance. **Confirmed.**

This establishes layout staleness for the horizontal-percentage conversion in this controlled
pair. It refutes layout updating as the explanation for the independent top-offset conversion.

An uncapped comparison across the same tracked cohort found 602 paired StaffSystem instances.
Exact agreement/difference counts were 536/66 for `startMeas`, 519/83 for `endMeas`, and 314/288
for `horzPercent`. Differences occur from Finale 97 through 2008 and include zero-to-positive,
nonzero-to-nonzero, and boundary-reconstruction changes. The initial zero-to-positive rule was
first broadened to every source-backed difference in these three layout-calculated fields. The
later terminal-end analysis above superseded that broad `endMeas` classification. A subsequent
policy briefly withdrew classification from `startMeas`; the outer-boundary rule below supersedes
that policy while keeping the classification structurally bounded.

The all-corpus `startMeas` case has the same legacy content ID as the public
`F2012-upstem-flags` fixture but a separately saved modern companion. The source stores starts
`1, 4, 8, 12, 16, 20`, which the reader recovers exactly. The public companion preserves that
sequence, while the other companion explicitly stores `1, 2, 6, 10, 14, 18`; their final ends
both remain 22. Thus the difference is companion relayout history, not source decoding. It stays
within the outer-boundary layout classification. **Confirmed.**

## 2026-09-13 — Top coordinate epoch boundary (superseded)

**Question.** Does the stored-to-target `top` offset end at a reproducible format boundary?

**Method.** StaffSystem `top` differences were grouped by source epoch and saving product in the
combined tracked-evidence snapshot. Its DCL cohort contains 81 successful distinct source and
companion pairs, and its zlib cohort contains 53. The one Finale 2001 document whose compact
difference list reached its per-document cap was checked directly: all 67 source `SS` records and
raw companion `staffSystemSpec` elements were compared. Synthetic records then held the same
stored top across uncompressed, DCL, and zlib framings.

**Result.** The last observed stored-to-companion disagreement is in Finale 2000; DCL and zlib
pairs store matching `top` values. The proposed uncompressed `-80` conversion moved 33 values to
equal. This interpretation is **superseded** by the cross-field analysis below: 29 were system-1
records whose separately recovered system-top margin was `-80`, and four were accidental
later-system arithmetic matches. The controlled Update Layout fixture remains valid evidence for
its exact source and companion values, but not for a universal conversion.

## 2026-09-13 — Uncompressed vertical-layout synthesis (top conclusion superseded)

**Question.** Do the raw uncompressed `top` and `distanceToPrev` words instead feed different
modern fields according to system position?

**Method.** An uncapped tracked-evidence capture compared all paired StaffSystem instances, and
raw `SS` words were checked separately. The stored byte-0 word was evaluated against both modern
fields without the provisional `-80` conversion. PageFormatOptions system-top values were then
included as an independent input. Public Finale 97, 98, and 2000 fixtures cover both system-1 and
later-system behavior; four private occurrences add a 40-system layout.

**Result.** All 51 paired system-1 instances satisfy `modern top = recovered score system-top
margin + SS byte-0 word`. All 230 paired later systems use the byte-0 word as modern
`distanceToPrev`; this includes every one of the 154 nonzero companion distances. Their modern
top normally equals the recovered score system-top margin. Applying those rules provisionally
removed all 154 distance differences and 244 of 248 top differences. The four remaining top
differences are one private source repeated as four controlled MIDI variants: system 35 changes
from the page-format value `-188` to `-72`.

The source's `IU(35)` incidence 0 stores staff 39 followed by the signed long `-72`; adjacent
`IU(34)` and `IU(36)` incidence-0 rows store `-188`. No system-related record stores the apparent
delta `-116`. This makes the first `StaffUsed` position a plausible source of the effective system
top and means zero-normalization in `SS` cannot be excluded. Top synthesis from page format is
therefore withdrawn pending `StaffUsed` recovery, while the later-system byte-0
`distanceToPrev` mapping remains implemented. **Strong** for the distance mapping. The deferred
top conclusion is **superseded** by [`staff_used.md`](staff_used.md).

The refreshed production capture selected 325 public and six private occurrences, representing
328 distinct contents; all 331 sources and companions imported. StaffSystem has 10,058 equal
leaves, 437 expected layout recalculations, 341 differences awaiting dependent recovery, no
unexpected leaves, 756 reader-only score leaves, and 2,844 companion-only leaves. The deferred set
is 281 uncompressed `top` values and 60 `hasStaffScaling` values. Both remain visible as unexpected
when the probe runs with `--strict-deferred`.

## 2026-09-13 — All-corpus comparison

**Question.** Do the represented post-Coda layouts introduce any StaffSystem differences beyond
the reviewed layout recalculations and fields deferred until `StaffUsed` recovery?

**Method.** One instrumented Release capture selected the two private reference inventories,
the public tracked-evidence inventory, and the private controlled-evidence inventory, with all
required external symbol-font and percussion mappings supplied. Their 16,432 occurrences
represent 7,394 distinct content IDs; 16,343 sources imported, 4,940 had successful companions,
and 89 inputs failed classification before comparison.

**Result.** StaffSystem has 2,152,555 equal leaves, 30,117 expected differences, 206 unexpected
differences, 11,898 reader-only score leaves, and 30,258 companion-only leaves. By epoch, the
equal/expected/unexpected counts are 28,527/3,703/206 Coda, 107,133/10,461/0 uncompressed,
356,810/6,178/0 DCL, and 1,660,085/9,775/0 zlib. This supersedes the earlier capture in which Coda
StaffSystem was unsupported. **Strong** for the represented post-Coda population and the direct
Coda geometry.

All 206 unexpected leaves belong to 103 systems in fourteen distinct Finale 2.6 content IDs. Each
affected system contributes `scaleVert false -> true` from an unmapped source and
`ssysPercent 100 -> nondefault` from the provisional Coda behavior. Subsequent raw inspection
found an `SP` row for every one of these systems; word 0 equals the companion percentage and each
flag word is `0x6000`, consistent with both controlled option bits. The new controlled fixture
establishes that decoding, but these are still the pre-implementation capture counts; no
post-change corpus result has yet been recorded.

The post-implementation tracked-evidence capture included 330 public and six private occurrences,
representing 333 distinct contents. All 336 sources and companions imported. StaffSystem produced
13,160 equal leaves, 1,168 expected differences, no unexpected differences, 756 reader-only score
leaves, and 1,998 companion-only leaves. A separate capture of the fourteen previously failing
Finale 2.6 contents likewise imported all fourteen sources and companions. StaffSystem produced
3,386 equal leaves, 484 expected differences, no unexpected differences, and 3,960 reader-only
score leaves. The 103 `ssysPercent` and 103 `scaleVert` discrepancies that defined that cohort now
match. **Strong** across the focused population; these development-cohort results warranted the
all-corpus recapture below.

The post-implementation all-corpus capture, superseded by the grid-normalization capture below,
selected 16,433 occurrences representing 7,395
distinct contents. Of those, 16,344 sources imported, 4,941 had successful companions, and 89
failed before comparison: 58 Finale library files and 31 inputs that were not Finale MUS
documents. StaffSystem produced 2,153,177 equal leaves, 30,169 expected differences, no
unexpected differences, 11,898 reader-only score leaves, and 30,258 companion-only leaves.
Relative to the preceding capture, all 206 formerly unexpected `ssysPercent` and `scaleVert`
leaves moved to equality. The newly added controlled fixture contributed the other 416 equal and
52 expected StaffSystem leaves. All surveyed classes likewise had no unexpected differences.
**Strong** across the represented all-corpus population.

The subsequent universal grid-normalization capture selected 330 public and six private
occurrences, representing 333 distinct contents; all 336 sources and companions imported.
StaffSystem produced 12,282 equal leaves, 966 expected differences, no unexpected differences,
and 3,078 companion-only leaves. Relative to the preceding tracked capture, 66 source systems
were deliberately rejected and their 1,188 leaves became companion-only. `F97-altnotation`
stores starts `1, 4, 0`; `F2001Win-bookmarks` stores starts `1, 4` followed by 65 zero starts.
Both therefore retain their valid two-system prefixes and reject the zero-start tails. The 19
previously unexpected terminal `endMeas` differences also disappeared after all retained ends
were resynthesized. **Strong** for the structural prefix rule in the tracked population.

The corresponding all-corpus capture selected 16,433 occurrences representing 7,395 distinct
contents. Of those, 16,344 sources imported, 4,941 had successful companions, and the same 89
inputs failed before comparison. StaffSystem produced 2,133,582 equal leaves, 32,952 expected
differences, 126 unexpected differences, and 46,944 companion-only leaves. The unexpected set is
121 `endMeas` leaves across eleven contents: 47 leaves in five Finale 2007 contents, 24 in two
Finale 2008 contents, and 50 in four Finale 2012 contents. The five remaining leaves are the
deliberately unclassified `startMeas` sequence `4, 8, 12, 16, 20` versus `2, 6, 10, 14, 18` in
Finale 2012 content `mus-f95d2ea21b99022e`. No other surveyed class has an unexpected difference.
This capture predates the terminal-end refinement below. **Strong** for the represented
population; the 126 layout-boundary disagreements were retained for review.

Direct record counts show that 71 of those `endMeas` leaves are final source systems whose values
are exactly one past the source's last measure. Decoding the corresponding companions shows the
same last measure, not additional music, while their final system ends farther past it. For
example, `mus-dbf7a5f9c3b479fb` has only measure 1 on both sides but ends its source and companion
systems at 2 and 4; `mus-b0e2c1c68e58c241` ends at measure 74 on both sides but uses 75 and 76.
These are stale companion layout boundaries. A final source `endMeas` is therefore classified
when its grid begins at measure 1 and the value is one past the last source measure; the companion
endpoint is deliberately ignored. The same examples show that middle `startMeas` and `endMeas`
changes can be coupled parts of one relayout. A middle boundary is classified when the source
satisfies those outer bounds and the companion also begins at measure 1 and ends at or beyond the
source. All 126 previously unexpected differences meet the applicable conditions. **Strong** for
the represented layout-boundary cases.

The post-change all-corpus capture used the same 16,433 occurrences and import funnel.
StaffSystem produced 2,133,582 equal leaves, 32,882 expected differences, 196 unexpected
differences, and 46,944 companion-only leaves. The preceding 126 differences all moved to the
layout classification, but the part-scoped boundary check exposed 196 different terminal
`endMeas` leaves in 31 contents: 46 in Finale 2007, 48 in Finale 2008, and 102 in Finale 2012.
Every path is part-owned. In all 31 contents the source terminal end is correctly one past the
last score measure, while the corresponding companion part ends earlier. The former
document-wide maximum could borrow a qualifying endpoint from a different part and classify
these disagreements. No other surveyed class has an unexpected difference. This capture predates
the subsequent source-only terminal exception. **Strong** for the represented population.

The source-only terminal-exception capture used the same all-corpus selection and import funnel.
StaffSystem produced 2,133,582 equal leaves, 33,078 expected differences, no unexpected
differences, and 46,944 companion-only leaves. All 196 part-owned terminal differences moved to
the layout classification, and no other surveyed class has an unexpected difference. **Strong**
for the represented population and the source-boundary gate.
