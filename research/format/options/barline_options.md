# BarlineOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Barline options

**Implemented in full.** The public PDK documents the semantic fields across
[`FCSizePrefs`](https://pdk.finalelua.com/class_f_c_size_prefs.html),
[`FCDistancePrefs`](https://pdk.finalelua.com/class_f_c_distance_prefs.html), and
[`FCMiscDocPrefs`](https://pdk.finalelua.com/class_f_c_misc_doc_prefs.html) (accessed
2026-08-28). It identifies heavy and thin widths and the two inter-line spaces as EFIX,
and both dash values as 32-bit EVPU values. It also states that double barlines before
key changes are available only in Finale 2014.5 and later. Those declarations are
**public-PDK-derived**; they do not expose the legacy record locations.

Authorized read-only Framework history supplied the initial locations below. They are
**private-framework-derived** leads, promoted into the reader after comparison with legacy
records and Finale 27 companions. Fixed-row epochs address selector, incidence, and word;
the zlib epoch reaches the same word through `numericGlobalClass(selector)`. The two longs
store the high word first after each payload word has been decoded in the container's byte
order.

| musxdom field | Selector | Incidence | Word | Width | Earliest interpreted layout |
|---|---:|---:|---:|---:|---|
| `drawBarlines` | `36` | 0 | 4 | 2 | Coda banner |
| `drawCloseSystemBarline` | `03` | 0 | 4 | 2 | uncompressed |
| `drawCloseFinalBarline` | `03` | 0 | 5 | 2 | uncompressed |
| `drawFinalBarlineOnLastMeas` | `09` | 0 | 5 | 2 | DCL |
| `drawLeftBarlineSingleStaff` | `36` | 0 | 3 | 2 | unified pre-expanded layout |
| `drawLeftBarlineSingleStaff` | `36` | 0 | 2 | 2 | expanded barline layout |
| `drawLeftBarlineMultipleStaves` | `36` | 0 | 3 | 2 | Coda banner |
| `leftBarlineUsePrevStyle` | `36` | 0 | 1 | 2 | Finale 2000 |
| `thickBarlineWidth` | `67` | 0 | 2 | 2 | when present |
| `barlineWidth` | `58` | 0 | 4 | 2 | when present |
| `doubleBarlineSpace` | `67` | 0 | 3 | 2 | when present |
| `finalBarlineSpace` | `67` | 0 | 4 | 2 | when present |
| `barlineDashOn` | `68` | 0 | 2 | 4 | when present |
| `barlineDashOff` | `68` | 0 | 4 | 4 | when present |

Selector `36` word 1 is the inverse of the PDK's “normal default barline” property, so a
nonzero stored word maps directly to musxdom's “use previous style” field. Selector `58`,
`67`, and `68` are absent from the surveyed Coda layouts. The absent selector `58` width is
fixed legacy behavior: Finale renders the thin barline at 224 EFIX, or 3.5 EVPUs, and every
Coda companion carries that value. Because the pinned baseline instead supplies 115, the
reader reports 224 as `LegacyBehavior`. The geometry and dash fields from absent selectors
`67` and `68` retain their pinned defaults; no Coda values for them have been established.

Several words exist before they acquire these meanings. In every companion-backed Coda
document, interpreting selector `03` words 4--5 as the later close-barline options produces
values the conversion does not preserve. The reader gates those words at the uncompressed epoch.
The previous-style option was introduced in Finale 2000; selector `36` word 1 is therefore gated
at Finale 2000 inside the uncompressed epoch. That introduction boundary is a user-supplied
interoperability fact. A contemporary public [Finale Forum thread about Finale
98](https://www.finaleforum.com/boards/general/messages/1376.html) and its
[technical reply](https://www.finaleforum.com/boards/general/messages/1378.html) (accessed
2026-08-28) independently support the earlier half of the boundary: producing a double barline
at the left of a system required a graphical workaround rather than a document option. The forum
evidence does not by itself identify Finale 2000 as the first release. Earlier documents behave
as though the option were false, which both pinned Finale 27 baselines already supply, so those
documents retain `Finale27Default` rather than duplicating the value as `LegacyBehavior`.

The Coda-era dialog exposes one left-barline checkbox rather than separate single- and
multiple-staff choices. In the controlled Finale 1.0.0 pair, toggling that checkbox changes only
selector `36` word 3, from one to zero. Finale 27 converts the baseline to both modern fields true
and the edited document to both false. The unified layout therefore fans word 3 out to both
musxdom fields rather than treating the later single-staff field as absent.

The adjacent general “draw barlines” checkbox is independent. Clearing it in a second controlled
Finale 1.0.0 save changes only selector `36` word 4 from one to zero; word 3 remains one. Its ETF
repeats the tuple, and Finale 27 removes only `drawBarlines` while preserving both left-barline
fields. Word 4 therefore maps directly with nonzero meaning enabled in the Coda layout.

The split layout is self-identifying. Selectors `67` and `68` are part of the expanded barline
options family, carrying the line geometry and dash preferences above. Both are absent from every
readable Finale 3.0/3.2 specimen and present in every readable Finale 3.5/3.7 specimen examined;
selector `67` presence selects word 2 as the independent single-staff switch. DCL and zlib epochs
use the expanded layout by epoch. This removes the Finale 3.5 version gate and also covers an
uncompressed file whose header version is unavailable. The printed Finale 3.5 addendum independently
identifies the split single-staff/multiple-staff controls as a 3.5 change. The version boundary is
therefore **confirmed**, although the decoder continues to prefer the structural marker.

The automatic-final-barline word is not operative before the DCL epoch. Earlier selector `09`
values change across releases without tracking the companion option, while all corresponding
companions disable it. The reader supplies false as `LegacyBehavior` before DCL because the
pinned Finale 27 baseline enables it. In the controlled Windows Finale 2001 pair,
`F2001Win-finalbarline-toggle.mus` changes selector `09` word 5 from 1 to 0, and both its ETF and
Finale 27 companion disable `drawFinalBarlineOnLastMeas`. This confirms the DCL lower boundary.
`drawDoubleBarlineBeforeKeyChanges` is likewise false as
`LegacyBehavior` in every supported MUS epoch, all of which predate the public PDK's Finale
2014.5 boundary. Absent mapped fields otherwise retain `Finale27Default` origin.

The tracked-evidence capture compares all fourteen fields in 137 source/companion pairs. All 1,918
leaves agree, including both controlled Finale 1.0.0 checkbox edits and every Coda fixed-width
case. No expected-difference rule is needed for BarlineOptions.

The three-survey regression compares all fourteen fields in 4,545 successful companion pairs.
All 63,630 leaves agree, with no expected or unexpected differences and no missing or extra
leaves. The epoch spread is 1,918 Coda-banner leaves, 4,508 uncompressed leaves, 28,490 DCL
leaves, and 28,714 zlib leaves. This independently exercises source versions from Finale 1.0.0
through the final MUS era, as well as 24 companion-backed documents with no recovered version.
No expected-difference rule is needed for BarlineOptions.
