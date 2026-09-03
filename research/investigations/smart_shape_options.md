# SmartShapeOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-21 — The first word-extension connection table has eight entries

- **Question:** why did one Finale 2004 source disagree with its Finale 27 companion on six fields in selector
  `55`'s word-extension connection table?
- **Result:** the source has four incidences, exactly 24 words or eight complete three-word elements. Reading those
  eight elements in the established style order reproduces every companion connection point and offset. The
  previous nine-element assumption admitted only the later 30-word layout and caused the early payload to be
  interpreted from seeded values rather than from its own bytes.
- **Structural boundary:** 24 words carries the first eight style types and omits `zeroOffset`; 27 or more words
  carries all nine, with the observed fixed-row form using 30 words and leaving its final three as padding. The
  importer now selects the layout from that payload length rather than from the saving version.
- **Validation:** a focused synthetic test exercises both shapes, and a one-document recovery probe reproduces all
  nine companion values with no lyric-options differences. The ninth value in the early document remains the
  pinned baseline because no source element states it.
- **Artifacts:** `src/import/options/lyric_options.cpp`, `tests/mapping_tests.cpp`, and
  [`lyric_options.md`](../format/options/lyric_options.md#two-collections-and-the-two-orders-that-do-not-match-musxdom).

## 2026-08-24 — SmartShapeOptions structural families

- **Question:** Which legacy preference rows populate `SmartShapeOptions`, and which apparent early rows are name
  collisions rather than the later option layout?
- **Lead:** The current authorized private framework's Smart Shape preference map names the `FI`, `50`–`53`,
  `92`–`93`, and `97` families. These locators began as **private-framework-derived** evidence and were checked
  against the project's distilled option-mapping table before any decoder was written.
- **Controlled evidence:** Record dumps from Finale 1.0.0, 2.6.3, 3.7.2, 2000, 2002, 2005, 2011, and 2012 were
  compared with their semantic companions. Finale 3.7.2 supplies `FI` and the twelve-word selector `52` contours;
  Finale 2000 adds selectors `92` and `93`; Finale 2002 adds selector `97`, at which point selectors `50`, `51`,
  and `53` agree with the modern scalar meanings. The 2011 and 2012 class records preserve the same payloads under
  the numeric-global class bridge, while named `FI` becomes class `0x008d`.
- **Refuted assumption:** Selector spelling is not enough to extend the later layout backward. The Coda-banner files
  contain some of `50`–`53`, but selector `52` has only six words with floating-point-shaped values and the other
  rows disagree with the later meanings. The modern companions manufacture complete Smart Shape defaults, so they
  cannot prove that those defaults came from the early MUS records.
- **Implementation:** `smart_shape_options.cpp` gates contours on the exact twelve-word family, the later slur
  scalars on selector `97`, the custom-line group on selector `92`, and the figure group on `FI`/class `0x008d`.
  It recovers 38 scalar fields plus four contours across the uncompressed, DCL, and zlib epochs. Coda remains
  explicitly uncovered and retains the pinned defaults.
- **Open evidence:** The locations and semantics of any Smart Shape preferences actually stored by Finale 2.6.3,
  four scalar fields, and four connection-style collections remain unknown. At this stage custom-line identifiers
  were recovered but the pre-capability tool references had not yet been repaired.
- **First coverage review:** The 104 controlled companion pairs produced 356 unclassified SmartShapeOptions leaves.
  The 155 Coda-banner leaves were reviewed as upgrade/default differences and classified only where their source
  origin is `Finale27Default`; the approval does not extend to any later epoch. Raw selector `53` inspection exposed
  a decoder error in the remaining population: words 2 through 4 are vertical break adjustment, the avoid-lines
  flag, and padding, respectively. The first four words persist in the earlier fixed-row layout even though its
  other slur selectors differ, so that smaller layout is selected by the twelve-word contour family.

## 2026-08-25 — Pre-custom-line Smart Shape defaults

- **Capability boundary:** The established `ls` census places custom-line definitions at internal major version 5.
  The fallback treats the complete Coda-banner epoch as earlier and applies the version comparison only inside the
  uncompressed epoch. DCL and zlib are wholly later; an unknown epoch is not guessed into the fallback.
- **Implementation:** Before that boundary, `SmartShapeOptions` requests the pinned baseline's glissando, tab-slide,
  and guitar-bend lines in semantic order. Deferred resolution uses musxdom's custom-line importer, including its
  existing shape, font, and generic raw-text importers, and assigns every imported object `ShareMode::All`.
- **Comparator result:** The controlled pre-2000 and Coda fixtures contain no source custom-line definitions. Their
  imported definitions therefore receive comparators 1, 2, and 3, and the three Smart Shape option fields receive
  those same values. Finale 2000 is on the custom-line side of this gate and retains its two source-owned lines;
  the later guitar-bend investigation below adds its separate tool-specific fallback.
- **Focused verification:** The mapping gate test covers Coda despite a bogus version, uncompressed majors 4 and 5,
  and a deliberately contradictory DCL major 4. The reader test checks the resulting three line objects and option
  comparators in the pre-2000 and Coda fixtures while retaining Finale 2000's two source-owned objects.
- **Imported-object provenance:** musxdom's font, shape, raw-text, and custom-line importers now accept one callback
  that is forwarded through nested imports and invoked for every newly created pool object. The reader records the
  baseline origin once on each such object; the coverage observation inherits that origin through every descendant
  leaf unless a field-specific origin overrides it. Reused target objects are not reported as imports.
- **Tracked-evidence result:** A fresh 104-document capture completed with 104 successful companion comparisons.
  All 95 unexpected custom-line-style leaves now identify their origin as `Finale27Default`, including the observed
  `solidWidth` 115-to-224 differences. The 272 SmartShapeOptions differences remain unclassified pending individual
  review. After separate review, any differing custom-line `solidWidth` inherited from `Finale27Default` is
  classified as `different_defaults` without constraining its value or source epoch; provenance alone remains
  insufficient for every other custom-line leaf. This classifies 90 leaves; five `charFontSize` differences remain
  unclassified in that capture. Subsequent review applies the same origin-only rule to custom-line `charFontSize`;
  the 108-document capture contains six such Finale 97 differences, all inherited from `Finale27Default`. The same
  origin-only treatment also applies to seven reviewed `SmartShapeOptions` leaves:
  `crescHorizontal`, `crescLineWidth`, `slurAvoidStaffLines`,
  `slurLeftBreakHorzAdj`, `smartLineWidth`, and `useEngraverSlurs`. A difference in any of those fields
  is `different_defaults` only when its source origin is `Finale27Default`.

## 2026-08-25 — Finale 2002 Engraver-Slur contour boundary

- **Question:** Is selector `52`'s fourth tuple a source-owned extra-long contour before Finale 2002,
  and does selector `53` word 3 already mean `slurAvoidStaffLines`?
- **Public lead:** MakeMusic's Finale 2012 documentation says Finale 3.5 through Finale 2001 files
  open with Engraver Slurs disabled and that slurs from Finale 2001 or earlier are not converted
  automatically to the new Engraver Slurs. This is **public-source-derived** evidence, accessed
  2026-08-25:
  [Smart Shape/Slur Direction](https://usermanuals.finalemusic.com/Finale2012Win/Content/Finale/Smart_ShapeOSlur_Direction.htm)
  and [Importing](https://usermanuals.finalemusic.com/Finale2012Win/Content/Finale/Importing5.htm).
- **Corpus boundary:** An aggregate record sweep found one pattern in all 63 installed Finale 2001
  documents: selector `52` tuple four `(0, 0, 0)` and selector `53` word 3 `0`. All 774 Finale 2002
  documents instead have a real fourth tuple and word 3 `1`; their three observed tuples are
  `(1152, 307, 72)`, `(1152, 341, 72)`, and `(1152, 369, 80)`. This independently confirms the
  release boundary without using the marketing version as the decoder gate.
- **Controlled discriminator:** `F97-slurtieopts-changed.mus`, made from the Finale 97 baseline,
  changes the short, medium, and long selector-`52` tuples to `(36, 532, 13)`, `(288, 553, 43)`,
  and `(864, 358, 73)`. Its fourth tuple remains zero. Finale 27 produces those three contours plus
  extra-long `(1152, 358, 73)`: span comes from the later baseline, while inset and height copy the
  source long contour. The companion's separately edited tie-control spans change only `TieOptions`.
- **Implementation:** Selector `97`, already the structural marker for the enhanced slur scalar
  family, now gates both the fourth contour tuple and selector `53` word 3. Before it appears, the
  reader recovers three contours, retains the seeded extra-long span as `Finale27Default`, copies
  long inset and height as `LegacyBehavior`, and retains the seeded avoid-staff-lines value. From
  selector `97` onward, all four tuples and the flag remain `LegacyMus`.

## 2026-08-25 — Pre-2002 slur-thickness source discriminators

- **Question:** Does Finale 27 synthesize pre-2002 Smart Shape thickness-control values from the
  document's tie-thickness options?
- **Controlled pair:** `F2000-tieopts-changed.mus` was saved from `F2000-empty.mus`; both are 8,200
  bytes. The fixed-global options diff changes only selector 84 incidence 0, with `thicknessRight`
  moving from 6 to -17 and `thicknessLeft` from 6 to 11. Two ETF detail families, `#c2` through
  `#c10` and `#s2` through `#s10`, also change, so the fixture is controlled for tie-option edits
  rather than for one isolated leaf. No Smart Shape preference selector changes.
- **Companion result:** An exact comparison finds no changed `SmartShapeOptions` leaf. In particular,
  `slurThicknessCp1Y` and `slurThicknessCp2Y` remain 6 in both companions, while `TieOptions`
  alone receives the edited thickness values 11 and -17.
- **Conclusion:** This rejects direct copying from the editable tie-thickness options.
- **Inset discriminator:** `F2000-tie-insets.mus` switches the ordinary tie contours from percentage
  to fixed insets and changes their cp1/cp2 fixed insets from 8/8 to 17/13. The raw diff is confined
  to the inset-mode word in selector 84 and the six corresponding inset positions in selector 86.
  Finale 27 preserves those edits in `TieOptions` but again leaves both Smart Shape values at 6.
  Selector 86 is therefore also excluded.
- **Direct discriminator:** `F2000-slur-thickness.mus` changes the Slur Thickness control from 6
  to 17. The complete record diff changes exactly selector 59 incidence 0 word 5, and its companion
  changes exactly `slurThicknessCp1Y` and `slurThicknessCp2Y` from 6 to 17. No `TieOptions` leaf
  changes. Selector 59 word 5 is therefore the single stored pre-Engraver-Slur thickness, copied
  into both modern vertical control-point fields. The earlier correlations with tie thickness,
  tie-placement offsets, and default music-font size were shared template defaults rather than the
  upgrade source.
- **Implementation:** In fixed-row files with the twelve-word selector-52 contour family and no
  selector 97, the Smart Shape importer recovers selector 59 word 5 into both vertical thickness
  controls. The same structural marker already selects the era's three-contour and early-adjustment
  layout, so the rule does not depend on a marketing version.

## 2026-08-25 — Single-incidence enhanced-slur behavior

- **Structural boundary:** Finale 2002 contains selector `97`, establishing the enhanced-slur
  scalar family, but selector `53` still has only its first six-word incidence. Finale 2003 adds
  the second incidence that stores accidental padding and initial-adjustment order independently.
- **Controlled discriminator:** `F2002-slursavoid-no-acci.mus`, derived from the empty Finale 2002
  document, changes selector `53` word 4 from 18 to 37 and selector `50` word 4 from 2 to 1. Its
  companion keeps general padding 37, copies 37 to accidental padding despite avoidance being off,
  and keeps `slurDoStretchFirst` false. The other Finale 2002 companions establish the same copy at
  general-padding values 12 and 18.
- **Implementation:** When selector `97` is present and selector `53` contains exactly one fixed-row
  payload, the importer copies `slurPadding` to `slurAcciPadding` and sets `slurDoStretchFirst`
  false. Both are reported as `LegacyBehavior`; the complete two-incidence family remains directly
  recovered as `LegacyMus`.

## 2026-08-25 — Staff-line tip-avoidance amount

- **Controlled discriminator:** `F2002-tips-avoid-stafflines.mus`, derived from the empty Finale
  2002 document, changes the displayed Avoid Staff Lines By amount from 8 to 17. Selector `50`
  incidence 0 word 5 is the only changed source word, from 9 to 18, and the modern companion
  preserves 17. Positive stored amounts are therefore one-based.
- **Zero evidence:** Six distinct paired survey files store zero in the same word while their
  companions use 8. This establishes the recovery outcome but not the historical meaning of zero.
- **Implementation:** A nonzero word overlays `slurAvoidStaffLinesAmt` as the stored value minus
  one and is reported as `LegacyMus`. Zero leaves the pinned Finale 27 value and
  `Finale27Default` origin intact. Confidence is **confirmed** for positive values and **strong**
  for the zero treatment.

## 2026-08-25 — Finale 2.6 fixed Smart Shape hook length

- **Coverage pattern:** Every tracked Finale 2.6.3 source lacks the named `FI` preference family,
  while all eight semantic companions set `SmartShapeOptions.hookLength` to 8 EVPU. The Finale
  1.0.0 sources also lack `FI`, but their companions retain 12; Finale 3.7 and later sources store
  12 directly as `FI` comparator 11 word 2.
- **Discriminator:** The controlled Finale 2.6.3 curve edit changes selector `51` word 5 from 8 to
  5 while its companion hook length remains 8. That word is therefore not the hook-length source.
- **Implementation:** The source structure cannot separate the two observed Coda behaviors, so an
  exact version gate is nested inside the Coda-banner epoch. Version 2.6 receives the fixed value 8
  as `LegacyBehavior`, with no claimed source offset. Finale 1.0, a Coda profile without a recovered
  version, and numerically similar profiles in other epochs retain their existing imported values.
  Confidence is **strong**.

## 2026-08-26 — Smart Shape connection-style collections

- **Private lead:** Authorized historical Settings Scrapbook code identifies the four preference
  families as selectors `26`, `90`, `91`, and `98`, all at globals comparator 65534. This locator
  evidence is **private-framework-derived**; only the interoperability facts are recorded here.
- **Independent structure check:** Fixed-row record families and zlib class payloads contain the
  same signed 16-bit triples: connection index, horizontal offset, and vertical offset. The zlib
  classes are `0x0028`, `0x0068`, `0x0069`, and `0x0070`, matching the established numeric-global
  bridge. Existing Finale 2008 big-endian and Finale 2012 little-endian documents establish both
  zlib byte orders, so no additional zlib fixture is needed for this mapping.
- **Enum order:** Collection elements occur in the order of musxdom's four collection-type enums.
  The stored connection index is a separate zero-based enum whose order runs from head-left-top
  through note-right-center. Musxdom's `ConnectionIndex` declaration and XML mapping now use that
  same order, so the reader can assign the stored ordinal without maintaining a second conversion
  table.
- **Collection shapes:** Selector `26` has three observed shapes: two semantic styles in Finale
  2.6.3; 25 styles plus a terminal zero triple in Finale 97 through 2002; and all 29 styles plus a
  terminal zero triple from Finale 2003 onward. Selectors `90`, `91`, and `98` contain exactly 18,
  2, and 8 styles, with the first two present by Finale 2000 and the bend family present by Finale
  2003. The payload length, rather than a version gate, selects the slur prefix.
- **Implementation:** `smart_shape_options.cpp` overlays each complete source tuple onto the seeded
  collection and reports its three leaves as `LegacyMus`. It ignores the slur family's structural
  terminal tuple, retains seeded styles beyond a shorter source prefix, and reports every retained
  or wholly absent family leaf as `Finale27Default`. Synthetic fixed-row, DCL, big-endian zlib, and
  little-endian zlib tests cover exact values and enum ordinals; tracked Finale 2.6.3, 97, 2000,
  2003, and 2012 files cover the observed collection boundaries.
- **Remaining class work:** All 41 scalar fields and all five collections are accounted for;
  `maximumShortHairpinLength` and `articAvoidSlurAmt` are `MusxOnly`. Most Coda-era scalar source
  locations remain open, so the class is still partial.
- **Approved classification:** A differing horizontal or vertical connection-style offset is
  `different_defaults` only when the reader retained the pinned baseline value. The rule excludes
  connection indices and every `LegacyMus` offset.
- **Sparse Finale 27 slur collections:** The combined companion population contains slur
  connection maps of 4, 25, and 29 elements within the same source versions. Three discriminating
  Finale 2003 sources each retain all 29 stored tuples; their companions respectively serialize
  all 29 types, types 0-24 while omitting a zero-valued tab tail, and only the nonzero tab types
  25-28 while omitting 25 zero-valued types. Engraver Slurs is enabled in all three companions, so
  neither the source version nor that option determines the serialized size.
- **Reader-complete classification:** The reader deliberately retains a complete 29-element map.
  A missing companion object whose recovered `connectIndex`, X, or Y is nonzero is
  `finale-upgrade-loss`; an object containing only zero recovered values or seeded values is
  `reader-completed-connection-array`. The rule applies one disposition to all four leaves of the
  missing object, including its identity leaf. In the 16,222-occurrence combined capture, this
  reclassifies all 27,808 former SmartShapeOptions reader-only leaves: 26,744 as completed-array
  structure and 1,064 as upgrade loss, leaving zero reader-only and zero unexpected
  SmartShapeOptions leaves.

## 2026-08-26 — Finale 2003 bend-connection upgrade loss

- **Question:** Are the Finale 2003 bend-connection offset disagreements source mapping errors,
  changed defaults, or lossy Finale 27 upgrade behavior?
- **Source evidence:** An untouched Finale 2003 document stores the eight selector-`98` Y values as
  `(0, 40, 40, 4, 48, 48, 40, 48)`. Separate controlled edits change X and Y for types 0, 1, 2, 3,
  and 6. The MUS reader and same-version ETF agree exactly in all three documents.
- **UI boundary:** Finale 2002 has no bend-connection controls, Finale 2003 presents three, and
  Finale 2004 presents the complete six-control interface. The three Finale 2003 controls therefore
  fan out across five stored semantic tuples while types 4, 5, and 7 remain hidden support entries.
- **Companion behavior:** Finale 27 resets every edited X coordinate to `(8, 0, 0, 0, 36)` and every
  edited Y coordinate to `(0, 40, 40, 4, 40)` for the five exposed tuples. It also changes the three
  hidden top-line Y values from 48 to 40. The reader preserves the source tuples as `LegacyMus`.
- **Classification:** These offset disagreements use the shared `finale-upgrade-loss` category. Its
  predicate requires the DCL epoch, source major version 8, `LegacyMus` origin, the bend-connection
  collection, and an X or Y leaf. Connection indices remain unclassified pending a discriminator.
