# ShapeDef, ShapeData, ShapeInstructionList

**Covers:** Shape definition records, their instruction streams, and their data.
**Read when:** Working on shape definitions or shape-driven smart shapes.
**Confidence:** confirmed for the mapped structure.

## Shape definitions, instructions, and data

**Confirmed for the controlled Coda, fixed-row, DCL, and zlib fixtures; strong across the broad
corpus sweep.** `ShapeDef` uses `SD`, with instruction/data families `SL`/`SB` in the Coda era and
`sL`/`sb` from Finale 3 through 2006. Zlib class ids `0x00d6`, `0x00d7`, and `0x00d5` carry the same
three families respectively. Each fixed instruction or data row holds three signed 32-bit values;
the two normalized 16-bit words composing a long are high-word first on Mac and low-word first on
Windows. Zlib payloads use the container byte order directly.

The instruction long is `revision:numData:tag`, with one byte each for revision and data count and
the low two bytes holding the two-character instruction tag. A zero long terminates the logical
list. This is not padding that may be skipped: real files retain nonzero stale instructions after
zero, while their exact Finale 27 upgrades omit that tail, and consuming it makes the instruction
data count impossible. The importer recognizes the instruction tags represented by musxdom's
`ShapeDefInstructionType`; revision-1 `sw` becomes `LineWidth`, with its data converted from
hundredths of a point to Efix, and the Coda meaning of `gs` becomes `GoToOrigin`.

The first two `SD` words are the instruction-list and data-list comparators. Finale 3-2006
fixed rows store modern `ShapeType` in word 2. **The earlier claim that zlib dropped this field
was wrong:** every controlled 12-byte `0x00d6` record stores `ShapeType` in word 2, in both byte
orders, and its value agrees with the Finale 27 companion for Other, Expression, Arrowhead, and
Clef. Its last three words are zero. The tracked companion cohort contains 38 nonzero examples:
36 Clef, one Expression, and one Arrowhead.

The broader installed zlib corpus has nonzero values in the last three words and sometimes a word
2 outside the enum. The importer reads an enum-valued word 2 and otherwise retains `Other`; the
remaining words are opaque. Some class records have 24, 36, or 60-byte payloads containing
multiple six-word units. **Open:** the meaning of those trailing fields and the aggregated class-
record form. Only the first unit currently supplies the definition named by the class-record
header. Coda `SD` remains bounds-bearing and supplies `Other`.

The `shape_definitions` sweep selected all 15,841 inventoried occurrences in `rpatters1-main` and
`rpatters1-installs`, de-duplicated to 6,890 content identities. All 6,890 imported. Of 364,482
source definitions, 4,816 stored zero list references, 34 stored nonzero references to resolved
empty instruction lists, and 359,632 nonblank definitions resolved both supporting collections;
none of those nonblank shapes consumed more data than was stored, and none retained an undocumented
opcode after applying the zero terminator. Eight Finale 2.2 Windows files contributed 32 of the
resolved-empty definitions: their referenced `SL` families terminate immediately and their `SB`
families are absent. Two Finale 2.6 files contributed one resolved-empty definition each. Finale's
UI displays the Windows shapes as blank. The reader preserves their references and bounds, and
musxdom recognizes a resolved empty instruction list as `Blank`; neither layer has to discard
source data to obtain the source application's semantics.

Another 41 selected identities do not classify as legacy score containers and recover no source
shape definitions. Private location metadata strongly identifies these as library artifacts rather
than an uncovered score epoch: 36 occur in library locations, while their suffixes are ten `.lib`,
29 extensionless names, and two `.mus` names. They remain in the all-files denominator so that a
misnamed library cannot silently become a successful score import, but they are not evidence of a
ShapeDef layout missing from a recognized score container.

All eight adjacent-exact Finale 27 companions instead synthesize the same visible rectangle
instruction sequence and data from the old `SD` bounds. That is deliberate upgrade behavior, not
source content: reproducing it would make the imported shapes nonblank when the source application
shows them blank. Companion comparison must therefore classify these 32 shapes as `synthesized`
rather than as a reader discrepancy.
Thirty-one recovered instructions reference graphics. `ShapeGraphicAssign` uses fixed-row tag
`sg` through Finale 2006 and zlib class `0x00d8`; its payload is the same 18-word placement tuple
described for page graphics below. Thirty of the 31 instruction operands resolve through
musxdom's assignment lookup. The remaining DCL source names graphic 3 but defines assignments only
for graphics 1 and 2, so the dangling reference is source content rather than a dropped record.
**Confirmed** across `rpatters1-main` and `rpatters1-installs`.

The tag spellings and revision-1 conversion agree with Finale 2000 PDK `SHAPETAG.H` and `edata.h`
at immutable commit
[`9f74ba9b3e287f240bbd454c2259fc3f7737c6ad`](https://github.com/rpatters1/finale-pdk-framework/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/SHAPETAG.H)
(accessed 2026-08-13). Those names are **public-PDK-derived**; the binary packing, byte order,
terminator, epoch layouts, and semantic conversions were checked independently against the
controlled fixtures and corpus observations.
