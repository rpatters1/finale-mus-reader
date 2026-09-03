# Format Notes (moved)

**This file no longer holds content.** It was split into one file per subject and per musxdom class under [`research/format/`](format/), because agents were loading all 5,333 lines of it to answer one question.

Do not add anything here. New material goes in the file that owns the subject; see
[`INDEX.md`](INDEX.md).

Class notes are named after their source file; the rule is in
[`ORIENTATION.md`](ORIENTATION.md#navigation-contract).

## Where each former section went

| Former section | Now in |
|---|---|
| `#determining-byte-order`, `#the-word-at-0x80-and-the-absent-application-version` | [`format/container/byte_order.md`](format/container/byte_order.md) |
| `#chained-pools`, `#coda-banner-files`, `#enigma-string-region`, `#header-text-records`; +1 more | [`format/container/coda_banner.md`](format/container/coda_banner.md) |
| `#finale-20012006-physical-records-and-the-16-word-hypothesis`, `#finale-20012006-typed-dcl-blocks` | [`format/container/dcl_blocks.md`](format/container/dcl_blocks.md) |
| `#entry-pool` | [`format/container/entries.md`](format/container/entries.md) |
| `#checksums-compression-and-wrapping`, `#platform-coverage-risk`, `#proposed-format-eras` | [`format/container/eras.md`](format/container/eras.md) |
| `#banner-era-files`, `#document-metadata-in-the-header-text-region` | [`format/container/header.md`](format/container/header.md) |
| `#record-fields`, `#record-pools-and-row-shapes-through-finale-2006`, `#word-order-in-32-bit-fields-open-hypothesis-not-tested` | [`format/container/record_rows.md`](format/container/record_rows.md) |
| `#finale-2002-controlled-pair`, `#sharing-and-linked-parts`, `#zlib-part-ownership-and-sharing` | [`format/container/sharing.md`](format/container/sharing.md) |
| `#encoding` | [`format/container/text_encoding.md`](format/container/text_encoding.md) |
| `#finale-3x2000-uncompressed-typed-pools` | [`format/container/uncompressed_pools.md`](format/container/uncompressed_pools.md) |
| `#2007-typed-blocks`, `#finale-2007-generic-record-frame`, `#the-2007-2012-record-encoding`, `#which-blocks-are-compressed` | [`format/container/zlib_blocks.md`](format/container/zlib_blocks.md) |
| `#accidental-options` | [`format/options/accidental_options.md`](format/options/accidental_options.md) |
| `#alternate-notation-options` | [`format/options/alternate_notation_options.md`](format/options/alternate_notation_options.md) |
| `#barline-options` | [`format/options/barline_options.md`](format/options/barline_options.md) |
| `#beam-options` | [`format/options/beam_options.md`](format/options/beam_options.md) |
| `#clef-definitions`, `#corpus-verification` | [`format/options/clef_options.md`](format/options/clef_options.md) |
| `#flag-options` | [`format/options/flag_options.md`](format/options/flag_options.md) |
| `#finale-100-fonts`, `#later-pre-zlib-default-font-array`, `#the-1328-boundary-is-finale-2012-not-finale-2003`, `#the-single-name-preference-reaches-all-four-modern-name-types`; +1 more | [`format/options/font_options.md`](format/options/font_options.md) |
| `#grace-note-options` | [`format/options/grace_note_options.md`](format/options/grace_note_options.md) |
| `#options` | [`format/options/index.md`](format/options/index.md) |
| `#key-signature-options` | [`format/options/key_signature_options.md`](format/options/key_signature_options.md) |
| `#line-and-curve-options` | [`format/options/line_curve_options.md`](format/options/line_curve_options.md) |
| `#automatic-lyric-numbering-and-a-record-that-states-its-own-layout`, `#lift-and-push-the-only-lyric-values-the-earliest-era-stores`, `#lyric-options`, `#syllable-edge-punctuation-and-a-confound-that-nearly-named-nine-wrong-answers`; +5 more | [`format/options/lyric_options.md`](format/options/lyric_options.md) |
| `#miscellaneous-options` | [`format/options/misc_options.md`](format/options/misc_options.md) |
| `#multimeasure-rest-defaults` | [`format/options/multimeasure_rest_options.md`](format/options/multimeasure_rest_options.md) |
| `#music-symbol-options` | [`format/options/music_symbol_options.md`](format/options/music_symbol_options.md) |
| `#noterest-options` | [`format/options/note_rest_options.md`](format/options/note_rest_options.md) |
| `#page-format-options` | [`format/options/page_format_options.md`](format/options/page_format_options.md) |
| `#piano-brace-and-bracket-options` | [`format/options/piano_brace_bracket_options.md`](format/options/piano_brace_bracket_options.md) |
| `#repeat-options-the-document-staff-list-reference` | [`format/options/repeat_options.md`](format/options/repeat_options.md) |
| `#smart-shape-options` | [`format/options/smart_shape_options.md`](format/options/smart_shape_options.md) |
| `#staff-options` | [`format/options/staff_options.md`](format/options/staff_options.md) |
| `#stem-connections`, `#the-eight-stem-scalars-and-the-marker-that-dates-them`, `#the-finale-2012-payload-and-its-stale-predecessor`, `#the-reverse-stemming-flag-moved-and-the-word-says-which-spelling-it-is-in` | [`format/options/stem_options.md`](format/options/stem_options.md) |
| `#accidental-symbol-inserts`, `#text-options` | [`format/options/text_options.md`](format/options/text_options.md) |
| `#tie-options` | [`format/options/tie_options.md`](format/options/tie_options.md) |
| `#time-signature-options` | [`format/options/time_signature_options.md`](format/options/time_signature_options.md) |
| `#tuplet-options` | [`format/options/tuplet_options.md`](format/options/tuplet_options.md) |
| `#a-shape-naming-a-font-the-source-never-defines-third-deliberate-disagreement`, `#font-definitions`, `#reading-companion-face-missing`, `#unresolvable-comparators-and-the-missing-font-n-placeholder` | [`format/others/font_definitions.md`](format/others/font_definitions.md) |
| `#fret-instruments-groups-styles-and-diagrams` | [`format/others/fret_records.md`](format/others/fret_records.md) |
| `#earliest-controlled-graphic-placement`, `#embedded-graphics`, `#page-graphic-assignments` | [`format/others/graphic_assignments.md`](format/others/graphic_assignments.md) |
| `#shape-definitions-instructions-and-data` | [`format/others/shape_definitions.md`](format/others/shape_definitions.md) |
| `#smartshapecustomline-ls-is-absent-from-the-public-pdk` | [`format/others/smart_shape_custom_lines.md`](format/others/smart_shape_custom_lines.md) |
| `#textblock-attributes` | [`format/others/text_blocks.md`](format/others/text_blocks.md) |
| `#bookmarks` | [`format/texts/bookmark_text.md`](format/texts/bookmark_text.md) |
| `#the-coda-banner-epoch` | [`format/texts/coda_texts.md`](format/texts/coda_texts.md) |
| `#expression-text-before-it-moved-into-the-pool` | [`format/texts/expression_text.md`](format/texts/expression_text.md) |
| `#file-info` | [`format/texts/file_info_text.md`](format/texts/file_info_text.md) |
| `#enigma-commands-in-the-compressed-epochs`, `#initial-formatting-state`, `#section-markers-before-finale-97`, `#the-parenthetical-value-after-a-font-name`; +1 more | [`format/texts/text_pool.md`](format/texts/text_pool.md) |
| `#failed-or-revised-hypotheses` | [`history/FAILED_HYPOTHESES.md`](history/FAILED_HYPOTHESES.md) |
| `#archive-derived-early-version-evidence`, `#correlation-between-finale-18726-and-finale-30`, `#etf-evidence-set` | [`investigations/early_versions.md`](investigations/early_versions.md) |
| `#open-questions-the-regression-comparison-surfaced` | [`investigations/regression_open_questions.md`](investigations/regression_open_questions.md) |
| `#public-finale-2000-pdk-evidence` | [`reference/pdk_public_evidence.md`](reference/pdk_public_evidence.md) |

The verbatim pre-split file is kept at [`FORMAT_NOTES.md`](FORMAT_NOTES.md).
