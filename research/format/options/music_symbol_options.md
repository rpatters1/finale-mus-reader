# MusicSymbolOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Music symbol options

**Implemented.** The pre-Unicode layout distributes music characters across
numeric globals. Each value occupies one 16-bit word, but the character is the low byte;
zero-extended and sign-extended bytes decode identically. The zlib epoch preserves those
locations as `numericGlobalClass(selector)` records. The implemented selector families are:

| Selectors | Recovered fields |
|---|---|
| `5`--`10` | noteheads, ordinary accidentals, rests, augmentation dot, and curved/secondary flags |
| `11`, `12` | chord accidentals and octave symbols |
| `18` | parts abbreviated-common, abbreviated-cut, and plus time-signature symbols in the zlib epoch |
| `19`, `23` | abbreviated score time signatures and repeat dots |
| `38`, `42`, `43`, `46` | parenthesized and key-signature accidentals, slash/time-plus symbols, and bar repeats |
| `69` | two-octave, trill, and wiggle symbols |
| `75` | straight upward and downward flags |

Selector `75` also supplies the structural transition to Unicode. Before the extension it is
the ordinary six-word straight-flag row. The extended class `0x0059` payload is exactly 276
bytes: that 12-byte row, followed by all 65 persisted `MusicSymbolOptions` fields as 32-bit
code points in musxdom field order, followed by four bytes. Each code point stores its low
16-bit word first, with each word using the container's byte order. The payload size selects
the layout directly; no Finale 2012 version gate is required. **Confirmed** in both byte
orders synthetically and by controlled Finale 2012 sources, including a straight-flag edit
that places 183 in the final two array elements.

The narrow locations are source-derived from controlled and companion comparisons. Before the
zlib epoch, the score's abbreviated-common, abbreviated-cut, and plus symbols are the stored
options used by both the score and linked parts; the importer propagates them to the three parts
fields as `LegacyBehavior`. In the zlib epoch numeric selector `18` class `0x0020` stores the
independent parts symbols as unpacked words: plus at word 11, abbreviated common at word 12, and
abbreviated cut at word 13. The controlled Finale 2008 edit stores `42`, `98`, and `69` there while
the corresponding score values remain `43`, `99`, and `67`.
Every pre-2012 value is a byte in the encoding of its applicable `FontOptions` category, not a
Unicode code point. The importer's single field manifest therefore owns both the selector-word
location and the font type used by `text::codepointFromByte`; unresolved and symbol fonts retain
the byte as the symbol-glyph value. The score time fonts are also the effective font categories
for the corresponding shared parts behavior before the zlib epoch. The Finale 2012 extended
array already contains Unicode code points and bypasses this conversion.

The Coda epoch has no independent key-signature character controls in the tested Finale 1.0.0
and 2.6.3 dialogs, and neither era carries selector `46`. Its five key-signature characters
therefore reuse the corresponding ordinary accidental bytes as `LegacyBehavior`, but decode those
bytes through the key-signature font. Selector `46` remains the source-owned family after Coda.
Finale 2.6.3 displays a 128th-rest control but does not save its changes: a controlled edit leaves
the MUS and ETF unchanged, the setting reverts after relaunch, and the modern companion retains
229. Finale 97 is the first version whose UI exposes the default-measure-rest character. Its
controlled edit changes selector `9` word 4 from 183 to 206, with the ETF preserving 206, while
the importer already gates both the 128th-rest and default-measure-rest locations at Finale 97.
Finale uses the configured default-measure-rest character only when it recognizes that glyph as
a rest; otherwise it renders the whole-rest glyph. This explains why an unrecognized stored value
can disagree with a companion that contains the whole-rest character without indicating a reader
decoding error. The importer implements the identifiable zero case by replacing a recovered zero
with the already-decoded `restWhole` value and reporting `LegacyMusAdjusted`; the report retains
zero as the raw source value.

`eightVbDown` remains at the pinned default in Coda. Finale 1.0.0 has no smart shapes, and the
Finale 2.6.3 UI has no independent character setting; selector `11` word 5 is therefore not treated
as a Coda-wide source mapping despite its value 215. Repeat dots, `eightVaUp`, straight flags, and
16th-note flags are not read in Coda. Unsupported fields retain the pinned Finale 27 values and
report `Finale27Default`.

The repeat dots, `eightVaUp`, `slashBar`, `quarterSlash`, `dblWholeSlash`, `eightVbDown`, one-
and two-bar repeats, and the straight flags begin with Finale 3.5. `fifteenMaUp`,
`fifteenMbDown`, `trillChar`, `wiggleChar`, and the separate 16th-note flags begin with Finale
3.5.1. Earlier selector words can contain plausible character values or zeroes, but they do not
represent those modern settings and are left at the pinned defaults. These version boundaries are
framed inside the uncompressed epoch; DCL and zlib sources use the same locations without needing
a recovered version.

For Coda-banner sources that carry no header version tuple, differing `halfSlash` and `wholeSlash`
values are treated as different defaults. A version inferred from the Coda product string has a
zero raw version and remains inside this structural gate. Their recovered glyph numbers can both be
normalized to the same companion glyph, and the available evidence cannot distinguish a Finale
upgrade default from an early Windows font-layout change.

Retained `Finale27Default` values for `quarterSlash` and `slashBar` are different defaults
throughout the Coda epoch. `dblWholeSlash` retains the narrower no-header-version condition.
Unlike the source-recovered `halfSlash` and `wholeSlash` case, recovered values for these three
fields remain strict. The exact double-whole-slash 218-to-213 conversion remains the more
specific upgrade-loss case below.

For the pre-3.5 double-whole slash behavior, character 218 is the correct legacy glyph. Finale's
conversion writes character 213 instead, which is the filled-notehead slash glyph. Coverage treats
that exact 218-to-213 transformation as `finale-upgrade-loss` only when the imported source retains
218 from `Finale27Default`; recovered source values and every other transformation remain strict.

The fresh tracked-evidence capture has 184 source occurrences and companions, representing 182
distinct `corpus_id` values; every source and companion completes. Of 11,960
`MusicSymbolOptions` leaves, 11,476 compare equal, 484 are expected differences, and none
remain unexpected. All differences belong to the 64 Coda-banner occurrences (62 distinct
`corpus_id` values); the uncompressed, DCL, and zlib rows have none for this class. The expected
population comprises 128 straight-flag leaves and 128 16th-flag leaves for behaviors that do not
exist in Coda, 80
one- and two-bar-repeat leaves for controls Coda does not expose, 64 `fifteenMbDown` leaves with no
Coda source setting, 39 128th-rest leaves whose visible control does not persist, and 21
`eightVbDown` leaves where the pinned 195 differs from a companion's 215. Those 460 differences
are `different_defaults`; another 24 exact double-whole-slash 218-to-213 transformations are
`finale-upgrade-loss`. Default-difference rules ordinarily require `Finale27Default` origin,
leaving recovered source disagreements visible; the versionless Coda `halfSlash` and
`wholeSlash` font-layout exception is deliberately source-recovered. The
controlled Finale 2006 baseline and plus edit, and the revised Finale 2008 parts-symbol edit,
compare equal across all 65 fields. The controlled Finale 1.0.0 key-font fixture's shared double-accidental bytes
decode through Monaco to the companion's `U+222B` and `U+2039`.

Werner Eickhoff's *Susato: Variations on a Masterpiece* (MusicFontLab, 1996), page 4,
identifies Finale 3.5 as the availability boundary for the expanded flag adjustments and
Finale 3.5.1 as the introduction of separate 16th-note flags
([source](https://pdfcoffee.com/susato-font-pdf-free.html), accessed 2026-08-31; PDF SHA-256
`4b21bda90a33c9fd03c8784e362c35b58c8178283c667cddd4f71aeee434e83a`). This is
**strong** documentation evidence for the importer availability gates and the already-reviewed
default-difference rules.
When the source still has `Finale27Default` origin, `eightVbDown`, one- and two-bar repeats, and
the two straight flags are `different_defaults` before Finale 3.5; `fifteenMaUp`,
`fifteenMbDown`, `trillChar`, `wiggleChar`, and the two 16th-note flags use the narrower
pre-3.5.1 boundary. The independently established `rest128th` rule remains Coda-only. A
recovered source value remains strict in every case.

A controlled Finale 1.0.0 edit of all four flag-character controls changes selector `7` word
5, selector `8` word 0, and selector `10` words 1 and 2. They confirm `flagUp`, `flagDown`,
`flag2Up`, and `flag2Down`, respectively; the ETF repeats the same four changes and Finale 27
preserves their values. The complete Coda UI has no independent `flag16Up` or `flag16Down`
control, so those fields retain the pinned `Finale27Default` and companion disagreements are
classified as `different_defaults`.
