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

### The 2007-2012 record encoding

**Confirmed** for the font record against Finale 27's own conversion of the same document, and
**strong** for the general shape, which is inferred from that one record type.

The 2007 serialization abandons the fixed 16-byte row. Records in block `0x001a` are
variable-length and self-describing, little-endian:

| Field | Width | Meaning |
|---|---|---|
| class id | 2 | numeric identifier standing in for what EnigmaXML names as an element |
| cmper | 2 | comparator, as in every earlier era |
| incidence | 2 | incidence, as in every earlier era |
| length | 4 | payload size |
| payload | length | per class |
| padding | 4 | zero in every observed record |

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

**Confirmed.** In the zlib era a stored `0x0013` block holds embedded graphics. Nineteen
distinct corpus files carry one, in three shapes:

| Payload signature | Files |
|---|---:|
| `C5 D0 D3 C6`, the binary EPSF header | 12 |
| `%!PS-Adobe-3.0 EPSF-3.0` or `%!PS-Adobe-2.0 EPSF-1.2` | 6 |
| `89 50 4E 47 0D 0A 1A 0A`, the PNG signature | 1 |

The stored payload is itself framed: a nested `type` and `size` header precedes the image
signature — types `0x000f` and `0x0043` are both observed — and a block may hold more than one
graphic. That inner framing is only partly read and is **open**; nothing depends on it yet,
because the bytes are preserved and reported rather than interpreted. See
[PRODUCTION_READINESS.md](PRODUCTION_READINESS.md#p31-embedded-graphics-are-preserved-but-not-imported).

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

In the big-endian `0x001a` variant, the first five words behave as:

1. numeric record type;
2. primary key/`cmper` or `0xfffe` option sentinel;
3. secondary key/`inci`/part dimension candidate;
4. additional key/flags/part dimension candidate;
5. payload byte count.

Fields 2–4 are not fully named. For ordinary “other” records, field 2 tracks the `cmper` sequence and fields 3–4 are often zero. The `0xfffe` records at the front of `0x001a` are singletons with codes beginning at `0x000f` and are strongly interpreted as options. This shows that options are not wholly free-form in 2007+, although their code-to-option and field mappings remain open.

For details, codes around `0x03ef`–`0x0455` correlate with EnigmaXml detail names. The multiple key fields are consistent with `entnum` plus `inci` and possibly part/sharing scope.

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
already carry bit 2 set, so reading the bit gives the correct answer for them. Gating on the epoch avoids a version
test that the three Finale 3.0 files, which recover a major version of 15, would fail anyway.

## Text and variable-length data

**Strong.** Block `0x0017` contains decoded strings, font names, XML printer settings, Enigma text commands such as `^font(...)`, and binary control data. Its representation is not the generic record frame. Header offsets `0x0b0`–`0x1ff` also contain title/composer/copyright/file-info strings when present.

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
