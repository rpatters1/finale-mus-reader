# TextOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Text options

The framework preference tables name three of this class's fields and nothing else, so the rest was located by
searching for the Finale 27 defaults as a byte pattern and then diffing controlled one-variable saves. Counts
below are the reference corpus's 1,189 adjacent-exact companion pairs unless stated otherwise. The mapping is in
[`data/text_options_mapping.csv`](../../data/text_options_mapping.csv).

**Implemented in full.** `src/import/options/text_options.cpp` recovers all fourteen scalar fields and all five
accidental inserts across all four epochs. Verified against every tracked fixture with a companion: 2,064 insert
fields agree, 266 are the intended pre-2001 disagreement described below, and 50 are the Coda-era case where the
reader takes the pinned baseline and Finale 27 synthesizes the older defaults instead.

The scalars live in five numeric globals at comparator `65534`, reached in the zlib epoch through the usual
`numericGlobalClass` rule. Every one agrees with its companion on every document that carries the record:

| Field | Location | Documents | Notes |
|---|---|---:|---|
| `showTimeSeconds` | `05` word 4 | 1,189 | |
| `dateFormat` | `05` word 5 | 1,189 | musxdom's `DateFormat` values directly |
| `tabSpaces` | `13` word 0 | 1,189 | |
| `textTracking` | `81` words 0–1 | 1,108 | 32-bit |
| `textBaselineShift` | `81` words 2–3 | 1,108 | 32-bit Evpu |
| `textSuperscript` | `81` words 4–5 | 1,108 | 32-bit Evpu |
| `textLineSpacingPercent` / `textLineSpacingEvpu` | `82` word 0, mode in word 1 | 1,108 | word 1 selects the member; see below |
| `textWordWrap` | `82` word 2 | 1,108 | |
| `textPageOffset` | `82` word 3 | 1,108 | |
| `textJustify` | `82` word 4 | 1,108 | **not** musxdom's values; see below |
| `textExpandSingleWord` | `82` word 5 | 1,108 | |
| `textHorzAlign` | `83` word 0 | 1,108 | `AlignJustify` values directly |
| `textVertAlign` | `83` word 1 | 1,108 | **not** musxdom's values; see below |
| `textIsEdgeAligned` | `83` word 3 | 1,108 | by elimination within `83` |

**Selectors `81`, `82` and `83` arrive with Finale 97**, matching the `83` boundary the multimeasure-rest
defaults already establish: no document of Finale 2.6, 3.0, 3.2, 3.5 or 3.7 carries any of the three, and every
Finale 97 and later one carries all three. `05` and `13` are present in every era including Coda-banner, so
`dateFormat` and `tabSpaces` need no gate at all — a Finale 1.0.0 and a Finale 2.6 fixture each move both fields
from the same words as every later era. That agrees with the report that the Coda-era UI exposes tab spacing and
date format and no other document-wide text setting.

**Three enums order their lists first, opposite, center, and two of them therefore disagree with musxdom.**
`AlignJustify` already uses Finale's order, `Left, Right, Center`, so `textHorzAlign` passes through untouched.
`TextJustify` is `Left, Center, Right, Full, ForcedFull` in musxdom against `Left, Right, Center, Full,
ForcedFull` in the file, and `VerticalAlignment` is `Top, Center, Bottom` against `Top, Bottom, Center`. Both need
positions 1 and 2 exchanged; nothing else moves.

Each was settled by a single specimen, because the corpus never varies any of them:

- `textJustify` — one Finale 2003 document stores 2 and Finale 27 converts it to `center`.
- `textHorzAlign` — one Finale 2001 document stores 2 in `83` word 0 against a converted `center`. That document
  is also what identifies word 0, since the controlled fixtures set right and true together and both read as 1.
- `textVertAlign` — `tests/evidence/F2005/F2005-textvert-center.*` moves `83` word 1 alone, 0 → 2, and its
  companion gains `<textVertAlign>center</textVertAlign>`. Center at 2 with the earlier fixtures' `bottom` at 1
  fixes the whole list, and leaves word 3 as `textIsEdgeAligned` by elimination.
- `textExpandSingleWord` — `tests/evidence/F97/F97-expword-off.*` moves `82` word 5 alone, 1 → 0, and its
  companion loses `<textExpandSingleWord/>`.

- the line-spacing mode — `tests/evidence/F2005/F2005-linespace-to-evpu.*` moves `82` words 0 and 1 and nothing
  else, `[100, 1, 1, 0, 0, 1]` → `[72, 0, 1, 0, 0, 1]`, with the ETF reading `^82(65534) 72 0 1 0 0 1`. Its
  companion replaces `<textLineSpacingPercent>100</…>` with `<textLineSpacingEvpu>72</…>` and keeps
  `<textExpandSingleWord/>`. **Word 1 set means percent and clear means Evpu**, with word 0 the value either way.

That last fixture matters more than its size suggests, because **Finale 27 has no boolean for the mode**. It
writes either `<textLineSpacingPercent>` or `<textLineSpacingEvpu>` and never both, so the mode is the element's
identity rather than a value a companion could be compared against: across 1,730 corpus companions and all 68
tracked-fixture companions, only two documents carry the Evpu spelling and none carries a flag beside either.
Word 1 is therefore load-bearing but never itself recovered — it decides which of musxdom's two members `82`
word 0 belongs in, and nothing holds it afterwards. Before this fixture the word was identified only by
elimination against word 5, since the Finale 2012 scalars save had moved both at once; this one moves word 1
with word 5 held still, which settles it directly.

One consequence for the DOM: a document storing 0 in word 0 loses the distinction, because both members are then
zero. That is harmless for the value and only means the mode cannot be round-tripped in that one case. musxdom
now models the pair as `std::optional`, with `TextOptions::integrityCheck` reporting and repairing a document
that supplies both spellings or neither.

`83` word 2 remains unassigned. It is 0 in every pre-2007 document and 1 in every 2007-and-later one, and a
controlled Finale 2012 text-options save cleared it, but it is not any of `TextOptions`'s fields. This is the
same word the multimeasure-rest note records as set in 468 companion-backed documents without
`autoUpdateMmRests`; those 468 are exactly the zlib-era documents whose word 4 is clear, so the two observations
are one fact.

Finale 27 writes `<textLineSpacingEvpu>` in place of `<textLineSpacingPercent>` when line spacing is absolute.
musxdom had no such member and silently dropped the value; it now has one.

### Accidental symbol inserts

**Confirmed for Finale 2001 onward; strong physical mapping for Finale 3.7–2000.**
`TextOptions::symbolInserts` is a direct five-element array at selector `78(65534)`, class
`0x005c` in the zlib epoch, in musxdom's own `AccidentalInsertSymbolType` order: sharp, flat,
natural, double sharp, double flat. The repository owner's inspection of the Finale 3.7 addendum
confirms that configurable text inserts first appeared in that release's UI. This independently
supports the structural boundary: the family is absent through Finale 3.5 and present from Finale
3.7 onward. The UI boundary is confirmed; the early field widths and byte order remain strong for
the separate reasons below. The field order is the same in every era:

| Offset | Width | Field |
|---:|---|---|
| 0 | 4 | `trackingBefore` |
| 4 | 4 | `trackingAfter` |
| 8 | 2, signed | `baselineShiftPerc` |
| 10 | 2 | `symFont` font-definition comparator |
| 12 | 2 | `symFont` size |
| 14 | 2 | `symFont` effects bitmask |
| 16 | varies | `symChar` |

A 32-bit field is two 16-bit words, **high word first**, each word in the container's byte order — the framework's
`MACFOURBYTE`. That is one rule for both byte orders: a Finale 2005 big-endian file stores 1000 as
`00 00 03 e8` and a Finale 2012 little-endian file stores it as `00 00 e8 03`.

What changes between eras is the element size, and the family's own size states which layout applies:

| Era | Payload | Element | `symChar` | Documents |
|---|---:|---|---|---:|
| Coda-banner 1.x–2.6, Finale 3.0–3.5 | absent | — | — | 61 |
| Finale 3.7–2000 | 96 bytes | **17 bytes** | 1 byte at offset 16 | 179 |
| Finale 2001–2010 | 96 bytes | **18 bytes** | 2 bytes, low byte only | 701 |
| Finale 2012 | 108 bytes | **20 bytes** | 4 bytes, low word first | 248 |

The epoch separates the 17-byte layout from the 18-byte one, and inside the zlib epoch the payload length
separates 18 from 20, so no version gate is needed. The Finale 2012 widening of `symChar` is the same Unicode
boundary the stem-connection symbol crosses.

Agreement with exact Finale 27 companions is complete from Finale 2001 on: 21,030 field comparisons across the
701 documents of the 18-byte layout and 7,440 across the 248 of the 20-byte layout, with no disagreement.
This is not a defaults-only result. 460 elements name a real font definition, sizes range over 100, 110, 112,
120, 130 and 150, baseline shifts over −70, 10, 15, 16, 19, 30, 34 and 110, and 198 documents set effects bits;
all of those agree. Effects are `FontInfo::setEnigmaStyles` unchanged — 990 comparisons on the documents that
set any bit, including a value of 56 whose 0x08 and 0x10 bits neither Finale 27 nor musxdom models, and which
both therefore drop identically. The controlled fixtures pin the individual fields that the corpus leaves at
their defaults: tracking before 1000, tracking after 250 and a baseline shift of −25 in one save, and a
Petrucci reference at 79% with bold, then italic plus underline, in two more.

**The `symChar` slot is a byte, not a word, before Finale 2012.** Four Finale 2006 fixtures store the two
characters above 127 sign-extended, `ff dc` for 220 and `ff ba` for 186, while a fifth fixture of the same
version stores `00 dc`. Finale 27 keeps the low byte and so must the reader.

**The Finale 2012 form is a plain little-endian long, and the two trackings in the same element are not.** That
asymmetry is measured rather than assumed, and only one kind of specimen can measure it: a character above the
basic multilingual plane, where the candidate orders finally disagree.
`tests/evidence/F2012/F2012-dblsharp-insert-outside-BMP.*` supplies one. Its double-sharp insert stores
`69 64 02 00` and its companion reads `<symChar>156777</symChar>`, U+26469 in CJK Extension B, with the font set
to LiSong Pro at comparator 19. Low word first gives exactly that; the high-word-first order `trackingBefore`
and `trackingAfter` use would give 0x64690002, which is not a codepoint at all. So one element holds two
different 32-bit conventions, which is consistent with the trackings being old fields carried forward in Finale's
two-word form while the character was widened later as a native long.

**Finale 27 mis-converts the Finale 3.7–2000 layout, and this reader deliberately disagrees with it.** Finale 27
uses the correct 17-byte stride but reads the multi-byte fields as though the element were the 18-byte one, so
it reports the sharp insert's tracking as 2293760 — the bytes `00 23 00 00` read as a big-endian long — and its
character as 50, which is the first byte of the *next* element. Read as a little-endian byte structure the same
records yield 35, 50, 0, 40, 60 and characters 35, 98, 110, 220, 186: the values every other era stores, on all
179 documents and all thirteen tracked fixtures of that era. No companion can confirm the era because every
companion is wrong, so the layout stays **strong** rather than confirmed until a controlled Finale 97 or 2000
save exists. The 32-bit width of the two tracking fields there is inferred from the offsets, which are identical
to the later layout; only the character narrows.

Why that era's structure is little-endian inside a big-endian container is **open**. Every observed file of the
era is big-endian, so "opposite to the container" and "always little-endian" cannot be told apart; a Windows
Finale 3.x–2000 document would separate them and none is available. The reader undoes the container word order
on a big-endian file, which gives the same answer under either explanation.

**Finale's own upgrade path bakes that corruption into later files.** Eight documents — six Finale 2012 and two
Finale 2009 — store a record that already contains the misconverted values, evidently from an old file opened
and re-saved in a later Finale. The reader reproduces Finale 27 exactly on all eight, which is independent
confirmation of both the later layouts and the misconversion.

The Coda-banner absence is intended and is not a gap: no document of that era carries selector `78`, the era's
UI is reported to expose no such option, and Finale 27 synthesizes the pre-2001 defaults when converting one.
Those documents keep the pinned Finale 27 baseline's five inserts, which the options pool has already seeded, and
each `symFont` comparator is translated into the imported document's own numbering through musxdom's
`importFontDefinitionInto`. **This is a second deliberate disagreement with the companion**, and a smaller one:
Finale 27 gives a converted Coda document the pre-2001 defaults, so its flat and natural inserts read 50 and 0
where the baseline reads 60 and 50. That is 50 field differences across the 25 Coda fixtures, all in those two
fields. Taking the baseline is a decision rather than a finding -- no evidence says what the Coda era actually
rendered, and Finale 27's choice may be its converter's own table rather than a fact about the era.
Finale 3.0–3.5 is the same case for a different reason — the record simply is not there yet — and rests on only
eight documents of the reference corpus, which is thin; the installs survey has not been run for this class and
would firm up where between 3.5 and 3.7 the record appears.

The third deliberate disagreement is in a different class and is recorded under
[Font definitions](../others/font_definitions.md#a-shape-naming-a-font-the-source-never-defines--third-deliberate-disagreement): a shape whose
`SetFont` names a comparator the source never defines, which Finale 27 resolves to the wrong face by accident of
its own renumbering.
