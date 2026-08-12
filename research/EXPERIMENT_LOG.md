# Experiment Log

## 2026-08-05 — Corpus enumeration

- **Question:** What evidence is actually present?
- **Method:** recursive `find`, then deterministic `scripts/inventory.py`; full-file SHA-256 for legacy sources and exact matched exports.
- **Observation:** 1,218 `.mus`, 2,042 `.fin27.musx`, 1,189 exact adjacent matches, 29 unmatched sources. Legacy data totals 165,758,809 bytes; matched exports total 178,990,189 bytes.
- **Conclusion:** The corpus is much larger than an initial shallow listing suggested and supports statistical comparison. The assertion that every source has an export is nearly, but not completely, true.
- **Follow-up:** preserve full rows in `private/generated/corpus_inventory.csv` and sanitized findings in `CORPUS_INVENTORY.md`.

## 2026-08-05 — Explicit saving version

- **Question:** Is the saving Finale version encoded?
- **Method:** hex dump bytes `0x000–0x080` and aggregate strings from bytes `0x020–0x060`.
- **Observation:** Banner-era files say `Finale(R) <product> Copyright...`; products range 3.0–2012. Counts are in `inventory_summary.json`.
- **Conclusion:** Saving-product identification is direct for 1,163 files. Filename/timestamp classification is unnecessary there.
- **Failed hypothesis:** version classification would require record-set heuristics. It does not for banner-era files.

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

## 2026-08-05 — Finale 27 semantic census

- **Question:** Can converted XML give record identities at corpus scale?
- **Method:** decode all 1,189 matched `score.dat` members in memory with `scripts/musx_semantics.py`; count direct pool children. Correlate numeric binary code vectors with XML tag vectors over 375 other-framed and 324 detail-framed files.
- **Observation:** dozens of exact/near-exact mappings. Examples: `0x0086/durAllot` exact 375/375, `0x011a/partDef` exact 375/375, `0x03f3/baselinesExprAboveStaff` exact 324/324. Others show conversion expansion but correlations above 0.99.
- **Conclusion:** type mapping is practical without proprietary code. Count collisions remain possible; field-level value correlation or ETF should confirm names.
- **Failed hypothesis:** a Finale 27 XML object count always equals legacy binary count. Counterexamples include `frameSpec`, smart shapes, text definitions, and part-scoped data.

## 2026-08-05 — Sharing census

- **Question:** Is sharing visible and localized?
- **Method:** count `partDef`, `part`, and `shared` in decoded Finale 27 XML.
- **Observation:** 301 multi-part documents, 315 with part-scoped objects, 2,188,767 part-scoped elements, and both true/false shared attributes. `0x011a` maps to `partDef`.
- **Conclusion:** sharing is pervasive within ordinary record types rather than visibly isolated in a named XML section. Legacy duplication versus references remains unresolved.

## 2026-08-05 — Public-source search

- **Question:** Is an existing open decoder documented?
- **Method:** web search for exact signature and Finale MUS binary decoder/source.
- **Observation:** public preservation descriptions confirm MUS/ETF roles, but no primary open MUS decoder or technical specification was found in the searched results.
- **Conclusion:** continue clean-room correlation. No proprietary MakeMusic source was accessed.

## 2026-08-05 — Finale 2002 controlled MUS/ETF pair

- **Question:** Does a same-version one-pitch edit reveal outer record framing in the 2001–2006 high-entropy family?
- **Files examined:** `tests/evidence/F2002/F2002-baseline.mus` and `.etf`, plus `F2002-changed-C-to-D.mus` and `.etf`; provenance records Finale `2002a.r1`, Mac OS 9.0.4, SheepShaver.
- **Method:** SHA-256, binary comparison, hex inspection from `0x200`, typed/length candidate scan, and normalized ETF diff.
- **Observation:** The baseline is 2,713 bytes; the changed file is 2,717 bytes. Both contain big-endian records beginning at `0x200`: type `0x000f`, total length 1,976; type `0x0010`, total length 158; type `0x0011`, total length 53/57; and terminal type `0x0012`, total length 6. The length includes the six-byte header. The changed terminal marker moves by four bytes.
- **Observation:** Only 45 byte positions differ before the baseline EOF, from 1-based offset 2,652 through 2,712; the preceding `0x000f` and `0x0010` records are byte-identical. The ETF diff changes the three `eE` entry records, including the note value from `0` to `16` and related entry fields.
- **Conclusion at the time:** This pair provides strong evidence for a variable-length typed outer container in Finale 2002, not fixed 16-word records. It localizes pitch/entry changes to type `0x0011`. The payload codec was still unresolved at this stage; the 2026-08-08 DCL experiment below supersedes that limitation.
- **Follow-up:** Create the same minimal baseline and one-pitch variant in F2003, F2004, and F2005; compare type sequences, length semantics, terminal markers, and whether entry edits remain isolated to the corresponding pool.

## 2026-08-05 — Finale 2003 controlled MUS/ETF pair

- **Question:** Does the F2002 typed/length outer framing persist in Finale 2003, and does a one-pitch edit remain localized to the entry pool?
- **Files examined:** Tracked `tests/evidence/F2003/F2003-baseline.mus` and `.etf`, plus `F2003-changed-C-to-D.mus` and `.etf`; provenance records Finale `2003a.r1`, Mac OS 9.0.4, SheepShaver.
- **Method:** SHA-256, binary comparison, scan for big-endian type/total-length records from `0x200`, and normalized ETF diff.
- **Observation:** The baseline is 3,088 bytes; the changed file is 3,089 bytes. Both contain the same outer sequence: `0x000f` length 2,346 at `0x0200`; `0x0010` length 158 at `0x0b2a`; `0x0011` length 58/59 at `0x0bc8`; and terminal `0x0012` length 6 at `0x0c02`/`0x0c03`.
- **Observation:** Only 44 byte positions differ before the baseline EOF, from 1-based offset 3,022 through 3,088; the preceding records remain byte-identical. The ETF changes only the first `eE` note payload from `0` to `16`.
- **Conclusion at the time:** Finale 2003 independently confirms the F2002 six-byte big-endian typed/length outer framing and localized `0x0011` entry-pool edits. The framing was strong across two adjacent releases; the 2026-08-08 DCL experiment below subsequently solved the payload codec.
- **Follow-up:** Repeat the same pair in F2004 and F2005. If the framing persists, focus the reverse engineering on the payload bytes and test whether the type/payload vocabulary is stable across 2002–2005.

## 2026-08-05 — Finale 2004/2005 controlled MUS/ETF pairs

- **Question:** Does the F2002/F2003 outer framing persist through Finale 2004 and 2005, and does the one-pitch edit remain localized?
- **Files examined:** Tracked `tests/evidence/F2004/` and `tests/evidence/F2005/` baseline and C-to-D MUS/ETF pairs. Provenance records Finale `2004c.r1` and `2005b.r1`, both Mac OS X 10.4.11 under a QEMU PowerPC emulator.
- **Method:** SHA-256, binary comparison, big-endian type/total-length scan from `0x200`, and normalized ETF diff.
- **Observation:** F2004 baseline/changed sizes are 3,179/3,181 bytes; F2005 sizes are 3,183/3,186 bytes. Both preserve the four-record sequence: `0x000f`, `0x0010`, `0x0011`, and terminal `0x0012`, with six-byte headers and total lengths. F2004 lengths are `2429, 166, 58→59, 6`; F2005 lengths are `2436, 166, 55→57, 6`.
- **Observation:** Unlike F2002/F2003, F2004 and F2005 also change ETF `BC` records when C is changed to D. The binary diff therefore spans much of the file, even though the entry pool still changes in `0x0011`. The leading explanation is that these documents have automatic note spacing enabled, causing derived spacing/layout records to be rewritten; this is not yet confirmed.
- **Conclusion at the time:** The typed six-byte big-endian outer framing and four principal record types were strongly reproduced across Finale 2002, 2003, 2004, and 2005. These controlled pairs did not identify the payload codec; the 2026-08-08 DCL experiment below did. The `BC` difference should be treated as a configuration-dependent derived-record effect until a matched automatic-spacing-disabled test confirms it.
- **Revised follow-up:** Decode both sides with `blast`, then use a less layout-sensitive edit to map internal records and distinguish direct entry changes from derived layout changes.

## 2026-08-05 — Clean public ETF/MUS search

- **Question:** Is there a public, non-proprietary description or implementation of the ETF grammar or the 2001–2006 MUS codec?
- **Method:** Search public preservation records, Finale manuals/help, historical format articles, independent ETF tooling, and public source-code indexes. No MakeMusic plugin-development source or headers were accessed.
- **Observations:** The Library of Congress describes ETF as Finale's plain-text transport counterpart to binary MUS and states that no formal MUS specification is available. Finale documentation says ETF files could be created through Finale 2006, but not from Finale 2007 onward. LilyPond's `etf2ly` utility documents an independent parser for a subset of ETF, useful for validating ETF grammar and entry syntax but not for decoding binary MUS. Historical public articles provide signatures and examples, but no 2001–2006 binary codec.
- **Conclusion:** The public search materially improves ETF grammar knowledge but found no credible public 2001–2006 MUS codec implementation or specification. Continue with exact same-version MUS/ETF pairs and controlled differences before considering the side-channel.
- **References:** Library of Congress MUS description; Finale help/glossary; LilyPond `etf2ly` manual; Tyler Thorsted's historical Finale format notes. URLs are listed in the final report and README's external references.

## 2026-08-05 — Archive and extensionless survey

- **Question:** Do archives contain additional Finale files, including files without `.mus` suffixes or earlier than the direct corpus?
- **Method:** `scripts/archive_probe.py` scanned ZIP archives and hashed candidate `.mus`/extensionless members without altering originals; candidate Finale 2.6 members were extracted to `/tmp` for the existing structure probe. StuffIt archives were counted but not treated as extracted because no compatible extractor was available.
- **Observation:** 230 ZIP archives yielded 3,468 candidate members (1,737 with Enigma banners, 1,846 extensionless, 1,870 unique member hashes). 831 unique Enigma hashes were not in the direct inventory. Nine members explicitly identify as `Finale(TM) 2.6`; all show a pre-banner/low-entropy body family and had no Finale 27 counterpart at this stage. Later targeted conversion succeeded after adding `.mus`. An archive of extensionless originals contains normal Enigma-banner files, confirming that suffix absence is not itself a different binary format.
- **Conclusion:** Archives materially expand the corpus and provide the earliest explicit-version samples found so far. ZIP-derived candidates are inventoried in `data/archive_members.csv`; the nine 2.6 probes are summarized in `data/archive_legacy_probe.csv`. The 197 `.sit` files remain an extraction gap.
- **Follow-up:** Request ETF exports for the highest-value 2.6 samples and obtain a compatible StuffIt extractor before selecting additional candidates.

## 2026-08-05 — Complete StuffIt extraction

- **Question:** Does the expanded corpus contain earlier Finale formats inside StuffIt archives?
- **Method:** Installed `unar`/`lsar` 1.10.7; listed and extracted all 275 `.sit` files into temporary directories, preserving data-fork hashes and observing resource-fork extraction. Combined results with the 230 ZIP archives.
- **Observation:** The complete archive pass produced 4,898 candidate members, including 2,271 Enigma-banner members and 2,990 extensionless candidates. Explicit pre-banner products include 1.8.7 (10 unique hashes), 2.0.1 (26), 2.6 (100), 3.0 (15), 3.2 (14), 3.5 (19), and 3.7 (25).
- **Conclusion:** StuffIt archives substantially extend the corpus backward. Finale 1.8.7 is the earliest explicit product currently found; no explicit Finale 1.0 sample has been identified. Resource forks can be extracted by `unar` and should be retained for future metadata analysis.
- **Follow-up:** Add targeted ETF/opening requests for 1.8.7, 2.0.1, and 2.6 representatives; inspect any remaining pre-banner candidates for earlier implicit versions.

## 2026-08-05 — Platform coverage risk

- **Observation:** The direct and archive corpora are overwhelmingly Macintosh-derived, including classic Mac resource-fork and StuffIt evidence. Current Windows-origin samples are too sparse to establish platform invariants.
- **Conclusion:** Platform bias is a material feasibility risk. Header tuples, byte order, string encoding, padding, and serialization behavior may differ on Windows even when Finale release labels match.
- **Follow-up:** Request a matched Windows corpus and Mac/Windows same-document saves, especially around the 2007/2008 transition.

## 2026-08-05 — ETF evidence set

- **Question:** Can ETF exports provide a semantic bridge to the pre-2007 binary families, and what does the same source document reveal when exported by Finale 2000 versus Finale 2005?
- **Files examined:** Local private evidence assets `nestedTupletFin05RC2.etf`, `template-Fin2000-from-Fin2000.etf`, `template-Fin2000-from-Fin2005.etf`, `guitar pc.etf`, `Dream of Summer I-from-Fin2.6.3.etf`, and `Score-from-sit-archive.etf`. The corresponding source IDs are `mus-d89e8fe12e271440`, `mus-3597fd4fce0c272b`, `mus-7aa45639c14b3864`, `mus-2c0a5e8897b436d5`, and the Finale 2.6 `Score` archive class.
- **Method:** SHA-256 and byte counts; normalize the classic ETF carriage-return line endings for section and structure counting; inspect headers, section order, structure identifiers, `eE` entry records, and controlled template differences.
- **Observation:** ETF sizes/hashes are: `nestedTupletFin05RC2.etf` 16,893 bytes (`4345805a001e9c198cc2f022c8c469cb1654e290d4c5e8000d16e8d547f64517`); `template-Fin2000-from-Fin2000.etf` 27,945 (`76ce06887b36720aac51ef2578eda11ece72cf3c921ccf832c3f5790eed413de`); `template-Fin2000-from-Fin2005.etf` 34,029 (`b96c24f43be5bcb0a54ffbd7a44a42311c2cae8cf45b0a4c4e2180ca4f7f2e76`); `guitar pc.etf` 123,084 (`d30d569e4b9cc4e642d9a494d1efdc2a975f624a7353f26b2d22a133b1e49844`); `Dream of Summer I-from-Fin2.6.3.etf` 74,040 (`03aa22ec769a5d10dbdb2cde4512cf31d9730c95cad109ebd766f02368edf15e`); `Score-from-sit-archive.etf` 1,272,164 (`ca9ed81de7f782bf1e2e2ccf22714018dae8bb75648d4ff083e01b97a6fd58c1`).
- **Observation:** All six exports contain `others`, `details`, `entries`, `text`, and `lyrics` sections. The older exports use `eE` entry structures at scale: 1,094 lines for `guitar pc`, 549 for Dream of Summer I, and 9,446 for Score. The Finale 2005 nested-tuplet sample contains six `eE` entries; the Finale 2000 template contains no `eE` entries.
- **Observation:** The ETF header explicitly identifies Finale 2000 or Finale 2005 for the template pair and Finale 2005 for the nested-tuplet sample. The older pre-banner ETFs do not have the modern `^header` section, so their exporter/source version cannot be inferred from ETF structure alone.
- **Observation:** The same Finale 2000 template exported by Finale 2005 is larger and contains additional `&f`, `PD`, `XA`, expression, and other records; the saver header also changes to Finale 2005. This demonstrates semantic upgrade/synthesis during resave.
- **Observation:** The ZIP copy of the Finale 2.6 Quartet `Score` could not be opened by Finale 2.6.3 because the ZIP did not preserve the classic Mac resource fork. The parallel StuffIt extraction supplied the readable source and ETF.
- **Conclusion:** ETF confirms that the conceptual other/detail/entry/text decomposition predates the 2007 typed-zlib container and gives strong logical targets for the old binary families. It does not by itself reveal the pre-2007 binary codec or prove byte-for-byte correspondence. Later-version ETF exports must be treated as normalized semantic references.
- **Follow-up:** Correlate ETF structure lines with binary offsets for the five available source classes; use the two template exports to separate source-document records from Finale 2005 upgrade records; obtain the missing Finale 3.0 ETF before attempting a general legacy decoder.

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
- **Files examined:** `mus-3a8b724cf3adba80` (`tremolos.mus`, 20,149 bytes) and the newly supplied local-only `tremolos-from-Fin2000.etf`, 28,718 bytes, SHA-256 `c02e859d6026de960a44ea07bd0d3154e07e7f85d690cc5eaf5a84b623d3149d`; then all direct Finale 3.0, 3.2, 3.5, 3.7, 97, and 2000 files resolved through the private mapping.
- **Method:** Walk two-byte type/four-byte total-length blocks from `0x200` in both byte orders; require exact EOF consumption and type sequence `1,2,3,4`; test pool payloads modulo 16/16/38; compare ETF section counts and tag order; normalize only ETF separators between text records and compare raw bytes. The reusable aggregate probe is `scripts/uncompressed_probe.py` and never emits source paths.
- **Pair observation:** The binary blocks begin at `0x0200`, `0x4736`, `0x4b3c`, and `0x4c72`, with types `1,2,3,4` and total sizes `0x4536`, `0x0406`, `0x0136`, and `0x0243`. Their payloads contain 1,107 16-byte other rows, 64 16-byte detail rows, eight 38-byte entries, and 573 text bytes, exactly matching the ETF section counts. All ordinary tags are in identical order; ordinary values such as `IS`, `Iu`, `MS`, `PS`, and `SS`, detail values such as `CN`, `GF`, and `TP`, and selected `eE` fields are directly recognizable. After removing ETF record separators, the complete text section is byte-identical to block 4.
- **Corpus observation:** 189/190 direct Finale 3.x–2000 files traverse exactly as four pools. All 1,552,762 type-1 rows and 770,960 type-2 rows divide by 16, and all 394,984 type-3 rows divide by 38. The recognized population is 185 big-endian files and four little-endian files. All four little-endian files are Windows-origin: the three Finale 3.0 samples and one Finale 2000 sample. One Finale 97 file remains unrecognized.
- **Conclusion:** **Confirmed.** Finale 3.x–2000 stores the same fixed physical other/detail/entry rows uncompressed. Finale 2001 primarily changes the pool identifiers and adds CRC-protected DCL compression. The previous “low-entropy codec/table encoding” hypothesis is disproved for the recognized banner-era files. Byte order is platform-sensitive and should be validated from the block headers.
- **Follow-up:** Parse all type-1/type-2 rows into versioned PDK-era logical structures; investigate the compact binary encoding of ETF pseudo-tags `#v*`, `#c*`, and `#s*`; classify the remaining Finale 97 file; then test whether the pre-banner Finale 1.x–2.x families use an earlier form of the same four pools.

## 2026-08-08 — Private PDK Framework option-map audit

- **Authorization and boundary:** Inspected `RGPPDKFramework` and `JWPDKFramework` histories read-only under explicit
  authorization. No source was copied. Only distilled interoperability facts are published, labeled
  `private-framework-derived`.
- **Snapshots:** Current mapping from `RGPPDKFramework` commit
  `44650a9a11cc8a5f86628b52a1ae75cc523a19a6`; historical comparison at `JWPDKFramework` commit
  `d8a4c7782a7213bfd7350e3f03976b12afb1d2ab` and initial import
  `37326071691ba6ce67a4c894ec3c5a0a616ab434`. The original pre-Finale-2014-oriented line was checked explicitly at
  branch `RGP-JWOriginalCleanup`, commit `982939e1c14b4dfcb9fe73ce2369fdd77e88392f`.
- **Question:** Do the framework's synthetic preference structures explain how ETF `^NN(65534)` globals map to
  modern logical options?
- **Method:** Reduced each compatibility-table row to a neutral tuple of group, semantic field, tag/global number,
  comparator, incident, word slot, width, conversion, and version bound. Compared selectors against all available ETF
  evidence without publishing private paths.
- **Observations:** Current snapshot has 435 rows in 24 groups: 424 baseline plus 11 Finale 26.2 compatibility
  replacements. Widths are 383 two-byte, 36 four-byte, and 16 one-byte fields. The map uses 61 numeric globals and
  nine named tags. Available ETFs contain selectors used by 385 rows and 59 of 61 numeric globals; `^47(65534)` and
  `^48(65534)` were not observed. ETFs contain 35 additional numeric globals absent from the map.
- **Historical observation:** The original branch has 343 rows, 341 of which remain unchanged in the current table.
  Row count grew to 373 at the historical head and 435 in the current snapshot. `specialExtractedPartCmper` moved
  from `^23(65534)` incident 0/slot 4 to `^PG(65534)` incident 0/slot 3. `score_in_c` moved from `^PG(0)` to
  `^PG(65534)` at incident 0/slot 0. The published union therefore has 437 rows and preserves both legacy alternatives.
- **Direct-block observation:** Slur contours, tie placement, tie contours, grids/guides, and stem connections bypass
  the field map at `^52`, `^85`, `^86`, `^88`, and `^40` respectively, all with comparator 65534. Every selector is
  present in available ETF evidence. Stem-connection element layout changes at Finale 2012.
- **Conclusion:** Options are a data-driven, partially solved problem. The table is sufficient to begin a guarded
  default-plus-overlay importer, but semantic fields and conversion rules still require controlled verification.
- **Artifacts:** `LEGACY_OPTION_MAPPINGS.md`, `data/legacy_option_mappings.csv`, and
  `data/legacy_direct_option_blocks.csv`.
- **Follow-up:** Produce single-option controlled MUS/ETF pairs for music spacing, ties, score page format, repeats,
  and chords; then promote verified rows individually.

## 2026-08-08 — Finale 1.8.7–2.6 correlation with Finale 3.0

- **Question:** Are the explicit early archive files structurally unrelated to Finale 3.0, or do they contain an earlier serialization of the same records?
- **Files examined:** Re-extracted data forks for `mus-7aa45639c14b3864` (Finale 1.8.7), `mus-2c0a5e8897b436d5` (Finale 2.0.1), and the Finale 2.6 `Score` source class, paired with their existing private ETFs. ZIP and several parallel StuffIt `Score` copies were distinguished by hash rather than assumed identical.
- **Method:** Inspect from `0x200`; test 16-byte cadence from `0x20a`; compare literal and compact tag sequences; reconstruct each old `eE` record as four 32-bit values, two packed 16-bit values, and three 32-bit flag values; compare all reconstructed 32-byte rows against the binary; locate raw `^block` text and compare ETF content.
- **Observation:** All three binaries begin 16-byte records at `0x20a`. Finale 1.8.7 matches all 1,447 ordinary ETF tags; the 2.0.1 and 2.6 comparisons match initial prefixes before later-export/source-copy normalization changes ordinary ordering. Detail tags match completely: 891/891, 497/497, and 6,814/6,814, including compact encodings `0x80xx`, `0x90xx`, and `0xa0xx` for ETF `#v*`, `#c*`, and `#s*` pseudo-tags.
- **Entry observation:** Entry IDs are implicit array positions. Every ETF entry reconstructs its corresponding 32-byte binary row exactly: 1,094/1,094, 549/549, and 9,446/9,446. Finale 3.x extends that same entry body to 38 bytes by prepending the explicit four-byte entry ID and appending two zero/reserved bytes.
- **Text observation:** The ETF text payloads are exact prefixes of the raw binary tails: 405, 936, and 1,401 bytes respectively. Each binary has 13 additional trailing bytes outside the ETF text payload.
- **Conclusion:** **Strongly confirmed continuity.** Finale 1.8.7–2.6 and Finale 3.0 share the logical record vocabulary, 16-byte ordinary/detail cadence, entry field representation, and raw text. Finale 3.0 adds explicit typed/length pool framing, rotates key fields within ordinary/detail rows, and expands entries from implicit-ID 32-byte rows to explicit-ID 38-byte rows. The early index/directory spans and generic pool-boundary algorithm remain unresolved.
- **Finale 27 result:** Finale 27 opened all three sources—1.8.7, 2.0.1, and 2.6—after `.mus` was appended to the extensionless filenames. It produced valid MUSX containers with decoded pool counts of 2,018/787/1,320, 1,347/478/659, and 14,913/6,095/11,153 for others/details/entries. The 2.6 file had font issues but converted. This disproves a parser cutoff at 2.6.x for the tested data forks and identifies filename recognition as the original barrier.
- **Follow-up:** Request exact minimal MUS/ETF pairs saved and exported by Finale 2.6.3 and, if supported, Finale 1.0; test Finale 1.0 separately because it remains outside the verified compatibility range.

## 2026-08-10 — Second corpus: Finale application installations

- **Question:** A cache of MacFin and WinFin installations was located. Do application installs contain legacy
  material the document corpus does not, and does the existing pipeline actually see it?
- **Method:** Registered it as survey `rpatters1-installs` and rebuilt private output per survey so two corpora can
  coexist. `Finale PDK` was excluded throughout: it holds MakeMusic proprietary PDK sources and stays outside this
  project's provenance boundary until a deliberate, separately scoped decision says otherwise. `*.lib` and `*.fan`
  were excluded as ENIGMA-framed non-documents. Finale 2014.5 and later application bundles were excluded because
  they save only MUSX.
- **Observation — the pipeline was blind to most of it.** An extension-only scan finds 7,542 `.mus` files. Content
  sniffing finds 2,518 more, because classic Mac Finale kept the file type in the resource fork and its documents
  carry no extension. Products 1.0.0, 2.6, 3.0, 3.2, 3.5, 3.7, 3.8, 97, 98 and 99 would otherwise have reported a
  confident zero. A third banner spelling was also unmatched: Finale 1.0.0 writes `Finale` + MacRoman 0xAA +
  version + `ENIGA Structures`, so even the archive member sniffer rejected those files.
- **Observation — new coverage.** 22 Finale 1.0.0 files (14 samples, 6 templates, 2 tool demos), 11 Finale 3.8,
  56 Finale 98 and 4 Finale 99, none of which appear in `rpatters1-main`. Total 10,060 loose specimens,
  4,535 distinct by content. The 32 legacy installer ZIPs yielded no cacheable members at all.
- **Observation — two banners are not releases.** `Finale 3.8` files carry Enigma 3.8.0 build 7, byte-identical to
  every Finale 97 file that yields a tuple; `Finale 99` files carry Enigma 5.0.0 build 15 against Finale 2000's 5.0
  line. Coda replaced version-numbered product names with year-numbered ones, and files written around the rename
  keep the older banner. Recorded in `VERSION_MATRIX.md` under *Renamed releases*.
- **Observation — Finale 98 application major is 4.** 41 of the 44 macOS-origin files that yield a tuple decode
  application version 4.0.x, settling a presumption `VERSION_MATRIX.md` had carried as unverified. The Enigma
  version is not uniformly 4: 41 files carry 4.0.0 build 10 and 8 carry 3.8.0 build 7. Four Windows-origin files
  decode nonsense at the same offset and are unclassified.
- **Observation — entropy separates the eras cleanly.** Median body entropy is 2.21–3.99 with no compressed member
  for every product through 2000, and 7.64–7.70 for 2001–2006. This is what places 1.0.0, 3.8, 98 and 99 in their
  structural families on evidence rather than on where their names fall in sequence.
- **Correction to method.** An initial exclusion of `*/Libraries*` was wrong and was withdrawn. No `.mus` lives
  under those directories, but real documents do: at version 2007 Finale moved two stock font-default documents
  into them, and the exclusion also removed the only Finale 3.0, 3.2, 3.5 and 99 material in the corpus.
- **Reference-corpus effect.** Re-running `rpatters1-main` through the same tool dropped it from 1,290 to 1,289
  files: a 4,096-byte AppleDouble `._` sidecar previously carried as an `unknown` product row is no longer treated
  as a specimen. No file changed saving product, and the corpus fingerprint changed accordingly.
- **Conclusion:** Installation media are a distinct and worthwhile evidence class — they supply version coverage
  that authored documents cannot, especially at the earliest eras. The discovery gap they exposed was in the survey
  tooling, not in the corpus.
- **Follow-up:** Run `dcl_probe.py` against both surveys; the framing figures in `VERSION_MATRIX.md` predate this
  run. Classify the four Windows-origin Finale 98 files. Decide separately and deliberately whether `Finale PDK`
  is ever surveyed.

## 2026-08-10 — Pre-zlib default-font preference array

- **Question:** Where does the pre-zlib representation store the font IDs used by musxdom `FontOptions`?
- **Method:** Followed the framework's direct default-font preference family and its zero-based preference numbers,
  then inspected selector `24(65534)` in all eight controlled Finale 2002–2005 ETFs. Compared the preference-number
  order with musxdom's `FontType` order and treated each six-word incidence as candidate fixed-size tuples.
- **Observation:** Each incidence is two consecutive `(font ID, size, effects)` triples. For physical tuple index `n`,
  the incidence is integer division `n / 2`; the triple begins at word slot 0 for even `n` and slot 3 for odd `n`.
  Finale 2002 carries 20 incidences (indices 0–39). Finale 2003–2005 carry 22 (indices 0–43), with an unused
  not-yet-defined half-incidence zero-filled. Every baseline/changed pair has an identical array.
- **Later semantic correction (2026-08-11):** Here `n` is a physical tuple index, not one timeless modern
  `FontType`. Finale 2002 physical 13 is a holding slot and 28 is default tablature; Finale 2003 reassigns 13 to
  tablature and 28 to percussion. Physical 43 remains reserved through Finale 2006.
- **Correction:** The initial font-ID audit incorrectly treated `FontType` itself as the incidence and always named
  slot 0. That would read every odd type from the following physical row and was corrected before importer code was
  added.
- **Conclusion:** **Private-framework-derived; strongly ETF-supported.** The complete pre-zlib array is located at
  `24(65534)`, and every stored font ID, size, and effects word now has a deterministic address. Controlled
  one-font-at-a-time edits are still required for `confirmed` semantic status.
- **Artifacts:** `FORMAT_NOTES.md`, `LEGACY_OPTION_MAPPINGS.md`, and
  `data/legacy_option_font_id_locations.csv`.

## 2026-08-10 — Zlib default-font preference array

- **Question:** Did the pre-zlib default-font tuple array survive the 2007 class-record transition, and where is it?
- **Method:** Inflated block `0x001a` in the controlled big-endian Finale 2007 and little-endian Finale 2012 MUS
  fixtures, walked the established class-record framing to exact block exhaustion, and inspected every record with
  comparator `65534`. Compared candidate payloads with selector `24(65534)` from the controlled Finale 2005 ETF and
  compared the complete numeric-selector sets across the format boundary.
- **Observation:** Class `0x0026`, incidence 0, has a 276-byte payload in both zlib fixtures. It is 46 consecutive
  `(font ID, size, effects)` triples. Indices 0–44 correspond to the 45 musxdom `FontType` values; index 45 is zero.
  The first 43 Finale 2007 tuples exactly match Finale 2005. Finale 2007 fills the two later time-parts types and
  retains a final zero pad. Finale 2012 has the same organization in little-endian byte order.
- **Broader observation:** The zlib option class ID equals the pre-zlib numeric selector plus `0x000e`. Finale 2007
  contains every numeric global family in the Finale 2005 ETF under that transform and adds selector 47. Finale 2012
  retains the transform with version-specific membership changes. Thus each zlib option class payload is the old
  selector's incidence stream coalesced into one length-governed record, not a new unrelated layout.
- **Effects:** The third tuple word is an Enigma style mask. Import must call musxdom
  `FontInfo::setEnigmaStyles(uint16_t)` to populate `bold`, `italic`, `underline`, `strikeout`, `absolute`, and
  `hidden` rather than assign it to a scalar.
- **Conclusion:** **Strong.** FontOptions is locatable and implementation-ready for both endian variants at
  `0x0026(65534)`, with tuple byte offset `6n`. A one-font-at-a-time zlib edit and trusted conversion are still
  needed to promote the semantics to `confirmed`.
- **Artifacts:** `FORMAT_NOTES.md`, `LEGACY_OPTION_MAPPINGS.md`, and
  `data/legacy_option_font_id_locations.csv`.

## 2026-08-10 — Finale 1.0.0 font-definition and selector-24 counts

- **Question:** Can modern fallback FontOptions safely fill types absent from a Finale 1.0.0 file, and does the
  later selector-24 mapping apply to that era?
- **Method:** Resolved the 22 content-derived Finale 1.0.0 aliases in the existing installation survey through its
  ignored private path map. Read the corpus in place without modification, walked the already-established first
  Coda-banner pool, counted distinct `FN` comparators, and inspected every `24(65534)` row. No path or filename was
  added to a public artifact.
- **Observation:** Every specimen contains exactly five `FN` families with cmpers 0–4 and the same five-name font
  table. Every specimen contains one selector-24 incidence with words `13, 69, 52, 48, 65, 60`. Interpreted as the
  later pair of `(font ID, size, effects)` tuples, both IDs would be outside the existing definition range.
- **Conclusion:** **Confirmed for the surveyed Finale 1.0.0 specimens.** They have five font definitions, and their
  selector 24 is not the later FontOptions array. A pinned modern `FontOptions` cannot safely be retained because
  its cmpers can resolve to unrelated source definitions. Filter it out and build a fresh 45-entry object: use
  era-verified source entries where available, then clone missing `FontInfo` values from a separate baseline document
  while remapping nonzero font IDs by musxdom's whitespace-insensitive, case-folded font-name key or by copying the
  baseline `FontDefinition` to a new target cmper. Baseline cmper 0 transfers unchanged. Locating the actual Finale
  1.0.0 font-option representation remains open.
- **Artifacts:** `FORMAT_NOTES.md`, `LEGACY_OPTION_MAPPINGS.md`, and
  `data/legacy_option_font_id_locations.csv`.

## 2026-08-10 — Default-font ordinal sequence: proposed Finale 27 pair test

- **Question:** Has every legacy default-font representation used the current musxdom `FontType` order, especially
  Finale 1.0.0 whose representation has not yet been located?
- **Proposed method:** Compare each legacy source with Finale 27's upgrade of that exact source. Resolve font IDs on
  both sides to the same normalized font-name key and compare `(name, size, effects)` tuples. For each physical
  source ordinal, accumulate all equal named Finale 27 categories across varied pairs and intersect those candidate
  sets. Do not compare numeric cmpers and do not treat an upgraded category with no source tuple as historical data.
- **Coverage plan:** First upgrade the controlled Finale 2002–2005, 2007, and 2012 fixtures. Then analyze the exact
  pairs already present across the authored-document corpus. Finally create private Finale 27 upgrades for the
  installation-only gaps, beginning with several structurally different Finale 1.0.0 specimens. Use the named
  Finale 27 vector, translated back through the source's five font definitions, as a signature when scanning early
  global records without presuming selector 24.
- **Ambiguity rule:** Identical defaults may leave several candidate category names for one ordinal. Resolve only
  those cases with a controlled source-version save that changes one category to a distinctive face, size, and
  effect combination, followed by Finale 27 upgrade of that exact save.
- **Status:** **Open.** Exact paired upgrades are the intended semantic oracle; a stable payload length or apparent
  prefix is insufficient to establish the sequence. The complete procedure is in
  `LEGACY_OPTION_MAPPINGS.md`; the evidence request is C7 in `EVIDENCE_REQUESTS.md`.

## 2026-08-11 — Source-only FontOptions capture

- **Question:** Can the reader safely capture the default-font tuples already identified without first deciding how
  many entries every historical version should contain or synthesizing the modern tail?
- **Method:** Filtered the pinned Finale 27 `FontOptions` object, added a two-row physical layout table for the
  Finale 2002-2006 fixed-row family and Finale 2007-2012 zlib class record, and walked each source to its physical
  end. Every complete tuple is reported in encounter order. Representable ordinals create fresh document-owned
  `FontInfo` instances; effects pass through `setEnigmaStyles`.
- **Observation:** The controlled Finale 2002 fixture captures 40 tuples. Finale 2005 captures all 44 physical
  tuples, including the zero-filled second tuple of its final fixed row, without adding a 45th semantic type.
  Finale 2007 and Finale 2012 each populate 45
  musxdom ordinals and also report the physical tuple at ordinal 45; that tuple is zero and is not cast to an invalid
  `FontType`. Big- and little-endian zlib values both decode correctly.
- **Conclusion:** **Confirmed for the controlled fixtures.** First-stage capture has no expected tuple count and
  imports no baseline font id. Completeness, normalized-name remapping, and the unidentified Finale 1.0.0 layout are
  separate later stages.
- **Artifacts:** `src/import/mappings/font_options.cpp`, `tests/reader_tests.cpp`, and
  `data/font_options_mapping.csv`.

## 2026-08-11 — Versioned FontOptions semantics against Finale 27 upgrades

- **Question:** Do exact Finale 27 companions complete shorter legacy font-option arrays, and does the physical
  ordinal sequence retain one semantic meaning across the Finale 2003 and 2007 boundaries?
- **Method:** Applied the `survey-class-coverage` procedure to 1,189 adjacent-exact occurrences representing 1,115
  distinct sources in `rpatters1-main`; 36 fallback-unique occurrences were retained as a separate weaker cohort.
  Source and companion font cmpers were resolved independently and compared by normalized font name, size, and
  effects. The interpretation used the private-framework-derived canonical boundary supplied for Finale 98 and
  Finale 27 rather than casting physical ordinals directly to the modern enum.
- **Observation:** All 1,189 distinct companions carry the same complete 45-type vector. The 42 distinct Finale 2002
  sources carry 40 physical tuples: physical 13 is a legacy holding slot and physical 28 upgrades as modern
  tablature. The 329 distinct Finale 2003-2006 sources carry 44 physical tuples, and physical 43 is `(0, 0, 0)` in
  every one as structural fill for the second half of the final fixed row; zlib-era slot 43 is populated as
  `TimeParts`. Semantic completion therefore supplies six modern types
  to each Finale 2002 source and two to each Finale 2003-2006 source, for 910 synthesized option observations.
  Of 169 synthesized nonzero font references, 130 match a source definition by normalized name. The remaining 39
  are `Maestro Percussion` for Finale 2002 sources and require cloning a baseline definition.
- **Conclusion:** **Strong.** FontOptions needs versioned physical-to-semantic descriptors. The 2003 transition
  remaps default tablature from physical 28 to 13 and reuses 28 for percussion; by the zlib era the growing logical
  array carries `TimeParts` at 43 and `TimePlusParts` at 44. Order and representation before
  Finale 98 remain open. Finale 27 companions complete every observed modern type, but completion sometimes must
  add a missing font definition rather than merely remap an existing one.
- **Private artifacts:** `private/generated/rpatters1-main/class_coverage/font_options_fin27/`.

## 2026-08-11 — Controlled Finale 1.0.0 FontOptions locations and complete import

- **Question:** Where does Finale 1.0.0 store default fonts, and can the importer combine the recoverable early
  values with a safe complete modern collection?
- **Method:** Compared four newly authored Mac source files: an untouched control and copies changing only Music,
  TextBlock, or LyricVerse to distinctive font, size, and effects combinations. Verified their exact Finale 27.4
  companions independently by normalized font name. Implemented the three confirmed locations, the already
  established versioned Finale 2002–2012 physical-to-semantic rules, and baseline completion for every missing type.
- **Observation:** Music is tuple 0 of `02(65534)`; TextBlock and LyricVerse are tuples 0 and 1 of `26(65534)`.
  Their changed source values are `(12, 60, 0)`, `(2, 17, 3)`, and `(3, 13, 28)`. Upgrade output also changes
  categories not edited in the sources, so those companion values are synthesis rather than evidence of more early
  locations. All controlled imports now contain 45 semantic FontOptions. Baseline id 0 passes unchanged; every
  other synthesized ID is matched to the lowest target comparator by musxdom's normalized font name, or the full
  baseline FontDefinition is cloned at the next nonzero comparator.
- **Conclusion:** **Confirmed** for the three Finale 1.0.0 mappings and full controlled-fixture completion;
  **strong** for the versioned Finale 2002–2012 semantic map. Effects are expanded with `setEnigmaStyles`, while raw
  physical masks and nonsemantic holding and structural-fill tuples remain in the report.
- **Artifacts:** `tests/evidence/F100/`, `tests/evidence/finale27-provenance.txt`,
  `data/font_options_mapping.csv`, `src/import/mappings/font_options.cpp`, and `tests/reader_tests.cpp`.

## 2026-08-11 — Complete Finale 1.0.0 Font Preferences UI sweep

- **Question:** Where does Finale 1.0.0 store every font exposed in its Font Preferences UI, and which values can
  safely seed the early 1.x–2.x importer under an additive-only compatibility hypothesis?
- **Method:** Compared a baseline with thirteen controlled UI saves and independently parsed each Finale 27.4
  companion. Compared normalized source records by tag, comparator, incidence, and word rather than raw file offsets,
  because the baseline uses a smaller save allocation. Surveyed the same global families in 10 distinct readable
  Finale 1.8.7, 26 Finale 2.0.1, and 152 Finale 2.6 sources; separately compared 53 adjacent-exact Finale 2.6
  companions.
- **Observation:** Twelve controlled preferences map to modern Music, Key, Clef, Time, Chord, ChordAcci, Ending,
  Tuplet, TextBlock, LyricVerse, LyricChorus, and LyricSection. Clef splits its ID into `04(65534)` while its size and
  effects occupy words 4–5 of `39(65534)`. The historical `Name` tuple is words 3–5 of `04(65534)`, but Finale 27
  drops the controlled change. The Tuplet save also changes ChordAcci and Finale 27 preserves both. Every surveyed
  1.8.7–2.6 source retains the relevant record families; all 53 exact 2.6 upgrades uniquely corroborate Chord,
  Ending, and Tuplet when natural source variation distinguishes them.
- **Conclusion:** **Confirmed** for the twelve directly named Finale 1.0.0 mappings. Treat historical `Name` as
  modern `StaffNames` at **strong** confidence. Under the explicit additive-only hypothesis, apply all thirteen
  mappings through Finale 2.6 and synthesize any later additions by the safe baseline-remapping procedure. Fonts
  functioning outside the Finale 1.0.0 UI remain open.
- **Artifacts:** `tests/evidence/F100/`, `data/font_options_mapping.csv`,
  `src/import/mappings/font_options.cpp`, and `tests/reader_tests.cpp`.

## 2026-08-12 — The FontOptions 13/28 boundary is Finale 2012, not Finale 2003

- **Question:** At which version does physical ordinal 13 become `Tablature` and 28 become `Percussion`? The
  2026-08-11 entry above records Finale 2003, taken from private framework history. **This supersedes that date.**
- **Method:** Imaged the Finale 2011 install DVD (hybrid APM/HFS + ISO9660; both payloads mined) to obtain the first
  Finale 2011 specimens in any survey, generated Finale 27 companions for them, and compared our recovered physical
  13 and 28 against each companion's independently resolved `tablature` and `percussion`. Counted only documents
  where the companion's two values differ *and* our two slots differ; any other document is consistent with both
  hypotheses and would inflate whichever was tested first.
- **Observation:** Of 1,211 discriminating documents — 405 from Finale 2003–2010, 597 from Finale 2011, 209 from
  Finale 2012 — every pre-2012 document places tablature at physical 28 with 13 a holding slot, and every Finale
  2012 document places tablature at 13 and percussion at 28. No document contradicts this, on either platform.
- **Why the earlier date survived so long:** the corpus was said to fit the Finale 2003 boundary, and it did, but not
  discriminatingly. The test used Finale 2002 sources, and Finale 2002 precedes both candidate boundaries. Finale
  2011 is the only version whose behavior differs between the two hypotheses, and no Finale 2011 document existed in
  any survey until this one.
- **Method caution:** font names must be normalized with musxdom's `normalizeFontName` before comparison.
  `EngraverTextT` and `Engraver Text T` are one face; comparing raw spellings produced 324 false disagreements and
  made Finale 2011 look internally inconsistent. The first pass of this analysis would have been reported as
  inconclusive on that artifact alone.
- **Conclusion:** **Confirmed.** The boundary is Finale 2012 (major 17). Where measurement and the private framework
  history disagree, the measurement governs. Decide the layout by epoch first — the uncompressed and DCL epochs are
  entirely pre-2012 and need no version test, and major 12 occurs in both the DCL epoch (Finale 2006) and the zlib
  epoch (Finale 2007), so no version range alone separates them.
- **Impact:** the previous gate cost every Finale 2003–2011 document its tablature font and gave it a percussion font
  it never stored. 2,516 of the 2,629 FontOptions disagreements then present in the corpus were this single rule.
- **Artifacts:** `src/import/options/font_options.cpp` (`semanticType`), `tests/reader_tests.cpp`,
  `tools/options_coverage_probe.cpp`, `scripts/options_coverage_report.py`.

## Commands

Reproduction commands are in [README.md](README.md). Additional spot checks used `xxd -g 1`, `strings -a`, `unzip -l`, `unzip -p`, Python's `zlib`, `gzip`, `zipfile`, and `xml.etree.ElementTree`. Temporary decoded samples were written only under `/tmp`.
