# FlagOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Flag options

**Partially implemented.** The public PDK documents the three omnibus preference
classes [`FCSizePrefs`](https://pdk.finalelua.com/class_f_c_size_prefs.html),
[`FCDistancePrefs`](https://pdk.finalelua.com/class_f_c_distance_prefs.html), and
[`FCMiscDocPrefs`](https://pdk.finalelua.com/class_f_c_misc_doc_prefs.html) (accessed
2026-08-29), but their published properties do not expose the modern FlagOptions
members. Those declarations are **public-PDK-derived**. The authorized read-only
Framework preference-location tables likewise contain no FlagOptions member names;
that negative result is **private-framework-derived** and leaves the physical mapping
to binary evidence.

The editable fixed-row layout stores nineteen fields across five numeric globals. The
zlib layout reaches the same word offsets through `numericGlobalClass(selector)`:

| musxdom field | Selector | Incidence | Word | Width |
|---|---:|---:|---:|---:|
| `straightFlags` | `5` | 0 | 2 bit 0 | 2 |
| `upHAdj` | `73` | 0 | 0 | 2 |
| `downHAdj` | `73` | 0 | 1 | 2 |
| `upHAdj2` | `73` | 0 | 2 | 2 |
| `downHAdj2` | `73` | 0 | 3 | 2 |
| `upHAdj16` | `73` | 0 | 4 | 2 |
| `downHAdj16` | `73` | 0 | 5 | 2 |
| `upVAdj` | `74` | 0 | 0 | 2 |
| `downVAdj` | `74` | 0 | 1 | 2 |
| `upVAdj2` | `74` | 0 | 2 | 2 |
| `downVAdj2` | `74` | 0 | 3 | 2 |
| `upVAdj16` | `74` | 0 | 4 | 2 |
| `downVAdj16` | `74` | 0 | 5 | 2 |
| `stUpHAdj` | `75` | 0 | 2 | 2 |
| `stDownHAdj` | `75` | 0 | 3 | 2 |
| `stUpVAdj` | `75` | 0 | 4 | 2 |
| `stDownVAdj` | `75` | 0 | 5 | 2 |
| `flagSpacing` | `76` | 0 | 0 | 2 |
| `secondaryGroupAdj` | `76` | 0 | 1 | 2 |

**Confirmed** for the zlib layout by
`tests/evidence/F2012/F2012-flagopts.mus`, whose class records and independently
parsed companion agree on the straight-flags switch and all eighteen mapped distance
values. **Confirmed** for the uncompressed layout
by `tests/evidence/F372/F372-flagopts.mus`: selector `5` word 2 changes from zero to one,
selector `75` words 2 and 3 change from zero to 704 and -704, and selectors `73`--`76`
carry distinct edited values for every numeric field. Its ETF repeats the source words
and its companion agrees on all nineteen semantic results. Selector `65` word 1 also
changes in that save, but the companion's `eighthFlagHoist` remains 1856 in both the
baseline and edit, so that unrelated change is not used. DCL uses the same fixed-row
addressing and has no disagreement in the tracked-evidence capture. **Confirmed** for
that layout by `tests/evidence/F2003/F2003-flagopts.mus`: its ETF repeats the source
rows, and its independently parsed companion agrees on the straight-flags switch and
all eighteen mapped distance values.

The editable layout identifies itself structurally: selector `75`, which stores its
straight-flag coordinates, is absent together with selectors `73`, `74`, and `76` from
all 44 tracked Coda-banner files and from checked Finale 3.0 and 3.2 corpus samples; all
four selectors are present in a checked Finale 3.5 sample, every tracked Finale 3.7--2006
file, and as corresponding class records from Finale 2007 onward. Selector `5` exists on
both sides and cannot mark the boundary. The importer therefore reads its word-2 bit as
`straightFlags` only when selector `75` is present. This avoids a version gate, covers
versionless Windows files, and agrees with the straight-flags option documented in the printed
Finale 3.5 addendum. The addendum and record population together strongly place its introduction
at that release. A later fixed-row file damaged enough to lose selector `75` conservatively
retains seeded defaults for the editable family.

The Coda-banner layout stores vertical-origin choices rather than independent coordinates.
Selector `10` words 4 and 5 independently select the upward and downward vertical origins.
Two controlled Finale 2.6.3 edits, each accompanied by an era ETF, make those switches and
their modern expansions **confirmed**. Selector `10` word 3 changes from 7 to 11 with a
controlled Finale 1.0.0 Flag Offset edit that also changes selector `36` word 5. A second
controlled edit changes Flag Offset alone from 7 to -3; Finale 27 leaves all three modern
upward-horizontal adjustments at 1696. An independent Finale 2.6.3 edit repeats the same
one-field source change and modern result. Selector `10` word 3 is therefore the Flag Offset,
but neither a bitfield nor the source of the 1696-to-zero conversion in the two-change
fixture. It may be a distance in an unidentified unit, potentially interpreted with the
selected music font's glyph geometry.

The importer therefore leaves `upHAdj`, `upHAdj2`, and `upHAdj16` at their pinned defaults
and reports `Finale27Default`. This era has no `straightFlags` option; the importer likewise
leaves the pinned Finale 27 value `false` untouched. Selector `36` word 5 was
initially mistaken for that switch because it moved in the Finale 1.0.0 save, but the UI
correction and the absent editable-family structure refute that interpretation. Its
meaning remains open and it is not read as FlagOptions.

The remaining Coda fields are fixed source behavior: downward horizontal and both
straight horizontal adjustments are zero; the secondary curved vertical positions and
both straight vertical positions are 1536 in magnitude; flag spacing is 24 and the
secondary-group adjustment is zero. The controlled companions agree, and the existing
Coda cohort repeats the same values. `eighthFlagHoist` has no established source in any
supported epoch, and musxdom notes that it is absent from Finale's Flag Options UI. The
tracked companions vary it when only the default music font changes (1600, 1856, or 1984),
while the controlled UI edits do not identify a stored field for it. It therefore retains
the pinned Finale 27 value and is reported as `Finale27Default` in every epoch.

Controlled uncompressed, DCL, and zlib fixtures locate every UI-editable member in each
physical representation. The absence of a recovered `eighthFlagHoist` is intentional
fallback behavior rather than remaining implementation scope.

The refreshed 158-document `tracked-evidence` capture reports 2,974 equal FlagOptions
leaves and 186 expected `different_defaults`, with no unexpected differences. Of those,
135 are the three upward-horizontal fields in the 45 Coda companions that store 1696; the
one controlled companion that stores zero agrees with the pinned default. The other 51 are
`eighthFlagHoist`. The classifier requires `Finale27Default` origin, so a future
source-owned disagreement remains visible. Its allowlist comprises `eighthFlagHoist` and
the twelve members for which the broader corpus exhibits reviewed baseline/companion
differences; it does not classify every baseline-sourced FlagOptions member.

The authorized three-survey capture selects 16,253 occurrences representing 7,219
distinct source ids. Of those, 16,164 occurrences import successfully and 4,564 have
successful companions. `eighthFlagHoist` contributes 724 expected `different_defaults`
leaves across all four epochs. The broader population also exposes 318 unexpected
FlagOptions leaves in 60 distinct documents, all outside DCL and zlib. Forty-three Coda
documents store selector `10` word 3 as `2`; expanding that state to 1696 disagrees with
raw companions that explicitly store zero in all three upward horizontal fields. Seventeen
Finale 3.0/3.2 documents lack the editable selector `75` family and retain later seeded
defaults for up to twelve fields where their raw companions carry earlier values. The
Finale 3.0/3.2 disagreements are approved as `different_defaults` when, and only when, the
source origin remains `Finale27Default`. The exact mechanism is open; automatic adjustment
from the selected music font and its annotated glyph geometry is a plausible explanation,
and these values are not treated as recoverable UI preference buckets. The 129 Coda
horizontal differences in that capture resulted from the now-refuted bitfield
interpretation of selector `10` word 3. Those fields now retain `Finale27Default`; the
all-corpus capture predates that correction and has not been rerun.
