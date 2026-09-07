# Measure investigations

**Covers:** How the `MS` layouts, flag-word readings, and compact part record were established,
including the predictions that were refuted along the way.
**Read when:** A measure field is labeled `weak` or `open`, a companion comparison disagrees, or
you are about to propose a new reading of either flag word.
**Confidence:** method and counts as stated; conclusions live in
[`../format/others/measure.md`](../format/others/measure.md).

## 2026-09-06 — Locating the layouts

**Question.** Which words does an `MS` record hold, and where does the layout change?

**Method.** `tools/record_dump` over one fixture per release directory in `tests/evidence/`,
counting incidences and payload bytes, with each fixture's Finale 27 companion decoded and its
`<measSpec>` elements read beside the words.

**Result.** One incidence through Finale 98, two from Finale 2000, three from Finale 2005, and a
fixed 26-byte class record in the zlib epoch. The zlib payload holds thirteen words, which is what
bounds the third fixed row: its last five words are padding. The twelve-word form matches the
public Finale 2000 PDK's `EDTMeasureSpec` word for word, including the order of `dispBeats`,
`dispDivbeat`, the new flag word and the two custom barline shapes.

**Prediction refuted.** The PDK names word 11 `CCCC`, unused, and word 12 does not exist there, so
the first reading was that Finale 2005 added a single flag word and that the front and back space
lived elsewhere. `mus-c2cf68bd12bf7532` of the reference corpus disproves it directly: word 11
holds -22 on measures 9 and 10, whose companions carry `<frontSpaceExtra>-22</frontSpaceExtra>`,
and word 12 holds -9 on measure 27, whose companion carries `<backSpaceExtra>-9</backSpaceExtra>`.
The PDK's "unused" describes its API view of an `edSpecialOther`, not the stored record.

## 2026-09-06 — The whole-corpus field check

**Question.** Does one field map explain every measure of every era?

**Method.** A disposable Python model of the proposed map decoded every score `MS` record of the
1,193 companion-backed sources of `rpatters1-main` and compared each decoded member with the
`<measSpec>` element of its Finale 27 companion, per field. 283,666 measures.

**Result.** Thirty-seven of forty fields disagreed nowhere. The disagreements were confined and
each turned out to be a real finding rather than a decoding error:

- `barline` 820, `groupBarlineOverride` 356, `hasExpr` 3,259 and `breakWordExt` 2,538, all in
  Finale 2.6, 3.0 and 3.5 documents — the two flag-word findings below.
- `showFullNames` 44, all in one Finale 3.5 document.
- `beats` 21, in Finale 2.6 documents whose companion states a different time signature than the
  record; not investigated further, and small enough to be measures whose signature Finale 27
  recomputes from an independent time signature elsewhere.
- 1,629 six-word measures whose companion carries a display time signature the record cannot hold.

**Prediction refuted.** The first reading of the six-word era took the Coda-banner epoch to differ
from Finale 3.x only in that the barline was not stored, on the strength of
`tests/evidence/F100/F100-instlist.mus`, whose three measures carry flag words 0, 0 and `0x0010`
while all three companions show a normal barline. That is right about Finale 1.0.0 and wrong about
where the boundary is: Finale 3.0 and 3.5 use the later reading, and the boundary is the epoch.

## 2026-09-06 — The Coda-banner flag word

**Question.** What do bits 4 through 7 mean in the epoch where the later barline code does not
explain the companions?

**Method.** Cross-tabulated the flag word's bits 4-7 and the auxiliary word's bit 12 against each
companion field, per saving product, over every six-word document of `rpatters1-main`.

**Result.** For Finale 2.6, 13,706 measures:

| Bits 4-7 | Companion barline | Companion `breakRest` | Companion `breakWordExt` |
|---:|---|---|---|
| 0 | normal 6,655 | none | none |
| 1 | normal 6,199 | none | none |
| 2 | double 162 | all | all |
| 3 | double 272 | all | all |
| 4 | normal 14 | all | none |
| 5 | normal 18 | all | none |
| 12 | final 11 | all | all |
| 13 | final 19 | all | all |

Bit 4 changes nothing in this table and is `hasExpression`: `F100-instlist.mus` measure 3 sets it
alone and is the only one of its eleven measures whose companion carries `<hasExpr/>`. Bit 5 is a
double barline, bit 6 a multimeasure-rest break, bit 7 a final barline. Auxiliary bit 12 accounts
for the barline disagreements separately: 356 Finale 2.6 measures set it and every companion omits
`<barline>` entirely.

For Finale 3.0 (211 measures) and 3.5 (395), bits 4-7 are the later barline code — 1 normal,
2 double, 5 final — which is what places the boundary at the epoch rather than at Finale 3.7.

**Superseded by the all-corpus capture below.** This entry first read the word-extension break as a
Coda-era pairing with the double and final barline bits. It is not era-specific.

## 2026-09-06 — The compact part record

**Question.** How does an unlinked part state a measure?

**Method.** `tools/record_dump` was given a `--part` filter and made to enumerate every source part
a tag carries, rather than the score alone. Payload sizes were then counted over 150
companion-backed zlib sources of `rpatters1-main`, and each part record's words compared with the
`part=`-attributed `<measSpec>` of its companion.

**Result.** 23,055 score records at 26 bytes and 40,384 part records at 8 bytes; no other size.
The four compact words are width, a flag word, the front space and the back space.
`mus-baa69cec632f4b4f` measure 112 fixes the flag word's page break: parts 1 and 2 store
`0x4006` and their companions carry `<width>` and `<pageBreak/>` and nothing else, while the score
record's own page break is at `0x0800` of the new flag word and its auxiliary `0x4000` is
`hideCaution`. The low three bits are the positioning mode, which the companion omits whenever it
equals the score's and states whenever it does not.

**Prediction refuted.** The flag word was first read as a copy of the score record's auxiliary
word, since its low bits agree. Measure 112 disproves it: the score's auxiliary word is `0x2006`
and the part's is `0x4006`, so that reading would give the part `hideCaution` and take away
`hasSmartShape`, and the companion shows neither change.

## 2026-09-06 — `hasChord` is not in the record

**Question.** Which bit carries `hasChord`?

**Method.** Compared measures with and without `<hasChord/>` in one reference-corpus document whose
chords are confined to part of the score.

**Result.** None. Measures 1-3 and 10-18 of `mus-453786316898a024` have
identical auxiliary, measure and new flag words, and only the second group's companions carry
`<hasChord/>`. Finale 27 recomputes the flag from the chord assignments it finds. The same is true
of `hasExpression`, `hasSmartShape` and `hasTextBlock`, which the corpus check saw the companion
set 46,624 and 6,483 times respectively where the record leaves the bit clear. `hasChord` is
reported `Unmapped` rather than as era behavior, because the chord records exist and this reader
has not reached them.

## 2026-09-06 — The controlled chord pair, and why `hasChord` waits on entries

**Question.** Which record does `hasChord` track, and can a measure importer reach it?

**Method.** Three controlled fixtures were added: `tests/evidence/F100/F100-chord2.mus` and its
upgrades `F263/F263-F100-chord2.mus` and `F372/F372-F263-F100-chord2.mus`, each with an ETF export
and a Finale 27 companion. `F100-chord2.mus` differs from the existing `F100-chord.mus` by one
chord. Both files' record pools were dumped and compared identity by identity.

**Result.** The two measure records are byte-identical -- `[600, 0, 4, 1024, 1, 0]` in both -- and
only `F100-chord2.mus`'s companion carries `<hasChord/>`. The only difference between the files is
that `F100-chord2.mus` has a details pool at all, holding one `CH` record (`0x4348`) beside the
`GF` frame holder. The same `CH` record survives unchanged into the Finale 2.6.3 upgrade and into
Finale 3.7.2, where its first payload word changes from `0x0082` to `0x2042`.

**Prediction refuted, before it was tested.** The reading about to be validated was that `CH` is a
measure-keyed detail like `GF`, so that a measure could set `hasChord` when a `CH` record carried
its comparator. `CH` is an **entry** detail. Its comparator fields are the halves of a 32-bit entry
number: the fixture reads cmper1 0, cmper2 1 for entry 1, against a companion `chordAssign` keyed
staff 1, measure 1. `GF` in the same pool reads cmper1 1, cmper2 1 and is genuinely (staff,
measure), which is what made the coincidence look like a pattern -- in a one-measure, one-staff,
one-entry document every candidate key is 1.

The consequence is that no measure importer can resolve `hasChord`: reaching the chord means
resolving an entry number to its measure, which is entry recovery. The member stays `Unmapped` and
its deferral names entry recovery rather than a pooled class.

## 2026-09-06 — All-corpus capture

**Question.** Does the implementation hold across every registered survey?

**Method.** One authorized `recovery_coverage_probe` capture from the instrumented Release tree
over a manifest of all three generated corpus TSVs, with Finale's `MacSymbolFonts.txt` supplied.
16,328 documents; 16,239 imported; 4,830 had a companion and all 4,830 compared.

**Result.** 32,448,153 measure leaves agree. 159,261 are the deferred presence flags. 8,466 are
unexpected, and they fall into four groups:

| Leaf | Count | Epochs | Reading |
|---|---:|---|---|
| `break_word_ext` | 2,703 | all four | below |
| `disp_beats`, `disp_divbeat`, `use_display_timesig`, `composite_disp_*` | 1,027 | uncompressed | the six-word display time signature, C12 |
| `beats` | 21 | coda-banner | not investigated; a companion time signature the record does not state |
| `global_key_sig.hide_key_sig_show_accis` | 214 | zlib | one back-saved document, below |

Every one of the 2,703 `break_word_ext` differences has the source bit clear and the companion's
set, in 346 documents spread over all four epochs, which refutes the Coda-specific reading above.
`mus-07c6c32c34aa71cf` isolates the rule: measures 2 and 5 carry a backward repeat and the
companion sets the flag; measures 3 and 6 differ only in lacking the repeat and it does not. The
auxiliary word's own bit is clear in all four.

**No other class in the reader has an unexpected difference anywhere in the capture**: the
whole-corpus unexpected total is 8,466 and `measures` accounts for all of it.

## 2026-09-06 — Finale 27 materializes part measures

**Question.** Why do 17 documents show 954,580 companion-only measure leaves?

**Method.** Counted `<measSpec>` elements per part in the companion of
`mus-248e9fccc4d22bad` (Finale 2007) against the source's class `0x00b0` records.

**Result.** The source stores 153 score records and no part records at all. The companion holds
2,601 elements: 153 for the score and 153 for each of 16 linked parts. Of the 2,448 part elements,
1,989 are empty `shared="true"` objects carrying nothing, and 459 carry only `width` and `posMode`
from Finale's own re-layout on upgrade.

So the upgrade creates a part instance for every measure of every linked part whether or not the
legacy file has one. The reader creates only what the source stores, and musxdom resolves a part
request with no part object to the score object, so the two documents are semantically equal. All
17 affected documents are Finale 2007 or 2008. **This is a report alignment question, not a reader
defect.**

**Resolved 2026-09-06.** The comparison drops a companion part measure the reader does not have
when it states exactly what its own score measure states, counted as
`finale-materialized-part-measure`. Note the elements do not arrive empty: musxdom's factory runs
`initializePartial`, so an empty `shared="true"` element becomes a full copy of the score measure
in the snapshot, which is what makes the equivalence easy to state.

The 459 instances carrying a re-laid-out width and positioning mode differ from their score measure
and are deliberately left visible. They are Finale's own values, computed during the upgrade, and
no record in the source states them, so the reader can never supply them -- which is a fact worth
seeing rather than silencing.

## 2026-09-06 — The key-signature switch a beta back-save carries

**Question.** One Finale 2012 document's companion sets `hideKeySigShowAccis` on every measure.
Where does the value come from?

**Method.** Decoded the file's two header version blocks and compared them with the `<created>`
block its companion preserves, then read every measure record.

**Result.** `mus-0c3c7a4332e692d5` is a back-save. Its banner is `Finale(R) 2012 File Converter`,
the documented back-save spelling; the creator block is internal major 18 dated 2015-12-14 on
`MAC`, and the last-saver block is major 17 dated 2016-12-14 on `WIN`. The value is not in the
source: the key word is `0x0000` in all 214 records and every payload is 26 bytes, so there is no
fourteenth word.

The companion's preserved `<created>` block names the development status in words —
`enigmaVersion` and `fileVersion` `beta`, `appVersion` `dev` build 5545 — which pins the numeric
codes this project had recorded as unmapped: **2 is `beta` and 0 is `dev`**. `4` is still
unconfirmed and is presumably `release`.

**Correction to the method.** The first reading of the numeric code was that `devStatus == 2`
could not mean beta because 1,301 of the reference corpus's 3,981 files carry it. That inference
was wrong: frequency in one corpus says nothing about a code's meaning, and the corpus owner ran
beta builds while writing plug-in test files.

A note on the packing: the companion reports `appVersion` build 5545 where the stored field reads
`0xff`. The build field is eight bits, so a dev build above 255 saturates and the stored value is
lossy.

## 2026-09-06 — The chord bit, located by a controlled zlib pair

**Question.** Does any word of the measure record carry `hasChord` after all?

**Method.** `tests/evidence/F2012/F2012-chord.mus` was saved from `F2012-baseline.mus` by adding one
chord. Both measure records were dumped and compared byte for byte.

**Result.** They differ in one nibble. The new flag word moves from `0x00f0` to `0x00f8`, so
**`hasChord` is `0x0008` of the new flag word**, next to `useDisplayTimesig` at `0x0004`. The
public Finale 2000 PDK marks that exact bit `// 0x0008 is available`, so the flag took a reserved
bit after that PDK was published. The companions agree: the baseline has no `chordAssign` and no
`<hasChord/>`, the edit has one of each.

**Correction.** Two earlier readings here were wrong, and both failed the same way -- by inferring
absence from a corpus that could not show presence.

The first was that no bit carries the member at all. That rested on the Coda-banner pair, whose
records are six words and therefore have no new flag word to differ in, and on one Finale 2012
document whose flag words match across measures that differ in `<hasChord/>`. The second reading,
proposed while testing `0x8000` of the same word, was that a corpus scan settles it: 19 zlib
documents with chords, ~1,700 measures, and no bit of any of the thirteen words tracking the
companion. Both were measurements of documents whose stored bit is stale -- the flag is a cache
that Finale 27 recomputes, so a real-world document proves nothing about where the bit lives. Only
a one-variable save does, which is what this fixture is.

The reference corpus does not contain the evidence: of 408 companion-backed DCL documents, **none
has a single chord**, and the 19 zlib documents that do have one all carry the bit clear.

## 2026-09-06 — The six-word display time signature

**Question.** Where does a six-word document keep a display time signature, given 1,629 such
measures have one in their companion and the record has no word for it?

**Method.** `tests/evidence/F97/F97-disptime.mus` was saved from `Fin97-baseline.mus` by giving one
measure a display time signature. Both files' whole record pools were dumped and compared identity
by identity, ignoring offsets.

**Result.** The measure records are byte-identical. The entire difference is one new record:
fixed-row tag **`ms`**, comparator 1, words `[0, 0, 3, 1536, 0, 0]`, against a companion reading
`<dispBeats>3</dispBeats> <dispDivbeat>1536</dispDivbeat> <useDisplayTimesig/>`.

`ms` mirrors `MS`: the display beats and divisions sit in slots 2 and 3, where the measure record
holds the actual ones. Checked against the reference corpus, 57 documents and 1,629 measures agree
exactly -- slot 2 matches `dispBeats` and slot 3 `dispDivbeat` in every case, and every measure with
the record has `<useDisplayTimesig/>` while no measure without one does. Slot 4 is the auxiliary
word: it reads `0x00c0` on 41 measures and every one of those has both `displayAltNumTsig` and
`displayAltDenTsig`, which are the same `0x0080` and `0x0040` bits `MS` uses for its own composite
flags. Slots 0, 1 and 5 are zero throughout.

This closes the gap the earlier corpus check opened, and the count matches it exactly: the 1,629
measures whose display time signature could not be explained are the 1,629 that have an `ms`
record.

**Closed.** `abbrvTime` was briefly carried here as unmapped, on the reasoning that slot 5 would be
its natural home by the mirror and nothing had been observed there. That was the wrong question:
the era has no per-measure abbreviation to store. A time signature is abbreviated by the
document-wide setting, so slot 5 being zero is not a gap and the member is the era's behavior.

## 2026-09-06 — The full-names bit is gated on Finale 2011

"Show Full Staff & Group Names" arrives in Finale 2011, so the auxiliary word's `0x0800` is read
only from that release. The bit is older than the setting: 44 Finale 3.5 measures set it and none
of their companions shows full names.

The earlier gate here was structural -- read the bit once the record has twelve words -- and it was
wrong in the safe direction rather than the unsafe one. It would have taken the bit as the setting
for every release from Finale 2000 through Finale 2010, where its meaning is unknown. The corpus
did not catch that, because no document of those releases sets it: the check that reported
`showFullNames` correct across 283,666 measures was confirming agreement on a bit that is clear
almost everywhere, and its only disagreement was the Finale 3.5 file.

**A version gate, not a marker.** The word is the same width in every layout that has it and no
field states which reading applies, so nothing in the record can decide. A document whose version
cannot be recovered fails closed and reports the era's behavior.

## 2026-09-07 — The composite time-signature comparators

**Question.** Twenty-one Coda-banner measures report a `beats` their companion disagrees with. Is
the word being decoded wrongly?

**Method.** A controlled Finale 2.6.3 pair gives one measure a composite numerator and another a
composite denominator, against that release's baseline. Records and companion compared directly.

**Result.** No, the word is right, and the flags decide what it means. Measure 1 holds `beats` 1,
`divBeat` 512, aux `0x0081`; measure 2 holds `beats` 2, `divBeat` 1, aux `0x0041`. The companion
agrees with both exactly, `altNumTsig` and `altDenTsig` respectively, and keeps its `timeUpper` and
`timeLower` lists at comparator 1. So `0x0080` makes `beats` a `timesigUpper` comparator and
`0x0040` makes `divBeat` a `timesigLower` one.

**Prediction refuted.** The reading proposed before this fixture existed was that Finale 27
renumbers the composite lists on upgrade, which would have made every such comparison meaningless.
It does not, at least where a document has one list: the pair's comparator 1 survives on both sides and the
reader scores no difference on it.

**The likely cause, and why it is not being pursued.** The Coda-banner interface lets a composite
numerator and a composite denominator be shared across measures and combined freely, while later
releases bind a composite pair to one time signature. **Believed:** the upgrade resynthesizes the
lists on that account, leaving a measure pointing at a differently numbered one. Nothing here tests
that -- a document with two distinct composite numerators would -- so the mechanism is a working
explanation for an observed disagreement, not an established one. `mus-711f5298068dce1b` measure 31 is the
shape: aux
`0x0086`, source `beats` 1, companion 2, with `divBeat` agreeing because that half is not composite
there. A discriminating fixture would need two distinct composite numerators in one Coda document.

The consequence is recorded rather than chased: the twenty-one differences stay visible, and a
comparison that wants to settle them must resolve each side's comparator through its own list and
compare the resulting time signatures.

## 2026-09-07 — The comparator-zero record

**Question.** Two Finale 2004b documents hold a measure the companion lacks. Is the reader building
something the source does not have?

**Method.** Compared the `MS` comparators of each document with its companion's `measSpec`
comparators, then read the odd record.

**Result.** Neither document has a gap. Both run 0 through their last measure while the companion
runs 1 through the same, so the extra object is a record at comparator **0**. It holds
`[0, 0, 0, 0, 0x0400, 0]` and `[0, 0, 0x00f0, 0, 0, 0]` in both files -- identical bytes in
unrelated documents, with a zero width, key and time signature.

The 44 leaves each contributed to the comparison were the visible part. The real cost was that
musxdom numbers measures from 1: a comparator-zero object shifts every real measure off its
expected index, so a 47-measure document logged 48 sequence warnings and a 17-measure document 18.
The corpus-wide count of that message is 320, so more documents carry one than the two with
companions that surfaced it.

The reader now skips the record and logs a verbose diagnostic. The level is deliberate: the
condition does not affect what the document can be used for, and Finale itself tolerates and
discards it.
