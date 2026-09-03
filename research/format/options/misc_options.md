# MiscOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Miscellaneous options

**Partial.** `MiscOptions` has ten persisted leaves. Pickup duration is recovered in every supported epoch from
selector `17`, comparator `65534`, word 5 (class `0x001f` in zlib). Show Active Layer Only is recovered in the
uncompressed and DCL epochs from `FI(10)`, word 4, and in zlib from class `0x008d`, comparator 10, word 4. The Coda
epoch predates layers, so that field deliberately retains the pinned Finale 27 default of false and reports
`Finale27Default`. These locations are `private-framework-derived`; selector 17 and `FI(10)` are independently
ETF-observed in the applicable fixed-row eras. The controlled Coda edit
`tests/evidence/F100/F100-pickup-512.mus` changes only selector 17, comparator 65534, word 5 from 0 to 512, and its
ETF repeats the same value. This independently confirms the Coda pickup location. **Confirmed.** A controlled
Finale 3.7.2 edit changes only `FI(10)` word 4 from 0 to 1, its ETF repeats the value, and its Finale 27 companion
writes `<showCurrentLayerOnly/>`. This independently confirms the uncompressed Show Active Layer Only location.
Opening that fixture in Finale 2012 and saving it preserves 1 at class `0x008d`, comparator 10, word 4, confirming
the zlib location; the staged save has unrelated upgrade changes and is not treated as a whole-file controlled diff.
Layers begin with Finale 3.0, making the uncompressed-epoch gate plausible from its start, but the earliest controlled
fixture that can set this option is Finale 3.7.2. **Confirmed by Finale 3.7.2; provisional for Finale 3.0--3.7.1.**

The modern Show Repeats for Parts option begins in Finale 2005, but Finale's upgrade interprets selector `16`,
comparator `65534`, word 5 as the same setting in earlier uncompressed documents. Across all 39 tracked
uncompressed fixtures, that word is 1 in exactly the twelve documents whose companions store
`<showRepeatsForParts/>`, and every source with 0 agrees with a false companion. The importer therefore reads the
word throughout the uncompressed and DCL epochs; the zlib layout stores it at class `0x001e`, comparator `65534`.
The controlled
`tests/evidence/F2005/F2005-rpts-forparts.mus` edit changes only that decoded DCL word from 0 to 1, and its ETF prints
the same selector. Opening that document in Finale 2012 and saving it as
`tests/evidence/F2012/F2012-F2005-rpts-forparts.mus` preserves 1 at class `0x001e`, word 5. Coda sources retain the
pinned Finale 27 default of false and report `Finale27Default`. The all-zero Finale 3.7 and Finale 98 populations do
not independently establish when the earlier meaning or UI began. **Confirmed from Finale 97 onward; provisional
for Finale 3.x.**

The public Framework 0.79 header identifies Consolidate Rests Across Layers as a Finale 2014 addition, Keep Octave
Transposition in Concert Pitch as a Finale 25.2 addition, and Align Measure Numbers With Barlines as a Finale 27.4
addition. Because supported legacy MUS ends at Finale 2012, Consolidate Rests Across Layers and Align Measure Numbers
With Barlines have fixed legacy behavior of false. The importer overrides their later pinned defaults and reports
both as `LegacyBehavior`. Keep Octave Transposition in Concert Pitch instead retains the pinned Finale 27 default
and reports `Finale27Default`; the option's introduction date does not establish that earlier documents behaved as
though it were disabled. Source:
[`ff_prefs.h` verbatim header, generated for Framework 0.79](https://pdk.finalelua.com/ff__prefs_8h_source.html),
accessed 2026-08-30. This is `public-PDK-derived` and has not been independently binary-verified.

Finale's older Display Concert Pitch option is distinct from Keep Octave Transposition in Concert Pitch. The
controlled `tests/evidence/F263/F263-concert-pitch.mus` fixture exercises Display Concert Pitch and is reserved for
the unimplemented `PartGlobals` class; it supplies no `MiscOptions` mapping. Its physical record change is deferred
to the `PartGlobals` investigation.

No authorized mapping establishes legacy provenance for `shapeDesignerDashLength` or `shapeDesignerDashSpace`.
They appear to persist the document-specific last-used dash choices in the Shape Designer rather than score semantics.
They are not inferred from the similarly valued Smart Shape dash fields; for now they deliberately retain the
platform-matched pinned baseline and report `Finale27Default`. `restWidthAdjust` and `dblWholeVertAdjust` likewise
have no established legacy mapping, known purpose, or located UI in any examined Finale version. They also retain the
pinned baseline and report `Finale27Default` rather than asserting source provenance.

The 179-document `tracked-evidence` comparison spans 62 Coda-banner, 39 uncompressed, 51 DCL, and 27 zlib sources,
all with successful companions. The capture has 38 Coda-banner differences each for `restWidthAdjust` and
`dblWholeVertAdjust`, both 0 to 1 and classified as `different_defaults` when their source origin is
`Finale27Default`. Before the concert-pitch correction, all 179 occurrences of
`keepWrittenOctaveInConcertPitch` compared the asserted false legacy behavior to a true companion value across every
epoch; that uniform contradiction is why the field now retains its true pinned default. **Confirmed.**

The broader all-corpus comparison also finds source-default disagreements for Show Repeats for Parts, Show Active
Layer Only, and the two Shape Designer dash values. These are classified as `different_defaults` only when the
source origin is `Finale27Default`; a recovered source value remains visible. Two Finale 2012-format documents
created by Finale 18.0.4 development build 5545 instead disagree on Consolidate Rests Across Layers, whose source
origin is `LegacyBehavior`. They are classified as `beta-discrepancy` only for a false-to-true disagreement whose
source version has development-status code 2, corresponding to the companion's `beta` marker. The classification
identifies the source status without asserting whether the beta build used the field for its eventual purpose or an
abandoned one; its stored companion value does not establish legacy source behavior. **Open.**
