# StaffOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-09-01 — StaffOptions name-position recovery

- **Public interface evidence:** The published `FCStaffNamePositionPrefs` and
  `FCGroupNamePositionPrefs` APIs expose full and abbreviated loads, horizontal and vertical EVPUs,
  justification, alignment, and Expand Single Word. Their published source exposes the distinct
  staff and group flag packing. Accessed 2026-09-01 at `pdk.finalelua.com`.
- **Location evidence:** Authorized read-only inspection of a non-public compatibility reference
  identifies selectors `04`, `66`, `79`, and `80`, comparator `65534`, and the twelve word
  locations recorded in `data/legacy_option_mappings.csv`. This remains
  `private-framework-derived`.
- **Independent structure check:** Existing ETFs contain all four selector families from Finale
  3.7.2 through the compressed eras. The Finale 1.0.0 and 2.6.3 controlled ETFs contain selector
  `04`, but Coda exposes no name-position UI and does not yet have group names; neither early ETF
  contains the other three selectors. No EMEL spelling naming these fields was found in the
  tracked evidence or public headers.
- **Implementation:** `StaffOptions` recovers five leaves for each present name-position selector
  from the uncompressed epoch onward. In Coda it ignores the apparent selector values, retains
  matching pinned defaults, and asserts the full/abbreviated staff-name horizontal offset,
  predominant vertical offset, justification, and alignment as `LegacyBehavior`.
- **Scalar evidence:** The controlled Finale 2012 edit changes class `0x006f` words 12--14 from
  `-320`, `72`, `1` to `-289`, `71`, `0`; its Finale 27 companion reports the same two numbers and
  omits the boolean element. Every inspected Finale 2008 record has the 36-byte form carrying this
  tail, while the Finale 2007 record is only 24 bytes. The implementation consequently selects the
  scalar layout from a payload of at least 15 words rather than from the recovered version.
- **Scalar fallback:** On the other side of the payload-shape gate, sources without that tail
  receive the fixed `staffSeparation` value `-320` as `LegacyBehavior`; `staffSeparIncr` and
  `autoAdjustStaffSepar` retain their pinned values as `Finale27Default`. In this options context,
  `indivPos` and `hidden` are likewise non-persisted pinned defaults and report `Finale27Default`.
- **Focused validation:** Synthetic fixed rows cover Coda, uncompressed, and DCL; class records
  cover both byte orders in zlib. Missing and truncated records retain the seeded objects without
  partial overlay. A deliberately long fixed-row selector confirms that only the zlib class-record
  encoding can satisfy the scalar-tail recovery gate.
- **Tracked-pair schema check:** The Finale 2006 empty-document pair compares all 31 persisted
  leaves. With the fixed `staffSeparation` behavior on the no-tail side of the recovery gate, all
  31 are equal. This was initially a one-pair check and is now also covered by the tracked cohort.
- **Tracked-evidence coverage:** The regenerated 198-source cohort imported 198 sources and 198
  companions successfully. The new scalar fixture agrees on all 31 `StaffOptions` leaves. The
  class initially had 5,618 equal leaves and 520 unclassified differences overall, all in the Coda
  epoch. After applying fixed Coda name-position behavior, the recapture has 6,088 equal leaves,
  50 `different_defaults`, and no unexpected differences. The 50 classified leaves are the two
  vertical paths in the 25 companions that do not retain `-27`.
- **Coda companion refinement:** Every Coda companion uses horizontal offset `-192`, left
  justification, and left alignment for both full and abbreviated staff names. Expand and all
  group-name leaves match the pinned baseline. Vertical positioning is not uniform: `-27` appears
  43 times, `-24` once, and `-22` 24 times. The importer asserts the predominant `-27` as
  `LegacyBehavior`; the other two companion transformations are `different_defaults` rather than
  separately recoverable source behavior.
- **All-corpus refinement:** Seven distinct Coda installation documents disagree with the fixed
  horizontal behavior in both the full and abbreviated paths. Their companions contain `-152`,
  `-160`, `-200`, `-216`, `-228`, `-232`, or `-320` where the importer supplies `-192` as
  `LegacyBehavior`. These 14 leaves are classified as `different_defaults`. The other 114
  unexpected `StaffOptions` leaves are confined to 20 uncompressed Finale 3.0--3.5 documents and
  remain unclassified pending separate characterization.
- **Pre-3.7 layout:** All 20 companion-backed Finale 3.0--3.5 documents use selectors `04` and
  `66` as six-word tuples: horizontal offset, stored vertical offset, font id, point size, font
  effects, and justification. Words 0 and 5 agree respectively with companion horizontal offset
  and justification in all 40 positions. Both group-position selectors are absent; Finale 3.7
  introduces them and changes the staff tuple to the later packed-flag layout.
- **Vertical approximation:** Finale's vertical conversion is font-metric-dependent and cannot be
  recovered exactly from these records alone. The deliberately simple cross-platform rule
  `stored vertical + 3 * point size` matches all 34 Times-font positions. The six Times New Roman
  positions in three Windows documents require 43 Efix rather than the rule's 36, leaving a
  seven-Efix residual. Adjusted values report `LegacyMusAdjusted`, and residual companion
  differences receive the new `font-metric-approximation` classification.
- **Other early leaves:** The companion horizontal alignment always equals the tuple's stored
  justification, and Expand Single Word is true throughout this sample. The importer therefore
  uses justification as source-era alignment behavior only where it differs from the pinned
  baseline, while leaving matching alignment and expand at `Finale27Default`. Group positions are
  absent and remain pinned defaults. The all-corpus recapture turns 108 of the 114 previously
  unexpected early leaves into equality; the remaining six are expected font-metric
  approximations across three distinct Windows documents. Together with the separately approved
  Coda horizontal-default classification, `StaffOptions` now has 142,604 equal leaves, 182 expected
  differences, and no unexpected differences across 4,606 companion occurrences.
