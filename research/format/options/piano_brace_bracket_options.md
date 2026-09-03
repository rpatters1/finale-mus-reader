# PianoBraceBracketOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Piano brace and bracket options

**Provisionally implemented; private-framework-derived and not yet independently
binary-verified.** The musxdom `PianoBraceBracketOptions` singleton combines the default
group-bracket distance with eleven brace-shape values. The current public
[`FCPianoBracePrefs` documentation](https://pdk.finalelua.com/class_f_c_piano_brace_prefs.html),
accessed 2026-08-28, identifies the eleven shape semantics and states that each is measured
in ten-thousandths of an EVPU. It does not expose their legacy record locations.

Authorized read-only Framework history supplies the locations below. Fixed-row epochs use
numeric globals at comparator `65534`; the zlib epoch reaches the same logical words through
the established `numericGlobalClass` relationship. Four-byte values store the high word first
after each word is decoded in the container's byte order.

| musxdom field | Selector | Incidence | Word | Width |
|---|---:|---:|---:|---:|
| `defBracketPos` | `14` | 0 | 3 | 2 |
| `centerThickness` | `45` | 0 | 2 | 4 |
| `tipThickness` | `45` | 0 | 4 | 4 |
| `outerBodyV` | `60` | 0 | 0 | 4 |
| `innerTipV` | `60` | 0 | 2 | 4 |
| `innerBodyV` | `60` | 0 | 4 | 4 |
| `outerTipH` | `61` | 0 | 0 | 4 |
| `outerTipV` | `61` | 0 | 2 | 4 |
| `outerBodyH` | `61` | 0 | 4 | 4 |
| `width` | `64` | 0 | 2 | 4 |
| `innerTipH` | `65` | 0 | 2 | 4 |
| `innerBodyH` | `65` | 0 | 4 | 4 |

The importer applies the located geometry layout in the uncompressed, DCL, and zlib epochs and
leaves any absent selector at the pinned Finale 27 value. Most field meanings and scaling in those
epochs still have no controlled one-variable verification. `defBracketPos` is the exception: a
Finale 2004 save changing the dialog value from 12 to 17 moves selector `14` word 3 from -12 to -17,
and its ETF and companion agree. An intentionally ungated 140-document tracked capture put all 46
remaining `defBracketPos` disagreements before Finale 2004 and none at or after it. The implemented
mapping therefore reads this word only in Finale 2004-and-later DCL files and in the zlib epoch.
Earlier files retain the pinned -12 baseline even when selector `14` is present. The subsequent
tracked capture has no unexpected class differences: 1,671 leaves agree and nine retain their
previously approved classifications out of 1,680.

Selector `45` predates the brace-thickness options it eventually stores. In the full corpus, three
documents each from Finale 3.0, 3.2, and 3.5 carry an all-zero selector `45`; interpreting its long
at word 2 as `centerThickness` produces zero while each raw companion stores 2. Eleven other Finale
3.0 documents omit selector `45`; the pinned baseline then produces `centerThickness` 3.6 and
`tipThickness` 7.2 while their companions store 2 and 0. Finale 3.7 documents recover the stored
values without disagreement. The Finale 3.7 addendum independently identifies piano-brace
thickness controls as a 3.7 enhancement. The importer therefore treats 2 and 0 as
`LegacyBehavior` before Finale 3.7 and begins reading these two selector words at that confirmed
boundary; selector presence alone cannot distinguish the layouts.

The Coda-banner epoch uses a different representation. A controlled Finale 1.0.0 save changes only
three 32-bit IEEE-754 values from its baseline: selector `52` word 4 stores `1.61802995` for the
dialog's Beam Depth, while selector `55` words 2 and 4 store `3.14159012` and `2.71828008` for Piano
Brace 1 and Piano Brace 2. The ETF repeats the same six-word rows exactly. A second controlled save
in Finale 2.6.3 changes Beam Depth to 11.5, Def Line Width to 1.3, Piano Brace 1 to 12.7, and Piano
Brace 2 to 13.1. It stores the four values as `2.875`, `0.325`, `3.175`, and `3.275`; only the latter
two alter the modern class, producing `innerTipH` 12.6996 and `innerBodyH` 13.1 in Finale 27's XML.
The small decimal discrepancy is Finale's XML serialization of the binary float. Together the two
saves establish selector `55` words 2 and 4 as `innerTipH` and `innerBodyH`, scaled by four throughout
the Coda epoch. Finale 1 displays the stored values directly while Finale 2.6.3 displays the scaled
values; the unchanged stored default 3 becomes the modern default 12 in either case. Interpreting
selectors `45`, `60`, `61`, `64`, or `65` in this epoch is contradicted by the controlled record
diffs.

The importer recovers the two horizontal inner-body values from selector `55`. It supplies the
remaining modern geometry as Coda-era behavior: `centerThickness` is 2, `width` is 12, and the other
seven geometry leaves are zero. When selector `55` is absent, both horizontal inner-body values are
fixed at 12 instead. This matters for 24 distinct Windows Coda documents in the installs corpus:
their pinned fallback values are 24 and 0 while their raw companions explicitly store 12 and 12.
`defBracketPos` remains at the pinned default while its Coda representation is unresolved. The
ordinary selector-55 defaults recover as 12 and match the companions. Finale 27 discards the
deliberately changed Finale 1 values, so those two disagreements are classified as
`finale-upgrade-loss`. In the 140-document tracked survey, all 37 unedited Coda
documents match on both recovered fields; the four expected differences are the two fields in each
controlled edit. Five other companions use width 24, but selector `55` does not distinguish the
populations: a width-12 Finale 1.0.0 specimen, a width-24 Finale 2.6.3 specimen, and width-12 Finale
2.6.3 curve-option specimens all carry the identical row
`16128 0 16448 0 16448 0` (floats 1, 3, 3). Width 12 is therefore epoch behavior rather than a
recovered selector-55 value. Two temporary reconversions of the unchanged width-24 Finale 2.6.3
source both produced width 12, confirming that the conversion result is not source-determined; the
possible UI-versus-Lua workflow distinction remains open. The five width-24 differences are
classified as `different_defaults` only when the reader reports width as `LegacyBehavior`.
