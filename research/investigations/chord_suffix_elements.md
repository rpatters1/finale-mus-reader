# Chord suffix element investigation

**Covers:** Evidence behind the `ChordSuffixElement` identities, fields, flags, and layout boundary.
**Read when:** Revising the element decoder or testing an alternate interpretation of its fields.
**Confidence:** public-PDK-derived plus targeted independent binary verification.

## 2026-09-04 — Record and field mapping

The public Finale 2000 PDK `EDATA.H` defines the `IV` special-other family, its cmper/incidence
semantics, the six-word field order, and the five interpreted flag masks. The immutable source was
accessed 2026-09-04 at
[`edata.h`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/edata.h).
The maintained API documentation independently describes the same element semantics through
[`FCChordSuffixElement`](https://pdk.finalelua.com/class_f_c_chord_suffix_element.html).

**Confirmed for the narrow fixed-row layout.** `mus-9ce47f4111aac6e7` (Finale 2000) and
`mus-d0856f8164ebd47e` (Finale 2005) contain `IV` rows whose six decoded words match the PDK order.
The two samples cover uncompressed and DCL framing.

**Strong for zlib identity and the narrow/wide layouts.** In `mus-8fc93fa852c6c878` (Finale 2007),
class `0x007d` consists of repeated 12-byte values recognizable as the same symbol, offsets, packed
font, effects, and flags. In `mus-62181aaa4ce12449` (Finale 2012), each recognizable element occupies
24 bytes: the supplied eight-word layout followed by four zero words. Symbols such as `0x0032` occur
as normalized words `0, 0x0032`, establishing high-half-first word order for this class.

**Confirmed for Coda-banner.** `mus-800f4949a5b6a7ee` (Finale 1.0.0) contains two `IV(1)` incidences
and one `IV(2)` incidence. Its ETF repeats the six-word records. Their packed font word `0x0c02`
selects font id 2 at 12 points, while flags `0x0880` combine numeric representation with a flat
prefix.

**Confirmed conversion behavior.** The Finale 27 companion clears `isNumber` and `prefix` on the
controlled fixture's one-element numeric suffix while retaining its stored numeric value and all
other element fields. Six distinct Finale 1.0.0 documents in a private corpus independently show
the same loss for one-element numeric suffixes, with and without a prefix; their numeric elements
inside multi-element suffixes retain `isNumber`. Those companions also append exact terminal
elements with symbol 0, font id 0, and size 24 to selected suffix definitions. The added elements
are independently recognized as filler rather than folded into the conversion-loss classification.

The final 2026-09-04 `tracked-evidence` recovery-coverage snapshot compared 224 sources with 224
companions. It found 41,998 exact element leaves, two expected Coda conversion-loss leaves, one
Finale-added filler transformation, and no unexpected, reader-only, or companion-only leaves. The
subsequent all-corpus snapshot covered 16,321 source occurrences and 4,632 companions; it found
13,135,129 exact element leaves, 56 expected Coda conversion-loss leaves, 97 filler transformations,
and no unexpected or unmatched element leaves. The source representation remains usable directly by
consumers.
