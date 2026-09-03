# Container and framing investigations

**Covers:** Framing, entropy, pool identification, DCL identification, and the uncompressed-era validation runs.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-05 — Creator versus saver header

- **Question:** Can creation and last save be distinguished?
- **Method:** interpret bytes at `0x66` and `0x8c` as `tm_year`, month, day; compare to filesystem dates and Finale 27 EnigmaXml header.
- **Observation:** `0x8c` date matches filesystem day in 98.6% of valid cases. `0x66` precedes/equal `0x8c` in all 1,039 valid pairs. Adjacent creator/saver version/platform tuples decode semantically in the Finale 27 export.
- **Conclusion:** The header distinguishes creation metadata, last-save metadata, internal Enigma/application/file versions, and platform. Exact binary tuple packing remains partly open.

## 2026-08-05 — Body clusters and entropy

- **Question:** Are there format eras?
- **Method:** entropy of up to 16 KiB after `0x200`, prefix clustering, zlib validation.
- **Observation:** 3.x–2000 is low entropy (roughly 3.1–4.1); 2001–2006 abruptly becomes high entropy (usually 7.3–7.8) without zlib; 2007+ has four valid zlib members; pre-banner files differ again.
- **Conclusion:** At least four codec/layout strategies are needed. The 2000/2001 boundary is high value for ETF comparison.

## 2026-08-05 — Typed zlib wrapper

- **Question:** How are 2007+ streams delimited and checked?
- **Method:** parse both byte orders from `0x200`; validate stored size, zlib EOF, and CRC-32.
- **Observation:** header is type (2), total length (4), CRC-32 (4). Principal types repeat as `0x1a`, `0x1b`, `0x16`, `0x17`; terminal six-byte `0x13`/`0x1d` markers follow. 2007 and 2008 contain both endian variants.
- **Conclusion:** Wrapper traversal and corruption detection are solved for this era.
- **Failed hypothesis:** platform string alone selects byte order. Transition-era Mac files use both.

## 2026-08-05 — 16-word record test

- **Question:** Are logical records fixed at 16 words, and what are the two unexplained words?
- **Method:** decompress block `0x1a`; test candidate strides and then a length-driven frame. Require exact full-member consumption and four zero trailer bytes on every record.
- **Observation:** the fixed strides drift. A variable frame succeeds on more than 1.59 million records. Common 12-byte records serialize to 13 words, not 16; large payloads vary widely. Every accepted record ends in two zero words.
- **Conclusion:** Fixed 16-word records are disproved for framed 2007+ pools. The two unexplained words are a stable zero trailer/reserved terminator in this era.
- **Open:** whether this represents padding, reserved metadata, or a terminator; whether pre-2007 decompressed records differ.

## 2026-08-05 — Pool identification

- **Question:** What do the four block types contain?
- **Method:** inspect decoded strings/structure and compare Finale 27 XML pool counts.
- **Observation:** `0x1a` accepts other/option frame and record counts; `0x1b` accepts detail frame; `0x16` scales with entries but fails generic framing; `0x17` exposes text/font/Enigma commands.
- **Conclusion:** block categories are strongly identified. Entry/text layouts remain separate work.
- **Failed hypothesis:** one generic record parser covers all four blocks.

## 2026-08-08 — PKWARE DCL identification and corpus-wide validation

- **Question:** Are the Finale 2001–2006 high-entropy members PKWARE Data Compression Library (DCL) streams, and can Mark Adler's open-source `blast` implementation decode them reliably?
- **Method:** Download `blast.c`, `blast.h`, and the small test driver from the official `madler/zlib` repository; compile them outside the repository; walk big-endian typed/total-length blocks from `0x200`; treat bytes 6–9 of each nonempty block as a big-endian checksum and bytes 10 onward as the candidate DCL stream; run `blast`; compute CRC-32 over the decoded output. Test every structurally recognized Finale 2001–2006 file resolved through the private corpus mapping. No proprietary MakeMusic material was accessed.
- **Observation:** All 1,603 candidate compressed members encountered returned `blast` success and matched the stored CRC-32. Separately, 410 files traversed cleanly as complete typed-block sequences. Counts by saving product were: 2001, 3/4 completely framed files and 9 decoded streams; 2002, 47/48 and 181; 2003, 168/168 and 659; 2004, 7/7 and 25; 2004b, 6/13 and 23; 2005, 116/120 and 459; 2006, 63/66 and 247. Some incompletely framed files contributed valid leading streams before the probe reached an unknown or malformed outer block; they were framing/classification failures, not DCL failures.
- **Observation:** Every stream begins with `00 04` or `01 04`, representing DCL literal modes 0/1 and dictionary parameter 4. There were 1,202 `00 04` streams and 401 `01 04` streams. The controlled F2002 baseline `0x000f` stream expands from 1,966 to 8,000 bytes; both its stored and computed CRC-32 are `0xcb68f112`. Its output begins with orderly rows containing `&f` identifiers rather than high-entropy bytes.
- **Conclusion:** **Confirmed.** PKWARE DCL is the Finale 2001–2006 payload codec, and the preceding four bytes are the CRC-32 of the decompressed member. This supersedes the earlier “unknown high-entropy codec” conclusion. The major remaining task for this era is decoded record/pool interpretation, not decompression.
- **Terminology caveat:** This is the format used by the PKWARE DCL `implode()` function, which `blast.h` explicitly distinguishes from PKZIP's incompatible compression method also named “implode.” `blast` is in zlib's `contrib` tree, not the core zlib API.
- **References:** [official `blast` directory](https://github.com/madler/zlib/tree/master/contrib/blast), [`blast.h`](https://github.com/madler/zlib/blob/master/contrib/blast/blast.h).
- **Follow-up:** Add a deterministic, read-only DCL block probe to the analysis scripts; catalog decoded `0x000f`–`0x0012` framing; correlate the F2002–F2005 controlled pairs and the Finale 2005 nested-tuplet ETF with decoded offsets and values.

## 2026-08-08 — Public Finale 2000 PDK consultation and fixed-row validation

- **Boundary change:** The project replaced its strict clean-room rule with the public-source provenance policy in the README. Public historical PDK material may be consulted for interoperability facts, but no PDK source is stored in this repository; derived claims remain labeled until independently checked against the corpus.
- **Public source:** GRAME GUIDOLib commit `9f74ba9b3e287f240bbd454c2259fc3f7737c6ad`, `platforms/win32/finale-plugin/`. Its public README explicitly calls the included material the Finale 2000 PDK. Accessed 2026-08-08. The data-definition, entry-detail, primitive-type, and data-API headers were consulted; plug-in implementation source was not needed.
- **Question:** Does the PDK explain the decoded 2001–2006 row organization, the proposed 16-word records, and the two allegedly missing words?
- **PDK-derived observation:** A 32-bit extended tag combines a 16-bit storage class with a two-character low-word tag. Other IDs carry one 16-bit comparator, details carry two, and entries carry a 32-bit entry number. Larger logical structures are stored across successive physical incidences; strings, arrays, and special structures use separate storage classes.
- **Controlled-pair method:** Decode all four F2002–F2005 baseline/change pairs with `blast`; split `0x000f` and `0x0010` on 16-byte boundaries and `0x0011` on 38-byte boundaries; count the corresponding ETF section lines.
- **Controlled-pair observation:** All eight files match exactly: 4,552 decoded 16-byte other rows equal 4,552 ETF `others` lines; 272 decoded 16-byte detail rows equal 272 ETF `details` lines; 24 decoded 38-byte entry rows equal 24 ETF `eE` records. In each ordinary other row the serialized fields are one comparator, a two-byte tag, and 12 payload bytes. In each detail row they are two comparators, a two-byte tag, and 10 payload bytes.
- **Corpus method:** Run a direct-source fixed-row pass over the 375 resolved Finale 2001–2006 files, then use `scripts/dcl_probe.py` over every source resolvable from the private location mapping (including the broader archive-derived survey), using a compiled `blast`-compatible executable and reporting aggregate counts only.
- **Corpus observation:** All 375 files in the direct-source pass have `0x000f` and `0x0010` decoded lengths divisible by 16, totaling 4,601,857 other rows and 1,574,280 detail rows. All 348 nonempty `0x0011` members are divisible by 38, totaling 837,086 entry rows. The broader probe finds 410 completely framed files; its totals differ because it covers archive-derived locations as well. No row-size counterexample occurred from Finale 2001 through Finale 2006.
- **Conclusion:** The historical fixed record is **16 bytes (eight words), not 16 words**. The remembered 12/10 payload capacities are bytes. Therefore the two unexplained words do not exist in 2001–2006 ordinary other/detail rows; the accounting was off by a factor of two. Incident is implicit in ordered repeated rows rather than serialized as an additional word.
- **Version observation:** Stable tags survive logical structure growth. In the controlled ETF baselines, `IS` uses three rows in Finale 2002 and six from Finale 2003; `MS` and `SS` use two rows through Finale 2004 and three in Finale 2005. `Iu` and `PS` remain two. This supports a stable physical-row parser plus versioned tag-specific assemblers.
- **Failed hypothesis:** `0x0012` is a terminal marker. Many corpus files contain a nonempty, DCL-compressed `0x0012`; empty six-byte blocks instead mark the first absent/end pool. In 58 Finale 2006 files a nonempty `0x0012` is followed by empty `0x0013`. Text/lyrics are the leading interpretation of nonempty `0x0012`, but this is not yet field-verified.
- **Follow-up:** Decode exact raw fields for `MS`, `IS`, `Iu`, `PS`, `SS`, `GF`, and `eE`; compare PDK-era logical sizes with each saving version; then map the resulting values directly into existing musxdom classes.

## 2026-08-08 — Finale 2000 exact ETF pair and uncompressed-era validation

- **Question:** Does the Finale 2000 low-entropy body contain the same fixed rows as Finale 2001–2006, and can an exact MUS/ETF pair reveal its container?
- **Files examined:** `mus-3a8b724cf3adba80` (20,149 bytes) and its newly supplied local-only ETF, 28,718 bytes, SHA-256 `c02e859d6026de960a44ea07bd0d3154e07e7f85d690cc5eaf5a84b623d3149d`; then all direct Finale 3.0, 3.2, 3.5, 3.7, 97, and 2000 files resolved through the private mapping.
- **Method:** Walk two-byte type/four-byte total-length blocks from `0x200` in both byte orders; require exact EOF consumption and type sequence `1,2,3,4`; test pool payloads modulo 16/16/38; compare ETF section counts and tag order; normalize only ETF separators between text records and compare raw bytes. The reusable aggregate probe is `scripts/uncompressed_probe.py` and never emits source paths.
- **Pair observation:** The binary blocks begin at `0x0200`, `0x4736`, `0x4b3c`, and `0x4c72`, with types `1,2,3,4` and total sizes `0x4536`, `0x0406`, `0x0136`, and `0x0243`. Their payloads contain 1,107 16-byte other rows, 64 16-byte detail rows, eight 38-byte entries, and 573 text bytes, exactly matching the ETF section counts. All ordinary tags are in identical order; ordinary values such as `IS`, `Iu`, `MS`, `PS`, and `SS`, detail values such as `CN`, `GF`, and `TP`, and selected `eE` fields are directly recognizable. After removing ETF record separators, the complete text section is byte-identical to block 4.
- **Corpus observation:** 189/190 direct Finale 3.x–2000 files traverse exactly as four pools. All 1,552,762 type-1 rows and 770,960 type-2 rows divide by 16, and all 394,984 type-3 rows divide by 38. The recognized population is 185 big-endian files and four little-endian files. All four little-endian files are Windows-origin: the three Finale 3.0 samples and one Finale 2000 sample. One Finale 97 file remains unrecognized.
- **Conclusion:** **Confirmed.** Finale 3.x–2000 stores the same fixed physical other/detail/entry rows uncompressed. Finale 2001 primarily changes the pool identifiers and adds CRC-protected DCL compression. The previous “low-entropy codec/table encoding” hypothesis is disproved for the recognized banner-era files. Byte order is platform-sensitive and should be validated from the block headers.
- **Follow-up:** Parse all type-1/type-2 rows into versioned PDK-era logical structures; investigate the compact binary encoding of ETF pseudo-tags `#v*`, `#c*`, and `#s*`; classify the remaining Finale 97 file; then test whether the pre-banner Finale 1.x–2.x families use an earlier form of the same four pools.
