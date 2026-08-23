---
name: analyze-recovery-coverage
description: Run recovery_coverage_probe over already-inventoried Finale corpora and iteratively analyze its recovery_coverage.jsonl snapshot against modern companions. Use for reader regression, class or field coverage, upgrade transformations, unexpected differences, and repeated hypothesis analysis. Do not rediscover or rebuild a corpus inventory.
---

# Analyze recovery coverage

Use the registry-driven recovery probe to capture what the current reader constructs for legacy
sources and independently parsed companions, then analyze that snapshot as many times as needed.
Corpus discovery is a separate operation owned by `inventory-a-corpus`.

## Two distinct loops

### Capture loop — expensive

Rerun `recovery_coverage_probe` only when its observations may have changed, such as after:

- source or companion membership/path mappings changed;
- importer, surveyor, musxdom, or probe behavior changed;
- the selected corpus/cohort changed.

Require current `private/generated/<survey_id>/corpus_locations.csv`, the public manifest, and the
generated `private/generated/corpus-<survey_id>.tsv` for every selected survey. If these are absent
or stale, stop and use `inventory-a-corpus`; do not rescan the corpus here.

Treat `private/active_corpora.txt` as user-managed state: do not use or modify it unless the user
explicitly asks for that exact manifest. The probe CLI already accepts the full range of useful
inputs, so choose the narrowest one that answers the question:

- a generated `private/generated/corpus-<survey_id>.tsv` for one whole corpus;
- an analysis-local manifest whose lines name several generated corpus TSVs;
- an analysis-local TSV containing a selected cohort of `corpus_id<TAB>source_path` rows, with the
  applicable `#root:` and repeatable `#companion:` declarations copied from its generated corpus
  TSV; or
- a single source path when the CLI recognizes it directly. For a single source whose extension
  would be interpreted as a manifest, use a one-row analysis-local TSV.

Do not edit generated per-corpus TSVs to select a cohort. Put disposable manifests under `/tmp` or
an analysis directory under `private/generated/`. Build the existing coverage target, inspect
`--help`, and write the probe to an explicit ignored path. A normal one-corpus capture is:

```bash
cmake --build build --target recovery_coverage_probe
build/tools/coverage/recovery_coverage_probe --progress \
  private/generated/corpus-rpatters1-installs.tsv \
  private/generated/recovery_coverage.next.jsonl
```

Validate the completed `.next.jsonl` before replacing the prior snapshot; an interrupted probe must
not destroy the last analyzable capture. The canonical full-regression snapshot is:

```text
private/generated/recovery_coverage.jsonl
```

Capture stderr privately. Every failed occurrence must still be printed locally with its private
source path and error. Validate row count, JSON parsing, source status counts, companion count, and
the selected-corpus funnel before analysis.

For any timing study, run a separately configured instrumented Release probe (normally
`build-release`, with `CMAKE_BUILD_TYPE=Release` and
`FINALE_MUS_READER_INSTRUMENTATION=ON`). Treat a Debug
probe, especially one launched under LLDB or another debugger, as diagnostic only: unoptimized
standard-library, map, shared-pointer, and instrumentation overhead can swamp or distort the
production-code differences being measured. Compare timings only between equivalent Release
configurations, and record the build type and whether instrumentation was enabled with the result.

### Analysis loop — repeat freely

Do not rerun the probe merely because a hypothesis, aggregation, matching rule, or report changed.
Treat the JSONL as an immutable snapshot for the analysis pass. Rerun
`scripts/recovery_coverage_report.py` or use a disposable script under `/tmp` until the question is
settled. Preserve a reusable, location-agnostic analyzer under `scripts/` only when the method will
be repeated.

Always state which snapshot was analyzed, its row/status/companion counts, selected surveys, and
whether counts are occurrences or distinct `corpus_id` values. Distinct content is the primary
statistical unit; occurrences describe corpus reach and duplication.

## Comparison rules

- Extract source and companion observations independently. A Finale 27 save is upgrade behavior,
  not proof of the source representation.
- Match cmper-keyed objects by same-side semantic content or referents, never by assuming cmpers or
  ordering survive a save.
- Separate physical recovery, semantic equivalence, companion transformation, and reader coverage.
- Classify transformations as `preserved`, `remapped`, `normalized`, `substituted`, `synthesized`,
  `removed`, or `unresolved`. Record `reader-gap` independently.
- Do not begin with an assumed one-to-one match or record count. Preserve contradictions and narrow
  confidence instead of forcing a match.
- When testing a transformation, test its converse and search every represented version/epoch.
  Report unmatched populations separately; they may hide additional transformations but are not
  counterexamples.
- `EXPECTED_DIFFERENCES` in `scripts/recovery_coverage_report.py` is executable interpretation.
  Add a rule only after the difference is characterized; scope it by path, category, and origin so
  it cannot conceal a recovered-value disagreement.

## Privacy and publication

Keep per-fixture observations, selection files, paths, and filenames under
`private/generated/<survey_id>/class_coverage/<analysis_id>/` or `/tmp`. Local console diagnostics
may name private fixtures. User-facing or tracked findings use aggregates and, when reproducibility
needs them, a small number of `corpus_id` tokens.

Use the repository confidence vocabulary: `confirmed`, `strong`, `weak`, and `open`. Before
changing shared findings, read `research/CITING_EVIDENCE.md`. If tracked deliverables are requested,
publish only sanitized aggregates and concise research notes, then apply the leak checks documented
by `inventory-a-corpus`.

## Validation and handoff

- Manually inspect at least one observation from every represented source era and every
  non-preservation outcome relevant to the claim.
- Where the claim concerns raw format structure, verify a small sample at byte offsets rather than
  trusting only the reader under evaluation.
- Ensure candidate counts equal matched plus explicitly classified unmatched/excluded counts.
- Run syntax checks for changed analysis scripts and focused/full tests when production or surveyor
  code changed.
- Inspect `git status --short` and `git diff --check`; preserve unrelated worktree changes.

Lead the result with the selection funnel, then the finding, contrary/unresolved evidence,
confidence, and the precise condition that would require another capture-loop run.
