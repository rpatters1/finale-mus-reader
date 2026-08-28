---
name: analyze-recovery-coverage
description: Run recovery_coverage_probe over already-inventoried Finale corpora and analyze its compact recovery_coverage.jsonl classifications against modern companions. Use for reader regression, class or field coverage, upgrade transformations, and unexpected differences. Do not rediscover or rebuild a corpus inventory.
---

# Analyze recovery coverage

Use the registry-driven recovery probe to compare what the current reader constructs for legacy
sources with independently parsed companions, then aggregate its compact classifications.
Corpus discovery is a separate operation owned by `inventory-a-corpus`.

## Two distinct loops

### Capture loop — expensive

Rerun `recovery_coverage_probe` only when its observations may have changed, such as after:

- source or companion membership/path mappings changed;
- importer, surveyor, musxdom, comparison, or classification behavior changed;
- the selected corpus/cohort changed.

Require current `private/generated/<survey_id>/corpus_locations.csv`, the public manifest, and the
generated `private/generated/corpus-<survey_id>.tsv` for every selected survey. If these are absent
or stale, stop and use `inventory-a-corpus`; do not rescan the corpus here.

`private/generated/<survey_id>/` is persistent survey output. It belongs only to a complete corpus
inventory and remains in place until that survey is regenerated. Never put recovery-coverage probe
captures, rendered reports, or other development-cycle analysis artifacts there.

Recovery-coverage output is transient. Keep one flat artifact family per requested report target
under `private/reports/`. `tracked-evidence.*` and `all-corpus.*` are the standing targets; a user may
explicitly request another separately named target. Each family may contain the target's current
JSONL, rendered report, probe stdout/stderr, and report stderr. A later run of the same target
replaces that family's files in place. Do not create report subdirectories or encode a hypothesis,
date, sequence number, or result label in a filename. Put unrequested focused cohorts, record dumps,
disposable manifests, and every other intermediate artifact under `/tmp`, then remove them after
answering the immediate question. These files are working evidence, not survey state or a historical
archive.

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

Do not edit generated per-corpus TSVs to select a cohort. Put disposable manifests under `/tmp`.
Always build and run the coverage probe from the instrumented Release
tree, normally `build-release`; the Debug probe is never appropriate for coverage captures. Coverage
and timing use the same instrumentation. Inspect `--help`, and write every capture stream to
`private/reports/`. A normal one-corpus capture is:

```bash
mkdir -p private/reports
cmake --build build-release --target recovery_coverage_probe
build-release/tools/coverage/recovery_coverage_probe --progress \
  --mac-symbol-fonts="${HOME}/Library/Application Support/MakeMusic/Finale 27/Configuration Files/MacSymbolFonts.txt" \
  private/generated/corpus-tracked-evidence.tsv \
  private/reports/tracked-evidence.recovery_coverage.jsonl \
  > private/reports/tracked-evidence.probe.stdout.txt \
  2> private/reports/tracked-evidence.probe.stderr.txt
```

**Every probe capture must supply Finale's `MacSymbolFonts.txt` with
`--mac-symbol-fonts`.** On macOS, Finale 27 installs it at
`~/Library/Application Support/MakeMusic/Finale 27/Configuration Files/MacSymbolFonts.txt`.
Confirm the probe startup summary names the supplied `MacSymbolFonts` path; if the file is unavailable,
stop and report the missing prerequisite rather than producing a coverage snapshot. Without the
list, legacy symbol-font bytes can be decoded through the wrong text encoding and create spurious
source/companion differences. Record the supplied file's path privately with the capture metadata,
never in public aggregate results.

There is no canonical or archival recovery-coverage snapshot. A subsequent probe of the same target
may overwrite its JSONL and stream captures. Every failed occurrence must still be available in the
captured stderr with its private source path and error. Validate row count, JSON parsing, source
status counts, companion count, and the selected-corpus funnel before analysis.

For a timing study, use the same instrumented Release probe and pass `--include-timings`; normal
schema-3 output omits timing structures. Compare timings only between equivalent Release
configurations, and record the build configuration with the result. Capture stderr separately when
diagnostic messages are needed because JSON rows retain only their counts.

### Rendering loop — repeat freely

Do not rerun the probe merely because aggregation or presentation changed. Treat the schema-3
JSONL as an immutable set of classifications for the rendering pass and rerun
`scripts/recovery_coverage_report.py` freely. Matching rules, semantic comparison, and expected-
difference classification live in `tools/coverage/`; changing any of them requires a new capture.
Capture report output under `private/reports/` as well, for example:

```bash
python3 scripts/recovery_coverage_report.py \
  private/reports/tracked-evidence.recovery_coverage.jsonl \
  > private/reports/tracked-evidence.report.txt \
  2> private/reports/tracked-evidence.report.stderr.txt
```

Always state which snapshot was analyzed, its row/status/companion counts, selected surveys, and
whether counts are occurrences or distinct `corpus_id` values. Distinct content is the primary
statistical unit; occurrences describe corpus reach and duplication.

## Comparison rules

- Extract source and companion observations independently. A Finale 27 save is upgrade behavior,
  not proof of the source representation.
- Treat every newly observed unexpected difference as evidence to review, not as a classification
  backlog to clear. Before adding or broadening an expected-difference rule, show the user the
  unexpected paths, source and companion values, origins, and distinct-document counts (using
  public `corpus_id` tokens rather than private paths), and wait for the user to review them. For a
  large repeated population, show every distinct path/value transformation and representative
  corpus IDs rather than hiding variation in an aggregate. Do not classify a difference in the
  same step that discovers it unless the user has already reviewed that exact transformation and
  explicitly directed its classification.
- Before treating a source/companion disagreement as Finale upgrade behavior, inspect the raw
  companion representation when available. Confirm whether the differing value is actually stored
  in EnigmaXML or was introduced by musxdom, the XML backend, a surveyor, or comparison logic. A
  dependency or observation bug must remain unexpected until fixed and recaptured; never make an
  expected-difference rule that merely conceals it.
- Classify the format behavior, not the finite table of values the current corpus happened to
  exercise. Prefer a broad rule stated in terms of the relevant class, source structure, era, and
  difference category. Do not enumerate observed source/companion value pairs or construct a gate
  from unrelated records merely because either approach fits every current row. Use a value
  predicate only when the value itself defines the behavior being classified.
- Match cmper-keyed objects by same-side semantic content or referents, never by assuming cmpers or
  ordering survive a save.
- Compare every font reference by its resolved, normalized font face on each side, never by its
  numeric font ID. Font-definition cmpers are document-local references and Finale may renumber
  them when saving a companion. Keep the other members of a font tuple, such as size and effects,
  as independent comparisons. If either ID does not resolve, or the resolved faces differ, keep
  the disagreement visible for review; semantic comparison alone does not authorize classifying a
  font substitution as expected.
- Separate physical recovery, semantic equivalence, companion transformation, and reader coverage.
- Classify transformations as `preserved`, `remapped`, `normalized`, `substituted`, `synthesized`,
  `removed`, or `unresolved`. Record `reader-gap` independently.
- Do not begin with an assumed one-to-one match or record count. Preserve contradictions and narrow
  confidence instead of forcing a match.
- When testing a transformation, test its converse and search every represented version/epoch.
  Report unmatched populations separately; they may hide additional transformations but are not
  counterexamples.
- Expected-difference rules are executable interpretation registered by the class descriptor in
  that class's `tools/coverage/surveyors/` translation unit. After the user has reviewed and
  approved a characterized transformation, scope its rule by path, category, origin, and the
  narrowest evidence-backed structural predicate so it cannot conceal a recovered-value
  disagreement. Generic comparison mechanics remain in `tools/coverage/comparison.cpp`; Python
  must not reclassify a probe result.
- Surveyors return structured `coverage::Value` observations. Use the C++20 field descriptors in
  `tools/coverage/schema.h` for class leaves; do not serialize or parse an intermediate JSON value.

## Privacy and publication

Keep transient per-fixture observations, selection files, paths, filenames, and captured streams
under `private/reports/` or `/tmp`, never under `private/generated/<survey_id>/`. Local console
diagnostics may name private fixtures. User-facing or tracked findings use aggregates and, when
reproducibility needs them, a small number of `corpus_id` tokens.

Use the repository confidence vocabulary: `confirmed`, `strong`, `weak`, and `open`. Before
changing shared findings, read `research/CITING_EVIDENCE.md`. If tracked deliverables are requested,
publish only sanitized aggregates and concise research notes, then apply the leak checks documented
by `inventory-a-corpus`.

## Milestone accounting

At every coverage milestone, state what remains as well as what passed. Milestones include a
successful capture, a clean tracked-evidence report, a clean all-corpus report, validation of a
reader or classification change, and the final handoff for a class-focused investigation.

Keep these four questions separate:

1. Are any observed differences still unexpected?
2. Which target fields, collections, record families, or source epochs are still unsupported or
   intentionally left at the baseline?
3. Which claims still lack controlled evidence or corpus representation?
4. Which selected documents failed before comparison?

For class-focused work, determine the second answer independently of the report: compare the
target's registered musxdom members with the importer and
`research/MUSXDOM_CLASS_COVERAGE.md`, then consult the target section of
`research/FORMAT_NOTES.md` for known open eras or fields. Enumerate the remaining members and
epochs concisely; if none are known, say so explicitly. Update the checklist when that accounting
changes.

Never use “clean,” “zero unexpected differences,” or successful companion coverage as a synonym
for complete source recovery. Baseline-seeded unsupported members can compare equal to companions
and therefore disappear from the difference report. Describe such a result as clean only for the
observations and classifications exercised by that snapshot. Claim a class is complete only when
its member-by-member accounting also has no remaining recovery scope.

## Validation and handoff

- Manually inspect at least one observation from every represented source era and every
  non-preservation outcome relevant to the claim.
- Where the claim concerns raw format structure, verify a small sample at byte offsets rather than
  trusting only the reader under evaluation.
- Ensure candidate counts equal matched plus explicitly classified unmatched/excluded counts.
- Run syntax checks for changed analysis scripts and focused/full tests when production or surveyor
  code changed.
- Confirm `private/reports/` contains only one current flat artifact family per requested report
  target; remove superseded runs and unrequested investigative artifacts immediately. They are not
  a historical archive.
- Inspect `git status --short` and `git diff --check`; preserve unrelated worktree changes.

Lead the result with the selection funnel, then the finding, contrary/unresolved evidence,
confidence, remaining implementation and evidence scope, and the precise condition that would
require another capture-loop run.
