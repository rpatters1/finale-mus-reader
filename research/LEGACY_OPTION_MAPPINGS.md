# PDK Framework Option Mappings

## Scope and provenance

This note records interoperability facts distilled from two privately supplied histories of Jari Williamsson's PDK
Framework. The sources were inspected read-only with the owner's authorization. No framework source, declarations,
comments, or implementation code is stored in this repository.

The principal snapshot used for the published mapping is `RGPPDKFramework` commit
`44650a9a11cc8a5f86628b52a1ae75cc523a19a6`. Historical comparison used `JWPDKFramework` commit
`d8a4c7782a7213bfd7350e3f03976b12afb1d2ab` and its initial source import at commit
`37326071691ba6ce67a4c894ec3c5a0a616ab434`. The inspected `finaleframework.cpp` was unmodified in each snapshot;
unrelated local changes in the historical worktree were not consulted.

These findings are labeled **private-framework-derived**. They are not clean-room findings and should not be
described as public-PDK-derived. The provenance is historically mixed: the compatibility routines are described as
originating in `finalepdk.cpp`, while the user reports that Jari wrote routines later adopted by MakeMusic. This note
makes no ownership determination. It records only the field-location facts needed to read user-owned documents.

The complete distilled field table is
[`data/legacy_option_mappings.csv`](data/legacy_option_mappings.csv). Direct multi-incidence option
blocks that do not use the field map are cataloged in
[`data/legacy_direct_option_blocks.csv`](data/legacy_direct_option_blocks.csv). These are factual
schemas, not translations of the C++ source.

## Central finding

Before Finale 26.2 exposed regular preference structures, the framework reconstructed synthetic preference objects
from ordinary Enigma records. Numeric tags such as `^94` normally select comparator `65534`; this is the same form
printed by ETF as `^94(65534)`. Each incidence supplies six 16-bit payload words. A mapping identifies:

- the two-character tag or numeric global number;
- comparator and incidence;
- a zero-based word slot within the six-word payload;
- a one-, two-, or four-byte value width;
- any word-order or boolean conversion;
- a semantic field and preference group; and
- for a few fields, a minimum Finale version.

This explains the otherwise opaque ETF `^NN(65534)` records. They are not arbitrary blobs: many are shared backing
records whose individual words feed fields in several logical options objects. Conversely, one logical options object
can draw fields from many global numbers and incidences. A reader must therefore index physical option records first
and assemble logical musxdom options afterward.

The framework's four-byte conversion rules distinguish two word orders:

- `MACFOURBYTE`: first payload word is the high word and the next is the low word;
- `WINFOURBYTE`: first payload word is the low word and the next is the high word.

It also identifies an inverted-boolean conversion. One-byte fields remain a binary-validation target because the
framework selects them through a 16-bit payload slot and then narrows the value.

## Mapping inventory

The current snapshot yields 435 mapping rows across 24 groups: 424 baseline rows and 11 Finale 26.2 compatibility
overrides. The original branch preserves two legacy locations that no longer occur in the current table, making the
published union 437 rows. The union contains 385 two-byte fields, 36 four-byte fields, and 16 one-byte fields. It
references 61 distinct numeric globals and nine nonnumeric tags.

The available ETF set contains 59 of the 61 numeric global selectors; only `^47(65534)` and `^48(65534)` are absent.
Across numeric and nonnumeric selectors, 386 of 437 published rows refer to a selector observed in at least one available ETF.
This verifies selector existence only. It does not independently prove the semantic field, slot, width, signedness, or
conversion of any individual row. ETFs also contain 35 numeric `^NN(65534)` selectors not covered by the framework
table, so this is substantial partial coverage rather than a complete options map.

| Group | Rows | Principal selectors | Candidate musxdom target |
|---|---:|---|---|
| `ChordPrefs` | 14 | `^37`, `^41`, `^44`, `^45`, `^76` | `ChordOptions` |
| `DistancePrefs` | 44 | 14 numeric globals plus `^OA(0)` | Multiple geometry option classes |
| `ExtraTupletPrefs` | 4 | `^14`, `^23`, `^69` | `TupletOptions` |
| Group/staff-name positions | 12 | `^04`, `^66`, `^79`, `^80` | `StaffOptions` |
| `LayoutDistancePrefs` | 6 | `^01`, `^76`, `^77`, `^93` | `PageFormatOptions` or `MiscOptions` |
| `LyricsPrefs` | 19 | `^15`, `^35`, `^57`, `^67`, `^87`, `^OL(2)` | `LyricOptions` |
| `MMRestDefaultsPrefs` | 13 | `^25`, `^83` | `MultimeasureRestOptions` |
| `MiscDocPrefs` | 50 | 20 numeric globals plus five named records | `MiscOptions` and related classes |
| Music-character groups | 11 | `^11`, `^12`, `^19`, `^22`, `^69` | `MusicSymbolOptions` |
| `MusicSpacingPrefs` | 17 | `^41`, `^94` | `MusicSpacingOptions` |
| Page-format score/parts | 44 | score globals and `^77` for parts | `PageFormatOptions` |
| `PartExtractPrefs` | 25 | `^77` | Page format or unsupported extraction policy |
| `PartScopePrefs` | 1 | `^PG(65534)` | Part metadata or `MiscOptions` |
| `PianoBracePrefs` | 11 | `^45`, `^60`, `^61`, `^64`, `^65` | `PianoBraceBracketOptions` |
| `PlaybackPrefs` | 53 | five numeric globals plus `^FI`, `^OP`, and `^pd` | Currently unsupported playback/document settings |
| `RepeatPrefs` | 25 | `^05`, `^20`, `^69`–`^72`, `^76` | `RepeatOptions` |
| `SizePrefs` | 21 | 14 numeric globals | Multiple size-related option classes |
| `SmartShapePrefs` | 39 | `^50`, `^51`, `^53`, `^92`, `^93`, `^97`, `^FI` | `SmartShapeOptions` or `LineCurveOptions` |
| `TiePrefs` | 26 | `^41`, `^84`, `^97` | `TieOptions` |

The candidate musxdom targets are routing guidance, not verified one-to-one class mappings. Aggregates such as
`DistancePrefs`, `SizePrefs`, and `MiscDocPrefs` distribute fields across multiple modern options classes.

## Historical differences

The `RGP-JWOriginalCleanup` branch at `982939e1c14b4dfcb9fe73ce2369fdd77e88392f` preserves Jari's original
pre-Finale-2014-oriented mapping line: 343 rows in 22 groups. Of those, 341 remain byte-for-byte equivalent as factual
mapping tuples in the current table. The 2022 historical head contained 373 rows, and the current snapshot contains
435. Later work primarily expanded playback data, added chord preferences, and added a small number of newer Finale
fields and compatibility overrides. Current-only rows are not automatically applicable to old files merely because
they were discovered later.

Two directly relevant original-branch locations no longer appear in the current table. `specialExtractedPartCmper`
was originally mapped to numeric
global `^23(65534)`, incidence 0, word slot 4. A later version moved it to `^PG(65534)`, incidence 0, word slot 3.
The original `score_in_c` mapping used `^PG(0)`, incidence 0, word slot 0 with inverted-boolean interpretation; the
current mapping uses `^PG(65534)` at the same incidence and slot. These are probably version-dependent and must not
be resolved by blindly applying the newest table to every MUS era. The CSV preserves all alternatives with an
explicit `mapping_lineage` column.

Eleven current rows are runtime replacements used by the framework's Finale 26.2 compatibility path. They relocate
selected distance, lyrics, playback, and miscellaneous-document fields to named records such as `OA`, `OL`, `OP`,
`OY`, and `pd`. These overrides describe the newer compatibility layer; the baseline numeric-global locations are the
starting point for Finale 2001–2006 MUS files.

## Direct multi-incidence option blocks

Five legacy option structures bypass the synthetic field map but still have explicit physical locations:

| Logical structure | Legacy selector | Organization | Candidate target |
|---|---|---|---|
| Slur contours | `^52(65534)` | two fixed incidences | `SmartShapeOptions` or `LineCurveOptions` |
| Tie placement | `^85(65534)` | four fixed incidences | `TieOptions` |
| Tie contours | `^86(65534)` | five fixed incidences | `TieOptions` |
| Grids and guides | `^88(65534)` | eleven fixed incidences | currently unsupported document settings |
| Stem connections | `^40(65534)` | variable incidence collection | `StemOptions` |

All five selectors occur in the available ETF evidence. Their internal field layouts remain source-derived. Stem
connection elements use one layout before Finale 2012 and a different layout from Finale 2012 onward, an explicit
format-era boundary that a reader must honor.

## Consequences for the reader

The options problem is no longer wholly unmapped. A practical first implementation can:

1. create every musxdom options instance from a pinned Finale 27 new-document-without-libraries baseline;
2. decode and index every available legacy option record by `(tag, cmper, incident)`;
3. apply only table rows appropriate to the classified Finale era and platform;
4. overlay confidently decoded values into the candidate musxdom classes;
5. retain baseline defaults for absent, unverified, or unsupported fields; and
6. emit a capability report distinguishing recovered values from synthesized defaults.

The mapping must be data-driven and versioned. It should not be compiled into assumptions that every table row works
unchanged across Finale 2000, 2001–2006, 2007–2012, and the 26.2 compatibility API.

## Corpus verification of promoted mappings

**Confirmed** by running the reader over all 1,218 direct corpus files, last measured
2026-08-09. Eight option mappings are promoted: four `MusicSpacingOptions` fields from selector
`94(65534)` and the four `layerAtts.restOffset` values from `LA`. Font definitions are recovered
separately and are not counted here; see
[FORMAT_NOTES.md](FORMAT_NOTES.md#font-definitions).

| Era | Files | Music spacing 4/4 | Layer offsets 4/4 |
|---|---:|---:|---:|
| Coda banner, 1.8.7-2.6 | 54 | 0 | 0 |
| Finale 3.0-3.7 | 28 | 0 | 28 |
| Finale 97 | 70 | 0 | 60 |
| Finale 2000 | 92 | 90 | 92 |
| Finale 2001-2006 | 426 | 426 | 426 |

Three era facts follow:

- **Every framed DCL file recovers all eight values.** The sixteen that previously did not were
  failing on tag byte order, not on framing.
- **`LA` is present from at least Finale 3.0** and absent from the Coda-banner era. Ten Finale 97
  documents carry no `LA` records at all.
- **Selector `94` is not observed before Finale 2000.** Finale 3.0 through 97 recover layer
  offsets and no spacing values, and the Finale 1.8.7 file `mus-7aa45639c14b3864` carries
  comparator 65534 records under many other selectors but not `94`. The introduction point lies
  after Finale 97, internal 3.8, and no later than Finale 2000, internal 5.0. Finale 98, internal
  major 4, would settle it but is absent from the corpus and its release notes are not online.

No version gate was added for the spacing mappings. A gate protects against reading a field that
*moved*; a field that is simply absent needs none, because the record lookup fails and the value
correctly reports as a synthesized default. Adding one would encode an unverified boundary while
changing no behavior. Font definitions are gated, because there the layout genuinely differs: files
before Finale 3.2 carry no header incidence.

This measures recovery, not accuracy. Only the fixtures with ETF counterparts independently confirm
that the recovered values are correct.

## Confidence and validation plan

| Claim | Status |
|---|---|
| Numeric tags with implicit comparator select `65534` | private-framework-derived; strongly ETF-supported |
| An incidence contains six 16-bit payload words | private-framework-derived; consistent with known 12-byte payload rows |
| Selector exists in ETF | independently text-verified where CSV column says `yes` |
| Semantic field and word slot | private-framework-derived; generally not independently verified |
| Four-byte word-order conversions | private-framework-derived; binary verification pending |
| Finale 26.2 replacement locations | private-framework-derived; outside the immediate 2001–2006 target |
| Candidate musxdom class | architectural inference; field-by-field verification pending |

Highest-value verification is a controlled Finale 2000 or 2005 document in which one visible option is changed at a
time and both MUS and ETF are saved. Tests should begin with `MusicSpacingPrefs`, `TiePrefs`, `PageFormatPrefsScore`,
`RepeatPrefs`, and `ChordPrefs`: each has a direct musxdom destination and compact, well-localized selectors.
