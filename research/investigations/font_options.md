# FontOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

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
- **Artifacts:** `research/format/options/font_options.md`, `research/reference/LEGACY_OPTION_MAPPINGS.md`, and
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
- **Artifacts:** `research/format/options/font_options.md`, `research/reference/LEGACY_OPTION_MAPPINGS.md`, and
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
- **Artifacts:** `research/format/options/font_options.md`, `research/reference/LEGACY_OPTION_MAPPINGS.md`, and
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
  The two coverage tools are historical names retained because they performed this experiment;
  both were later replaced by the registry-driven recovery-coverage pipeline and deleted. Their
  historical forms on main remain available as
  `git show f3b8905:tools/options_coverage_probe.cpp` and
  `git show f3b8905:scripts/options_coverage_report.py`.

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
