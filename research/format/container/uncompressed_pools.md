# Finale 3.x-2000 uncompressed typed pools

**Covers:** The four uncompressed typed pools and their platform-dependent byte order.
**Read when:** Decoding or framing any Finale 3.0-2000 document.
**Confidence:** confirmed.

## Finale 3.x–2000 uncompressed typed pools

**Confirmed.** Starting at offset `0x200`, recognized files contain four consecutive pools. Each has a two-byte type
and four-byte total length in the file's byte order; the length includes the six-byte header. There is no CRC and no
compression:

| Type | Contents | Physical representation |
|---:|---|---|
| `0x0001` | others/options | 16-bit comparator, two-byte tag, 12-byte payload; 16 bytes total |
| `0x0002` | details | two 16-bit comparators, two-byte tag, 10-byte payload; 16 bytes total |
| `0x0003` | entries | 32-bit entry ID plus entry data; 38 bytes total |
| `0x0004` | text | raw concatenated Enigma text commands; variable length |

The deterministic probe recognizes 189/190 direct Finale 3.x–2000 files through EOF with the exact type sequence
`1,2,3,4`: 185 big-endian files and four Windows-origin little-endian files. Across those files, all 1,552,762
other rows and 770,960 detail rows are exact multiples of 16 bytes, and all 394,984 entries are exact multiples of
38 bytes. The only unrecognized file is one Finale 97 sample; it requires separate integrity/classification work.

The exact Finale 2000 pair `mus-3a8b724cf3adba80` is decisive. Its blocks are:

| File offset | Type | Total bytes | Payload interpretation |
|---:|---:|---:|---|
| `0x0200` | `0x0001` | `0x4536` | 17,712 bytes = 1,107 other rows |
| `0x4736` | `0x0002` | `0x0406` | 1,024 bytes = 64 detail rows |
| `0x4b3c` | `0x0003` | `0x0136` | 304 bytes = eight entry rows |
| `0x4c72` | `0x0004` | `0x0243` | 573 bytes of raw text through EOF |

Those counts equal the ETF sections exactly. All 1,107 ordinary tags occur in identical order. The first 34 detail
tags, including `CN`, `GF`, and `TP`, also occur literally and in ETF order; the remaining `#v1`–`#v10`,
`#c1`–`#c10`, and `#s1`–`#s10` pseudo-tags use compact non-ASCII binary identifiers. The eight 38-byte rows equal
the eight `eE` records, and selected fields match directly. Removing only ETF's separators between six text records
makes its text section byte-identical to the 573-byte `0x0004` payload.

This proves that Finale 2001 changed the wrapper/codec, not the core fixed physical rows. Finale 3.0's three files
are Windows-origin and little-endian; the single little-endian Finale 2000 file is also Windows-origin. Thus byte
order must be detected from the block headers rather than inferred only from the release name.
