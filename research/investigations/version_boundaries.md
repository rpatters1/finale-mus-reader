# Version boundary investigations

**Covers:** Saving-version reads, the printed-manual audit, and the Finale 3.5/3.7 feature boundaries.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-05 — Explicit saving version

- **Question:** Is the saving Finale version encoded?
- **Method:** hex dump bytes `0x000–0x080` and aggregate strings from bytes `0x020–0x060`.
- **Observation:** Banner-era files say `Finale(R) <product> Copyright...`; products range 3.0–2012. Counts are in `inventory_summary.json`.
- **Conclusion:** Saving-product identification is direct for 1,163 files. Filename/timestamp classification is unnecessary there.
- **Failed hypothesis:** version classification would require record-set heuristics. It does not for banner-era files.

## 2026-09-01 — Printed Finale 3.0/3.2/3.5 manual boundary audit

- **Source:** The repository owner checked a printed Finale 3.0 manual and the Finale 3.2 and
  3.5 addenda. Page-level bibliographic details have not yet been recorded, so these are retained
  as user-supplied manual facts rather than public citations.
- **Finale 3.5 findings:** The addendum identifies adjustable accidental spacing, editable rest
  positioning, split single-staff/multiple-staff left barlines, enhanced multimeasure rests with
  symbol representation and a separate dialog, and half-stem length as 3.5 changes. The
  straight-flags option is present and appears to have been introduced there. Page Format adds
  First Page Drop and First Staff System Drop and Indent; Absolute Staff Sizing arrives in 2002.
  The accidental-spacing importer boundary moves from the provisional 3.7 gate to 3.5; the other
  decoders already select their layouts structurally or recover realized values that predate the
  UI.
- **Finale 3.2 Smart Shapes:** The addendum introduces enhanced Smart Shapes, including
  entry-attached slurs, slur contours and connection types, Slur Thickness, Line Thickness, Dash
  Length, and Dash Space. It says nothing about hook length, so the later independent
  crescendo-width/hook-length boundary remains open.
- **Still unresolved by these manuals:** Piano-brace thickness, font metadata, configurable symbol
  inserts, and hook length. Absence from an addendum is not treated as proof of absence unless the
  relevant dialog or feature inventory is complete.
- **Validation:** The refreshed tracked-evidence capture completed all 194 source occurrences and
  companions. All 1,164 `AccidentalOptions` leaves agree, but the cohort contains no Finale 3.5
  source, so the corrected boundary is covered by the focused test and manual evidence rather than
  by this tracked capture.

## 2026-09-02 — Finale 3.5 and 3.7 feature boundaries

- **Source:** The repository owner inspected the Finale 3.7 addendum and confirmed its enhancement
  list, and confirmed from the Finale 3.5 addendum that bookmarks were introduced in 3.5.
  Page-level bibliographic details have not yet been recorded, so these remain user-supplied manual
  facts rather than public citations.
- **Graphics:** Finale 3.7 introduced the Graphics Tool and Place/Export Graphics, with EPS, PICT,
  and TIFF explicitly supported. The addendum does not mention embedded graphics. Embedding
  remains at the observed Finale 2006 boundary established by the controlled linked/embedded
  saves and their `0x0013` blocks. PICT is distinct from PNG; PNG occurs in later observed
  zlib-era payloads.
- **Group names:** Finale 3.7 introduced group names, including positioning. This agrees with the
  simultaneous appearance of the two group-position selectors and supports the structural marker
  already used by `StaffOptions`.
- **Piano braces:** Finale 3.7 introduced piano-brace thickness controls. This closes the
  provisional introduction boundary already used by `PianoBraceBracketOptions`; its individual
  wire-field mappings retain their existing confidence levels.
- **Smart Shapes:** Finale 3.7 introduced the Smart Shape Options dialog, including Hook Length.
  This confirms the existing hook-length gate. The independent crescendo-line-width boundary
  remains strong from record and companion evidence rather than confirmed by this manual fact.
- **Bookmarks:** Finale 3.5 introduced bookmarks. Their earliest controlled binary specimen remains
  Finale 3.7.2, so the 3.5 object representation is still unobserved and pre-2012 import remains
  deferred with the bookmark class.
