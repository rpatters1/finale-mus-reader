# PianoBraceBracketOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-28 — Finale 1 piano-brace float discriminator

- **Controlled edit:** `F100-brackets.mus` changes three Finale 1.0.0 float controls from
  `F100-baseline.mus`: Beam Depth to approximately phi, Piano Brace 1 to approximately pi, and
  Piano Brace 2 to approximately e. Its ETF and Finale 27 MUSX conversion accompany it.
- **Physical result:** A complete globals diff changes only three big-endian IEEE-754 values.
  Selector `52` word 4 stores `1.61802995`; selector `55` words 2 and 4 store `3.14159012` and
  `2.71828008`. The ETF prints exactly the same words. This separates selector `52`'s Beam Depth
  from the two brace parameters despite their shared dialog.
- **Companion result:** Finale 27 leaves all twelve `PianoBraceBracketOptions` leaves at the values
  produced from the unedited baseline. By itself this conversion supplies no mapping from Piano
  Brace 1 or 2 to the modern eleven-field brace geometry.
- **Population comparison:** The tracked companions convert 34 of 39 Coda specimens to width 12
  and five to width 24. Selector `55` does not separate them: width-12 and width-24 specimens,
  including both outcomes within the Finale 2.6.3 group, carry the identical floats 1, 3, and 3.
- **Later discriminator:** `F263-brace-psvs.mus` changes Beam Depth to 11.5, Def Line Width to 1.3,
  Piano Brace 1 to 12.7, and Piano Brace 2 to 13.1. Selector `52` word 4 changes 3 to 2.875,
  selector `54` word 0 changes 0.25 to 0.325, and selector `55` words 2 and 4 change 3 to 3.175 and
  3.275. Its Finale 27 companion changes only `innerTipH` to 12.6996 and `innerBodyH` to 13.1 in
  `PianoBraceBracketOptions`. Thus the two selector-55 values map in order to those fields at a
  factor of four; Beam Depth and Def Line Width are separate preferences despite sharing the dialog.
- **Scale conclusion:** Finale 1 displays selector `55`'s stored floats directly, whereas Finale
  2.6.3 displays four times the stored floats. The storage and modern semantic scale nevertheless
  stay constant: the default stored 3 recovers as modern 12 and matches both eras' companions.
- **Implementation consequence:** Coda-banner files do not use the later selector `14`, `45`,
  `60`, `61`, `64`, and `65` table. The importer recovers `innerTipH` and `innerBodyH` from selector
  `55` at four times the stored float. It supplies center thickness 2, width 12, and the other seven
  geometry leaves zero as `LegacyBehavior`; `defBracketPos` remains at the pinned default pending a
  discriminator. Finale 27 drops the deliberately edited Finale 1 values, so exact disagreements on
  these source-backed fields are `finale-upgrade-loss`.
- **Width conversion experiment:** The committed Finale 2.6.3 baseline companion carries width 24.
  Two temporary reconversions of the exact unchanged source both carried width 12; restoring the
  committed companion restored width 24. The curve-options and brace-parameter edits also convert
  to width 12, although their complete source-record differences are disjoint and neither changes a
  stored width. Whether the temporary conversions differ because one used the UI and another a Lua
  script remains open, but the source cannot determine 12 versus 24. A differing width whose reader
  origin is `LegacyBehavior` is therefore narrowly classified as `different_defaults`.
- **Finale 2004 bracket-position discriminator:** `F2004-brakpos-17.mus` changes the option from
  12 to 17. Selector `14` word 3 moves -12 to -17, its ETF repeats the row, and the companion reads
  `defBracketPos` -17. The save also normalizes center thickness from 2.0000 to 1.9987 and three
  horizontal geometry values from 12.0000 to 12.0009; source and companion agree on all four, so
  they are not attributed to the bracket-position edit. The first ungated tracked run leaves the
  new class at 12 same leaves and adds no difference. All 46 remaining unexpected class leaves are
  `defBracketPos` before Finale 2004: 12 Finale 3.7, nine Finale 97, eight Finale 2000, ten of eleven
  Finale 2002, and seven Finale 2003 documents. Every Finale 2004-and-later document agrees. The
  implemented mapping therefore ignores selector `14` word 3 before Finale 2004 and retains the
  pinned Finale 27 default of -12; Finale 2004-and-later DCL and every zlib source recover the word
  when its record is present.
- **Tracked recovery result:** The refreshed tracked-evidence survey contains 140 distinct sources,
  all with successful companions, including 39 Coda documents. After the Finale 2004 gate, the
  class comparison counts 1,671 same leaves, nine expected leaves, and no unexpected leaves out of
  1,680. All 37 unedited Coda
  documents match on both recovered selector-55 values. Four expected differences are the two
  fields in each controlled edit: Finale 27 discards both Finale 1 changes and emits slightly
  altered decimal values for both Finale 2.6.3 binary floats. The other five expected differences
  are the width-24 conversions classified as `different_defaults`.
- **Full-corpus behavior refinement:** The first all-corpus run covers 16,237 occurrences representing
  7,203 distinct source ids, with 4,548 companion-backed occurrences. It exposes 79 unexpected class
  leaves in 44 distinct documents. Twenty-four Windows Coda documents omit selector `55`; the pinned
  values 24 and 0 for `innerTipH` and `innerBodyH` disagree with raw companions that explicitly store
  12 and 12. Nine Finale 3.0-through-3.5 documents carry an all-zero selector `45`, while eleven
  Finale 3.0 documents omit it; their raw companions establish fixed `centerThickness` 2 and
  `tipThickness` 0 before the stored options begin. Selector presence therefore cannot mark the
  latter boundary. The importer now supplies the missing Coda horizontal values as behavior and
  applies the two thickness behaviors before a provisional Finale 3.7 boundary. The authorized
  rerun has 54,567 same leaves, nine approved differences, and no unexpected differences for this
  class out of 54,576. Across every surveyed class it has no unexpected companion differences.
