# PercussionNoteInfo

**Covers:** Legacy percussion-map note records, their upgrade, and notehead conversion.
**Read when:** Working on percussion maps or exporting their note types and noteheads.
**Confidence:** `confirmed` for the observed layouts; `strong` for the Finale 3.5 introduction
boundary and the pre-2010 upgrade and attachment rules.

## Identity and availability

Percussion maps begin in Finale 3.5. The Coda-banner epoch predates them and is intentionally
excluded from recovery.

The native zlib class is `0x0139`. Its comparator identifies a percussion map; each packed
element becomes a note incidence. The logical plug-in selector is `Di`.

The registered `rpatters1-installs` and `rpatters1-main` catalogs first contain class `0x0139` in
Finale 2010 documents. The catalogs contain no occurrence in their earlier represented releases,
and direct checks of controlled and installation specimens from Finale 1.0 through 2009 found
none. Finale 2010 therefore introduced the modern stored class.

The precursor is detail selector `DF`. `cmper1` identifies a legacy map and `cmper2` is its input
MIDI key. Its five words are playback MIDI note, signed staff position, closed notehead, open
notehead, and an unused zero. Input and playback MIDI can differ. Uncompressed and DCL files store
the fixed detail directly. Pre-2010 zlib files retain the same identity and five-word payload as
class-detail `0x040e`.

Before the native map class, other selector `DS` attaches a staff to a map and selects the map
rows usable on that staff. Zlib files retain it as class-other `0x0084`. Its `cmper` is the staff
ID. Concatenate its incidences in order:
the first word is the `DF.cmper1` map ID, and every following word contributes 16 LSB-first bits
indexed by `DF.cmper2`. The fixed-row importer applies this layout identically to uncompressed and
DCL containers. It constructs the union of rows selected by every staff that names a map, in
ascending input-key order. This explains why a row that resembles filler can still be relevant and
why some visibly customized rows are omitted: the selection bitmap, rather than the row values,
determines membership. The record layout is documented with its own class in
[`drum_staff.md`](drum_staff.md).

For observed General MIDI-compatible maps, the playback MIDI note selects the base musxdom
percussion note type with that `generalMidi` value. This is only a fallback for legacy maps whose
map-specific MIDI-to-type table is unknown. The Basic Orch Percussion Finale Edition map is a
confirmed counterexample: its input keys do not denote General MIDI instruments. The reader still
uses the fallback so its synthesized note assignments reach the correct recovered row, but the
resulting `percNoteType` does not identify the original instrument. Repeated selected rows with the
same fallback type receive successive order IDs in the high nibble. Closed and staff values carry
over, while the old open notehead supplies half, whole, and double-whole noteheads. Characters use
the document's Percussion font.

Finale's note-level assignment is documented with its detail class in
[`percussion_note_code.md`](../details/percussion_note_code.md).

Finale 27 discards the examined Finale 98 maps and synthesizes its contemporary canonical library.
That is evidence about Finale's converter, not evidence that the early maps are semantically
useless to this reader.

## Modern layouts

Each class payload packs a sequence of elements in musxdom member order:

| Field | Finale 2010–2011 offset | Finale 2012 offset | Type |
|---|---:|---:|---|
| `percNoteType` | 0 | 0 | unsigned word |
| `staffPosition` | 2 | 2 | signed word |
| `closedNotehead` | 4 | 4 | byte-in-word / low-word-first `char32_t` |
| `halfNotehead` | 6 | 8 | byte-in-word / low-word-first `char32_t` |
| `wholeNotehead` | 8 | 12 | byte-in-word / low-word-first `char32_t` |
| `dwholeNotehead` | 10 | 16 | byte-in-word / low-word-first `char32_t` |

The narrow element is 12 bytes. The Finale 2012 element is 24 bytes: 20 bytes of fields followed
by four zero bytes. A non-multiple payload or nonzero Finale 2012 padding is diagnosed; complete
elements remain recoverable.

Before Finale 2012, each notehead byte is decoded with the document's Percussion font from
`FontOptions`. Unresolved fonts use the symbol fallback. Finale 2012 stores the four noteheads as
32-bit codepoints, so they require no font-dependent byte conversion.

## Coverage

The 2026-09-08 tracked capture compared 249 public and private controlled occurrences, representing
247 distinct sources, with companions: 81 Coda-banner, 71 DCL, 55 uncompressed, and 42 zlib. All
source and companion reads succeeded, and no comparison reported an unexpected finding. The
comparison preparation retained every map defined by the source and every map referenced by a
companion `DrumStaff`, then removed 84,935 other companion note rows as Finale-synthesized
percussion-map notes. A zlib source-only
`DrumStaff` reference does not retain a companion map because `DS` can name a dormant map there;
any active precursor rows are already present in the recovered source map.

The corrected all-corpus capture compared 16,346 occurrences representing 7,309 distinct sources.
It produced 1,780,638 equal `PercussionNoteInfo` leaves, 26 expected map-specific type differences,
zero unexpected differences, 5,568 reader-only score leaves, and 1,032 companion-only leaves.
**Strong.** Observed across `rpatters1-installs`, `rpatters1-main`,
`rpatters1-private-evidence`, and `tracked-evidence`.

Four uncompressed controlled documents contributed 512 equal leaves from 64 selected `DF` rows,
and the controlled Finale 2008 edit contributed eight equal leaves from its selected `0x040e`
row. The represented DCL sources have no active `DS` attachment, so their unreferenced `DF`
libraries are not constructed. All unrelated canonical-library leaves were removed. The ten
tracked Finale 2011 fixtures and twenty-one tracked Finale 2012 fixtures have no `0x0139` class in
either the source or companion; stock post-2010 documents therefore do not necessarily store the
canonical maps.

The field layouts are independently binary-verified against `mus-325173b1552c944c` for the narrow
form and `mus-4c75dfe49da42598` for the wide form. In seven uncompressed `rpatters1-main`
documents, all 107 upgraded map rows exactly followed the `DS` selection, General MIDI identity,
duplicate order, staff position, and noteheads; all 4,769 generated note attachments had upgraded
pitches matching the selected row's playback MIDI value. The current reader reproduces all 856
comparison leaves for those rows. The controlled input-key discriminator and exact assignment
census are documented in
[`percussion_note_code.md`](../../investigations/percussion_note_code.md).

A Finale 27 upgrade does not preserve a legacy map's numeric `cmper`: it may renumber the map or
split one map shared by several staves into several modern maps. **Strong.** Companion comparison
must associate maps through each staff's `DrumStaff` record. When one source map and target map
contain the same number of rows, their incidence order preserves the row correspondence even when
Finale assigns map-specific percussion types. Those differences are classified as the deliberate
legacy General MIDI fallback rather than as pending recovery. Unequal and split maps remain less
certain. The
coverage preparer applies this association only to its comparison view; it does not change the
imported map or row identities. The map-specific type translation and note-entry attachment remain
unrecovered, so the class stays partial. The aggregate evidence is documented in
[`percussion_note_info.md`](../../investigations/percussion_note_info.md).
