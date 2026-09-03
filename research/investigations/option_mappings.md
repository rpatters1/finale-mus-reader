# Option-map investigations

**Covers:** The private PDK Framework option-map audit that produced the distilled mapping table.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-08 — Private PDK Framework option-map audit

- **Authorization and boundary:** Inspected authorized current and historical Framework histories
  read-only under explicit authorization. No source was copied. Only distilled interoperability
  facts are published, labeled `private-framework-derived`.
- **Snapshots:** Compared the current mapping with a historical snapshot and its initial imported
  state. The original pre-Finale-2014-oriented line was checked explicitly at
  the corresponding point in the historical Framework history.
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
