# Finale 2001-2006 DCL blocks

**Covers:** PKWARE DCL block framing, CRC-32 validation, and the disproved 16-word record hypothesis.
**Read when:** Decoding or framing any Finale 2001-2006 document.
**Confidence:** confirmed; the 16-word reading is disproved -- see [FAILED_HYPOTHESES.md](../../history/FAILED_HYPOTHESES.md).

## Finale 2001–2006 typed DCL blocks

**Confirmed.** The high-entropy payloads use the PKWARE Data Compression Library (DCL) format decoded by Mark
Adler's open-source [`blast`](https://github.com/madler/zlib/tree/master/contrib/blast) implementation. This is the
format produced by the PKWARE DCL `implode()` function, not the incompatible PKZIP compression method that was also
named “implode.” `blast` is shipped as a small, permissively licensed contribution in the zlib source tree, but it is
not part of the ordinary installed zlib API and should be built or vendored separately.

At `0x200`, a compressed block has this layout in the file's detected byte order:

| Field | Size | Meaning |
|---|---:|---|
| block type | 2 | numeric pool/block identifier, commonly `0x000f`–`0x0013` |
| block size | 4 | complete block size, including the six-byte type/length header |
| CRC-32 | 4 | CRC-32 of the decompressed bytes, in the file's byte order |
| payload | variable | complete PKWARE DCL stream |

All 1,603 candidate compressed members encountered in the Finale 2001–2006 corpus decoded successfully and matched
their stored CRC-32. Independently, 410 files traverse cleanly as complete typed-block sequences. The completely
recognized file counts were 3/4 for Finale 2001, 47/48 for 2002, 168/168 for 2003,
7/7 for 2004, 6/13 for the `2004b` banner variant, 116/120 for 2005, and 63/66 for 2006. The remaining files did not
match this outer framing through EOF at `0x200`; some nevertheless contain valid leading DCL members, so they were
not DCL failures and still require outer-framing classification.

Every tested stream begins with either `00 04` or `01 04`, the two legal literal-mode values followed by dictionary
parameter 4 (a 1 KiB dictionary). Of the 1,603 members, 1,202 used `00 04` and 401 used `01 04`. The reference decoder
returned success for every member, and CRC validation independently rules out accidental decoding.

The F2002 baseline `0x000f` member is a compact example: its DCL stream is 1,966 bytes, expands to 8,000 bytes, and
has stored and computed CRC-32 `0xcb68f112`. Its 500 fixed 16-byte rows correspond one-for-one and in order with the
500 lines in the ETF `others` section. The following decoded `0x0010` member contains 33 fixed 16-byte rows matching
the 33 ETF `details` lines, and `0x0011` contains three 38-byte rows matching the three ETF `eE` entries.

Six-byte blocks are empty pool markers. Their type is the next sequential pool type that has no data: for example,
the minimal controlled files end with empty `0x0012`, while 58 Finale 2006 files with nonempty `0x0012` end with an
empty `0x0013`. Therefore the earlier interpretation of `0x0012` itself as a terminal type was wrong. Nonempty
`0x0012` members are variable-length and follow the entry pool; text/lyrics are the leading interpretation, but
their internal organization remains open.

The controlled Windows Finale 2001 files under `tests/evidence/F2001/` confirm the symmetric
little-endian layout. All four block headers, stored CRC values, and decoded fixed-row words use
little-endian serialization. A nonempty `0x0012` text pool may also end exactly at EOF: the measure-text
and section-lyric fixtures carry no following empty `0x0013` marker or eight-byte trailer. The parser
therefore accepts either a sequential empty-pool marker or complete consumption after the last data pool.

**Confirmed in the controlled Finale 2002–2005 baselines.** Each empty final pool marker is followed by the same
eight-byte trailer, `ff ff ff ff 01 04 01 ff`. The trailer is outside the marker's declared six-byte size. Its
meaning is open; readers should preserve/report it as trailing framing data rather than treating it as another typed
block or requiring the empty marker itself to end at EOF.

## Finale 2001–2006 physical records and the 16-word hypothesis

**Confirmed across every directly resolved framed sample.** Decoded `0x000f` and `0x0010` pools consist of fixed
**16-byte**, not 16-word, physical records:

| Pool | Physical row | Total |
|---|---|---:|
| `0x000f` other/options | 16-bit comparator, two-byte tag, 12-byte payload | 16 bytes / 8 words |
| `0x0010` details | two 16-bit comparators, two-byte tag, 10-byte payload | 16 bytes / 8 words |

The apparent “two unexplained words” were an accounting error: the remembered payload capacities were 12 and 10
**bytes**, not words. Thus `2 + 2 + 12 = 16` bytes for an other and `4 + 2 + 10 = 16` bytes for a detail, with no
unaccounted trailer or metadata. Incident number is implicit in the ordered run of rows sharing a key/tag.

Across the 375-file directly resolved source subset, all 375 `0x000f` members contain an integral 4,601,857
rows and all 375 `0x0010` members contain an integral 1,574,280 rows; there are no remainder bytes. All 348 nonempty
`0x0011` members are exact multiples of 38 bytes, totaling 837,086 entry rows. In the eight controlled Finale
2002–2005 MUS/ETF files, the decoded/ETF counts match exactly: 4,552 other rows, 272 detail rows, and 24 entries.

The PDK independently explains the row fields and multi-incidence continuation model; the MUS/ETF pairs establish
the serialized sizes and ordering. This is the clearest solved part of the legacy semantic container so far.
