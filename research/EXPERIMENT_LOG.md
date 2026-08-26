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

## 2026-08-21 — Zlib class-record part dimension

- **Question:** Does the fourth header word of a zlib detail record identify an incidence or
  the source part?
- **Method:** Compared all class `0x041d` headers and payloads in two Finale 2008 documents
  with the raw `measGraphicAssign` nodes in their independently decoded Finale 27 companions.
- **Observation:** Each source has nine records whose header value is zero and three whose
  value is 17. The latter repeat the complete score payloads for the same staff and measures.
  Each companion has corresponding empty nodes with `part="17" shared="true"`; every node's
  XML incidence is zero. Header and trailer bytes otherwise match their score counterparts.
- **Conclusion:** **Confirmed for `0x041d`.** The header field is the part id. Incidence remains
  structural within the class payload. Current import remains deliberately score-only; linked
  part reconstruction and sharing-mode inference are deferred.

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
  The two coverage tools are historical names retained because they performed this experiment;
  both were later replaced by the registry-driven recovery-coverage pipeline and deleted. Their
  historical forms on main remain available as
  `git show f3b8905:tools/options_coverage_probe.cpp` and
  `git show f3b8905:scripts/options_coverage_report.py`.

## 2026-08-16 — MultimeasureRestOptions, and a layout boundary inside the uncompressed epoch

- **Question:** Where does each field of `MultimeasureRestOptions` live, and does the distilled framework's
  selector `25` / selector `83` map hold across every era?
- **Method:** Dumped selector `25` and `83`, and their zlib class ids `0x0027` and `0x0061`, from all 3,725 files of
  the reference corpus and compared each against the exact Finale 27 companion of the 1,189 that have one. Checked
  the raw physical reads independently against the ETF exports of the tracked Finale 97, 2000, 2005, 3.7.2, 2.6.3 and
  1.0.0 fixtures, which print the same `^25(65534)` words.
- **Observation:** The framework map is correct for Finale 3.5 and later, all nine scalars and the flag agreeing with
  every one of the 1,130 companion-backed documents. But **Finale 1.8.7 through 3.2 store a different record**: one
  incidence of six words rather than two, with `numAdjY` in word 4 and `shapeDef` in word 5 instead of words 2 and 3.
  All 264 such documents are on one side of that line and all 3,458 later ones on the other, with no file carrying
  any other word count. The reference corpus begins at 1.8.7, so the era's lower bound was taken from the
  `tracked-evidence` survey registered in the same session: all 19 of its Finale 1.0.0 fixtures carry the six-word
  record, and each has an exact companion, which no 1.0.0 document in any other survey does. Words 1–3 of the early record vary per document and Finale 27 carries nothing from them.
  Separately, selector `83` first appears in Finale 97; its word **4** is `autoUpdateMmRests`, while word 2 is set in
  most documents and is a different thing entirely — 468 companion-backed documents carry word 2 with the companion
  flag off.
- **All three surveys were run, and the two smaller ones carry the decisive cases.** The `tracked-evidence`
  fixtures supply the only companion-backed Finale 1.0.0 documents anywhere (19, all six-word). The
  `rpatters1-installs` corpus, 12,116 documents and 0 import failures, supplies three releases the reference
  corpus does not contain at all — Finale 3.8 (11), Finale 98 (43) and Finale 2011 (1,295), every one on the
  later side — plus 22 more Finale 1.0.0 documents on the early side. It also holds the 24 Coda-era Windows
  documents that state a platform where their Mac contemporaries state a version, and therefore have no version
  at all; the marker recovers all 24, where any version range would have skipped every one silently. Selector
  `83` is present in all 11 Finale 3.8 and all 43 Finale 98 documents and absent from every earlier one, which
  confirms the Finale 97 arrival on both spellings of that release.
- **Why this is a marker and not a gate:** the boundary is Finale 3.5, which falls inside the uncompressed epoch, so
  an epoch gate cannot express it at all. A version range would have to guess a cut point between 3.2 and 3.5, which
  no corpus can narrow because no Finale 3.3 or 3.4 document exists in either survey, and it would fail closed on the
  Coda-banner Windows documents that state no version. The family's word count says which layout the file uses. This
  is the same Finale 3.5 boundary the stem family shows, decided by a different fact about a different record; no
  common cause is asserted and no constant is shared.
- **Conclusion:** **Confirmed** for both layouts and for every field of the class. Three values the early era cannot
  state — both H-bar adjustments and automatic updating — are asserted as `LegacyBehavior`, because the pinned
  baseline supplies 30, -30 and true where every early companion shows 0, 0 and false.
- **`noHorizontalStretch` is not open,** though this survey alone could not have said so. The corpus observation is
  only that bit 0 is the sole bit of the flags word any document uses and that no companion ever sets the option;
  that is consistent with a bit nobody happened to set. The repository owner supplied the fact that settles it:
  **"Stretch Horizontally" is a Finale 27 feature**, so no legacy format has anywhere to put it. The value is
  therefore known exactly for every file this reader will open, and is asserted false in every era. A controlled
  save was going to be requested for it; that request is withdrawn.
- **Incidental finding:** the H-bar shape comparator agrees with the companion everywhere, but 319 zlib documents
  name a shape their own file does not define. Those files carry no shape records at all, of any of the three shape
  classes, while other zlib documents in the same corpus carry all three — so it is a property of those sources, and
  their conversions materialize a shape library the source never stored. Recorded in
  [PRODUCTION_READINESS.md](PRODUCTION_READINESS.md#p22-dangling-shape-references-in-seeded-options); the reader
  keeps the comparator as read and warns.
- **Next evidence:** one Coda-era save that moves a single multimeasure-rest field, so words 1–3 of the early record
  can be named. Nothing else about this class is outstanding.
- **Artifacts:** `src/import/options/multimeasure_rest_options.cpp`, `tests/reader_tests.cpp`,
  `tests/mapping_tests.cpp`, `tools/options_coverage_probe.cpp`, `scripts/options_coverage_report.py`,
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#multimeasure-rest-defaults). The two coverage-tool names
  refer to the deleted historical tools identified in the 2026-08-12 entry above.

## 2026-08-17 — LyricOptions: six selectors, four arrival dates, and eleven fields nobody stores

- **Question:** Where does each field of `LyricOptions` live, and how much of it does the distilled framework's
  `LyricsPrefs` group actually cover?
- **Method:** Dumped the six candidate selectors from the 69 tracked fixtures with `tools/record_dump` and compared
  each against the fixture's exact Finale 27 companion, working outward from the zlib era as the most recognizable
  representation. Read the framework's `_FCEDTLyricsPrefs` struct and `FCLyricsPrefs` accessors read-only for the
  numberings the corpus could not supply. **No corpus survey was run**; every claim below rests on the fixtures.
- **Observation — the layout.** Six numeric globals at `65534`, none of them a direct block in the framework's
  sense, and each arriving at a different release: `15` (word 1, hyphen separation) and `67` (word 5, line width)
  from Finale 3.x; `87` (four syllable positions over two incidences) from Finale 2000; `55` (nine word-extension
  connection styles over five incidences) and `57` (three smart-lyric scalars) from Finale 2004; `35` word 5
  (smart hyphens) usable only from Finale 2004 though the record exists in every era. The zlib era coalesces each
  through the usual `numericGlobalClass` rule, and both byte orders are exercised by the Finale 2007 and 2012
  fixtures.
- **Observation — three orderings, none of which matches musxdom.** The syllable alignment list is
  `1 = centre, 2 = left, 3 = right` (the framework's `LYRICS_ALIGN_` constants, corroborated by every fixture for
  1 and 2 and by no fixture at all for 3). The connection point runs `0x10`–`0x15` on the *smart-shape entry
  connection* scale and orders lyric/head/dot/duration/systemLeft/systemRight, where musxdom puts the two system
  attachments third and fourth. Bit 15 of each position's flags word is musxdom's `on`.
- **The two decisive fixtures were already in the tree.** `F2006-embedded-tif.mus` carries all six connection
  numbers with five distinct vertical offsets and a horizontal offset of 8 where every other fixture has 4; its
  companion names each point beside the same offsets, which fixes the element layout, the whole numbering, and the
  fact that `wordExtHorzOffset`/`wordExtVertOffset` are the starting connection's own offsets rather than separate
  fields. `F2000-multilayer.mus` is the one document that clears the 0x8000 bit for the three optional positions,
  and the one whose companion omits their `<on/>`.
- **Why presence rather than a version, and the one place presence is unsafe.** Neither collection has two layouts,
  so nothing states a layout the way the multimeasure-rest word count does; a document either carries the record or
  does not, and presence reaches the Coda-era Windows documents that state no version. The exception is selector
  `55`, which the **Coda-banner era reuses for an unrelated option** — the Finale 1.0.0 and 2.6.3 fixtures store
  16128 and 16448 in it, and a controlled 1.0.0 stem-options save moves its first two words. That epoch is excluded
  outright, with the word count as a second guard. Selector `35` word 5 is the other qualified case: it is 0 in
  every pre-2004 document and 1 after, while every companion of every era says smart hyphens are on, which is what
  an option arriving switched on looks like from before it existed. Reading it early would have switched smart
  hyphens off for the whole pre-2004 corpus.
- **Conclusion: partial, and the negative half is the interesting half.** Twelve fields and both collections are
  recovered; three more are asserted as `LegacyBehavior` (the optional syllable positions off before selector `87`,
  the line width 224 before selector `67`, edge punctuation not ignored before Finale 2012). **Eleven fields are
  read from no era**, and they are invariant across every companion *and* absent from the framework tree, so
  neither source can separate them. The **Coda-banner epoch recovers nothing from its records for this class**,
  which is intended: it stores none of the six selectors usably.
- **Three of the eleven were then settled without a fixture, by the repository owner supplying a version
  boundary.** `hyphenChar`, `useAltHyphenFont` and `altHyphenFont` all postdate **Finale 2012**, the last release
  this reader opens, so no `.mus` file of any era has anywhere to put them. Nothing is read for them and nothing is
  overwritten, so they keep the baseline's values and are reported as `Finale27Default`. This is the second time a
  boundary the owner knew has closed a question the corpus could only ever have shown as "nothing varies" — which
  is what an absent option and an unfound one look like alike. Eight remain genuinely open.
- **Three owner corrections, and the rule they converge on.** `ValueOrigin::LegacyBehavior` marks a value the
  reader *asserts* on the strength of era knowledge — whether or not the pinned baseline happens to agree, because
  the baseline states one Finale 27 document's setting while the assertion states a fact about the formats.
  `Finale27Default` marks a value the reader inherits because it has nothing better. What decides between them is
  whether the reader writes the value, not whether the value differs from the baseline; `noHorizontalStretch` has
  always been the worked example and agrees with the baseline.
- **But a value the reader cannot state without duplicating the baseline stays inherited.** `useAltHyphenFont` is
  asserted false, because a boolean that is false through the non-existence of its feature can be written in code
  without restating anything. `hyphenChar` is not: writing U+002D beside a pinned resource that already says 45
  would be a second copy of one fact, which is the case the repository's rule is aimed at. So the two post-2012
  fields land on opposite sides — asserted `LegacyBehavior` and seeded `Finale27Default` — and the line between
  them is "can this be said without repeating the resource", not "is this known".
- Second, **`altHyphenFont` needed no work at all, and the first attempt at it was wrong.** It was built from the
  reference document's object and its comparator remapped through `musx::dom::importFontDefinitionInto`, which
  produced plausible results — each fixture resolving to its own music font, Petrucci in the Finale 1.0.0 and 3.7.2
  documents, Pmusic in the Finale 2.6.3 one, Maestro from Finale 2005 on. But the pinned baseline carries no
  `<altHyphenFont>` element either, so what was being copied was the reference's *own* `integrityCheck` placeholder:
  a value no document ever stated, imported into a document that had not asked for it. The reader now declines, and
  musxdom synthesizes the member after construction as it does for any document that omits it.
- **The detection question that settled it is reusable.** A seeded sub-object member is null during the import
  exactly when the baseline omitted its element, because musxdom populates such a member only from the element and
  otherwise synthesizes it in `integrityCheck`, which runs at `finish()` — after every importer. So "did the
  baseline state this?" is answerable from the pointer alone, with no access to the baseline's XML, which no
  importer has anyway.
- **Loose end deliberately not implemented:** in pre-2004 documents Finale 27 synthesizes the starting connection's
  vertical offset as 1 when the word-extension positioning bit is set and 4 when it is not. Six fixture groups agree
  and nothing else separates them; whether that is legacy behaviour or converter invention is **open**, so those
  documents keep the baseline's 1.
- **Next evidence:** [L1](EVIDENCE_REQUESTS.md) — one Finale 2005/2006 save moving as many of the eight still
  unlocated Lyric Options fields at once as the dialog exposes, after checking which of them that era's dialog
  offers at all: any it does not is a candidate for the same after-2012 answer the three struck-off fields got.
  Then a corpus survey, which this class has not had.
- **Artifacts:** `src/import/options/lyric_options.cpp`, `tests/reader_tests.cpp`,
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#lyric-options),
  [`data/lyric_options_mapping.csv`](data/lyric_options_mapping.csv).

## 2026-08-17 — Syllable edge punctuation, and the lineage confound that mimicked nine flags

- **Question:** How far back does "Ignore Syllable Edge Punctuation" go, and where does a file store it? The owner
  reported that Finale 2012 has the setting and Finale 2000 does not, leaving 2001-2011 unaccounted for.
- **Method:** Extracted `<lyricUseEdgePunctuation/>` and `<lyricPunctuationToIgnore>` from all 1,189 adjacent-exact
  companions of the reference corpus, tabulated by saving product, then searched the record stream of the cohort
  that varies for any word or bit partitioning it exactly. Deliverables in
  `private/generated/rpatters1-main/class_coverage/lyric_edge_punctuation/`.
- **Observation — the boundary is Finale 2012 itself.** Every release from Coda-banner through **Finale 2010**
  converts with punctuation not ignored, 941 documents with no exception, which extends the owner's Finale 2000
  observation by ten releases. Only Finale 2012 varies: 50 not ignored against 198 ignored. Finale 2011 is absent
  from this corpus; the installs survey holds 1,295 such documents and has not been run for this class.
- **Observation — the search returned thirteen exact answers, and nine were wrong.** Any bit partitioning the 248
  Finale 2012 documents is a candidate, and thirteen do, spread across fonts, stems, clefs and beams. The split is
  not about punctuation but about **document lineage**: the 198 that ignore it are born-in-2012 documents carrying
  that release's new defaults, and the 50 that do not are upgrades from older files. Already-mapped fields prove it,
  because `wordExtLineWidth`, the syllable positioning bits and the starting connection's vertical offset all
  partition the cohort identically and none of them is this setting.
- **What broke the confound was a negative control in an earlier release.** A pre-existing field already varies
  among Finale 2008 documents; a field arriving with Finale 2012 is identically zero throughout them. That cut
  thirteen candidates to four, of which only one is a boolean and only one sits in a lyric record: **selector `57`
  word 4**, class `0x0047` byte 8, the fourth field of the row already holding `smartHyphenStart`,
  `wordExtMinLength` and `wordExtOffsetToNotehead`. The other three survivors are a coordinate and two values
  reading as a percent and a mask.
- **Conclusion: confirmed.** The reader recovers the word from Finale 2012 and asserts the era's behaviour before
  it. Verified end to end through the public reader over the whole companion-backed corpus: **1,189 of 1,189 agree,
  zero read failures**, including all 248 Finale 2012 documents. This also corrects a real defect rather than only
  improving provenance -- the previous version gate left every Finale 2012 document at the baseline's *ignored*,
  which was wrong for the 50 that do not ignore.
- **Why it stays a version gate.** The record is twelve bytes in Finale 2007 and Finale 2012 alike, so its shape
  states nothing and only the release distinguishes them. The gate is bounded inside the zlib epoch and fails closed
  onto the pre-2012 behaviour, which is the right answer for every release but one.
- **Incidental:** the same sweep confirms selector `57` arrives with Finale 2004 across 941 documents, a gate
  previously resting on a handful of fixtures.
- **Follow-up, same day: `lyricPunctuationToIgnore` closed by the requested fixture, which refuted the prediction
  it was requested on.** The request predicted a cmper in selector 57 word 1 or 5, the two words zero in all 1,189
  companion-backed documents. It is instead a **variable-length tail on the selector 57 record itself**: with the
  list set to `#@%&` the record grows from twelve bytes to twenty-four, six scalars unchanged, then the characters
  as 16-bit code units and a zero. Finale writes the tail **only when the list differs from the stock set**, which
  is the whole reason the corpus was silent and the stock set appeared in no Finale 2012 file. An absent tail means
  the stock list, and the reader does nothing about it because musxdom's `integrityCheck` already owns that default.
  The decode reads word 6 to the first zero, so it is agnostic to how the record grows; the owner's guess that it
  expands in twelve-byte chunks fits the specimen, and the terminator rather than the chunking is what the decode
  depends on.
- **The fixture paid for itself twice.** Finale rewrote the word-extension connection table as the dialog closed,
  giving five of nine styles a vertical offset of 5 and a sixth an offset of 1 — a second non-default specimen for a
  collection that otherwise rested on one Finale 2006 document. It also renumbered two font definitions. Neither was
  asked for, and both are recorded in the fixture's provenance rather than treated as noise.
- **Follow-up: the checkbox-cleared save arrived and confirmed the corpus prediction exactly.**
  `F2012-lyropts-noign-punct.mus` moves byte 8 of class `0x0047` from 0 to 1 and moves no other word of that record,
  and its companion gains `<lyricUseEdgePunctuation/>`. This is the rarer kind of confirmation: the mapping was
  derived from 1,189 companions and a negative control *before* any fixture could exercise it, and the controlled
  save then landed on the predicted byte. It is also the only published document anywhere with the switch cleared,
  so the word-set path now has a real fixture instead of a synthetic record.
- **The pair separates two things that looked like one.** Ignoring is off in the new fixture and its list is stock,
  so its record stays twelve bytes, while the earlier fixture keeps ignoring on and grows to twenty-four. Finale
  writes the tail on the **list** differing from stock, not on the switch being touched.
- **Next evidence:** two corpus documents ignore punctuation while carrying no `<lyricPunctuationToIgnore>` element,
  which neither fixture explains. A Finale 2012 save clearing the list entirely, with ignoring left on, would say
  whether an empty list is representable.
- **Artifacts:** `src/import/options/lyric_options.cpp`, `tests/reader_tests.cpp`, `tests/mapping_tests.cpp`,
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#lyric-options),
  [`data/lyric_options_mapping.csv`](data/lyric_options_mapping.csv).

## 2026-08-17 — Lift and Push, and a six-group correlation that was a coincidence

- **Question:** The Coda-banner epoch recovered nothing at all for `LyricOptions`. The repository owner reported
  that its dialog exposes exactly two lyric settings, word extension "Lift" and "Push", and that Finale 3.7.2 adds
  hyphen spacing and line thickness to them. Where are they?
- **Method:** Two controlled one-variable saves, `F100-wext-push-6-lift-5.mus` and `F372-lyricopts-changed.mus`,
  each with a Finale 27 companion and an ETF, diffed at record granularity against their baselines.
- **Observation:** Lift is **selector `29` word 5** and Push is **selector `30` word 5**, and both exist in *every*
  epoch including the Coda banner. The Finale 1.0.0 pair moves those two words and no other word in the file; its
  companion reads 5 and 6 and its ETF prints the same two rows. The Finale 3.7.2 pair moves four words -- adding
  selector `15` word 1 for hyphen spacing and selector `67` word 5 for line thickness -- and its companion agrees
  with all four. Every previously tracked fixture agrees with its companion on both fields across all four epochs.
- **Conclusion: confirmed on three independent representations**, and the Coda-banner epoch now recovers exactly
  what its dialog exposes rather than nothing. The Finale 3.7.2 save is also the only tracked document anywhere
  that varies the hyphen separation, which promotes selector `15` word 1 from consistent-everywhere to confirmed.
- **A recorded correlation was a coincidence, and this is the lesson worth keeping.** Pre-Finale-2004 documents
  whose companions show a vertical offset of 1 rather than 4 had looked as though the value tracked the
  word-extension syllable positioning bit: six fixture groups agreed and no other record separated them. It was
  never a rule. Those documents store 1 in selector 29. The correlation was convincing for exactly as long as the
  real field was missing, and it was retired by a controlled save in the era with the fewest settings to confound
  it -- not by a better test on the same data.
- **It also replaced a derivation with a read.** The class-level offsets had been taken from selector `55`'s first
  element, which does not exist before Finale 2004; they now come from 29 and 30 in every era, and where selector 55
  is absent the starting connection takes those values, as Finale 27 does.
- **One synthesis deliberately not reproduced:** the companion moves the `oneEntryEnd` element's horizontal offset
  with Push, 42 to 44. A single specimen cannot distinguish that formula from others that fit, so the baseline's 42
  stands and the difference is intended.
- **Unexplained and recorded rather than smoothed over:** the Finale 3.7.2 save also moves selector `13` word 1 from
  1024 to 4096, which belongs to no field this reader maps, and respaces the lyric baseline details from 40 to 48.
- **Follow-up: the syllable positioning table closed too, and in two steps neither of which was a guess.** Finale
  2000 is the first release with a dedicated Lyric Options dialog, and that table is exactly what it adds to the
  four settings Finale 3.x already had. Two questions were open about it. The **order** of the last two positions
  was untested, because `first` and `systemStart` carry identical values in almost every document; five
  reference-corpus documents differ, and all five confirm the order in both directions, so no fixture was needed.
  The **`right` member** could never have been settled by a corpus at all: across all 1,189 companion pairs not one
  document uses it as an alignment or a justification, for any position.
  `F2000-lyropts-align-just.mus` supplies it twice over -- align 3 on the first syllable, justify 3 on the system
  start -- so a mapping that translated only one of the two fields would fail on it. Against its parent the only
  options record that moves is selector 87's second incidence. **All three members of the legacy alignment list are
  now verified against Finale's own conversion.**
- **A reminder that a corpus and a fixture answer different questions.** The order was a corpus question, because
  it needed documents that happened to disagree, and five existed among 1,189. The `right` value was a fixture
  question, because no number of documents helps when nobody ever chose the setting. Asking which kind a question
  is, before reaching for either tool, is the cheap step.
- **Artifacts:** `src/import/options/lyric_options.cpp`, `tests/reader_tests.cpp`,
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#lyric-options),
  [`data/lyric_options_mapping.csv`](data/lyric_options_mapping.csv).

## 2026-08-17 — The edge-punctuation boundary was Finale 2011, and the corpus never said 2012

- **Question:** The reader gated syllable edge punctuation at Finale 2012. The repository owner found release notes
  saying the new lyrics features arrived in **Finale 2011**, which would make the gate wrong and, worse, would put
  the punctuation list in a pre-Unicode release.
- **Method:** Corroborated online against MakeMusic's own manuals before looking at any data, at the owner's
  direction, then ran the Finale 2011 cohort of the installs survey — the population the reference corpus lacks.
- **Observation — the manuals bracket it from both sides.** The Finale 2010 Document Options-Lyrics dialog has
  neither "Ignore Syllable Edge Punctuation" nor a "Punctuation to Ignore" field; the Finale 2011 dialog has both,
  plus "Automatic Lyrics Numbers"; and the Finale 2012 manual's "Finale 2011 Interface Changes" page says the
  punctuation feature arrived in 2011 outright. The same page records that "Create Automatically When Notes Follow
  Without Lyrics" was renamed "Only Create on Lyrics with Underscores", which is the rewording the owner had
  described. The Finale 2011 What's New page independently confirms **Smart Hyphens and Word Extensions arrived in
  Finale 2004**, which is the boundary this reader already had from fixtures.
- **Observation — the installs survey confirms it at exactly that line.** All 22 companion-backed Finale 2010
  documents carry 0 in selector 57 word 4; all 597 companion-backed Finale 2011 documents carry 1. The gate moved
  to major 16.
- **How the wrong boundary got in, which is the part worth keeping.** The reference corpus contains **no Finale
  2011 document at all**. It has Finale 2010 and Finale 2012 and nothing between, so "the boundary is Finale 2012"
  was an interpolation across a gap presented as a measurement. The gap had even been written down at the time --
  the notes said "Finale 2011 is absent from this corpus" -- and the gate was still coded to the nearest release
  that happened to be present. A corpus that is silent about a release does not say the release is on either side
  of a line, and silence is easy to read as evidence when every document that *is* present agrees.
- **The punctuation list could not follow the switch, until the owner installed Finale 2011 and made the specimen.**
  0 of the 597 shipped Finale 2011 documents carries a tail, so the release's encoding was unknowable from any
  survey. `F2011-lyric-punct.mus` settles it: **the tail is packed 8-bit bytes in the platform code page**, where
  Finale 2012 writes one 16-bit code unit per character. Both containers are little-endian, so the two layouts are
  not variants of one rule -- reading the 2011 byte string through the word path transposes every pair of
  characters.
- **The two non-ASCII bytes are what turn the code page from an assumption into a measurement.** The list is
  `#@%&«»`; the tail is `23 40 25 26 c7 c8`; `0xc7 0xc8` is the guillemet pair in Mac Roman and `ÇÈ` in
  Windows-1252, and the companion reads the guillemets. The reader takes the bank from the document's own platform,
  as it does for a font definition carrying no charset, and this is the only specimen anywhere that tests that
  choice for this field. A fixture with an ASCII-only list would have confirmed the packing and left the encoding
  exactly as open as before.
- **A second corpus generalization corrected by the same fixture.** All 597 shipped Finale 2011 documents carry the
  switch set, and reading that as the release's default would have been wrong: `F2011-baseline.mus`, created new in
  Finale 2011, carries it clear. The 597 are Finale's own sample content, authored earlier and converted into the
  release, and conversion switches ignoring off to preserve the older look -- the same behaviour the Finale 2012
  cohort's 50 upgraded documents show. A corpus of one vendor's shipped content is not a sample of what a release
  writes for a new document.
- **Automatic lyric numbering, located the same day by three coded saves.** Neither survey could reach it --
  `lyricAutoNumType` is `align` in all 597 Finale 2011 documents and the three `showAutoNumbersOn...` flags are set
  in no document of any era, anywhere -- so it was always going to be a fixture question. Three booleans and an enum
  cannot be separated by saves that each move one thing, because four fields need four distinct signatures, so the
  three saves carried a binary code instead: the type moves only in the first, Verses in the first and second,
  Choruses in the first and third, Sections in all three. All four fields fell out unambiguously in **words 6 to 9
  of selector 58**.
- **And that record states its own layout, so it needs no version gate.** Selector 58 is six words in every era
  before Finale 2011 and twelve from it on; the installs survey splits at exactly that line, twelve bytes in all 22
  companion-backed Finale 2010 documents and twenty-four in all 597 Finale 2011 ones. The contrast with edge
  punctuation in the same class is the useful part: that field had to take a version range because its record does
  not change shape, and the range was wrong for a year of releases until the manuals corrected it. A record that
  changes shape asks nothing about which release wrote it.
- **Artifacts:** `src/import/options/lyric_options.cpp`, `tests/mapping_tests.cpp`,
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#lyric-options),
  [`data/lyric_options_mapping.csv`](data/lyric_options_mapping.csv).

## 2026-08-18 — The text pool: a format that spells its own class names

- **Question:** Where does each musxdom `texts` class come from, and what has to change about a legacy text block
  before musxdom can read it?
- **Method:** Dumped the fourth typed block of one fixture per epoch below the record layer, then held each chunk
  against the same-numbered element in that file's Finale 27 companion. Compared all 81 companion-backed tracked
  fixtures element by element once the reader existed.
- **Observation — the pool is prose, not records.** Every epoch from Finale 97 on stores it as `^keyword(n) ...
  ^end` chunks packed end to end, which is the shape ETF prints in its own text section. The keyword names the
  class and the number is the comparator. That is why one importer covers six classes: the file states which is
  which, so nothing has to be inferred from position or comparator range. The zlib epoch keeps it in block
  `0x0017`, which the notes had described only as "decoded strings ... and binary control data".
- **Observation — three things stop it being modern Enigma already.** The uncompressed epoch spells styles as a run
  of `^efx(name)`, which has to become one `^nfx(bits)`; the compressed epochs write commands in a binary form with
  a hexadecimal-digit argument offset by one; and the bytes are in a code page named by whichever font is in force,
  while EnigmaXML is UTF-8. Legacy line breaks are carriage returns and must become line feeds.
- **The parenthetical after a font name is the `FN` header word.** `^font(Engraver Text T,8194)` is `0x2002`, bank
  2 with character set 2, which is exactly the `FN(9)` header in the same file; `^font(Times,4096)` is `0x1000`
  and matches `FN(1)`. The value is redundant with the font definition it names, which is what makes it safe to
  drop rather than merely convenient.
- **Two encoding rules, and a fixture that separates them.** `F97-fileinfo-short.mus` sets fourteen expressions in
  font 0 and one in font 16, `Patmm`. Finale 27 converts the font-0 characters byte for byte and the `Patmm` `0xb0`
  to an infinity sign — so the music font's bytes are glyph numbers and `Patmm`'s are Mac Roman, even though the
  file records the same character set for both. Font id 0 is therefore treated as a symbol font on the strength of
  its id, which is the only statement a Finale 97 file makes about it.
- **Expression text used to live in its own record.** In the uncompressed epoch `DT` packs point size and font
  comparator into the two bytes of its first payload word, the style bits into the next, and the display text into
  every incidence after. By Finale 2006 that same embedded string is the expression's description and the display
  text has moved into the pool. Reading `DT` as display text in the wrong era would fill the texts pool with
  category descriptions, so the pass is gated to the uncompressed epoch; the fixtures say nothing about Finale
  2001 through 2005, which define no expressions at all.
- **Result against the companions.** All 81 fixtures agree on every recovered character. What remains is one
  spelling difference Finale makes inconsistently with itself — it normalizes a font command to `^fontid(n)` in a
  block text and a smart shape text but passes `^font(Name,charset)` through in an expression, from the same
  stored bytes — plus the Coda-banner epoch, which recovers nothing, and four Finale 2006 block texts for staves
  the document no longer has, which the reader keeps and Finale 27 discards.
- **Next evidence:** a document containing lyric chorus or section text, to replace the inferred keywords with
  observed ones; a bookmark, whose keyword is unknown; a Finale 2002-2005 document defining a text expression, to
  place the boundary where expression text moved into the pool; and a File Info string in any epoch after Finale
  97, to show whether the header offsets still hold.
- **Artifacts:** `src/import/texts/text_pool.cpp`, `src/import/texts/expression_text.cpp`,
  `src/import/texts/file_info_text.cpp`, `src/import/support/enigma_text.cpp`, `tests/text_pool_tests.cpp`,
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#the-text-pool).

## 2026-08-18 — Twenty-two binary command codes, from the one release that can tell

- **Question:** From Finale 2006 the MUS stores Enigma commands as a caret, a one-byte code, and a digit
  argument. Seven codes had been read off incidental fixtures. What are the rest?
- **Method:** Noticed that Finale 2006 is simultaneously the first release to write the binary form and the last
  to export ETF, and that its ETF spells every command out. That makes one document a crib for the other. Asked
  for `F2006-text-inserts.mus`: one text block per insert the Text Tool offers, each holding that insert alone so
  the block number keys the `.mus` and the `.etf` to each other. Its Finale 27 companion agrees with the ETF on
  all 21 records, so every pairing has two witnesses rather than one.
- **Observation — 25 codes, in one pass.** `0x81` baseline, `0x84` nfx, `0x85` fontid, `0x86` size, `0x87`
  superscript, `0x88` tracking, `0x8a` composer, `0x8b` copyright, `0x8c` date, `0x8d` fdate, `0x8e` dbflat,
  `0x8f` dbsharp, `0x90` description, `0x91` filename, `0x92` flat, `0x94` natural, `0x95` page, `0x96` sharp,
  `0x99` title, `0x9a` totpages, `0x9b` perftime, `0x9c` cprsym, `0x9d` value, `0x9e` control, `0x9f` pass. Before
  the fixture the reader dropped fifteen of those and named every one in a diagnostic, which is what made the
  request precise.
- **Three properties of the argument, each resting on one block and nothing else.** It is one value, not a
  sequence of shorter ones: block 6's page offset `01 01 01 01 01 02 02 04` is `0x113` and both witnesses write
  `^page(275)`, where two four-digit arguments would be 0 and 275 — so every earlier specimen, all offset by zero,
  was consistent with both readings. The digit range runs the full 0 to 15: block 21's `^superscript(15)` is
  `01 01 01 01 01 01 01 10`, the only nibble of `0xf` anywhere. And an argument is signed: block 19's
  `^baseline(-13)` is `0xfffffff3`, which read unsigned is 4294967283.
- **The signedness was a live defect, not a nicety.** A negative baseline is ordinary, and the first
  implementation would have written 4294967283 into every document that had one, with nothing to notice it. The
  reader now treats both widths as signed, including the four-digit one, where no negative has been observed: the
  encoding is one encoding, and a dialog clamping a field at zero is a fact about the dialog.
- **Two of these came from asking whether an assumption had a witness.** The `+1` digit offset had been
  extrapolated from digits 0 to 14; a scan of every fixture showed the highest byte ever stored was `0x0f`, so
  `0x10` was an inference. Chasing it produced the three style commands, and with them the only `0xf` nibble and
  the only negative arguments in any survey. It also exposed a second problem the scan alone had shown: run
  lengths are only ever 4 or 8, so the width belongs to the command, and a greedy scan for digit-range bytes would
  eat a literal `0x10` — perfectly possible in symbol-font text — as a fifth digit, losing both the character and
  the value.
- **The ordering is nearly alphabetical and then is not.** `0x8a` to `0x9a` run in alphabetical order with `fdate`
  the single exception, which it would not be if its internal name began with `date`; `perftime`, `cprsym`,
  `value`, `control` and `pass` follow in the order a later release would have appended them. `0x81` to `0x88` are
  the style commands and follow no order this reader can see. On that reading the four gaps inside the
  alphabetical run fall where `arranger` (`0x89`), `lyricist` (`0x93`), `subtitle` (`0x97`) and `time` (`0x98`)
  belong. **None of the four is in the reader.** A wrong
  name resolves to the wrong document field and reads as recovered content; an unlisted code is reported by number
  and reads as what it is, which is how this fixture came to be asked for in the first place.
- **A second finding the same fixture settled.** Finale 2006's Text Tool offers a `^description` insert and no
  `^lyricist`, `^arranger` or `^subtitle`, so File Info still has exactly four fields at that release. The three
  further types musxdom names are therefore not a gap in the header mapping for any era this reader covers.
- **Why none of this was ever visible.** The ETF export and the PDK both hand back the spelled-out form, so no
  plug-in could see the encoding: it exists only between Finale and its own `.mus`. The one place a binary command
  reaches an ETF at all is the Coda era, where `HT` is dumped as raw quoted payloads and its own — different,
  two-byte-argument — form leaks through because ETF is printing bytes rather than text.
- **Next evidence:** the same one-insert-per-block document from a release with the fuller File Info. It needs no
  ETF; the Finale 27 companion is the crib, and produced the first seven codes before this fixture existed.
- **Artifacts:** `src/import/support/enigma_text.cpp`, `tests/text_pool_tests.cpp`,
  `tests/evidence/F2006/F2006-text-inserts.mus`, [`FORMAT_NOTES.md`](FORMAT_NOTES.md#the-text-pool),
  [`EVIDENCE_REQUESTS.md`](EVIDENCE_REQUESTS.md).

## 2026-08-18 — Finale 2008 finishes the code table's shape, and refutes half a prediction

- **Question:** The Finale 2006 code table left four gaps inside its alphabetical run, at `0x89`, `0x93`, `0x97`
  and `0x98`, exactly where `arranger`, `lyricist`, `subtitle` and `time` would sort. Finale 2006 has none of
  those inserts, so it could not test that. Does a release that has them put them there?
- **Method:** `F2008-BE-text-inserts.mus`, the same one-insert-per-block document from Finale 2008, with all
  seven File Info fields filled in. Finale 2008 writes no ETF, so the Finale 27 companion is the only witness —
  which is what produced the first seven codes before either insert fixture existed.
- **Observation — no, and the reason generalizes.** `^time` is at `0x98` as predicted, but `^lyricist`,
  `^arranger` and `^subtitle` are at `0xa1`, `0xa2` and `0xa3`, appended past the end of the table alongside
  `^partname` at `0xa0`. Appending is the only thing a release can do when it adds an insert: renumbering would
  change what every already-saved document says. So the alphabetical run reflects one original alphabetized set
  and is not a rule still in force, and `0x89`, `0x93` and `0x97` are unexplained holes rather than reserved
  slots. `0x98` was a member Finale 2006 simply did not expose.
- **The refutation cost nothing, which was the point.** None of the four predictions was ever written into the
  reader. An unlisted code is reported by number and reads as a gap; a wrongly named one resolves to the wrong
  document field and reads as recovered content. This is the case that shows the difference is not academic.
- **Observation — a third argument width, and the one insert Finale 27 throws away.** `^time` takes a *single*
  digit where every previously observed argument was four or eight. Finale 27 emits no `^time` at all on
  conversion, so the companion cannot name it; the evidence is the fixture's own controlled pair, two blocks
  labelled "Time" and "Time with seconds" differing in that one byte, which is exactly musxdom's `^time` flag.
  The reader carries it forward. Finale 27 dropping an insert says what that conversion does, not what the
  document contains, and recovering the latter is the whole job: discarding a command the file states and musxdom
  can spell, on no better authority than a converter's choice, would be deleting content. The mapping is labelled
  strong rather than confirmed because of the missing witness, not because the decision is in doubt.
- **Observation — File Info leaves the header.** All seven fields are `^fileInfo(n)` records in the text pool,
  numbered by musxdom's own `FileInfoText::TextType` and confirmed field for field, while the header offsets that
  carry them in Finale 97 are empty. The reader needed no version boundary for this: the header pass now fills in
  only the types the pool did not supply, so each document states for itself which way it stores them. Where
  between Finale 2006 and Finale 2008 the move happened stays open, and does not have to be answered.
- **Two companion differences, both intended.** The fixture's first block is the bare text `FULL SCORE` with no
  style commands, and Finale 27 writes `^fontid(1)^size(12)^nfx(0)FULL SCORE`; the reader keeps what the file
  says rather than synthesizing a font the document never stated. And Finale 27 drops `^time`, where the reader
  carries it forward, for the reason above.
- **A byte-order specimen as a side effect.** This is the first big-endian Finale 2008 document in any survey.
  Its author produced it under Mac OS X 10.4 in QEMU because the Intel-era save crashed, which is a fair summary
  of why that half of the transition era is thin everywhere.
- **Next evidence:** whatever occupies `0x89`, `0x93`, `0x97`, `0x80`, `0x82`, `0x83`, and anything from `0xa4`
  up. Those are no longer predictable from the ordering, so they need a document that uses them rather than an
  argument about where they would sort.
- **Artifacts:** `src/import/support/enigma_text.cpp`, `src/import/texts/text_pool.cpp`,
  `src/import/texts/file_info_text.cpp`, `tests/text_pool_tests.cpp`,
  `tests/evidence/F2008/F2008-BE-text-inserts.mus`, [`FORMAT_NOTES.md`](FORMAT_NOTES.md#the-text-pool).

## 2026-08-18 — The last four codes, and a second prediction refuted the same way

- **Question:** four commands `musx/util/EnigmaString.h` documents had no binary code located: `^rehearsal` and
  the three font-category commands `^fontMus`, `^fontTxt` and `^fontNum`. Where are they?
- **Evidence:** `F2011-text-inserts.mus` with its Finale 27 companion. Finale 2009 introduced marking categories
  and Finale 2010 automatic rehearsal marks, so Finale 2011 is the earliest available release that can write all
  four in one document. One block text and four expressions, each carrying one command.
- **Result — the table is complete.** `0xa4` is `^fontTxt`, `0xa5` `^fontMus`, `0xa6` `^fontNum`, each with a
  four-digit argument, and `0xa7` is `^rehearsal` with none. Every command musxdom documents now has a code, and
  no fixture in the tracked corpus reports an unread code any more.
- **Refuted — the font-category commands are not in the style group.** Three free slots, `0x80`, `0x82` and
  `0x83`, sit between `^baseline` at `0x81` and `^nfx` at `0x84`, which is precisely where three font-category
  commands would belong; this reader said so in writing and did not act on it. They are appended past the last
  insert instead. That is the second prediction from the shape of the code table to be refuted by the first
  fixture able to test it, after the Finale 2008 one, and it generalizes the earlier lesson: **appending is what
  every release after the original set does, for style commands as much as for inserts.** Both groups are closed.
  Six slots stay empty in the reader.
- **Observation — a categorized font command states a comparator, not a name.** The three stored arguments are
  9, 11 and 11 where the companion writes `Times New Roman` and `Engraver Text T`, which are font definitions 9
  and 11. `^rehearsal` takes no argument, and the fixture shows it directly rather than by absence: the byte
  after the code is `0x20`, the leading space of the literal " Rehearsal", and `0x20` is not a digit byte.
- **Decision — a font reference is written under the font's name, and `^fontid` is the fallback.** Every font
  command resolves to a comparator first, and the comparator is written out as the name the document's own
  `FontDefinition` gives it; `^fontid` is what a comparator with no definition behind it becomes, being the one
  spelling that needs none. `^font` covers a plain reference whatever command the source used, and the three
  categorized commands keep their spelling, because the marking category they name is the one thing `^fontid`
  cannot carry. This makes the ordering constraint load-bearing rather than incidental: font definitions must be
  imported before any text, which the importer registry already does.
- **Follow-on:** every text fixture's expected strings changed with it, from `^fontid(4)` to `^font(Times)` and
  the like. The fallback had no fixture, so the synthetic cases now cover both halves of it: a comparator the
  document defines, one it does not, and a categorized command in each case.
- **The tripwire earned its keep.** Before the fix the reader reported `0xa4 0xa5 0xa6 0xa7` by number and
  dropped their text, rather than guessing a width and swallowing the following characters. The width-per-command
  table added for the Finale 2006 work is what made that possible.
- **Next evidence:** `0x80`, `0x82`, `0x83`, `0x89`, `0x93`, `0x97`, and anything from `0xa8` up. Nothing about
  the ordering predicts these, and two refutations say not to try.
- **Artifacts:** `src/import/support/enigma_text.cpp`, `tests/text_pool_tests.cpp`,
  `tests/evidence/F2011/F2011-text-inserts.mus`, [`FORMAT_NOTES.md`](FORMAT_NOTES.md#the-text-pool).

## 2026-08-18 — The Coda-banner pool walk, and how little text that era actually gives us

- **Defect:** the container's Coda-banner walk ended on the first pool with zero pages. A Finale 1.0.0 document
  reported **one** block where it has three, because its details pool is empty; its entries pool and the text
  region behind it were unreachable.
- **Fix:** an empty pool is an ordinary pool. The page size is the only thing that identifies a prologue, and the
  chain needs no terminator of its own — what follows the last pool is the text region, whose first four bytes
  are a chunk length rather than 0x200, so the page-size test ends the walk there anyway. The offset advances by
  the prologue even for an empty pool, so a run of them cannot spin.
- **Result:** `F100-baseline.mus` goes from 1 block to 3. No diagnostic changed on any fixture, all 84 still
  parse, and no existing test asserted the old count. A new synthetic case builds pools of {1, 0, 1} and {0, 0,
  0} and fails without the fix.
- **The finding that matters more than the fix.** The era's text region is two length-prefixed chunks, and they
  are empty in all 26 tracked documents — but that is not why recovery is blocked. Block text lives in the `HT`
  others family, and `F263-baseline.mus` holds it plainly: `TITLE`, `Licensed by ASCAP`, `One Lincoln Plaza`,
  `New York, NY  10023`, `All rights reserved.` and a composer and copyright line, in NUL-terminated runs
  alternating with binary layout rows. Finale 27 recovers all eleven blocks and seventeen expressions from it.
- **The corpus is thinner here than its count suggests, and this is the reason to ask for fixtures rather than
  keep looking.** All 21 Finale 1.0.0 documents contain no text at all — their companions carry only Finale 27's
  `Score` part name, which is PartDefinition's and is not yet imported — and the five Finale 2.6.3 documents are five saves of one document. The era has
  **one** text specimen. A companion names what the answer should be but cannot separate the text bytes from the
  layout bytes woven through them; only a controlled one-variable edit does that.
- **Observation — the two releases spell the region markers differently.** Finale 1.0.0 writes `^text \0` and
  `^lyric \0`; Finale 2.6.3 writes `^text()` and `^lyrics()`. Singular against plural, and a space-plus-NUL
  against parentheses. That is reason enough not to assume one `HT` layout covers both.
- **Observation — File Info's lower boundary is unanswerable from any corpus.** An unfilled dialog leaves the
  same empty header offsets as a dialog that does not exist, and only three tracked documents anywhere have the
  fields filled. The arrival may be around Finale 3.7 rather than earlier, which would make the header offsets
  meaningless for every release before it.
- **Next evidence:** X6 and X7 in [`EVIDENCE_REQUESTS.md`](EVIDENCE_REQUESTS.md).
- **Artifacts:** `src/container/mus_container.cpp`, `tests/reader_tests.cpp`,
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#the-coda-banner-epoch).

## 2026-08-18 — Four fixtures open the Coda-banner text, and a third pool framing appears

- **Question:** the previous entry ended with the pool walk fixed and the `HT` framing undecoded, and with the
  observation that the era had exactly one text specimen. Four Finale 1.0.0 documents were authored to answer it.
- **Result — block text is `HT` plus `HS`, one pair per block.** `HT` holds the characters in four consecutive
  incidences, 48 bytes, ending at the first NUL; `HS` holds the style, keyed so that incidence n describes the nth
  `HT` record of the same comparator. `HS` word 2 packs the font comparator above the point size, word 3 is the
  `nfx` mask, word 4 is an insert argument and word 5 selects the insert.
- **What proved style is not in `HT`:** two blocks of the Finale 2.6.3 document differ in size and style — 14/1
  against 12/0 — while sharing a byte-identical `HT` trailer. Whatever that trailer holds, it is not this.
- **The controlled pair did the work a companion cannot.** `F100-short-text` and `F100-long-text-w-insert` differ
  in one block's string, and exactly two records move between them. That is what says a block is those two records
  and nothing else, and what showed the record is a fixed four incidences either way, so a shortened string leaves
  the previous save's bytes behind the terminator.
- **One insert character, three inserts.** `#` converts to `^page`, `^date` or `^time` depending on `HS` word 5,
  with word 4 as the argument in each case. `F100-text-other-inserts` is what separates them. **Believed** rather
  than established: three observations cannot distinguish a two-bit field from two independent flags, so an
  unlisted value keeps the character and reports it.
- **Lyric text is elsewhere again** — in the text region behind the pools, spelled out, where the `^text` chunk
  beside it is empty even in documents that plainly have block text. `F100-lyric-text` is the only document of the
  era anywhere with lyrics, and it forced one converter fix: an `^efx` run written spaced apart is still one run
  and still one `^nfx`, with the spaces belonging before the command.
- **A third pool framing, from `F372-fileinfo-text`.** Finale 3.7.2 keeps one pool stream divided by the ETF
  section markers themselves, `^text` and `^lyrics`, and **a record has no terminator**: it runs to the next
  record, to the next marker, or to the end of the stream. Finale 97 drops the markers and closes records with
  `^end`. The reader tells them apart by reading the opening bytes, because the boundary falls inside the
  uncompressed epoch and no epoch gate can express it.
- **Observation — File Info is bounded, not dated.** That release carries all four fields at the header offsets
  and is the earliest available whose dialog offers them. That is a ceiling on the arrival and not the arrival:
  no earlier release is available to test, and no corpus can settle it at any size, because an unfilled dialog
  leaves the same empty offsets as a dialog that does not exist.
- **A caution the fixture earned.** An intermediate save of that document lost both block texts, from the MUS and
  the ETF alike, leaving five lyric records where seven text items had been entered. The file was structurally
  valid and nothing announced the loss. Finale of this vintage under emulation can corrupt a document silently.
- **Artifacts:** `src/import/texts/coda_texts.cpp`, `src/import/texts/text_pool.cpp`,
  `src/import/support/enigma_text.cpp`, `tests/text_pool_tests.cpp`,
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#the-coda-banner-epoch).

## 2026-08-18 — Bookmarks, and two deferrals stated as deferrals

- **Question:** `BookmarkText` was the one text class with no located source. Does Finale 2012 pool it, and is the
  pooled text Unicode?
- **Result — yes to both.** From Finale 2012 a bookmark is an ordinary text-pool record, keyword `bookmark`,
  `^end`-terminated and carrying no style commands. Its text is UTF-8, shown rather than inferred: `c3 bc` is the
  u-umlaut of `Page über` and `c2 ab c2 bb` the guillemet pair of `Scroll «» Bookmark`. The same characters are
  one byte each in every earlier era, which is what makes it a measurement — the guillemets settled the lyric
  punctuation code page the same way, as `c7 c8` in Mac Roman.
- **Before that it is the `BK` others family**, comparators from 0x8000 up, in the same shape the Coda era uses
  for block text: 48 bytes of string across four incidences, then two numeric incidences. The text pool of such a
  document holds no bookmark at all.
- **Two deferrals, both asserted rather than assumed.** `BK` is not read until the bookmark class is imported, and
  the `DT` expression text of the fixed-row eras is not read until `TextExpressionDef` is. In both cases the text
  without the class behind it would claim more coverage than it has. The synthesis that existed for `DT` was
  removed rather than switched off, and tests assert that both eras produce nothing, so reinstating either is a
  deliberate act.
- **Unverified: that the move into the pool belongs to the Unicode project.** It fits, but the boundary has not
  been tested inside Finale 2012, and a point release may have changed it. Nothing turns on it while the reader
  takes whichever form the document presents — a missing answer is possible, a wrong one is not.
- **Observation — comparators are not stable across an upgrade.** The same two bookmarks are 1 and 2 before and
  2 and 3 after.
- **Artifacts:** `src/import/texts/text_pool.cpp`, `tests/text_pool_tests.cpp`,
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#bookmarks).

## 2026-08-21 — The first word-extension connection table has eight entries

- **Question:** why did one Finale 2004 source disagree with its Finale 27 companion on six fields in selector
  `55`'s word-extension connection table?
- **Result:** the source has four incidences, exactly 24 words or eight complete three-word elements. Reading those
  eight elements in the established style order reproduces every companion connection point and offset. The
  previous nine-element assumption admitted only the later 30-word layout and caused the early payload to be
  interpreted from seeded values rather than from its own bytes.
- **Structural boundary:** 24 words carries the first eight style types and omits `zeroOffset`; 27 or more words
  carries all nine, with the observed fixed-row form using 30 words and leaving its final three as padding. The
  importer now selects the layout from that payload length rather than from the saving version.
- **Validation:** a focused synthetic test exercises both shapes, and a one-document recovery probe reproduces all
  nine companion values with no lyric-options differences. The ninth value in the early document remains the
  pinned baseline because no source element states it.
- **Artifacts:** `src/import/options/lyric_options.cpp`, `tests/mapping_tests.cpp`, and
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#two-collections-and-the-two-orders-that-do-not-match-musxdom).

## 2026-08-21 — FontOptions semantic companion comparison deferred

- **Current limitation:** `tools/coverage/surveyors/options/font_options.cpp` obtains its tuple
  list from `ImportReport` field provenance. Independently parsed companions have no such
  provenance, so they currently report no FontOptions tuples and
  `scripts/recovery_coverage_report.py` excludes `font_options.tuples` from comparison.
- **Deferred design:** emit each document's actual FontOptions values independently of optional
  source provenance, identify entries by FontType, and compare normalized font identity, size,
  and effects. Source-only provenance can remain attached as diagnostic metadata.
- **Acceptance criterion:** source and companion observations both contain every actual
  FontOptions entry; semantic comparison reports missing types, unresolved fonts, and value
  differences; and the tuple exclusion in `recovery_coverage_report.py` is removed.
- **Historical implementation:** the deleted standalone analyzer remains available as
  `git show f3b8905:scripts/font_options_coverage.py` from main. Its direct corpus inventory,
  subprocess, MUSX parsing, hard-coded FontType count, and duplicate normalization logic are
  obsolete; only the semantic comparison it attempted should be carried forward.

## 2026-08-22 — TextBlock is stored after Coda and assembled inside Coda

- **Question:** whether public `EDTTextBlock` describes a disk record, and how the same class can
  identify Coda block text when that epoch has no `TX` family.
- **Post-Coda result:** `TX` and zlib class `0x00b7` share the public structure's first twelve
  words. Controlled fixed-row, DCL, and big-endian zlib fixtures reproduce the companions'
  scalar fields, conditional line-spacing member, justification, flags, and high-word-first
  Efix values. The public declaration remains a hint; its first twelve words are now independently
  verified format facts.
- **Coda result:** `EDTTextBlock` is synthetic there. Neither the MUS record index nor the era's
  ETF exports contain `TX`. Each `HS` incidence describes the corresponding ordered `HT` block;
  its low flag bits carry justification, and the companion supplies the era's invariant remaining
  behavior. Text recovery owns allocation of the musxdom text number, and TextBlock recovery reads
  that finished object rather than reimplementing the ordering.
- **Refuted lead:** raw searches initially appeared inconsistent with a direct Coda `TX` record.
  Record dumps and ETF exports settled the issue: there is no such family, and the apparent modern
  record was the companion's synthetic representation.
- **Upgrade behavior:** Finale may insert a shape block, duplicate a page-number text, synthesize
  expression blocks, and assign different modern cmpers to synthetic Coda TextBlocks. Semantic
  text plus layout describes that transformation but is not TextBlock identity. The maintained
  report compares TextBlocks by cmper and consults each side's referenced Enigma text only to
  classify the paired block. Semantically matching referents make a `textId` change equivalent;
  nonmatching referents, zero-id absence, and a combined text-family/id reassignment with one
  resolvable referent classify the block as Finale renumbering and suppress its remaining leaf
  differences. This classification applies across all eras. Text comparison and its detailed
  difference table remain exclusively owned by the texts pool.
- **Broad capture:** the immutable three-survey recovery snapshot contains 15,938 occurrences:
  15,883 imported successfully,
  4,490 had companions, and the represented epochs were 4,247 zlib, 10,302 DCL, 1,028
  uncompressed, 306 Coda-banner, and 55 failed/unclassified. Counts are occurrences unless stated
  otherwise. Among Coda sources, 594 blocks in 83 distinct documents had exact semantic-layout
  matches, 520 with changed IDs; seven blocks in seven documents lacked a semantic match.
- **Follow-up — `textType` is word 12:** Finale 2003 component documents store zero there. Finale
  2004 and 2005 component documents store packed `bl` on block TextBlocks and `xp` on expression
  TextBlocks. The controlled Finale 2006 graphic fixture changes the representation to decimal
  `2004` for all 17 block TextBlocks and `2006` for all 41 expression TextBlocks; the controlled
  Finale 2008 block-only fixture retains `2004`. Thus the user's proposed appended field and
  Finale 2004 boundary are confirmed, while the raw encoding has two generations. The importer
  selects from either recognized stored value rather than from the document version. This
  resolves the expression-text ownership that was previously left for `TextExpressionDef`.
- **Follow-up — corners are legacy behavior, not a decoded location:** Finale 2012 has no
  rounded-corner option, establishing that the feature postdates the legacy MUS era. The
  three-survey capture independently has seven rounded companion TextBlocks, all with radius
  512: six reserved high-comparator objects absent from the source and one expression TextBlock
  added during upgrade. Every companion TextBlock corresponding to a source block has square
  corners and radius zero. The importer now assigns and reports those values as
  `LegacyBehavior`; the discarded `0x2000` and words 13–14 hypothesis was attempting to locate
  fields that the source format never stored.
- **Open Coda reference infrastructure:** the existing shared `HS`/`HT` walk already resolves a
  synthesized TextBlock inward to its BlockText without a separate map. Outward references from
  staff/group names, page or measure text, text expressions, and similar records are different:
  their legacy reference token has not been identified. Those importers will require one
  document-level legacy-token-to-synthesized-TextBlock-cmper map; none may independently count
  blocks or use semantic text as identity. Stored post-Coda `TX` cmpers remain direct identities.
- **Artifacts:** `src/import/others/text_blocks.cpp`,
  `tools/coverage/surveyors/others/text_blocks.cpp`, `tests/mapping_tests.cpp`,
  `tests/text_pool_tests.cpp`, and
  [`FORMAT_NOTES.md`](FORMAT_NOTES.md#textblock-attributes).

## 2026-08-24 — RepeatOptions document staff-list reference

- **Question:** Where does `RepeatOptions::showOnStaffListNumber` live, without yet decoding the repeat staff-list
  families it references?
- **Framework lead:** The current authorized private framework maps the field to numeric global selector `72`,
  comparator `65534`, incidence 1, word 2. The zlib numeric-global bridge predicts class `0x0056`, payload byte
  offset 16. The older framework snapshot supplied for the investigation has no corresponding row.
- **Method:** Used the reader's container and normalized record index in a disposable read-only `/tmp` probe. Scanned
  all 15,941 inventoried paths from the three registered surveys (6,984 distinct content identities), recording the
  size of every selector `72` and class `0x0056` globals family. Independently searched all 4,379 available Finale 27
  companions for `<showOnStaffListNumber>`.
- **Observation:** No source stores incidence 1. Every observed family is exactly 12 bytes, incidence 0 only. The
  selector is absent throughout Coda-banner and in the observed Finale 3.0–3.2 files. No companion contains the
  target XML element, although repeat staff-list objects themselves are common. The scan includes little-endian
  Windows Coda-banner files and both byte orders in the later epochs where available.
- **Conclusion:** The framework row is a **private-framework-derived current-format locator**, not a verified legacy
  location. Applying it to Finale 1.0–2012 would recover nothing. The legacy location, or alternatively fixed legacy
  top-staff behavior with no stored field, remains **open**.
- **Next evidence:** In one legacy Finale release, save a baseline and a copy differing only by selecting repeat staff
  list 1 in the document-level repeat options. An ETF from Finale 2006 or earlier is useful but not required; a modern
  companion should verify the semantic value. Do not alter the staff-list membership between the pair.

### Implementation and Finale 2005 follow-up

- **Implemented slice:** `RepeatOptions` now imports 22 scalar fields from the seven located numeric globals in the
  uncompressed, DCL, and zlib epochs. The older zero maximum-pass sentinel becomes 20. The post-legacy
  `bracketEndAnchorThinLine` setting is asserted false as `LegacyBehavior`. The Coda-banner and Finale 3.0–3.2
  layouts remain explicitly uncovered.
- **Coverage funnel:** A fresh Release capture processed all 15,941 occurrences from `rpatters1-main`,
  `rpatters1-installs`, and `tracked-evidence`: 6,984 distinct sources, 15,886 successful imports, 55 known failures,
  and 4,493 successful companion comparisons. RepeatOptions had 111,515 equal leaves and 810 unexpected leaves.
  Every DCL and zlib leaf agreed, as did every uncompressed document from Finale 3.7 onward. The differences were
  confined to 130 distinct Coda-banner sources and six distinct Finale 3.0–3.2 sources, whose layout is not claimed.
- **Corrected controlled Finale 2005 pair:** Selecting repeat staff list 1 leaves all seven RepeatOptions selector
  rows identical, including the sole `72(65534)` incidence. The staff-list MUS has exactly five additional physical
  other rows: one `DC` score-membership row, two `Dc` name incidences, one `dc` parts-membership row, and one `io`
  parts-override row. The corresponding ETF reports the same records and no changed repeat object. The checked-in
  Finale 27 companion predates this corrected save and is not evidence for the corrected pair; the fixture author
  independently confirmed that Finale 27 does not upgrade the legacy selection into the document-level option.
- **Conclusion:** `showOnStaffListNumber` remains unlocated in every legacy epoch. The Finale 2005 edit creates the
  repeat staff-list family but supplies no stored pointer to it in RepeatOptions. Whether legacy Finale treated a
  particular list identity or the mere presence of this family as an implicit selection remains open. Staff-list
  decoding is deferred.

### Pre-layout companion behavior

- **Unmasked observation:** Across 136 successful companion-backed documents without selector `72`, six differences
  are uniform: the companions produce `addPeriod` false, `thinLineWidth` 224, `upperDotVPos` and `lowerDotVPos` zero,
  `bracketLineWidth` 224, and `bracketEndAnchorThinLine` false. The reader had supplied the conflicting Finale 27
  defaults because the structural gate correctly found no source RepeatOptions family.
- **Implementation:** Those six values are now reported as `LegacyBehavior` when the family is absent. No MUS byte is
  claimed for them, and the same structural marker continues to select the later recoverable layout.
- **Contradiction retained:** `bracketHeight` is not uniform. Of the same 136 comparisons, 116 companions produce 144
  and 20 produce 72. The 20 are Windows Finale 2.2 tutorial files, but four other files from that same installation
  produce 144, leaving no version or platform gate. The field stays at the pinned default with the split **open**.
  After review, these 136 leaves were classified as `different_defaults`; the umbrella does not resolve the split
  and admits future individually reviewed baseline-default differences.

### Finale 2012 implicit repeat-list selection

- **Controlled pair:** From the tracked Finale 2012 baseline, selecting repeat staff list 1 leaves numeric-global
  class `0x0056` byte-for-byte unchanged at its sole 12-byte payload. Apart from Finale renumbering existing font
  definitions, the edit adds only three class records, all at cmper 1: `0x00e1` contains the UTF-8 name `Staff List
  1`, `0x00e2` contains parts member `-3`, and `0x00e4` contains score members `-1` and `-2`.
- **Semantic reference:** The Finale 27 companion maps those records to `repeatStaffListName`,
  `repeatStaffListParts`, and `repeatStaffListScore`, but writes `<showOnTopStaffOnly/>` and no
  `showOnStaffListNumber`. This is an upgrade transformation rather than evidence for the source setting. The
  fixture author reports that Finale 2006–2012 instead upgraded the old selection to All Staves and that a later
  release drifted to Top Staff Only; no tested Finale version preserves the selection as-is.
- **Three-list discriminator:** A second Finale 2012 save contains three repeat staff lists and selects list 2. Relative
  to the one-list/list-1 save, the complete normalized record set adds only seven staff-list component records:
  names `0x00e1` at cmpers 2 and 3, parts memberships `0x00e2` at cmpers 2 and 3, score memberships `0x00e4` at
  cmpers 2 and 3, and score override `0x00e5` at cmper 2. The companion independently identifies `0x00e5` as
  `repeatStaffListScoreOverride`; it is not a selection marker. Class `0x0056` and every record outside the list
  families are unchanged.
- **Revised conclusion:** The cmper-1 coincidence is disproved. Selecting list 1 or list 2 does not persist a list
  number in the observed Finale 2012 MUS representation. Combined with the corrected Finale 2005 result, this is
  **strong** evidence that `showOnStaffListNumber` has no legacy MUS location in scope, rather than an implicit
  reference through a distinguished list identity.
- **Implementation consequence:** Do not reproduce either converter fallback and do not synthesize a legacy staff-list
  reference from list presence or cmper. Leave both staff-display fields at the pinned default. Repeat staff-list
  objects may be imported later without treating one of them as the document option's selected list.

## 2026-08-24 — Recovery-coverage registration and schema 3

- **Problem:** Adding `repeat_options` to the C++ surveyor registry did not add it to the report's independently
  maintained Python class-to-pool table. A third C++ type-to-origin router separately repeated class knowledge. The
  probe therefore observed the class while one report section silently omitted it.
- **Correction:** Each `COVERAGE_SURVEYOR` invocation now registers the class key and musxdom pool together. The
  comparison output carries that hierarchy in compact schema 3, and the Python report renders it without a class
  registry of its own. Text comparison also selects classes from the registered `texts` pool. Surveyors query the
  typed `ImportReport` maps directly, eliminating the separate origin router.
- **Origin audit:** Direct typed lookup exposed field-specific legacy origins on font charset leaves that the former
  object-level font origin obscured. The already-recognized `baseline-font` classification now names only the exact
  observed legacy-to-companion charset bank/value pairs rather than treating every font field alike.
- **All-corpora result:** With Finale 27 `MacSymbolFonts.txt` supplied, schema 3 processed 15,945 rows: 15,890 imports
  succeeded, 55 known non-document/library inputs failed, and all 4,497 companion comparisons succeeded. Across all
  pools there were zero unexpected differences. Enigma-text findings contained zero `other` differences.

## 2026-08-24 — SmartShapeOptions structural families

- **Question:** Which legacy preference rows populate `SmartShapeOptions`, and which apparent early rows are name
  collisions rather than the later option layout?
- **Lead:** The current authorized private framework's Smart Shape preference map names the `FI`, `50`–`53`,
  `92`–`93`, and `97` families. These locators began as **private-framework-derived** evidence and were checked
  against the project's distilled option-mapping table before any decoder was written.
- **Controlled evidence:** Record dumps from Finale 1.0.0, 2.6.3, 3.7.2, 2000, 2002, 2005, 2011, and 2012 were
  compared with their semantic companions. Finale 3.7.2 supplies `FI` and the twelve-word selector `52` contours;
  Finale 2000 adds selectors `92` and `93`; Finale 2002 adds selector `97`, at which point selectors `50`, `51`,
  and `53` agree with the modern scalar meanings. The 2011 and 2012 class records preserve the same payloads under
  the numeric-global class bridge, while named `FI` becomes class `0x008d`.
- **Refuted assumption:** Selector spelling is not enough to extend the later layout backward. The Coda-banner files
  contain some of `50`–`53`, but selector `52` has only six words with floating-point-shaped values and the other
  rows disagree with the later meanings. The modern companions manufacture complete Smart Shape defaults, so they
  cannot prove that those defaults came from the early MUS records.
- **Implementation:** `smart_shape_options.cpp` gates contours on the exact twelve-word family, the later slur
  scalars on selector `97`, the custom-line group on selector `92`, and the figure group on `FI`/class `0x008d`.
  It recovers 38 scalar fields plus four contours across the uncompressed, DCL, and zlib epochs. Coda remains
  explicitly uncovered and retains the pinned defaults.
- **Open evidence:** The locations and semantics of any Smart Shape preferences actually stored by Finale 2.6.3,
  four scalar fields, and four connection-style collections remain unknown. At this stage custom-line identifiers
  were recovered but the pre-capability tool references had not yet been repaired.
- **First coverage review:** The 104 controlled companion pairs produced 356 unclassified SmartShapeOptions leaves.
  The 155 Coda-banner leaves were reviewed as upgrade/default differences and classified only where their source
  origin is `Finale27Default`; the approval does not extend to any later epoch. Raw selector `53` inspection exposed
  a decoder error in the remaining population: words 2 through 4 are vertical break adjustment, the avoid-lines
  flag, and padding, respectively. The first four words persist in the earlier fixed-row layout even though its
  other slur selectors differ, so that smaller layout is selected by the twelve-word contour family.

## 2026-08-25 — Pre-custom-line Smart Shape defaults

- **Capability boundary:** The established `ls` census places custom-line definitions at internal major version 5.
  The fallback treats the complete Coda-banner epoch as earlier and applies the version comparison only inside the
  uncompressed epoch. DCL and zlib are wholly later; an unknown epoch is not guessed into the fallback.
- **Implementation:** Before that boundary, `SmartShapeOptions` requests the pinned baseline's glissando, tab-slide,
  and guitar-bend lines in semantic order. Deferred resolution uses musxdom's custom-line importer, including its
  existing shape, font, and generic raw-text importers, and assigns every imported object `ShareMode::All`.
- **Comparator result:** The controlled pre-2000 and Coda fixtures contain no source custom-line definitions. Their
  imported definitions therefore receive comparators 1, 2, and 3, and the three Smart Shape option fields receive
  those same values. Finale 2000 is on the custom-line side of this gate and retains its two source-owned lines;
  the later guitar-bend investigation below adds its separate tool-specific fallback.
- **Focused verification:** The mapping gate test covers Coda despite a bogus version, uncompressed majors 4 and 5,
  and a deliberately contradictory DCL major 4. The reader test checks the resulting three line objects and option
  comparators in the pre-2000 and Coda fixtures while retaining Finale 2000's two source-owned objects.
- **Imported-object provenance:** musxdom's font, shape, raw-text, and custom-line importers now accept one callback
  that is forwarded through nested imports and invoked for every newly created pool object. The reader records the
  baseline origin once on each such object; the coverage observation inherits that origin through every descendant
  leaf unless a field-specific origin overrides it. Reused target objects are not reported as imports.
- **Tracked-evidence result:** A fresh 104-document capture completed with 104 successful companion comparisons.
  All 95 unexpected custom-line-style leaves now identify their origin as `Finale27Default`, including the observed
  `solidWidth` 115-to-224 differences. The 272 SmartShapeOptions differences remain unclassified pending individual
  review. After separate review, any differing custom-line `solidWidth` inherited from `Finale27Default` is
  classified as `different_defaults` without constraining its value or source epoch; provenance alone remains
  insufficient for every other custom-line leaf. This classifies 90 leaves; five `charFontSize` differences remain
  unclassified in that capture. Subsequent review applies the same origin-only rule to custom-line `charFontSize`;
  the 108-document capture contains six such Finale 97 differences, all inherited from `Finale27Default`. The same
  origin-only treatment also applies to seven reviewed `SmartShapeOptions` leaves:
  `crescHorizontal`, `crescLineWidth`, `slurAvoidStaffLines`,
  `slurLeftBreakHorzAdj`, `smartLineWidth`, and `useEngraverSlurs`. A difference in any of those fields
  is `different_defaults` only when its source origin is `Finale27Default`.

## 2026-08-25 — Finale 2.6.3 curve-options discriminator

- **Question:** Does a controlled edit in Finale 2.6.3's curve-options dialog survive conversion into modern
  `SmartShapeOptions`?
- **Controlled pair:** `mus-e0d207b83365f47d` was saved from `mus-ec910badddfa6699`. A complete fixed-global
  record diff found changes only in selectors `15`, `35`, `50`, and `51`; the modified ETF independently gives
  the same values. Selector `50` changes from `(0, 0, 0, 0, 8, -8)` to `(1, 3, 5, 7, 11, -11)`, and selector
  `51` from `(0, -6, 0, -6, 0, 8)` to `(13, -13, 17, -17, 3, 5)`.
- **Companion result:** An exact semantic comparison of the two Finale 27 companions finds only two changed
  `SmartShapeOptions` leaves: `slurThicknessCp1Y` and `slurThicknessCp2Y`, each 6 to 13. Other parts of the old
  dialog are distributed into `LineCurveOptions`, `PianoBraceBracketOptions`, and `TieOptions`.
- **Reader result:** A fresh two-row recovery capture completed with two successful Coda-banner imports and two
  successful companion comparisons. The reader currently leaves both thickness values at the pinned baseline's
  6, with `Finale27Default` origin, so the modified document reports two additional unexpected differences at 13.
- **Discriminator:** `mus-d2076c77b9f3f65d` reverses the C1 displacement signs and gives the two sides unequal
  magnitudes. Selector `51` begins `(-13, 17)`, and both modern thickness leaves become -17. This confirms that
  Finale 27's upgrade discards selector `51` words 0, 2, and 3 rather than showing that the source did not use
  them. The fixture also changes non-PostScript tie settings in selectors `17` and `22`, but an exact subtree
  comparison finds no additional `SmartShapeOptions` change.
- **Rendered discriminator:** `mus-ba0652c39a4fd350` carries an actual legacy curve and gives selector `51` the
  first four words `(131, -171, -173, 179)`. Comparing its appearance before and after upgrade shows that Finale
  27's discarded horizontal values and duplicated first vertical value visibly distort the curve. A manually
  corrected modern companion, SHA-256
  `e8f361373c9962ddae60844da1b597ecaf19494c0c05194d85d654333ee26e95`, stores the four modern
  fields as `(131, 171, -173, -179)` and reproduces the legacy appearance.
- **Implementation:** The Coda-banner table treats selector `51` words 0–3 as two `(horizontal, vertical)` pairs.
  Horizontal values map directly to the corresponding thickness-control X field; vertical values map sign-inverted
  to the Y field. This deliberately preserves source geometry rather than reproducing Finale 27's lossy upgrade.
  All other Coda Smart Shape scalars remain unsupported. After review, the nine differing `Cp1X`, `Cp2X`, and
  `Cp2Y` leaves in the three edited fixtures are classified as `finale-upgrade-loss`; the rule requires
  Coda-banner epoch and `LegacyMus` origin and does not include `Cp1Y`, which Finale upgrades correctly.

## 2026-08-25 — Finale 2002 Engraver-Slur contour boundary

- **Question:** Is selector `52`'s fourth tuple a source-owned extra-long contour before Finale 2002,
  and does selector `53` word 3 already mean `slurAvoidStaffLines`?
- **Public lead:** MakeMusic's Finale 2012 documentation says Finale 3.5 through Finale 2001 files
  open with Engraver Slurs disabled and that slurs from Finale 2001 or earlier are not converted
  automatically to the new Engraver Slurs. This is **public-source-derived** evidence, accessed
  2026-08-25:
  [Smart Shape/Slur Direction](https://usermanuals.finalemusic.com/Finale2012Win/Content/Finale/Smart_ShapeOSlur_Direction.htm)
  and [Importing](https://usermanuals.finalemusic.com/Finale2012Win/Content/Finale/Importing5.htm).
- **Corpus boundary:** An aggregate record sweep found one pattern in all 63 installed Finale 2001
  documents: selector `52` tuple four `(0, 0, 0)` and selector `53` word 3 `0`. All 774 Finale 2002
  documents instead have a real fourth tuple and word 3 `1`; their three observed tuples are
  `(1152, 307, 72)`, `(1152, 341, 72)`, and `(1152, 369, 80)`. This independently confirms the
  release boundary without using the marketing version as the decoder gate.
- **Controlled discriminator:** `F97-slurtieopts-changed.mus`, made from the Finale 97 baseline,
  changes the short, medium, and long selector-`52` tuples to `(36, 532, 13)`, `(288, 553, 43)`,
  and `(864, 358, 73)`. Its fourth tuple remains zero. Finale 27 produces those three contours plus
  extra-long `(1152, 358, 73)`: span comes from the later baseline, while inset and height copy the
  source long contour. The companion's separately edited tie-control spans change only `TieOptions`.
- **Implementation:** Selector `97`, already the structural marker for the enhanced slur scalar
  family, now gates both the fourth contour tuple and selector `53` word 3. Before it appears, the
  reader recovers three contours, retains the seeded extra-long span as `Finale27Default`, copies
  long inset and height as `LegacyBehavior`, and retains the seeded avoid-staff-lines value. From
  selector `97` onward, all four tuples and the flag remain `LegacyMus`.

## 2026-08-25 — Pre-2002 slur-thickness source discriminators

- **Question:** Does Finale 27 synthesize pre-2002 Smart Shape thickness-control values from the
  document's tie-thickness options?
- **Controlled pair:** `F2000-tieopts-changed.mus` was saved from `F2000-empty.mus`; both are 8,200
  bytes. The fixed-global options diff changes only selector 84 incidence 0, with `thicknessRight`
  moving from 6 to -17 and `thicknessLeft` from 6 to 11. Two ETF detail families, `#c2` through
  `#c10` and `#s2` through `#s10`, also change, so the fixture is controlled for tie-option edits
  rather than for one isolated leaf. No Smart Shape preference selector changes.
- **Companion result:** An exact comparison finds no changed `SmartShapeOptions` leaf. In particular,
  `slurThicknessCp1Y` and `slurThicknessCp2Y` remain 6 in both companions, while `TieOptions`
  alone receives the edited thickness values 11 and -17.
- **Conclusion:** This rejects direct copying from the editable tie-thickness options.
- **Inset discriminator:** `F2000-tie-insets.mus` switches the ordinary tie contours from percentage
  to fixed insets and changes their cp1/cp2 fixed insets from 8/8 to 17/13. The raw diff is confined
  to the inset-mode word in selector 84 and the six corresponding inset positions in selector 86.
  Finale 27 preserves those edits in `TieOptions` but again leaves both Smart Shape values at 6.
  Selector 86 is therefore also excluded.
- **Direct discriminator:** `F2000-slur-thickness.mus` changes the Slur Thickness control from 6
  to 17. The complete record diff changes exactly selector 59 incidence 0 word 5, and its companion
  changes exactly `slurThicknessCp1Y` and `slurThicknessCp2Y` from 6 to 17. No `TieOptions` leaf
  changes. Selector 59 word 5 is therefore the single stored pre-Engraver-Slur thickness, copied
  into both modern vertical control-point fields. The earlier correlations with tie thickness,
  tie-placement offsets, and default music-font size were shared template defaults rather than the
  upgrade source.
- **Implementation:** In fixed-row files with the twelve-word selector-52 contour family and no
  selector 97, the Smart Shape importer recovers selector 59 word 5 into both vertical thickness
  controls. The same structural marker already selects the era's three-contour and early-adjustment
  layout, so the rule does not depend on a marketing version.

## 2026-08-25 — Single-incidence enhanced-slur behavior

- **Structural boundary:** Finale 2002 contains selector `97`, establishing the enhanced-slur
  scalar family, but selector `53` still has only its first six-word incidence. Finale 2003 adds
  the second incidence that stores accidental padding and initial-adjustment order independently.
- **Controlled discriminator:** `F2002-slursavoid-no-acci.mus`, derived from the empty Finale 2002
  document, changes selector `53` word 4 from 18 to 37 and selector `50` word 4 from 2 to 1. Its
  companion keeps general padding 37, copies 37 to accidental padding despite avoidance being off,
  and keeps `slurDoStretchFirst` false. The other Finale 2002 companions establish the same copy at
  general-padding values 12 and 18.
- **Implementation:** When selector `97` is present and selector `53` contains exactly one fixed-row
  payload, the importer copies `slurPadding` to `slurAcciPadding` and sets `slurDoStretchFirst`
  false. Both are reported as `LegacyBehavior`; the complete two-incidence family remains directly
  recovered as `LegacyMus`.

## 2026-08-25 — Staff-line tip-avoidance amount

- **Controlled discriminator:** `F2002-tips-avoid-stafflines.mus`, derived from the empty Finale
  2002 document, changes the displayed Avoid Staff Lines By amount from 8 to 17. Selector `50`
  incidence 0 word 5 is the only changed source word, from 9 to 18, and the modern companion
  preserves 17. Positive stored amounts are therefore one-based.
- **Zero evidence:** Six distinct paired survey files store zero in the same word while their
  companions use 8. This establishes the recovery outcome but not the historical meaning of zero.
- **Implementation:** A nonzero word overlays `slurAvoidStaffLinesAmt` as the stored value minus
  one and is reported as `LegacyMus`. Zero leaves the pinned Finale 27 value and
  `Finale27Default` origin intact. Confidence is **confirmed** for positive values and **strong**
  for the zero treatment.

## 2026-08-25 — Finale 2.6 fixed Smart Shape hook length

- **Coverage pattern:** Every tracked Finale 2.6.3 source lacks the named `FI` preference family,
  while all eight semantic companions set `SmartShapeOptions.hookLength` to 8 EVPU. The Finale
  1.0.0 sources also lack `FI`, but their companions retain 12; Finale 3.7 and later sources store
  12 directly as `FI` comparator 11 word 2.
- **Discriminator:** The controlled Finale 2.6.3 curve edit changes selector `51` word 5 from 8 to
  5 while its companion hook length remains 8. That word is therefore not the hook-length source.
- **Implementation:** The source structure cannot separate the two observed Coda behaviors, so an
  exact version gate is nested inside the Coda-banner epoch. Version 2.6 receives the fixed value 8
  as `LegacyBehavior`, with no claimed source offset. Finale 1.0, a Coda profile without a recovered
  version, and numerically similar profiles in other epochs retain their existing imported values.
  Confidence is **strong**.

## 2026-08-25 — Guitar-bend custom-line boundary

- **Coverage pattern:** All eight tracked Finale 2000 files and all six tracked Finale 2002 files
  store selector `92` as `(0, 1, 2, 0, 0, 0)`: glissando and tab slide name source custom lines 1
  and 2, while the guitar-bend reference is zero. Their Finale 27 companions append a preset-arrow
  solid bend curve. It receives cmper 3 in the ordinary two-line pool and cmper 7 in the controlled
  Finale 2000 fixture whose edits added cmpers 3 through 6.
- **Boundary:** Every tracked Finale 2003 and later fixed-row sample stores the bend reference in
  selector `92`; the Finale 2003 baseline begins `(0, 1, 2, 3, 0, 0)`. This agrees with the bend-curve
  tool arriving in Finale 2003.
- **Implementation:** Coda-banner and uncompressed documents request the baseline bend curve. The
  DCL epoch does so only before internal major version 8; zlib and unknown epochs do not. Before the
  separate custom-line boundary, the existing request for baseline glissando and tab-slide remains.
  Deferred resolution appends the bend curve after every source-owned custom line and reports the
  imported object and resolved option field as `Finale27Default`.

## 2026-08-25 — Finale 3.x crescendo line-width boundary

- **Question:** Is `FI` comparator 11 word 1 an independently meaningful crescendo-line width
  throughout the uncompressed epoch?
- **Pre-3.7 evidence:** Nineteen paired sources from Finale 3.0, 3.2, and 3.5 store word 1 as 0, 32,
  or 72 and word 4 as 111 or 118. Every modern companion uses the recovered word-4 value for both
  `smartLineWidth` and `crescLineWidth`. The same sources store word 2 as 0 or 8, while every
  companion uses the pinned baseline's `hookLength` of 12.
- **Boundary discriminator:** A Finale 3.7 source stores 144 in word 1, 12 in word 2, and 118 in
  word 4. Its modern companion preserves all three values independently, and every other Smart
  Shape option leaf matches.
- **Implementation:** Inside `UncompressedLegacy` only, recovered versions 3.0 through 3.6 copy
  `smartLineWidth` to `crescLineWidth` and report the result as `LegacyBehavior` with word 4's source
  evidence. The same gate ignores word 2 and retains the selected baseline hook length with
  `Finale27Default` origin. Finale 3.7 and later, other epochs, and files without a recovered version
  retain direct word-1 and word-2 recovery. The gate is version-based because the six-word `FI` row
  has no identified structural change at the boundary.
- **Confidence:** **Strong** across the stated range: the available versions agree and Finale 3.7
  discriminates the upper boundary, but no paired 3.1, 3.3, 3.4, or 3.6 specimen exists.

## 2026-08-26 — Smart Shape connection-style collections

- **Private lead:** Authorized historical Settings Scrapbook code identifies the four preference
  families as selectors `26`, `90`, `91`, and `98`, all at globals comparator 65534. This locator
  evidence is **private-framework-derived**; only the interoperability facts are recorded here.
- **Independent structure check:** Fixed-row record families and zlib class payloads contain the
  same signed 16-bit triples: connection index, horizontal offset, and vertical offset. The zlib
  classes are `0x0028`, `0x0068`, `0x0069`, and `0x0070`, matching the established numeric-global
  bridge. Existing Finale 2008 big-endian and Finale 2012 little-endian documents establish both
  zlib byte orders, so no additional zlib fixture is needed for this mapping.
- **Enum order:** Collection elements occur in the order of musxdom's four collection-type enums.
  The stored connection index is a separate zero-based enum whose order runs from head-left-top
  through note-right-center. Musxdom's `ConnectionIndex` declaration and XML mapping now use that
  same order, so the reader can assign the stored ordinal without maintaining a second conversion
  table.
- **Collection shapes:** Selector `26` has three observed shapes: two semantic styles in Finale
  2.6.3; 25 styles plus a terminal zero triple in Finale 97 through 2002; and all 29 styles plus a
  terminal zero triple from Finale 2003 onward. Selectors `90`, `91`, and `98` contain exactly 18,
  2, and 8 styles, with the first two present by Finale 2000 and the bend family present by Finale
  2003. The payload length, rather than a version gate, selects the slur prefix.
- **Implementation:** `smart_shape_options.cpp` overlays each complete source tuple onto the seeded
  collection and reports its three leaves as `LegacyMus`. It ignores the slur family's structural
  terminal tuple, retains seeded styles beyond a shorter source prefix, and reports every retained
  or wholly absent family leaf as `Finale27Default`. Synthetic fixed-row, DCL, big-endian zlib, and
  little-endian zlib tests cover exact values and enum ordinals; tracked Finale 2.6.3, 97, 2000,
  2003, and 2012 files cover the observed collection boundaries.
- **Remaining class work:** All 41 scalar fields and all five collections are accounted for;
  `maximumShortHairpinLength` and `articAvoidSlurAmt` are `MusxOnly`. Most Coda-era scalar source
  locations remain open, so the class is still partial.
- **Approved classification:** A differing horizontal or vertical connection-style offset is
  `different_defaults` only when the reader retained the pinned baseline value. The rule excludes
  connection indices and every `LegacyMus` offset.
- **Sparse Finale 27 slur collections:** The combined companion population contains slur
  connection maps of 4, 25, and 29 elements within the same source versions. Three discriminating
  Finale 2003 sources each retain all 29 stored tuples; their companions respectively serialize
  all 29 types, types 0-24 while omitting a zero-valued tab tail, and only the nonzero tab types
  25-28 while omitting 25 zero-valued types. Engraver Slurs is enabled in all three companions, so
  neither the source version nor that option determines the serialized size.
- **Reader-complete classification:** The reader deliberately retains a complete 29-element map.
  A missing companion object whose recovered `connectIndex`, X, or Y is nonzero is
  `finale-upgrade-loss`; an object containing only zero recovered values or seeded values is
  `reader-completed-connection-array`. The rule applies one disposition to all four leaves of the
  missing object, including its identity leaf. In the 16,222-occurrence combined capture, this
  reclassifies all 27,808 former SmartShapeOptions reader-only leaves: 26,744 as completed-array
  structure and 1,064 as upgrade loss, leaving zero reader-only and zero unexpected
  SmartShapeOptions leaves.

## 2026-08-26 — Finale 2003 bend-connection upgrade loss

- **Question:** Are the Finale 2003 bend-connection offset disagreements source mapping errors,
  changed defaults, or lossy Finale 27 upgrade behavior?
- **Source evidence:** An untouched Finale 2003 document stores the eight selector-`98` Y values as
  `(0, 40, 40, 4, 48, 48, 40, 48)`. Separate controlled edits change X and Y for types 0, 1, 2, 3,
  and 6. The MUS reader and same-version ETF agree exactly in all three documents.
- **UI boundary:** Finale 2002 has no bend-connection controls, Finale 2003 presents three, and
  Finale 2004 presents the complete six-control interface. The three Finale 2003 controls therefore
  fan out across five stored semantic tuples while types 4, 5, and 7 remain hidden support entries.
- **Companion behavior:** Finale 27 resets every edited X coordinate to `(8, 0, 0, 0, 36)` and every
  edited Y coordinate to `(0, 40, 40, 4, 40)` for the five exposed tuples. It also changes the three
  hidden top-line Y values from 48 to 40. The reader preserves the source tuples as `LegacyMus`.
- **Classification:** These offset disagreements use the shared `finale-upgrade-loss` category. Its
  predicate requires the DCL epoch, source major version 8, `LegacyMus` origin, the bend-connection
  collection, and an X or Y leaf. Connection indices remain unclassified pending a discriminator.

## 2026-08-26 — Page and measure graphic assignment layout

- **Hypothesis:** Page-attached and measure-attached legacy graphic assignments use the same
  record layout.
- **Structure:** Both assignments have the same 18-word semantic layout. An other row carries six
  payload words, so a page assignment fills three rows exactly. A detail row carries only five
  payload words after its second comparator, so a measure assignment requires four rows and the
  remaining two slots are zero filler. Zlib class `0x041d` preserves that padded 20-word stride.
  Word 8 is the packed horizontal alignment, vertical alignment, positioning reference, and
  preserve-aspect value in both families.
- **Controlled evidence:** The Finale 3.7.2 linked measure graphic, Finale 2006 embedded EPS, and
  Finale 2012 embedded GIF each carry word-8 bits whose decoded values match the independently
  parsed Finale 27 companion. Before the shared decoding was applied, the only five tracked
  companion differences were the nonzero `posFrom` and `fixedPerc` leaves those words predict;
  `hAlign` and `vAlign` already matched their zero-valued companion leaves by coincidence.
- **Display flags:** The supplied PDK declarations assign `0x0001/0x0002/0x0004/0x0008` to
  one/all/odd/even and `0x0010` to hidden in the assignment's display-flags word. Nine independently
  upgraded Finale 2012 page assignments store `0x0011` and preserve both `One` and `hidden=true`,
  distinguishing the packed interpretation from separate words 6 and 7.
- **Conclusion:** **Confirmed** for the common 18-word prefix across the uncompressed, DCL, and
  zlib epochs. The reader now uses one packed-position implementation for page, shape, and measure
  graphic assignments. A Coda-banner graphic assignment remains structurally accepted but has no
  corpus specimen.

## Commands

Reproduction commands are in [README.md](README.md). Additional spot checks used `xxd -g 1`, `strings -a`, `unzip -l`, `unzip -p`, Python's `zlib`, `gzip`, `zipfile`, and `xml.etree.ElementTree`. Temporary decoded samples were written only under `/tmp`.
