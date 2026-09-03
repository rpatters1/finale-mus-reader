# PageFormatOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-09-01 — PageFormatOptions recovery

- **Coda edit:** The supplied `F100-pageformat` MUS, ETF, and Finale 27 companion locate the
  one-set Coda layout. The staff-height word is in selector `39`, not the later selector `93`,
  and converts from 96 EVPUs to musxdom's raw value 1536. The converted document duplicates the
  source set into score and parts.
- **Later mappings:** Public `FCPageFormatPrefs` documentation supplied semantic and unit
  contracts; authorized Framework history supplied preliminary score/parts selector locations.
  Synthetic fixed-row and class-record tests exercise every contained leaf and both zlib byte
  orders.
- **Refined fixed-row layouts:** A first targeted comparison against F2000, F2002, and F2008
  appeared clean after copying the score format into uncompressed parts, but the complete
  tracked-evidence capture exposed 300 contained-field differences. Raw F97, F98, and F2000
  records show that selector `77` already stores independent parts values; its pre-DCL system
  distance and first-system offsets occupy different words from the DCL absolute fields. They
  also expose both `IU` and `Iu` spellings for the current-system row, with record presence
  selecting the applicable layout. The zlib parts dimensions follow the container byte order.
- **3.5/3.7 boundary:** The owner's printed manuals identify Finale 3.5 as the expanded page-format
  dialog, specifically including First Page Drop and First Staff System Drop and Indent, and Finale
  3.7 as the arrival of separate parts settings. Two targeted, companion-backed Finale 3.5
  documents both omit selector `77`. Copying their recovered score format to parts and guarding
  the later score first-page switch with that structural marker changes each comparison from 50
  equal plus two unexpected leaves to all 52 equal. No full private-corpus probe was run.
- **Pre-3.5 shared behavior:** The owner established that all four modern right-page margins use
  the recovered left-page margins before Finale 3.5, and that first-system left uses the recovered
  ordinary system-left margin. The complete resulting score format supplies the parts format.
  These synthesized destinations report `LegacyBehavior`.
- **Targeted recapture:** An ad hoc TSV selected exactly the 418 occurrences that had a
  PageFormatOptions unexpected difference in the earlier all-corpus snapshot, representing 277
  distinct contents. All sources and companions imported. The platform-dependent dimension fix
  and pre-3.5 shared behavior remove 718 of 1,008 unexpected leaves; 290 remain across 132 distinct
  contents. A repeat over the identical cohort after treating pre-2002 staff height as fixed
  `LegacyBehavior` removes all 196 score/parts staff-height differences, leaving 94 unexpected
  leaves across 39 distinct contents. The parts first-page-top rule removes 21 more. Applying the
  structurally selected pre-3.5 system-top word and first-system formula removes all remaining 68
  system-geometry leaves. The final recapture imports all 418 sources and companions and leaves
  five unexpected leaves across five distinct contents: four collision-avoidance flags and one
  parts facing-pages value. Collision avoidance remains unclassified for separate investigation.
- **Uncompressed parts first-page top:** The first-page top follows the recovered left-page top
  margin, not the right-page top. This is fixed behavior and reports `LegacyBehavior`.
- **Pre-3.5 system top:** In all 17 affected Finale 3.0/3.2 documents, `IU(0)` word 2 exactly
  equals the companion score system top while word 5 contains the unrelated value previously
  imported. The five distinct companion values are all reproduced. A Finale 3.5 sample has zero
  at word 2 and its system top at word 5. Selector `75` presence selects the expanded layout;
  before it appears, first-system top adds selector `17` word 0 without the later selector-`03`
  offset.
- **Staff-height UI boundary:** The Finale 2002 for Windows Read Me (August 2001, SHA-256
  `d55b9e2094ca347304fa340d68624e3433178ae0332b6a24a3eb4761f866f0ae`) lists Absolute Staff
  Sizing under “New Features in Finale 2002” and describes direct control of resulting staff
  height without changing page margins or fonts. The Finale 2001d Windows Read Me (January 2001,
  SHA-256 `9b72e7452c2567bd328d08ef6e6681685e27d2f3dc8ddb44312a921924c9da03`) discusses system
  resizing and first-system Page Format behavior but contains no Absolute Staff Sizing feature.
  This establishes Finale 2002 as the option boundary. The broader corpus shows that earlier
  values at the suspected location vary while every differing Finale 27 companion supplies 1536;
  they therefore do not map to musxdom's absolute `rawStaffHeight` preference. The pinned Finale 27
  baseline is 1312, so the importer supplies the fixed earlier value 1536 as `LegacyBehavior`
  before Finale 2002.
- **System-percentage UI boundary:** The controlled Finale 2001 Page Format UI has no system
  percentage. Its selector `76` words 3 and 4 are zero, as in Finale 2000, while the Finale 2002
  empty document stores 100 in both words. Both pinned Finale 27 baselines already supply 100,
  so pre-2002 DCL sources leave those destinations at `Finale27Default`; selector `76` is read as
  system scaling beginning with Finale 2002.
- **Collision-avoidance discriminators:** Controlled Finale 2.6.3 and 3.7.2 edits disable Avoid
  Margin Collisions. The former changes `fi(65534)` incidence 51 word 5 from 1 to 0; Finale 1.0.0
  lacks that incidence and has fixed-false behavior. The latter clears bit 15 of `FI(10)` word 2.
  Both ETFs repeat the source edit and both Finale 27 companions omit the enabled XML element.
- **Tracked recovery result:** The current capture has 211 successful source occurrences and
  211 successful adjacent-exact companion imports, representing 209 distinct contents. All 10,972
  `PageFormatOptions` leaves are equal with no unexpected differences. `adjustPageScope` is
  document-editor UI state and intentionally retains `Finale27Default`; companion disagreements
  on that exact path are classified as `different_defaults` only with that origin.
- **Facing-pages discriminator:** Enabling Facing Pages for parts in Finale 3.7.2 changes selector
  `77` word 17 from 0 to 1 while word 18 remains zero. Finale 2000 preserves that complete selector
  payload, and both Finale 27 companions preserve parts facing pages while leaving the score
  switch off. The uncompressed parts location is therefore word 17; DCL and zlib retain word 18.
- **Final focused and all-corpus results:** Applying the scalar pre-expanded collision flag and the
  uncompressed parts-facing-pages location removes the last five unexpected leaves from the same
  418-occurrence focused cohort. Its 21,720 equal leaves and 16 expected `adjustPageScope` default
  differences represent 277 distinct contents. The subsequent three-corpus capture selected
  16,308 occurrences representing 7,272 distinct contents: 16,219 imported, all 4,619 available
  companions imported, and 89 failed before comparison because they are Finale libraries or not
  Finale MUS documents. `PageFormatOptions` contributes 239,770 equal leaves, 418 expected
  `adjustPageScope` default differences, and no unexpected differences.
