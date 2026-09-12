# Staff

**Covers:** Raw legacy `Staff` and `StaffStyle` records, their shared Staff payload layouts, and
later stored extensions.
**Read when:** Working on `Staff` or `StaffStyle`, selectors `IS`/`SY`, classes `0x00e7`/`0x00e8`,
or the pre-Finale-3.7 staff layout.
**Confidence:** `confirmed` for identities and represented payload boundaries; `public-PDK-derived`
and independently binary-verified for the 18-word base; `strong` for the represented Coda
attribute extension; `open` for remaining unmapped fields.

## Identity and scope

The fixed-row selector is `IS`; the zlib class is `0x00e7`. Both are keyed by staff ID. Recovery
constructs source-owned `musx::dom::others::Staff` objects. Before Finale 3.7, staff names are
parallel Others rows keyed by the same staff ID: uppercase `IN` holds the full name, and by Finale
2.6.3 lowercase `in` holds the abbreviation. Each incidence carries 12 eight-bit, NUL-terminated
or padded bytes. Finale 3.7 moves the names to TextBlock references in the Staff record. Recovery
converts each nonempty parallel name with the corresponding imported name font, creates a
`texts::BlockText` and an `others::TextBlock`, and writes the TextBlock comparator to the Staff.
Other synthesized referents remain outside this raw Staff slice. **Strong.** The
pre-Finale-3.7 representation occurs in Coda controlled fixtures and two companion-backed Finale
3.5 documents from `rpatters1-main`; Finale 3.7 is the documented transition.

## Staff styles

The fixed-row StaffStyle selector is `SY`; the zlib class is `0x00e8`. The comparator is the
one-based style ID. Recovery constructs source-owned `musx::dom::others::StaffStyle` objects and
does not gate them by saving version. No controlled pre-Finale-2000 source stores this family;
StaffStyles that Finale synthesizes while upgrading earlier alternate-notation assignments remain
companion-only until those assignments are recovered. **Confirmed** for the identities and
represented absence.

The record begins with one of the Staff payload shapes decoded below. The narrow form appends a
12-byte metadata row and a 48-byte NUL-terminated or padded style name, which has no associated font
and is converted through the source platform's default code page. The Finale 2012 form instead has
a 96-byte Staff prefix, the same 12-byte metadata row, and a 192-byte NUL-terminated or padded
UTF-16LE name. Represented narrow records are at most 144 bytes, while the Finale 2012 records are
300 bytes; those record geometries select the name encoding directly. No record from 145 through
299 bytes occurs in the inventoried corpora. For that unrepresented interval, the decoder uses the
Finale 2012 Unicode boundary and still requires enough bytes for the selected trailer and base
layout. The three leading metadata words map bits to the correspondingly named members of
`StaffStyle::Masks`. The alternate-notation bit also includes the `hideChords` and
`hideFretboards` overrides, which musxdom represents as independent masks; the separately stored
bits for either override remain additive. The two words after the masks remain uninterpreted. The
final word's low bit selects explicit control values. When set, `0x0002` stores `copyable` and
`0x0004` stores `addToMenu`; when clear, `addToMenu` is implicitly true and alternate-notation
styles are implicitly copyable. The low bit itself has no musxdom destination. **Confirmed** for
both trailer geometries and the represented metadata values. The three explicit control states
are reproducible with `tests/evidence/F2000/F2000-style-menu.mus`,
`tests/evidence/F2000/F2000-style-nomenu.mus`, and
`tests/evidence/F2000/F2000-style-menu-copyable.mus`; the implicit state is independently
binary-verified against `mus-09deed3ef3f054d6`. The remaining bit meanings are
`private-framework-derived`.

| Metadata word | Mask bit to musxdom member |
|---:|---|
| 0 | `0x0001 floatNoteheadFont`, `0x0002 flatBeams`, `0x0004 notationStyle`, `0x0010 blankMeasureRest`, `0x0020 noOptimize`, `0x0040 defaultClef`, `0x0080 staffType`, `0x0100 transposition`, `0x0200 blineBreak`, `0x0400 rbarBreak`, `0x1000 negMnumb`, `0x2000 negRepeat`, `0x4000 negNameScore`, `0x8000 hideBarlines` |
| 1 | `0x0001 fullName`, `0x0002 abrvName`, `0x0004 floatKeys`, `0x0008 floatTime`, `0x0010 hideRptBars`, `0x0020 negKey`, `0x0040 negTime`, `0x0080 negClef`, `0x0100 hideStaff`, `0x0200 noKey`, `0x0400 fullNamePos`, `0x0800 abrvNamePos`, `0x1000 altNotation` (also `hideChords` and `hideFretboards`), `0x2000 showTies`, `0x4000 showDots`, `0x8000 showRests` |
| 2 | `0x0001 showStems`, `0x0002 hideChords`, `0x0004 hideFretboards`, `0x0008 hideLyrics`, `0x0010 showNameParts`, `0x0020 showNoteColors`, `0x0040 hideStaffLines`, `0x0080 useNoteShapes`, `0x0100 hideKeySigsShowAccis`, `0x0200 redisplayLayerAccis`, `0x0400 negTimeParts` |

Before the Finale 2012 layout, Note Shapes is a fourth source notation style alongside Normal,
Percussion, and Tablature. Recovery splits that source style into the modern Staff values
`notationStyle = Standard` and `useNoteShapes = true`. On a StaffStyle, its source notation-style
override becomes only `masks.useNoteShapes`; `masks.notationStyle` remains active for Normal,
Percussion, and Tablature because those are the instrument-changing notation choices. The Finale
2012 layout stores the notation-style and note-shape masks independently. **Strong.** The tracked
Finale 2006 “15. Note Shapes” style supplies the pre-Finale-2012 conversion, while the distinct
Finale 2012 Staff and mask representation supplies the boundary.

The represented Finale 2000 style has an 18-word Staff prefix; the Finale 2003 and 2011 styles
have 36-word prefixes. Their prefix values agree with the same Staff decoder and their modern
companions. **Confirmed.** Evidence:
`tests/evidence/F2000/F2000-staff-style.mus`,
`tests/evidence/F2003/F2003-staffstyle.mus`, and
`tests/evidence/F2011/F2011-staffstyle.mus`, with their tracked ETF or Finale 27 companions. The
Finale 2012 geometry and its six stored standard style names are reproducible with
`tests/evidence/F2012/F2012-bookmarks.mus` and its Finale 27 companion.

The public Finale-2000 `EDTStaffSpec` declaration establishes that its fields through the final
padding word are the raw base. The following staff ID, related-record tags and cmpers, and measure
range form the PDK's synthesized tail and are not decoded from `IS`. Source: public PDK
[`edata.h`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/edata.h),
accessed 2026-09-09.

## Stable 18-word base

The base begins in Finale 3.7 in the represented fixtures and remains the prefix of every later
layout through Finale 2012.

| Word | Raw field | Recovered Staff value |
|---:|---|---|
| 0 | bottom barline offset | `botBarlineOffset` |
| 1 | packed tablature positions | low-byte `capoPos`, high-byte `lowestFret`, except in the short one-string tablature form described below |
| 2 | alternate-notation flags | `altNotation`, `altLayer`, and the PDK show-detail inverses |
| 3 | notehead/tablature font ID | `noteFont.fontId` |
| 4 | font size and effects | remaining `noteFont` members |
| 5 | primary flags | notation style, note-shape/font switches, repeat-dot hiding, flat beams, blank measures, optimization |
| 6 | two clef bytes | `defaultClef`, `transposedClef` |
| 7–8 | staff-line representation | `staffLines` or `customStaff` |
| 9 | top barline offset | `topBarlineOffset` |
| 10 | packed transposition | `transposition` and its key-signature or chromatic variant |
| 11 | display flags | independent elements, barline breaks, and represented hide switches |
| 12–13 | four signed rest-position bytes | `dwRestOffset`, `wRestOffset`, `hRestOffset`, `otherRestOffset` |
| 14 | stem reversal | `stemReversal` |
| 15–16 | name text IDs | `fullNameTextId`, `abbrvNameTextId` |
| 17 | padding in the 18-word layout | no value |

The low bit of word 7 selects the custom-line representation. Together, words 7–8 form a rotated
32-bit line mask: rotate the stored value left by 11 bits and retain the low 27 bits, after which
each bit number is its staff-line number. Word 7 bits 5–15 therefore represent lines 0–10, word 8
bits 0–15 represent lines 11–26, word 7 bit 0 is the custom-layout marker, and word 7 bits 1–4 are
outside the line vector. Otherwise word 8 is the ordinary staff-line count. **Confirmed.**
Controlled fixtures exercise both ranges and their line 0, 10, 11, 14, and 15 boundaries.

When the post-Coda source layout has no repeat-dot-offset field, its line representation selects
the fallback. An ordinary `staffLines` count retains the reference Staff's offsets. A `customStaff`
uses `Staff::calcMiddleStaffPosition()`, including an empty custom vector. An even result is already
a line; an odd result is the center space, in which case the line immediately above it is selected.
The bottom and top dots are placed one staff-position unit below and above that line. A structurally
present repeat-dot-offset field supersedes either fallback. **Strong.** The controlled Finale 2000
custom-line fixtures reproduce both odd- and even-line-count companion results, while the Finale
2000 Guitar Tablature template retains the reference offsets for ordinary one-line staves. The
Coda exception is described below.

An 18-word tablature Staff uses word 1 differently. Its low byte is the MIDI Base Key used to
synthesize a one-string `FretInstrument`, and its signed high byte is the tablature-number vertical
offset in quarter-EVPUs. `capoPos` and `lowestFret` are unavailable in this layout. Before Finale
2000, words 7–8 encode the tablature Staff as legacy custom line 11; Finale 2000 uses the ordinary
one-line count, while Finale 2002 permits an ordinary five-line tablature Staff without changing
the word-1 interpretation. A non-tablature layout without the stored fret-instrument field retains
the Finale 27 default. The pre-Finale-2000 form hides both repeat dots. All represented
18-word tablature Staffs show the clef
on every system and hide rests, dots, stems, and tuplets. The layout has no recovered
`breakTabLinesAtNotes` field, so that value retains the pinned Finale 27 Staff default. **Confirmed.**
The controlled Finale 2000 and Finale 2002 tablature companions leave it false; the earlier
template-derived contrary observation is preserved in the investigation notes. The general
one-string conversion is also continuous with the controlled
`tests/evidence/F263/F263-staffopts.mus` attributes layout.

The transposition masks and signed subfields follow the public PDK declaration. A zero word omits
the contained object. Bit `0x2000` selects the chromatic variant; otherwise the low two six-bit
fields form the key-signature variant. Bits `0x8000` and `0x4000` are `setToClef` and
`noSimplifyKey`.

Before Finale 2000, including the Coda epoch, alternate notation is not a Staff property. It is
applied over measure ranges by separate records that Finale 27 upgrades into synthesized Staff
Styles and assignments. The early Staff has none of the modern alternate-notation properties, so
recovery supplies the legacy behavior as `Normal`, layer 0, slash dots enabled, and every other
alternate-notation boolean disabled. Recovery of the range records and their synthesized Staff
Styles is outside the raw Staff slice.

The remaining undecoded Coda Staff leaves retain `Unmapped` provenance. Recovery coverage does not
classify disagreements on those leaves as expected; they remain unexpected until their raw values
are resolved correctly.

Post-Finale-2000 layouts that lack the second alternate-notation flag word use three aggregate
controls in word 2. The shared decoding applies these controls identically to Staff and StaffStyle.
Clearing `0x0100` maps to hiding same-layer articulations, lyrics, Smart Shapes, and also hides
fretboards and chords. It does not supply `altHideExpressions`, which retains the Finale 27 default
of false until an independent field is present. Clearing `0x0200` hides notes in other layers.
Clearing `0x0400` hides articulations, lyrics, smart shapes, and expressions in other layers. The
first and third controls therefore each expand into several modern booleans.
The expression exclusion is **confirmed** for Finale 2000 and **weak** for later aggregate layouts.
Opening the Staff dialog can also materialize the note-font size/effects word as `0x1800`; the
controlled tablature records establish that this word is not another flag field. Layouts
containing the second alternate-notation flag word use the independent modern interpretations for
the other-layer fields.

From Finale 2000 through Finale 2008, `altHideSmartShapes` is synthesized as the logical OR of the
note-attached-items control and the alternate-notation type. The type enables it for Slash, One Bar
Repeat, Two Bar Repeat, and Blank and disables it for Normal and Rhythmic; Blank With Rests is
believed to follow the enabled group. Recovery calls musxdom's notation-type predicate for this
partition and reports the combined result as `LegacyMusAdjusted`; Finale 2009 and later recover
the independent stored value. **Strong** for the represented types; **weak** for Blank With Rests
and the exact Finale 2009 boundary.

The low nibble uses the musxdom `AlternateNotation` order through its terminal `Blank` value;
notably, stored value 6 is `Blank`, not `BlankWithRests`. **Strong.** This agrees across 22 distinct
Finale 2011 documents in the current companion-backed Staff cohort.

A nonempty Staff font tuple is a direct override: enabling the independent font in the controlled
Finale 97 pair exposes sizes of 28 and 26 directly in word 4. When `useNoteFont` is false, recovery
preserves the stored size rather than replacing zero from a document font option. Coverage treats
any size disagreement as `DifferentDefaults` for a disabled independent font, without a version,
value-pair, or provenance gate; an enabled font remains subject to exact comparison. The withdrawn
inheritance trial is recorded in [`../../investigations/staff.md`](../../investigations/staff.md).
**Confirmed** for the stored override; the dormant-value treatment is a semantic comparison rule.

Before Finale 2012, Finale 27 can change `useNoteFont` in either direction while upgrading
percussion and tablature Staffs. Recovery preserves the stored switch; coverage classifies either
Boolean conversion as `FinaleUpgradeLoss` for those notation styles. Standard Staffs, other
provenance, and Finale 2012 or later are excluded. **Strong.**

The setting does not recover `hasStyles`. That property
is not stored in the Finale 2000 or 2003 `IS` record. Finale 27 synthesizes it when the staff has a
separate `Sy` assignment record; merely defining an `SY` Staff Style does not set it. By Finale
2011, word 5 bit `0x1000` stores the value. Its exact introduction is intentionally unresolved:
future `StaffStyleAssign` recovery will refresh `hasStyles` from assignment presence in every
source version rather than treating the stored bit as authoritative. Until then it remains
`Unmapped`, and a differing companion value is deferred as awaiting that dependent recovery. The
classification stops applying when assignment recovery gives the field a calculated provenance.

Word 5 bit `0x0020` independently enables `noteFont`; word 3 is its font ID and word 4 packs its
size in the high byte and effects in the low byte. This representation is already present in Finale
3.7.2, so it has no Finale-2000 gate. Finale 27 may spuriously enable the property when upgrading a
pre-Finale-2009 percussion Staff whose stored switch is clear. Recovery preserves the stored
switch, and coverage classifies only that narrowly identified conversion as upgrade loss.

## Stored extensions

Payload structure, rather than saving-version numbers, gates the extensions:

- More than 18 words repurposes word 17 as two signed repeat-dot offsets: bottom in the low byte,
  top in the high byte. The 18-word post-Coda layout uses the legacy geometry calculation
  described above.
- At least 40 bytes stores `lineSpace` as a signed 32-bit Efix at byte 36; divide by 64 for EVPUs.
  Shorter post-Coda layouts retain the ordinary Finale 27 Staff's 24-EVPU value as
  `Finale27Default`.
- At least 44 bytes stores the signed 32-bit `vertTabNumOff` at byte 40.
- At least 46 bytes stores visibility, tablature, stem-direction, fixed-stem, and hide-mode flags
  at byte 44. Values represented as “show” switches are inverted into musxdom's hide properties.
- At least 48 bytes stores `fretInstId` at byte 46.
- The 72-byte layout stores six signed 32-bit stem offsets from byte 48 through byte 71, in the
  order of horizontal up/down, vertical start up/down, and vertical end up/down.
- At least 74 bytes stores the three remaining alternate-notation visibility switches at byte 72,
  inverted into musxdom's hide properties. Its presence also distinguishes the independent modern
  interpretation of the earlier alternate-notation flag word from the aggregate compatibility
  controls in shorter post-Finale-2000 layouts.
- The represented 96-byte Finale 2012 layout maps display word bit `0x0004` to
  `hideStaffLines`, stores automatic name numbering in word 37, and stores the canonical 16 UUID
  bytes at byte 80. In word 37, bit `0x8000` enables numbering and the remaining bits select
  `autoNumbering`. Shorter layouts take `hideStaffLines` from the pinned Finale 27 Staff because
  the setting is structurally absent and the earlier display bit has a different meaning. They
  report automatic numbering disabled with numeric style value zero and `instUuid` as
  `uuid::Unknown`, both as `LegacyBehavior`.

The 32-bit fields use native long-word order: high word first in big-endian files and low word first
in little-endian files. The controlled cohort exercises both byte orders for the early 32-bit
fields. All six stem-offset fields are zero in the cohort, so their signed values and order remain
structurally supported rather than independently varied. Bytes not named above remain unmapped.

Controlled Finale 2005 tablature edits independently exercise the extended flag switches for
showing clefs on every system, using letters, breaking lines at numbers, and hiding tuplets. They
also distinguish both bytes of word 1 and corroborate the stored fret-instrument reference and
vertical number offset.

When the payload ends before the extended flag word, its non-tablature fields and the absent stem
offset tail come from the pinned Finale 27 Staff. Tablature instead supplies its fixed legacy
behavior for showing the clef on every system and hiding rests, dots, stems, and tuplets;
`breakTabLinesAtNotes` retains the pinned default. **Confirmed.** The 18-word boundary and
tablature values are reproducible with the controlled Finale 2000 and Finale 2002 fixtures named
below.

`redisplayLayerAccis`, `hideTimeSigsInParts`, and `hideKeySigsShowAccis` postdate the legacy
formats and have no raw Staff location to recover. Legacy behavior leaves `redisplayLayerAccis`
and `hideKeySigsShowAccis` false, while `hideTimeSigsInParts` follows the resolved
`hideTimeSigs` value. All three report `LegacyBehavior` provenance.

StaffStyle recovery applies the same coupled time-signature behavior: `negTimeParts` follows
`negTime`, and the derived mask reports `LegacyBehavior` provenance rather than an independent
legacy source location.

`STUDIO_VIEW_STAFF_ID` identifies Finale's application-owned Studio View Staff rather than authored
score content. Recovery imports that Staff on a best-effort basis, but coverage omits the entire
instance from both source and companion observations.

## Coda layout

Finale 1.0 and 2.6 use a distinct six-word `IS` payload. Within that row, word 0 is `defaultClef`.
Word 4 uses the later packed `transposition` representation unless bit `0x8000` selects Set to
Clef; in that mode, bits `0x7000` hold the three-bit `transposedClef` index and do not carry the
later simplify-key or chromatic meanings. Word 5 selectively retains the
later display masks: `0x8000` is `floatKeys`, `0x4000` is `floatTime`, `0x2000` is
`blineBreak`, `0x1000` is `rbarBreak`, `0x0400` is `hideMeasNums`, `0x0200` is
`hideRepeats`, `0x0100` is `hideNameInScore`, `0x0020` expands to both `hideKeySigs` and
`noKey`, `0x0010` is `hideTimeSigs`, and `0x0008` is `hideClefs`. The `0x0800` control has
no persisted musxdom Staff destination. The lower active bits do not retain the later
`hideBarlines`, `hideStaffLines`, `hideChords`, or independent `noKey` meanings. Words 1–3 remain
open; in particular, their common values in ordinary Staffs do not establish modern field
meanings.
**Strong.** The mapped bits agree across 509 non-Studio-View Staff occurrences in 77
companion-backed Coda documents selected from `rpatters1-installs` and `rpatters1-main`.

The structurally identical Staff row in early uncompressed files keeps the `0x0020` controls
separate: it recovers `hideKeySigs`, while `noKey` retains the Finale 27 default. This distinction
is confirmed for Finale 3.0 files and applies to the structurally gated 3.0–3.2 layout. **Strong.**

The Coda layout can add a parallel six-word `IA` row for optional Staff controls. Word 5 bit
`0x0040` activates the signed staff-line value in word 2. Without that bit, the Staff has five
ordinary lines. With it, nonnegative values are ordinary line counts except that `1` selects
custom line 13 with barline extents two spaces below and above its reference line. A negative
value *n* selects custom line `10 - n`; its bottom barline offset is zero and its top offset is
`(-n - 1)` spaces. Positive values retain the reference Staff's repeat-dot offsets. Zero follows
the general legacy geometry rule above, as do negative values. The legacy visual suppression for
negative values is not represented by the modern Staff repeat-dot hide flags.
**Confirmed.** Controlled source/companion pairs cover active values `0`, `1`, `2`, `4`, `17`,
`-1`, `-3`, and `-11`, plus an inactive baseline.

A zero line count hides the lines but retains standard five-line staff geometry, whether represented
by `staffLines == 0` or an empty `customStaff`. Consequently,
`Staff::calcMiddleStaffPosition()` returns -4 for either zero-line representation and for a
five-line staff.

Word 1 packs the legacy MIDI Base Key in its low byte and the signed vertical tablature-number
offset in its high byte; the latter becomes an Efix value by multiplication by 256. Every
tablature form converts Base Key to the pitch of a synthesized one-string `FretInstrument`, and
the Staff points to that new referent rather than interpreting the byte as `capoPos` or
`lowestFret`. `IA` words 3 and 4 hold the note font ID and packed size/effects, as in the later
base. Word 5 bit `0x0020` enables `useNoteFont` independently; tablature notation also implies
`useNoteFont` when that bit is absent. Bit `0x0080` enables `useNoteShapes`, bit `0x0100` is
`blankMeasure`, and bit `0x0200` selects tablature notation. **Confirmed.** No ordinary Coda
`fretInstId` is stored; its comparator is assigned while synthesizing the referent. A Coda
tablature Staff shows clefs on every system while hiding rests, augmentation
dots, stems, and tuplets; the other modern tablature visibility switches retain their false
defaults. Without the optional row, recovery supplies the era's fixed five-line Staff as
`LegacyBehavior`. It also creates the otherwise unavailable Staff-local note-font tuple with the
pinned Finale 27 notehead-font size and reports that size as `Finale27Default`; fields without
another stated mapping remain open.

Before Finale 3.7, each nonempty `IN` or `in` family becomes a regular block-text pair. The raw
eight-bit bytes are decoded through `FontOptions::StaffNames` or `FontOptions::AbbrvStaffNames`,
respectively. The created `TextBlock` uses block text, automatic 100-percent line spacing, the
modern position and shape switches, word wrapping, and the square-corner legacy behavior. Coda
`HT`/`HS` block text is materialized first so its historical ordinal IDs remain available to page
and measure text assignments. Staff-name text is then allocated after those pools are complete.
Recovery coverage follows both full and abbreviated Staff-name references through their
`TextBlock` and compares the resulting `BlockText` semantically in every source version. If either
Staff reference is zero, it compares the comparator values directly instead.

Neither six-word row has locations for the later line spacing, four rest offsets, or stem
reversal. Recovery takes those unavailable scalars from the ordinary Staff in the pinned Finale
27 reference and reports them as `Finale27Default`. The same baseline supplies `transposedClef`
when Set to Clef is not active, `capoPos`, `lowestFret`, and the later Staff-item switches that the
six-word UI cannot edit. Recovery reports `instUuid` as `uuid::Unknown`; fields without another
stated mapping or legacy behavior remain unmapped.

The live work queue for fields that can still retain `Unmapped` provenance is maintained in
[`STAFF_UNMAPPED_FIELDS.md`](../../state/STAFF_UNMAPPED_FIELDS.md).

## Evidence

The byte offsets and both byte orders are reproducible with
`tests/evidence/F2003/F2003-baseline.mus`,
`tests/evidence/F2003/F2003Win-empty.mus`, and
`tests/evidence/F2012/F2012-bookmarks.mus`, and the Finale 2012 display-bit meaning with
`tests/evidence/F2012/F2012-hidestafflines.mus`. The earlier boundaries are reproducible with
`tests/evidence/F100/F100-baseline.mus`, `tests/evidence/F100/F100-staffprops.mus`,
`tests/evidence/F100/staffopts/F100-floatfont.mus`,
`tests/evidence/F100/staffopts/F100-floatkey.mus`,
`tests/evidence/F100/staffopts/F100-floatnoteshapes.mus`,
`tests/evidence/F100/staffopts/F100-floattime.mus`,
`tests/evidence/F263/F263-timecomp.mus`, `tests/evidence/F263/F263-staffopts.mus`, and
`tests/evidence/F372/F372-baseline.mus`. The 18-word DCL tablature interpretation is reproducible
with `tests/evidence/F2002/F2002-tablature.mus` and
`tests/evidence/F2002/F2002-tablature-basekey47.mus`; the early note-font representation is
corroborated by
`tests/evidence/F372/F372-notehead-font.mus`, and its inherited size by
`tests/evidence/F97/F97-notehead-size28.mus` and
`tests/evidence/F97/F97-notehead-size26.mus`. The signed Coda line forms are reproducible with the
`tests/evidence/F263/staffopts/F263-lines-*` fixtures. The tablature fields are reproducible with
the `tests/evidence/F2005/F2005-tab-*` fixtures. The broader controlled comparison and remaining
differences are recorded in [`../../investigations/staff.md`](../../investigations/staff.md).
