# Options pool

**Covers:** How legacy options are addressed, the `^NN(65534)` selector form, and the direct multi-incidence option blocks.
**Read when:** Starting work on any options class, before opening that class's own file.
**Confidence:** substantially mapped; mostly source-derived.

## Options

**Substantially mapped for the pre-26.2 compatibility representation, but mostly source-derived.** Options are at the
beginning of the 2007+ `0x001a` block, identified by primary key `0xfffe`; many are fixed 12-byte records, while some
have large variable payloads. ETF represents the corresponding older globals as `^NN(65534)` records.

An independently maintained legacy-format reference yielded a 437-row union across
24 logical preference groups: 435 current mappings plus two original-branch locations needed for older Finale
behavior. The table identifies tags, comparators, incidents, word slots, widths, conversion rules,
semantic fields, and some version gates. Available ETFs independently contain the selectors used by 386 rows, but
field meanings and offsets remain reference-derived until controlled binary comparisons verify them. See
[`LEGACY_OPTION_MAPPINGS.md`](../../reference/LEGACY_OPTION_MAPPINGS.md) and
[`data/legacy_option_mappings.csv`](../../data/legacy_option_mappings.csv).

Five additional structures use direct multi-incidence blocks rather than the field map: slur contours at
`^52(65534)`, tie placement at `^85(65534)`, tie contours at `^86(65534)`, grids/guides at `^88(65534)`, and stem
connections at `^40(65534)`. All selectors are ETF-observed; see
[`data/legacy_direct_option_blocks.csv`](../../data/legacy_direct_option_blocks.csv).

## Per-class files

One file per musxdom options class, named after its source file in `src/import/options/`:

`accidental_options` · `alternate_notation_options` · `barline_options` · `beam_options` ·
`chord_options` · `clef_options` · `flag_options` · `font_options` · `grace_note_options` ·
`key_signature_options` · `line_curve_options` · `lyric_options` · `misc_options` ·
`multimeasure_rest_options` · `music_symbol_options` · `note_rest_options` ·
`page_format_options` · `piano_brace_bracket_options` · `repeat_options` ·
`smart_shape_options` · `staff_options` · `stem_options` · `text_options` · `tie_options` ·
`time_signature_options` · `tuplet_options`

`AugmentationDotOptions` and `MusicSpacingOptions` are implemented but have no
dedicated notes yet; their locations are in
[`../../reference/LEGACY_OPTION_MAPPINGS.md`](../../reference/LEGACY_OPTION_MAPPINGS.md).

Experiment history for a class is in `research/investigations/<same name>.md`.
