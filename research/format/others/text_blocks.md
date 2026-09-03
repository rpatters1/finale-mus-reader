# TextBlock

**Covers:** TextBlock attributes across the fixed-row and zlib epochs, including the Coda-banner `HT`/`HS` arrangement.
**Read when:** Working on text blocks or block text assembly.
**Confidence:** confirmed for the first twelve words from Finale 3.7.2 through 2012.

## TextBlock attributes

**Confirmed for the first twelve words from Finale 3.7.2 through 2012.** The fixed-row epochs
store `TextBlock` as the `TX` others family, and the zlib epoch stores the same words in class
`0x00b7`. The class payload is 36 bytes in the controlled 2008 fixture; fixed families may carry
three or four incidences. The first trailing word becomes a text-family discriminator in Finale
2004; later trailing words remain open.

| Word | musxdom field |
|---:|---|
| 0 | `textId` |
| 1 | `width` |
| 2 | `height` |
| 3 | `shapeId` |
| 4 | `lineSpacingPercentage` or `lineSpacingEvpu`, selected by flag `0x1000` |
| 5 | `xAdd` |
| 6 | `yAdd` |
| 7 | flags |
| 8–9 | `inset`, signed 32-bit high word first |
| 10–11 | `stdLineThickness`, signed 32-bit high word first |
| 12 | `textType`, when it carries a recognized text-family discriminator |

The word-7 flags are justification `0x0007`, `newPos36` `0x0008`, `showShape` `0x0200`,
`noExpandSingleWord` `0x0400`, `wordWrap` `0x0800`, and percentage line spacing `0x1000`.
Finale orders justification left, right, center, full, forced-full, while musxdom orders left,
center, right, full, forced-full; stored values 1 and 2 are therefore exchanged. Bit `0x0010`
is present in the public structure but has no `TextBlock` destination.

Finale allows absolute zero-EVPU line spacing to be selected and persists that setting, although
it has no visible effect. The controlled Finale 2005 fixture stores word 4 as zero with the
percentage flag clear, and its Finale 27 upgrade still presents zero EVPU in Finale's UI. The
upgraded EnigmaXML omits both line-spacing elements. A controlled zero-percent source produces
the same omission, and Finale 27 reopens both upgraded packages as zero EVPU. Thus omission in a
modern TextBlock means zero EVPU: absolute zero is preserved, while percentage zero is converted
to absolute zero. musxdom follows that interpretation. The importer applies the same upgrade:
it preserves stored absolute zero and converts stored percentage zero to EVPU zero, while its
recovery diagnostics retain the source word and percentage flag.

This layout is **public-PDK-derived and independently confirmed** against the controlled Finale
97, 2000, 2006, and 2008 records and their companions. The public source is
[`EDTTextBlock` in the immutable guidolib PDK snapshot](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/edata.h#L1426-L1485),
accessed 2026-08-22. The public declaration is a layout hint rather than proof of a disk record:
the Coda case below demonstrates that it can be assembled from older structures.

**Confirmed: word 12 identifies the referenced text family from Finale 2004 onward.** The wire
encoding changes without changing the field's meaning:

| Stored form | Block text | Expression text |
|---|---:|---:|
| Finale 2004–2005 fixed rows | `0x626c` (`bl`) | `0x7870` (`xp`) |
| Finale 2006 fixed rows and zlib class records | `2004` | `2006` |

The Finale 2003 installed component documents retain the earlier four-incidence shape with zero
at word 12. Finale 2004 and 2005 component documents shorten the record to three incidences and
use both `bl` and `xp`; the controlled Finale 2006 graphic fixture uses `2004` on all 17 block
TextBlocks and `2006` on all 41 expression TextBlocks, and the controlled 2008 block-only fixture
uses `2004`. This is a structural discriminator, so the importer accepts either recognized
encoding wherever it occurs rather than selecting one from a version. An absent or zero word
retains musxdom's block default.

**Confirmed: legacy MUS TextBlocks use square corners and a zero corner radius as format
behavior.** Finale 2012 does not offer the rounded-corner option, so the feature postdates the
entire legacy MUS era. Independently, the three-survey companion capture contains only seven TextBlocks with
`roundCorners=true` and `cornerRadius=512`. Six are reserved high-comparator objects with no
source TextBlock, and the seventh is an expression TextBlock added by Finale's upgrade; none
corresponds to a source `TX` or Coda `HS`/`HT` block. Every companion TextBlock corresponding
to a source block has `roundCorners=false` and `cornerRadius=0`. The importer sets both values
and reports them as `LegacyBehavior`; no legacy raw location exists to decode. In particular,
flag `0x2000` and words 13–14 are not assigned to these fields.

**The Coda-banner epoch has no `TX` family at all.** Its synthetic `TextBlock` identity is the
same ordered `HS`/`HT` pair already used to create the `BlockText`. The importer waits for text
recovery and takes that `BlockText`'s allocated number, so the structural relationship has one
implementation and cannot drift. `HS` word 5 low values 0, 1, and 2 select left, right, and
center justification. The reader supplies 100-percent line spacing and `wordWrap`. Because the
Coda era has neither text shapes nor the later single-word expansion option, it reports
`shapeId` zero, `newPos36` false, `showShape` false, and `noExpandSingleWord` false as legacy
behavior because the Coda record supplies no corresponding values. The structural association and
justification are source-derived; the allocated cmper is not. The invariant settings are
reported separately as legacy behavior.

**Open Coda reference mapping:** the inward `TextBlock` → `BlockText` relationship needs no
separate map. Both objects are constructed from the same ordered `HS`/`HT` walk, and the
TextBlock directly takes the finished BlockText's allocated number. The reverse direction is
not solved: staff and group names, page and measure text assignments, text expressions, and
other legacy records can refer to a TextBlock. Before those references are imported, the Coda
layout must establish what legacy token they store, and one document-level mapping must resolve
that token to the synthesized TextBlock cmper. Each referring importer must use that shared map;
independent ordinal counting or semantic-text matching would create divergent identities.
Post-Coda `TX` records do not have this problem: their stored comparator is the TextBlock cmper.

The controlled Finale 1.0 fixture's sole block agrees field-for-field with its companion. The
Finale 2.6.3 fixture has nine source blocks: their semantic text and layout match companion
blocks even though Finale inserts a custom-frame block, duplicates one page-number text, adds
expression blocks, and renumbers the surviving page blocks. Comparator and generated text
numbers are therefore not correspondence keys between the Coda source and its upgraded
companion. Semantic text plus layout can describe the upgrade transformation, but it is not
TextBlock identity and must not be used to resolve references. The maintained report compares
TextBlocks by cmper and compares their `text_id` referent rather than resolving its contents;
the texts pool owns text comparison. Coda TextBlock mismatches are classified as expected
upgrade renumbering. When a paired Coda TextBlock's `text_id` differs, that mismatch establishes
the renumbering for the whole block: the report records it once and discards the block's other
leaf differences.
In the 2026-08-22 three-survey capture, a separate semantic-layout analysis found 594 Coda source
blocks across 83 distinct documents with exact companion counterparts; 520 had different
upgrade-assigned IDs. Seven source blocks in seven documents had no semantic-text counterpart,
while 9,101 companion blocks were additions or otherwise unmatched rather than source recovery
candidates.

The pool walk that reaches all this is now correct, which it was not. It stopped at the first
pool with zero pages, so a Finale 1.0.0 document — whose details pool is empty — reported one
block instead of three and never reached its entries pool or the text region beyond it. **An
empty pool is an ordinary pool**, and the page size is the only thing that identifies a
prologue. The chain needs no terminator of its own: what follows the last pool is the text
region, whose first four bytes are a chunk length rather than `0x200`.

The controlled Finale 1.0.0 text fixtures and the Finale 2.6.3 baseline now cover both Coda
spellings. Their region markers still differ — `^text \0` and `^lyric \0` against `^text()` and
`^lyrics()` — and the reader continues to recognize the structure rather than date the file.
