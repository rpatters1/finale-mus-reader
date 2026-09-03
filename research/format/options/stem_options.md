# StemOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Stem connections

**Confirmed for the collection, its element, and both of its boundaries.** Stem connections are a numeric
global like the clef table: selector `40`, comparator `65534`, and from Finale 2007 the class id the
`numericGlobalClass` rule derives, `40 + 0x0e = 0x0036`. One connection occupies exactly one 16-byte row, so
the element is the six-word payload of one incidence and the collection is as many elements as the family
carries. Counts below are distinct files of the reference corpus unless stated otherwise.

| Era | Identity | Element | Adjustment unit | Files |
|---|---|---|---|---:|
| Finale 1.0.0–3.2 | selector `40`, 32 incidences | 6 words | **Evpu** | 69 (plus 22 Finale 1.0.0 in the installs corpus) |
| Finale 3.5–2006 | selector `40`, 128 incidences | 6 words | Efix | 657 |
| Finale 2007–2011 | class `0x0036`, 1536 bytes | 6 words | Efix | 552 (plus 1,191 Finale 2011 in the installs corpus) |
| Finale 2012 | class `0x0036`, 1800 bytes | **7 words** | Efix | 462 |

The element is the same field order in every era, with the symbol widening at Finale 2012:

| Word | Field |
|---:|---|
| 0 | `fontId`; zero means the document's default music font |
| 1 | `symbol`, one byte through Finale 2011 and a long in words 1–2 from Finale 2012 |
| 2 (3) | `upStemVert` |
| 3 (4) | `downStemVert` |
| 4 (5) | `upStemHorz` |
| 5 (6) | `downStemHorz` |

The symbol's high byte is zero in **every element of every fixed-row file in both corpora**, so nothing was
ever packed beside it; the reader still narrows to the low byte, because a symbol font character may be
stored sign-extended, as clef characters demonstrably are.

A tracked Finale 1.0.0 flag-character fixture stores 0 for the first connection's `upStemHorz`,
while its Finale 27 companion carries -59. The unrelated flag edits do not establish a source
location for that upgraded value, which may be calculated. Coverage therefore classifies a
remaining Coda source-owned disagreement on this path as `different_defaults`; the earlier,
independently characterized 0-to-199/221/589/6969 transformations retain the more specific
`stem-horizontal-correction` classification. **Open:** the calculation or historical default
that produces -59 is unknown.

**The adjustments are Evpu through Finale 3.2 and Efix from Finale 3.5**, a factor of 64. Both halves rest on
exact Finale 27 companions: a Finale 1.0.0 and a Finale 2.6.3 document each store `12` and `-12` for the
default connection and upgrade to `768` and `-768`, while a Finale 3.7.2 and a Finale 2000 document store
`768` and `-768` already and upgrade unchanged. The collection size changes at the same release, 32 slots to
128. No Finale 3.3 or 3.4 document exists in either corpus, so the boundary is known only to lie between 3.2
and 3.5; the reader gates the Evpu range inside the uncompressed epoch and decides the Coda-banner era by its
epoch alone, because that era's Windows documents state a platform where its Mac documents state a version.

**The table is terminated by the first element with no symbol.** No file in any fixed-row era carries a symbol
after a symbol-less element. Elements after the terminator are frequently not empty — a Finale 97 document
carries 29 of them holding a stray `512` — and Finale ignores them, but Finale 27 writes them into EnigmaXML
verbatim. A companion therefore reports more `stemConnect` elements than this reader recovers, which is an
intended difference rather than a decoding error.

### The eight stem scalars, and the marker that dates them

The scalars around the collection are not one record. They are eight fields in five numeric
globals, distilled from the framework's preference location maps and then checked against exact
Finale 27 companions per era:

| musxdom field | Location | Present from |
|---|---|---|
| `halfStemLength` | `03(65534)` word 2 | Finale 3.5 |
| `stemLength` | `20(65534)` word 4 | every era |
| `shortStemLength` | `20(65534)` word 5 | every era |
| `revStemAdj` | `21(65534)` word 2 | every era |
| `stemWidth` | Coda `54` float 2 or `64` word 5; later `64` word 5 | every era |
| `stemOffset` | Coda `55` float 0 or `65` long 0; later `65` long 0 | every era |
| `useStemConnections` | `31(65534)` word 5 | every era |
| `noReverseStems` | `41(65534)` word 1, bit 2 | every era |

`noReverseStems` is bit `0x0004` of the beam flags word, whose other bits carry beaming options.
Its sense already matches musxdom: set means reverse stemming is *not* displayed. The same word
also confirms, from the framework side, the courtesy-flag bit order this project had established
by controlled edit: clef `0x0004`, time `0x0002`, key `0x0001`.

**All eight are confirmed.** Five are settled by the corpus, every file whose companion carries a
non-default value agreeing: 243 for the stem offset, 233 for the thickness, 208 for the connection
switch, 199 for the reverse adjustment and 68 for the shortened length. The other three are settled
by controlled saves the corpus could never have supplied, because no surveyed document varies them:
a Finale 3.7.2 pair moves the half-stem length 18 -> 19, a Finale 2002 pair moves the normal stem
length 84 -> 96, and Finale 1.0.0, 3.7.2 and 2002 pairs each set the reverse-stemming flag, the
first two in bit 0 and the third in bit 2.

No inference remains anywhere in this class. Every location, every unit conversion and both
spellings of the reverse-stemming flag rest on either corpus agreement across files that vary the
value or a controlled save that varies it deliberately.

**Finale 3.5 changed every unit in this family at once.** Before it, the three lengths are stated
in staff positions rather than Evpu: all 59 early files with an exact companion store 7, 5 and 18
where the companion carries 84, 60 and 216, the same factor of twelve for three different
numbers. The connection adjustments make the matching change from Evpu to Efix, and the
collection grows from 32 slots to 128.

The printed Finale 3.5 addendum independently identifies half-stem length as a new setting in
that release. This confirms the availability boundary while the collection size remains the
decoder's more reliable layout marker.

Controlled Finale 1.0.0 saves settle the unit rather than merely being consistent with it.
Lengthening the normal and shortened stems by one staff position each moves selector `20` word 4
from 7 to 8 and word 5 from 5 to 6, and the exact companion moves from 84 and 60 to **96 and 72**.
A second save sets the reverse adjustment to 25, moving selector `21` word 2 from 18, and its
companion carries **300**. Three different stored numbers, one factor of twelve, each measured.

The reverse adjustment's magnitude is worth stating, because it invites a wrong conclusion: the
modern default of 216 Evpu is nine spaces, which looks far too large for an adjustment until one
knows what it adjusts. It slides the stem **entirely** to the other side of the notehead, so the
value is a whole displacement rather than a nudge. That also disposes of the suspicion that Finale
converts this field wrongly when it upgrades a Coda document: the twelvefold step is the same unit
change the other two lengths make, and all three Coda defaults are exactly a twelfth of the modern
ones, which is what preserving a physical size across a unit change looks like.

### The reverse-stemming flag moved, and the word says which spelling it is in

This is the only stem row any evidence shows changing location. Two controlled saves put it in
**bit 0** of selector `41` word 1: switching off "Display Reverse Stemming" moves that word from 0
to 1 in Finale 1.0.0 and again in Finale 3.7.2, and both companions gain `<noReverseStems/>`. The
framework places it at bit `0x0004` for the era it describes, and the corpus shows the early
spelling cannot still hold there: **25 companion-backed Finale 2003 and 2007 documents carry bit 0
set while their companions leave reverse stemming on**, so by then bit 0 is another beam option.
A controlled Finale 2002 save settles the packed spelling directly — switching the option off moves
the word from 26 to 30, a gain of exactly 4 — and the companion gains `<noReverseStems/>`. Bit 2 is
set in no other corpus file, which is what a rarely-changed option looks like.

The word itself dates the layout. Selector `41` word 1 is **exactly zero in every corpus file of
Finale 97 and earlier** and carries packed values — 408, 178, 26 and the like — from Finale 2000
on, so the word acquires its other tenants at Finale 2000. The reader therefore reads bit 0 when no
bit above it is set and bit 2 otherwise, which needs no version and dates the three major-15 Finale
3.0 files correctly. Checked against every companion-backed file sampled across all eras, the rule
and the companion agree without exception, and it also disposes of the Finale 3.0–3.4 question: a
file in that window is dated by its own word rather than by a boundary nobody can observe.

**The connection switch is `31(65534)` word 5, confirmed by controlled edit.** Enabling stem
connections in Finale 1.0.0 moves that word from 0 to 1 and moves **nothing else in the file** —
one word in one record — and the companion gains `<useStemConnections/>` where the baseline has
none. The era's own ETF export carries `^31(65534) 2 -6 63 -2 -6 1` against the baseline's
trailing 0, so source and companion agree from independent directions.

A third save that chose "Disable" changed no record at all, which is what a no-op looks like: that
document was already disabled, and the Finale 1.0.0 dialog gives no indication of the current
state. It is kept as the regression test for finding no difference where there is none.

**The reader dates a file by that slot count, not by its version.** A version range would work on
every file surveyed, so this is a preference, and the same one that decides the clef tuple width
from its payload size. It rests on three things: **the boundary version is unobserved**, since no
Finale 3.3 or 3.4 document exists in either corpus and a range must therefore guess a cut point
between 3.2 and 3.5; **one fact decides three questions**, so the collection size and the two unit
changes cannot drift apart; and **the Coda era states no version at all in its Windows documents**,
so a version range would need splitting across two tables by epoch to reach them. The slot count
splits all 741 fixed-row files into 69 early and 672 later, and every file whose companion could be
compared agrees with the unit that marker predicts.

One location is era-scoped rather than universal, in the same way the clef scalars were:

- `halfStemLength` is not stored before Finale 3.5. Selector `03` carries no row at all in the
  Finale 3.0 and 3.2 files, and in the Coda era its word 2 is zero while the companion shows 18.

The two Coda stem sizes have an original floating-point layout and a migrated fixed-point layout.
Finale 1.0.0 stores Stem Line Width as selector `54` float 2 and Stem Lift as selector `55`
float 0, both in points. Finale 2.6.3 retains those floats and also writes the corresponding
semantic values at selector `64` word 5 and selector `65` long 0 in ten-thousandths of a point.
Selector `64`'s presence identifies the migrated layout and makes the later pair authoritative;
otherwise the original floats are used. Both representations convert to modern Efix.

A controlled three-stage upgrade confirms the migration. The Finale 1.0.0 source stores 2.71828
and 1.618 in the two floats. Finale 2.6.3 writes 27182 and 16180 in the later locations, Finale
3.7.2 converts them to 696 and 414 Efix, and both later modern companions preserve those exact
values as `stemWidth` and XML `stemLift`. The direct Finale 1.0.0 companion instead normalizes
them to 224 and 256. Those source-owned direct-upgrade disagreements are conversion loss rather
than defaults. They are classified only when the Coda source uses the original layout, as shown by
the absence of selector `64`. **Confirmed.**

The tracked capture reports 76 expected Finale-upgrade-loss StemOptions leaves: one `stemWidth`
and one `stemOffset` for each of the 38 original-layout Coda sources. There are no unexpected
StemOptions differences. The Finale 2.6.3 migration and Finale 3.7.2 resave agree with their
companions.

The obsolete Coda Stem Offset control itself is selector `21` word 0. A controlled Finale 1.0.0
edit from 4 to 23 changes that word to 23, and opening and saving the document in Finale 2.6.3
retains 23 in the same location. The resave nevertheless leaves selector `65` at 5000, and neither
upgrade carries the edit into modern `stemLift`: the direct Finale 1.0.0 companion contains 256,
while the Finale 2.6.3 companion contains 128. The importer therefore does not transfer selector
`21` word 0 into musxdom `stemOffset`; it remains evidence of source behavior that Finale's own
modern conversion discarded. **Confirmed.**

The zlib era reaches the same eight through `numericGlobalClass` and byte offsets. The stem offset
is the field that proved four-byte class-record values are **not** a plain four-byte read: they
are the same two payload words the fixed rows carried, high word first, which differs from a
straight little-endian long on exactly the files where it matters.

### The Finale 2012 payload and its stale predecessor

Every Finale 2012 file carries 1800 bytes where Finale 2007–2011 carry 1536: 128 elements of 14 bytes plus 8
unused, against 128 of 12. The widening is Finale 2012 and not Finale 2011, and the installs corpus settles
it from the other side — all 1,191 Finale 2011 documents carry the 1536-byte payload.

Only the front of that record is necessarily live. In 194 of the 248 reference-corpus documents the bytes
after the connections Finale wrote are **byte-identical to the pre-Unicode default table**, `0000 8300 00f7
0009 00fc 0004 | 0000 8400 …`, sometimes partly overwritten. Read through the widened element that stale copy
decodes to out-of-range symbols such as `0x0900F700` and `33554432`, which is what musxdom's `StemOptions`
documentation reports seeing in Finale-produced files. Two populations follow:

- 140 documents whose connection 0 was edited: one live wide element, then the stale table.
- 53 documents that never wrote one at all: the widened element reads symbol `0x030000C0`, which is the old
  table's first two words, and the record states **no** connections.

Finale 27 reads all 1800-byte payloads as widened elements, so its conversions of the second group are
garbage from index 0. The reader recovers nothing from such a record rather than re-reading it as the older
layout: that is what Finale itself sees, and asserting the pre-Unicode layout for a 2012 document would be a
layout this era does not use. The document is otherwise an ordinary 2012 file — its clef table is the widened
10-word tuple in all three populations — so the stale bytes are a leftover in one record, not a second format.
