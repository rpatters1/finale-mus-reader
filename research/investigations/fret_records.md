# FretRecords investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-26 — Fret record identities and layouts

- **Public-header search:** Searched every header in the immutable public Finale 2000 PDK
  snapshot for `Fret` and for `EDT` declarations containing it. No target fret declaration was
  present. The current public Framework headers expose the style, instrument, group, diagram
  header, and diagram item types. They publish `ft` for the style and `fb` for the diagram; the
  instrument tag implementation is out of line and the group uses the unresolved symbolic
  `otx_FretGroup`.
- **Record correlation:** Existing zlib catalog candidates `0x0094`, `0x0095`, `0x0097`, and
  `0x0413` were compared byte-for-byte with the independently decoded Finale 27 companion for
  `mus-21b30fb5dfc9bca2`. The public structures explain every payload boundary, including the
  diagram's two-items-per-incidence padding and the separate cell/barre incidence arrays.
- **Source ownership:** The source and companion contained identical instance sets for all four
  classes. Importers therefore construct only stored records and leave an absent class empty; no
  baseline fret objects are copied.
- **Capture result:** One instrumented Release row imported and compared successfully. Instruments
  agreed on 510 leaves, groups on 465, diagrams on 55,373, with no reader-only or companion-only
  fret instances. Style numeric leaves agreed. Three source/companion name leaves remain unequal
  only in the musxdom observation because its `FretboardStyle::name` XML mapping tokenizes at
  whitespace; direct companion XML retains the same full names as the source.
- **Fixed-row identities:** A supplied locator identified group `fg`, instrument `fI`, style `ft`,
  and diagram `fb`, with the diagram header at incidence 0. Direct record dumps independently
  verified all four in Finale 2001 DCL files. Concatenating each fixed incidence reproduces the
  already-decoded zlib payload byte-for-byte.
- **Boundary census:** None of 186 filesystem-origin Finale 2000 paths (149 distinct contents) in
  `rpatters1-main` contains any of the four tags. Four distinct Finale 2001 documents contain all
  four; the remaining Finale 2001 content is a `.FAN` without them. This supports a **strong** 2001
  introduction boundary, but record presence remains the implementation's structural gate.
- **Fixed-row capture:** `mus-46c4619dfdc99ae6` agreed on all 459 group and 54,601 diagram leaves,
  plus all companion instrument and numeric style leaves. Two stored instrument diatonic values
  were reader-only because Finale 27 omitted them, and three style names reproduce the known
  companion whitespace-tokenization issue. No companion-only fret instances appeared.
- **Tracked-evidence capture:** The instrumented Release probe read all 125 distinct tracked
  sources and all 125 Finale 27 companions: 33 Coda-banner, 25 uncompressed, 44 DCL, and 23 zlib.
  With no fret-specific expected-difference classifications, instruments report 2,968 equal,
  one reader-only, and 928 companion-only leaves; groups report 1,556 equal and 280 unequal;
  styles report 2,250 equal, 12 unequal, and 1,682 companion-only; all 218,404 diagram leaves
  agree.
- **Mixed byte order:** The first tracked pass exposed big-endian instrument pitches such as
  16,384 where same-version ETF stores 64. Fixed and class-record tuning words remain
  little-endian while the surrounding header follows container order; the diatonic mask likewise
  has a fixed low-word/high-word order. A big-endian synthetic test now exercises both facts.
- **Companion synthesis:** Every one of the 58 pre-2001 tracked companions adds one 16-leaf
  `FretInstrument` and one 29-leaf `FretboardStyle`, for 928 and 1,682 companion-added leaves.
  No pre-2001 source contains any of the four fret record identities, and no companion adds a
  group or diagram. This confirms modern upgrade synthesis but does not change the reader's
  source-only construction policy.
- **Upgrade byte swap:** Four big-endian Finale 2006 DCL companions byte-swap the instrument
  reference in every later fretboard-group incidence: `1` becomes `256` and `2` becomes `512`.
  Incidence zero, the raw source words, and same-version ETF retain `1` or `2`, and the companions
  define no fret instruments 256 or 512. A recovered fretboard-group instrument reference is
  classified as `finale-upgrade-loss` when a big-endian source and its companion differ by exactly
  a 16-bit byte swap. The same four companions also reorder
  `Minor7 b5 R4 (copy)` to `Minor7 b5 R4( ocyp`, while ETF preserves the source text. One Finale
  2002 companion omits a stored diatonic-fret leaf. The group name's eight-byte C-string tail is
  transformed from ` (copy)\0` to `( ocyp\0)`, swapping each adjacent byte including the final
  parenthesis with the terminator. It is classified as `finale-upgrade-loss` only for that exact
  big-endian transformation. The 12 style-name differences exposed a musxdom mapping that used
  numeric token extraction for text; after that mapping was corrected to read the complete XML
  content, those names agree. The diatonic-fret omission remains visible and unclassified pending
  review.
- **Older-display hypothesis:** The suggestion that pre-2001 chord fretboards used only Seville
  font glyphs is **open**. The tag census proves only that these four editable record families are
  absent, not what older Finale used to display fretboards.
- **Finale 2012 group layout:** Content `mus-559cbb2d3a3ca516` stores a 408-byte group-class
  payload containing exactly two 204-byte tuples. Their instrument comparators occur at offsets 0
  and 204, and their names begin at offsets 12 and 216 as UTF-16LE, independently matching the two
  Finale 27 companion incidences. The 12 tracked Finale 2012 fixtures contain no fretboard-group
  source record, which is why tracked-evidence coverage did not exercise this boundary. Authorized
  Framework history names a distinct F2012 group layout; that corroboration remains
  `private-framework-derived`, while the byte layout itself is independently corpus-verified.
- **All-corpus F2012 correction:** After selecting the 204-byte tuple and UTF-16LE name at the
  shared Finale 2012 Unicode boundary, group reader-only leaves fell from 164,184 to zero and
  unexpected group differences fell from 28,729 to eight. The same-leaf count rose from 654,673
  to 683,394. The exposing content now agrees on all 405 group leaves. The eight remaining group
  observations are the same name difference, `Minor7 b5 R4` versus `Minor7 b5 R4t`, repeated
  across corpus occurrences. The raw big-endian record ends that visible source string with the
  word `00 74`; Finale's companion has `74 00`, so the existing byte-swapped C-string-tail
  classification now includes the word containing the terminator.
- **Pre-25.3 tuning reinterpretation:** Several Finale 2003 records contain nonsensical old
  signed pitch words with nonzero high bytes. Finale 27 splits each word as though it were the
  post-MUS Finale 25.3 string structure: `0x5089` becomes pitch 137 and nut offset 80, and the
  other observed pairs behave identically. The paired classification requires the companion's
  pitch and nut offset to reconstruct the complete source word exactly. A focused capture of
  `mus-227c6cf2d1aed761` moved all ten affected leaves from unexpected to `finale-upgrade-loss`.
  The reader retains the old pitch word and reports zero nut offset as `LegacyBehavior`. The
  complete all-corpus rerun classified all 518 such instrument leaves and left none unexpected;
  the terminator-word extension likewise classified the eight remaining group-name observations.
- **Confidence:** **Confirmed** for DCL and zlib layouts, identities, and source-owned construction;
  **strong** for the observed Finale 2001 introduction boundary.
