# Corpus survey investigations

**Covers:** Corpus enumeration, archive and StuffIt extraction, fixture acquisition, the second corpus, and coverage registration.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-05 — Corpus enumeration

- **Question:** What evidence is actually present?
- **Method:** recursive `find`, then deterministic `scripts/inventory.py`; full-file SHA-256 for legacy sources and exact matched exports.
- **Observation:** 1,218 `.mus`, 2,042 `.fin27.musx`, 1,189 exact adjacent matches, 29 unmatched sources. Legacy data totals 165,758,809 bytes; matched exports total 178,990,189 bytes.
- **Conclusion:** The corpus is much larger than an initial shallow listing suggested and supports statistical comparison. The assertion that every source has an export is nearly, but not completely, true.
- **Follow-up:** preserve full rows in `private/generated/corpus_inventory.csv` and sanitized findings in `CORPUS_INVENTORY.md`.

## 2026-08-05 — Finale 27 semantic census

- **Question:** Can converted XML give record identities at corpus scale?
- **Method:** decode all 1,189 matched `score.dat` members in memory with `scripts/musx_semantics.py`; count direct pool children. Correlate numeric binary code vectors with XML tag vectors over 375 other-framed and 324 detail-framed files.
- **Observation:** dozens of exact/near-exact mappings. Examples: `0x0086/durAllot` exact 375/375, `0x011a/partDef` exact 375/375, `0x03f3/baselinesExprAboveStaff` exact 324/324. Others show conversion expansion but correlations above 0.99.
- **Conclusion:** type mapping is practical without proprietary code. Count collisions remain possible; field-level value correlation or ETF should confirm names.
- **Failed hypothesis:** a Finale 27 XML object count always equals legacy binary count. Counterexamples include `frameSpec`, smart shapes, text definitions, and part-scoped data.

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
- **Files examined:** Six local private ETF evidence assets associated with source IDs `mus-d89e8fe12e271440`, `mus-3597fd4fce0c272b` (two exporter versions), `mus-7aa45639c14b3864`, `mus-2c0a5e8897b436d5`, and `mus-bd0042f8e0354192`.
- **Method:** SHA-256 and byte counts; normalize the classic ETF carriage-return line endings for section and structure counting; inspect headers, section order, structure identifiers, `eE` entry records, and controlled template differences.
- **Observation:** In the source-ID order above, ETF sizes/hashes are: 16,893 bytes (`4345805a001e9c198cc2f022c8c469cb1654e290d4c5e8000d16e8d547f64517`); 27,945 (`76ce06887b36720aac51ef2578eda11ece72cf3c921ccf832c3f5790eed413de`); 34,029 (`b96c24f43be5bcb0a54ffbd7a44a42311c2cae8cf45b0a4c4e2180ca4f7f2e76`); 123,084 (`d30d569e4b9cc4e642d9a494d1efdc2a975f624a7353f26b2d22a133b1e49844`); 74,040 (`03aa22ec769a5d10dbdb2cde4512cf31d9730c95cad109ebd766f02368edf15e`); and 1,272,164 (`ca9ed81de7f782bf1e2e2ccf22714018dae8bb75648d4ff083e01b97a6fd58c1`).
- **Observation:** All six exports contain `others`, `details`, `entries`, `text`, and `lyrics` sections. The three pre-banner sources use `eE` entry structures at scale: 1,094 lines for `mus-7aa45639c14b3864`, 549 for `mus-2c0a5e8897b436d5`, and 9,446 for `mus-bd0042f8e0354192`. The Finale 2005 nested-tuplet sample contains six `eE` entries; the Finale 2000 template contains no `eE` entries.
- **Observation:** The ETF header explicitly identifies Finale 2000 or Finale 2005 for the template pair and Finale 2005 for the nested-tuplet sample. The older pre-banner ETFs do not have the modern `^header` section, so their exporter/source version cannot be inferred from ETF structure alone.
- **Observation:** The same Finale 2000 template exported by Finale 2005 is larger and contains additional `&f`, `PD`, `XA`, expression, and other records; the saver header also changes to Finale 2005. This demonstrates semantic upgrade/synthesis during resave.
- **Observation:** The ZIP copy of the Finale 2.6 source could not be opened by Finale 2.6.3 because the ZIP did not preserve the classic Mac resource fork. The parallel StuffIt extraction supplied the readable source and ETF.
- **Conclusion:** ETF confirms that the conceptual other/detail/entry/text decomposition predates the 2007 typed-zlib container and gives strong logical targets for the old binary families. It does not by itself reveal the pre-2007 binary codec or prove byte-for-byte correspondence. Later-version ETF exports must be treated as normalized semantic references.
- **Follow-up:** Correlate ETF structure lines with binary offsets for the five available source classes; use the two template exports to separate source-document records from Finale 2005 upgrade records; obtain the missing Finale 3.0 ETF before attempting a general legacy decoder.

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

## 2026-08-24 — Recovery-coverage registration and schema 3

- **Problem:** Adding `repeat_options` to the C++ surveyor registry did not add it to the report's independently
  maintained Python class-to-pool table. A third C++ type-to-origin router separately repeated class knowledge. The
  probe therefore observed the class while one report section silently omitted it.
- **Correction:** Each `COVERAGE_SURVEYOR` invocation now registers the class key and musxdom pool together. The
  comparison output carries that hierarchy in compact schema 3, and the Python report renders it without a class
  registry of its own. Text comparison also selects classes from the registered `texts` pool. Surveyors query the
  typed `ImportReport` maps directly, eliminating the separate origin router.
- **Origin audit:** Direct typed lookup exposed field-specific legacy origins on font charset leaves that the former
  object-level font origin obscured. The `symbol-font-equivalence` classification names only charset bank/value
  differences where both definitions identify symbol fonts, rather than treating every font field alike.
- **All-corpora result:** With Finale 27 `MacSymbolFonts.txt` supplied, schema 3 processed 15,945 rows: 15,890 imports
  succeeded, 55 known non-document/library inputs failed, and all 4,497 companion comparisons succeeded. Across all
  pools there were zero unexpected differences. Enigma-text findings contained zero `other` differences.
