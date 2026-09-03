# TieOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-09-02 — TieOptions recovery

- **Public semantics:** The published `FCTiePrefs`, `FCTieContourPrefs`, and
  `FCTiePlacementPrefs` documentation describes the scalar controls, four contour types, and
  six placement groups. The latter expand to twelve start/end connection styles in musxdom.
- **Private interoperability source:** Authorized read-only inspection of a private mapping
  and its history identifies selectors 84 and 97 and their scalar positions. These facts remain
  `private-framework-derived` except where the controlled files below verify them independently.
- **Physical layout:** Record dumps of controlled Finale 97, 2000, and 2005 files show four
  selector-84 rows, four selector-85 rows, and five selector-86 rows. The logical payloads are
  respectively 24 scalar words, 24 connection words, and 28 contour words plus two zero words.
  Finale 2008 big-endian and Finale 2012 little-endian class records carry the same streams under
  classes `0x0062`, `0x0063`, and `0x0064`.
- **Controlled changes:** `F2000-tieopts-changed.mus` changes selector 84 words 0 and 1 to
  `-17` and `11`, matching the companion thicknesses. `F2000-tie-insets.mus` changes the inset
  mode and the first three contours' two fixed inset words to 17 and 13; its companion preserves
  the six values in the corresponding `TieOptions` control points.
- **Extra-system separation:** `F2005-tie-xtrasys.mus` changes Extra System Separation from
  -12 to -11. Selector 41 incidence 2 word 3 is the sole decoded MUS change, the ETF repeats
  it, and the Finale 27 companion changes only `frontTieSepar`. The corresponding zlib class
  `0x0037` carries the same flattened word at index 15; the Finale 2008 baseline stores -12
  there and its companion agrees.
- **Early-layout discriminator:** `F100-tieopts.mus` changes every control visible in Finale
  1.0's tie dialog to distinct unusual values. Selector 51 words 1 and 3 become -9 and -11;
  negating them yields the companion's `thicknessLeft` 9 and `thicknessRight` 11. Selector 50
  words 0 and 2 become 1 and 3; the companion copies them to the inner and outer-stem start/end
  horizontal connection positions. Three Finale 2.6 curve discriminators independently preserve
  these relationships with different signed values.
- **Early contour discriminator:** `F100-tieheight-inset.mus` changes only the Finale 1.0 Tie
  Height and Inset controls, to 9 and 7. Selector 22 words 4 and 5 carry those exact values;
  selector 17 word 4 changes from 6 to 3 as a derived one-third copy of height. With selector 51
  words 0 and 2 both zero, Finale 27 emits height 9 on both control points of the medium, long,
  and tie-end contours, and integer half-height 4 on the short contour. Combining this with the
  three Finale 2.6 curve discriminators establishes each side as selector 22 word 4 plus selector
  51 word 0 or 2 before the short-contour division. The inset transform remains open: this
  controlled 7 and the Finale 2.6 baseline's -12 both upgrade to fixed inset 8, while the curve
  discriminators produce asymmetric modern insets from other source words.
- **Finale 3.7 position discriminator:** `F372-tieopts.mus` changes its three Position from
  Notehead values from 6, 18, and 8 to 5, 17, and 7. The MUS and ETF place them at selector 17
  word 4 and selector 22 words 4--5. Finale 27 expands the first into the four signed inner-tie
  vertical offsets and the second into the four contour heights, halving it for the short
  contour. The third has no distinguishable modern target: the other post-baseline Finale 3.7
  companions retain 8 there while already carrying the same remaining modern tie settings.
- **Finale 3.7 PostScript discriminator:** `F372-tieopts-ps.mus` assigns distinct values to
  all thirteen PostScript tie controls, changing selector 35 word 5 and all six words of
  selectors 50 and 51. Together with the Finale 1.0 and 2.6 discriminators, it confirms the
  shared start/end X mapping, the two inner Y additions, the two thickness negations, and the
  two contour-height additions. For the short, medium, and long contours it also establishes
  fixed insets as selector 50 word 4 minus word 0 and word 2 minus word 5. The tie-end
  fixed-inset conversion remains ambiguous.
- **Obsolete tie-end displacements:** The single-edit `F100-tendxdisp-13.mus` and
  `F100-tendydisp-17.mus` fixtures locate Tend XDisp at selector 51 word 4 and Tend YDisp at
  selector 35 word 5. Each ETF repeats the one changed word. Finale 27 discards both controls;
  the complete `TieOptions` object in each companion is identical to the baseline's. These
  source locations therefore do not map to musxdom fields. In particular, selector 51 word 5
  is not Tend YDisp and remains unidentified by the controlled Finale 1.0 evidence.
- **Scattered-layout fallback rule:** Selector 84's absence, rather than a Finale 3.7 version
  test, gates the early representation. Across all 24 Finale 3.7 companions, the invariant
  unmapped values that differ from the Finale 27 baseline are applied as `LegacyBehavior`:
  disabled time- and key-signature breaks, zero left system-break adjustment, disabled outer
  placement, no seconds shift, outside/inside chord direction, disabled opposing-seconds and
  single-note-accidental handling, opposite-first mixed-stem direction, enabled tie-end control
  style, 48-EVPU short/medium/long spans, `specialPosMode::None`, and 512 for all eight inset
  ratios. Unmapped leaves whose companion values vary remain `Finale27Default`; their differences
  are classified as `different_defaults`.
- **Scope:** The implementation covers the located scattered Coda fields, the pre-selector-84
  Finale 3.7 vertical positions and contours, the later uncompressed layout, DCL, and zlib.
  Selector 41 incidence 2 remains absent throughout Coda and the tracked uncompressed epoch.
  Those layouts assign zero to `frontTieSepar` as `LegacyBehavior`; DCL and zlib recover the
  stored value. Coda fixed insets remain at the pinned value 8 and therefore report
  `Finale27Default`.
- **Tracked-evidence coverage:** The refreshed 211-source cohort contains 209 distinct source
  contents; every source and companion imported successfully. All 59 DCL and 31 zlib documents
  agree on every `TieOptions` leaf, including the controlled -11 extra-system separation. The 24
  uncompressed documents that carry the later tie families also agree completely. All 24 Finale
  3.7 documents without those
  families now have no unexpected differences: 2,084 leaves agree and 172 differences are
  classified as expected, including the two new discriminators. Across the 73 Coda documents,
  1,584 leaf disagreements are classified as expected and every document has no unexpected
  `TieOptions` differences. Five Finale 2.6 occurrences disagree on only `thicknessLeft` and
  `thicknessRight`: the source-derived value is 6 while the upgraded companion stores zero.
  Those ten comparisons are classified as Finale upgrade loss because a source-derived Coda
  thickness is preferred to the discarded companion value. The staged Finale 1.0-to-2.6 fixture
  separately preserves distinct thicknesses 9 and 11 in both the reader and companion, so the
  rule is gated by source origin, epoch, leaf, and disagreement rather than by a particular value
  or source version. The Coda leaves
  that remain at `Finale27Default` have no visible early UI control with which to discriminate a
  source mapping,
  so their varying companion disagreements are classified as `different_defaults` rather than
  being accepted as recoverable values. Four invariant patterns that disagree with the baseline
  are instead synthesized as `LegacyBehavior`: zero `frontTieSepar`, `specialPosMode::None`, zero
  `sysBreakLeftHAdj`, and 512 for all eight control-point inset ratios. Across the complete class,
  18,078 leaves agree, 1,756 differences are expected, and no tracked TieOptions difference is
  unexpected. The all-corpus capture covers 16,308 occurrences (7,272 distinct contents), of
  which 16,219 imported successfully; 4,619 occurrences have successful companions. `TieOptions`
  contributes 429,520 equal leaves and 4,666 expected differences with none unexpected. One
  expected difference is the singular Finale 2006 DCL conversion anomaly:
  source-derived `mixedStemDirection` 2 versus companion 0. The approved classification treats
  that exact release, leaf, source-derived origin, and 2-to-0 conversion as
  `finale-upgrade-loss`; broader mixed-stem disagreements remain unexpected.
