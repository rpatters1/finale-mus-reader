# Staff

**Covers:** Raw legacy `Staff` records, their stable Finale-2000 base, and later stored extensions.
**Read when:** Working on `Staff`, selector `IS`, class `0x00e7`, or the pre-Finale-3.7 staff layout.
**Confidence:** `confirmed` for identities and represented payload boundaries; `public-PDK-derived`
and independently binary-verified for the 18-word base; `strong` for the represented Coda
attribute extension; `open` for remaining unmapped fields.

## Identity and scope

The fixed-row selector is `IS`; the zlib class is `0x00e7`. Both are keyed by staff ID. Recovery
constructs source-owned `musx::dom::others::Staff` objects. Coda staff names are parallel Others
rows keyed by the same staff ID: uppercase `IN` holds the full name, and by Finale 2.6.3 lowercase
`in` holds the abbreviation. Each incidence carries 12 eight-bit, NUL-terminated or padded bytes.
Recovery converts each nonempty name with the corresponding imported name font, creates a
`texts::BlockText` and an `others::TextBlock`, and writes the TextBlock comparator to the Staff.
The PDK's other synthesized referents remain outside this raw Staff slice.

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
| 1 | packed tablature positions | low-byte `capoPos`, high-byte `lowestFret` |
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

The low bit of word 7 selects the custom-line representation. Remaining set bits in word 7 name
lines 16–26; bits in word 8 name lines 15–0. Otherwise word 8 is the ordinary staff-line count.

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
controls in word 2. Clearing `0x0100` hides same-layer note-attached items and also hides fretboards
and chords. Clearing `0x0200` hides notes in other layers. Clearing `0x0400` hides articulations,
lyrics, smart shapes, and expressions in other layers. The first and third controls therefore each
expand into several modern booleans. Opening the Staff dialog can also materialize the note-font
size/effects word as `0x1800`; the controlled tablature records establish that this word is not
another flag field. Layouts containing the second alternate-notation flag word use the independent
modern interpretations instead.

Through Finale 2000, a stored zero note-font size on a Staff that does not use an independent
notehead font becomes the Finale 27 default size of 24 on upgrade. Where the layout contains that
tuple, the reader preserves the stored zero; recovery coverage classifies only that exact
disabled-font `0` to `24` comparison as `DifferentDefaults`. Finale 2001 and later companions
retain zero, so the classification ends at that boundary. Nonzero sizes and Staffs with
`useNoteFont` enabled are excluded.

Through Finale 2008, Finale 27 can enable `useNoteFont` while upgrading percussion and tablature
Staffs whose stored value is false. Recovery preserves the stored switch; coverage classifies that
exact false-to-true conversion as `FinaleUpgradeLoss` for either notation style. Standard Staffs,
the reverse conversion, other provenance, and Finale 2009 or later are excluded.

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
  top in the high byte. The 18-word post-Coda layout retains the ordinary Finale 27 Staff's
  `-5`/`-3` values as `Finale27Default`.
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
  `autoNumbering`. Shorter layouts leave `hideStaffLines` unmapped because its bit had an earlier
  whole-staff hiding meaning; they report automatic numbering disabled with numeric style value
  zero and `instUuid` as `uuid::Unknown`, both as `LegacyBehavior`.

The 32-bit fields use native long-word order: high word first in big-endian files and low word first
in little-endian files. The controlled cohort exercises both byte orders for the early 32-bit
fields. All six stem-offset fields are zero in the cohort, so their signed values and order remain
structurally supported rather than independently varied. Bytes not named above remain unmapped.

Controlled Finale 2005 tablature edits independently exercise the extended flag switches for
showing clefs on every system, using letters, breaking lines at numbers, and hiding tuplets. They
also distinguish both bytes of word 1 and corroborate the stored fret-instrument reference and
vertical number offset.

`redisplayLayerAccis`, `hideTimeSigsInParts`, and `hideKeySigsShowAccis` postdate the legacy
formats and have no raw Staff location to recover. Legacy behavior leaves `redisplayLayerAccis`
and `hideKeySigsShowAccis` false, while `hideTimeSigsInParts` follows the resolved
`hideTimeSigs` value. All three report `LegacyBehavior` provenance.

`STUDIO_VIEW_STAFF_ID` identifies Finale's application-owned Studio View Staff rather than authored
score content. Recovery imports that Staff on a best-effort basis, but coverage omits the entire
instance from both source and companion observations.

## Coda layout

Finale 1.0 and 2.6 use a distinct six-word `IS` payload. Within that row, word 0 is `defaultClef`,
word 4 uses the later packed `transposition` representation, and word 5 bit `0x0010` hides time
signatures. The other bits and words remain open. In particular, a constant word 2 value of 4 and
word 3 value of 1024 in ordinary Staffs do not establish modern field meanings.

Finale 2.6.3 can add a parallel six-word `IA` row for controls absent from Finale 1.0. In the
represented row, word 2 low bit marks a custom staff and `IS` word 2 supplies its line mask; the
combination maps to custom line 13. Word 1 packs the legacy Base Key UI value in its low byte and
the signed vertical tablature-number offset in its high byte; the latter becomes an Efix value by
multiplication by 256. Musxdom has no Staff member for Base Key. `IA` words 3 and 4 hold the
notehead font ID and packed size/effects, as in the later base. Word 5's high byte holds the
fret-instrument ID; its low byte includes the `useNoteFont` switch, while the represented value
also selects tablature notation. A Coda tablature Staff has the legacy behavior of showing clefs
on every system while hiding rests, augmentation dots, stems, and tuplets; the other modern
tablature visibility switches retain their false defaults. A recovered one-line custom Staff has
legacy barline extents of -48 below and 48 above its reference line. The remaining `IA` values
remain open. Without the optional row, recovery supplies the era's fixed five-line Staff as
`LegacyBehavior`. It also creates the otherwise unavailable Staff-local note-font tuple with the
pinned Finale 27 notehead-font size and reports that size as `Finale27Default`; the other tuple
members remain unmapped.

Each nonempty `IN` or `in` family becomes a regular block-text pair. The raw eight-bit bytes are
decoded through `FontOptions::StaffNames` or `FontOptions::AbbrvStaffNames`, respectively. The
created `TextBlock` uses block text, automatic 100-percent line spacing, the modern position and
shape switches, word wrapping, and the square-corner legacy behavior. Coda `HT`/`HS` block text is
materialized first so its historical ordinal IDs remain available to page and measure text
assignments. Staff-name text is then allocated after those pools are complete.
Recovery coverage follows both full and abbreviated Staff-name references through their
`TextBlock` and compares the resulting `BlockText` semantically in every source version. If either
Staff reference is zero, it compares the comparator values directly instead.

Neither six-word row has locations for the later line spacing, four rest offsets, stem reversal,
or repeat-dot offsets. Recovery takes only those scalars from the ordinary Staff in the pinned
Finale 27 reference and reports them as `Finale27Default`. It reports `instUuid` as
`uuid::Unknown`; fields without another stated mapping or legacy behavior remain unmapped.

## Evidence

The byte offsets and both byte orders are reproducible with
`tests/evidence/F2003/F2003-baseline.mus`,
`tests/evidence/F2003/F2003Win-empty.mus`, and
`tests/evidence/F2012/F2012-bookmarks.mus`, and the Finale 2012 display-bit meaning with
`tests/evidence/F2012/F2012-hidestafflines.mus`. The earlier boundaries are reproducible with
`tests/evidence/F100/F100-baseline.mus`, `tests/evidence/F100/F100-staffprops.mus`,
`tests/evidence/F263/F263-timecomp.mus`, `tests/evidence/F263/F263-staffopts.mus`, and
`tests/evidence/F372/F372-baseline.mus`; the early note-font representation is corroborated by
`tests/evidence/F372/F372-notehead-font.mus`. The tablature fields are reproducible with the
`tests/evidence/F2005/F2005-tab-*` fixtures. The broader controlled comparison and remaining
differences are recorded in [`../../investigations/staff.md`](../../investigations/staff.md).
