# Version Matrix

The saving product comes from the binary banner, not filenames or timestamps. Both spellings count: `Finale(R)` at
0x20 for signature-bearing files and `Finale(TM)` at offset 0 for the Coda-banner era, which is why 1.8.7, 2.0.1,
and 2.6 now appear as products in their own right rather than as one `unknown` bucket. Counts include archive
members, which is where nearly all the pre-3.x material lives. “ETF likely” means the file should be
tested with the earliest compatible application available; it is not a guarantee. Fifteen local ETF exports are now
available across private and tracked evidence, including two saves of the same Finale 2000 template by Finale 2000
and Finale 2005, the exact Finale 2000 `tremolos` pair, plus controlled Finale 2002–2005 pairs. Three targeted Finale
27 conversions now cover the ETF-backed 1.8.7, 2.0.1, and 2.6 sources.

| Saving product | Files | Structural family | Header/body characteristic | MUSX counterparts | ETF likely | Notes |
|---|---:|---|---|---:|---|---|
| Finale 1.8.7 | 19 | early fixed-row/indexed | Coda banner at offset 0; no Enigma signature; 0x60-0x200 zero apart from `01 03` at 0x80 | 0 | three ETF exports analyzed across the era | archive-only in this corpus; indexes/boundaries unresolved |
| Finale 2.0.1 | 33 | early fixed-row/indexed | as above | 0 | as above | archive-only in this corpus |
| Finale 2.6 | 177 | early fixed-row/indexed | as above | 58 exact | as above | 63 loose, the rest archive-derived; Finale 27 opens selected files after adding `.mus`; detail tags and all 11,089 entry rows correlate exactly; no Finale 1.0 yet |
| unknown | 3 | unclassified | no recognizable banner in either spelling; includes an AppleDouble artifact | 0 | uncertain | what is left once both banner spellings are read |
| Finale 3.0 | 3 | uncompressed fixed-row | four little-endian typed pools; all three are Windows-origin | 3 | yes | 3/3 framed; earliest explicit product in direct corpus |
| Finale 3.2 | 3 | uncompressed fixed-row | four big-endian typed pools | 3 | yes | 3/3 framed |
| Finale 3.5 | 2 | uncompressed fixed-row | four big-endian typed pools | 2 | yes | 2/2 framed |
| Finale 3.7 | 20 | uncompressed fixed-row | four big-endian typed pools | 20 | yes | 20/20 framed; incidental zlib signature is not a wrapper |
| Finale 97 | 70 | uncompressed fixed-row | four big-endian typed pools in 69/70 | 68 | yes | one file needs integrity/classification work |
| Finale 2000 | 92 | uncompressed fixed-row | four typed pools; 91 big-endian, one Windows-origin little-endian | 91 | analyzed | exact `tremolos` ETF pair; same fixed rows as 2001–06; template resave evidence remains |
| Finale 2001 | 4 | DCL-compressed legacy | big-endian typed/length/CRC blocks; 3/4 framed files, all 9 members DCL/CRC-valid | 4 | yes | major codec boundary; resolved direct files retain 16-byte other/detail rows |
| Finale 2002 | 48 | DCL-compressed legacy | same framing; 47/48 files, all 181 members DCL/CRC-valid | 47 | yes | controlled pairs prove 16-byte other/detail and 38-byte entry rows; `IS` uses three rows |
| Finale 2003 | 168 | DCL-compressed legacy | same framing; 168/168 files, all 659 members DCL/CRC-valid | 163 | yes | fixed physical rows persist; controlled `IS` expands to six rows |
| Finale 2004 | 7 | DCL-compressed legacy | same framing; 7/7 files, all 25 members DCL/CRC-valid | 7 | yes | controlled MUS/ETF pairs available |
| Finale 2004b | 13 | DCL-compressed legacy | 6/13 framed files, all 23 recognized members DCL/CRC-valid | 6 | yes | seven unframed variants need classification |
| Finale 2005 | 120 | DCL-compressed legacy | 116/120 framed files, all 459 members DCL/CRC-valid | 118 | analyzed | fixed rows persist; controlled `MS` and `SS` expand from two physical rows to three |
| Finale 2006 | 66 | DCL-compressed legacy | 63/66 framed files, all 247 members DCL/CRC-valid; empty `0x0013` after nonempty `0x0012` | 63 | unlikely | fixed rows persist; three unframed files need classification |
| Finale 2007 | 108 | typed zlib transition | four CRC blocks; 81 big-endian, 27 little-endian | 104 | no | first solved wrapper era; mixed serialization |
| Finale 2008 | 182 | typed zlib transition | 180 little-endian, 2 big-endian | 180 | no | sharing/linked parts common |
| Finale 2009 | 5 | typed zlib stable | four blocks, little-endian | 5 | no | small sample |
| Finale 2010 | 4 | typed zlib stable | four blocks, little-endian | 4 | no | small sample |
| Finale 2012 / File Converter | 248 | typed zlib stable | four principal blocks, little-endian in all sampled wrappers | 248 | no | banner count combines 238 Finale 2012 and 10 File Converter files |

## Internal and creation versions

The header stores separate creator and last-saver version tuples. Finale 27 conversion exposes the creator tuple in EnigmaXml. Across 1,189 decoded exports, common creator application versions include 5.0.2.1 (138), 17.0.3.13 (123), 13.0.2.1 (118), 3.8.2.1 (103), and 12.0.1.3 (100); 173 exports have no creator application version. This demonstrates that “created by” and “last saved by” are distinct and that many documents were upgraded across releases.

The exact meaning of all tuple members (Enigma version, application version, file version, status/build) is already represented in Finale 27 EnigmaXml and should be used as the semantic reference when the binary packing is decoded. The banner remains the reliable saving-product label for this study.

### Decoded version packing

**Confirmed** against all 1,163 signature-bearing corpus files. Each version is a 32-bit value stored in the file's own byte order and packed as:

| Bits | Field |
|---|---|
| 31-24 | major |
| 23-20 | minor |
| 19-16 | maintenance |
| 15-8 | development status code |
| 7-0 | build |

Three such values sit in each file-info block: the Enigma version at the tuple start, the application version at tuple+12, and the file version at tuple+16, with the application and platform strings between them.

The decoding reproduces this document's own aggregate creator-version figures exactly. Finale 97 stores application version `0x03820401`, which decodes to 3.8.2 build 1, matching the reported `3.8.2.1`; Finale 2000 stores `0x05020401` for `5.0.2.1`; a Finale 2012 file stores `0x0d040311` little-endian for `17.0.3.13`. The same packing places the minor version at bits 23-20 as the running application reports it to plug-ins, so a file version and a runtime version are directly comparable.

Byte order tracks the container. Finale 2007 splits 81 big-endian against 27 little-endian and Finale 2008 splits 180 little-endian against 2 big-endian, matching the wrapper counts in the table above exactly. Reading the tuple in file order regardless of byte order reports a little-endian file's build as its major version.

### Saving product to internal major version

**Confirmed** from the last-saver Enigma tuple across the corpus. Major alone does not order Finale's history: the entire 3.x line and Finale 97 share major 3.

| Saving product | Internal | Saving product | Internal |
|---|---|---|---|
| 3.0 | 3.0 | 2004 / 2004b | 9.0 |
| 3.2 | 3.2 | 2005 | 10.0 |
| 3.5 | 3.5 | 2006 | 11.x |
| 3.7 | 3.7 | 2007 | 12.x |
| 97 | 3.8 | 2008 | 13.x |
| 2000 | 5.0 | 2009 | 14.x |
| 2001 | 6.0 | 2010 | 15.0 |
| 2002 | 7.0 | 2012 | 17.0 |
| 2003 | 8.0 | | |

Finale 98 is absent from the corpus and is presumed to be major 4; Finale 2011 is likewise absent and would be major 16. Neither is verified.

### Back-saved files

**Confirmed** on the tracked fixture `tests/evidence/F2012/F2012-upstem-flags.mus`.

Finale 2014 through Finale 27 can write a musx document out as a Finale 2012 MUS file. Such a
file is a genuine Finale 2012 document by format, and its banner says so, commonly reading
`Finale(R) 2012 File Converter` rather than plain `Finale(R) 2012`. The saving-product table
above already counts those separately: of the 248 files in the Finale 2012 row, 10 carry the
File Converter banner.

The two header tuples then record different things, which is the useful part:

| Tuple | Records | Example |
|---|---|---|
| creator | the application that authored the document | internal 18.0, Finale 2014 |
| last saver | the format actually written | internal 17.0, Finale 2012 |

This is why the reader gates on the last-saver tuple. On a back-save it yields the version that
governs the layout on disk rather than the version of the program that happened to produce it.
It also establishes internal major 18 for Finale 2014, extending the mapping above.

A consequence for reading corpus statistics: a saving-product count is a count of *formats*, not
of authoring applications. Any file in the Finale 2012 row may have been written by any release
from Finale 2012 through Finale 27.

### Controlled fixture versions

Decoded from `tests/evidence`, and asserted by the reader's tests.

| Fixture | Saving product | Enigma version | Application version |
|---|---|---|---|
| F2002 | 2002 | 7.0.1 build 2 | 7.0.1 build 1 |
| F2003 | 2003 | 8.0.0 build 5 | 8.0.1 build 1 |
| F2004 | 2004b | 9.0.0 build 58 | 9.0.3 build 1 |
| F2005 | 2005 | 10.0.0 build 10 | 10.0.2 build 1 |

The creator and last-saver blocks agree in every fixture.

## Compatibility families

Evidence currently supports at least four parsers/codecs, not one parser per Finale release:

1. archive-derived explicit Finale 1.8.7–2.6 fixed-row/indexed family;
2. pre-banner/Finale 2;
3. Finale 3.x–2000 uncompressed typed pools with platform byte order;
4. Finale 2001–2006 typed PKWARE DCL with CRC-32;
5. Finale 2007–2012 typed zlib, with two record serialization variants around 2007–2008.

The explicit 1.8.7–2.6 family shares Finale 3.0's logical record model; the direct apparent-Finale-2/unknown family still needs classification. Exact minimal pairs are the shortest path to its index and boundary rules.
