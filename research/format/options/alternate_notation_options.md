# AlternateNotationOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Alternate notation options

**Implemented in full.** The seven scalar fields occupy three numeric-global families once their
corresponding settings exist. Earlier layouts may reserve the same words before they become
operative. The zlib epoch reaches the same word slots through the usual `numericGlobalClass`
relationship.

| musxdom field | Selector | Incidence | Word | Width |
|---|---:|---:|---:|---:|
| `halfSlashLift` | `22` | 0 | 1 | 2 |
| `wholeSlashLift` | `22` | 0 | 2 | 2 |
| `dWholeSlashLift` | `22` | 0 | 3 | 2 |
| `halfSlashStemLift` | `43` | 0 | 3 | 2 |
| `quartSlashStemLift` | `43` | 0 | 4 | 2 |
| `quartSlashLift` | `43` | 0 | 5 | 2 |
| `twoMeasNumLift` | `46` | 0 | 5 | 2 |

The public [`FCDistancePrefs` documentation](https://pdk.finalelua.com/class_f_c_distance_prefs.html),
accessed 2026-08-27, identifies the first three as the half-, whole-, and double-whole-note
alternate-notation baseline adjustments. It does not expose their stored locations. Authorized
read-only Framework history supplied selector `22` as the initial **private-framework-derived**
lead.

The controlled `tests/evidence/F97/F97-altnot-offsets.mus` edit independently confirms all seven
mappings. Its legacy dialog values and Finale 27 companion are `-25`, `-26`, `-27`, `7`, `-13`, `17`, and `11` in
the musxdom field order above. Against the untouched Finale 97 baseline, its MUS and ETF change
only selector `22` words 1--3, selector `43` words 3--5, and selector `46` word 5. Direct zlib
inspection of `tests/evidence/F2008/F2008-BE-text-inserts.mus` and
`tests/evidence/F2011/F2011-baseline.mus` confirms the latter families as classes `0x0039`
and `0x003c` in both byte orders. The mappings are therefore **confirmed**.

The controlled Finale 3.7 `tests/evidence/F372/F372-altnot-offsets.mus` edit confirms that
selector `43` words 3--5 can carry user-controlled values before Finale 97. Its source stores
`31`, `17`, and `27`, while its Finale 27 companion explicitly writes `7`, `-7`, and `3`.
Selector `22` words 1--3 remain zero while the companion writes `-24` for all three. A one-space
subtraction reproduces that fixture. A three-survey capture also contains 93 leaves in 32 distinct
Coda-banner and Finale 3.0--3.5 documents where Finale's upgrader chooses different defaults;
selector presence does not identify a different source layout for those values.

The structural model which preserves the most source information is:

| Structure before Finale 97 | Six slash fields | `twoMeasNumLift` | Origin |
|---|---|---:|---|
| no selector `43` | fixed `-24` | `0` | `LegacyBehavior` |
| selector `43`, no selector `46` | first three fixed `-24`; selector `43` values minus two staff spaces | `0` | fixed fields `LegacyBehavior`; adjusted fields `LegacyMusAdjusted` |
| selector `46` present | stored values minus one staff space | stored value | slash fields `LegacyMusAdjusted`; number field `LegacyMus` |
| Finale 97 and later | stored values directly | stored value | `LegacyMus` |

`LegacyMusAdjusted` identifies a source-backed value combined with source-era behavior to
produce the modern semantic value. Its diagnostics retain the stored number and source offsets;
ordinary representation decoding remains `LegacyMus`. A review of the limited available Finale 2
settings UI found no exposed alternate-notation controls, consistent with the fixed early behavior,
although preferences hidden from the UI remain possible. The coordinate- or font-origin explanation
is **weak**: the arithmetic reproduces substantially more corpus evidence than leaving the fields at
modern defaults, but no consulted declaration states why the origin moved.

The 134-document tracked capture agrees on all 938 leaves, including the controlled Finale 3.7
and Finale 97 edits. The broader three-survey capture contains 31,701 equal and 93
`different_defaults` leaves across 4,542 successful source/companion occurrences, with no
unexpected differences. The classification expresses the general behavior rather than the values
in that snapshot: before Finale 3.7, any differing source-backed alternate-notation value to which
the legacy origin adjustment applies is a source-era default difference. It does not enumerate
the observed source/companion pairs or gate on unrelated selectors.
