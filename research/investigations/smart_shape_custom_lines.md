# SmartShapeCustomLines investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

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
