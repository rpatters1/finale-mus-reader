# LayerAttributes

**Covers:** The `LA` layer-attribute record, its flag word, when a release writes it, what a layer with no record means, and how the four seeded objects are overlaid.
**Read when:** Working on layer attributes or interpreting their coverage numbers.
**Confidence:** `confirmed` throughout; the class agrees with its companion across the whole tracked survey.

## The record

**Confirmed.** Layer attributes are ordinary other records, not numeric globals. The comparator is
the 0-based layer index, so a document that stores them at all stores four, at comparators 0
through 3.

| Epoch | Identity | Addressing |
|---|---|---|
| Coda banner, uncompressed, DCL | fixed-row tag `LA` | six-word row, one incidence |
| Zlib | class id `0x00a3` | 12-byte payload, the same six-word stream |

Two of the six words are live. Word slot 0 is the signed rest offset in staff steps; word slot 5
is a packed flag word. Slots 1 through 4 are zero in every observed document. The class record
keeps the row's word stream unchanged, so its offsets are the slots doubled: the rest offset at
byte 0 and the flag word at byte 10.

## The flag word

**Confirmed** against `tests/evidence/F97/Fin97-baseline.mus`,
`tests/evidence/F2002/F2002-baseline.mus`, `tests/evidence/F2007/F2007-lyric-hyphens.mus`, and
`tests/evidence/F2012/F2012-baseline.mus`, which between them exercise both byte orders and both
encodings. The member-to-flag correspondence is `public-PDK-derived` from `FCLayerPrefs`; the mask
values and the tag spelling are `private-framework-derived`.

| Bit | Mask | musxdom member | Finale UI |
|---:|---:|---|---|
| 0 | `0x0001` | `ignoreHiddenLayers` | Ignore Hidden Layers |
| 1 | `0x0002` | `hideLayer` | Hide Layer when Inactive |
| 7 | `0x0080` | `freezTiesToStems` | Freeze Ties in the Same Direction as Stems |
| 8 | `0x0100` | `onlyIfOtherLayersHaveNotes` | Apply Settings Only if Notes are in Other Layers |
| 9 | `0x0200` | `useRestOffset` | Adjust Floating Rests by |
| 10 | `0x0400` | `freezeStemsUp` | Freeze Stems Up, clear meaning down |
| 11 | `0x0800` | `freezeLayer` | Freeze Stems and Ties |
| 12 | `0x1000` | `playback` | Playback |
| 13 | `0x2000` | `affectSpacing` | Affect Music Spacing |
| 14 | `0x4000` | `ignoreHiddenNotesOnly` | Ignore Layers Containing Only Hidden Notes |

The two groups arrived at different times, which is why they are not adjacent. Bits 7 through 11
are the original layer dialog. Bits 0, 1, 12, 13, and 14 first carry a value in Finale 2002:
every tracked Finale 97, 98, 2000, and 2001 document holds `0x0f80`, `0x0b80`, or `0x0000` in the
flag word, and every tracked Finale 2002 and later document holds `0x7f80`, `0x7b80`, or
`0x3000`. The consequence for playback and music spacing is recorded under
[Playback and music spacing before Finale 2002](#playback-and-music-spacing-before-finale-2002)
below.

## Which releases store the record

**Confirmed.** A release writes the row only once a layer setting leaves its default.
`tests/evidence/F372/F372-layer-adjrests.mus` is a Finale 3.7.2 save whose only decoded change
from its baseline sibling is four new `LA` rows, so the row exists by 3.7.2 in exactly the layout
above; its unmodified sibling, like every other tracked Finale 3.7.2 and Coda-banner document,
carries none. That resolves the apparent conflict with the Finale 3.0-3.7 row of the corpus table
in [`../../reference/LEGACY_OPTION_MAPPINGS.md`](../../reference/LEGACY_OPTION_MAPPINGS.md):
authored documents have non-default layer settings and therefore the row, while purpose-built
fixtures saved at the defaults do not.

That same fixture fixes the era's defaults directly. Only "Adjust Floating Rests by" was enabled,
and the created rows carry only `0x0200`; had the freeze settings defaulted on, their bits would
have been written too. Whether the Coda-banner releases write the row at all is still **open** --
no document of that era carrying one has been seen, and Finale 1.0.0 has no layers to write.

## What a layer with no record means

**Confirmed** across the 98 tracked documents that store no row: 45 Finale 1.0.0, 28 Finale 2.6.3,
24 Finale 3.7.2, and one Finale 2000 re-save of a 3.7.2 document. Every one of their Finale 27
companions carries the identical set of values on all four layers, with no variation between
documents:

| member | value |
|---|---|
| `restOffset` | 0 |
| `playback`, `affectSpacing` | true |
| every other member | false |

This is era behavior rather than upgrade synthesis, and the fixture above is what distinguishes
the two: a release with no stored row behaved as though nothing was set, and it always played the
layer back and always let it affect music spacing because it had no setting for either.

The reader therefore applies those values to every layer whose record is absent. Layers 2 and 3
reach exactly what the pinned baseline already holds, so nothing is asserted and they stay
`Finale27Default`; layers 0 and 1 disagree with the baseline on seven members each -- the rest
offset and the five original dialog flags plus `ignoreHiddenNotesOnly` -- and those report
`LegacyBehavior`. The gate is the row's absence, not a version.

**Limit:** all 98 documents are big-endian macOS saves. The Coda-banner era's Windows documents,
which state no version at all, are not represented in this cohort.

## Construction and sharing

Every record identity the source carries reaches the document. The importer is record-driven, so
the generic mapping engine walks each identity the family holds — every part id, every comparator
— and asks the class what its destination is. Where the pinned Finale 27 baseline seeded an object
of that identity, the record is overlaid onto it; where it did not, a source-owned object is
created, taking its identity and share mode from the row through the same helper every other
`others` class uses.

Two consequences follow, and both are deliberate:

- **A comparator outside the modern layer range is imported, not discarded.** musxdom reads only
  layers 0 through 3 and logs the rest as extra, but a comparator beyond them is still something
  the file states, and a corrupt or unusual document that carries hundreds of them keeps them all.
  Deciding otherwise would make the reader the arbiter of what a document may contain.
- **A part-scoped row becomes a part-owned object.** Finale's own UI cannot produce one, because
  layer attributes are not unlinkable, so this is a shape no authored document should have. It
  costs nothing to handle correctly and nothing has to recognize it as a special case.

The four objects the baseline seeds are the whole of its `<others>` allowlist, described in
[`../../reference/options_fallback.md`](../../reference/options_fallback.md). They are the
identities the engine never reaches when the file stores no row for them, which is why the era
behavior above is applied by a second pass over the finished pool rather than by the tables.

## The Finale 2011 128-entry table

**Confirmed** across `rpatters1-installs`. 324 distinct Finale 2011 documents store 128 layer
records at consecutive comparators 0 through 127, all score-shared. Comparators 0 through 3 carry
that release's ordinary defaults; comparators 4 through 127 are 124 records whose every byte is
zero. Every other surveyed document of every release stores exactly four.

Finale 27 preserves all 128 on upgrade, so the extras survive a round trip rather than being
dropped: source and companion agree leaf for leaf, with nothing reader-only and nothing
companion-only. musxdom accepts the surplus and ignores everything past the modern layer range.

The population correlates with the creating release without being determined by it: every
Finale 2001-created and most Finale 2009-created document in the group carries the table, no
Finale 2010-created document does, and Finale 2009 produces both shapes. All 324 were last saved
by Finale 2011.

**Believed: a defect in Finale 2011 that later releases do not reproduce.** No Finale 2012 document
carries the table. This has not been investigated further and does not need to be — the reader
imports whatever comparators a file states, so the shape costs nothing to support.

## Playback and music spacing before Finale 2002

**Confirmed through Finale 2000; `strong` for Finale 2001.** No release before Finale 2002 offers
a setting for either: Finale 2000's layer dialog has neither control. Finale 2001's own dialog has
not been inspected, so that release rests on its stored flag words, which match every earlier
release exactly, and on its companions, which write both settings true. Such a release always
plays the layer back and always lets it affect music spacing, and bits 12 and 13 are clear in
every one of them, so reading those bits yields the opposite of what the era did. The reader
therefore declines to read them below the boundary and supplies the era's `true` instead, for all
four layers and whether or not the document carries a row.

The pinned baseline already holds `true` for both on all four layers, so nothing is asserted
against it and both report `Finale27Default`. That is the same rule the absent-record behavior
follows: `LegacyBehavior` is reserved for a value the baseline does not supply.

The gate is a version, `sourcePredatesVersion(profile, DclLegacy, finale2002)`, framed inside the
epoch it divides. The boundary falls between Finale 2001 and Finale 2002, inside the DCL epoch, so
no epoch gate can express it, and the record is six words in both layouts so it states nothing
about which one it uses. The only candidate structural marker is the bit content itself, which a
Finale 2002 user who muted every layer would reproduce exactly — a guess about contents rather
than a fact about shape. The documented cost stands: a DCL document whose version cannot be
recovered fails closed and keeps the stored bits.

## Companion agreement

With the record layout, the absent-record behavior, and the rule above in place, every document in
the tracked survey agrees with its Finale 27 companion on all eleven members of all four layers,
in every epoch and both byte orders.
