# RepeatOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

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
