# Documentation index

**"I am working on X. What do I read?"** Follow one entry. Do not read directories recursively.

## The naming rule comes first

`src/import/<pool>/<name>.cpp` → reference notes in `research/format/<pool>/<name>.md`, experiment
history in `research/investigations/<name>.md`. Working on `tie_options.cpp` means opening
`format/options/tie_options.md`. You do not need this index for that.

Every detailed file opens with **Covers / Read when / Confidence**. Read those three lines before
reading the file.

## Container and framing — `format/container/`

| Topic | Read | Contains | Read when |
|---|---|---|---|
| File header | [`header.md`](format/container/header.md) | Banner area, record-body offset field, File Info text region | Classifying a file, recovering versions or dates, mapping document metadata |
| Byte order | [`byte_order.md`](format/container/byte_order.md) | Per-epoch trialling, the Coda `PC` platform token, the missing header marker | Changing classification, or a file classifies as an unknown variant |
| Format eras | [`eras.md`](format/container/eras.md) | The four epochs, platform coverage risk, checksum summary | Deciding which epoch a problem belongs to |
| Coda banner | [`coda_banner.md`](format/container/coda_banner.md) | Chained pools, pool 0 rows, Enigma string region | Any Finale 1.x–2.6 container work |
| Uncompressed | [`uncompressed_pools.md`](format/container/uncompressed_pools.md) | The four typed pools | Any Finale 3.0–2000 work |
| DCL | [`dcl_blocks.md`](format/container/dcl_blocks.md) | DCL framing, CRC-32, the disproved 16-word reading | Any Finale 2001–2006 work |
| Zlib | [`zlib_blocks.md`](format/container/zlib_blocks.md) | Typed blocks, which are compressed, the 2007+ record frame | Any Finale 2007–2012 work |
| Record rows | [`record_rows.md`](format/container/record_rows.md) | 16-byte rows, incidences, field addressing, word-order question | Adding a field mapping |
| Entries | [`entries.md`](format/container/entries.md) | Entry row widths and pool framing | Entry decoding, note/rest conversion |
| Sharing | [`sharing.md`](format/container/sharing.md) | Part ownership, part scope, the `shared` attribute | Linked parts or part-scoped records |
| Text encoding | [`text_encoding.md`](format/container/text_encoding.md) | Per-font charset, platform fallback, byte-not-codepoint rule | Any text, symbol, or character conversion |

## Classes — `format/`

| Pool | Directory | Start at |
|---|---|---|
| options | [`format/options/`](format/options/index.md) | [`options/index.md`](format/options/index.md) for addressing, then the class file |
| others | [`format/others/`](format/others/) | `chord_suffix_elements`, `chord_suffix_playback`, `font_definitions`, `shape_definitions`, `fret_records`, `graphic_assignments`, `smart_shape_custom_lines`, `staff_list_category`, `text_blocks`, `layer_attributes`, `part_definitions`, `part_globals` |
| details | [`format/details/`](format/details/) | `fretboard_diagrams`, `measure_graphic_assign` |
| texts | [`format/texts/`](format/texts/) | `text_pool` first, then `coda_texts`, `file_info_text`, `bookmark_text`, `expression_text` |

Class-by-class implementation status: [`state/MUSXDOM_CLASS_COVERAGE.md`](state/MUSXDOM_CLASS_COVERAGE.md).

## Current work — `state/`

| Read | Contains | Read when |
|---|---|---|
| [`STATE.md`](STATE.md) | Status, priorities, open questions, gaps | Always, at session start |
| [`state/PRODUCTION_READINESS.md`](state/PRODUCTION_READINESS.md) | P0–P3 blockers and gaps with status and dates | Choosing what to work on, or judging whether something is already decided |
| [`state/MUSXDOM_CLASS_COVERAGE.md`](state/MUSXDOM_CLASS_COVERAGE.md) | One line per registered class | Starting or finishing a class |
| [`state/EVIDENCE_REQUESTS.md`](state/EVIDENCE_REQUESTS.md) | Precise ETF and controlled-difference requests | You need a fixture that does not exist, or are about to ask for one |
| [`state/OPTIONS_EXTERNAL_CMPER_TODO.md`](state/OPTIONS_EXTERNAL_CMPER_TODO.md) | Options-pool external-cmper work | Options referencing external cmpers |

## Reference — `reference/`

| Read | Contains | Read when |
|---|---|---|
| [`reference/VERSION_MATRIX.md`](reference/VERSION_MATRIX.md) | Version packing, product-to-major mapping, renamed releases, back-saved files | Interpreting or gating on a version |
| [`reference/LEGACY_OPTION_MAPPINGS.md`](reference/LEGACY_OPTION_MAPPINGS.md) | The distilled 437-row option map, provenance, validation plan | Locating an option field with no dedicated notes |
| [`reference/decoder_rules.md`](reference/decoder_rules.md) | Structural markers vs. version gates, epoch gating, bounds safety | Writing or changing a gate |
| [`reference/options_fallback.md`](reference/options_fallback.md) | The baseline-seeding sequence and `ValueOrigin` | Adding an overlay or reporting value origin |
| [`reference/embedded_defaults.md`](reference/embedded_defaults.md) | Pinned baseline resources and their hashes | Touching `src/defaults/` or the generator |
| [`reference/build_invariants.md`](reference/build_invariants.md) | Unity build, `/bigobj`, `/utf-8`, dependency opt-out | Changing CMake or adding a dependency |
| [`reference/code_conventions.md`](reference/code_conventions.md) | Naming, file headers, namespaces, MSVC flags, unity cleanliness | Writing any project-owned C++ file |
| [`reference/duplication_scope.md`](reference/duplication_scope.md) | Where the no-duplication rule binds, and the coverage probe/report pair | A repetition in `tools/`, `scripts/`, or tests |
| [`reference/CITING_EVIDENCE.md`](reference/CITING_EVIDENCE.md) | Evidence tokens, what each confidence level requires, contrary findings | Writing any finding |
| [`reference/pdk_public_evidence.md`](reference/pdk_public_evidence.md) | The public PDK material and what it establishes | Citing a PDK-derived fact |
| [`reference/REPRODUCING_THE_SURVEY.md`](reference/REPRODUCING_THE_SURVEY.md) | Corpus conventions and reproducible commands | Running or re-running a survey |
| [`reference/SURVEY_PROMPT.md`](reference/SURVEY_PROMPT.md) | The public prompt for surveying an outside corpus | A user wants to survey their own files |

## Evidence and history

| Read | Contains | Read when |
|---|---|---|
| [`investigations/index.md`](investigations/index.md) | Which file holds which experiments | A field is `open`/`weak`, or before proposing a hypothesis |
| [`history/FAILED_HYPOTHESES.md`](history/FAILED_HYPOTHESES.md) | Disproved interpretations, kept deliberately | Before proposing anything about framing, row width, byte order, or the DCL payload |
| [`history/EXPERIMENT_LOG_INDEX.md`](history/EXPERIMENT_LOG_INDEX.md) | Every entry in date order with its new home | Date-ordered archaeology |
| [`history/FEASIBILITY_ASSESSMENT.md`](history/FEASIBILITY_ASSESSMENT.md) | The original recommendation and risk assessment | Historical context for why the project is shaped as it is |
| [`corpora/<survey_id>/`](corpora/) | Per-corpus inventories, record catalogs, archive surveys | You need to know what evidence exists — these are large; open one file, not the directory |
| [`data/surveys.csv`](data/surveys.csv) | The survey registry, one row per corpus | Choosing a corpus; surveys are not interchangeable |
| [`README.md`](README.md) | Public study identity, corpora summary, provenance policy, citations | Writing something a public reader will see |

## Adding to this documentation

Use the `maintain-documentation` skill. A new file that this index does not name is invisible.
