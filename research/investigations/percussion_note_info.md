# PercussionNoteInfo investigations

**Covers:** The class boundary, packed widths, and upgrade from detail selector `DF`.
**Read when:** Revisiting class `0x0139`, selector `Di` or `DF`, or a companion difference.
**Confidence:** `confirmed` for observed bytes; `strong` for the upgrade rule and boundaries.

## 2026-09-07 — Introduction boundary and packed records

**Question.** Does selector `Di` describe a record extending back through the early Finale epochs,
and how did the four notehead characters change in Finale 2012?

**Method.** Record catalogs for the registered `rpatters1-installs` and `rpatters1-main` surveys
were searched by saving product for class `0x0139`. Direct record dumps then checked early
controlled specimens and representative Finale 2001–2009 installation documents. Packed payloads
from narrow token `mus-325173b1552c944c` and wide token `mus-4c75dfe49da42598` were decoded in
member order and compared with their adjacent-exact Finale 27 companions. The current surveyor was
then captured over both controlled evidence surveys.

**Result.** Class `0x0139` occurs in the catalogs only for Finale 2010, 2011, and 2012. A Finale
2010-era payload packs repeated 12-byte, six-word elements. For example, its first two elements
decode as `(35, 7, 48, 48, 48, 48)` and `(4131, 7, 48, 48, 48, 48)`, exactly matching the companion.
The Finale 2012 payload packs repeated 24-byte elements: two words, four low-word-first 32-bit
codepoints, and four zero bytes. Its first observed element decodes as
`(38, 5, 207, 250, 250, 250)`, also matching the companion.

The controlled capture contained no source occurrence of the class. Finale 27 nevertheless added
large default percussion maps to 211 of the 243 compared occurrences. This was not universal: all
ten tracked Finale 2011 fixtures and all twenty-one tracked Finale 2012 fixtures have no class
`0x0139` in either source or companion. The companion-only objects therefore describe upgrade
synthesis for older documents, not canonical maps stored in every post-2010 document, and cannot
establish earlier source storage. The durable layout and current coverage boundary are recorded in
[`../format/others/percussion_note_info.md`](../format/others/percussion_note_info.md).

## 2026-09-07 — DCL `DF` upgrade

**Question.** Does Finale translate the old five-word detail into the modern class, and which of
the two MIDI identities determines `percNoteType`?

**Method.** Three pre-2010 controlled/private specimens were examined:
`tests/evidence/F98/F98-baseline.mus`,
`tests/evidence/F2006/F2006-linked-tiff.mus`, and `mus-2fc904f04995a9cd`. Their `DF` rows were
compared with Finale 27 companions. The modern percussion-type table was independently grouped by
`generalMidi` to test ambiguity. A focused recovery comparison then checked the controlled Finale
2006 specimen.

**Result.** The Finale 2006 linked-TIFF document's map 1 has 47 rows, of which 22 differ from the
filler staff/notehead tuple. Its companion contains exactly those 22 as custom map 1.
`mus-2fc904f04995a9cd` has 128 rows with only three nonfillers; its companion contains exactly
three. Both preserve staff position and closed notehead, copy open into
half/whole/double-whole, compact incidence, and map word 0 through `generalMidi`. Every MIDI value
0–127 currently has one unique table entry. The focused controlled comparison reported 176 equal
class items, zero expected or unexpected differences, and 3,128 companion-only items belonging to
the synthesized canonical maps.

The role of the two MIDI values is visible outside map 1: for example, one legacy library row has
entry key 62 but playback MIDI 44. The positive map-1 specimens keep them equal, so no observed
upgrade directly discriminates them; word 0 is nevertheless the stronger interpretation because
it is explicitly the playback value and reproduces both companions.

The examined Finale 98 companions contain only the canonical 391 notes and no direct upgrade of
their five populated `DF` maps. This establishes Finale 27's behavior, but it does not establish a
reader boundary: an early map may still be the only source for an actual note's staff position and
notehead.

## 2026-09-08 — Semantic recovery boundary

**Question.** Should the reader retain only the `DF` rows and maps that Finale 27 preserves?

**Result.** No. The recovery goal is the effective staff position and duration-appropriate
notehead of each percussion note. Finale's synthesized libraries are irrelevant unless a source
staff or note refers to them, while a legacy map discarded by Finale may still carry the source's
only usable appearance information. Map 1 and the `(0, 207, 250)` omission are correlations from
the current upgrade specimens rather than general selection rules.

The next discriminator is the complete legacy reference chain. In modern musxdom data a
percussion staff reaches a map through `DrumStaff::whichDrumLib`, and a note reaches a row through
`PercussionNoteCode::noteCode` matched against `PercussionNoteInfo::percNoteType`. The legacy
counterparts must be located before narrowing imported maps or suppressing companion data. A
note-level coverage observation of the resolved staff position and notehead can then exclude
unreferenced Finale synthesis without discarding potentially useful legacy maps.

## 2026-09-08 — Focused project cohort and legacy map selection

**Question.** Which old map rows does Finale upgrade?

**Method.** An 85-document cohort was projected from `rpatters1-main` for three related project
groups, with 78 Finale 27 companions. Thirty-two valid legacy documents contained `DF`; all were
uncompressed documents from one project group and carried 128 rows in map 1. The seven documents
whose companions contained both `DrumStaff` and `PercussionNoteCode` were then compared at the raw
record and musxdom percussion-type levels. The evidence tokens are
`mus-5e377325bf2a68bb`, `mus-7be555f3c12df760`, `mus-d3614a4d793f3201`,
`mus-c37089222a7f68a4`, `mus-83764909dd8b43c9`, `mus-c6a556d06a15e981`, and
`mus-7e52db7272aa3564`.

**Result.** Other selector `DS` is the legacy staff attachment and row-selection record. Its
`cmper` is the staff ID. Across its incidence array, word 0 is the map ID and the remaining words
are an LSB-first bitmap indexed by `DF.cmper2`. The seven documents selected 15 or 16 rows each.
Finale produced exactly one map incidence for every set bit, in ascending input-key order: 107 of
107 rows matched. Each base note type's `generalMidi` equals `DF` word 0, all staff positions and
noteheads matched, and repeated base types received order IDs 0, 1, and so on in selection order.

The note-level experiment continues in
[`percussion_note_code.md`](percussion_note_code.md).

Twenty Finale 2012 documents in the cohort independently show the native map, and seventeen also
show the staff attachment. Class `0x0084` attaches staff 32 to map 3 in all seventeen, class
`0x0139` supplies 391 notes across 23 maps in all twenty. Detail class `0x0451` is analyzed in
[`percussion_note_code.md`](percussion_note_code.md).

## 2026-09-08 — Companion synthesis filter

**Question.** How can coverage suppress Finale 27's canonical percussion libraries without
discarding an early source map that may determine a note's staff position and notehead?

**Method.** The `PercussionNoteInfo` comparison preparation collected map IDs present in the
source observation and map IDs referenced by `DrumStaff` on either side. It retained companion
notes belonging to that union and classified every other companion note as Finale synthesis. The
tracked public and private controlled surveys were then recaptured together. The capture contained
247 documents: 81 Coda-banner, 71 DCL, 55 uncompressed, and 40 zlib.

**Result.** All 247 source and companion reads succeeded, with no unexpected findings. The filter
removed 84,065 unreferenced companion note rows. It retained 704 equal leaves from the four
recovered DCL maps and 512 companion-only leaves from the active maps in the four controlled
Finale 97 documents. `DrumStaff` contributed eight equal leaves. The controlled legacy note-entry
edits contributed 128 `PercussionNoteCode` leaves classified as
`awaits-dependent-recovery`, isolating the remaining entry attachment gap without restoring the
canonical-map noise.

## 2026-09-08 — Unified fixed-row recovery

**Question.** Can uncompressed and DCL `DF` maps use one recovery path without deciding row
membership from apparent filler values?

**Method.** The importer was changed to read the normalized fixed-row index for both container
epochs. It unions the LSB-first `DS` bitmaps by map ID, visits selected input keys in ascending
order, and constructs only the corresponding `DF` rows. A focused test supplies identical logical
records through both container framings and both byte orders. It includes a selected
`(staffPosition=0, closed=207, open=250)` row, an unselected customized row, a second unreferenced
map, and two selected rows sharing one playback MIDI value.

**Result.** All four framing and byte-order combinations produce the same four map notes. The
selected filler-looking row survives, customized values do not override a clear bitmap, and the
duplicate playback type receives order ID 1. Coda-banner input is excluded because percussion
maps begin in Finale 3.5.

The refreshed tracked capture contains 247 distinct documents from `tracked-evidence` and
`rpatters1-private-evidence`. All sources and companions read successfully, with zero unexpected
differences. The four controlled uncompressed maps now contribute 512 equal leaves; 84,153
unreferenced companion rows are filtered. The represented DCL documents do not have an active
`DS` attachment, including `tests/evidence/F2006/F2006-linked-tiff.mus`, so their standalone `DF`
libraries are deliberately left unconstructed.

The refreshed 85-document focused `rpatters1-main` cohort contains 83 readable sources and 78
successful companion comparisons. Seven active uncompressed maps contribute 856 equal leaves.
Two non-percussion staff documents retain stale source `DS` records that Finale drops; comparing
their 16 selected source rows now produces 128 source-only leaves per document. Their unreferenced
companion maps are classified as synthesis. The cohort has no unexpected findings.

## 2026-09-08 — Finale 2008 precursor classes

**Question.** Does Finale 2008 store editable map notation data, and how does its staff assignment
select a row before native `PercussionNoteInfo` exists?

**Method.** The controlled pair
`tests/evidence/F2008/F2008-percussion-staff.mus` and
`tests/evidence/F2008/F2008-percussion-staff-edit.mus` was created from the same Finale 2008
baseline. The second document changes MIDI-60's staff position and two noteheads, then assigns that
row to staff 1. Complete decompressed blocks were compared after the normalized record dump failed
to show the map edits.

**Result.** Class-detail `0x040e` contains 128 map-1 records whose comparators and five-word
payloads are identical to fixed `DF`. Its MIDI-60 row changes from `(60, 0, 207, 250, 0)` to
`(60, 6, 208, 194, 0)`. Class-other `0x0084` changes payload word 4 from zero to `0x1000`, selecting
input key 60 through the same LSB-first bitmap used by fixed `DS`. Finale 27 emits one custom map
row with type 32, staff position 6, closed notehead 208, and the three open-derived noteheads 194.
The reader reproduces all eight comparison leaves for that row.

The initial record dump omitted the entire big-endian class-detail block because its decoder read
a ten-byte header and a 16-bit length. Both byte orders actually use a twelve-byte header and a
32-bit length after the two comparators and part ID. Correcting the shared framing exposes all 135
class-detail records in the edited fixture, including the 128 percussion rows.

The refreshed tracked capture compared 249 public and private controlled occurrences representing
247 distinct sources, all with successful source and companion reads. The edited row contributes
eight equal leaves. Treating a zlib source-only `DS` map number as dormant removes the unassigned
baseline's remaining 376 companion-only leaves; the class has 520 equal leaves and no other
findings. The capture has zero unexpected differences across all registered classes.

## 2026-09-08 — registered-corpus precursor validation

**Question.** How much active Finale 2007–2008 map data does the zlib precursor decoder recover
outside the controlled fixtures?

**Method.** The instrumented Release probe captured the four registered surveys
`rpatters1-installs`, `rpatters1-main`, `rpatters1-private-evidence`, and `tracked-evidence` with
the current map filter. The 16,346 occurrences represent 7,309 distinct sources; 16,257 source
reads succeeded, 89 known non-document or LIB inputs failed, and all 4,848 available companions
read successfully. Counts below compare this capture with the immediately preceding 16,344-row
capture after removing the two newly added controlled Finale 2008 fixtures.

**Result. Strong.** The decoder exposes 473 additional legacy `PercussionNoteInfo` rows. Of these,
319 replace 2,552 companion-only leaves with 2,238 equal and 314 unexpected leaves; the other 154
produce 1,232 reader-only score leaves because Finale 27 did not retain them. The paired leaves are
therefore 87.7% equal. Another 22 distinct zlib documents contain source-only rows. The controlled
edited fixture remains fully equal, while its unassigned baseline remains empty.

A follow-up extracted both percussion snapshots for the 42 distinct documents contributing all
2,971 unexpected `PercussionNoteInfo` leaves. Every leaf was paired under the same numeric `cmper`,
although the same staff's `DrumStaff.whichDrumLib` linked the legacy map to a different modern map,
linked one legacy map to several modern maps, or had no companion staff counterpart:

| Saving product | Source version | Documents | Unexpected leaves | Renumbered | Split | No counterpart |
|---|---|---:|---:|---:|---:|---:|
| 97 | 3.8.0.7 | 4 | 492 | 244 | 248 | 0 |
| 98 | 4.0.0.10 | 7 | 809 | 759 | 50 | 0 |
| 2000 | 5.0.0.5 | 1 | 24 | 24 | 0 | 0 |
| 2001 | 6.0.0.7 | 9 | 1,250 | 1,250 | 0 | 0 |
| 2006 | 11.0.2.4 | 1 | 82 | 19 | 0 | 63 |
| 2007 | 12.0.1.27 | 4 | 37 | 25 | 12 | 0 |
| 2008 | 13.0.1.66 | 16 | 277 | 195 | 61 | 21 |
| **Total** | | **42** | **2,971** | **2,516** | **371** | **84** |

The first analysis assigned each legacy row a type through musxdom's `generalMidi` field and then
paired rows by that derived type. That works for the represented General MIDI-compatible maps, but
it is not a general legacy upgrade rule. The Basic Orch Percussion Finale Edition map provides a
direct counterexample: legacy MIDI 59 is Snare Drum LH and upgrades to type 236, while the General
MIDI fallback calls MIDI 59 Shaker. The earlier claims that `percNoteType` was stable and that some
Finale 2008 rows reordered are therefore refuted.

The comparison preparer maps source and companion maps through matching `DrumStaff` records and
duplicates its source comparison view when Finale splits a shared map. For one-to-one maps with
equal row counts, it pairs rows by incidence order and reports differing derived types as
`legacy-percussion-general-midi-fallback`. Maps and rows without a counterpart receive distinct
comparison identities so they remain source-only. These changes affect only comparison snapshots,
not imported object identities. A source-defined native map remains eligible for same-ID
comparison even when no staff references it.

The all-corpus capture that reduced 3,091 unexpected findings to 15 was stale because it used the
refuted General MIDI pairing. All 15 apparent residuals belonged to `mus-2efe626e25523012`, where
staff 21 maps legacy map 26 to companion map 1. The source and companion maps each have 26 rows in
the same incidence order. Pairing those rows correctly produces 182 equal notation and identity
leaves, 26 expected map-specific type translations, and zero unexpected differences. For example,
legacy MIDI 85 maps to companion type 5, while MIDI 95 and 96 map to types 8 and 9. The prior
comparison instead paired those types with General MIDI keys 57, 80, and 81.

The corrected all-corpus capture retained the same 16,346 occurrences, 16,257 successful source
reads, 89 known invalid or LIB inputs, and 4,848 successful companions. `PercussionNoteInfo` now
has 1,780,638 equal leaves, 26 `legacy-percussion-general-midi-fallback` leaves, zero unexpected
differences, 5,568 reader-only score leaves, and 1,032 companion-only leaves. The 26 expected
differences are the map-specific percussion types in the same Finale 2008 document. Across all
classes, the capture has zero unexpected differences. **Strong.** Observed across
`rpatters1-installs`, `rpatters1-main`, `rpatters1-private-evidence`, and `tracked-evidence`.

The refreshed controlled-evidence capture still compares 249 source/companion occurrences with no
unexpected findings. Its represented maps do not exercise a differing map-specific type.

The result supports preserving attached legacy rows, including rows Finale drops. Map-specific
type translation is an accepted General MIDI approximation unless licensing circumstances change.
Note-entry attachment remains unresolved, so the class stays partial.

## 2026-09-08 — externally supplied MIDI annotation maps

**Question.** Can Finale's installed MIDI Device Annotation files supply the map-specific
MIDI-to-`PercNoteType` translation without bundling those files in this repository?

**Result. Confirmed for the counterexample.** The four Finale 27 annotation files examined contain
38 named `NoteNameList` tables. The Garritan `Basic Orch Percussion` table has 26 MIDI/type rows,
and all 26 agree with the Finale 27 companion for `mus-2efe626e25523012`, including MIDI 57 to type
279, MIDI 59 to type 236, MIDI 80 to type 135, MIDI 81 to type 136, MIDI 85 to type 5, MIDI 95 to
type 8, and MIDI 96 to type 9. Removing the suffixes `Finale Edition` and `GPO Finale Edition` from
the legacy and companion map names yields that table name. The same normalization produces no
collisions among the 38 examined lists.

Selector `DL` supplies the legacy map name and shares the numeric map identity used by `DS` and
`DF`. A caller-supplied annotation document can therefore translate each selected `DF` playback
MIDI value after resolving the normalized `DL` name. A missing name or note retains the General
MIDI fallback. The installed annotation files remain external inputs; their tables must not be
copied into the MIT-licensed repository.

The installed `General MIDI` table has 66 rows over 65 MIDI numbers. The reader's inferred mapping
agrees with the first XML row for 61 of those numbers. It disagrees at MIDI 27, 89, 90, and 92,
and it cannot reproduce the second MIDI 38 row, which maps to Snare Roll type 277. Before this
change, the inference admitted reserved custom types 3968 through 4095; their `generalMidi` fields
enumerate custom slots and caused spurious mappings for every MIDI value from 0 through 127. The
fallback now excludes those types. An externally supplied General MIDI annotation table can also
retain Finale's nonstandard extension rows; when MIDI numbers repeat, the first XML row wins.

**Implementation and validation.** The reusable reader accepts any number of annotation XML byte
buffers together with `MacSymbolFonts.txt`, parses both resources once through the caller's
`IXmlDocument` implementation, and retains the first table for each normalized name. Invalid note
types are skipped, and the first row wins when a table repeats a MIDI number because the legacy
row has no further discriminator. The probe exposes the XML inputs as a repeatable option.

The established 85-document orchestral cohort had 83 readable sources and 78 successful companion
comparisons. It produced 63,416 equal `PercussionNoteInfo` leaves and no expected or unexpected
differences. The subsequent full capture contained 16,346 occurrences representing 7,309 distinct
sources: 16,257 source reads succeeded, the 89 failures were known invalid or LIB inputs, and all
4,848 companions succeeded. `PercussionNoteInfo` produced 1,780,432 equal leaves, zero expected or
unexpected differences, 5,352 reader-only score leaves, and 1,264 companion-only leaves. Every
comparison pool had zero unexpected differences. **Strong.** Observed across
`rpatters1-installs`, `rpatters1-main`, `rpatters1-private-evidence`, and `tracked-evidence`.
