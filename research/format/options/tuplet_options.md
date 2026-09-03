# TupletOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Tuplet options

`TupletOptions` uses numeric global selector `56`. The fixed-row form is one six-word row in the
short layout and three rows in the expanded layout; the zlib form is the same expanded word stream
in numeric global class 56. The expanded stream has this order:

| Word | musxdom field |
|---:|---|
| 0–3 | `displayNumber`, `displayDuration`, `referenceNumber`, `referenceDuration` |
| 4 | secondary flags from Finale 2005 onward; an obsolete value before then |
| 5–8 | `tupOffX`, `tupOffY`, `brackOffX`, `brackOffY` |
| 9 | number, positioning, bracket, and primary behavior flags |
| 10–14 | hook lengths/extensions and `manualSlopeAdj` |

The word count is the layout marker. A 15-word family supplies the expanded fields regardless of
the recovered version; a six-word family supplies only words 0–5. This covers the 3.5 enhancement
without guessing at a version boundary. `tests/evidence/F263/F263-baseline.mus` carries the short
six-word form, `tests/evidence/F372/F372-baseline.mus` carries the expanded pre-2005 form, and
`tests/evidence/F2005/F2005-baseline.mus` carries the expanded 2005 form. **Confirmed.**

Word 4 belongs to the layout rather than to one stable chronology. In the six-word form, bit 0 is
always-flat in both Coda and uncompressed files, and bit 1 is full-duration in the uncompressed
form. In the expanded form, the same word is an obsolete value before Finale 2005 and becomes the
secondary flags described below beginning in 2005. The short-layout bit-0 mapping is
**confirmed** by the controlled Finale 2.6 fixture and the early uncompressed corpus; bit 1 is
**strong**, observed in the uncompressed Finale 3.0 samples but not yet represented by a
controlled fixture.

Word 9 uses four low bits for number style, three bits for positioning, individual behavior bits,
two bits for shape style, and one engraver-tuplet bit. Its high bit selects automatic bracketing.
From Finale 2005 onward, word 4 bit `0x0010` combines with that high bit to distinguish “unbeamed
only” from “never on the beam side”; before 2005 the high bit alone distinguishes automatic from
always. The other meaningful 2005 word-4 bits are always flat, full duration, duration centering,
and staff avoidance. The two note-bearing number encodings `3` and `4` describe the opposite visual
results from their historical XML names, so they map to musxdom's corrected semantic enum values.
These meanings agree with the public `FCTupletPrefs` interface and preference structure
([class documentation](https://pdk.finalelua.com/class_f_c_tuplet_prefs.html),
[public header](https://pdk.finalelua.com/ff__prefs_8h_source.html), accessed 2026-09-02).
The numeric masks and the pre-2005 placeholder distinction are **private-framework-derived**; the
three controlled fixtures independently confirm the physical words and the ordinary values they
exercise.

Four values remain outside selector 56. `tupNUpstemOffset` and `tupNDownstemOffset` use selector 14
words 4–5 in every fixed-row era and in the corresponding zlib class. `tupLineWidth` uses selector
69 word 0 whenever that family is present, beginning with the expanded 3.5 layout. The values in
the controlled 3.7.2, 98, 2000, 2002, 2003, and 2005 files agree exactly with their companions,
including the changed line widths and number offsets. `tupMaxSlope` begins with the 2005 enhancement
at selector 23 word 5. The mappings are **private-framework-derived** and independently reproduced
by the controlled fixtures.

`tests/evidence/F100/F100-baseline.mus` has no selector 56, while the Finale 2.6 fixture has its
six-word prefix. The short-layout controls absent from both files have the same zero/disabled
behavior in their controlled companions, except that the 2.6 word-4 low bit supplies always-flat.
The importer supplies those settings as legacy behavior only where the modern baseline disagrees:
always-bracket, zero vertical offset, no number, manual placement, no break or matching hooks, no
shape, no engraver tuplets, zero hook lengths, and an Efix line width of 224. Before 2005, staff
avoidance is likewise disabled behavior rather than the modern baseline's enabled setting.

The all-corpus comparison found the same six-word behavior in 17 distinct Finale 3.0–3.2
uncompressed documents. Before the short-layout handling was extended to that epoch, those files
accounted for all 211 unexpected `TupletOptions` differences: the eleven unstored behavior values
in every document, bit-0 always-flat in every document, and bit-1 full-duration in seven. Raw rows
from `mus-254b77cdb2ab24f4`, `mus-023827b663a788fa`, and `mus-497e3cfb12919afc` confirm the
six-word structure; the second carries word 4 value `0x0303`, including both established low bits.
**Confirmed for the controlled Coda behavior and strong for the uncompressed behavior.** No Finale
3.3 or 3.4 document exists in the surveyed corpora, so the record length remains the boundary
rather than an extrapolated version gate.
