# StaffSystem

**Covers:** Legacy staff-system records, their compact, fixed-row, and class-record layouts, and the
persisted `musx::dom::others::StaffSystem` fields.
**Read when:** Working on selector `SS`, class `0x00df`, system layout, or a StaffSystem
source/companion difference.
**Confidence:** `confirmed` for represented compact geometry, Coda system options, and expanded
field layouts; `public-PDK-derived` for expanded-layout flag meanings not isolated by a controlled
edit; `open` for the remaining compact flags and StaffUsed-dependent synthesis.

## Identity and scope

The fixed-row selector is `SS`; the zlib class is `0x00df`. The comparator is the one-based
system ID, and recovery constructs source-owned score and part instances. The public PDK names
the logical type `EDTStaffSystemSpec`; the equivalent Finale Lua wrapper is `FCStaffSystem`.
The shared public-PDK provenance is recorded in
[`pdk_public_evidence.md`](../../reference/pdk_public_evidence.md).

`pageId` is calculated by musxdom rather than persisted in the StaffSystem XML mapping, so it is
not a source field. The importer and coverage surveyor account for all 17 persisted members.

## Compact layout

Coda-banner files through represented Finale 3.7 files store one 12-byte `SS` row per system.
The exact payload width selects this decoder independently of the container epoch; the expanded
layout begins with a disjoint 24-byte prefix in Finale 3.8/97-era files.

| Byte | Width | Meaning |
|---:|---:|---|
| 0 | 2 | system 1 top adjustment; otherwise `distanceToPrev` |
| 2 | 2 | `left` |
| 4 | 2 | `right` |
| 6 | 2 | `bottom` |
| 8 | 2 | `startMeas` |
| 10 | 2 | flags; `0x0800` accompanies a separate `SP` system-options row |

All four geometry words are signed. The row has no stored `endMeas`; recovery derives it through
the common system-grid normalization below. `horzPercent` is calculated during layout conversion,
has no identified Coda source, and is deliberately left unmapped rather than synthesized; its
companion differences are still layout recalculations.
Compact-layout behavior supplies `ssysPercent = 100`, `holdMargins = true`, and the standard
`staffHeight = 4 * EFIX_PER_SPACE` when no separate system-options row exists. Other fields
absent from the row remain zero or false.
**Confirmed** in controlled Finale 1.0, 2.6.3, and 3.7.2 fixtures.

An optional 12-byte `SP` row with the same system comparator overrides the percentage and option
defaults. Word 0 is `ssysPercent`; word 1 duplicates the represented percentage, but its
independent purpose has not been isolated. The last word maps `0x2000 scaleVert` (the Coda Close
Up Space control) and `0x4000 holdMargins`. Systems with an `SP` row also set `SS 0x0800`; the
record's presence is the stronger structural selector. `horzPercent` is not synthesized from the
duplicate percentage word. **Confirmed** by independent edits in the Finale 2.6.3 fixture.

Complete `top` recovery waits for `StaffUsed`. In ordinary systems, the first stored `IU` distance
is the base system top; system 1 adds byte 0, while later systems retain that base and use byte 0
as `distanceToPrev`. Finale normalizes the selected `IU` array by subtracting its first distance.
Optimized systems and special extraction require choosing or synthesizing a different array, so
`top` remains unmapped until that selection is implemented. `hasStaffScaling` has the same
dependency. **Confirmed** for ordinary systems; optimized and extraction behavior remains
`open`.

The common Coda `SS 0x0080` bit does not vary with Hold Margins and has no assigned meaning.
Earlier evidence associating it with `holdMargins` was coincidental: the no-`SP` behavior supplies
true, while an `SP` row explicitly sets or clears the field. The post-Coda mapping independently
uses `0x0001`.

## Expanded layout

The base payload is 24 bytes and is present by the Finale 3.8/97 format. Finale 2005 adds two
signed 16-bit fields, making 28 meaningful
bytes; its third fixed row pads the physical payload to 36 bytes. The same 36-byte shape is stored
as zlib class `0x00df`. Payload size, not a saving-version gate, selects the extension.

| Byte | Width | StaffSystem member | Conversion |
|---:|---:|---|---|
| 0 | 2 | uncompressed first-system positional word or later `distanceToPrev`; otherwise `top` | signed word |
| 2 | 2 | `left` | signed word |
| 4 | 2 | `right` | signed word |
| 6 | 2 | `bottom` | signed word |
| 8 | 2 | `startMeas` | unsigned word |
| 10 | 2 | flag word | mappings below |
| 12 | 2 | `endMeas` | unsigned word |
| 14 | 4 | `horzPercent` | signed long divided by 100 |
| 18 | 2 | `ssysPercent` | signed word |
| 20 | 2 | DCL/zlib `distanceToPrev`; uncompressed identity open | signed word |
| 22 | 2 | `staffHeight` | signed word multiplied by 4 Efix |
| 24 | 2 | `extraStartSystemSpace` | signed word, extended layout |
| 26 | 2 | `extraEndSystemSpace` | signed word, extended layout |

The horizontal-percentage long is high-word-first in both byte orders. Individual words still use
the container byte order. In uncompressed files, byte 0 supplies `distanceToPrev` after system 1;
system 1 retains the byte-20 value. DCL and zlib records store `top` and `distanceToPrev` directly
at bytes 0 and 20. Uncompressed `top` remains unmapped until `StaffUsed` recovery can account for
per-system staff positions; the near-complete page-format formula is retained in the
[investigation](../../investigations/staff_system.md#2026-09-13--uncompressed-vertical-layout-synthesis-top-recovery-deferred).
The uncompressed byte-20 word has not been identified beyond being zero in the represented
comparisons. A zero stored staff height in represented early post-Coda files denotes the standard
four-space height; recovery supplies `4 * EFIX_PER_SPACE` as `LegacyBehavior`. Nonzero values are
source-owned quarter-Efix units. **Confirmed** for the distance mapping in the public Finale 97,
98, and 2000 fixtures; uncompressed top recovery is **open**.

The flag word maps `0x0001 holdMargins`, `0x0002 scaleVert`, `0x0008 noNames`, and
`0x0010 placeEndSpaceBeforeBarline`. `hasStaffScaling` is an aggregate presence value and remains
unmapped until it can be recalculated from recovered `StaffUsed` records in every version. Recovery
deliberately discards `0x4000` until that same work. It also discards `0x8000`; Finale did not use
that bit for page breaks, which are persisted on `Measure`.

## System-grid normalization

Every physical layout is normalized by the same structural rule. Per part, a usable family is the
maximal prefix whose comparators are systems `1..N` and whose nonzero `startMeas` values rise
strictly without passing the last recovered measure. A zero, non-rising, out-of-range, or
nonsequential record terminates that prefix; it and all later records are stale layout data and are
not constructed as musxdom systems. Each retained non-final `endMeas` is derived from the next
system's `startMeas`, and the final end is one past the last recovered measure. Expanded stored
`endMeas` words remain recovery provenance but do not define the musxdom grid. Without recovered
measures, the same sequence validation applies, derived non-final ends remain available, and the
terminal end is zero. **Strong** across the represented compact and expanded tracked population.

## Companion behavior

Tracked-evidence coverage still contains unmatched system collections. A controlled Finale 2000
Update Layout save confirms that `horzPercent` can be recalculated by layout. Any paired-value
difference in `horzPercent` is classified as `finale-layout-recalculation`. A differing final
`endMeas` receives that classification when the source grid begins at measure 1 and its final
system ends one past the last source measure, regardless of the companion value. A middle
`startMeas` or `endMeas` difference additionally requires the companion grid to begin at measure
1 and end at or beyond the correct source end. Differences in uncompressed `top`,
`distanceToPrev`, and `hasStaffScaling` are classified as awaiting dependent `StaffUsed`
recovery; differences in other fields remain unexpected. Counts and representative
transformations are in
[`../../investigations/staff_system.md`](../../investigations/staff_system.md).

The class remains partial until compact and expanded `top`, `hasStaffScaling`, optimized-system
selection, and special extraction are implemented with `StaffUsed`.
