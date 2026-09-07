# Measure

**Covers:** The `MS` measure record, its three word counts, the two readings of its measure flag
word, the compact part record the zlib epoch uses for an unlinked measure, and the members no
layout carries.
**Read when:** Working on measures, on measure-attached classes that key off them, or on any other
class that turns out to use a compact part record.
**Confidence:** `confirmed` for every member of the twelve- and thirteen-word layouts; `strong` for
the Coda-banner flag word; `weak` where stated below.

## The record

**Confirmed.** Measures are ordinary other records. The comparator is the 1-based measure number,
and musxdom expects those comparators to run sequentially from 1.

| Epoch | Identity | Addressing |
|---|---|---|
| Coda banner, uncompressed, DCL | fixed-row tag `MS` | six-word rows, one to three incidences |
| Zlib | class id `0x00b0` | 26-byte score payload, 8-byte compact part payload |

A six-word document keeps its display time signature in a **second record under the same
comparator**, fixed-row tag `ms`. See [The display time-signature record](#the-display-time-signature-record).

The public Finale 2000 PDK's `EDTMeasureSpec` is exactly the twelve-word form below; `MS` is an
`edSpecialOther`, so the API view need not match the stored record, but for every word the PDK
names it does. See [`../../reference/pdk_public_evidence.md`](../../reference/pdk_public_evidence.md).

## Three word counts, and how the reader tells them apart

**Confirmed.** The record has grown twice and never moved a word. **The layout is read from the
record's own shape rather than dated**, which is what [`../../reference/decoder_rules.md`](../../reference/decoder_rules.md)
asks for: the widest word stream any score measure of the document supplies selects one of three
layouts.

| Words | Releases | Adds |
|---:|---|---|
| 6 | Finale 1.0.0 – Finale 98 | the six words below |
| 12 | Finale 2000 – Finale 2004 | display time signature, new flag word, custom barline shapes, front space |
| 13 | Finale 2005 – Finale 2012 | back space |

The fixed-row epochs spend one 16-byte row per six words, so the same three layouts appear as one,
two, and three incidences; the third row is padded, and its last five words are zero in every
observed document. The zlib epoch coalesces the stream into one payload and stops at thirteen
words, which is what fixes the thirteenth as the last one this reader's window contains. Finale
2014 is outside that window; it is where the filler past word 12 became the key signature's
`keyless` and `hideKeySigShowAccis`.

| Slot | Member | Notes |
|---:|---|---|
| 0 | `width` | Evpu, signed |
| 1 | `globalKeySig->key` | linear keys below `0x4000`; low byte is the signed alteration |
| 2 | `beats` | count, or a `timesigUpper` comparator |
| 3 | `divBeat` | Edu per beat, or a `timesigLower` comparator |
| 4 | auxiliary flag word | below |
| 5 | measure flag word | below; **two readings** |
| 6 | `dispBeats` | |
| 7 | `dispDivbeat` | |
| 8 | new flag word | below |
| 9 | `customBarShape` | `ShapeDef` comparator |
| 10 | `customLeftBarShape` | `ShapeDef` comparator |
| 11 | `frontSpaceExtra` | Evpu, signed. The PDK names this word `CCCC`, unused. |
| 12 | `backSpaceExtra` | Evpu, signed |

## The auxiliary flag word

**Confirmed** for every bit below across 283,666 measures of the reference corpus compared against
their Finale 27 companions, except as noted. Every bit holds its position from Finale 1.0.0
through Finale 2012.

| Mask | musxdom member | PDK name |
|---:|---|---|
| `0x8000` | `breakWordExt` | — |
| `0x4000` | `hideCaution` | `MEAS_HIDECAUTION` |
| `0x2000` | `hasSmartShape` | `MEAS_SMARTSHAPEBIT` |
| `0x1000` | `groupBarlineOverride` | `MEAS_GRP_BARLINE_OVERRIDE` |
| `0x0800` | `showFullNames` | — |
| `0x0400` | `hasMeasNumbIndivPos` | `MEAS_MNSEPPLACE` |
| `0x0100` | `allowSplitPoints` | `MEAS_POSSPLIT` |
| `0x0080` | `compositeNumerator` | `MEAS_ALTNUMTSIG` |
| `0x0040` | `compositeDenominator` | `MEAS_ALTDENTSIG` |
| `0x0020` | `showKey` = `Never` | `MEAS_IGNOREKEY` |
| `0x0010` | `showTime` = `Never` | `MEAS_IGNORETIME` |
| `0x0008` | `evenlyAcrossMeasure` | `MEAS_INDIVPOSDEF` |
| `0x0007` | `positioningMode` | `MEAS_POSDEFBITS` |

Two of these are not universal.

`0x1000` is the group-barline override only from Finale 3.0. **Strong**: in the Coda-banner epoch
the bit instead means the measure draws no barline at all, and the reader maps it to
`BarlineType::None` there. 356 Finale 2.6 measures set it and every one of their companions omits
both `<barline>` and `<groupBarlineOverride>`; every Finale 3.7 and 97 measure that sets it has the
override and an ordinary barline beside it.

`0x0800` is `showFullNames` only **from Finale 2011**, which is where "Show Full Staff & Group
Names" arrives. The bit is older than the setting and meant something else before it: 44 Finale 3.5
measures set it and none of their companions shows full names. Nothing in the record distinguishes
the two readings, so this is a **version gate** -- the one the class has -- and a document whose
version cannot be recovered fails closed and reports the era's behavior.

### The composite time-signature comparators

**Confirmed** by a controlled Coda-banner pair, the only composite specimen in any
survey and the only evidence anywhere that exercises `divBeat` as a comparator.

`0x0080` and `0x0040` do not merely flag a composite time signature: they change what the two
time-signature words mean. With `0x0080` set, `beats` is a comparator into a `timesigUpper` list
rather than a beat count; with `0x0040` set, `divBeat` is a comparator into a `timesigLower` list
rather than an Edu value. It states both, one per measure:

| Measure | Words | Companion |
|---|---|---|
| 1 | `beats` 1, `divBeat` 512, aux `0x0081` | `<beats>1</beats> <divbeat>512</divbeat> <altNumTsig/>` |
| 2 | `beats` 2, `divBeat` 1, aux `0x0041` | `<beats>2</beats> <divbeat>1</divbeat> <altDenTsig/>` |

**Comparing these by number is not sound, and the Coda-banner epoch is where it breaks.** That
release's interface lets composite numerators and denominators be shared between measures and mixed
and matched independently, while later releases treat a composite pair as one unit belonging to a
particular time signature. **Believed:** the upgrade therefore resynthesizes the lists, leaving a
measure pointing at a differently numbered one. What is observed is only the disagreement itself --
twenty-one Coda-banner measures of the reference corpus differ from their companion on `beats` --
and the mechanism behind it has not been tested. A single-list document such as the fixture cannot
show it, because comparator 1 survives the upgrade unchanged.

The comparison defers both words as `awaits-dependent-recovery`, and **only where the measure's own
composite flag says the word is a comparator**: a plain beat count or Edu value that disagrees is a
decoding failure and stays visible. What they wait on is `others::TimeCompositeUpper` and
`others::TimeCompositeLower`. Once those are recovered, the comparison can resolve each side's
comparator through its own list and compare the time signatures rather than the numbers, which is
what settles these for good.

`positioningMode` is a code, not an ordinal: Finale's 0, 1, 2, 4 and 6 are musxdom's `Manual`,
`TimeSignature`, `BeatChart`, `TimeSigPlusPositioning` and `BeatChartPlusPositioning`. Code 3 is
reserved and code 5 is the PDK's `MEAS_POSareABSOLUT`, which musxdom has no value for; both fall
back to `Manual` while the report keeps the stored code.

## The measure flag word has two readings

**Confirmed** for Finale 3.0 and later. Bits 4 through 7 are a barline code there.

| Mask | musxdom member | PDK name |
|---:|---|---|
| `0x8000` | `beginNewSystem` | `MEAS_MS_LINEBREAK` |
| `0x4000` | `hasExpression` | `MEAS_DYNAMBIT` |
| `0x2000` | `breakMmRest` | `MEAS_BREAKREST` |
| `0x1000` | `noMeasNum` | `MEAS_RIOVERRIDE` |
| `0x0800` | `showKey` = `Always` | `MEAS_DELTAKEY` |
| `0x0400` | `showTime` = `Always` | `MEAS_DELTATIME` |
| `0x0200` | `hasOssia` | `MEAS_MS_ARBITMUSIC` |
| `0x0100` | `hasTextBlock` | `MEAS_MEASURETEXT` |
| `0x00f0` | `barlineType` | `MEAS_BARLINEBITS` |
| `0x0008` | `forwardRepeatBar` | `MEAS_FORREPBAR` |
| `0x0004` | `backwardsRepeatBar` | `MEAS_BACREPBAR` |
| `0x0002` | `hasEnding` | `MEAS_BARENDING` |
| `0x0001` | `hasTextRepeat` | `MEAS_REPEATS` |

`0x1000` is `noMeasNum`, **independently binary-verified**, and this corrects the PDK, which
describes the bit as "system use only (do not modify)". Every measure of the reference corpus that
sets it has `<noMeasNum/>` in its companion and no measure that clears it does.

**Strong: the Coda-banner epoch reads bits 4 through 7 differently**, and an epoch gate selects
between the two — the boundary is exactly the Coda-banner/uncompressed boundary, with every Finale
2.6 document on one side and every Finale 3.0 document on the other. Nothing in the record states
which reading applies, and the two are indistinguishable by shape, since both are one six-word row.

| Mask | Coda-banner meaning |
|---:|---|
| `0x0010` | `hasExpression` |
| `0x0020` | `barlineType` = `Double`, and `breakMmRest` |
| `0x0040` | `breakMmRest` |
| `0x0080` | `barlineType` = `Final`, and `breakMmRest` |

Every other bit of the word keeps the position the later reading gives it, which is **weak** for
the four repeat bits and the always-show-key bit: no Coda-banner document in any survey sets any of
them. The two bits the later word uses for `hasExpression` and `breakMmRest` are deliberately not
read in this epoch, because it demonstrably spells both elsewhere.

Finale's musx conversion additionally writes `breakWordExt` from the barline, where the source
leaves the bit clear. **This reader declines to reproduce that** and reports what the file says; the
comparison classifies the difference as `finale-upgrade-loss`. It is the class's largest, some
2,800 measures across 346 documents and every epoch, always with the source bit clear and the
companion's set.

The trigger is the barline: a **double, final or solid** barline, or a **backwards repeat**.
Selecting any of them in the modern interface checks "Barline ends word extensions" and selecting
anything else clears it, so only a deliberate edit afterwards separates the two.

Two controlled fixtures fix both ends of this, and neither could have done it alone.
`tests/evidence/F2002/F2002-breakwexts.mus` is a pre-2004 document whose five measures leave the
bit clear throughout: its companion sets `breakWordExt` on the double and final measures and on
neither normal one, and its fifth measure carries no lyrics at all, so the conversion keys off the
barline rather than off anything it is converting.
`tests/evidence/F2005/F2005-breakwexts.mus` is the same design in the first release where the
setting exists, and there the companion follows the **bit** and never the barline: a double barline
with the bit clear stays clear. The reader's mapping of `0x8000` is confirmed in both directions by
its measures 2 and 4.

**Not fully characterized.** `mus-07c6c32c34aa71cf`, a Finale 2011 document authored in Finale
2009, carries a backwards repeat with the bit clear and its companion sets the flag anyway — which
the post-2004 half of the account above does not predict. Whether the repeat trigger is
version-independent while the barline-type trigger is not was not pursued.

## The new flag word

**Confirmed**, and present only from twelve words.

| Mask | musxdom member | PDK name |
|---:|---|---|
| `0x0800` | `pageBreak` | — |
| `0x0008` | `hasChord` | — (PDK: "0x0008 is available") |
| `0x0200` | `compositeDispDenominator` | `MEAS_DISPLAY_ALTDENTSIG` |
| `0x0100` | `compositeDispNumerator` | `MEAS_DISPLAY_ALTNUMTSIG` |
| `0x00f0` | `leftBarlineType` | `MEAS_LEFT_BARLINEBITS` |
| `0x0004` | `useDisplayTimesig` | `MEAS_USE_DISPLAY_TIMESIG` |
| `0x0002` | `abbrvTime` | `MEAS_ABBRVTIME` |
| `0x0001` | — | `MEAS_PARENTIME`, unimplemented |

`0x0800` is `pageBreak` and `0x0008` is `hasChord`, both **independently binary-verified**. The
PDK names nothing at `0x0800` and marks `0x0008` explicitly as available, so the chord flag took a
bit that was reserved when that PDK was published.

`tests/evidence/F2012/F2012-chord.mus` fixes the chord bit: it differs from `F2012-baseline.mus`
by one chord, and the whole difference in the measure record is the new flag word moving from
`0x00f0` to `0x00f8`.

## Barline codes

**Confirmed** for codes 0 through 6, which are the PDK's `BARLINE_*` values, and for 15.
musxdom's enum reorders them and adds two values the PDK does not name.

| Code | musxdom | Code | musxdom |
|---:|---|---:|---|
| 0 | `None` | 4 | `Solid` |
| 1 | `Normal` | 5 | `Final` |
| 2 | `Double` | 6 | `Tick` |
| 3 | `Dashed` | 7 | `Custom` (**believed**) |
| | | 15 | `OptionsDefault` |

Code 7 is **believed**: it is the only remaining musxdom value, and `customBarShape` sits beside
the code for it to select, but no surveyed document uses either. Code 15 appears in the left
barline only; it is how a measure says to take the type from
[`../options/barline_options.md`](../options/barline_options.md), and it is what every
twelve-word-or-longer document in the reference corpus stores there.

## Comparator zero is not a measure

**Confirmed** on two reference-corpus documents, `mus-e09f13d06ff0f25e` and
`mus-4c2fe14dd692de72`. Some documents carry an `MS` record at comparator 0 holding a zero width,
key and time signature. The bytes are identical in unrelated documents, so it is a fixed artifact
rather than content, and Finale discards it on upgrade.

**The reader does not build a measure from it**, and logs one verbose diagnostic instead. musxdom
numbers measures from 1 and checks that a document's comparators run sequentially from there, so
importing the record would put an object where there is no place for one *and* make every real
measure of that document fail the check: a 47-measure document produced 48 warnings, and the
corpus-wide count of that message was 320. It is mild corruption that Finale tolerates, and this
reader tolerates it the same way.

The record is also excluded from the word count that selects the layout, so a stray two-incidence
artifact in a six-word document cannot promote it to the later layout.

**Gaps in the comparator sequence** are a related condition and are deliberately not handled. None
has been observed. Were one to occur, musxdom's own check reports it through the same channel that
exposed the comparator-zero records, so nothing here needs to detect it; and closing a gap by
renumbering would invent comparators the source does not have.

## The display time-signature record

**Confirmed** across 57 documents and 1,629 measures of the reference corpus, every one agreeing
with its companion. The six-word layout has no word for a display time signature, and Finale 3.0
introduced the feature, so the era keeps it in a separate `ms` record whose comparator is the
measure number.

**`ms` mirrors `MS`.** The display beats and divisions occupy the slots the measure record gives
the actual time signature, and the auxiliary word carries the composite bits at the same positions:

| Slot | Member | Same slot in `MS` |
|---:|---|---|
| 2 | `dispBeats` | `beats` |
| 3 | `dispDivbeat` | `divBeat` |
| 4 | `0x0080` `compositeDispNumerator`, `0x0040` `compositeDispDenominator` | the auxiliary flag word's own composite bits |

**The record's presence is what sets `useDisplayTimesig`**: all 1,629 measures with an `ms` record
have it in their companion and no measure without one does. Slots 0, 1 and 5 are zero in every
observed record. The individual attribution of `0x0080` to the numerator and `0x0040` to the
denominator is **believed** rather than observed, since the only value seen is `0x00c0` with both
companion flags set; it follows the `MS` layout the rest of the record mirrors.

`tests/evidence/F97/F97-disptime.mus` is the controlled specimen and the only fixture in any survey
that carries the record: it differs from `Fin97-baseline.mus` by one display time signature, its
measure record is unchanged, and the whole difference is one `ms` record holding `[0, 0, 3, 1536,
0, 0]` against a companion reading `<dispBeats>3</dispBeats> <dispDivbeat>1536</dispDivbeat>
<useDisplayTimesig/>`.

No document of the era abbreviates a display time signature, so `abbrvTime` remains the one member
of the six-word layout whose location is unknown.

## What a six-word document does not carry

A six-word record has no seventh word onward, and the members those words carry divide in two.

Era behavior, reported as `LegacyBehavior`: `leftBarlineType` is `OptionsDefault`, which is what a
later record's code 15 spells and what every six-word document's companion shows;
`customBarShape`, `customLeftBarShape`, `pageBreak`, `frontSpaceExtra` and `backSpaceExtra` are
absent features and take zero.

The display time signature is recovered from the `ms` record above, so a measure without that
record has none, reported as era behavior. The chord flag is era behavior here too, for the reason
given below. So is `abbrvTime`: no layout without the new flag word has a per-measure abbreviation
at all, because the era abbreviates a time signature by the document-wide setting in
[`../options/time_signature_options.md`](../options/time_signature_options.md) rather than measure
by measure.

**Nothing in this class is unmapped.** Every member is either recovered from a record or supplied
as the era's behavior, in every layout.

## The compact part record

**Strong.** A zlib-era part that unlinks a measure stores an 8-byte record beside the score's
26-byte one. The general mechanism, and how the reader tells a compact part record from a
standalone one, is in [`../container/sharing.md`](../container/sharing.md); `measSpec` is that
mechanism's first user, and it is the only class known to use it rather than the same-sized
continuation mask every other part-scoped class uses.

The record holds the four values a part may differ in, and its flag word is its own rather than a
copy of either flag word of the score layout.

| Slot | Member |
|---:|---|
| 0 | `width` |
| 1 | flag word: `0x0007` is `positioningMode`, `0x4000` is `pageBreak` |
| 2 | `frontSpaceExtra` |
| 3 | `backSpaceExtra` |

The page-break bit is at `0x4000` here, where the score record's auxiliary word spells
`hideCaution`, and the score record's own page break is at `0x0800` of the new flag word. No other
bit of the compact flag word is set in any surveyed document, so the rest is `open`.

Every other member is the score measure's, which is what partial linkage means: the reader
initializes the part object from the score object through musxdom's own
`PartSharingFactory::initializePartial` before overlaying the four, and an inherited member keeps
the score record's provenance and offsets in the report.

### Part measures Finale 27 creates on upgrade

Finale 27 gives every measure of every linked part an instance whether or not the legacy file has a
record for it, so a companion can hold thousands of part measures where the source holds none.
Seventeen Finale 2007 and 2008 documents of the reference corpus account for 954,580 such leaves.

**The reader does not create them, and should not.** musxdom resolves a part request with no part
object to the score object, so the two documents state the same thing, and inventing the objects
would make an absent part record indistinguishable from one this reader failed to read.

The comparison drops them instead, counted as `finale-materialized-part-measure`, and **only where
the companion's part measure states exactly what its own score measure states**. A part measure
Finale re-laid out — carrying its own width or positioning mode — differs from its score measure
and stays in the comparison, because those values are Finale's own and no record in the source
states them. In the one document measured, that leaves about a fifth of the instances visible.

## Members no measure record carries

`hasChord` is recovered from the new flag word above wherever that word exists. The six-word layout
has none, and the reason is not that the flag was lost: **that era keeps chords as entry details
rather than measure ones**, so the measure record has nothing to say about them. False is what it
implies, reported as the era's behavior. The Coda-banner pair shows exactly this —
`tests/evidence/F100/F100-chord.mus` and `F100-chord2.mus` carry **identical** measure words while
only the second's companion has `<hasChord/>`, because in that era the flag has nowhere to live.

The member is nonetheless **deferred in the coverage comparison for every layout, not just that
one**: the bit is a cache, and once `ChordAssign` is recovered the flag is re-derived from the
chord records themselves for all eras, including the ones whose bit this reader reads.

Like the other presence flags the bit is a cache, and the record that actually holds the chord is
an **entry** detail. `CH` (`0x4348`) appears in the Coda-banner details pool with its two
comparator fields holding the halves of a 32-bit entry number rather than a staff and a measure:
`F100-chord2.mus` reads cmper1 0, cmper2 1 for entry 1, while the companion's `chordAssign` is
keyed cmper1 1, cmper2 1 for staff 1, measure 1. **Believed** for the high/low split, which an
entry number below 65536 cannot distinguish. So a stale bit can only be corrected once entries are
recovered, which is the term on which the coverage comparison defers the member.

`globalKeySig->keyless` and `globalKeySig->hideKeySigShowAccis` are **era behavior**, both false.
Finale 2014 built them out of words that are filler through Finale 2012, so no document this reader
accepts can carry either.

One case makes a companion disagree anyway, and it is not a recovery failure. A later Finale can
write a document out in the Finale 2012 format, and the header then records two versions: the
creator names the release that made the document and the last saver the format on disk. Where the
creator postdates Finale 2012 **and its development status is beta**, it held members the older
layout has nowhere to put, and Finale 27 reads the file knowing what made it. The comparison
classifies that as `beta-discrepancy`, on those two members only and only under both conditions.
`mus-0c3c7a4332e692d5` of the reference corpus is the specimen: created 2015-12-14 by internal major
18 with Enigma and file status `beta`, back-saved 2016-12-14 as major 17, banner `Finale(R) 2012
File Converter`. Its companion sets `hideKeySigShowAccis` on all 214 measures while the key word is
`0x0000` in every record and every payload is 26 bytes, so the value is not in the source at all.

## Upgrade behavior to expect from a companion

Finale 27 recomputes `hasExpression`, `hasSmartShape`, `hasTextBlock` and `hasChord` from the
objects actually attached to a measure rather than trusting the stored bit. The stored bit can
therefore be stale in either direction, and the reader reports what the source says.

Neither direction is answerable while the objects are unrecovered. A clear source bit against a
set companion one is a value the reader owes; a set one against a clear companion one is a cache
the reader cannot yet know is stale, having nothing to check it against. Both are therefore
classified `awaits-dependent-recovery`, from the deferred table in
`tools/coverage/classification_rules.cpp`:

| Member | Waits on |
|---|---|
| `hasSmartShape` | `others::SmartShape` |
| `hasExpression` | `others::MeasureExprAssign` |
| `hasTextBlock` | `details::MeasureTextAssign` |
| `hasOssia` | `details::MeasureOssiaAssign` |
| `hasChord` | the chord records |

A row leaves the table when its class lands and the reader derives the member from the objects
themselves; when the table is empty the rule goes with it. The table is the whole of the deferral,
so deleting a row makes its differences unexpected again and nothing else changes.
`recovery_coverage_probe --strict-deferred` does that for every row at once, which is how the
outstanding work is counted rather than read past.

Across the tracked survey this produces one difference, on
`tests/evidence/F263/F263-curve-opt-3.mus`, where the companion sets `hasSmartShape` and the record
does not. The reference corpus holds far more: 46,624 measures whose companion sets
`hasExpression` against a clear record bit, and 6,483 for `hasSmartShape`. The experiment behind all of the above is in
[`../../investigations/measure.md`](../../investigations/measure.md).
