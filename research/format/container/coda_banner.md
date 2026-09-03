# Coda-banner container (Finale 1.x-2.6)

**Covers:** Chained pools, pool 0 rows, the Enigma string region, and header text records in the pre-`ENIGMA BINARY FILE` era.
**Read when:** Working on any Finale 1.x-2.6 file, or on directory/pool spans in that era.
**Confidence:** strong for the pool cadence; index and directory spans remain open.

## Coda-banner files

Previously described here as "pre-banner", which is inaccurate: these files do have a banner.

**Confirmed.** These files lack the `ENIGMA BINARY FILE` signature but open at offset 0 with a plain-text product banner reading `Finale(TM) <version> Copyright 1987 by Coda. All rights reserved.`. The banner is the only place their version appears: bytes 0x60-0x200 are entirely zero apart from a constant `01 03` at 0x80, which is a candidate format version rather than an application version.

Finale 1.0.0 belongs to this family but spells the banner a third way, as in
`Finale™ 1.0.0 ENIGA Structures Copyright 1987 by Coda.`; the full set of spellings and their
terminators is in [`header.md`](header.md#the-three-banner-spellings). It shows the same absence of an Enigma tuple — 22 of 22 files in `rpatters1-installs` yield no `FIN` application string at the offset where later eras carry one — which is consistent with the zeroed 0x60-0x200 region described above rather than with a differently placed tuple.

**Contradicted: the era is not Finale 2.6 alone.** This section previously read "every one of the 54 is Finale 2.6", measured when the survey saw only loose files. Including archive members raised the sample to 229 files, and 52 of them state a version older than 2.6 in their own banner:

| banner version | files |
|---|---:|
| 2.6 | 177 |
| 2.0.1 | 33 |
| 1.8.7 | 19 |

The earlier claim was not wrong about what it measured; the loose corpus genuinely held only 2.6. Finale 1.8.7 and 2.0.1 survive in this collection only inside archives, which is worth remembering whenever an era looks uniform.

Two claims made against the original 54 were re-tested across all 229 and **both hold**: the region 0x60-0x200 is zero apart from `01 03` at 0x80 in every file of all three versions, and every file satisfies the chained-pool test below. Confidence stays **strong** rather than rising: 229 files are one collection, and independent corroboration would need a second survey.

Other claims about this era elsewhere in these notes were measured over the 54 loose files known before archives
were surveyed, and are marked "then known" where they state a count. They have not been re-tested against the
full 229 and should not be read as covering it. Re-measuring them is **open** work.

Because the version is explicit and machine-readable, `scripts/inventory.py` reports these as `1.8.7`, `2.0.1`, and `2.6` rather than `unknown`.

The `(TM)` spelling is what separates the era. All signature-bearing files spell it `Finale(R)`, and later banners name Coda as well, so the copyright holder does not distinguish the two.

These files reserve the same 0x200 header as later eras, and the region at 0x200 begins with a page count followed by the page size, as in `000000ab000002000000444100300000`. That second word, not anything at the top of the file, is the structural confirmation: all 229 satisfy it, while the AppleDouble metadata artifact in the corpus satisfies neither test. Corpus ID `mus-6d3b75475a2ca67b` is also Coda-banner but has weaker provenance.

### Chained pools

**Confirmed** across all 229 Coda-banner files, spanning versions 1.8.7, 2.0.1, and 2.6. There is no per-block framing in this era. Instead the file is a chain of pools starting at 0x200, each laid out as:

| Field | Width | Meaning |
|---|---|---|
| page count | 4 bytes | number of 512-byte pages of record data |
| page size | 4 bytes | always `0x00000200` |
| records | page count × 512 | record rows, zero-padded to the page boundary |

Each pool begins immediately after the previous one ends. Every file contains **exactly three pools**, and the walk terminates in the same place every time. The page count is a count of pages, not of records: a file whose first pool declares 46 pages carries far more than 46 rows.

Pool 0 is the tagged others pool and pool 1 is the details pool; both are described under
[record pools and row shapes](record_rows.md#record-pools-and-row-shapes-through-finale-2006), which covers every
epoch through Finale 2006. Pool 1 was initially reported here as having no tags, because it was
measured at the others tag offset; a detail's second comparator displaces its tag by two bytes.
Pool 2 shows no tag at any offset or stride and is presumed to hold entries, which remains
inference rather than measurement.

### Pool 0 rows

Rows are 16 bytes in the shared others shape, and incidence is implicit in encounter order with no
incidence field, exactly as in the later fixed-row eras.

Trailing space in the final page is zero-filled, so an all-zero row marks the end of populated records. This holds for 51 of the 54 then known files; the other three fill their pages exactly. A row of `ffffffff` can appear *within* the populated region and is not a terminator: the Finale 1.8.7 file `mus-7aa45639c14b3864` carries one at a point where valid rows continue afterward. Populated row counts across the 54 files run from 602 to 11,299, median 5,797.

Comparator 65534, which later eras use for synthetic preference records, is already in use here. `mus-7aa45639c14b3864` carries 150 such rows under numeric tags `01`-`43`, `50`-`55`, `62`-`65`, plus `40` and `fi`. The `^NN(65534)` preference mechanism therefore predates the Enigma signature. Selector `94`, which the distilled mapping uses for music spacing, is **not** present in that file.

Two tags that the distilled PDK mapping relies on, `FI` and `HE`, do not occur anywhere in pool 0 of
any of the 54 files then known. Both searches predate the correction below about tag byte order, but this era
is uniformly big-endian, so they are unaffected.

### Enigma string region

**Confirmed** across all 54 files then known. After the third pool the remainder of the file holds exactly two length-prefixed chunks, each a 4-byte big-endian byte count followed by that many bytes of text. Walking them lands precisely on end-of-file in every case. The first chunk always begins with `^` and carries `^text()` followed by `^block(n)` sections; the second is `^lyrics()`.

Enigma string markup therefore already exists in Finale 1.8.7: `mus-7aa45639c14b3864` contains `^font`, `^size`, and `^efx` commands inside its text chunk. This region is better delimited than the banner era's mixed `0x0017` block, because the pool chain locates it exactly with no scanning.

### Header text records

The `HT` tag in pool 0 carries page text rather than document metadata. Each logical text block occupies **four consecutive incidences**, that is 48 payload bytes, holding a NUL-terminated string followed by numeric fields. Every `HT` family observed across 20 files has an incidence count that is a multiple of four, in 32 of 32 families. In `mus-7aa45639c14b3864` the comparator-1 family holds five blocks in twenty incidences: a composer credit, a dedication, a copyright line, a typesetting credit, and the title.

These are page titles, not document metadata. Whether this era stores document metadata at all is **open**.

Text in these records is **Mac OS Roman**, not Latin-1: `0xa9` renders as `©` in the copyright line and `0xaa` as `®` in a `Finale®` credit. Any import of this text needs the encoding conversion step before the strings are usable.
