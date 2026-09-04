# ClefOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Clef definitions

**Confirmed for the collection and its fields; three version boundaries are established and one is inferred.**
Clef definitions are an ordinary numeric global, not a record type of their own. There is no `cf` tag or
comparator anywhere in the corpus: a search of the others, details, and class pools of specimens spanning
Finale 1.8.7 through 2012 found none, and the identity is the numeric selector in every era.

The collection changed size twice and the tuple once. Counts below are distinct corpus files:

| Era | Identity | Layout | Definitions | Files |
|---|---|---|---|---:|
| Finale 1.8.7–2.6 | selectors `28`–`35`, comparator `65534` | one 6-word row each | 8 | 63 |
| Finale 3.0–2000 | the same eight selectors | one 6-word row each | 8 | 208 |
| Finale 2001–2002 | selector `95`, 24 incidences | 9-word tuples, streamed across rows | 16 | 67 |
| Finale 2003–2006 | selector `95`, 27 incidences | 9-word tuples | 18 | 403 |
| Finale 2007–2010 | class `0x006d`, 324 bytes | 9-word tuples | 18 | 292 |
| Finale 2012 | class `0x006d`, 360 bytes | 10-word tuples | 18 | 235 |

Selector `36` is the tuplet font, so eight is a ceiling the record vocabulary itself imposes on the early eras
rather than a guess. The class id follows the established `numericGlobalClass` rule, `95 + 0x0e`. Unlike the
default-font array there is no structural zero fill: 16 and 18 nine-word tuples occupy exactly 24 and 27 rows.

The pre-2001 six-word record is a different layout, not a short tuple:

| Word | Field |
|---:|---|
| 0 | `middleCPos` (`adjust`) |
| 1 | **open**: a per-clef value the Coda era populates and Finale 3.0 stops writing |
| 2 | `clefChar`, one byte |
| 3 | `staffPosition` (`clefYDisp`) |
| 4 | baseline adjustment, in harmonic levels |
| 5 | **open** |

Word 1 holds `6, 0, -2, -6, 6, -1, -13, -4` across selectors 28 through 35 in the Coda era and zero in almost every
Finale 3.0 and later file, six of which retain inherited values. The controlled baseline edits leave it untouched,
so it is not the baseline adjustment and nothing is mapped from it.

The tuple, with the Finale 2012 slot in parentheses where it differs:

| Word | Field | Notes |
|---:|---|---|
| 0 | `middleCPos` (`adjust`) | |
| 1 (1–2) | `clefChar` | one word until Finale 2012, then a long |
| 2 (3) | `staffPosition` (`clefYDisp`) | |
| 3 (4) | baseline adjustment, in Efix | signed 16-bit; see below |
| 4 (5) | `shapeId` | non-zero only at indices 16 and 17 |
| 5–7 (6–8) | `fontId`, `fontSize`, effects | present only when the own-font bit is set |
| 8 (9) | flags | bit 0 `isShape`, bit 1 `useOwnFont`, bit 2 `scaleToStaffHeight` |

Two decoding rules are needed. Before Finale 2012 the clef character is a single byte of a symbol font stored in a
word, and a source may store it either zero-extended or sign-extended: character 139 appears as `0x008b` in some
files and `0xff8b` in others, so it must be narrowed to its low byte. From Finale 2012 it is a long, which is what
the tuple's two extra bytes are; MakeMusic's release notes for that version give Unicode text support as a headline
feature, which is consistent with the widening and places the boundary at 2012 rather than earlier. No Finale 2011
specimen exists in any surveyed corpus, so the reader treats 2011 as narrow and that half of the boundary is
**open**. No big-endian Finale 2012 specimen exists in any surveyed corpus either, so the long's word order is
verified for little-endian files only, and that is unlikely to change.

Finale 2012's published requirements read `OS X 10.7, 10.6, or 10.5. Mac Power PC or Mac Intel`, which is
internally inconsistent: 10.6 Snow Leopard was the first release to drop PowerPC hardware and 10.7 Lion removed
Rosetta as well, so a PowerPC Mac can run neither. The line matches the Finale 2009 requirements verbatim, where it
was coherent, and reads as boilerplate carried forward.

Writing a big-endian Finale 2012 document therefore needs the whole of a narrow intersection: PowerPC hardware,
which caps at 10.5 Leopard, meeting Finale 2012's 10.5 floor exactly, with a release from late 2011 running on a
machine Apple had stopped selling five years earlier. Whether MakeMusic still shipped a PowerPC slice at all is
unverified, given that the requirement line looks carried forward. Such files are unlikely to exist in any number.

That is a reason not to expect a specimen, not a reason to depend on there being none. The reader decodes the
big-endian case as the symmetric counterpart of the verified little-endian one and warns when it meets one, so an
unverified path announces itself instead of passing silently.

The flag bits are confirmed both physically and semantically: across 1,268 files, bit 1 occurs 9 times and every one
of those tuples carries a non-zero font triple whose exact Finale 27 companion shows `<useOwnFont/>` with the same
`fontID` and `fontSize`; the 15,931 tuples without it never carry one. This closes the previously `not_identified`
row for `ClefOptions.clefDefs[*].font.fontId` in
[`data/legacy_option_font_id_locations.csv`](../../data/legacy_option_font_id_locations.csv).

Word 3 is the difference between the clef's musical baseline, such as the G line of a treble clef, and its
typographic baseline: a font whose clefs already sit on the musical baseline leaves it zero. It is zero in all 1,268
corpus specimens, because no unedited document sets it, so three controlled fixtures carry the whole weight here.

**Finale 2001 onward stores Efix. Confirmed.** `F2001Win-tclef-baseline.mus` sets the treble
clef baseline to -0.25 inch in Windows Finale 2001; tuple word 3 stores `-4608`, exactly
`-0.25 * 288 * 64`, and its exact companion preserves `<baseAdjust>-4608</baseAdjust>`. This
places the unit boundary at the first DCL release directly. `F2005-clef-baseline.mus` sets one inch of baseline on the treble
clef; the stored word is `18432`, which is exactly one inch — 288 Evpu at 64 Efix each — and the exact Finale 27
companion carries `<baseAdjust>18432</baseAdjust>` through unchanged. The same fixture asks for minus two inches on
the bass clef, which would be `-36864`, and the file stores `-32768`. **The field is a signed 16-bit word**, so its
usable range is about ±512 Evpu, and Finale saturates rather than wrapping. Recovering `-32768` is the file being
read correctly. Finale's own dialog reads that value back as `-1.7778` inches, which is `-32768 / 18432`; the UI and
the storage agree, and the discrepancy is entirely the clamp.

**From Finale 3.0 through 2000 it is a small signed count of harmonic levels, in word 4 of the clef's own selector
rather than word 3 of a tuple. Confirmed.** The Coda era stores a value in the same word, but the reader does not
transfer it: there the number adjusts the baseline of mid-measure clefs only, which is not what musxdom's
`baselineAdjust` means, and Finale 27 discards it. The gate is the epoch. `F100-clef-baseline.mus` and `F263-clef-baseline.mus` each change two clefs'
baseline adjustments and move word 4 and nothing else: the Finale 2.6.3 pair differs from its baseline in exactly
three bytes across the whole file. Each era's own ETF shows the same words. The Coda era ships these populated —
`-2, -4, -5, -6, -4, 0, 0, 0` for selectors 28 through 35 — while Finale 3.0 onward ships zeros and keeps the field,
which is why 165 Coda and 143 uncompressed source clefs carry a non-zero value.

**The conversion is one harmonic level to 768 Efix — half a space, since a harmonic level is a staff position.
Confirmed in two independent eras.** `F372-clef-baseline` stores `1`, `-2` and `-5` and its companion carries
`768`, `-1536` and `-3840`; `Fin97-clef-baseline` stores `1` and `-2` and its companion carries `768` and `-1536`.

**A stored count only applies when the document switches the feature on.** Before Finale 97 that switch is bit 0 of
word 5 of the *first* clef's selector, and it governs the whole document rather than one clef: the Finale 3.7.2 pair
toggles it `0 -> 1` and the companion then converts all eight clefs, including ones that save never touched, while
the same document with the switch off loses baselines it plainly stores. Finale 97, internally 3.8, dropped the
checkbox and adjusts unconditionally.

Testing word 5 for non-zero instead of testing bit 0 looks right and is not: Finale 97 files carry 24, 30 and 36 in
that word for unrelated reasons, and bit 0 is clear in all of them. The always-on window is also bounded at both
ends, at versions 3.8 through 5.x, rather than left open at "3.8 or later". The early path only ever sees pre-2001
versions, so the upper bound changes nothing today; it is there so that a file whose version is recovered as
something wild falls back to the bit test, which reads the document, rather than to an unconditional yes.

The corpus never sets the switch: every one of the 271 pre-2001 files leaves bit 0 clear, and every file from 3.8
onward leaves all eight counts at zero. Only the controlled fixtures discriminate between the possible rules, which
is exactly why they were needed. With the switch honoured, `baseAdjust` agrees with all 1,120 adjacent-exact
companions.

Indices 8–17 do not exist before Finale 2001 and 16–17 do not exist before Finale 2003. Finale's own upgrade
supplies the missing ones from the version doing the opening, and the shape comparators it assigns to indices 16 and
17 differ per document — 1/2, 2/3, 3/4, and 25/26 across four controlled companions — which is a direct
demonstration that a shape comparator must never be carried between documents as an identity.

### Corpus verification

Every one of the 1,120 adjacent-exact source/Finale 27 pairs was imported and compared with its companion,
`baseAdjust` included. Of the source-supplied definitions, all fields agree except four, and those four are
upgrade-time font substitution rather than decoding error.

**The discriminator is the document's music font.** Grouping the 57 Coda-era pairs by the music font and the clef
character stored at index 4:

| Music font | Stored | Finale 27 wrote | Files |
|---|---:|---:|---:|
| Pmusic | 214 | 214 | 34 |
| Petrucci | 32 | 32 | 16 |
| Petrucci | 214 | 214 | 3 |
| **Sonata** | **214** | **32** | **3** |
| Sonata | 100 | 100 | 1 |

Only Sonata documents are altered, and 214 is kept in every other font including Petrucci. Finale 27 writes 32, a
space, which musxdom reads as a blank clef.

**Why it substitutes is unknown.** Character 214 is `unpitchedPercussionClef2` in Sonata as well as in Petrucci and
Pmusic, so this is not a codepoint that means something different in the substituted font, and an encoding
difference does not explain it. The behavior is recorded as observed and unexplained. The fourth difference is in
one of the same three files, whose index 7 also moved from `adjust 0, clefYDisp -4` to `adjust -5, clefYDisp -2`
while keeping its character; that one is a single unexplained instance of position drift.

Two observations bound the claim. The one Sonata document with a hand-edited clef table — indices 4 through 7 holding
100, 68, 247 and 175 rather than the stock values — is carried through completely unchanged, so the substitution
applies to the stock Coda table rather than to Sonata documents generally. And index 7 changed in only one of the
three files that share an identical table, so whatever selects that adjustment is not the table alone and is
**open**.

This is the same font that needs a baseline adjustment where Petrucci does not, so both known Sonata-specific
behaviors involve the same font, but no common cause has been established. **The reader keeps the stored character
in every case.** Reproducing Finale's substitution is not attempted and is not a goal: the file says 214 and the
importer says 214.

The scalar options around the collection do **not** share locations across eras, which is the trap in this class:

| Field | Location | Coda | Finale 3.0–2006 | 2007+ |
|---|---|:--:|:--:|:--:|
| `defaultClef` | selector `01` word 0 | yes | yes | yes |
| `endMeasClefPercent` | selector `13` word 2 | yes | yes | yes |
| `endMeasClefPosAdd` | selector `13` word 3 | yes | yes | yes |
| `clefFront` | selector `19` word 0 | yes | yes | yes |
| `clefBack` | selector `19` word 1 | yes | yes | yes |
| `clefKey` | selector `38` word 5 | **no** | yes | yes |
| `clefTime` | selector `39` word 4 | **no** | yes | yes |
| `showClefFirstSystemOnly` | selector `27` word 1 bit 0 | **no** | yes | yes |
| `cautionaryClefChanges` | selector `44` word 3 bit 2 | **no** | yes | yes |

In the Coda era selector `27` word 1 and selector `39` word 4 are font sizes — the lyric-chorus and clef font tuples
the FontOptions mapping already reads — and selector `38` word 5 disagrees with the companion on every Coda file
that has a non-default value. The reader therefore leaves those three at the Finale 27 default before Finale 3.0.
Agreement for the locations that are used is 57/57 Coda, 173/173 uncompressed, 374/374 DCL, and 497/497 zlib, with
genuine non-default coverage for `endMeasClefPercent`, `endMeasClefPosAdd`, `clefFront`, and `clefKey`.

`cautionaryClefChanges` is **bit 2** of the courtesy flags at selector `44` word 3, and `cautionaryKeySigChanges`
is bit 0. **Confirmed** by a controlled Finale 2005 pair: turning off the courtesy clef alone moves that word
`7 -> 3`, and turning off the courtesy key signature alone moves it `7 -> 6`. The second save is what makes this a
mapping rather than a guess — without it, any bit that happened to be clear would have fitted.

The corpus could not have settled this at all. All 1,120 companions have the option set, and only the values 5 and 7
occur, both of which leave bit 2 set. That is the same shape of trap as the Coda-era scalars: a location that is
never contradicted because nothing in the corpus varies it.

**Four of the nine clef options do not exist in the Coda era.** Alongside the courtesy clef below,
the reader treats `clefKey`, `clefTime` and `showClefFirstSystemOnly` as absent from that era, on
four independent grounds:

- the Finale 3.0-and-later locations for all three hold something else entirely before 3.0 —
  selectors `27` and `39` carry font tuples there, so reading them would report a font size as a
  spacing value;
- no Coda document in the corpus has a non-default `clefTime` or `showClefFirstSystemOnly` in its
  exact Finale 27 companion, in 57 pairs;
- the three whose companion shows a non-default `clefKey`, of 1, 1 and 12, hold no word matching
  those values anywhere in their globals, scaled or otherwise, and are the same three Sonata
  documents that carry every other Coda companion anomaly, so that is upgrade synthesis rather
  than a value read from the file; and
- the era's own user interface does not appear to offer them.

This is **strong** rather than confirmed: absence is being inferred, and one Coda document that set
any of the three would overturn it. The reader leaves all three at the Finale 27 baseline, which
already carries zero and false — the same values Finale 27 produces when it upgrades one of these
documents. Only the courtesy clef needs asserting, because there the baseline and the era agree but
the *record* would disagree.

The Coda era has no courtesy-clef option at all; the earliest version found to offer one is 3.6.2. Those documents
always show a courtesy clef, so the reader asserts that for the whole era rather than reading a record, and reports
it as `ValueOrigin::LegacyBehavior`: known exactly, stored nowhere, and not a guess at a default. It must not
read one: selector `44` word 3 is **zero in all 57 Coda files**, so bit 2 there would assert the opposite. That era
does store the courtesies it has as separate boolean words — the same controlled edit in Finale 2.6.3 moves selector
`12` word 1, the key signature's — and which word would hold a clef's is moot, since there is none.

The boundary is the epoch, not version 3.6.2. Finale 3.0 through 3.5 predate the option as well, but their files
already carry bit 2 set, so reading the bit gives the correct answer for them, and an epoch gate says that in one
line where a version range would have to name a release whose behavior the bit already reports.
