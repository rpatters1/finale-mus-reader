# FlagOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-29 — FlagOptions straight-flag correction and structural gate

- **Refuted interpretation:** `F100-flag-options.mus` changes selector `36` word 5 from zero
  to one, but Finale 1.0.0 has no straight-flags control. Its companion's omitted
  `straightFlags` is the correct false value, not upgrade loss. Selector `36` word 5 remains
  unexplained and is no longer read as FlagOptions.
- **Structural discriminator:** selector `5` exists in Coda and early 3.x files, so its word-2
  bit cannot identify its own meaning. The related editable geometry family does: selectors
  `73`--`76` are absent from all 44 tracked Coda files and from the checked Finale 3.0
  (`mus-254b77cdb2ab24f4`) and 3.2 (`mus-8db5f15157ec4701`) corpus samples, while all four are
  present in the checked Finale 3.5 sample (`mus-09b78d5f64c4b5fe`) and every tracked fixed-row
  document from Finale 3.7 onward. Selector `75` is the straight-coordinate member of that
  family and is the narrow marker used by the importer.
- **Implementation consequence:** when selector `75` is absent, the editable fixed-row table
  does not apply and `straightFlags` is reported as `Finale27Default`; the separate Coda
  importer still expands that era's stored origin choices. When selector `75` is present,
  selector `5` word 2 bit 0 is recovered normally. This establishes the layout without
  relying on the incomplete version record; a damaged later file missing selector `75`
  fails closed to defaults.
- **Boundary:** the structure supports the reported likely Finale 3.5 introduction, but the
  exact product boundary remains open because there is no controlled edit between 3.2 and
  3.7.2.
- **Tracked result:** all 154 sources and all 154 companions complete. FlagOptions has 3,030
  equal leaves and 50 unexpected leaves; every unexpected leaf is the deliberately unmapped
  `eighthFlagHoist`. The prior Coda `straightFlags` difference is gone.
- **DCL controlled confirmation:** `F2003-flagopts.mus` changes selector `5` word 2 and
  gives every mapped word in selectors `73`--`76` a non-default value. The era ETF repeats
  those rows, and the independently parsed Finale 27 companion agrees on the straight-flags
  switch and all eighteen distance values. The DCL layout therefore no longer relies only on
  unchanged corpus agreement. After adding the fixture, all 155 sources and companions
  complete; FlagOptions has 3,050 equal leaves and the same 50 `eighthFlagHoist` differences.
- **Zlib controlled confirmation:** `F2012-flagopts.mus` makes the same complete edit in
  the class-record representation. Numeric-global classes `0x0057`--`0x005a` and the
  straight-flags bit agree with the independently parsed Finale 27 companion on all
  nineteen edited values. With the 156th fixture, all 156 sources and companions complete;
  FlagOptions has 3,070 equal leaves and the same 50 `eighthFlagHoist` differences.
- **Intentional fallback:** no supported epoch has an established source for
  `eighthFlagHoist`, and no public PDK property or Flag Options UI control exposes it.
  The controlled source comparisons instead associate the companion's varying value with
  the default music font. The importer now reports the untouched pinned value as
  `Finale27Default`; only a disagreement carrying that origin is classified as
  `different_defaults`. The refreshed tracked capture has 3,070 equal and 50 expected
  FlagOptions leaves, with no unexpected differences.
- **Full-corpus result:** the authorized three-survey run selects 16,253 occurrences
  representing 7,219 distinct source ids; 16,164 occurrences import, and all 4,564
  companion-backed occurrences compare successfully. The origin-scoped rule classifies
  724 `eighthFlagHoist` leaves as `different_defaults` across all four epochs. It also
  exposes 318 unexpected FlagOptions leaves in 60 distinct documents: 129 leaves from
  43 Coda documents whose selector-10 word-3 value `2` expands to 1696 where raw companions
  store zero, and 189 leaves from 17 Finale 3.0/3.2 documents whose absent editable family
  currently leaves later defaults where raw companions carry earlier fixed behavior. No
  DCL or zlib FlagOptions difference is unexpected; both early-layout populations were left
  pending review at capture time.
- **Reviewed default differences:** the 189 Finale 3.0/3.2 leaves cover twelve specific
  members. Fourteen Mac documents use `Pmusic` and show all twelve differences; three
  Windows documents use `Petrucci` and show seven of them. The population is heavily
  clustered by document family, so it does not distinguish platform defaults from automatic
  adjustment based on the selected music font or its annotated glyph geometry. These twelve
  members are now classified as `different_defaults` only when their source origin is
  `Finale27Default`; source-owned disagreements remain unexpected. The existing all-corpus
  capture predates this classification change and has not been rerun.
- **Coda horizontal correction:** the controlled horizontal-origin edit changes selector
  `10` word 3 from 7 to 11 while selector `36` word 5 also changes from 0 to 1. The new
  one-variable `F100-flagoff-neg3` edit changes only Flag Offset, moving selector `10` word
  3 from 7 to -3; its Finale 27 companion leaves all three upward-horizontal adjustments at
  1696. This refutes the former bit-3 interpretation and shows that selector `10` word 3
  did not cause the two-change fixture's conversion to zero. `F263-flagoff-neg3`
  independently repeats the same source and modern results in Finale 2.6.3. The importer now leaves
  `upHAdj`, `upHAdj2`, and `upHAdj16` at
  `Finale27Default`. The separate selector-10 words 4 and 5 remain implemented: their
  controlled 0-to-1 edits independently establish the upward and downward vertical-origin
  switches. The existing all-corpus capture's 129 Coda horizontal differences predate this
  correction.
- **Refreshed tracked result:** all 158 sources and all 158 companions complete. FlagOptions
  has 2,974 equal leaves, 186 `different_defaults`, and no unexpected differences. The
  expected population is 135 upward-horizontal leaves from 45 Coda companions storing 1696,
  plus 51 `eighthFlagHoist` leaves. The focused FlagOptions tests pass all ten cases. No
  all-corpus probe was run.
