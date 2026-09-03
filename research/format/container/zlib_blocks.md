# Finale 2007-2012 zlib blocks

**Covers:** Typed zlib block framing, which blocks are compressed, the generic 2007+ record frame, and the 2007-2012 record encoding.
**Read when:** Decoding or framing any Finale 2007-2012 document.
**Confidence:** confirmed for framing and CRC; some record classes remain unmapped.

## 2007+ typed blocks

**Confirmed.** At `0x200`, a principal block has:

| Field | Size | Meaning |
|---|---:|---|
| block type | 2 | numeric block identifier |
| block size | 4 | complete block size, including the 10-byte header |
| CRC-32 | 4 | CRC-32 of decompressed payload |
| payload | variable | zlib member |

Both byte orders occur. Length and CRC validate the choice. The recurring block sequence is:

| Type | Strong interpretation | Evidence |
|---:|---|---|
| `0x001a` | options plus “other” records | generic frame; hundreds of exact XML count correlations |
| `0x001b` | detail records | second generic frame; detail count correlations |
| `0x0016` | entry pool | position and semantic scale; generic frame does not fit |
| `0x0017` | texts/free-form data | decoded strings and Enigma text commands; generic frame does not fit |

Terminal six-byte markers of types `0x0013` and, in later files, `0x001d` occur after the data blocks. No separate central directory was required to walk the four principal blocks; each stored length leads to the next.

### Which blocks are compressed

**Confirmed** by an exhaustive census of the reference corpus. A block header is the same
shape whatever the block holds, so the type — not the size — is what says whether the payload
is a compressed member. Only these types are:

| Era | Compressed types | Everything else |
|---|---|---|
| Finale 2001–2006 (DCL) | `0x000f`–`0x0012` | `0x0013`, always an empty marker in 388 files |
| Finale 2007–2012 (zlib) | `0x0016`, `0x0017`, `0x001a`, `0x001b` | `0x0013` and `0x001d` |

Every listed type decoded in every one of the 388 DCL and 522 zlib files that carries it, and
no unlisted type ever decoded in any file. Note that DCL `0x0012` carries a compressed
payload in 380 files despite also being one of that era's terminal type numbers, so a rule
keyed on terminal types rather than on this list is wrong.

A block outside the list is **stored**: it has no checksum word, so its payload begins
immediately after the six-byte header rather than after ten. Reading it as a compressed
member fails, and failing one member used to abandon every block already decoded — which is
how nineteen otherwise ordinary documents lost their whole options pool, fonts included, to a
single embedded picture. The reader now decides from the allowlist and preserves an unlisted
block verbatim, so an unknown type costs nothing.

## Finale 2007+ generic record frame

**Confirmed for ordinary records in successfully framed `0x001a` and `0x001b` members.** The frame
is variable-length and ordinarily ends with two zero words. More than 1.59 million records across
Finale 2007–2012 were accepted only when the proposed frame consumed the complete decompressed
member and every trailer was zero. The continuation form documented below adds a second terminal
state.

Two serialized variants were observed:

| Variant | Header before data | Stored size behavior | Trailer |
|---|---|---|---|
| Big-endian/earlier | five 16-bit fields; fifth is payload bytes | payload is exactly stored bytes | two zero words |
| Little-endian/transition | four 16-bit fields; fourth is size | serialized data extends two bytes beyond the stored size | two zero words |

The latter may mean that one logical header word moved across the size boundary rather than that the payload truly grew; this is unresolved. The stable logical fields are a numeric type, one or more key fields, a byte count, variable data, and two zero words.

The literal **16-word fixed-record hypothesis is disproved for these blocks**:

- a common big-endian record with a 12-byte payload occupies 13 words total;
- the corresponding little-endian serialized record also occupies 13 words;
- observed payload sizes include 12, 24, 26, 36, 48, 60, 72, 84, 96, 108, 120, 132, 180, 276, 1,536, 8,796, and others;
- records begin at variable offsets rather than a 32-byte grid.

The hypothesized two unaccounted words do exist in this era, but as an ordinary record's
**four-byte all-zero trailer/reserved terminator**, not as the last two words of a fixed 16-word
structure. Their semantic purpose remains open (reserved fields versus terminator/padding), but
their position and zero value are strongly established.

**Strong for `mus-aab617acbbb54646`.** A little-endian Finale 2011 `0x001a` member also permits a
same-sized continuation segment between the declared primary payload and the terminal words. The
continuation begins by repeating the 16-bit payload byte count followed by zero and occupies exactly
the declared payload size including that prefix. Its terminal state is either `0000 0000` or
`ffff 0000`; the latter occurs on classes `0x00cb` and `0x00cc` in this document. Advancing across
145 such continuations consumes the complete 293,030-byte member and exposes 189 `ShapeData`, 115
`ShapeDef`, and 189 `ShapeInstruction` records, exactly matching the independently parsed companion.
The continuation is not another incidence. Its first four bytes repeat the payload length; the
remaining bytes align bit-for-bit with the corresponding prefix of the primary payload. A set bit
selects the stored part bit and a clear bit retains the corresponding score bit. The reader keeps
both the physical payload and continuation, and separately materializes this effective payload
before any class importer parses it. The DOM therefore needs neither XML child-node names nor
class-specific overlay callbacks.

### The 2007-2012 record encoding

**Confirmed** for the font record against Finale 27's own conversion of the same document, and
**strong** for the general shape, which is inferred from that one record type.

The 2007 serialization abandons the fixed 16-byte row. Ordinary records in block `0x001a`
are variable-length and self-describing:

| Field | Width | Meaning |
|---|---|---|
| class id | 2 | numeric identifier standing in for what EnigmaXML names as an element |
| cmper1 | 2 | primary comparator, as in every earlier era |
| part id | 2 | source part owning the record; zero denotes the score |
| length | 4 | payload size |
| payload | length | per class |
| padding | 4 | zero in every observed record |

Detail records in block `0x001b` add a 2-byte `cmper2` between `cmper1` and part id. The
big-endian form then carries a 16-bit payload length; the little-endian transition form carries
the length across the next two words and begins its payload two bytes later. This is
**confirmed** for class `0x041d`: each of two Finale 2008 documents supplies nine score records
with part id 0 and three records with part id 17. Their companions contain the same three
part-owned `MeasureGraphicAssign` nodes as `part="17" shared="true"`, while `cmper1`, `cmper2`,
and the padded payload correspond to the score nodes' staff, measure, and values. Structural
incidences remain the ordered tuples inside the payload, as they do in the fixed-row encoding.

The logical model is therefore unchanged. What moved is the physical encoding: the
two-character tag became a numeric class id, and the fixed six-word payload became a
length-governed byte payload. `RECORD_CATALOG.md` already catalogs these numeric identifiers,
and `0x0090` is the font definition, which that catalog lists as `fontName` at `weak`
confidence.

The font payload keeps the earlier character-set encoding unchanged: `0x1fff` for a Mac symbol
font and `0x1000` for a Mac text font at payload offset 0, the pitch and family pair at offset 2,
and the name from offset 12 to the end of the payload. A longer name simply grows the payload:
`Maestro Percussion` carries length 36 where short names carry 24.

Verified against a Finale 2012 corpus document and its Finale 27 conversion, which agree
on every comparator, every gap in the comparator sequence, every name, and every character set
value.

**Hypothesis, not yet examined: the sharing data should be here too.** EnigmaXML carries part and
sharing information as attributes on each element, and this encoding otherwise lines up field for
field with that model. If the correspondence holds, the part comparator and share mode should
appear in the record header or the payload prologue rather than being derived. This is **open**
and no evidence has been gathered for it yet.
