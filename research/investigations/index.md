# Investigations index

**Covers:** Which investigation file holds the experiments behind a given subject.
**Read when:** A field is labeled `open` or `weak`, a companion comparison disagrees, or you are
about to propose a hypothesis — the experiment may already have been run and refuted.
**Confidence:** navigational only.

Files here hold dated experiment entries moved verbatim from the former `EXPERIMENT_LOG.md`.
Each entry states its question, method, result, and artifacts. **Refuted predictions are kept
deliberately**; they exist so an expensive experiment is not repeated.

Do not read this directory recursively. Open the one file that matches your subject.

| Subject | File |
|---|---|
| Bookmarks | [`bookmark.md`](bookmark.md) for the record and its recovery, [`bookmark_text.md`](bookmark_text.md) for where the text is stored |
| Chord suffix element identities, layouts, and flags | [`chord_suffix_elements.md`](chord_suffix_elements.md) |
| Chord suffix playback identity and zero fill | [`chord_suffix_playback.md`](chord_suffix_playback.md) |
| Container framing, entropy, pool identification, DCL identification | [`container.md`](container.md) |
| Corpus enumeration, archives, fixture acquisition, coverage registration | [`corpus_surveys.md`](corpus_surveys.md) |
| Custom-key clef octave arrays | [`custom_key_octaves.md`](custom_key_octaves.md) |
| Custom-key others records and map ordering | [`custom_keys.md`](custom_keys.md) |
| Finale 1.x–2.6 evidence and the 1.8.7–3.0 correlation | [`early_versions.md`](early_versions.md) |
| Graphic file locator selectors and conversion differences | [`file_path.md`](file_path.md) |
| Fret record identities and layouts | [`fret_records.md`](fret_records.md) |
| Graphic assignment layout | [`graphic_assignments.md`](graphic_assignments.md) |
| Custom-key accidental symbol lists | [`key_symbol_list.md`](key_symbol_list.md) |
| Layer attribute layout, release coverage, pre-2002 playback and spacing | [`layer_attributes.md`](layer_attributes.md) |
| Marking category records, name width, enum mapping, flags, and fallback differences | [`marking_category.md`](marking_category.md) |
| Measure layouts, flag-word readings, and the compact part record | [`measure.md`](measure.md) |
| The private format-reference option-map audit | [`option_mappings.md`](option_mappings.md) |
| Part definition class id, flag word, default-name sign, the absent `pD` tag | [`part_definitions.md`](part_definitions.md) |
| Part globals, pre-zlib options, Coda Scroll View cache, zlib class `0x0120` | [`part_globals.md`](part_globals.md) |
| Percussion-note assignments, `nC`/`NC` search, and implicit upgrade | [`percussion_note_code.md`](percussion_note_code.md) |
| Percussion-note introduction, `DF` upgrade, packed layouts, and notehead encoding | [`percussion_note_info.md`](percussion_note_info.md) |
| Companion-comparison disagreements and the instrument errors behind them | [`regression_open_questions.md`](regression_open_questions.md) |
| Part ownership, structural sharing, the sharing census | [`sharing.md`](sharing.md) |
| Custom smart-shape lines, guitar-bend boundary | [`smart_shape_custom_lines.md`](smart_shape_custom_lines.md) |
| Staff-list identities, text width, forced arrays, and category baseline fill | Category: [`staff_list.md`](staff_list.md); repeat: [`repeat_options.md`](repeat_options.md) |
| TextBlock storage and assembly | [`text_blocks.md`](text_blocks.md) |
| Text expression epochs, synthesis, and companion IDs | [`text_expression_defs.md`](text_expression_defs.md) |
| Text pool, binary command table, Coda-banner pool walk | [`text_pool.md`](text_pool.md) |
| Saving-version reads, printed-manual audit, 3.5/3.7 feature boundaries | [`version_boundaries.md`](version_boundaries.md) |

Per-class investigations use the same filename as the class's reference file under
`research/format/` and as its source file under `src/import/`:

`flag_options` · `font_options` · `grace_note_options` · `line_curve_options` · `lyric_options` ·
`multimeasure_rest_options` · `music_symbol_options` · `note_rest_options` ·
`page_format_options` · `piano_brace_bracket_options` · `repeat_options` ·
`smart_shape_options` · `staff_options` · `stem_options` · `text_options` · `tie_options` ·
`time_signature_options`

A class with no file here has no recorded experiment history yet.

The full chronological ordering of every entry is preserved in
[`../history/EXPERIMENT_LOG_INDEX.md`](../history/EXPERIMENT_LOG_INDEX.md).
