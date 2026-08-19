# Format Notes

Confidence labels: **confirmed** means reproduced across the stated sample; **strong** means multiple independent observations agree; **weak** is a working hypothesis; **open** is not interpreted.

## File header and version identification

### Banner-era files

**Confirmed.** All 1,163 files classified as Finale 3.0 through Finale 2012 begin with this 32-byte area:

| Offset | Size | Meaning |
|---:|---:|---|
| `0x000` | 19 bytes plus zero fill | `ENIGMA BINARY FILE` signature |
| `0x020` | up to 64 bytes | Saving-product/copyright banner, e.g. `Finale(R) 2005 ...` |
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

The banner is the best direct saving-product classifier. The adjacent version tuples distinguish original creator metadata from the last saver. Their packing is now decoded and recorded in [VERSION_MATRIX.md](VERSION_MATRIX.md#decoded-version-packing): a 32-bit value in the file's own byte order, carrying major, minor, maintenance, a development-status code, and build. Finale 27 MUSX conversion preserves the legacy `created` tuple but rewrites `modified` to the conversion event; this was verified on corpus ID `mus-8565d1cad82178ae`.

### Document metadata in the header text region

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
becomes possible once these fields and their neighbours are named well enough to
excise by field rather than by scanning for readable text. No survey deliverable
publishes header bytes.

The field sequence beyond these four is **`open`**. The counts at `0x0D8`,
`0x118`, and `0x138` track each other closely, which is consistent with one
block of consecutive NUL-terminated strings rather than independent fields at
unrelated offsets; determining that sequence is what would allow metadata to be
excised by field.

### Determining byte order

The reader does not read the byte order from anywhere. It trials both orders against the container
framing and keeps whichever validates: for the uncompressed era, a block chain that consumes the
file exactly and yields types 1, 2, 3, 4 in order; for the compressed eras, a first block type that
matches the era plus successful decompression and a matching CRC-32 for every block. The compressed
test is strong evidence. The uncompressed test is structural but circumstantial, and the
Coda-banner era is not detected at all, only asserted, because it has no block framing to trial.

**Found for the Coda-banner era: the product field names the platform.** That era's Windows
documents carry the banner product `PC 1.0+`, where its Mac documents carry a bare version —
`1.0.0`, `1.8.7`, `2.0.1`, `2.6`. The `PC` prefix is a statement in the file rather than an
inference from it, and it discriminates perfectly: 24 Windows documents carry it and are
little-endian, 252 Mac documents of the same era do not and are big-endian, and no other product
string in either corpus contains `PC` at all across 11,053 files.

**Test the leading `PC` token and nothing else.** What follows it is a version string and must not
participate: `1.0+` is the only one observed, but the whole point of the token is that the platform
is stated separately from the version, and matching `PC 1.0+` whole would reject a Windows document
from any other release for no reason. The `+` also means this is not a version the reader can parse,
so such a file will carry no recovered version at all and every version-gated mapping will be
skipped for it. That is survivable precisely because the era's mappings are gated on the epoch
rather than on a version.

This is what the paragraph below was looking for, for the one era that most needed it, and it
arrives from the banner rather than from the uninterpreted header bytes it guessed at. It also
replaces the classifier's numeric-product test, which is what currently rejects these files: the era
has a product that is deliberately not a version number.

**Still suspected but not found for the 3.x era onward: a byte-order marker in the header.** A format that is
written on two platforms with opposite byte order, as the 3.x line demonstrably is, would ordinarily
record which one it used rather than leaving a reader to infer it. The header holds candidates: the
six bytes at `0x062` are still uninterpreted, and the Coda-banner era carries a word at `0x80`
described below. Finding such a marker would replace a heuristic with a fact, and
would also give the Coda-banner era a real test in place of its current assertion. This is **open**.

The header version tuples are a second consumer of the answer, and they fail quietly without it.
Each is a 32-bit value in the file's own byte order, so decoding one requires knowing that order
already. `header::decodeVersion` accepts a byte order and only guesses when given `Unknown`, by
trying big-endian and keeping the result if its major version falls inside Finale's 0-27 range.
That test is far weaker than it looks, because a swapped value frequently lands inside the range
too. A Windows Finale 3.0 document stores `0f 03 01 03`: big-endian that reads as major 15, which
passes, while the correct little-endian reading is `0x0301030f`, version 3.0.1 build 15. Its Mac
counterparts store `03 01 03 0b`, the same version at a different build. The reader is unaffected —
the container establishes the order before the header is described — but anything that describes a
header on its own gets a plausible wrong answer with no indication, which is worth knowing when
writing a probe.

**Half the answer is already in the header, and it is a fact rather than a heuristic.** The platform
strings at `0x074` and `0x09a` are text, so they are byte-order independent and readable before any
order is chosen. A document that originated on Windows **must** be little-endian: Finale's Windows
releases ran on x86, which has no big-endian mode, so there is no such thing as a big-endian
Windows Finale file. A `WIN` platform string therefore settles the question outright.

The relationship is asymmetric, and only the Mac side is ambiguous. Macs were big-endian on PowerPC
and little-endian on Intel, so `MAC` says nothing on its own. The corpus shows exactly that shape:

| Platform | Order | Files | Products |
|---|---|---:|---|
| `WIN` | LE | 20 | 3.0, 2000, 2001, 2002, 2004b, 2005, 2006, 2007, 2012 |
| `MAC` | BE | 628 | 3.2–97, 2000–2006, 2007, 2008 |
| `MAC` | LE | 439 | 2007, 2008, 2009, 2010, 2012 |
| absent | BE | 62 | 1.8.7, 2.0.1, 2.6 |

No Windows document in the corpus is big-endian, across nine products spanning Finale 3.0 to 2012.
The Mac split begins at Finale 2007 and genuinely overlaps — 2007 and 2008 appear in both rows —
which is the Intel transition rather than a format change.

A `WIN` platform string can therefore replace the trial for that whole population, including the
uncompressed era where the trial is only circumstantial.

**Windows Coda-banner documents exist, are little-endian, and the reader rejects them. Confirmed
2026-08-11 from specimens.** The Finale 2.2 for Windows install disks yielded 24 `.MUS` files —
eight templates and sixteen tutorial documents — and they settle every part of this at once.

They carry a previously unseen banner *product*, not a new spelling: `Finale(TM) PC 1.0+ Copyright
1987 by Coda. All rights reserved.` uses the same `Finale(TM)` spelling the Mac Coda-banner era
uses, but its product field is `PC 1.0+` rather than a version number. The distinction matters,
because the banner parser recognises the spelling perfectly well and it is the numeric-product test
that rejects these files. Their pool prologue reads
`pages=20, pagesize=512` little-endian and nonsense big-endian, and the first record at `0x208` is
`fe ff 31 30`: comparator `0xfffe`, the globals comparator, followed by the tag `01`, both
little-endian. The record model is therefore identical to the Mac era of the same period and only
the byte order differs.

Two independent things stop the reader from opening one, and both are confirmed by these files. The
container's classifier requires `hasNumericProduct()`, which `PC 1.0+` fails, so such a file never
reaches the Coda-banner parser at all. Past that, `parseCodaBanner` asserts big-endian and would
break at its first prologue check, because the page-size word does not read `0x200` in that order.

The rest of this note is what was known before those specimens arrived, and is left as the reasoning
that predicted them.

 Every one of its 62 corpus files is big-endian and none carries a platform string, but
Finale for Windows existed during that era: Microsoft Knowledge Base article Q107181, the README for
Windows Sound System 2.0, records that "Finale 2.2 for Windows from Coda Music Technology is
compatible with the Windows Sound System sound board and software if you modify the WIN.INI file",
and mentions MusicProse for Windows alongside it. Finale 2.2 sits inside this era's 1.8.7-to-2.6
range.

A little-endian Coda-banner document is therefore a real possibility rather than a hypothetical one,
and the container would misread it: `parseCodaBanner` hardcodes big-endian, so such a file would
decode with transposed words and tags that are not text, most likely presenting as an unrecognized
variant rather than as wrong data. No such file is in any surveyed corpus, so nothing is currently
misread.

The era does carry a usable test, which is what makes this fixable rather than merely recordable.
Each pool prologue holds a page-size word that reads as `0x200` in one order and `0x0002` in the
other, so the order can be trialled against the prologue instead of asserted. That is recorded as a
gap in [PRODUCTION_READINESS.md](PRODUCTION_READINESS.md).

Source, accessed 2026-08-11:
[Microsoft KB Q107181, "Contents of the Windows Sound System 2.0 README.TXT", revised 13 June 2001](https://jeffpar.github.io/kbarchive/kb/107/Q107181/).
This is `public-source-derived`: it dates Windows Finale to version 2.2 but says nothing about that
version's file format or byte order.

**The Mac side is determined too, for every era that matters here.** Finale 2007 was the first
Intel-native release. MakeMusic announced at NAMM in January 2006 that "the next version of Finale
... will be made as a Universal Binary, running natively on both PowerPC and Intel-based Macs", and
Finale 2007 shipped that August with "support for Intel-based Macs with a Universal binary
release". Finale 2006 and earlier were PowerPC binaries, which on an Intel Mac ran under Rosetta —
emulated PowerPC code, and therefore still writing big-endian regardless of the hardware underneath.

So a Mac-origin document saved by Finale 2006 or earlier must be big-endian, and only Finale 2007
and later can be either. The corpus matches exactly: every `MAC` little-endian file is 2007 or
later, and the 2007 and 2008 overlap is a user population still mostly on PowerPC rather than a
format difference.

Combining the two platform facts leaves nothing circumstantial before Finale 2007:

| Origin | Era | Order |
|---|---|---|
| Coda banner, product `PC ...` | Windows, stated in the banner product | little-endian |
| Coda banner, numeric product | Mac; the era has no other platform | big-endian |
| `WIN`, any era | x86 has no big-endian mode | little-endian |
| `MAC`, Finale 2006 or earlier | no Intel-native build existed | big-endian |
| `MAC`, Finale 2007 or later | universal binary; follows the machine | either |

The uncompressed era spans Finale 3.0 to 2000, so it falls entirely in the rows that a platform
string determines, and its "structural but circumstantial" trial now has a fact behind it rather
than only a pattern. The Coda-banner row is the exception: it is still an assertion, because that
era records no platform. The one
genuinely ambiguous population, Mac documents from Finale 2007 onward, is also the one where the
trial is strongest, because the compressed eras validate every block against a stored CRC-32.

This does not remove the value of finding a real marker, which would still let the container decide
before it has parsed anything and would cover a file whose platform string is missing or wrong. But
it does mean no era currently depends on a guess.

Sources, accessed 2026-08-11:
[NAMM announcement, Macworld, 18 January 2006](https://www.macworld.com/article/178521/finale-8.html);
[Finale 2007 release, Macworld, 6 August 2006](https://www.macworld.com/article/181055/finale-9.html);
[Finale 2007 release, MacTech, 7 August 2006](https://www.mactech.com/2006/08/07/makemusic-releases-finale-2007-music-software/).
These are `public-source-derived` and corroborated by the corpus rather than independently verified
against MakeMusic's own release notes, which are no longer online.

#### The word at 0x80, and the absent application version

**Confirmed across 138 files.** The Coda-banner header is entirely zero from `0x60` to `0x200`
except for one 16-bit field at `0x80`. It was previously recorded as a constant `01 03`; it is not
constant, and its value groups the era exactly:

| Product | Bytes at `0x80` | Files |
|---|---|---:|
| Finale 1.0.0 | `01 00` | 22 |
| Finale 1.8.7, 2.0.1, 2.6 | `01 03` | 92 |
| Windows, banner product `PC 1.0+` | `00 07` | 24 |

Those three pairs are the only ones in the corpus: all 234 distinct Coda-era documents take one of
them.

**It is not the application version.** It cannot be: it takes the same value for 1.8.7, 2.0.1 and
2.6, three products that differ. What it does track is the file format — one value for Finale 1.0.0,
another for the Mac releases after it, a third for Windows — which is consistent with the Windows
banner declaring a format of `PC 1.0+` rather than an application version.

**The best-fitting reading is that `0x81` is a flags byte and `0x04` marks the Windows build.** Its
values accumulate rather than enumerate: `0x00` for Finale 1.0.0, `0x03` for the Mac releases after
it, `0x07` for Windows. Read that way each release sets a further bit, and exactly one bit separates
Windows from the Mac line of the same period.

This is a better fit than treating `0x80` as the platform byte, for two reasons. First, that byte is
`1` on Mac and `0` on Windows, and Mac came first — a platform marker introduced when the second
platform arrived should leave the original at zero, not the newcomer. Second, if the field is a
16-bit word at all then it is subject to byte order, and `0x80` is therefore the high byte on Mac
and the low byte on Windows: comparing the two compares different halves of the field, so the
`1`-versus-`0` pattern is an artefact of the comparison rather than a finding.

What `0x04` actually means is **open**, and one specimen cannot separate the possibilities: with a
single Windows value observed, "little-endian", "PC build" and "the next feature bit, which happened
to arrive in the PC release" all predict the data equally. A Mac Coda document with the bit set, or
a Windows one without it, would distinguish them. Nothing reads this field — the banner's `PC` token
states the platform outright — so it is recorded rather than relied on.

**The field does not survive the Finale 3.0 redesign.** Across 1,500 signature-era files from
Finale 3.0 to 2012, `0x80` through `0x8b` is a twelve-byte run of zeros in 1,495 of them. That is
because the signature header states explicitly what the Coda word encoded implicitly: a Finale 3.7
document carries its version tuple at `0x6c`, the application `FIN` at `0x70`, the platform `MAC` or
`WIN` at `0x74`, an application version at `0x78` and a file version at `0x7c`, and the created block
ends exactly where the Coda field used to be. The modified block then begins at `0x8c`, leaving the
intervening twelve bytes unused.

Two documents out of roughly a thousand distinct signature-era files carry a single stray byte at
`0x80` — `01` in a Finale 2000 document and `03` in a Finale 2003 one, with `0x81` onward still zero.
Neither reproduces a Coda pair, so a stale remnant of an upgraded document is a guess rather than an
explanation, and the cause is **open**. Nothing reads the byte, so nothing depends on it.

**No application version is stored anywhere in these files.** A search of all 24 Windows documents
for the version their application reports, 2.2, found no encoding of it — not `0x0202`, `0x0220`,
decimal 22 or 220, nor the text `2.2` — at any offset common to all of them. Their exact Finale 27
companions corroborate it from the other side: every one carries a `<modified>` block naming Finale
27 itself and **no `<created>` block at all**, so Finale 27 recovers no creator version from them
either. For this era the banner product is the only version there is, which for Mac documents is the
application version and for Windows documents is a format marker that is not one.

### Coda-banner files

Previously described here as "pre-banner", which is inaccurate: these files do have a banner.

**Confirmed.** These files lack the `ENIGMA BINARY FILE` signature but open at offset 0 with a plain-text product banner reading `Finale(TM) <version> Copyright 1987 by Coda. All rights reserved.`. The banner is the only place their version appears: bytes 0x60-0x200 are entirely zero apart from a constant `01 03` at 0x80, which is a candidate format version rather than an application version.

Finale 1.0.0 belongs to this family but spells the banner a third way: `Finale` followed by a MacRoman trademark sign (0xAA), the version, and `ENIGA Structures` (sic) in place of the copyright notice, as in `Finale™ 1.0.0 ENIGA Structures Copyright 1987 by Coda.`. It shows the same absence of an Enigma tuple — 22 of 22 files in `rpatters1-installs` yield no `FIN` application string at the offset where later eras carry one — which is consistent with the zeroed 0x60-0x200 region described above rather than with a differently placed tuple.

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

Because the version is explicit and machine-readable, `scripts/inventory.py` reads both banner spellings and reports these as `1.8.7`, `2.0.1`, and `2.6` rather than `unknown`. Matching only `Finale(R)` had filed the whole era as unclassified.

The `(TM)` spelling is what separates the era. All signature-bearing files spell it `Finale(R)`, and later banners name Coda as well, so the copyright holder does not distinguish the two.

These files reserve the same 0x200 header as later eras, and the region at 0x200 begins with a page count followed by the page size, as in `000000ab000002000000444100300000`. That second word, not anything at the top of the file, is the structural confirmation: all 229 satisfy it, while the AppleDouble metadata artifact in the corpus satisfies neither test. Corpus ID `mus-6d3b75475a2ca67b` is also Coda-banner but has weaker provenance.

#### Chained pools

**Confirmed** across all 229 Coda-banner files, spanning versions 1.8.7, 2.0.1, and 2.6. There is no per-block framing in this era. Instead the file is a chain of pools starting at 0x200, each laid out as:

| Field | Width | Meaning |
|---|---|---|
| page count | 4 bytes | number of 512-byte pages of record data |
| page size | 4 bytes | always `0x00000200` |
| records | page count × 512 | record rows, zero-padded to the page boundary |

Each pool begins immediately after the previous one ends. Every file contains **exactly three pools**, and the walk terminates in the same place every time. The page count is a count of pages, not of records: a file whose first pool declares 46 pages carries far more than 46 rows.

Pool 0 is the tagged others pool and pool 1 is the details pool; both are described under
[record pools and row shapes](#record-pools-and-row-shapes-through-finale-2006), which covers every
epoch through Finale 2006. Pool 1 was initially reported here as having no tags, because it was
measured at the others tag offset; a detail's second comparator displaces its tag by two bytes.
Pool 2 shows no tag at any offset or stride and is presumed to hold entries, which remains
inference rather than measurement.

#### Pool 0 rows

Rows are 16 bytes in the shared others shape, and incidence is implicit in encounter order with no
incidence field, exactly as in the later fixed-row eras.

Trailing space in the final page is zero-filled, so an all-zero row marks the end of populated records. This holds for 51 of the 54 then known files; the other three fill their pages exactly. A row of `ffffffff` can appear *within* the populated region and is not a terminator: the Finale 1.8.7 file `mus-7aa45639c14b3864` carries one at a point where valid rows continue afterward. Populated row counts across the 54 files run from 602 to 11,299, median 5,797.

Comparator 65534, which later eras use for synthetic preference records, is already in use here. `mus-7aa45639c14b3864` carries 150 such rows under numeric tags `01`-`43`, `50`-`55`, `62`-`65`, plus `40` and `fi`. The `^NN(65534)` preference mechanism therefore predates the Enigma signature. Selector `94`, which the distilled mapping uses for music spacing, is **not** present in that file.

Two tags that the distilled PDK mapping relies on, `FI` and `HE`, do not occur anywhere in pool 0 of
any of the 54 files then known. Both searches predate the correction below about tag byte order, but this era
is uniformly big-endian, so they are unaffected.

#### Enigma string region

**Confirmed** across all 54 files then known. After the third pool the remainder of the file holds exactly two length-prefixed chunks, each a 4-byte big-endian byte count followed by that many bytes of text. Walking them lands precisely on end-of-file in every case. The first chunk always begins with `^` and carries `^text()` followed by `^block(n)` sections; the second is `^lyrics()`.

Enigma string markup therefore already exists in Finale 1.8.7: `mus-7aa45639c14b3864` contains `^font`, `^size`, and `^efx` commands inside its text chunk. This region is better delimited than the banner era's mixed `0x0017` block, because the pool chain locates it exactly with no scanning.

#### Header text records

The `HT` tag in pool 0 carries page text rather than document metadata. Each logical text block occupies **four consecutive incidences**, that is 48 payload bytes, holding a NUL-terminated string followed by numeric fields. Every `HT` family observed across 20 files has an incidence count that is a multiple of four, in 32 of 32 families. In `mus-7aa45639c14b3864` the comparator-1 family holds five blocks in twenty incidences: a composer credit, a dedication, a copyright line, a typesetting credit, and the title.

These are page titles, not document metadata. Whether this era stores document metadata at all is **open**.

Text in these records is **Mac OS Roman**, not Latin-1: `0xa9` renders as `©` in the copyright line and `0xaa` as `®` in a `Finale®` credit. Any import of this text needs the encoding conversion step before the strings are usable.

## Record pools and row shapes through Finale 2006

**Confirmed** on 2026-08-09 against the controlled fixtures, seven large corpus files of the
2002-2006 era, three uncompressed-era files, and all 54 Coda-banner files then known.

Every epoch through Finale 2006 stores its records in four pools in the same order, and only the
container framing differs. Pool identity was established by measuring, for each pool, the fraction
of 16-byte rows whose bytes at a given offset are alphanumeric.

| Logical pool | Coda banner | Uncompressed | DCL | Row test |
|---|---|---|---|---|
| others | pool 0 | `0x0001` | `0x000f` | 0.97-1.00 alphanumeric at offset 2 |
| details | pool 1 | `0x0002` | `0x0010` | 0.95-1.00 alphanumeric at offset 4 |
| entries | pool 2 | `0x0003` | `0x0011` | ~0.00 at both, size frequently not a multiple of 16 |
| text | length-prefixed chunks | `0x0004` | `0x0012` | ~0.55 at both, the signature of mixed ASCII |

The text pool is not made of rows at all; it is a byte stream of Enigma text chunks, and the
zlib epoch carries it in block `0x0017`. See [The text pool](#the-text-pool).

The others and details tests are mutually exclusive in every file examined: a pool scoring high at
offset 2 scores near zero at offset 4 and the reverse. That is a direct consequence of the two row
shapes, which are the only two needed to normalize the whole pre-2007 range:

| Shape | Layout | Payload |
|---|---|---|
| other | comparator (2), tag (2), payload (12) | six 16-bit words |
| detail | comparator 1 (2), comparator 2 (2), tag (2), payload (10) | five 16-bit words |

**Confirmed: the tag is a 16-bit value, not a character pair, and is subject to byte order like
every other field of the row.** A little-endian file stores `FN` as the bytes `NF` and `LA` as
`AL`. Reading the two bytes literally mismatches every tag in such a file, and the failure is
silent: the pool decodes, the rows look structurally valid, and no lookup ever matches. A Finale
3.0 file read that way appears to contain an unfamiliar vocabulary of `NF`, `AL`, `RF`, `SM`, `bs`,
`co` records, which is simply `FN`, `LA`, `FR`, `MS`, `sb`, `oc` reversed, alongside numeric tags
that read as `10`, `40`, `50` instead of `01`, `04`, `05`.

This is worth stating plainly because it is easy to mistake for a format difference. Before the
byte order was applied to tags, every little-endian file in the corpus recovered nothing, and the
failures looked like unframed variants or an older record vocabulary rather than a decoding error.
Correcting it took the Finale 2001-2006 recovery rate from 410 of 426 files to 426 of 426.

A detail carries a second comparator, which displaces its tag by two bytes. This is the same
distinction the ETF makes by argument count: in the 2.6-era ETF `mus-2c0a5e8897b436d5`, all
thirteen tags printed with two arguments (`AS`, `BL`, `CL`, `CN`, `ED`, `GF`, `IM`, `LL`, `MT`,
`ST`, `TN`, `TP`, `Ts`) appear in the binary details pool at offset 4, and none appear in the
others pool.

Entries are neither shape. Their pools are often not a multiple of 16 bytes, consistent with the
32-byte implicit-ID entry rows reported for the early family, so a reader must not apply the
others row stride to them.

Two cautions for anyone reproducing this:

- **The controlled fixtures cannot demonstrate the details pool.** Their `0x0010` block is roughly
  half a kilobyte and scores 0.09, because the fixtures contain almost no detail-bearing content.
  Real corpus files of the same era score 0.98-1.00. Any test of details framing needs a file with
  real content.
- **Coda-banner pools are page-padded, so a naive fraction understates them.** Measured over whole
  pools including padding the details score looks as low as 0.47. Excluding all-zero padding rows
  it is 0.965 across all 54 files then known, in line with the banner eras. The remaining 3.5% are not ASCII
  tags at all but sequential high-bit values, `0x8001`, `0x8002`, `0x8003` and so on, whose meaning
  is **open**. The Coda details pool also carries ten tags the ETF does not print with two
  arguments (`AC`, `BH`, `CD`, `DE`, `DO`, `MM`, `Te`, `UE`, `sB`, `ve`), which is **open** as well.

### Shape definitions, instructions, and data

**Confirmed for the controlled Coda, fixed-row, DCL, and zlib fixtures; strong across the broad
corpus sweep.** `ShapeDef` uses `SD`, with instruction/data families `SL`/`SB` in the Coda era and
`sL`/`sb` from Finale 3 through 2006. Zlib class ids `0x00d6`, `0x00d7`, and `0x00d5` carry the same
three families respectively. Each fixed instruction or data row holds three signed 32-bit values;
the two normalized 16-bit words composing a long are high-word first on Mac and low-word first on
Windows. Zlib payloads use the container byte order directly.

The instruction long is `revision:numData:tag`, with one byte each for revision and data count and
the low two bytes holding the two-character instruction tag. A zero long terminates the logical
list. This is not padding that may be skipped: real files retain nonzero stale instructions after
zero, while their exact Finale 27 upgrades omit that tail, and consuming it makes the instruction
data count impossible. The importer recognizes the instruction tags represented by musxdom's
`ShapeDefInstructionType`; revision-1 `sw` becomes `LineWidth`, with its data converted from
hundredths of a point to Efix, and the Coda meaning of `gs` becomes `GoToOrigin`.

The first two `SD` words are the instruction-list and data-list comparators. Only the Finale
3-2006 fixed-row layout stores modern `ShapeType` in word 2. Coda `SD` and zlib `0x00d6` put the
old bounding rectangle after the list ids, so the reader supplies `Other`; testing a coordinate
for the enum's numeric range would silently misclassify small bounding coordinates.

The `shape_definitions` sweep selected all 15,841 inventoried occurrences in `rpatters1-main` and
`rpatters1-installs`, de-duplicated to 6,890 content identities. All 6,890 imported. Of 364,482
source definitions, 4,816 stored zero list references, 34 stored nonzero references to resolved
empty instruction lists, and 359,632 nonblank definitions resolved both supporting collections;
none of those nonblank shapes consumed more data than was stored, and none retained an undocumented
opcode after applying the zero terminator. Eight Finale 2.2 Windows files contributed 32 of the
resolved-empty definitions: their referenced `SL` families terminate immediately and their `SB`
families are absent. Two Finale 2.6 files contributed one resolved-empty definition each. Finale's
UI displays the Windows shapes as blank. The reader preserves their references and bounds, and
musxdom recognizes a resolved empty instruction list as `Blank`; neither layer has to discard
source data to obtain the source application's semantics.

Another 41 selected identities do not classify as legacy score containers and recover no source
shape definitions. Private location metadata strongly identifies these as library artifacts rather
than an uncovered score epoch: 36 occur in library locations, while their suffixes are ten `.lib`,
29 extensionless names, and two `.mus` names. They remain in the all-files denominator so that a
misnamed library cannot silently become a successful score import, but they are not evidence of a
ShapeDef layout missing from a recognized score container.

All eight adjacent-exact Finale 27 companions instead synthesize the same visible rectangle
instruction sequence and data from the old `SD` bounds. That is deliberate upgrade behavior, not
source content: reproducing it would make the imported shapes nonblank when the source application
shows them blank. Companion comparison must therefore classify these 32 shapes as `synthesized`
rather than as a reader discrepancy.
Thirty-one recovered instructions reference graphics. `ShapeGraphicAssign` uses fixed-row tag
`sg` through Finale 2006 and zlib class `0x00d8`; its payload is the same 18-word placement tuple
described for page graphics below. Thirty of the 31 instruction operands resolve through
musxdom's assignment lookup. The remaining DCL source names graphic 3 but defines assignments only
for graphics 1 and 2, so the dangling reference is source content rather than a dropped record.
**Confirmed** across `rpatters1-main` and `rpatters1-installs`.

The tag spellings and revision-1 conversion agree with Finale 2000 PDK `SHAPETAG.H` and `edata.h`
at immutable commit
[`9f74ba9b3e287f240bbd454c2259fc3f7737c6ad`](https://github.com/rpatters1/finale-pdk-framework/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/SHAPETAG.H)
(accessed 2026-08-13). Those names are **public-PDK-derived**; the binary packing, byte order,
terminator, epoch layouts, and semantic conversions were checked independently against the
controlled fixtures and corpus observations.

### Font definitions

**Confirmed** against the controlled Finale 2002 fixture and its ETF, and applied to all 669
pre-zlib corpus files, which yield 12,759 definitions with no empty name.

A font definition is one `FN` family in the others pool, comparator equal to the font id used
throughout the document.

| Incidence | Contents |
|---|---|
| 0 | header: character set word, then pitch and family word, then four unused words |
| 1 onwards | the font name, as many rows as it needs |

The header's first word packs the character set: the high nibble is the bank, 1 for a Mac font and
2 for a Windows font, and the remaining twelve bits are the character set value. The bank describes
where the *font* came from, not where the document was written. The values agree with musxdom's own
documentation of the field, which records `0xfff` as the macOS symbol character set and 2 as the
Windows one. The fixture stores `0x1fff` for Maestro and `0x1000` for Times, and its ETF prints the
same headers as `^FN(0) 8191 0 0 0 0 0` and `4096`.

The header's second word carries the pitch in its low nibble and the family in its high nibble.
Across 7,622 header incidences it is non-zero only for Windows-bank fonts: all 7,355 Mac-bank fonts
store zero, while Windows-bank fonts take pitch 1, 2, or 7 and family 0, 1, 2, 4, or 5. Family 4
selects exactly the script and blackletter faces in the corpus, which is independent support for
the nibble boundary. The remaining four header words are unused; not one is non-zero anywhere in
the corpus.

The name is read as bytes, not as words, because character payloads are not byte-order sensitive,
and it ends at the first NUL. Rows are fixed width, so a name shorter than the row is padded and a
longer one simply continues into the next incidence.

**Before Finale 3.2 there is no header incidence**: the family opens with the name and no character
set is recorded. Observed in every Coda-banner file and in the Finale 3.0 files, against Finale 3.2
and later where the header is always present. The exact boundary is **open**: the corpus holds no
Finale 3.1, and its only Finale 3.0 files are Windows-origin, so a platform explanation cannot be
excluded.

#### Unresolvable comparators and the `Missing Font (n)` placeholder

**Confirmed.** Finale names a font it cannot resolve with a placeholder definition called
`Missing Font (n)`, where `n` is the comparator itself. Across 2,756 Finale 27 companions the record
is invariant in all 641 occurrences spanning 354 documents: bank `Mac`, `charsetVal` 0, `pitch` 0,
`family` 0, and the name formed from the comparator in every single case. It does not vary with era,
product, or comparator. The commonest comparators are 131, 512 and 100, at 193, 193 and 182
occurrences.

**It is not a conversion artifact.** Finale wrote the same placeholder into the legacy font table at
save time as far back as Finale 2009, so most documents carrying one recover it as `legacy-mus` and
agree with the companion with nothing done on our side. Finale 27 applies the rule unconditionally,
including to comparators that are plainly garbage: one corpus document has a corrupt font table whose
comparators decode as ASCII character pairs — 12596 (`"14"`), 25203 (`"bs"`), 26990 (`"in"`) and
similar — and each one still receives a correctly formed placeholder.

musxdom synthesizes the same record for any comparator registered during construction that the
finished document does not define. That is what keeps `FontInfo::getName` — and `calcIsSameTypeface`
and `calcIsSMuFL`, which route through it — from throwing on a dangling reference. The reader's whole
obligation is to register the comparators it leaves in the document, and to register **what a field
finally holds rather than every value it passes through**: `FontOptions` replaces a recovered
comparator whose definition is absent, and registering the discarded one would mint a placeholder
that nothing refers to. musxdom also registers font ids off `SetFont` instructions in every
`ShapeDef` once the document is built, which the reader does not need to duplicate.

Two Finale `14.0.0.11` documents reach this through the accidental-symbol inserts: their flat and
dblSharp inserts name comparator 100, the source's own table defines fourteen fonts and none of them
is 100, and the placeholder now makes the reader agree with the companion exactly where it
previously left the reference dangling.

#### A shape naming a font the source never defines — third deliberate disagreement

**Confirmed**, on five DCL documents, all Finale 2002 `7.0.1.2`. Take `mus-0bb3c333c0f80358`. Its
`FN` families run 0–13 and 22 and stop there. Shapes 14 and 15 are custom tab clefs whose `SetFont`
instruction names comparator **14**, which the file never defines — a dangling reference in the
source document itself, not something conversion introduced.

Finale 27 resolves it, but only by accident. Its converter prunes the source's duplicate font
entries — 0 and 6 are both `Pmusic`, 3 and 10 both `Symbol`, 8 and 13 both `Petrucci`, 9 and 11 both
`Sonata` — and injects its own defaults into the vacated numbers, which lands `Maestro Percussion`
at comparator 14. The shape's dangling reference then silently resolves to it. The strings `Maestro`
and `Engraver` appear nowhere in the source file's records, so those faces are Finale 27's, not the
document's.

That resolution is wrong on the merits. Both shapes draw plain ASCII at 12 point — shape 14 emits
`T`, `1`, `2`, then `-`, space, `0`; shape 15 emits `T`, `2`, then `-`, space, `2` — so the font
wanted is a text face, reported to be Helvetica or Arial with Times for these symbols, and never a
percussion music font. Finale 27 renders tab-clef letterforms as percussion glyphs. The reader keeps
the comparator as stored and lets it read `Missing Font (14)`, which is both truthful and safer to
render, since a name that resolves to nothing falls back to a system face and still produces letters.

**This is the one class of companion difference where matching Finale 27 would be the defect.** It is
recorded so a later coverage run does not re-open it as a regression.

#### Reading `companion-face-missing`

That metric counts faces the companion has and the reader does not, and it must **not** be read as a
recovery deficit. Across the reference corpus it is dominated by faces Finale 27 injects during
conversion rather than anything the source named: of 2,956 occurrences over distinct documents,
`Lucida Grande` (1,065), `Engraver Text T` (799), `Times New Roman` (449) and `Maestro` (327) are
2,640 of them, or 89%. Not reproducing those is correct — the reader does not seed font definitions,
because a pinned definition would collide with the source record sharing its comparator.

The interesting residue is small: 165 occurrences of `Missing Font (100)` where the companion carries
a placeholder and the reader does not. Those are comparators referenced only from option classes the
reader has not imported yet, so nothing registers them and no placeholder is minted. The count should
fall as those classes land, and it is a coverage measure rather than a font-table one.

### The 2007-2012 record encoding

**Confirmed** for the font record against Finale 27's own conversion of the same document, and
**strong** for the general shape, which is inferred from that one record type.

The 2007 serialization abandons the fixed 16-byte row. Ordinary records in block `0x001a`
are variable-length and self-describing:

| Field | Width | Meaning |
|---|---|---|
| class id | 2 | numeric identifier standing in for what EnigmaXML names as an element |
| cmper1 | 2 | primary comparator, as in every earlier era |
| incidence | 2 | incidence, as in every earlier era |
| length | 4 | payload size |
| payload | length | per class |
| padding | 4 | zero in every observed record |

Detail records in block `0x001b` add a 2-byte `cmper2` between `cmper1` and incidence. The
big-endian form then carries a 16-bit payload length; the little-endian transition form carries
the length across the next two words and begins its payload two bytes later. This is **strong**:
class `0x041d` supplies six
`MeasureGraphicAssign` records in two distinct Finale 2008 documents; its second header
comparator exactly supplies the companion measure numbers, while the primary comparator supplies
staff IDs and the payload is the same 20-word tuple stored by pre-zlib `mg` details. It is also
the same relative placement used by the fixed detail row. The reader treats this as the zlib
detail invariant, subject to revision if a contrary detail class is found.

The logical model is therefore unchanged. What moved is the physical encoding: the
two-character tag became a numeric class id, and the fixed six-word payload became a
length-governed byte payload. `RECORD_CATALOG.md` already catalogs these numeric identifiers,
and `0x0090` is the font definition, which that catalog lists as `fontName` at `weak`
confidence.

The font payload keeps the earlier character-set encoding unchanged: `0x1fff` for a Mac symbol
font and `0x1000` for a Mac text font at payload offset 0, the pitch and family pair at offset 2,
and the name from offset 12 to the end of the payload. A longer name simply grows the payload:
`Maestro Percussion` carries length 36 where short names carry 24.

Verified against `Score-Fin12.mus` and the Finale 27 conversion of the same document, which agree
on every comparator, every gap in the comparator sequence, every name, and every character set
value.

**Hypothesis, not yet examined: the sharing data should be here too.** EnigmaXML carries part and
sharing information as attributes on each element, and this encoding otherwise lines up field for
field with that model. If the correspondence holds, the part comparator and share mode should
appear in the record header or the payload prologue rather than being derived. This is **open**
and no evidence has been gathered for it yet.

## Archive-derived early-version evidence

The expanded archive survey changes the earliest-version picture. StuffIt extraction with `unar` 1.10.7 produced explicit Coda-banner samples labeled Finale 1.8.7, 2.0.1, and 2.6, plus additional 3.0–3.7 files. These files should not be conflated with the 55 direct Coda-banner/unknown files. Public IDs and hashes are in `data/archive_members.csv`; original archive/member locations are private.

**Superseded.** This section previously stated that 1.8.7 was the earliest explicit product observed and that no
explicit Finale 1.0 sample had been found. The `rpatters1-installs` survey holds 22 loose Finale 1.0.0 files — 14
samples, 6 templates and 2 tool demonstrations — so the earliest explicit product is now 1.0.0, and it is not
archive-derived. They were invisible to earlier surveys for two compounding reasons, both since fixed in
`scripts/`: they carry no extension, and their banner uses a third spelling, `Finale` followed by a MacRoman
trademark sign (0xAA) and terminated by `ENIGA Structures` (sic) rather than a copyright notice, which neither the
`Finale(R)` nor the `Finale(TM)` pattern matched.

Finale 27 successfully opened the selected 1.8.7, 2.0.1, and 2.6 files after `.mus` was appended to their
filenames and produced private `.fin27.musx` references. Thus there is no parser compatibility cutoff among these
tested versions. The initial failure mode was file recognition for extensionless classic-Mac documents, not rejection
of their data format. The 2.6 conversion reported font issues, which affect rendering fidelity but did not block
conversion. Finale 1.0 remains untested.

### ETF evidence set

**Confirmed.** Fifteen locally retained ETF exports now provide a semantic record-level reference for the older
families. The original six archival exports remain under ignored `private/evidence/`; the controlled F2002–F2005 pairs
are tracked under `tests/evidence/` because they contain no private source layout. Their hashes and provenance are recorded
in the experiment log.

| Evidence | Source era | ETF size | Observed sections | Selected observations |
|---|---|---:|---|---|
| `nestedTupletFin05RC2.etf` (`mus-d89e8fe12e271440`) | Finale 2005 | 16,893 bytes | header, others, details, entries, text, lyrics | Explicit ETF header identifies Finale 2005; six `eE` entry records and tuple/detail records expose the high-entropy era's logical model. |
| `template-Fin2000-from-Fin2000.etf` (`mus-3597fd4fce0c272b`) | Finale 2000 | 27,945 bytes | header, others, details, entries, text, lyrics | Explicit header identifies Finale 2000; no `eE` entries; compact options/defaults and text blocks. |
| `template-Fin2000-from-Fin2005.etf` | Finale 2000 source, Finale 2005 saver | 34,029 bytes | header, others, details, entries, text, lyrics | Same source document but Finale 2005 header; adds `&f`, `PD`, `XA`, expressions, and other records. This is direct evidence that resaving can synthesize/upgrade records. |
| `tremolos-from-Fin2000.etf` (`mus-3a8b724cf3adba80`) | Finale 2000 | 28,718 bytes | header, others, details, entries, text, lyrics | Exact source-version pair: 1,107 others, 64 details, eight entries, and raw text match the uncompressed binary pools; includes `CN`, `GF`, and `TP`. |
| `guitar pc.etf` (`mus-7aa45639c14b3864`) | Finale 1.8.7 | 123,084 bytes | others, details, entries, text, lyrics | 1,094 `eE` entry lines and 891 detail lines; no modern binary-style header section. |
| `Dream of Summer I-from-Fin2.6.3.etf` (`mus-2c0a5e8897b436d5`) | Finale 2.0.1 source, Finale 2.6.3 exporter | 74,040 bytes | others, details, entries, text, lyrics | 549 `eE` entry lines and 497 detail lines; old ETF uses the same broad logical sections despite the Coda-banner binary family. |
| `Score-from-sit-archive.etf` (`mus-bd0042f8e0354192` source class) | Finale 2.6 | 1,272,164 bytes | others, details, entries, text, lyrics | 9,446 `eE` entry lines and 6,814 detail lines; the large sample is suitable for testing whether early records scale regularly. The StuffIt copy was necessary because the ZIP copy lacked the resource fork. |
| `F2002-baseline.etf` | Finale 2002a.r1 | 14,068 bytes | header, others, details, entries, text, lyrics | Three `eE` entries; exact pair with `F2002-baseline.mus`. |
| `F2002-changed-C-to-D.etf` | Finale 2002a.r1 | 14,075 bytes | header, others, details, entries, text, lyrics | Same three entries, with localized pitch-related field changes; exact pair with `F2002-changed-C-to-D.mus`. |
| `F2003-baseline.etf` | Finale 2003a.r1 | 16,033 bytes | header, others, details, entries, text, lyrics | Three `eE` entries; exact pair with `F2003-baseline.mus`. |
| `F2003-changed-C-to-D.etf` | Finale 2003a.r1 | 16,034 bytes | header, others, details, entries, text, lyrics | Same pool layout; one-byte length increase in the entry record and localized entry-payload change. |
| `F2004-baseline.etf` | Finale 2004c.r1 | 16,334 bytes | header, others, details, entries, text, lyrics | Three `eE` entries; same typed pool sequence, with a larger `0x0010` record than F2002/03. |
| `F2004-changed-C-to-D.etf` | Finale 2004c.r1 | 16,334 bytes | header, others, details, entries, text, lyrics | Entry pitch changes plus `BC` records; automatic note spacing is the leading explanation for the derived changes. |
| `F2005-baseline.etf` | Finale 2005b.r1 | 16,381 bytes | header, others, details, entries, text, lyrics | Three `eE` entries; same typed pool sequence. |
| `F2005-changed-C-to-D.etf` | Finale 2005b.r1 | 16,381 bytes | header, others, details, entries, text, lyrics | Entry pitch changes plus the same `BC` dependency pattern seen in F2004; automatic note spacing is the leading explanation. |

Targeted Finale 27 conversions of the three earliest ETF-backed sources are retained locally:

| Private semantic reference | Size | SHA-256 | Finale 27 pool counts (others/details/entries) |
|---|---:|---|---:|
| `guitar pc.fin27.musx` | 60,732 | `4742e6fc35dd6892a6548c45c75c19dd139a6eb1be302733d8725a73d49ccd06` | 2,018 / 787 / 1,320 |
| `Dream of Summer I.fin27.musx` | 38,937 | `07d9f18fae4973f31fc69c8f0ecc551cfb8aa79b2131d2ff6dbdbeb818ab1328` | 1,347 / 478 / 659 |
| `Score-from-sit-archive.fin27.musx` | 392,491 | `bea52ad03c93c6ba5c7d6dd41c7a2c3ca57675ad5510bd68795963172cc1f311` | 14,913 / 6,095 / 11,153 |

The counts differ from ETF and binary physical rows, so these remain normalized semantic references rather than
serialization maps. Finale 27 reported font issues for `Score`, but generated a valid MUSX container.

ETF records are textual and use explicit section names plus two-character (or extended) structure identifiers, with
`(cmper)` for “other” records and `(cmper,inci)`-like keys for details. `eE` is the prominent entry structure in the
older exports. This confirms that the conceptual “other/detail/entry/text” decomposition predates the 2007 typed-zlib
container. ETF **by itself** did not identify the pre-2007 binary codec because it contains no binary offsets,
compressed bytes, or direct byte-for-byte rendering guarantee. Now that PKWARE DCL decoding is solved for 2001–2006,
an ETF paired with the exact `.mus` saved by the same Finale version is valuable as known semantic input for decoded
record and field correlation, especially with controlled edits.

The two exports of the same Finale 2000 template are especially important: the Finale 2005 export adds records and
changes some defaults, while the header changes from Finale 2000 to Finale 2005. A later-version ETF must therefore be
treated as a normalized semantic reference, not as a lossless reconstruction of the original binary save.

### Correlation between Finale 1.8.7–2.6 and Finale 3.0

**Strong physical and semantic continuity; framing still differs.** Re-extraction of the three ETF-backed early
binaries shows that all begin their main record region at `0x20a`, ten bytes after the common `0x200` body boundary,
and maintain a 16-byte cadence. They do not use Finale 3.0's four typed/length pool headers.

The early ordinary layout places the two-byte tag first, followed by the 12-byte payload and 16-bit comparator. Finale
3.x moves the comparator to the front but retains the same 16-byte capacity. Early detail tags occur ten bytes into
each 16-byte row; ordinary two-character tags and the compact pseudo-detail identifiers are directly recognizable.
The mapping `#v1`–`#v10` → `0x8001`–`0x800a`, `#c*` → `0x9001`–`0x900a`, and `#s*` →
`0xa001`–`0xa00a` is exact in the three samples.

Tag-order correlations are unusually strong:

| Source | Ordinary correlation | Detail correlation | Entries | Text |
|---|---|---|---|---|
| Finale 1.8.7 `mus-7aa45639c14b3864` | all 1,447 ETF ordinary tags match from the first binary row | all 891 tags match | all 1,094 32-byte rows reconstruct byte-for-byte from `eE` | ETF text is an exact 405-byte prefix of the raw binary tail |
| Finale 2.0.1 `mus-2c0a5e8897b436d5` | first 38 tags match; Finale 2.6.3 export then adds/reorders ordinary data | all 497 tags match | all 549 rows reconstruct byte-for-byte | exact 936-byte text prefix |
| Finale 2.6 source class | first 80 tags match in the ZIP data fork used for comparison; source-copy/build normalization remains possible | all 6,814 tags match | all 9,446 rows reconstruct byte-for-byte | exact 1,401-byte text prefix |

The early entry ID is implicit in row position. Each entry occupies 32 bytes and corresponds directly to one `eE`
record. Finale 3.x's 38-byte entry row is the same 32-byte entry data with a four-byte explicit entry ID prepended and
a two-byte zero/reserved suffix appended. This is a direct structural bridge across the 2.x/3.0 boundary.

Unexplained index/directory spans remain between the ordinary, detail, entry, and text regions. Thus a 1.x–2.x reader
cannot yet locate every pool generically, even though its core rows and semantics are closely related to Finale 3.0.
Finale 27 opened all three tested sources once their extensionless names were given a `.mus` suffix. This separates
container compatibility from classic Mac file discovery: suffix handling and missing type/creator metadata can make
a readable data fork appear unsupported. The three conversions are semantic references, not proof of losslessness.
Their Finale 27 pool counts differ substantially from ETF, confirming normalization: 1.8.7 converts to
2,018 others/787 details/1,320 entries; 2.0.1 to 1,347/478/659; and 2.6 to 14,913/6,095/11,153.

## Platform coverage risk

The corpus is overwhelmingly Macintosh-derived. Header platform tuples, classic Mac resource forks, Macintosh archive containers, and Mac-originated conversion history dominate the evidence. Four banner-era Windows files nevertheless establish one concrete platform difference: three Finale 3.0 files and one Finale 2000 file serialize the otherwise identical four-pool container and fixed rows little-endian, while 185 recognized Mac-era samples are big-endian. This does not resolve Windows string, option, padding, or later-era behavior; Windows remains a separate validation axis before declaring a parser cross-platform.

## Proposed format eras

| Era | Products in corpus | Structural evidence | Status |
|---|---|---|---|
| Pre-banner | apparent Finale 2 | Distinct header/body; no `ENIGMA BINARY FILE` | Separate parser likely |
| Uncompressed fixed-row legacy | 3.0, 3.2, 3.5, 3.7, 97, 2000 | Four typed/length pools; platform byte order; 16-byte other/detail rows, 38-byte entries, raw text | Container and physical rows solved; tag fields incomplete |
| DCL-compressed legacy | 2001–2006 | Big-endian typed/length blocks; CRC-32; PKWARE DCL; fixed 16-byte other/detail rows and 38-byte entry rows | Container, codec, and physical pool rows solved; logical field mapping incomplete |
| Typed zlib transition | 2007–2008 | Four typed blocks; 2007 is mixed big/little endian, 2008 mostly little endian | Wrapper solved; records partly solved |
| Typed zlib stable | 2009, 2010, 2012 | Same four principal blocks and CRC validation; little endian in all but transition exceptions | Best implementation target |

The release boundary is not absolute. Of 108 Finale 2007 files, 81 validate as big-endian wrappers and 27 as little-endian; all report `MAC` except one little-endian Windows file. Of 182 Finale 2008 files, 180 are little-endian and two big-endian. Therefore version plus an observed wrapper/CRC test is safer than version alone.

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

The exact Finale 2000 pair `mus-3a8b724cf3adba80` (`tremolos.mus`) is decisive. Its blocks are:

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

## Finale 2001–2006 typed DCL blocks

**Confirmed.** The high-entropy payloads use the PKWARE Data Compression Library (DCL) format decoded by Mark
Adler's open-source [`blast`](https://github.com/madler/zlib/tree/master/contrib/blast) implementation. This is the
format produced by the PKWARE DCL `implode()` function, not the incompatible PKZIP compression method that was also
named “implode.” `blast` is shipped as a small, permissively licensed contribution in the zlib source tree, but it is
not part of the ordinary installed zlib API and should be built or vendored separately.

At `0x200`, a compressed block has this big-endian layout:

| Field | Size | Meaning |
|---|---:|---|
| block type | 2 | numeric pool/block identifier, commonly `0x000f`–`0x0013` |
| block size | 4 | complete block size, including the six-byte type/length header |
| CRC-32 | 4 | big-endian CRC-32 of the decompressed bytes |
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

**Confirmed in the controlled Finale 2002–2005 baselines.** Each empty final pool marker is followed by the same
eight-byte trailer, `ff ff ff ff 01 04 01 ff`. The trailer is outside the marker's declared six-byte size. Its
meaning is open; readers should preserve/report it as trailing framing data rather than treating it as another typed
block or requiring the empty marker itself to end at EOF.

## Public Finale 2000 PDK evidence

**Public-PDK-derived, with the physical framing independently binary-verified.** On 2026-08-08 the project adopted
the public-source provenance policy in the README and consulted the Finale 2000 PDK copy included in GRAME's public
GUIDOLib repository at immutable commit
[`9f74ba9b3e287f240bbd454c2259fc3f7737c6ad`](https://github.com/grame-cncm/guidolib/tree/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin).
The included README explicitly identifies the material as the Finale 2000 PDK. No PDK file is stored in this
repository.

The PDK data API supplies three decisive factual concepts:

1. An extended data tag is a 32-bit value whose high 16 bits select a storage class and whose low 16 bits are the
   two-character Enigma tag.
2. Ordinary “other” IDs use one 16-bit comparator; detail IDs use two; entry IDs use a 32-bit entry number. An
   incident is an API selection/order dimension, not an additional field found in the fixed physical row.
3. Logical structures larger than one physical payload are stored across a declared number of successive
   incidences. Strings, arrays, and specially handled structures use separate storage classes.

These concepts explain both the two-character tags in the decoded pools and why records with the same tag/key recur
in ETF. They also warn against treating every physical row as a complete musxdom object. For example, the Finale 2000
definitions occupy two physical rows for `MS` (measure attributes), two for `Iu` (staff-list membership), two for
`PS` (page layout), two for `SS` (staff-system layout), and three for `IS` (staff attributes). The controlled ETF
evidence shows later expansion without changing the tag: `IS` grows from three rows in Finale 2002 to six in Finale
2003, while `MS` and `SS` grow from two rows through Finale 2004 to three in Finale 2005. A decoder must therefore
select a versioned logical layout after reading stable physical rows.

Public source files consulted, access date 2026-08-08:

- [`edata.h`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/edata.h): storage classes, IDs, tags, and ordinary logical structures;
- [`EEDDATA.H`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/EEDDATA.H): entry-detail tags and structures;
- [`EXTYPES.H`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/EXTYPES.H): entry/note API types and PDK version history;
- [`VERSION.H`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/VERSION.H): primitive widths and tag construction; and
- [`FINEXTND.H`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/FINEXTND.H): versioned data API behavior.

### SmartShapeCustomLine (`ls`) is absent from the public PDK

Neither `EDTCustomLineDefinition` nor a two-character tag `ls`/`sl` occurs anywhere in the consulted PDK
sources above, checked across every header, not just `edata.h`. `others::SmartShapeCustomLine` is
therefore not `public-PDK-derived`; the tag and layout below are corpus-correlated instead.

**Confirmed.** Fixed-row tag `ls` (packed logical order, i.e. `records::packTag("ls")`) is the pre-zlib
identity of `others::SmartShapeCustomLine`; zlib class `0x00de` already appears in the tag table above.
Established by exact one-for-one correlation of source comparators against Finale 27 `ssLineStyle`
elements in two controlled private fixtures: an 11-line-style Finale 2000 file (`mus-baf04c0390a2fd12`)
and a 30-line-style Finale 2003 DCL file (`mus-302c33d4dffb58dc`), both in `rpatters1-main`, plus the
tracked `tests/evidence/F2000/F2000-baseline.mus` (cmpers 1-2). Every incidence family is exactly six
16-byte rows (36 words / 72 bytes), matching the 72-byte payload already recorded for class `0x00de`.

The record is absent, not merely unread, before Finale 2000. **Confirmed** by an exhaustive
content-deduplicated census of every filesystem-origin pre-2000 fixed-row/CodaBanner file across all
three registered surveys (`rpatters1-main`, `rpatters1-installs`, `tracked-evidence`): 393 distinct
files spanning `1.0.0`, `PC 1.0+`, `1.8.7`, `2.0.1`, `2.6`, `3.0`, `3.2`, `3.5`, `3.7`, `3.8`, `97`, and
`98` (Enigma major 3 and earlier) carry zero `ls` rows between them, including 25 Finale 1.0.0 files and
5 CodaBanner files that are tracked, public fixtures. (Two further files of unrecovered version were
also checked and are likewise absent, but are excluded from this count since their era is not itself
confirmed.) The same census finds `ls` in 139 of 143 distinct
Finale 2000 (Enigma major 5) files; the four exceptions are shipped install templates ("Encore Import
Defaults", two Church templates, one General template) that carry thousands of other `others` rows but
none under this tag, so a real Finale 2000 document can still lack the class entirely. The boundary is
Enigma major 5, matching `research/VERSION_MATRIX.md`'s version table.

A further deduplicated sample (up to 25 distinct files per product) across 2001-2012 shows `ls`/class
`0x00de` in the large majority of files at every product, with a handful of absences at every product
from 2006 onward and a notably higher absence rate at 2012 (11/25 sampled). Spot-checking those Finale
2012 absences shows large, ordinary user documents with thousands of unrelated `others` rows and no
line-style record at all -- the class is populated only when a document actually uses a custom line
style, not synthesized into every source file the way Finale 27's own export synthesizes a baseline set
on upgrade, so an absence at any post-boundary product is an ordinary and expected outcome rather than a
gap in the reader.

The importer relies on tag/class presence as a structural marker rather than a version gate, per the
project's stated preference: a source before the boundary and a source at or after it that simply never
used the feature are indistinguishable in their effect (no rows, no objects), so no separate gate is
needed to tell them apart.

Physical word layout (0-indexed, pre-Finale-2012):

| word | field | notes |
|---:|---|---|
| 0 | raw `lineStyle` | `0`=Solid, `1`=Dashed, `2`=Char -- raw order differs from the modern enum |
| 1 | char: `lineChar`; solid/dashed: `lineWidth` (Efix) | union by `lineStyle` |
| 2 | dashed: `dashOn`; char: `font.fontId` | union |
| 3 | dashed: `dashOff`; char: `font.fontSize` | union |
| 4 | char: `font` Enigma style mask | bit meanings are musxdom's, through `FontInfo::setEnigmaStyles` |
| 5 | unrecovered | `-1` for Char and zero otherwise; meaning unconfirmed, not assigned |
| 6 | char: `baselineShiftEms` | |
| 7 | raw `lineCapStartType` | `0`=None, `1`=ArrowheadPreset, `2`=ArrowheadCustom, `3`=Hook -- again a raw order distinct from the modern enum |
| 8 | raw `lineCapEndType` | same raw mapping |
| 9 | `lineCapStartArrowId` or `lineCapStartHookLength` | shared slot, selected by word 7 |
| 10 | unrecovered | zero in every sample |
| 11 | `lineCapEndArrowId` or `lineCapEndHookLength` | shared slot, selected by word 8 |
| 12 | unrecovered | zero in every sample |
| 13 | flags | bit0 `makeHorz`, bit1 `lineAfterLeftStartText`, bit2 `lineBeforeRightEndText`, bit3 `lineAfterLeftContText` |
| 14-18 | `leftStartRawTextId`, `leftContRawTextId`, `rightEndRawTextId`, `centerFullRawTextId`, `centerAbbrRawTextId` | one word each, in that order |
| 19-28 | `leftStartX`, `leftStartY`, `leftContX`, `leftContY`, `rightEndX`, `rightEndY`, `centerFullX`, `centerFullY`, `centerAbbrX`, `centerAbbrY` | X and Y interleave per anchor, not grouped by axis |
| 29-33 | `lineStartX`, `lineStartY`, `lineEndX`, `lineEndY`, `lineContX` | musxdom's own declaration order |
| 34, 35 | unrecovered | zero in every sample, including one that fills every word through 33 |

**Confirmed, the anchor and line-adjustment offsets.** Words 19-33 were established by
`tests/evidence/F2000/F2000-ssline-offsets.mus`, a controlled fixture built for the purpose: one
Solid line style with all five text anchors defined and every position box set to a distinct
prime, so no two of the fifteen slots can be confused with one another. Its Finale 27 companion
names all fifteen and agrees word for word, including the one negative value.

The fixture also settles what the earlier samples could not distinguish about the vertical line
adjustment. Finale's dialog offers a single "V" control for the whole line and writes that one
value into **both** word 30 and word 32, which is what musxdom's note that Finale syncs
`lineStartY` with `lineEndY` describes. Entering 61 there produced 61 in both words while "Start
H", "End H" and "Cont H" produced three distinct values in words 29, 31 and 33.

**Confirmed, the Char font tuple.** Words 2, 3 and 4 of a Char record are the ordinary legacy font
tuple of id, size and Enigma style mask, the same triple the clef and font-options tables carry. Words
5, 10, 12, 34 and 35 correspond to no musxdom member; word 5 is the only one of them ever nonzero,
holding `-1` for a Char line and zero otherwise, and nothing establishes what it means.

**Confirmed, a stored character is a byte in its font's encoding.** `lineChar` is not a code point
before Finale 2012. It is a single byte in whatever encoding the font named by word 2 uses, exactly as
a run of legacy text is, and it is decoded by the same rule: a symbol font's byte is a glyph number and
is its own code point, and any other font's byte goes through the code page its charset fields name.
Font id 0 is the document's default music font and is a symbol font whatever its charset claims.
`F2000-ssline-offsets.mus` holds Mac Roman 199 in an Arial line and its companion reads 171, which is
`U+00AB`. From Finale 2012 the record stores the code point outright and there is nothing to decode;
the same document back-saved to that era holds 171 directly.

**Confirmed, and the rule is not particular to this class.** Every record that stores a bare
character stores it the same way and is decoded the same way: `ClefDef::clefChar`, the
stem-connection symbol, and `TextOptions`'s accidental symbol inserts.
`tests/evidence/F2002/F2002-clef-stem-font.mus` settles all three at once -- a clef given a font of
its own, a stem connection naming the same font, and the flat insert moved to it each store 199, and
the companion reads all three as 171.

Each carries its own control. The document's first stem connection and its four remaining symbol
inserts still name font 0, storing 192 and 35/110/186/220, and every one of those must read back
unchanged because a music font's byte is a glyph number -- decoding 192 through Mac Roman would name
an infinity sign. Every clef, connection and insert in every other fixture is of that second kind,
which is why no earlier document showed the difference.

Finale 2002 is the earliest available release whose clef dialog allows a font override, so the
fixture is a Finale 2002 document rather than a Finale 2000 one.

**Confirmed, Finale 2012 boundary.** The zlib class payload is the pre-2012 layout with the character
slot (word 1) widened to a 32-bit codepoint occupying words 1-2. The record stays 36 words because the
old final word is dropped. This is the same boundary already generalized in
`versions::firstUnicodeMajorVersion`/`versions::storesUnicodeCodepoints` (`legacy_mapping.h`) for the
clef and stem-connection tables, applied here as a new instance rather than a new rule.

**The shift is not uniform, and reading it as uniform is wrong.** The character belongs to the Char
parameter block, so only that block's own later fields move with it: for a Char record, old words 2 to 6
become new words 3 to 7. A Solid or Dashed record has no character and keeps its width and dash lengths
in words 1 to 3 exactly where every earlier layout put them. The block occupies one more word in every
record regardless, so the common part from word 7 onward moves for all three line styles (old word *n*
&rarr; new word *n*+1 for *n* &ge; 7).

Established by `tests/evidence/F2012/F2012-ssline-offsets.mus`, the back-save of the fixed-row fixture
below, which states all three parameter layouts in one document. A rule that shifted every word alike
reads its dashed line as a 7 EVPU dash with no gap; the source and its companion both say 3 and 7. The
earlier `F2012-baseline.mus` could not distinguish the two rules: its only non-default line style is
Solid with a cap, and every word that record uses lies in the common part.

Both controlled correlation fixtures also show the modern Finale 27 export carrying one more
`ssLineStyle` object than the legacy source stores physically (comparator 11 of 11 in the Finale 2000
file, comparator 3 of 3 in `F2000-baseline`): a further built-in default Finale's upgrade synthesizes
rather than something the reader failed to recover. The importer does not fabricate it.

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

### Embedded graphics

**Confirmed across controlled Finale 2006 evidence, `rpatters1-main`, and
`rpatters1-installs`.** From Finale 2006 onward a stored `0x0013` block holds embedded graphics.
The three controlled Finale 2006 documents carry one EPS and four TIFF files; twenty-seven distinct zlib-era
corpus documents carry 66 EPS or PNG files. Each item is exactly:

`16-bit type, 32-bit byte length, raw graphic bytes, 32-bit footer version 1, 8-bit footer value`

Every numeric field follows the container byte order. Walking `6 + length + 5` bytes per item
consumes every observed block exactly, including blocks holding as many as nine graphics. The
final footer byte varies and remains **open**; it is not needed for delimiting or identifying the
raw file. Binary EPSF, `%!PS-Adobe` EPS, TIFF, and PNG signatures select the extension without
depending on the nested type.

The nested items carry no comparator. Their one-based encounter order is the graphic comparator:
for every adjacent-exact Finale 27 companion, item sizes in encounter order exactly match
`graphics/1.<extension>`, `graphics/2.<extension>`, and so on, even where ZIP member enumeration
is shuffled. The reader supplies this map to musxdom before document construction finishes.

No stored embedded payload was found in the Coda-banner or uncompressed epochs, nor in the
uncontrolled DCL corpus. Controlled Finale 2006 linked and embedded saves prove that embedding
begins inside the DCL epoch: the linked file's `0x0013` block is empty, while otherwise comparable
embedded files carry one or two nested EPS/TIFF items. A controlled measure-assigned EPS followed
by a page-assigned TIFF becomes comparators 1 and 2, confirming that encounter order is independent
of assignment type. Their ETF exports retain the score structures
but cannot carry the binary attachments. Finale 2005 and earlier remain uncovered for embedded
payloads because the application did not offer the feature.

The controlled Finale 2012 `F2012-graphics-types` fixture confirms embedded GIF, JPEG, TIFF, and
PDF payloads in the zlib epoch. Its six stored items correspond one-for-one with six assignment
occurrences: repeated use of the same GIF and JPEG produces distinct, byte-identical items and
comparators, while the singly used TIFF and PDF each produce one. Measure assignments resolve
comparators 1 and 6, the page assignment resolves comparator 2, and the three ShapeDef graphic
assignments resolve comparators 3, 4, and 5. Finale 27 preserves the same order and duplication.

### Page graphic assignments

**Confirmed across `rpatters1-main` and `rpatters1-installs` for the uncompressed, DCL, and zlib
epochs; structurally supported but corpus-unverified for Coda-banner.** Page graphics use tag `pg`
through Finale 2006 and class `0x00bc` in the zlib era. All 56 observed assignments in 26 documents
are exact 18-word tuples: three six-word fixed rows per assignment, or successive 36-byte tuples
inside a zlib class payload. The assignment comparator remains the DOM cmper and tuple order is its
zero-based incidence.

Words 0-17 are `version`, `left`, `bottom`, `width`, `height`, `fDescId`, `hidden`, `displayType`,
packed left/all-page positioning, `startPage`, `endPage`, `savedRecord`, `origWidth`, `origHeight`,
`rightPgLeft`, `rightPgBottom`, packed right-page positioning, and `graphicCmper`. The positioning
word uses one-hot bits: horizontal left/right/center are `0x01/0x02/0x04`, vertical top/bottom/center
are `0x08/0x10/0x20`, margins/page-edge are `0x40/0x80`, and preserve-aspect is `0x100`.

### Earliest controlled graphic placement

**Confirmed in Finale 3.7.2; absent from the Finale 2.6.3-and-earlier UI.** The controlled
`F372-measure-graphic` fixture places a linked EPS on staff 1 at measure 3. Its MUS and ETF both
carry four `mg(1,3)` rows forming the same 20-word assignment used through Finale 2006, including
file-description cmper 1 and `graphicCmper` zero. No stored graphic block exists, as expected for
a linked file. The user who produced the fixture observed the Graphics Tool in Finale 3.7.2's Tool
menu and observed it absent in Finale 2.6.3 and earlier.

This establishes external placement by 3.7.2 and brackets its UI introduction after 2.6.3. It
does not determine which intervening 3.x release first supplied the tool. The controlled
`F372-page-graphic` fixture independently establishes `pg` at the same boundary: its three rows
contain the standard 18-word page assignment and refer to the linked `Photo_tiff.tiff`. Finale 27
preserves both the assignment and path in `score.dat`, but does not put the TIFF in the MUSX ZIP.

The corresponding `0x001d` block is non-empty in 208 zlib files. The reader does not reach it,
because the walk stops at the first terminal marker, so those bytes are counted as trailing.
Its payload does not begin with an image signature and its content is **open**.

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

## Finale 2007+ generic record frame

**Confirmed for successfully framed `0x001a` and `0x001b` members.** The frame is variable-length and ends with two zero words. More than 1.59 million records across Finale 2007–2012 were accepted only when the proposed frame consumed the complete decompressed member and every trailer was zero.

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

The hypothesized two unaccounted words do exist in this era, but as a **four-byte all-zero record trailer/reserved terminator**, not as the last two words of a fixed 16-word structure. Their semantic purpose remains open (reserved fields versus terminator/padding), but their position and zero value are strongly established.

This later variable frame does not retain the earlier fixed 16-byte physical rows. Many payload sizes are multiples
of the old 12-byte other capacity, suggesting that later versions coalesced successive incidences into one
variable-length payload, but that historical relationship still needs field-level verification.

### Finale 2002 controlled pair

**Strong for this sample; broader version coverage pending.** The F2002 baseline and one-pitch-change pair were saved
by Finale `2002a.r1` under Mac OS 9.0.4 in SheepShaver. Beginning at `0x200`, both files expose a simple outer stream
of big-endian variable-length records:

| Offset | Type | Total record length | Baseline / changed |
|---:|---:|---:|---:|
| `0x0200` | `0x000f` | 1,976 | unchanged |
| `0x09b8` | `0x0010` | 158 | unchanged |
| `0x0a56` | `0x0011` | 53 / 57 | changed |
| `0x0a8b` / `0x0a8f` | `0x0012` | 6 | empty/end-pool marker moved with preceding record |

The six-byte outer header is two bytes of type followed by a four-byte **total record length including the header**.
Records are not constrained to 16 words or 32-byte alignment; the terminal record begins at an odd offset in the
changed file. Changing the pitch from C to D changes 45 observed bytes overall, including the length field and a
four-byte extension, while the complete `0x000f` and `0x0010` records remain identical. The ETF changes only the
`eE` entry payloads, supporting the interpretation that `0x0011` is an entry-related pool and that the edit is
localized at the outer-record level.

Subsequent DCL testing solved the payload codec and confirmed the same checked container across all recognized
2001–2006 samples. The fixed rows inside `0x000f`–`0x0011` are now established, while tag-specific logical field
layouts and `0x0012` text organization remain incomplete. F2003, F2004, and F2005 independently confirm the same
outer framing and broad type sequence.

## Record fields

In the big-endian `0x001a` ordinary variant, the first five words behave as:

1. numeric record type;
2. primary key/`cmper` or `0xfffe` option sentinel;
3. secondary key/`inci`/part dimension candidate;
4. additional key/flags/part dimension candidate;
5. payload byte count.

Fields 2–4 are not fully named. For ordinary “other” records, field 2 tracks the `cmper` sequence and fields 3–4 are often zero. The `0xfffe` records at the front of `0x001a` are singletons with codes beginning at `0x000f` and are strongly interpreted as options. This shows that options are not wholly free-form in 2007+, although their code-to-option and field mappings remain open.

For `0x001b` details, the corresponding fields are numeric type, `cmper1`, `cmper2`, incidence,
and payload byte count. Class `0x041d` establishes `cmper1` as staff and `cmper2` as measure for
`MeasureGraphicAssign`; part and sharing scope remain open.

See [RECORD_CATALOG.md](corpora/rpatters1-main/RECORD_CATALOG.md) for all observed identifiers. Examples of exact corpus-wide matches include:

| Code | EnigmaXml structure | Evidence |
|---:|---|---|
| `0x0086` | `durAllot` | exact in 375/375 framed files |
| `0x0092` | `frameSpec` | Pearson 0.999; exact in 273/375 (conversion expands some) |
| `0x00a4` | `measNumbRegion` | exact whenever present; 368 files |
| `0x00d9` | `smartShape` | Pearson 0.998; conversion differences observed |
| `0x011a` | `partDef` | exact in 375/375 framed files |
| `0x03ef` | `acciAlter` | strong correlation with conversion differences |
| `0x03f3` | `baselinesExprAboveStaff` | exact in 324/324 detail-framed files |
| `0x0414` | `gfhold` | Pearson 0.999; exact in 291/324 |

### Word order in 32-bit fields — open hypothesis, not tested

Most 32-bit option fields are two 16-bit words with the **high word first**, each word in the
container's byte order (the framework's `MACFOURBYTE`); a minority are plain little-endian longs
(`WINFOURBYTE`). The text-options insert element holds one of each: its two trackings are high word
first in both byte orders, while its Finale 2012 `symChar` is a plain little-endian long. See
[Text options](#text-options).

**Hypothesis.** The convention follows *when the field entered the format*, not what it means. The
Enigma era — uncompressed and DCL, when big-endian Macintosh was a first-class target — fixed word
order deliberately so one rule served both byte orders. By the zlib era big-endian was fading, and
by Finale 2012 nobody was thinking about it, so fields added late are simply written in native
little-endian. If this holds, word order is predictable from a field's introduction version and does
not need to be determined per field.

**How to test it, incidentally.** `data/legacy_option_mappings.csv` carries 36 four-byte rows, 31
`MACFOURBYTE` and 5 `WINFOURBYTE`. Each future import that lands a 32-bit field adds a dated data
point: record the field's earliest observed version alongside its observed convention and see
whether the two `WINFOURBYTE` late arrivals stay late and the `MACFOURBYTE` set stays early. Do not
mount a dedicated study for this. One caution for whoever picks it up: the current `WINFOURBYTE`
rows are not uniformly late — `smpteStartTime` is Finale 2014, but the selector-77 page dimensions
are not a late field — so the hypothesis is not confirmed by a glance at the inventory and may need
a narrower statement than "late means little-endian".

## Entry pool

**Finale 2001–2006 framing solved; fields partly mapped.** Every nonempty decoded `0x0011` pool in the directly
resolved corpus is a sequence of 38-byte rows. The controlled documents contain three rows matching three ETF `eE`
records; the first four bytes are the big-endian entry number, and the controlled C-to-D edit changes the note pitch
inside the corresponding row while preserving its size. The public PDK confirms that entry numbers and entry/note
flags are 32-bit and several duration/count fields are 16-bit, but its 146-byte plug-in API structure includes
expanded note capacity and computed fields and must not be mistaken for the 38-byte disk row. Exact raw field and
flag mapping is now a bounded correlation task.

For Finale 2007+, block `0x0016` decompresses cleanly and CRC-validates but does not use either the earlier 38-byte
row or the later generic zero-trailed frame. Its decoded size tracks document complexity; that era remains open.

## Options

**Substantially mapped for the pre-26.2 compatibility representation, but mostly source-derived.** Options are at the
beginning of the 2007+ `0x001a` block, identified by primary key `0xfffe`; many are fixed 12-byte records, while some
have large variable payloads. ETF represents the corresponding older globals as `^NN(65534)` records.

Authorized read-only inspection of privately supplied PDK Framework histories yielded a 437-row union across
24 logical preference groups: 435 current mappings plus two original-branch locations needed for older Finale
behavior. The table identifies tags, comparators, incidents, word slots, widths, conversion rules,
semantic fields, and some version gates. Available ETFs independently contain the selectors used by 386 rows, but
field meanings and offsets remain `private-framework-derived` until controlled binary comparisons verify them. See
[`LEGACY_OPTION_MAPPINGS.md`](LEGACY_OPTION_MAPPINGS.md) and
[`data/legacy_option_mappings.csv`](data/legacy_option_mappings.csv).

Five additional structures use direct multi-incidence blocks rather than the field map: slur contours at
`^52(65534)`, tie placement at `^85(65534)`, tie contours at `^86(65534)`, grids/guides at `^88(65534)`, and stem
connections at `^40(65534)`. All selectors are ETF-observed; see
[`data/legacy_direct_option_blocks.csv`](data/legacy_direct_option_blocks.csv).

### Later pre-zlib default-font array

**Private-framework-derived; strongly ETF-supported for Finale 2002–2005.** In those versions the `FontOptions`
array is selector `24`, comparator `65534`. It is a packed array of three-word font tuples rather than one font per
physical incidence:

| Physical tuple index `n` | Incidence | Font ID | Size | Effects |
|---:|---:|---:|---:|---:|
| even | `n / 2` | slot 0 | slot 1 | slot 2 |
| odd | `n / 2` | slot 3 | slot 4 | slot 5 |

The framework's zero-based default-font preference numbers are versioned rather than one timeless musxdom enum
order. From at least Finale 98 **through Finale 2011**, index 13 is a legacy drawing-time tablature holding slot and
index 28 is the actual default tablature font; earlier versions remain open. **Beginning with Finale 2012**, tablature
uses index 13 and index 28 is percussion. Indices
40–42 are also present beginning in Finale 2003. The controlled Finale 2002 ETFs contain 20 incidences, enough for
indices 0–39. The Finale 2003–2005 ETFs contain 22, enough for indices 0–43; index 43 is the zero-filled second tuple
required to complete the final fixed 16-byte row. Each baseline/changed pair has
an identical selector-24 array, as expected because the controlled edit changed a note rather than a document font.

#### The 13/28 boundary is Finale 2012, not Finale 2003

**Confirmed** by measurement, and it **contradicts the private-framework history** these notes previously followed,
which places the transition at Finale 2003. Where the two disagree, the measurement governs and is what the importer
implements; see the comment on `semanticType` in `src/import/options/font_options.cpp`.

The earlier claim that the boundary "independently fits the exact-pair corpus" was true but not discriminating. It
was tested against Finale 2002 sources, and Finale 2002 precedes *both* candidate boundaries, so those documents are
consistent with either. Nothing in the corpus distinguished them until Finale 2011 specimens existed, because
Finale 2011 is the only version whose behavior differs between the two hypotheses.

Across 1,211 documents whose Finale 27 companion assigns tablature and percussion different values — the only
documents that can discriminate — the arrangement is:

| Source version | Documents | Arrangement |
|---|---|---|
| Finale 2003–2010 (majors 8–15) | 405 | index 28 is tablature; 13 is a holding slot |
| **Finale 2011 (major 16)** | **597** | index 28 is tablature; 13 is a holding slot |
| Finale 2012 (major 17) | 209 | index 13 is tablature; index 28 is percussion |

No document contradicts this on either side of the boundary, on either platform. Coding the boundary at Finale 2003
cost every Finale 2003–2011 document its tablature font and gave it a percussion font it never stored: 2,516 of the
2,629 FontOptions disagreements then present in the corpus were this single rule.

The Finale 2011 specimens that settled it came from the Finale 2011 install DVD; no Finale 2011 document existed in
any survey before that. Comparison must normalize font names with musxdom's `normalizeFontName`: `EngraverTextT` and
`Engraver Text T` are one face, and comparing raw spellings produced 324 false disagreements that made Finale 2011
look internally inconsistent.

Across 42 distinct
Finale 2002 sources, index 28 carries the value upgraded into modern tablature, while index 13 is not an independent
modern default. Across all 329 distinct Finale 2003–2006 exact-pair sources, physical index 43 is `(0, 0, 0)`. This
is structural row fill, not a terminator or a version-encoded collection limit. Physical capture therefore walks
every complete tuple; semantic insertion ignores only a zero-filled second tuple at the end of a fixed row.

The selector and packing are established well enough to locate every stored font ID, size, and effects word in this
verified range. Their semantic values remain **strong**, not `confirmed`, until a controlled file changes one
default font at a time. This layout must not be selected merely because a file predates zlib; Finale 1.0.0 proves
that selector meanings and layouts changed inside the broad pre-zlib era.

The same array is present, with the same packing, well before Finale 2002. The Finale 97 and Finale 2000 fixtures
each carry 20 incidences of selector `24`, and their tuples agree with their exact Finale 27 companions type for
type: `(0, 28)`, `(0, 28)`, `(0, 24)`, `(0, 26)`, `(2, 12)` for music, key, clef, time, and chord. Finale 3.7.2
shows the same array with that document's own sizes.

The reader recovers these. Selector 24 is the default-font array in every fixed-row epoch except the Coda banner, so
the layout is selected by epoch rather than by a version range, and 6,100 recovered sizes across 173 uncompressed
files agree with their exact companions. The physical-to-semantic quirks of the era are the same ones the DCL era
has before Finale 2003: physical slot 13 is not an independent value and physical slot 28 carries tablature.

### Finale 1.0.0 fonts

**Confirmed across the installation survey.** All 22 distinct Finale 1.0.0 specimens contain exactly five `FN`
families, with font-definition cmpers 0–4. Every specimen has the same five-name table. Consequently, importing any
modern fallback `FontInfo` whose font ID is outside 0–4 can silently resolve to an unrelated definition if comparator
spaces differ, or remain dangling; neither result is a safe fallback.

Every surveyed specimen also contains exactly one `24(65534)` row, but its two apparent triples are `(13, 69, 52)`
and `(48, 65, 60)`. Both putative font IDs exceed the definition range, and the values do not resemble font sizes or
effect masks. Selector 24 therefore is **not** the later default-font array in Finale 1.0.0.

**Confirmed by newly authored controlled Finale 1.0.0 fixtures.** Thirteen one-variable UI saves locate every font
preference exposed by Finale 1.0.0. Twelve map directly to modern FontOptions; the historical `Name` preference is
treated as the predecessor of `StaffNames` under the additive-only early-version hypothesis:

| Semantic type | Font ID | Size | Effects |
|---|---|---|---|
| Music | `02(65534)` word 0 | word 1 | word 2 |
| Key | `03(65534)` word 3 | word 4 | word 5 |
| Clef | `04(65534)` word 0 | `39(65534)` word 4 | `39(65534)` word 5 |
| Time | `03(65534)` word 0 | word 1 | word 2 |
| Chord | `02(65534)` word 3 | word 4 | word 5 |
| ChordAcci | `37(65534)` word 0 | word 1 | word 2 |
| Ending | `05(65534)` word 0 | word 1 | word 2 |
| Tuplet | `36(65534)` word 0 | word 1 | word 2 |
| TextBlock | `26(65534)` word 0 | word 1 | word 2 |
| LyricVerse | `26(65534)` word 3 | word 4 | word 5 |
| LyricChorus | `27(65534)` word 0 | word 1 | word 2 |
| LyricSection | `27(65534)` word 3 | word 4 | word 5 |
| StaffNames (historical `Name`) | `04(65534)` word 3 | word 4 | word 5 |

Clef is physically split across two records. The Tuplet save also changes the ChordAcci tuple, and Finale 27
preserves both changes. Raw effects `0x08` and `0x10` occur in controlled saves but are not among musxdom's six
represented Enigma style bits; reporting retains the raw mask while `setEnigmaStyles` expands the supported bits.
Finale 27 drops the controlled historical `Name` change, so its continuation as `StaffNames` is **strong**, not
confirmed. It also changes some unedited categories, demonstrating upgrade synthesis and shared-preference
behavior; those changes are not evidence for additional source locations.

#### The single `Name` preference reaches all four modern name types

The Coda-banner era exposes one `Name` font preference. Finale 3.0 replaced it with four — `StaffNames`,
`AbbrvStaffNames`, `GroupNames`, `AbbrvGroupNames` — which that era stores as separate tuples at physical ordinals
31, 32, 33 and 39. The importer therefore propagates the one recovered Coda tuple to all four types, and this
fan-out is gated on the Coda-banner epoch alone so that it can never overwrite the three independently recovered
values of any later epoch.

Recovering `StaffNames` alone would emit a document whose staff names use one face and size while its group and
abbreviated names use the Finale 27 default of Times New Roman 14 — a split neither the source nor the Finale 27
baseline has, since that baseline sets all four identically. The Finale 3.7 `F372-baseline` fixture, which never
touched these preferences, likewise carries all four as Times 12.

The three propagated types report as `ValueOrigin::LegacyBehavior` rather than `LegacyMus`: the bytes are read from
the source, but the assignment restores an era behavior rather than an option the source stored.

**This is a deliberate, revisitable divergence from the Finale 27 companions.** Across the 57 Coda-era documents
with companions, every companion disagrees with the recovered value: 39 report `Times 16`/`Times 14`, 17 report
`Monaco 16`/`Monaco 14`, and one reports `Pmusic 12` throughout, while the source tuple reads `Times 14` in all 57.
Those companion values track the personal default file the upgrade was performed under rather than the source
document, so they do not settle what Finale 27 does with a stored `Name`. Settling it needs a companion produced
under a stock default file.

The same `02`, `03`, `04`, `05`, `26`, `27`, `36`, `37`, and `39` global families persist through the Finale 1.8.7,
2.0.1, and 2.6 corpus. Under the working hypothesis that this interval only adds font preferences, the importer
uses the Finale 1.0.0 mappings through 2.6 and leaves any later additions at safely remapped Finale 27 defaults.
This extension is **strong**: 188 distinct readable early sources preserve the relevant physical records, and 53
adjacent-exact Finale 2.6 companions independently support Chord, Ending, and Tuplet. These newly authored Finale
1.0.0 documents contain 23 font definitions, unlike the five-definition installation cohort, so they establish
locations without changing that earlier census.

This rules out keeping the pinned `FontOptions` as a completeness skeleton, because its numeric IDs belong to the
baseline's font-definition table. The reader should filter that whole object out and create a fresh, fully populated
`FontOptions`. Era-verified source tuples are copied directly. Each missing type is synthesized from a separate,
fully populated platform-matched baseline document: baseline font cmper 0 remains 0; every nonzero baseline font is
matched to a target definition by normalized name, or its complete `FontDefinition` is cloned at the next target
cmper when no match exists. The normalization removes whitespace and folds case, matching musxdom's existing font
normalization so PostScript and family spellings compare equal. Normalization is used only to find a match: when a
definition must be cloned, its name retains the selected platform reference document's exact spelling. This
new definition receives the next sequential comparator after the target's highest existing font comparator; the
reference comparator is never copied into the target id space. This preserves all 45 keys without allowing a
baseline ID to resolve accidentally to an unrelated legacy font.

This completion is deliberately not an attempt to reproduce every choice Finale makes while upgrading a document.
For example, Finale 27 derives a Percussion preference from Music in controlled pre-2003 upgrades, but no separate
source location has been found. The importer therefore takes Percussion from the selected reference document. More
generally, a legacy MUS import will remain less complete than a native MUSX document; coverage improves only as
additional source fields are identified with sufficient confidence.

### Zlib default-font array

**Strong.** The same array is the singleton class record `0x0026(65534)`, incidence 0, in the `0x001a` block. Both
controlled zlib fixtures carry a 276-byte payload: 46 consecutive six-byte tuples in file byte order. Tuples 0–44
map to musxdom's 45 `FontType` values and tuple 45 is the zero-filled second tuple of the final 12-byte pair:

| Logical `FontType` index `n` | Font ID | Size | Effects |
|---:|---:|---:|---:|
| 0–44 | byte `6n` | byte `6n + 2` | byte `6n + 4` |

The 23 tuple pairs are consistent with the fixed-row representation's two tuples per Enigma record and with Finale
continuing to expose that record model to plug-ins, even though the zlib class record itself is length-governed.
Each member is a 16-bit word. Font ID is a font-definition cmper, size is the point size, and effects is an Enigma
style mask. The effects word must be expanded through musxdom `FontInfo::setEnigmaStyles`, yielding the `bold`,
`italic`, `underline`, `strikeout`, `absolute`, and `hidden` booleans rather than being assigned as one scalar field.

The identification is supported by continuity, not merely payload size. The first 43 tuples in the big-endian
Finale 2007 fixture exactly equal the corresponding Finale 2005 ETF tuples. Finale 2005 then has an unused zero
half-incidence; Finale 2007 fills indices 43 and 44 and moves the zero tuple to index 45. The little-endian Finale
2012 fixture has the same tuple organization with document-specific font IDs, sizes, and effects. The public record
catalog observes class `0x0026` with length 276 in every represented zlib product year from 2006 through 2012.

This also exposes a general options bridge. For every numeric global selector present in the Finale 2005 ETF, the
controlled Finale 2007 options prefix contains class ID `selector + 0x000e`; it adds selector 47 and omits none.
The Finale 2012 fixture follows the same transform, with the expected version-specific additions/removals. For
example, selector 24 becomes class `0x0026`, selector 94 becomes `0x006c`, and selector 98 becomes `0x0070`.
Therefore zlib did not replace the option schema wholesale: it coalesced each old selector family's incidences into
one length-governed class payload, while retaining comparator `65534` and the payload's logical word sequence.

The physical location and organization are strong enough to implement. The semantic claim remains **strong**, not
`confirmed`, until a controlled zlib file changes one default font at a time and a trusted conversion verifies the
result.

The mapping is incomplete: it covers 61 numeric globals while current ETFs contain 35 additional numeric globals not
mapped by the framework. Historical and Finale 26.2 replacement locations also prove that mappings can be
version-dependent. Earlier statements that option code names and layouts were wholly unknown are superseded by this
partial map.

### Clef definitions

**Confirmed for the collection and its fields; three version boundaries are established and one is inferred.**
Clef definitions are an ordinary numeric global, not a record type of their own. There is no `cf` tag or
comparator anywhere in the corpus: a search of the others, details, and class pools of specimens spanning
Finale 1.8.7 through 2012 found none, and the identity is the numeric selector in every era.

The collection changed size twice and the tuple once. Counts below are distinct corpus files:

| Era | Identity | Layout | Definitions | Files |
|---|---|---|---|---:|
| Finale 1.8.7–2.6 | selectors `28`–`35`, comparator `65534` | one 6-word row each | 8 | 63 |
| Finale 3.0–2000 | the same eight selectors | one 6-word row each | 8 | 208 |
| Finale 2001–2002 | selector `95`, 24 incidences | 9-word tuples, streamed across rows | 16 | 67 |
| Finale 2003–2006 | selector `95`, 27 incidences | 9-word tuples | 18 | 403 |
| Finale 2007–2010 | class `0x006d`, 324 bytes | 9-word tuples | 18 | 292 |
| Finale 2012 | class `0x006d`, 360 bytes | 10-word tuples | 18 | 235 |

Selector `36` is the tuplet font, so eight is a ceiling the record vocabulary itself imposes on the early eras
rather than a guess. The class id follows the established `numericGlobalClass` rule, `95 + 0x0e`. Unlike the
default-font array there is no structural zero fill: 16 and 18 nine-word tuples occupy exactly 24 and 27 rows.

The pre-2001 six-word record is a different layout, not a short tuple:

| Word | Field |
|---:|---|
| 0 | `middleCPos` (`adjust`) |
| 1 | **open**: a per-clef value the Coda era populates and Finale 3.0 stops writing |
| 2 | `clefChar`, one byte |
| 3 | `staffPosition` (`clefYDisp`) |
| 4 | baseline adjustment, in harmonic levels |
| 5 | **open** |

Word 1 holds `6, 0, -2, -6, 6, -1, -13, -4` across selectors 28 through 35 in the Coda era and zero in almost every
Finale 3.0 and later file, six of which retain inherited values. The controlled baseline edits leave it untouched,
so it is not the baseline adjustment and nothing is mapped from it.

The tuple, with the Finale 2012 slot in parentheses where it differs:

| Word | Field | Notes |
|---:|---|---|
| 0 | `middleCPos` (`adjust`) | |
| 1 (1–2) | `clefChar` | one word until Finale 2012, then a long |
| 2 (3) | `staffPosition` (`clefYDisp`) | |
| 3 (4) | baseline adjustment, in Efix | signed 16-bit; see below |
| 4 (5) | `shapeId` | non-zero only at indices 16 and 17 |
| 5–7 (6–8) | `fontId`, `fontSize`, effects | present only when the own-font bit is set |
| 8 (9) | flags | bit 0 `isShape`, bit 1 `useOwnFont`, bit 2 `scaleToStaffHeight` |

Two decoding rules are needed. Before Finale 2012 the clef character is a single byte of a symbol font stored in a
word, and a source may store it either zero-extended or sign-extended: character 139 appears as `0x008b` in some
files and `0xff8b` in others, so it must be narrowed to its low byte. From Finale 2012 it is a long, which is what
the tuple's two extra bytes are; MakeMusic's release notes for that version give Unicode text support as a headline
feature, which is consistent with the widening and places the boundary at 2012 rather than earlier. No Finale 2011
specimen exists in any surveyed corpus, so the reader treats 2011 as narrow and that half of the boundary is
**open**. No big-endian Finale 2012 specimen exists in any surveyed corpus either, so the long's word order is
verified for little-endian files only, and that is unlikely to change.

Finale 2012's published requirements read `OS X 10.7, 10.6, or 10.5. Mac Power PC or Mac Intel`, which is
internally inconsistent: 10.6 Snow Leopard was the first release to drop PowerPC hardware and 10.7 Lion removed
Rosetta as well, so a PowerPC Mac can run neither. The line matches the Finale 2009 requirements verbatim, where it
was coherent, and reads as boilerplate carried forward.

Writing a big-endian Finale 2012 document therefore needs the whole of a narrow intersection: PowerPC hardware,
which caps at 10.5 Leopard, meeting Finale 2012's 10.5 floor exactly, with a release from late 2011 running on a
machine Apple had stopped selling five years earlier. Whether MakeMusic still shipped a PowerPC slice at all is
unverified, given that the requirement line looks carried forward. Such files are unlikely to exist in any number.

That is a reason not to expect a specimen, not a reason to depend on there being none. The reader decodes the
big-endian case as the symmetric counterpart of the verified little-endian one and warns when it meets one, so an
unverified path announces itself instead of passing silently.

The flag bits are confirmed both physically and semantically: across 1,268 files, bit 1 occurs 9 times and every one
of those tuples carries a non-zero font triple whose exact Finale 27 companion shows `<useOwnFont/>` with the same
`fontID` and `fontSize`; the 15,931 tuples without it never carry one. This closes the previously `not_identified`
row for `ClefOptions.clefDefs[*].font.fontId` in
[`data/legacy_option_font_id_locations.csv`](data/legacy_option_font_id_locations.csv).

Word 3 is the difference between the clef's musical baseline, such as the G line of a treble clef, and its
typographic baseline: a font whose clefs already sit on the musical baseline leaves it zero. It is zero in all 1,268
corpus specimens, because no unedited document sets it, so three controlled fixtures carry the whole weight here.

**Finale 2001 onward stores Efix. Confirmed.** `F2005-clef-baseline.mus` sets one inch of baseline on the treble
clef; the stored word is `18432`, which is exactly one inch — 288 Evpu at 64 Efix each — and the exact Finale 27
companion carries `<baseAdjust>18432</baseAdjust>` through unchanged. The same fixture asks for minus two inches on
the bass clef, which would be `-36864`, and the file stores `-32768`. **The field is a signed 16-bit word**, so its
usable range is about ±512 Evpu, and Finale saturates rather than wrapping. Recovering `-32768` is the file being
read correctly. Finale's own dialog reads that value back as `-1.7778` inches, which is `-32768 / 18432`; the UI and
the storage agree, and the discrepancy is entirely the clamp.

**From Finale 3.0 through 2000 it is a small signed count of harmonic levels, in word 4 of the clef's own selector
rather than word 3 of a tuple. Confirmed.** The Coda era stores a value in the same word, but the reader does not
transfer it: there the number adjusts the baseline of mid-measure clefs only, which is not what musxdom's
`baselineAdjust` means, and Finale 27 discards it. The gate is the epoch. `F100-clef-baseline.mus` and `F263-clef-baseline.mus` each change two clefs'
baseline adjustments and move word 4 and nothing else: the Finale 2.6.3 pair differs from its baseline in exactly
three bytes across the whole file. Each era's own ETF shows the same words. The Coda era ships these populated —
`-2, -4, -5, -6, -4, 0, 0, 0` for selectors 28 through 35 — while Finale 3.0 onward ships zeros and keeps the field,
which is why 165 Coda and 143 uncompressed source clefs carry a non-zero value.

**The conversion is one harmonic level to 768 Efix — half a space, since a harmonic level is a staff position.
Confirmed in two independent eras.** `F372-clef-baseline` stores `1`, `-2` and `-5` and its companion carries
`768`, `-1536` and `-3840`; `Fin97-clef-baseline` stores `1` and `-2` and its companion carries `768` and `-1536`.

**A stored count only applies when the document switches the feature on.** Before Finale 97 that switch is bit 0 of
word 5 of the *first* clef's selector, and it governs the whole document rather than one clef: the Finale 3.7.2 pair
toggles it `0 -> 1` and the companion then converts all eight clefs, including ones that save never touched, while
the same document with the switch off loses baselines it plainly stores. Finale 97, internally 3.8, dropped the
checkbox and adjusts unconditionally.

Testing word 5 for non-zero instead of testing bit 0 looks right and is not: Finale 97 files carry 24, 30 and 36 in
that word for unrelated reasons, and bit 0 is clear in all of them. The always-on window is also bounded at both
ends, at versions 3.8 through 5.x, rather than left open at "3.8 or later". The early path only ever sees pre-2001
versions, so the upper bound changes nothing today; it is there so that a file whose version is recovered as
something wild falls back to the bit test, which reads the document, rather than to an unconditional yes.

The corpus never sets the switch: every one of the 271 pre-2001 files leaves bit 0 clear, and every file from 3.8
onward leaves all eight counts at zero. Only the controlled fixtures discriminate between the possible rules, which
is exactly why they were needed. With the switch honoured, `baseAdjust` agrees with all 1,120 adjacent-exact
companions.

Indices 8–17 do not exist before Finale 2001 and 16–17 do not exist before Finale 2003. Finale's own upgrade
supplies the missing ones from the version doing the opening, and the shape comparators it assigns to indices 16 and
17 differ per document — 1/2, 2/3, 3/4, and 25/26 across four controlled companions — which is a direct
demonstration that a shape comparator must never be carried between documents as an identity.

#### Corpus verification

Every one of the 1,120 adjacent-exact source/Finale 27 pairs was imported and compared with its companion,
`baseAdjust` included. Of the source-supplied definitions, all fields agree except four, and those four are
upgrade-time font substitution rather than decoding error.

**The discriminator is the document's music font.** Grouping the 57 Coda-era pairs by the music font and the clef
character stored at index 4:

| Music font | Stored | Finale 27 wrote | Files |
|---|---:|---:|---:|
| Pmusic | 214 | 214 | 34 |
| Petrucci | 32 | 32 | 16 |
| Petrucci | 214 | 214 | 3 |
| **Sonata** | **214** | **32** | **3** |
| Sonata | 100 | 100 | 1 |

Only Sonata documents are altered, and 214 is kept in every other font including Petrucci. Finale 27 writes 32, a
space, which musxdom reads as a blank clef.

**Why it substitutes is unknown.** Character 214 is `unpitchedPercussionClef2` in Sonata as well as in Petrucci and
Pmusic, so this is not a codepoint that means something different in the substituted font, and an encoding
difference does not explain it. The behaviour is recorded as observed and unexplained. The fourth difference is in
one of the same three files, whose index 7 also moved from `adjust 0, clefYDisp -4` to `adjust -5, clefYDisp -2`
while keeping its character; that one is a single unexplained instance of position drift.

Two observations bound the claim. The one Sonata document with a hand-edited clef table — indices 4 through 7 holding
100, 68, 247 and 175 rather than the stock values — is carried through completely unchanged, so the substitution
applies to the stock Coda table rather than to Sonata documents generally. And index 7 changed in only one of the
three files that share an identical table, so whatever selects that adjustment is not the table alone and is
**open**.

This is the same font that needs a baseline adjustment where Petrucci does not, so both known Sonata-specific
behaviours involve the same font, but no common cause has been established. **The reader keeps the stored character
in every case.** Reproducing Finale's substitution is not attempted and is not a goal: the file says 214 and the
importer says 214.

The scalar options around the collection do **not** share locations across eras, which is the trap in this class:

| Field | Location | Coda | Finale 3.0–2006 | 2007+ |
|---|---|:--:|:--:|:--:|
| `defaultClef` | selector `01` word 0 | yes | yes | yes |
| `endMeasClefPercent` | selector `13` word 2 | yes | yes | yes |
| `endMeasClefPosAdd` | selector `13` word 3 | yes | yes | yes |
| `clefFront` | selector `19` word 0 | yes | yes | yes |
| `clefBack` | selector `19` word 1 | yes | yes | yes |
| `clefKey` | selector `38` word 5 | **no** | yes | yes |
| `clefTime` | selector `39` word 4 | **no** | yes | yes |
| `showClefFirstSystemOnly` | selector `27` word 1 bit 0 | **no** | yes | yes |
| `cautionaryClefChanges` | selector `44` word 3 bit 2 | **no** | yes | yes |

In the Coda era selector `27` word 1 and selector `39` word 4 are font sizes — the lyric-chorus and clef font tuples
the FontOptions mapping already reads — and selector `38` word 5 disagrees with the companion on every Coda file
that has a non-default value. The reader therefore leaves those three at the Finale 27 default before Finale 3.0.
Agreement for the locations that are used is 57/57 Coda, 173/173 uncompressed, 374/374 DCL, and 497/497 zlib, with
genuine non-default coverage for `endMeasClefPercent`, `endMeasClefPosAdd`, `clefFront`, and `clefKey`.

`cautionaryClefChanges` is **bit 2** of the courtesy flags at selector `44` word 3, and `cautionaryKeySigChanges`
is bit 0. **Confirmed** by a controlled Finale 2005 pair: turning off the courtesy clef alone moves that word
`7 -> 3`, and turning off the courtesy key signature alone moves it `7 -> 6`. The second save is what makes this a
mapping rather than a guess — without it, any bit that happened to be clear would have fitted.

The corpus could not have settled this at all. All 1,120 companions have the option set, and only the values 5 and 7
occur, both of which leave bit 2 set. That is the same shape of trap as the Coda-era scalars: a location that is
never contradicted because nothing in the corpus varies it.

**Four of the nine clef options do not exist in the Coda era.** Alongside the courtesy clef below,
the reader treats `clefKey`, `clefTime` and `showClefFirstSystemOnly` as absent from that era, on
four independent grounds:

- the Finale 3.0-and-later locations for all three hold something else entirely before 3.0 —
  selectors `27` and `39` carry font tuples there, so reading them would report a font size as a
  spacing value;
- no Coda document in the corpus has a non-default `clefTime` or `showClefFirstSystemOnly` in its
  exact Finale 27 companion, in 57 pairs;
- the three whose companion shows a non-default `clefKey`, of 1, 1 and 12, hold no word matching
  those values anywhere in their globals, scaled or otherwise, and are the same three Sonata
  documents that carry every other Coda companion anomaly, so that is upgrade synthesis rather
  than a value read from the file; and
- the era's own user interface does not appear to offer them.

This is **strong** rather than confirmed: absence is being inferred, and one Coda document that set
any of the three would overturn it. The reader leaves all three at the Finale 27 baseline, which
already carries zero and false — the same values Finale 27 produces when it upgrades one of these
documents. Only the courtesy clef needs asserting, because there the baseline and the era agree but
the *record* would disagree.

The Coda era has no courtesy-clef option at all; the earliest version found to offer one is 3.6.2. Those documents
always show a courtesy clef, so the reader asserts that for the whole era rather than reading a record, and reports
it as `ValueOrigin::LegacyBehavior`: known exactly, stored nowhere, and not a guess at a default. It must not
read one: selector `44` word 3 is **zero in all 57 Coda files**, so bit 2 there would assert the opposite. That era
does store the courtesies it has as separate boolean words — the same controlled edit in Finale 2.6.3 moves selector
`12` word 1, the key signature's — and which word would hold a clef's is moot, since there is none.

The boundary is the epoch, not version 3.6.2. Finale 3.0 through 3.5 predate the option as well, but their files
already carry bit 2 set, so reading the bit gives the correct answer for them, and an epoch gate says that in one
line where a version range would have to name a release whose behaviour the bit already reports.

### Stem connections

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

#### The eight stem scalars, and the marker that dates them

The scalars around the collection are not one record. They are eight fields in five numeric
globals, distilled from the framework's preference location maps and then checked against exact
Finale 27 companions per era:

| musxdom field | Location | Present from |
|---|---|---|
| `halfStemLength` | `03(65534)` word 2 | Finale 3.5 |
| `stemLength` | `20(65534)` word 4 | every era |
| `shortStemLength` | `20(65534)` word 5 | every era |
| `revStemAdj` | `21(65534)` word 2 | every era |
| `stemWidth` | `64(65534)` word 5 | Finale 3.0 |
| `stemOffset` | `65(65534)` words 0–1, high word first | Finale 3.0 |
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

#### The reverse-stemming flag moved, and the word says which spelling it is in

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

Two locations are era-scoped rather than universal, in the same way the clef scalars were:

- `halfStemLength` is not stored before Finale 3.5. Selector `03` carries no row at all in the
  Finale 3.0 and 3.2 files, and in the Coda era its word 2 is zero while the companion shows 18.
- `stemWidth` and `stemOffset` hold their later locations only from Finale 3.0. In the Coda era
  both selectors read 5000 against a companion showing 128, on both platforms, and Finale 1.0.0
  carries neither selector. The reader excludes that era by epoch, which needs no version test and
  so cannot fail on the Coda-banner Windows documents that state no version.

Finale 27's own conversion of a Coda document invents a stem thickness that is in neither the
source nor the pinned baseline — 224 for the Finale 1.0.0 fixture, 128 for the Finale 2.6.3 one —
which is why those two fields are left at the baseline rather than matched to the companion.

The zlib era reaches the same eight through `numericGlobalClass` and byte offsets. The stem offset
is the field that proved four-byte class-record values are **not** a plain four-byte read: they
are the same two payload words the fixed rows carried, high word first, which differs from a
straight little-endian long on exactly the files where it matters.

#### The Finale 2012 payload and its stale predecessor

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

### Multimeasure rest defaults

**Confirmed for both physical layouts and for every field of the class.** The multimeasure-rest
defaults are the ordinary field-map kind of numeric global rather than a direct block: selector `25`,
comparator `65534`, and from Finale 2007 the class id `numericGlobalClass` derives, `25 + 0x0e = 0x0027`.
The automatic-update flag is the one field kept elsewhere, on selector `83` (class `0x0061`). Counts below
are distinct files of the reference corpus.

**Finale 3.5 rewrote the record**, and that boundary sits *inside* the uncompressed epoch, which is what makes
this a case for a structural marker rather than either kind of gate. The family's own size states which layout
a file uses:

| Era | Family | Files |
|---|---|---:|
| Finale 1.0.0–3.2 | selector `25`, **1 incidence**, 6 words | 264 (229 Coda-banner, 35 uncompressed), plus 19 Finale 1.0.0 fixtures |
| Finale 3.5–2006 | selector `25`, **2 incidences**, 12 words | 2,069 (767 uncompressed, 1,302 DCL) |
| Finale 2007–2012 | class `0x0027`, 24 bytes = the same 12 words coalesced | 1,389 |

No file in any survey carries any other word count, and none crosses the line: every 1.0.0, 1.8.7, 2.0.1, 2.6,
3.0 and 3.2 document is on the short side and every 3.5-and-later document on the long one. The files with no
selector `25` at all are containers the reader cannot classify to an epoch either.

All three surveys were run, and the two beyond the reference corpus are what make the boundary a measurement
rather than an interpolation:

| Survey | What it adds here |
|---|---|
| `rpatters1-main` | 3,725 documents, 1,130 later-layout and 59 early-layout companion comparisons |
| `tracked-evidence` | the only companion-backed Finale 1.0.0 documents anywhere: 19 fixtures, all six-word |
| `rpatters1-installs` | 12,116 documents, and the only 3.8, 98, 2011 and Coda-era-Windows material in any survey |

The installs survey settles three releases the reference corpus does not contain at all. **Finale 3.8 (11
documents) and Finale 98 (43) are both on the later side**, as are all 1,295 Finale 2011 documents; its 22
Finale 1.0.0 documents are on the early side, agreeing with the fixtures. It also supplies the population that
justifies reading the record instead of the header: **the 24 `PC 1.0+` Coda-era Windows documents state a
platform where their Mac contemporaries state a version, so they have no version at all**, and the marker
recovers all 24 where any version range would have skipped every one.

The later layout, addressed as absolute word slots across the two incidences:

| Word | Field | Word | Field |
|---:|---|---:|---|
| 0 | `measWidth` | 6 | `symSpacing` |
| 1 | *unnamed, zero in all 3,458 files* | 7 | `numAdjX` |
| 2 | `numAdjY` | 8 | `startAdjust` |
| 3 | `shapeDef` | 9 | `endAdjust` |
| 4 | `numStart` | 10 | *unnamed, zero in all 3,458 files* |
| 5 | `useSymsThreshold` | 11 | flags; bit 0 is `useSymbols` |

The early layout keeps only three of them, and two have moved: `measWidth` is still word 0, but `numAdjY` is
word **4** and `shapeDef` word **5**. Reading an early document through the later table would report its number
adjustment as a shape comparator. Words 1–3 hold something Finale 3.5 stopped writing — word 1 varies per
document across 0, 1, 2, 4, 5, 14, 16, 21, 22 and 25, and words 2–3 move together as `(24, 0)` or `(14, 1)` in
the Coda era and are `(0, 0)` in Finale 3.0 and 3.2. Finale 27's conversion carries nothing from them, so no
companion can name them; they are **open**.

Agreement with exact Finale 27 companions is complete: all 1,130 companion-backed later-layout documents match
on all nine scalars and on `useSymbols`, and all 59 companion-backed early-layout documents match on all three,
with no disagreement of any kind.

Two boundaries do not coincide with the layout marker:

- **Selector `83` arrives with Finale 97, internal 3.8.** No 1.0.0, `PC 1.0+`, 1.8.7, 2.0.1, 2.6, 3.0, 3.2, 3.5
  or 3.7 document in any survey carries it, and every 3.8, 97, 98 and later one does — the installs corpus
  confirms both spellings of that release and the Finale 98 that follows it, neither of which the reference
  corpus contains. Word 4 is `autoUpdateMmRests`; word **2** of the same record is also set in most documents
  and is *not* this flag — 468 companion-backed documents carry word 2 set with word 4 clear and none of their
  conversions has `<autoUpdateMmRests/>`, while all 73 that carry word 4 do. All 22 companion-backed documents
  that lack the selector entirely convert with the flag off. Across both large surveys the flag is only ever
  *set* in a zlib-era document, which is consistent with a feature added long after its record.
- **The H-bar adjustments and automatic updating are era behavior before their records exist.** The pinned
  Finale 27 baseline starts the H-bar 30 Evpu in, ends it 30 Evpu out, and switches automatic updating on; an
  early document states none of the three, and every early companion converts with all three at zero or false.
  The reader asserts them as `LegacyBehavior` rather than inheriting the baseline, and an absent selector `83`
  means automatic updating is off with no further qualification — including for a document whose epoch could not
  be classified, which is the document most at risk of being left claiming a Finale 27 setting. The rest of what
  the early era omits — `numStart` 2, `useSymsThreshold` 9, `symSpacing` 48, `numAdjX` 0 and `useSymbols` false —
  is left to the baseline, which already carries exactly what those conversions produce.

The `shapeDef` comparator is the source's own in every era and agrees with the companion in all 2,305 compared
documents, but 319 zlib documents name a shape their own file does not define. Those files carry no shape records at
all, while other zlib documents in the same corpus carry all three shape classes, so it is a property of those
sources rather than a decoding gap and the reader notes it at `Info` rather than warning; see [PRODUCTION_READINESS.md](PRODUCTION_READINESS.md#p22-dangling-shape-references-in-seeded-options).

`noHorizontalStretch` has no legacy spelling to find. **"Stretch Horizontally" is a Finale 27 feature**, so no
legacy format has anywhere to put it, and its value is known exactly — false — for every document this reader
will ever open. The corpus is consistent with that and cannot have shown it on its own: bit 0 is the only bit of
the flags word any document uses, the word is exactly 0 or 1 in all 3,458 later-layout documents, and no
companion sets `<noHorizontalStretch/>`. The reader asserts it as `LegacyBehavior` in every era rather than
leaving it to the pinned baseline, which also says false but says it as one Finale 27 document's setting rather
than as a fact about the formats.

### Text options

The framework preference tables name three of this class's fields and nothing else, so the rest was located by
searching for the Finale 27 defaults as a byte pattern and then diffing controlled one-variable saves. Counts
below are the reference corpus's 1,189 adjacent-exact companion pairs unless stated otherwise. The mapping is in
[`data/text_options_mapping.csv`](data/text_options_mapping.csv).

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

**Three enums order their lists first, opposite, centre, and two of them therefore disagree with musxdom.**
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

#### Accidental symbol inserts

**Confirmed for Finale 2001 onward; strong for Finale 3.7–2000.** `TextOptions::symbolInserts` is a direct
five-element array at selector `78(65534)`, class `0x005c` in the zlib epoch, in musxdom's own
`AccidentalInsertSymbolType` order: sharp, flat, natural, double sharp, double flat. The field order is the same
in every era:

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
[Font definitions](#a-shape-naming-a-font-the-source-never-defines--third-deliberate-disagreement): a shape whose
`SetFont` names a comparator the source never defines, which Finale 27 resolves to the wrong face by accident of
its own renumbering.

### Lyric options

**Partially implemented, and partial in a way worth stating plainly: eleven of this class's twenty-three fields have
no located storage at all.** What is implemented is `src/import/options/lyric_options.cpp`, covering the two
collections and five scalars across every epoch that stores them. The mapping is in
[`data/lyric_options_mapping.csv`](data/lyric_options_mapping.csv). Counts below are the 69 tracked fixtures and
their exact Finale 27 companions unless stated otherwise; **no corpus survey has been run for this class yet**, so
every confidence label here rests on the fixtures alone.

The class is spread over six numeric globals at comparator `65534`, reached in the zlib epoch through the usual
`numericGlobalClass` rule. They do not arrive together, which is why each is gated by its own record rather than by
one boundary:

| Selector | Zlib class | Arrives | Carries |
|---|---|---|---|
| `15` | `0x001d` | present in every era; word 1 usable from Finale 3.x | `maxHyphenSeparation` |
| `29` | `0x002b` | **every era, including the Coda banner** | `wordExtVertOffset`, the dialog's "Lift" |
| `30` | `0x002c` | **every era, including the Coda banner** | `wordExtHorzOffset`, the dialog's "Push" |
| `34` | `0x0030` | present in every era; word 5 usable from Finale 2004 | `useSmartWordExtensions` |
| `35` | `0x0031` | present in every era; word 5 usable from Finale 2004 | `useSmartHyphens` |
| `55` | `0x0045` | Finale 2004 | the nine word-extension connection styles |
| `57` | `0x0047` | Finale 2004 | `smartHyphenStart`, `wordExtNeedUnderscore`, `wordExtMinLength`, `wordExtOffsetToNotehead` |
| `58` | `0x0048` | Finale 2011, and the record grows to say so | the three `showAutoNumbersOn…` flags and `lyricAutoNumType` |
| `67` | `0x0051` | Finale 3.x | `wordExtLineWidth` |
| `87` | `0x0065` | Finale 2000 | the four syllable position styles |

Only three of the six are in the private framework study, which names selectors `15`, `35`, `57`, `67` and `87` for
this group. Selectors `55` and the whole connection table are **not in it at all** and were located from the corpus.

**`wordExtLineWidth` is the one scalar the fixtures vary, and it is what proves the group is read rather than
inherited.** Finale 3.7.2, 97, 2000 and 2002 store 118; Finale 2003 through 2007 store 224; one Finale 2012 document
stores 115. Each companion carries exactly that number, and the pinned baseline says 115, so a document inheriting
the baseline would be right once by accident and wrong everywhere else.

#### Two collections, and the two orders that do not match musxdom

The **word-extension connection table** at selector `55` is nine three-word elements — connection point, then the
horizontal and vertical offsets — laid out in musxdom's own `WordExtConnectStyleType` order. The payload is five
fixed rows, thirty words, of which the last three are padding.

Its connection point is numbered on a scale this class does not own. The values run `0x10` to `0x15`, continuing a
wider entry-connection numbering that begins at note and stem attachments, and their order is
`lyricRightBottom, headRightLyrBaseline, dotRightLyrBaseline, durationLyrBaseline, systemLeft, systemRight`.
musxdom puts the two system attachments third and fourth, so the value cannot be cast and is translated through a
table. `tests/evidence/F2006/F2006-embedded-tif.*` fixes the whole mapping in one document: it carries all six
numbers with five distinct vertical offsets beside them, and its companion names each connection point next to the
same offsets. The framework corroborates the base and the tail from an unrelated place — its smart-shape entry
connection enum reaches lyric-right-bottom at exactly `0x10` and ends with the two system attachments — with one
difference: that enum has no dotted entry, which is why its `duration` sits one place earlier than the records put
it. The records and the companions agree with each other, so they govern.

musxdom also keeps the **starting connection's two offsets twice**, once as that connection's own and once as the
class-level `wordExtHorzOffset` and `wordExtVertOffset` the Lyric Options dialog shows. The Finale 2006 fixture
shows they are one value rather than two that happen to agree: it stores 8 where every other tracked document
stores 4, and its companion moves both spellings together.

#### Lift and Push, the only lyric values the earliest era stores

**Confirmed on three representations, in every epoch.** The dialog's word-extension "Lift" and "Push" have numeric
globals of their own — selector `29` word 5 and selector `30` word 5 — and unlike everything else in this class
those exist in **every** era including the Coda banner. `tests/evidence/F100/F100-wext-push-6-lift-5.*` names them:
a Finale 1.0.0 save moving Push to 6 and Lift to 5 moves selector 29 word 5 from 4 to 5 and selector 30 word 5 from
4 to 6, moves no other word in the file, and its companion reads 5 and 6. Its ETF prints the same two rows.
`tests/evidence/F372/F372-lyricopts-changed.*` repeats it in the uncompressed epoch. Every tracked fixture agrees
with its companion on both fields across all four epochs.

Three things follow.

**The Coda-banner epoch is no longer empty for this class.** It supplies exactly these two fields and nothing else,
which is what its dialog exposes: Lift and Push are the whole of its lyric options. Finale 3.7.2 adds hyphen
spacing and word extension line thickness to them, and the four-option fixture confirms all four at once — it is
also the only tracked document anywhere that varies the hyphen separation, which promotes selector `15` word 1
from consistent-everywhere to confirmed.

**A recorded open correlation was wrong and is retired.** Pre-Finale-2004 documents whose companions show a
vertical offset of 1 rather than 4 had looked as though the value tracked the word-extension positioning bit. It
never did: those documents store 1 in selector 29. The correlation held across six fixture groups purely because
nothing else in the sample separated them, which is a fair warning about how convincing a coincidence can look
when the real field has not been found yet.

**Where selector 55 does not exist, the starting connection takes the dialog's values.** That is what Finale 27
does with such documents and what both new companions show. One further synthesis is deliberately *not*
reproduced: the companion also moves the `oneEntryEnd` connection's horizontal offset with Push, 42 to 44 in the
Finale 3.7.2 pair. A single specimen cannot distinguish that formula from several others that fit it, so the
reader keeps the pinned baseline's 42 and the difference is intended.

The **syllable position table** at selector `87` is four three-word positions across two fixed rows, again in
musxdom's own order: others (`default`), word extension, first syllable, start of system. Selector 87 arrives with
**Finale 2000**, the first release with a dedicated Lyric Options dialog, and these four positions are the whole of
what that dialog adds to the four settings Finale 3.x already had.

That order is corpus-confirmed rather than inferred from the framework's field names, which matters because the
last two positions carry identical values in almost every document and so discriminate nothing on their own. Five
reference-corpus documents are the exception and all five agree: a Finale 2004 document stores `(1, 2)` in words
6-8 against `(1, 1)` in words 9-11, with a companion whose `first` is centre/left and whose `systemStart` is
centre/centre, and four documents from Finale 2000 and 2010 carry the pair the other way round. Both directions are
observed, so the two cannot be swapped.

Its alignment and justification use a third numbering — `1 = centre, 2 = left, 3 = right`, from the framework's
`LYRICS_ALIGN_` constants — which is neither musxdom's `Left, Right, Center` nor its reverse. Every fixture that
carries the selector stores 1 where its companion says `center` and 2 where it says `left`, for all four positions.
**No surveyed document stores 3**: across all 1,189 companion pairs not one carries `right` as either an alignment
or a justification, for any position, so the corpus could never have settled that member.
`tests/evidence/F2000/F2000-lyropts-align-just.*` does, and supplies it twice over — the first syllable's alignment
set to Right and the system-start syllable's justification set to Right, so a mapping that translated only one of
the two fields would fail on it. Its companion reads `first` as right/left and `systemStart` as centre/right.
Against its parent the only options record that moves is selector 87's second incidence, where word 6 goes 1 to 3
and word 10 goes 2 to 3. **All three members of the legacy alignment list are now verified against Finale's own
conversion**, and the framework's `LYRICS_ALIGN_` constants are corroborated rather than relied on.

That fixture also carries both states of the flag bit in one record: setting a position's alignment enables it, so
`first` and `systemStart` gain 0x8000 while the word extension position stays off.

Bit 15 of each position's third word is musxdom's `on`. `tests/evidence/F2000/F2000-multilayer.mus` is the one
tracked document that clears it for the three optional positions, and it is the one whose companion omits their
`<on/>`; every other fixture sets all three and every companion emits them. The framework calls the first
position's flag a placeholder with no UI, and musxdom likewise documents `on` as meaningless for `default`.

#### Three values an era fixed rather than stored

- **The three optional syllable positions are off before selector `87` exists.** The pinned baseline switches all
  three on, and all 37 tracked fixtures from Finale 1.0.0, 2.6.3, 3.7.2 and 97 -- every document in the set without the
  selector -- convert with all three off.
  The alignments are left to the baseline, which already carries what those same conversions produce.
- **The word-extension line width is 224 where selector `67` does not exist.** All 25 Coda-banner fixtures convert
  to 224, and the baseline says 115. This is one of the few cases where the baseline supplies a value and supplies
  it wrongly.
- **Syllable edge punctuation is not ignored before Finale 2012**, where the setting does not exist. The pinned
  baseline says the opposite, so this is asserted rather than inherited. See the section below: the corpus settles
  both the boundary and the word, and the assertion covers every release through Finale 2010.

#### Syllable edge punctuation, and a confound that nearly named nine wrong answers

**Confirmed for Finale 2012 and for every earlier release, against all 1,189 adjacent-exact companion pairs of the
reference corpus.** `lyricUseEdgePunctuation` is **selector `57` word 4**, class `0x0047` byte 8 in the zlib epoch —
the fourth field of the six-word row that already holds `smartHyphenStart`, `wordExtMinLength` and
`wordExtOffsetToNotehead`. The reader recovers it from Finale 2012 and asserts the era's behaviour before that; both
branches agree with their companions on all 1,189 documents, with no read failure.

The boundary is **Finale 2011**, and getting there took a correction worth recording, because the reference corpus
alone gave the wrong answer:

| Release | Selector 57 word 4 | Companions: not ignored | ignored | Survey |
|---|---|---:|---:|---|
| Coda-banner through Finale 2003 | record absent | 454 | 0 | reference |
| Finale 2004 through Finale 2010 | 0 in every document | 487 | 0 | reference |
| **Finale 2010** | **0** in all 22 | 22 | 0 | installs |
| **Finale 2011** | **1** in all 597 | 597 | 0 | installs |
| Finale 2012 | 0 | 0 | 198 | reference |
| Finale 2012 | 1 | 50 | 0 | reference |

So the word exists for seven releases before it means anything, and reading it early would switch edge punctuation
off for all 941 of those documents against every one of their companions. That is what the version gate prevents.

**The reference corpus contains no Finale 2011 document at all**, so a boundary read off it is an interpolation
across that gap — and this reader had the gate at Finale 2012 until MakeMusic's own manuals contradicted it. The
Finale 2010 Document Options-Lyrics dialog has neither "Ignore Syllable Edge Punctuation" nor a "Punctuation to
Ignore" field; the Finale 2011 dialog has both; and the Finale 2012 manual's "Finale 2011 Interface Changes" page
states the feature arrived in 2011 outright. The installs survey then confirms it at exactly that line, all 22 of
its companion-backed Finale 2010 documents carrying 0 and all 597 Finale 2011 ones carrying 1.

The lesson is not that the corpus was wrong but that it was *silent*, and silence read as a boundary. The gap had
even been written down at the time — "Finale 2011 is absent from this corpus" — and the gate was still coded to the
nearest release that was present.

A generalization to avoid here, because the corpus invites it and a fixture contradicts it: all 597 companion-backed
Finale 2011 documents carry 1, and it is tempting to read that as the release's default. It is not.
`tests/evidence/F2011/F2011-baseline.mus`, created new in Finale 2011, carries **0** — a document made in that
release ignores edge punctuation by default. The 597 are Finale's own sample and template files, authored earlier
and converted into the release, and conversion switches ignoring off to preserve the older look. That is the same
behaviour the Finale 2012 cohort shows, where the 50 documents carrying 1 are the upgraded ones and the 198 born in
2012 carry 0. **A corpus of one vendor's shipped content is not a sample of what a release writes for a new
document.**

`tests/evidence/F2012/F2012-lyropts-noign-punct.mus` closes it with a controlled one-variable save, and confirms the
mapping predicted from the corpus before the fixture existed: clearing the checkbox moves byte 8 of class `0x0047`
from 0 to 1, no other word of that record moves, and the companion gains `<lyricUseEdgePunctuation/>`. It is the only
published document anywhere with the switch cleared, so it is the sole fixture exercising the word **set** rather
than clear; every other tracked Finale 2012 fixture sits on the ignored side. It also shows the switch and the tail
are independent — ignoring is off there and the list is stock, so the record stays twelve bytes.
**It is a version gate rather than a marker because nothing structural distinguishes the two cases**: the record is
twelve bytes in Finale 2007 and in Finale 2012 alike, so its shape says nothing and only the release does. It is
bounded inside the zlib epoch and fails closed onto the pre-2012 behaviour, which is right for every release but one.

The route to that word is worth recording, because the obvious search produced nine wrong answers first. Finale 2012
is the only release whose documents vary, so the 248 companion-backed Finale 2012 documents are the only cohort that
can locate the word — and a search for any record bit that partitions those 248 exactly returns **thirteen** of them,
across fonts, stems, clefs and beams. The split is not a punctuation split at all but a **lineage** split: the 198
that ignore punctuation are documents born in Finale 2012, carrying that release's own new defaults, and the 50 that
do not are documents upgraded into it from older files. Fields already mapped prove it — the same partition falls
across `wordExtLineWidth` (115 for the born documents against 118 or 224 for the upgraded ones), the syllable
positioning bits, and the starting connection's vertical offset.

What separates an arriving field from a lineage artifact is a **negative control in an earlier release**. A
pre-existing field already varies among Finale 2008 documents; a field that arrives with Finale 2012 is identically
zero throughout them. Applying that leaves four of the thirteen, and only one is a boolean in a lyric record:

| Candidate | Finale 2012 values | Finale 2008 values | Reading |
|---|---|---|---|
| `24` w120/123/126, `29` w5, `55` w2, `67` w5, `69` w1, `87` w8/w11 | — | already vary | lineage artifacts |
| `41` w15 | 0, −8 | all 0 | a 2012 arrival, but a coordinate |
| `48` w5 | 0, 100 | all 0 | a 2012 arrival, reads as a percent |
| `48` w17 | 0, 127 | all 0 | a 2012 arrival, reads as a mask |
| **`57` w4** | **0, 1** | **all 0** | **a 2012 arrival, boolean, in the lyrics record** |

`lyricPunctuationToIgnore` is **confirmed too, and it is a variable-length tail on the same selector `57` record**.
The corpus could not have found it and said so clearly: the element takes exactly two values across all 1,189
companions — absent in 993 and the stock set `,.?!;:'"“”‘’` in 196 — so not one document customises the list, and the
stock set appears nowhere in any Finale 2012 fixture's records or inflated blocks. **Finale writes the tail only when
the list differs from the stock one**, which is why searching for the default found nothing.

`tests/evidence/F2012/F2012-lyric-punct.mus` settles it: the list set to `#@%&`, four characters sharing nothing with
the stock set, grows the record from twelve bytes to twenty-four. The six scalars are unchanged and the characters
follow as 16-bit code units terminated by a zero:

| Words 0–5 | Words 6+ |
|---|---|
| the six scalars, unchanged | `0x23 0x40 0x25 0x26`, then `0x0000` |

The reader takes the tail from word 6 to the first zero word and converts it with musxdom's own
`EnigmaString::toU8`, so it is agnostic to how the record grows — the observed payload is consistent with expansion
in twelve-byte chunks, and the terminator rather than the chunking is what the decode depends on. Astral characters
would arrive as UTF-16 surrogate pairs and are handled, though nothing exercises that: every character of the stock
set and of the fixture is in the basic multilingual plane.

**The tail has two layouts, and the Unicode release is the boundary.** Finale 2012 stores one 16-bit code unit per
character; **Finale 2011 stores packed 8-bit bytes in the saving platform's code page**. Neither can be read as the
other: both controlled containers are little-endian, so decoding the Finale 2011 byte string through the word path
would transpose every pair of characters.

`tests/evidence/F2011/F2011-lyric-punct.*` settles the older layout and its encoding together, and it exists because
a Finale 2011 installation was obtained for the question — no survey had an authored document of that release, and
0 of the 597 shipped ones carries a tail at all. Its list is `#@%&«»` and its tail is `23 40 25 26 c7 c8`, six bytes
in string order rather than three words. **The last two bytes are what make the code page a measurement rather than
an assumption**: `0xc7 0xc8` is the guillemet pair in Mac Roman and `ÇÈ` in Windows-1252, and the companion reads the
guillemets. The reader takes the bank from the document's own platform, as it does for a font definition carrying no
charset of its own — the only source available, since this text belongs to no font record — and this is the one
specimen anywhere that tests that choice for this field.

**Finale writes the tail on the list, not on the switch.** `F2012-lyropts-noign-punct.mus` turns ignoring off and
leaves the list stock, and its record stays twelve bytes; the tail appears only when the list itself differs.

**A document with no tail keeps the stock list, and the reader does nothing about it.** The pinned baseline states no
`<lyricPunctuationToIgnore>` either, and musxdom's `LyricOptions::integrityCheck` supplies exactly that set for an
empty one, so writing it here would be a second copy of a default musxdom already owns. Two corpus documents ignore
punctuation while carrying no element at all, which is still unexplained and recorded rather than smoothed over.

The fixture also carries an unasked-for change worth keeping: Finale rewrote the word-extension connection table as
the dialog closed, giving five of its nine styles a vertical offset of 5 and a sixth an offset of 1. That makes it a
second non-default specimen for a collection that otherwise rests on one Finale 2006 document.

#### Automatic lyric numbering, and a record that states its own layout

**Confirmed.** Automatic lyric numbering arrives with **Finale 2011** — the Finale 2010 lyric dialog has no such
option and the Finale 2011 one does — and its four fields occupy words 6 to 9 of **selector `58`**: the three
`showAutoNumbersOn…` switches and then `lyricAutoNumType`. musxdom's `AutoNumberingAlign` is in the legacy order,
`None` then `Align`, so the type passes through untranslated.

**This one needs no version gate, because the record states its own layout.** Selector 58 is six words in every era
before Finale 2011 and twelve from it on. Every fixed-row fixture carries six; the Finale 2007 class record carries
twelve bytes; the Finale 2011 and 2012 ones carry twenty-four. The installs survey confirms the marker exactly at
the boundary: **all 22 companion-backed Finale 2010 documents carry twelve bytes and all 597 Finale 2011 ones carry
twenty-four.** That contrast with the edge-punctuation setting a few paragraphs up is worth keeping — that field
had to take a version range because its record does not change shape, and the range was wrong for a year of
releases until the manuals corrected it. This field's record changes shape, so nothing has to be inferred about
which release a file came from.

Three booleans and an enum cannot be separated by saves that each move one thing, because four fields need four
distinct signatures. The three tracked Finale 2011 saves carry a binary code instead — the type moves only in the
first, Verses in the first and second, Choruses in the first and third, Sections in all three — so every field has
a unique pattern and none can be mistaken for another.

A document with the short record shows no automatic numbers at all, and the three switches are asserted false on the
same footing as the smart-lyric group. `lyricAutoNumType` is left to the baseline, because the numbering type of a
document that displays no numbers means nothing and the baseline already carries what every companion of every era
shows.

All three saves also add one class-detail record, `0x0455` at cmper 1/1, identical in each. It is not part of this
class — the switches are entirely in selector 58 — and appears to be per-lyric state that numbering being in use
creates. It is noted for whichever class reaches it, not read here.

#### Two boundaries that are gates rather than markers, and why

Neither collection has a size that states which layout a file is in, because neither has two layouts: a document
either carries the record or does not. Presence is therefore the instrument throughout, which reaches the
Coda-banner era's Windows documents that state no version at all. **The one place presence is not safe is selector
`55` in the Coda-banner epoch**, where the number is reused by an unrelated option: the Finale 1.0.0 and 2.6.3
fixtures store values such as 16128 and 16448 in it, and one controlled Finale 1.0.0 stem-options save moves its
first two words. Reading that as the connection table would fabricate nine styles out of another option's bytes, so
the epoch is excluded outright and the word count guards it a second time.

Selectors `34` and `35` word 5 are the other qualified cases, and they behave identically. The word exists in every era and is zero in every document before
Finale 2004 and set in every one after, while every companion of every era switches smart hyphens on. That is what
an option arriving with a default of *on* looks like from before it existed, so reading the word on an older
document would turn smart hyphens off for the whole pre-2004 corpus. The reader gates both on selector `57`, which arrives with the same release. The Finale 2.6.3 fixture removes any
doubt about selector `34`: it stores **12** in that word, which is no boolean at all.

Two controlled Finale 2004 saves name the switches. `F2004-lyropts-nosmart-wext.*` moves selector 34 word 5 from 1
to 0 and its companion loses `<useSmartWordExtensions/>`; `F2004-lyropts-needuscore.*` moves selector 57 **word 1**
from 0 to 1 and its companion gains `<wordExtNeedUnderscore/>`. Both are confirmed on record, ETF and companion
together. The Finale 2004 dialog words the underscore setting the opposite way round and unclearly, and Finale had
reworded it to match the modern boolean by Finale 2012; the companion settles the sense rather than the wording, and
the stored value needs no inversion. That also fills the last unexplained word of the selector 57 record — word 1
had been noted as zero everywhere and was once a candidate for a punctuation-list reference — leaving only **word
5** of that record unassigned.

#### Why all three smart-lyric switches are forced off before Finale 2004

**Smart hyphens, smart word extensions and the underscore requirement arrive together with Finale 2004**, so all
three are false for an earlier document rather than merely unstated, and the reader asserts them rather than
inheriting the baseline. The words that carry them are zero in every earlier document whether the option is off or
has not been invented yet, so reading them would be reading nothing.

**The reason is about what this reader can produce, and it is worth stating plainly because it is the class's only
deliberate disagreement with Finale 27.** Smart hyphens and smart word extensions are not standalone settings:
each is implemented as smart shapes — hyphen smart shapes for the one, word-extension smart shapes for the other.
Finale 27 manufactures those during its upgrade, which is exactly why every pre-2004 companion comes back with
`<useSmartHyphens/>` and
`<useSmartWordExtensions/>`, and why the pinned baseline switches both on. **This reader does not manufacture
them.** Leaving the switches on would describe a document it has not built — an option claiming a rendering that
nothing in the imported pools can draw. The honest value is false, and a future coverage survey will report a
systematic companion mismatch on every pre-2004 document as a result. That is intended.

The underscore requirement is the quiet member of the group, since baseline and companions already leave it false;
it is asserted on the same footing as `MultimeasureRestOptions::noHorizontalStretch`, because the era's behaviour
is known rather than inherited.

#### What the Coda-banner epoch recovers, and what remains open

**The Coda-banner epoch recovers nothing from its records for this class**, and that exclusion is intended: it
stores none of the six selectors in a usable form. Its selector `15` word 1 is zero where every companion says 144,
and its selector `55` is a different option entirely. What it does get is the three assertions above, so a
Coda-banner document is not left silently claiming Finale 27 settings.

Eleven fields are not read from any era, and they divide into two quite different cases. Every companion agrees
with the pinned baseline on all eleven, so the fixture set cannot tell them apart on its own; what separates them
is knowing which Finale release introduced each option.

**Three of them postdate Finale 2012 and are therefore settled rather than open**: `hyphenChar`,
`useAltHyphenFont` and `altHyphenFont`. Finale 2012 is the last release this reader opens, so no `.mus` file of any
era has anywhere to put them. All three are nonetheless handled differently from each other, and the differences
are the useful part.

`useAltHyphenFont` is **asserted false and reported as `LegacyBehavior`**, exactly as
`MultimeasureRestOptions::noHorizontalStretch` is, and for the same reason: the pinned baseline also says false,
but the two statements are not the same statement. The baseline saying false is one Finale 27 document's setting,
which a later pinned resource could legitimately change; the setting postdating every legacy format is a fact about
the formats, and it is what makes the value *known* rather than synthesized. `LegacyBehavior` marks a value the
reader asserts on the strength of era knowledge, whether or not the baseline happens to agree — not merely one that
contradicts the baseline.

`hyphenChar` is **left exactly as seeded and reported as `Finale27Default`**, and the asymmetry is deliberate. A
boolean that is false because its feature does not exist can be stated in code without restating anything; the
character a legacy document drew cannot, because writing U+002D beside a pinned resource that already says 45 would
be a second copy of one fact — the case the repository's rule is actually aimed at.

`altHyphenFont` is the one that needed a decision. **It is absent from the pinned `<lyricOptions>` element
entirely**, so the seeded member holds a null pointer, and musxdom populates it only from an `<altHyphenFont>`
element and otherwise synthesizes one in `integrityCheck` — which runs at the end of construction, after every
importer. A null pointer during the import therefore means exactly one thing, that the baseline carried no such
element, **and it means it without reading the baseline's XML**, which no importer has access to in any case. The
reader declines: there is no value to import, so none is imported and none is reported, and musxdom fills the
member in afterwards as it does for any document that omits it.

The obvious-looking alternative is wrong and worth naming, because it was tried first. A `FontInfo` the baseline
*did* seed would carry the baseline's font numbering rather than the imported document's and would need
`musx::dom::importFontDefinitionInto` before it named anything here. But a member the baseline never filled in is
not a seeded value needing repair — it is an absent one, and copying the reference document's own synthesized
placeholder into it would put a value in the document that no document ever stated.

`tests/mapping_tests.cpp` pins all three behaviours with a `LyricOptions` seeded to contradict every assertion the
reader makes, `hyphenChar` set to `~` among them. Removing the boolean assertion fails it, and so does hard-coding
the hyphen character; both mutations were checked.

**Eight remain genuinely open**: `useSmartWordExtensions`, `wordExtNeedUnderscore`, `lyricPunctuationToIgnore`,
`lyricAutoNumType`, and the three `showAutoNumbersOn…` flags. There is one lead:
`tests/evidence/F2012/F2012-upstem-flags.mus` is the only document anywhere whose companion omits
`<lyricAutoNumType>`, and it differs from its sibling in no lyric record this study located. The framework's lyrics
preference map names none of the eleven, and explicitly has no field for the alternate hyphen font, which
[`data/legacy_option_font_id_locations.csv`](data/legacy_option_font_id_locations.csv) records as
`not_identified` — correctly, and now for a stated reason rather than as an unfinished search.

One correlation is recorded and **not** implemented. In documents older than selector `55`, Finale 27 synthesizes
the starting connection's vertical offset as 1 when the word-extension syllable positioning bit is set and 4 when
it is not — six fixture groups agree, and no other record separates the two cases. Whether that is a real legacy
behaviour or an artifact of the converter is unsettled, so the reader leaves those documents at the baseline's 1
and the question stays **open**.

## The text pool

**Confirmed** for the uncompressed, DCL, and zlib epochs against the controlled fixtures and
their Finale 27 companions. **Open** for the Coda-banner epoch.

The text pool is the one pre-2007 pool that is not made of records, and the one place the
format describes itself in words. It is a byte stream of `^keyword(n) ... ^end` chunks packed
end to end, with nothing between them — the same shape ETF prints in its own text section.
The keyword names the class, and the number in parentheses is the comparator musxdom keys the
texts pool by.

| Epoch | Block | Text form of commands | Binary form |
|---|---|---|---|
| Coda banner | not in a block; see below | — | — |
| Uncompressed | `0x0004` | `^font(Name)`, `^efx(name)` | not observed |
| DCL | `0x0012` | `^font(Name,charset)`, `^nfx(n)` | yes |
| Zlib | `0x0017` | same as DCL | yes, UTF-8 encoded from Finale 2012 |

Keywords observed, with the musxdom class each names:

| Keyword | Class | Evidence |
|---|---|---|
| `block` | `texts::BlockText` | every epoch's fixtures |
| `verse` | `texts::LyricsVerse` | `F2007-lyric-hyphens.mus` |
| `chorus` | `texts::LyricsChorus` | **inferred** from `verse`; no fixture has one |
| `section` | `texts::LyricsSection` | **inferred** from `verse`; no fixture has one |
| `smartshape` | `texts::SmartShapeText` | `F2006-embedded-tiff.mus` |
| `expression` | `texts::ExpressionText` | `F2006-embedded-tiff.mus` |
| `fileInfo` | `texts::FileInfoText` | `F2008-BE-text-inserts.mus` |

No keyword has been found for `texts::BookmarkText`. The reader reports any keyword it does
not recognize, by name, which is how a missing spelling is meant to be found.

### Section markers before Finale 97

**There are three framings, and each stream states its own.**

Finale 3.7.2 and earlier open the pool with the ETF section markers themselves. A Finale 3.7.2
document carries one stream beginning `^text`, then its `^block(n)` records, then `^lyrics` and
whatever lyric records follow; an empty document is the bare bytes `^text^lyrics`, two markers
with nothing between them. **A record in this framing has no terminator** — it runs to the start
of the next record, to the marker that opens the next section, or to the end of the stream.

`F372-fileinfo-text.mus` is what shows it. Its ETF export carries the same records in the same
order and the same spelling, differing only in the carriage returns ETF puts between sections,
so a MUS of this era stores very nearly what ETF prints.

Finale 97 and later drop the markers and close each record with `^end`. Each record names its
own kind, so a section header has nothing left to say.

The Coda-banner epoch is different again: two length-prefixed chunks behind the last record
pool, opening `^text ` and `^lyric ` (Finale 1.0.0) or `^text()` and `^lyrics()` (Finale 2.6.3).
Its lyric records live there, in the unterminated form; its block text does not — see
[The Coda-banner epoch](#the-coda-banner-epoch).

The reader tells the first two apart by reading the opening bytes rather than by dating the
file: a stream that opens with a section marker is unterminated, and one that opens with a
record is not. The boundary sits inside the uncompressed epoch, so an epoch gate cannot express
it, and the marker is something every stream carries.

### Enigma commands in the compressed epochs

From the DCL epoch Finale writes most commands in a binary form: a caret, a one-byte code, and
an argument spelled as hexadecimal digits one byte per digit, each digit stored one greater
than its value. `^\x86\x01\x01\x02\x09` is `0x0018`, which the companion writes as
`^size(24)`. The run ends at the first byte outside `0x01`–`0x10`, which is what lets literal
text follow an argument directly.

**Confirmed** by `tests/evidence/F2006/F2006-text-inserts.mus`, a document written to answer
this and nothing else: one text block per insert the Finale 2006 Text Tool offers, each
holding that insert alone, so the block number identifies the pairing. Finale 2006 is the only
release that can settle it — the first to write these codes and the last to export ETF — so
the same records exist encoded in the `.mus` and spelled out in the `.etf`. The Finale 27
companion agrees with the ETF on all 21 records, giving every row below two witnesses.

| Code | Command | Argument |
|---|---|---|
| `0x81` | `^baseline` | 8 digits |
| `0x84` | `^nfx` | 4 digits |
| `0x85` | `^font` | 4 digits — a comparator |
| `0x86` | `^size` | 4 digits |
| `0x87` | `^superscript` | 8 digits |
| `0x88` | `^tracking` | 8 digits |
| `0x8a` | `^composer` | — |
| `0x8b` | `^copyright` | — |
| `0x8c` | `^date` | 4 digits |
| `0x8d` | `^fdate` | 4 digits |
| `0x8e` | `^dbflat` | — |
| `0x8f` | `^dbsharp` | — |
| `0x90` | `^description` | — |
| `0x91` | `^filename` | — |
| `0x92` | `^flat` | — |
| `0x94` | `^natural` | — |
| `0x95` | `^page` | 8 digits |
| `0x96` | `^sharp` | — |
| `0x98` | `^time` | 1 digit |
| `0x99` | `^title` | — |
| `0x9a` | `^totpages` | — |
| `0x9b` | `^perftime` | 4 digits |
| `0x9c` | `^cprsym` | — |
| `0x9d` | `^value` | — |
| `0x9e` | `^control` | — |
| `0x9f` | `^pass` | — |
| `0xa0` | `^partname` | — |
| `0xa1` | `^lyricist` | — |
| `0xa2` | `^arranger` | — |
| `0xa3` | `^subtitle` | — |
| `0xa4` | `^fontTxt` | 4 digits — a comparator |
| `0xa5` | `^fontMus` | 4 digits — a comparator |
| `0xa6` | `^fontNum` | 4 digits — a comparator |
| `0xa7` | `^rehearsal` | — |

Three properties of the argument are fixed by three of that fixture's blocks, and by nothing
else in any survey:

- **It is one value, not a sequence of shorter ones.** Block 6's page offset is stored
  `01 01 01 01 01 02 02 04`, whose eight digits read together are `0x113`, and both witnesses
  write `^page(275)`. Read as two four-digit arguments the same bytes are 0 and 275, so only a
  non-zero offset could have told the two apart.
- **The digit range runs the full 0 to 15.** Block 21's `^superscript(15)` is
  `01 01 01 01 01 01 01 10` — the only place a nibble of `0xf` appears anywhere, and therefore
  the only evidence that the `+1` offset holds across the whole range rather than only up to
  the `0xe` that everything else stops at. No argument byte is ever `0x00`, which is
  presumably the point: a zero byte would end a C string.
- **Widths are 1, 4, or 8 digits, and belong to the command rather than to the value.**
  `^nfx(0)` still spends four digits on a zero, so nothing about a value shortens its
  argument. `^time` is the only one-digit command known, and its single digit is the
  seconds flag.
- **An argument is signed, in two's complement at whatever width it occupies.** Block 19's
  `^baseline(-13)` is `10 10 10 10 10 10 10 04`, or `0xfffffff3`; read unsigned that is
  4294967283. No four-digit argument has been observed negative — the commands that use that
  width carry a font comparator, a point size, a style mask, or a format ordinal — but both
  widths are read signed, and Finale itself is the reason. Its page offset and tracking dialogs
  both run 0 to 32767, the signed 16-bit maximum, in fields whose argument is eight digits and
  so has 32 bits to spend: the value behind each dialog is a signed short widened on the way
  out. Baseline and superscript write that same eight-digit argument and are routinely
  negative, so the floor belongs to each dialog while the width belongs to the encoding. The
  one case where the two readings
  would differ is a font comparator above 32767, which needs a document with 32768 font
  definitions; nothing is done about that until one appears.

Several of these are resolved by a parsing context rather than by the general Enigma parser.
`^value`, `^control` and `^pass` come from the context a `TextExpressionDef` supplies, all three
being playback properties of the expression the text belongs to. `^rehearsal` comes from the
context a `MeasureExprAssign` supplies, since the mark it produces depends on where in the score
the expression is assigned. `^filename` is resolved by neither, and is for the client to
resolve: only the client knows the name it saved the document under, or is about to save it
under. The reader writes all of them out regardless, because each is what the document says,
and an insert this reader cannot resolve is left for whoever can.

**Cross-referenced against every command `musx/util/EnigmaString.h` documents**, the table is
now complete: every command named there has a code. What remains is the reverse question, the
slots no observed command occupies.

**Still open:** `0x80`, `0x82`, `0x83`, `0x89`, `0x93`, `0x97`, and anything from `0xa8` up.

The codes fall into two groups. `0x81` to `0x88` are the style commands, in no order this
reader can see. From `0x8a` up they are inserts, in alphabetical order, with `fdate` at `0x8d`
the one member out of place — which it would not be if its internal name began with `date` —
and with `perftime`, `cprsym`, `value`, `control` and `pass` following `totpages` in the order
a later release appended them.

**A prediction made from that ordering was half right, and the wrong half is worth recording.**
The Finale 2006 table left four gaps inside the alphabetical run, at `0x89`, `0x93`, `0x97` and
`0x98`, exactly where `arranger`, `lyricist`, `subtitle` and `time` would sort. Finale 2008 has
all four inserts, so it settles them: `^time` is indeed at `0x98`, but `^lyricist`, `^arranger`
and `^subtitle` are at `0xa1`, `0xa2` and `0xa3`, **appended** past the end alongside
`^partname` at `0xa0`. That is the only thing a release can do when it adds an insert, since
renumbering would change the meaning of every document already saved — so a name absent from an
earlier release will never appear inside the alphabetical run, and the run reflects one
original alphabetized set rather than a rule still in force.

`0x89`, `0x93` and `0x97` are therefore unexplained holes rather than reserved slots, and
`0x98` was a member Finale 2006 simply did not expose. **None of the predictions was ever put
into the reader**, which is why the refutation cost nothing: a wrong name resolves to the wrong
document field and reads as recovered content, where an unlisted code is reported by number and
reads as what it is.

**One standing hypothesis, deliberately untested:** that `0x82` and `0x83` are `^fontid` and
`^fontNum`, or the two the other way round. It is recorded here as a tripwire — so that a
document which turns out to carry either code has something to be checked against — and not
because any evidence supports it. No corpus has been surveyed for it, and none should be on its
account alone. It stays out of the reader's table until a document states it, for the reason the
next paragraph gives.

**A second prediction from the same shape was also refuted, the same way.** Three of the free
slots — `0x80`, `0x82`, `0x83` — sit inside the style group between `^baseline` at `0x81` and
`^nfx` at `0x84`, which is exactly where the three font-category commands would belong.
`F2011-text-inserts.mus` puts them at `0xa4` to `0xa6` instead, appended past the last insert
alongside `^rehearsal` at `0xa7`. Appending is evidently what every release after the original
set does, whether the command is an insert or a style command, and the style group is as closed
as the alphabetical run is. The six remaining slots are left empty in the reader rather than
filled by inference, for the same reason as before.

From Finale 2012 the pool is UTF-8, so the same code arrives as its two-byte encoding:
`^\xc2\x85` rather than `^\x85`. The reader accepts both spellings structurally rather than
gating on the version.

### The parenthetical value after a font name

`^font(Times,4096)` and `^font(Engraver Text T,8194)` carry the same packed character-set word
the `FN` record's own header holds: the high nibble is the bank, 1 for Mac and 2 for Windows,
and the low twelve bits are the character set value. `4096` is `0x1000`, Mac with no character
set, and `8194` is `0x2002`, the Windows symbol character set — and both agree exactly with
the `FN` header of the font they name in `F2006-embedded-tiff.mus`. The value is therefore
redundant with the font definition, and the reader does not carry it forward.

A font referenced by id is spelled `Font` followed by the id, with a character set of `0`:
`^font(Font0,0)`. musxdom reads that spelling natively.

**A companion's font table is not stable evidence.** Finale 27 matches fonts against the ones
installed on the machine doing the upgrade, and the path taken — a scripted upgrade or a manual
one — can change the result too. Re-exporting the same source document has been observed to
renumber its whole `fontName` table, drop a face and shift every `FontOptions` comparator with
it. So a companion is a good witness to *which face* a text is set in and a poor one to *which
comparator*, and two companions of the same document generated at different times may disagree
about the comparator without either being wrong.

The source's own font table has no such elasticity: it is what the document stores. That is the
reason the rule below reads in the direction it does.

**The reader names the font wherever the document defines one.** Every font command resolves to
a comparator first — `^fontid` and every binary code state one outright, `Font` followed by
digits is the same thing under Finale's convention for a font it knows only by id, and a name is
matched back to a definition — and the comparator is then written out under the name that
definition carries. This is why `importFontDefinitions` runs before any text importer.

`^font` is the spelling for a plain reference, whatever command the source used, and
`^fontMus`, `^fontTxt` and `^fontNum` keep theirs: the marking category they name is the one
thing `^fontid` cannot carry.

**`^fontid` is the fallback, not the normal form.** It is the one spelling that needs no
definition to exist, so it is what a comparator the document does not define becomes. That
fallback also drops the marking category a categorized command names, which is the lesser harm
against inventing a name for a definition that is not there. A *name* that resolves to nothing
is different again: there is no comparator to fall back to, so the command and the name are both
kept and the absence stays visible.

The character-set parenthetical is not carried forward in any of these forms, for the reason
above: the definition states it, and musxdom reads it only from there.

### Encoding

Text is encoded per font, not per document: the font in force at that point in the string
names the code page through its own `charsetBank` and `charsetVal`. Two cases are not a code
page at all and preserve the byte as the code point of the same value:

- a font whose character set says symbol, which musxdom's `calcIsSymbolFont` decides;
- font id 0, the default music font, whose bytes are glyph numbers whatever its record says.
  Finale 97 records Mac Roman for its own `Pmusic`, yet Finale 27 converts the same document's
  expression characters byte for byte and rewrites the font as a symbol font. The character
  set fields did not start carrying the symbol marker until the compressed eras.

Legacy line breaks are carriage returns and become line feeds: Finale 27 writes `\n` where
`F97-fileinfo-short.mus` has `\r`, and emits no `&#xD;` anywhere.

### Expression text before it moved into the pool

**Confirmed** for the uncompressed epoch. Finale 2000 and earlier keep expression text inside
the text expression definition, in the `DT` family, one expression per comparator:

| Location | Field |
|---|---|
| incidence 0, byte 0 | point size |
| incidence 0, byte 1 | font definition comparator |
| incidence 0, word 1 | `nfx` style bits |
| incidence 1 onward | the display text, twelve bytes per row, ending at the first NUL |

**Recovering it is deferred until `TextExpressionDef` is imported.** The layout above is
established, and the reader once synthesized an Enigma string from it, but a text pool full of
expression strings with no definitions behind them claims more coverage than it has: the
definition is what gives the text its meaning. The synthesis is removed rather than switched
off, and a test asserts that these eras produce no `ExpressionText`, so reinstating it is a
deliberate act.

**The move happens inside the DCL epoch, not at it.** Finale 2002 still keeps display text in
`DT` under exactly the layout above — `F2002-fileinfo-text.mus` holds `ffff`, `pppp` and
`Tempo (=#)` there, matching its companion's expressions — and its text pool carries no
`^expression` record at all. By Finale 2006 the display text has moved to the pool and the string
embedded in `DT` is the expression's *description* instead, `Below Staff (Vel. 127)` and the
like. Reading `DT` as display text in that later range would fill the texts pool with category
descriptions.

The move is therefore bounded between Finale 2003 and Finale 2006 and is otherwise **open**. No
document of that range defines an expression.

The Coda-banner `DT` layout differs again — its size is a whole word rather than a packed
byte, and its text incidence carries further fields after the string — and is **open**.

### File Info

**Confirmed.** See [Document metadata in the header text region](#document-metadata-in-the-header-text-region)
for the offsets. Title, composer, copyright, and description map to
`texts::FileInfoText` types 1 through 4.

**Four fields is the whole of File Info through Finale 2006.** `F2006-text-inserts.mus`
establishes it from the other side: that release's Text Tool offers a `^description` insert and
no `^lyricist`, `^arranger` or `^subtitle`.

**By Finale 2008 all seven exist and File Info has left the header entirely.**
`F2008-BE-text-inserts.mus` carries every one as a `^fileInfo(n)` record in the text pool,
numbered by musxdom's own `FileInfoText::TextType` and confirmed field for field by its
companion, while the header offsets that carry them in Finale 97 are empty. Where between the
two releases the move happened is **open**, and does not need answering: the reader fills from
the header only what the pool did not supply, so each document states for itself which way it
stores them.

**The header offsets do still hold across the DCL epoch.** `F2002-fileinfo-text.mus` carries all
four fields there and its companion recovers all four, so the move to the pool happens somewhere
after Finale 2002 rather than at the epoch boundary.

**The lower end is bounded above by Finale 3.7.2 and is otherwise open.**
`F372-fileinfo-text.mus` carries all four fields at the header offsets and its companion recovers
all four, so File Info exists by that release, which is also the earliest that writes the modern
inserts reading them. Finale 3.7.2 is the earliest release *available here* whose dialog offers
the fields; whether an earlier release offers them is untested, because no earlier release is
available to test. It is a ceiling on the arrival, not the arrival.

Whether an earlier release stores the fields cannot be settled by any corpus at any size, since
an unfilled dialog leaves the same empty offsets as a dialog that does not exist. It would need
a release whose dialog offers the fields, and no earlier one is known to.

### Bookmarks

**From Finale 2012 a bookmark's text is an ordinary text-pool record**, keyword `bookmark`, with
the `^end` terminator of that era and no style commands of its own — `^bookmark(2)Page über^end`.
musxdom documents any Enigma insert appearing in one as meaningless, but the bytes still need
decoding, so the record goes through the same converter as every other.

**Its text is UTF-8, and `F2012-bookmarks.mus` shows it directly rather than by inference:**
`c3 bc` is U+00FC for the `ü` of `Page über`, and `c2 ab c2 bb` is the guillemet pair of
`Scroll «» Bookmark`. The same pair is 8-bit in the pre-Unicode eras — `c7 c8` in Mac Roman —
which is what makes it a measurement.

**Before that the text is in the `BK` others family**, one family per bookmark at comparators
from `0x8000` up, and the layout is the Coda-banner `HT` shape: the string occupies the first
four incidences as 48 bytes ending at the first NUL, and two numeric incidences follow it. The
text is 8-bit — `9f` for `ü` under Mac Roman — and the text pool of such a document holds no
bookmarks at all.

**Reading `BK` is deferred until the bookmark class itself is imported**, on the same footing as
expression text before `TextExpressionDef`: text with no bookmark behind it claims more coverage
than it has. A test asserts that these eras produce no `BookmarkText`, so reinstating it is a
deliberate act rather than a side effect. Deciding to support bookmarks is what would make the
rest of the `BK` family worth decoding.

**Unverified: that the move into the text pool belongs to the Unicode project.** It fits — the
pooled form is UTF-8 and the era is the one that converted stored text — but the boundary has not
been tested inside Finale 2012. It is possible that the first release of that version still wrote
`BK` and that a point release changed it, in which case a version gate on the major alone would
be wrong. Nothing turns on this while the reader takes whichever form the document presents:
`^bookmark` records are read where they exist and `BK` is read nowhere.

The earliest specimen available is Finale 3.7.2, which bounds the arrival of bookmarks from above
and says nothing about it otherwise: no earlier release is available to test. The comparators are
not stable across an upgrade — the same two bookmarks are 1 and 2 in the Finale 3.7.2 companion
and 2 and 3 in the Finale 2012 one.

### The Coda-banner epoch

**Open, and deliberately uncovered.** Its two length-prefixed text chunks are empty in every
fixture. Its block text is instead in the `HT` others family, as a stream that alternates
NUL-terminated strings with binary layout data across incidences — `F263-baseline.mus` holds
ten block texts that way, and Finale 27 recovers all ten. Its own binary commands use a
shorter argument form than the later epochs (`^\x82\x40\x81`), so the digit rule above does
not apply to it. None of this is decoded.

The pool walk that reaches all this is now correct, which it was not. It stopped at the first
pool with zero pages, so a Finale 1.0.0 document — whose details pool is empty — reported one
block instead of three and never reached its entries pool or the text region beyond it. **An
empty pool is an ordinary pool**, and the page size is the only thing that identifies a
prologue. The chain needs no terminator of its own: what follows the last pool is the text
region, whose first four bytes are a chunk length rather than `0x200`.

Two things stand between the fixed walk and actual recovery, and neither is a container
problem. The `HT` framing is one. The other is that **no available Coda-banner document with
text is a Finale 1.0.0 one**: all 21 tracked 1.0.0 fixtures carry no text at all, and their
companions show only the `Score` part name, which lives in `PartDefinition` and is not yet imported. The five Finale 2.6.3 fixtures
do carry text, but all five are the same document, so the era has one text specimen rather than
five. The 1.0.0 spelling of the region markers differs from 2.6.3's — `^text \0` and `^lyric \0`
against `^text()` and `^lyrics()` — which is reason enough not to assume one layout covers both.

## Open questions the regression comparison surfaced

Two differences against companions are unexplained and are recorded here rather than suppressed
in the comparison, because a regression that hides what it cannot explain is worth less than one
that counts it.

**Stem connection fonts are confirmed on every connection in every survey**, comparator and
resolved face alike: 53,125 preserved, none differing. The field's provenance is worth recording
because it was the weakest in the element. It is not in the distilled framework mapping, which
has no row for this table at all, and the notes above confirm the element's other properties —
the Evpu/Efix adjustments, the symbol's zero high byte, the terminator rule — without ever
confirming word 0. It rests on musxdom's own `StemConnection` field order matched positionally
to the six stored words. That correspondence is now corroborated across three corpora.

Getting there took three wrong readings, all of the instrument rather than the format, and they
are recorded because each is a trap a later comparison can fall into again:

  * musxdom maps `StemConnection::fontId` to the node `<font>`, not `<fontID>`. Reading the
    latter found nothing, defaulted to comparator zero and so resolved every companion
    connection to the default music font. The disagreement then looked like a systematic
    percussion-versus-base pattern affecting exactly twenty connections per document, which is
    a far more convincing shape than random noise.
  * Repairing it by string replacement changed the *first* `<fontID>` in the comparison script,
    which belongs to `FontOptions`, leaving the stem one untouched. That silently moved
    `FontOptions` face disagreements from 1,984 to 91,710 while appearing to fix stem fonts.
  * An intermediate reading concluded the companion's font table had been renumbered. It had
    not: the companion resolves comparator 5 to `Maestro Percussion` exactly as the source does.

**Layer rest offsets.** `LayerAttributes.restOffset` differs from the companion on 82 counts
across the tracked corpus, every one of them where the reader recovered nothing and the pinned
baseline supplied the value. That is a statement about the baseline rather than about a decoder,
but it means the field is unrecovered far more often than the class's coverage suggests.

## Sharing and linked parts

**Confirmed present; encoding unresolved.** Finale 27 references show:

- 301 matched documents with more than one `partDef`;
- 315 documents with part-scoped records;
- 2,188,767 part-scoped converted elements;
- 1,156,180 `shared="true"` and 1,032,587 `shared="false"` attributes.

Code `0x011a` maps exactly to `partDef`. Part scope and sharing are likely encoded in the remaining key/flag fields and/or duplicated records inside the ordinary “other” and detail pools, not a named sharing block. Finale 27 may expand shared relationships during conversion, so the counts do not prove the legacy representation is duplicated. A controlled link/unlink test is essential.

## Checksums, compression, and wrapping

- 2007+: zlib plus explicit CRC-32 and stored block length, confirmed.
- 2001–2006: PKWARE DCL streams, decoded with `blast`; explicit big-endian CRC-32 and stored block length, confirmed.
- 3.x–2000: uncompressed typed pools with no identified checksum; fixed rows and text framing confirmed.
- Coda-banner/Finale 2: separate organization.

No evidence of whole-file encryption was found. No checksum was identified in the pre-2001 eras.

## Failed or revised hypotheses

- Records do **not** start at offset zero; the common body boundary is `0x200`.
- The 2001–2006 fixed physical records are **16 bytes, not 16 words**. The remembered 12/10 payload capacities are
  bytes, so the proposed two unexplained words were an artifact of mixed units.
- The formerly “low-entropy/encoded” Finale 3.x–2000 family is **not encoded or compressed**: it contains the same
  fixed physical rows directly in four typed pools, with platform-dependent byte order.
- DCL block type `0x0012` is **not inherently a terminal marker**; it is compressed and nonempty in many documents.
  A six-byte block of the next sequential type marks the first empty/end pool.
- 2007+ principal records are **not** fixed at 16 words or 32-byte aligned.
- Treating all zlib members as the generic record pool failed; only `0x001a` and `0x001b` fit, while entries and texts use other layouts.
- Byte order is **not** derivable from `MAC` versus `WIN` alone during the 2007–2008 transition.
- XML counts are not always binary counts; Finale 27 expands or normalizes `frameSpec`, smart shapes, text definitions, details, and part-scoped data.
- The 2001–2006 high-entropy payload is **not** an unknown transform or encryption; it is standard PKWARE DCL. Tests
  that considered only zlib missed it.
