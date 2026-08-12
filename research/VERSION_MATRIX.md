# Version Matrix

The saving product comes from the binary banner, not filenames or timestamps. All three spellings count: `Finale(R)`
at 0x20 for signature-bearing files, `Finale(TM)` at offset 0 for the Coda-banner era, and `Finale` followed by a
MacRoman trademark sign (0xAA) for Finale 1.0.0, whose banner ends `ENIGA Structures` (sic) rather than a copyright
notice. Reading all three is why 1.0.0, 1.8.7, 2.0.1, and 2.6 appear as products in their own right rather than as
one `unknown` bucket. Counts include archive members, which is where nearly all the pre-3.x material lives.
“ETF likely” means the file should be tested with the earliest compatible application available; it is not a
guarantee. Fifteen local ETF exports are now available across private and tracked evidence, including two saves of
the same Finale 2000 template by Finale 2000 and Finale 2005, the exact Finale 2000 `tremolos` pair, plus controlled
Finale 2002–2005 pairs. Three targeted Finale 27 conversions now cover the ETF-backed 1.8.7, 2.0.1, and 2.6 sources.

Counts are given per survey, since no one corpus sees the whole format. `rpatters1-main` is documents and is the
corpus every structural claim below rests on; `rpatters1-installs` is Finale application installations, which supply
version coverage rather than authored music and have no MUSX counterparts at all. Both are registered in
[`data/surveys.csv`](data/surveys.csv). A product absent from one and present in the other is coverage, not
disagreement.

**A banner is a marketing label, not a version.** Two products in this table are alternate names for a release that
shipped under a different name, established below in *Renamed releases*: `Finale 3.8` is `Finale 97`, and `Finale 99`
is `Finale 2000`. They are listed separately because the banner really does differ, not because the format does.

| Saving product | main | installs | exact MUSX (main) | Structural family | Header/body characteristic | ETF likely | Notes |
|---|---:|---:|---:|---|---|---|---|
| Finale 1.0.0 | 0 | 22 | 0 | early fixed-row/indexed | `Finale` + 0xAA banner at offset 0; no Enigma signature; median body entropy 2.80, no compressed member | uncertain | first Finale 1.0 material in any survey; 14 samples, 6 templates, 2 tool demos; indexes/boundaries unresolved |
| Finale 1.8.7 | 19 | 0 | 0 | early fixed-row/indexed | Coda banner at offset 0; no Enigma signature; 0x60-0x200 zero apart from a format word at 0x80, `01 03` here and `01 00` for 1.0.0 | three ETF exports analyzed across the era | archive-only in this corpus; indexes/boundaries unresolved |
| Finale 2.0.1 | 33 | 0 | 0 | early fixed-row/indexed | as above | as above | archive-only in this corpus |
| Finale 2.6 | 177 | 1 | 53 | early fixed-row/indexed | as above | as above | 63 loose in main, the rest archive-derived; Finale 27 opens selected files after adding `.mus`; detail tags and all 11,089 entry rows correlate exactly |
| Finale PC 1.0+ | 0 | 24 | 0 | early fixed-row/indexed | Coda banner at offset 0, `Finale(TM)` spelling with product `PC 1.0+`; the leading `PC` token names the platform and the rest is a version that does not parse; **little-endian**, unlike every Mac document of the era | uncertain | the only Windows-origin Coda-banner material in any survey; 8 templates and 16 tutorial documents from the Finale 2.2 for Windows install disks; pool prologue and record model match the Mac era exactly, only the byte order differs; the reader opens none of them, see P2.6 |
| unknown | 2 | 2 | 0 | unclassified | no recognizable banner in any of the three spellings | uncertain | what is left once all three banner spellings are read; the AppleDouble artifact previously counted here is no longer inventoried |
| Finale 3.0 | 18 | 64 | 3 | uncompressed fixed-row | four typed pools | yes | earliest explicit product in the direct corpus |
| Finale 3.2 | 18 | 4 | 3 | uncompressed fixed-row | four big-endian typed pools | yes | |
| Finale 3.5 | 26 | 8 | 2 | uncompressed fixed-row | four big-endian typed pools | yes | |
| Finale 3.7 | 56 | 71 | 20 | uncompressed fixed-row | four big-endian typed pools | yes | incidental zlib signature is not a wrapper |
| Finale 3.8 | 0 | 11 | 0 | uncompressed fixed-row | median body entropy 3.54, no compressed member | yes | **not a distinct release**: same Enigma version as Finale 97, see *Renamed releases* |
| Finale 97 | 238 | 41 | 68 | uncompressed fixed-row | four big-endian typed pools | yes | |
| Finale 98 | 0 | 56 | 0 | uncompressed fixed-row | median body entropy 3.68, no compressed member | yes | first Finale 98 material in any survey; application major 4, Enigma version mixed 3.8/4.0, see below |
| Finale 99 | 0 | 4 | 0 | uncompressed fixed-row | median body entropy 2.21, no compressed member | yes | **not a distinct release**: same Enigma 5.0 line as Finale 2000, see *Renamed releases* |
| Finale 2000 | 449 | 68 | 91 | uncompressed fixed-row | four typed pools; predominantly big-endian, Windows-origin files little-endian | analyzed | exact `tremolos` ETF pair; same fixed rows as 2001–06; template resave evidence remains |
| Finale 2001 | 24 | 967 | 4 | DCL-compressed legacy | big-endian typed/length/CRC blocks; 3/4 framed files, all 9 members DCL/CRC-valid | yes | major codec boundary; resolved direct files retain 16-byte other/detail rows |
| Finale 2002 | 201 | 5399 | 47 | DCL-compressed legacy | same framing; 47/48 files, all 181 members DCL/CRC-valid | yes | controlled pairs prove 16-byte other/detail and 38-byte entry rows; `IS` uses three rows |
| Finale 2003 | 513 | 334 | 163 | DCL-compressed legacy | same framing; 168/168 files, all 659 members DCL/CRC-valid | yes | fixed physical rows persist; controlled `IS` expands to six rows |
| Finale 2004 | 43 | 245 | 7 | DCL-compressed legacy | same framing; 7/7 files, all 25 members DCL/CRC-valid | yes | controlled MUS/ETF pairs available |
| Finale 2004b | 58 | 1 | 6 | DCL-compressed legacy | 6/13 framed files, all 23 recognized members DCL/CRC-valid | yes | seven unframed variants need classification |
| Finale 2005 | 338 | 336 | 118 | DCL-compressed legacy | 116/120 framed files, all 459 members DCL/CRC-valid | analyzed | fixed rows persist; controlled `MS` and `SS` expand from two physical rows to three |
| Finale 2006 | 129 | 823 | 63 | DCL-compressed legacy | 63/66 framed files, all 247 members DCL/CRC-valid; empty `0x0013` after nonempty `0x0012` | unlikely | fixed rows persist; three unframed files need classification |
| Finale 2007 | 353 | 468 | 104 | typed zlib transition | four CRC blocks; both byte orders occur | no | first solved wrapper era; mixed serialization |
| Finale 2008 | 298 | 479 | 180 | typed zlib transition | predominantly little-endian | no | sharing/linked parts common |
| Finale 2009 | 22 | 189 | 5 | typed zlib stable | four blocks, little-endian | no | |
| Finale 2010 | 4 | 467 | 4 | typed zlib stable | four blocks, little-endian | no | |
| Finale 2012 / File Converter | 708 | 0 | 248 | typed zlib stable | four principal blocks, little-endian in all sampled wrappers | no | banner count combines Finale 2012 and File Converter files |

The DCL and framing figures in the two right-hand columns predate the current run and describe the sample they were
measured on; the `Files` counts have been refreshed and the framing counts have not. Re-running `dcl_probe.py`
against both surveys is outstanding work — it needs a `blast`-compatible executable, which the runs behind this
table did not have.

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
| 2003 | 8.0 | 98 | 4.0 (application) |

Finale 98 is no longer absent. **Strong.** 56 Finale-98 files in `rpatters1-installs` decode an application version of
4.0.x in 41 of the 44 macOS-origin files that yield a tuple at the expected offset, which settles the major-4
presumption for the *application*. The Enigma version is not uniformly 4: 41 files carry Enigma 4.0.0 build 10 and 8
carry Enigma 3.8.0 build 7, the same Enigma version Finale 97 writes. So Finale 98 could write the 3.8 format, and a
Finale-98 banner does not by itself imply a major-4 file layout. Four Windows-origin files decode nonsense at the
same offset and remain unclassified.

Finale 2011 is still absent from every survey and would be major 16. Not verified.

### Renamed releases

**Strong.** Two banners in the table above name a release that shipped under a different name. Coda replaced
version-numbered product names with year-numbered ones, and files written before or around the rename keep the older
banner while carrying the shipping release's internal version.

| Banner | Files | Enigma version | Application version | Shipped as |
|---|---:|---|---|---|
| `Finale(R) 3.8` | 11 | 3.8.0 build 7 | 3.8.0 build 13 | Finale 97 |
| `Finale(R) 97` | 279 | 3.8.0 build 7 | 3.8.0 build 3, 3.8.2 build 1, 3.8.2 build 6 | — |
| `Finale(R) 99` | 4 | 5.0.0 build 15 | 5.0.0 build 1 | Finale 2000 |
| `Finale(R) 2000` | 517 | 5.0.0 build 5, 5.0.1 build 9, 5.0.2 build 5 | 5.0.0–5.0.4 | — |

The Enigma version is the discriminator. Every Finale 97 file that yields a tuple carries Enigma 3.8.0 build 7, and
so does every Finale 3.8 file — the same value, not merely the same major. Finale 99 and Finale 2000 likewise share
the 5.0 line. Copyright years agree: the 3.8 and 97 banners both read 1987-1997, and 99 reads 1987-1999 against
2000's 1987-1999/2000.

The consequence for a decoder is that these four banners select two layouts, not four, and that a product string is
not a safe key for a layout decision on its own. It also means a `Finale 99` count is a count of files written under
a pre-release name, not evidence of a release that ever shipped.

Evidence: 11 `Finale 3.8`, 4 `Finale 99`, 41 `Finale 97` and 68 `Finale 2000` files in `rpatters1-installs`, plus
238 `Finale 97` and 449 `Finale 2000` files in `rpatters1-main`. No fixture in `tests/evidence/` demonstrates either
rename yet, which is what holds this at `strong` rather than `confirmed`.

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

Evidence currently supports at least five parsers/codecs, not one parser per Finale release:

1. explicit Finale 1.0.0–2.6 fixed-row/indexed family;
2. pre-banner/Finale 2;
3. Finale 3.x–2000 uncompressed typed pools with platform byte order, which by the *Renamed releases* section above
   covers the `3.8`, `97`, `98`, `99` and `2000` banners between them;
4. Finale 2001–2006 typed PKWARE DCL with CRC-32;
5. Finale 2007–2012 typed zlib, with two record serialization variants around 2007–2008.

The explicit 1.0.0–2.6 family shares Finale 3.0's *physical* record model — 16-byte other and detail rows, two-character tags, comparator `65534` for globals — but not its option vocabulary. Finale 3.0 renumbered what those globals hold, and the two must not be conflated when promoting a mapping.

The clef work is the clearest measurement of that discontinuity. Of eight `ClefOptions` scalar locations that hold across Finale 3.0 through 2012, **three mean something else entirely before Finale 3.0**: selector `27` word 1 and selector `39` word 4 are font sizes in the Coda era, and selector `38` word 5 disagrees with the companion on every Coda file with a non-default value. Within the clef record itself, word 1 is populated through 2.6 and abandoned from 3.0. Selector `24` is not the default-font array in the Coda era, confirmed independently from Finale 1.0.0 and 2.6.3, where it holds one row of unrelated values. It **is** the default-font array by Finale 97 at the latest, with the same two-tuples-per-incidence packing the DCL era uses: the Finale 97 and Finale 2000 fixtures both agree tuple for tuple with their exact companions.

Treat Finale 3.0 as a redesign boundary rather than an increment: a location verified from 3.0 onward carries no weight before it, and each earlier era needs its own verification. Extrapolating backward is how three plausible-looking wrong numbers reached a draft. The direct apparent-Finale-2/unknown family still needs classification, and exact minimal pairs remain the shortest path to its index and boundary rules.

Family 1 previously read "archive-derived 1.8.7–2.6", which was true of the corpora available at the time. It is not
any more: `rpatters1-installs` holds 22 loose Finale 1.0.0 files, so the earliest family now has evidence that is
neither archive-derived nor confined to 1.8.7 and later.
