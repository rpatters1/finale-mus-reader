# LineCurveOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-25 — Finale 2.6.3 curve-options discriminator

- **Question:** Does a controlled edit in Finale 2.6.3's curve-options dialog survive conversion into modern
  `SmartShapeOptions`?
- **Controlled pair:** `mus-e0d207b83365f47d` was saved from `mus-ec910badddfa6699`. A complete fixed-global
  record diff found changes only in selectors `15`, `35`, `50`, and `51`; the modified ETF independently gives
  the same values. Selector `50` changes from `(0, 0, 0, 0, 8, -8)` to `(1, 3, 5, 7, 11, -11)`, and selector
  `51` from `(0, -6, 0, -6, 0, 8)` to `(13, -13, 17, -17, 3, 5)`.
- **Companion result:** An exact semantic comparison of the two Finale 27 companions finds only two changed
  `SmartShapeOptions` leaves: `slurThicknessCp1Y` and `slurThicknessCp2Y`, each 6 to 13. Other parts of the old
  dialog are distributed into `LineCurveOptions`, `PianoBraceBracketOptions`, and `TieOptions`.
- **Reader result:** A fresh two-row recovery capture completed with two successful Coda-banner imports and two
  successful companion comparisons. The reader currently leaves both thickness values at the pinned baseline's
  6, with `Finale27Default` origin, so the modified document reports two additional unexpected differences at 13.
- **Discriminator:** `mus-d2076c77b9f3f65d` reverses the C1 displacement signs and gives the two sides unequal
  magnitudes. Selector `51` begins `(-13, 17)`, and both modern thickness leaves become -17. This confirms that
  Finale 27's upgrade discards selector `51` words 0, 2, and 3 rather than showing that the source did not use
  them. The fixture also changes non-PostScript tie settings in selectors `17` and `22`, but an exact subtree
  comparison finds no additional `SmartShapeOptions` change.
- **Rendered discriminator:** `mus-ba0652c39a4fd350` carries an actual legacy curve and gives selector `51` the
  first four words `(131, -171, -173, 179)`. Comparing its appearance before and after upgrade shows that Finale
  27's discarded horizontal values and duplicated first vertical value visibly distort the curve. A manually
  corrected modern companion, SHA-256
  `e8f361373c9962ddae60844da1b597ecaf19494c0c05194d85d654333ee26e95`, stores the four modern
  fields as `(131, 171, -173, -179)` and reproduces the legacy appearance.
- **Implementation:** The Coda-banner table treats selector `51` words 0–3 as two `(horizontal, vertical)` pairs.
  Horizontal values map directly to the corresponding thickness-control X field; vertical values map sign-inverted
  to the Y field. This deliberately preserves source geometry rather than reproducing Finale 27's lossy upgrade.
  All other Coda Smart Shape scalars remain unsupported. After review, the nine differing `Cp1X`, `Cp2X`, and
  `Cp2Y` leaves in the three edited fixtures are classified as `finale-upgrade-loss`; the rule requires
  Coda-banner epoch and `LegacyMus` origin and does not include `Cp1Y`, which Finale upgrades correctly.

## 2026-08-25 — Finale 3.x crescendo line-width boundary

- **Question:** Is `FI` comparator 11 word 1 an independently meaningful crescendo-line width
  throughout the uncompressed epoch?
- **Pre-3.7 evidence:** Nineteen paired sources from Finale 3.0, 3.2, and 3.5 store word 1 as 0, 32,
  or 72 and word 4 as 111 or 118. Every modern companion uses the recovered word-4 value for both
  `smartLineWidth` and `crescLineWidth`. The same sources store word 2 as 0 or 8, while every
  companion uses the pinned baseline's `hookLength` of 12.
- **Boundary discriminator:** A Finale 3.7 source stores 144 in word 1, 12 in word 2, and 118 in
  word 4. Its modern companion preserves all three values independently, and every other Smart
  Shape option leaf matches.
- **Implementation:** Inside `UncompressedLegacy` only, recovered versions 3.0 through 3.6 copy
  `smartLineWidth` to `crescLineWidth` and report the result as `LegacyBehavior` with word 4's source
  evidence. The same gate ignores word 2 and retains the selected baseline hook length with
  `Finale27Default` origin. Finale 3.7 and later, other epochs, and files without a recovered version
  retain direct word-1 and word-2 recovery. The gate is version-based because the six-word `FI` row
  has no identified structural change at the boundary.
- **Confidence:** **Strong** across the stated range: the available versions agree and Finale 3.7
  discriminates the upper boundary, but no paired 3.1, 3.3, 3.4, or 3.6 specimen exists.

## 2026-08-30 — LineCurveOptions legacy preference families

- **Question:** Which `LineCurveOptions` members have legacy preference locations, which locations
  reach the Coda-banner epoch, and which members instead describe fixed legacy behavior?
- **Public semantics:** The public `FCSizePrefs` and `FCMiscDocPrefs` documentation identifies the
  enclosure, staff, ledger, Shape Designer slur-tip, curve-resolution, and PostScript-underline
  properties and their units. The pages were accessed 2026-08-29.
- **Authorized private lead:** The authorized read-only Framework preference tables
  locate the eleven stored fields at selectors `01`, `15`, `27`, `58`, `59`, `62`, and `97`. The
  corrected Framework tree contains no rounded-enclosure or corner-radius preference. These facts
  remain **private-framework-derived** except where controlled evidence independently confirms them.
- **Controlled Coda result:** Finale 2.6.3 curve-options edits move selector `15` word 4 from 16
  to 33 and 67, and both modern companions preserve the edited value. All tracked Finale 1.0.0
  sources store zero there and upgrade to 16, establishing the Coda zero sentinel. Finale 2.6.3
  also carries selector `62`; its high-word-first values decode to `psUlDepth = -0.25` and
  `psUlWidth = 0.0419`. The controlled Finale 1.0.0 line-options edit identifies the earlier
  representation at selector `52`: its first two single-precision values move from -0.25 and
  0.0416 to -0.37 and 0.0713 in both MUS and ETF. Finale 27 discards both edits.
- **Finale 2.6.3 resave:** Opening that edited Finale 1 document and saving it in Finale 2.6.3
  retains selector `52`'s float bits and adds selector `62`'s later fixed-point values. Depth
  becomes -3700 exactly, while the stored float just below 0.0713 truncates to width 712. Thus
  the later semantic values are -0.37 and 0.0712, and the Finale 27 companion preserves both.
  Finale 2.6.3's factor-three UI presentation does not enter either stored representation.
- **Later-layout result:** Uncompressed, DCL, and zlib synthetic tests cover every located field;
  the zlib test covers both byte orders. Rest-ledger words are treated as stored only from DCL
  onward. All 37 tracked uncompressed sources carry zero while their companions and the pinned
  baseline use 3, so those earlier fields remain synthesized defaults.
- **Legacy behavior:** Rounded-enclosure and corner-radius controls are MUSX-only features. No public
  or authorized private legacy preference names them, and all 173 tracked companions across Coda,
  uncompressed, DCL, and zlib agree on square corners and radius zero. The importer reports that
  corresponding legacy behavior rather than pretending either value was recovered. Coda predates
  the enclosure-, staff-, and ledger-line width controls present by Finale 3.7.2. Upgrading the
  controlled Finale 1.0.0 line-options document through Finale 3.7.2 assigns 118 to all three, and
  the Finale 3.7.2 baseline independently stores 118 at the three later selectors. This supersedes
  the initial Finale 97 default hypothesis of 128 and 256. Because 118 differs from the pinned
  Finale 27 value 115, all three are reported as `LegacyBehavior` rather than `Finale27Default`.
  This applies the repository-wide origin rule: an unstored historical behavior remains
  `Finale27Default` whenever the pinned baseline already supplies the same value, regardless of
  epoch.
- **Tracked result:** The instrumented Release capture reads all 173 sources and all 173 companions:
  60 Coda, 38 uncompressed, 50 DCL, and 25 zlib; 171 source contents are distinct. `LineCurveOptions`
  has 1,993 equal leaves, 256 expected leaves, and no unexpected leaves. With the owner's approval,
  the two source-owned underline differences in each of the 38 Finale 1.0.0 fixtures are classified
  as `finale-upgrade-loss`; the controlled edit contributes -0.37 and 0.0713 rather than the stock
  -0.25 and 0.0416. The Finale 2.6.3 resave is the converse: both selector-62 values equal its
  companion and receive no classification. The 56 each for Coda enclosure, staff, and ledger width
  118 → 224 are classified as `different_defaults`; the rule requires the Coda epoch,
  `LegacyBehavior` origin, the exact values, and one of those three paths. No all-corpus probe was
  run at that stage.
- **Full-corpus refinement:** The subsequently authorized three-survey capture selected 16,270
  occurrences representing 7,234 distinct source ids. It imported 16,181 occurrences representing
  7,161 distinct ids, and all 4,581 companion-backed occurrences representing 2,994 distinct ids
  compared successfully. Its only 17 unexpected leaves were distinct uncompressed Finale 3.0-3.2
  sources with no selector `27`: `enclosureWidth` retained the pinned 115 while the raw companions
  explicitly stored 224. The importer now treats selector-`27` absence in the uncompressed epoch
  as enclosure-width behavior 118 and the exact 118-to-224 disagreement as `different_defaults`.
  The authorized post-change recapture preserved the same selection funnel and moved all 17 leaves
  into the approved classification: `LineCurveOptions` now reports 58,936 equal leaves, 617 expected
  leaves, and no unexpected leaves. The full report has no unexpected differences in any surveyed
  class. The same 89 source occurrences fail before comparison: 58 LIB files and 31 inputs that do
  not identify as MUS documents.

## 2026-08-30 — Coda line-width and stem-size migration through Finale 2.6.3

- **Question:** Whether the original Coda Def Line Width, Stem Line Width, and Stem Lift floats
  have later semantic equivalents despite being discarded by a direct Finale 27 upgrade.
- **Controlled source:** A Finale 1.0.0 save sets the three fields to 3.14159, 2.71828, and 1.618.
  Its MUS and ETF store them as selector `54` floats 0 and 2 and selector `55` float 0. The direct
  modern companion normalizes the related fields to `graceSlashWidth = 224`, `stemWidth = 224`,
  and `stemLift = 256`.
- **Finale 2.6.3 migration:** Opening and saving that source retains the floats and writes 31415 at
  selector `64` word 1, 27182 at selector `64` word 5, and 16180 at selector `65` long 0. Its
  modern companion contains 804, 696, and 414 respectively.
- **Finale 3.7.2 migration:** A second staged save converts the same values into Efix 804, 696, and
  414 at the later locations; its modern companion agrees exactly.
- **Decision:** Selector `64` presence selects the Coda ten-thousandths layout and makes it
  authoritative. Without it, the original floats are converted directly from points to Efix.
  Def Line Width recovers `graceSlashWidth`, Stem Line Width recovers `stemWidth`, and Stem Lift
  recovers musxdom `stemOffset` (XML `stemLift`). A disagreement on those three fields is classified
  as Finale upgrade loss only in the Coda epoch when selector `64` is absent and the original-layout
  field is source-owned. The unrelated obsolete selector-`21` Stem Offset remains unmapped.
- **Tracked result:** All 173 sources and companions import. The three migrated fields produce
  exactly 114 expected Finale-upgrade-loss leaves: 38 each for `graceSlashWidth`, `stemWidth`, and
  `stemOffset`, all in original-layout Coda sources. The Finale 2.6.3 and Finale 3.7.2 migration
  fixtures compare equal on all three fields.
