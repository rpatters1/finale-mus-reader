# MultimeasureRestOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

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
  [PRODUCTION_READINESS.md](../state/PRODUCTION_READINESS.md#p22-dangling-shape-references-in-seeded-options); the reader
  keeps the comparator as read and warns.
- **Next evidence:** one Coda-era save that moves a single multimeasure-rest field, so words 1–3 of the early record
  can be named. Nothing else about this class is outstanding.
- **Artifacts:** `src/import/options/multimeasure_rest_options.cpp`, `tests/reader_tests.cpp`,
  `tests/mapping_tests.cpp`, `tools/options_coverage_probe.cpp`, `scripts/options_coverage_report.py`,
  [`multimeasure_rest_options.md`](../format/options/multimeasure_rest_options.md#multimeasure-rest-defaults). The two coverage-tool names
  refer to the deleted historical tools identified in the 2026-08-12 entry above.
