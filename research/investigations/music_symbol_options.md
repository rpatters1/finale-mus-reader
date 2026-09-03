# MusicSymbolOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-30 — MusicSymbolOptions narrow words and Unicode expansion

- **Starting hypothesis:** The suggested `mc` options selector did not occur literally in the
  tracked ETFs or as a tagged fixed row. The companions nevertheless showed that most fields
  retained byte-sized legacy music characters, while Finale 2012 could carry values above
  `U+FFFF`.
- **Narrow correlation:** Comparing the option globals against the companion field order located
  all 65 fields across selectors `5`--`12`, `18`, `19`, `23`, `38`, `42`, `43`, `46`, `69`, and
  `75`. The words are 16 bits wide but behave as stored bytes. Coda comparisons rejected several
  tempting same-offset readings; those fields now fail closed to seeded defaults in that epoch.
- **Unicode record:** Pre-2012 zlib files carry class `0x0059` as selector `75`'s 12-byte row.
  Finale 2012 expands the same class to 276 bytes. After the original row are 65 low-word-first
  32-bit code points in exact musxdom order and a four-byte trailer. The controlled upstem-flags
  edit sets the final two array values to 183, while the prefix remains the ordinary flag-layout
  record, establishing that the extension—not the prefix—owns the Unicode fields.
- **Implementation boundary:** Before zlib, the three parts time-signature symbols derive from
  the shared score settings described below. Coda fixed behaviors and upgrade conversions remain
  visible rather than being converted into inferred source mappings.
- **Initial tracked result:** All 179 tracked sources and companions completed. Before the
  font-aware decoding and shared-parts correction, `MusicSymbolOptions` had 11,179 equal leaves
  and 456 unexpected leaves out of 11,635. No expected-difference rule was added.

### Time-signature symbols for linked parts

- **Pre-zlib behavior:** Finale 2006 exposes no independent parts options. The controlled plus
  edit changes selector `42`, word 5 from `43` to `44`; its ETF and Finale 27 companion use `44`
  for both `timeSigPlus` and `timeSigPlusParts`. The importer propagates all three recovered score
  symbols to the corresponding parts fields as `LegacyBehavior` throughout the pre-zlib epochs.
- **Zlib mapping:** The controlled Finale 2008 edit leaves selector `42`, word 5 at `43` and
  stores the independent parts plus at numeric selector `18` class `0x0020`, word 11. The revised
  fixture also leaves the score common/cut symbols at `99`/`67` while storing parts values
  `98`/`69` as unpacked words 12/13. The three zlib parts values use `TimePlusParts`, `TimeParts`,
  and `TimeParts`; earlier propagation retains the score fields' `TimePlus` and `Time` decoding.
  No Finale 2007 specimen was available, so the exact release boundary within the structural
  epoch was not asserted.
- **Validation scope:** Focused unit and fixture tests exercise the revised mapping. In the fresh
  tracked capture, both Finale 2006 fixtures and the revised Finale 2008 fixture compare equal
  across all 65 `MusicSymbolOptions` fields.

### Pre-Unicode character decoding

- **Interpretation:** Each narrow music-symbol word contributes its low byte in the encoding of
  the font category that draws that field. It is not intrinsically Mac Roman, Windows-1252, or
  Unicode. The importer now keeps the category beside the selector-word mapping and passes every
  pre-2012 value through `text::codepointFromByte`.
- **Controlled distinction:** In the Finale 1.0.0 accidental edit, bytes `0xba` and `0xdc` remain
  symbol-glyph values for ordinary accidentals but decode as `U+222B` and `U+2039` for chord
  accidentals because the controlled `ChordAcci` setting names a text font. In the key-font edit,
  the ordinary accidental font remains Petrucci while the key-signature font is Monaco. Coda
  shares the stored accidental bytes, so the ordinary characters remain `0xba` and `0xdc` while
  the key-signature characters decode as `U+222B` and `U+2039`.
- **Fallback and era behavior:** A missing font setting uses font id zero and symbol fallback.
  Before the zlib epoch, shared score/parts time symbols use the score font category; the Finale
  2012 long array is already Unicode and is not re-decoded.
- **Finale 2.6.3 character sample:** The controlled sample moves the quarter notehead and four
  flag bytes by 16, and its ETF and companion preserve all five changes at the mapped locations.
  The same UI session changed the 128th-rest control, but the source MUS and ETF remained unchanged,
  the companion retained 229, and relaunching Finale restored the prior setting. Finale 1.0.0 has
  no 128th-rest control. Selector `10` word 3 is therefore still excluded before Finale 97.
- **Coda behavior:** Neither tested Coda dialog exposes independent key-signature
  characters and neither file carries selector `46`. The importer now shares the five ordinary
  accidental bytes with the key-signature fields as `LegacyBehavior`; each copy is decoded through
  the key-signature font rather than retaining the ordinary accidental font's decoding. Although Finale 2.6.3 selector
  `11` word 5 and most of its companions carry 215 for `eightVbDown`, the UI exposes no independent
  character and Finale 1.0.0 has no smart shapes. The importer therefore leaves that later field at
  the pinned default rather than asserting one Coda-wide behavior.
- **Tracked recapture:** The instrumented Release probe completed all 184 occurrences and all 184
  companions with no failures, representing 182 distinct `corpus_id` values. `MusicSymbolOptions`
  has 11,500 equal, 460 expected, and no unexpected leaves out of 11,960. All expected
  occurrences are in the 64 Coda-banner rows (62 distinct `corpus_id` values); the uncompressed,
  DCL, and zlib rows have none for this class.
- **Reviewed default differences:** Straight flags do not exist in Coda, so the 128 differing
  straight-flag leaves are `different_defaults` only with `Finale27Default` origin. Removing the
  `eightVbDown` override makes 42 former disagreements exact matches and exposes the converse 21
  pinned-default 195 versus companion 215 differences. The same classification covers 80 one- and
  two-bar-repeat leaves for controls Coda does not expose, 64 `fifteenMbDown` leaves with no Coda
  source setting, 39 128th-rest leaves whose visible control does not persist, and 128 `flag16`
  leaves with no independent Coda controls. Every rule has the same Coda and origin constraints;
  a recovered source value remains strict.
- **Resolved Coda spread:** Re-decoding the shared bytes through the key-signature font makes the
  controlled Finale 1.0.0 double accidentals agree with the companion's `U+222B` and `U+2039`.
  Representative Finale 27 EnigmaXML omits the two 16th-flag fields, so companion zero comes from
  musxdom's omitted-field behavior rather than a stored Finale value; the complete Coda flag UI
  establishes their companion disagreements as different defaults.
- **Finale 1.0.0 flag-character edit:** Editing every flag character exposed by the UI changes
  selector `7` word 5, selector `8` word 0, and selector `10` words 1 and 2, confirming the
  existing `flagUp`, `flagDown`, `flag2Up`, and `flag2Down` mappings. The ETF repeats the four
  changes and Finale 27 preserves their edited values. No independent `flag16Up` or
  `flag16Down` setting exists in that UI, so those leaves remain at `Finale27Default`; their
  Coda companion disagreements are now classified as `different_defaults`.
- **1996 documentation boundary:** Page 4 of Werner Eickhoff's *Susato: Variations on a
  Masterpiece* (MusicFontLab, 1996) says Finale 3.5 made the expanded flag adjustments available
  and Finale 3.5.1 introduced separate 16th-note flags
  ([source](https://pdfcoffee.com/susato-font-pdf-free.html), accessed 2026-08-31; PDF SHA-256
  `4b21bda90a33c9fd03c8784e362c35b58c8178283c667cddd4f71aeee434e83a`). The
  existing `Finale27Default`-only classifications for `eightVbDown`, one- and two-bar repeats,
  `fifteenMbDown`, and the two straight flags now cover every source before Finale 3.5. The two
  16th-note flags cover every source before Finale 3.5.1. `rest128th` retains its separately
  established Coda-only rule. Supporting the point-release boundary required maintenance to
  participate in the shared `VersionBound` ordering. No coverage capture was run.
- **Configured symbol-font normalization:** The all-corpus MusicSymbolOptions result identified
  one distinct Finale 2011 source whose Noteheads font is `FinaleAlphaNotes`. Its source font
  definition stores Mac charset 0, while both Finale 27 companions store Mac symbol charset
  4095; the face is explicitly listed in the supplied `MacSymbolFonts.txt`. Symbol-font names
  are now applied once to imported font definitions for either charset bank. The returned
  document persists the bank-specific symbol charset, the report retains the stored value as
  `LegacyMusAdjusted`, and individual character decoders no longer accept the list. No coverage
  probe was run after this change.
- **Reviewed stem default:** The same fixture exposes an unrelated first-connection
  `upStemHorz` difference: the source-derived value is 0 and the companion carries -59. The
  source of the upgraded value is unresolved and may be calculated. Remaining Coda differences
  on that source-owned path are classified as `different_defaults`, after the separately
  characterized `stem-horizontal-correction` values. The refreshed tracked report has 29,353
  expected leaves and no unexpected leaves.
