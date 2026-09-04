# Banner-era file header

**Covers:** The 32-byte banner header area, the record-body offset field, and the File Info text region at fixed offsets.
**Read when:** Classifying a file, recovering versions or dates, or mapping document metadata.
**Confidence:** confirmed for the header layout; the field sequence past `0x178` is open.

## Banner-era files

**Confirmed.** All 1,163 files classified as Finale 3.0 through Finale 2012 begin with this 32-byte area:

| Offset | Size | Meaning |
|---:|---:|---|
| `0x000` | 19 bytes plus zero fill | `ENIGMA BINARY FILE` signature |
| `0x020` | up to 64 bytes | Saving-product/copyright banner — see **The three banner spellings** below |
| `0x060` | 2 bytes | Offset of the record body, subject to the file's byte order |
| `0x062` | 4 bytes | Unknown header flags/revision data |
| `0x066` | 3 bytes | Creation date: year byte + 1900, month, day |
| `0x06c` | 20 bytes | Creator Enigma/application/file version tuples plus `FIN` and `MAC`/`WIN` |
| `0x08c` | 3 bytes | Last-save date in the same encoding |
| `0x092` | 20 bytes | Last-saver Enigma/application/file version tuples plus platform |
| `0x0b0`–`0x1ff` | variable/zero fill | title, composer, copyright, file-info, or ancillary header data |
| `0x200` | — | first body/block boundary in all examined banner-era examples |

**Confirmed: the record body offset is a header field, not the constant `0x200`.** It is a
16-bit value at `0x60` read in the file's byte order. Across the 1,163 banner files it reads
`0x200` in 1,162 of them, which is why a constant worked, but the title, composer, copyright, and
file-info strings occupying `0x0b0` onwards can overrun that boundary and push the body later. The
controlled pair `tests/evidence/F97/F97-fileinfo-short.mus` and `F97-fileinfo-long.mus` differ only
in the length of their file-info text: the first begins its body at `0x200` and the second at
`0x1167`, and both record their own offset in this field. Treating `0x200` as constant leaves the
second unframed, and the failure presents as an unknown container variant rather than as a misread
header.

The last-save date matched the filesystem modification day for 1,147 of 1,163 files with a valid date (98.6%). The mismatches are consistent with later filesystem copies. Creation was not assumed from the filename. In 1,039 files where both dates decoded, creation was not later than last save.

The banner is the best direct saving-product classifier. The adjacent version tuples distinguish original creator metadata from the last saver. Their packing is now decoded and recorded in [VERSION_MATRIX.md](../../reference/VERSION_MATRIX.md#decoded-version-packing): a 32-bit value in the file's own byte order, carrying major, minor, maintenance, a development-status code, and build. Finale 27 MUSX conversion preserves the legacy `created` tuple but rewrites `modified` to the conversion event; this was verified on corpus ID `mus-8565d1cad82178ae`.

## The three banner spellings

**Confirmed.** The banner is the best direct saving-product classifier, and legacy Finale writes
it three ways. Recognizing a file means matching all three; matching only the first files the
whole Coda-banner era as unclassified.

| Spelling | Offset | Era | Version terminated by |
|---|---:|---|---|
| `Finale(R) ` | `0x20` | signature-bearing files, Finale 3.0–2012 | ` Copyright`, or ` File Converter` |
| `Finale(TM) ` | `0` | Coda-banner era, Finale 1.8.7–2.6 | ` Copyright` |
| `Finale` + MacRoman `0xAA` | `0` | Finale 1.0.0 only | ` ENIGA Structures` |

`ENIGA` is Coda's own typo and is present in the files. It terminates the version where the other
two spellings use a copyright notice, so a matcher that does not know it reports the product as
`1.0.0 ENIGA Structures`.

**The authoritative implementation is `src/container/product_banner.cpp`**, which holds the one
table of spellings and terminators. This section is the prose that points at it; do not restate
the list elsewhere. A spelling learned in one place and not another has already cost this project
a whole release — see the duplication rule in `AGENTS.md`.

Two consequences are recorded where they apply rather than here: what reading all three does to
the product census, in [`../../reference/VERSION_MATRIX.md`](../../reference/VERSION_MATRIX.md);
and the requirement that the two survey scripts stay in step, in
[`../../reference/REPRODUCING_THE_SURVEY.md`](../../reference/REPRODUCING_THE_SURVEY.md).

## Document metadata in the header text region

**Confirmed.** The header carries the document's File Info text at fixed offsets,
NUL-terminated, in the first `0x200` bytes:

| offset | field | files in `rpatters1-main` | longest observed |
|---|---|---:|---:|
| `0x0D8` | title | 355 | 33 |
| `0x118` | composer | 366 | 20 |
| `0x138` | a further short field, a four-digit year in the fixture | 365 | 38 |
| `0x178` | free description text | 20 | 64+ |

Demonstrable against public fixtures: `tests/evidence/F97/F97-fileinfo-short.mus`
carries a distinct value at each of those four offsets, matching what was typed
into the File Info dialog, and `F97-fileinfo-long.mus` is the same document with
longer text. Open either fixture at the offsets above to check the mapping.

The fields are not a uniform grid: `0x138` sits `0x20` after `0x118`, while the
others are `0x40` apart. The `0x178` description is the one that runs long — in
`F97-fileinfo-long.mus` it continues past `0x200` into the body region, so the
region's end is **`open`** and should not be assumed to stop at `0x200`.

Two consequences for the reader. Legacy documents carry authorship metadata that
a future importer should map rather than discard. And no part of the header is
published in survey evidence bundles: 433 of 1,289 files in `rpatters1-main`
contain free text here, including author names, work titles, and in some files
third-party email addresses. Publishing a metadata-free subset of the header
becomes possible once these fields and their neighbors are named well enough to
excise by field rather than by scanning for readable text. No survey deliverable
publishes header bytes.

The field sequence beyond these four is **`open`**. The counts at `0x0D8`,
`0x118`, and `0x138` track each other closely, which is consistent with one
block of consecutive NUL-terminated strings rather than independent fields at
unrelated offsets; determining that sequence is what would allow metadata to be
excised by field.
