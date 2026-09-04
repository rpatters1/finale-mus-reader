# PartGlobals

**Covers:** The pre-zlib PartGlobals option locations, zlib class `0x0120`, part ownership,
and the two synthesized view-list values.
**Read when:** Working on part-specific global settings, concert-pitch display, or linked-part
PartGlobals recovery.
**Confidence:** `confirmed` for the zlib layout and `showTransposed`; `strong` for the pre-zlib
special-extraction location; intentional reader policy for the two pre-zlib view-list values.

## Identity and ownership

`PartGlobals` always uses the globals comparator `65534`. It is score-owned before linked parts
exist. In the zlib epoch, the score and each stored linked-part override have separate class
records; the linked-part instances use `ShareMode::None`.

The plug-in selector `PG` names the logical class but is not a literal pre-zlib record tag in the
controlled ETF evidence. The earlier physical values are split between numeric global selectors.
The related historical mapping is preserved in
[`LEGACY_OPTION_MAPPINGS.md`](../../reference/LEGACY_OPTION_MAPPINGS.md).

## Physical layouts

| Epoch | Source | `showTransposed` | `scrollViewIUlist` | `studioViewIUlist` | `specialPartExtractionIUList` |
|---|---|---|---|---|---|
| Coda banner | numeric globals | selector 12 word 0 | synthesized `BASE_SYSTEM_ID` | synthesized `STUDIO_VIEW_SYSTEM_ID` | selector 23 word 4 |
| Uncompressed | numeric globals | selector 12 word 0 | synthesized `BASE_SYSTEM_ID` | synthesized `STUDIO_VIEW_SYSTEM_ID` | selector 23 word 4 |
| DCL | numeric globals | selector 12 word 0 | synthesized `BASE_SYSTEM_ID` | synthesized `STUDIO_VIEW_SYSTEM_ID` | selector 23 word 4 |
| Zlib | class `0x0120` | byte 0 | byte 2 | byte 4 | byte 6 |

The zlib payload is 12 bytes: four words in musxdom member order followed by two unused words.
Every recovered zlib field is reported as `LegacyMus`. The two synthesized pre-zlib view-list
members are reported as `LegacyBehavior`; the other two members retain their numeric-global
source offsets and identities.

## Scroll View state is intentionally not preserved before zlib

The controlled Coda pair
`tests/evidence/F100/F100-quartet.mus` and
`tests/evidence/F100/F100-quartet-oboeview.mus` differs by an `IU(65531)` family selecting the
oboe in Scroll View. Both Finale 27 companions omit `scrollViewIUlist`. This is document-specific
UI cache state rather than score semantics, so the reader deliberately supplies
`BASE_SYSTEM_ID` throughout the pre-zlib epochs instead of importing that family.

## Missing linked-part records

musxdom completes any `PartDefinition` lacking PartGlobals with an unshared instance. Its
synthesized values are `showTransposed = true`, `BASE_SYSTEM_ID`, `STUDIO_VIEW_SYSTEM_ID`, and a
zero special-extraction list. This matches the Finale 27 companions of the controlled Finale 2012
linked-part fixtures; the reader does not manufacture a physical zlib record or mark those values
as recovered source fields.

## Coverage

**Confirmed.** The 2026-09-04 tracked capture imported all 223 occurrences (221 distinct source
contents) and compared 226 PartGlobals instances, totaling 1,130 matching identity/member leaves
with no differences. The authorized all-corpus capture covered all three registered surveys:
16,320 occurrences, 16,231 successful imports, and 4,631 successful companion comparisons. Its
6,711 compared PartGlobals instances produced 33,555 matching identity/member leaves and no
unexpected, expected, reader-only, or companion-only differences.

All four persisted musxdom members are accounted for in every epoch. None is `Unmapped` or
`MusxOnly`.
