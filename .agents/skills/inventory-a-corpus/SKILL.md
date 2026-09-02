---
name: inventory-a-corpus
description: Discover or rediscover what a local legacy Finale corpus contains and regenerate its namespaced private inventories, mappings, caches, and public aggregate deliverables. Use when adding a corpus, changing corpus conventions, finding new source files or companions, refreshing archive contents, or rebuilding a survey's private/generated directory. For tracked-evidence fixture additions, batch them and invoke this skill only as the final prerequisite to a tracked probe/report cycle. Do not use merely to rerun or analyze recovery_coverage_probe.
---

# Inventory a corpus

Discover the contents and filing conventions of one registered corpus, then regenerate the
artifacts derived from that discovery. This skill establishes **what evidence exists and where it
is locally**. It does not test the current reader across that evidence; use
`analyze-recovery-coverage` for that.

`research/REPRODUCING_THE_SURVEY.md` is authoritative for the inventory pipeline. Read it before
running or changing the pipeline. If it and this skill disagree, follow it and fix this skill.

## Boundaries

This skill owns:

- `private/corpora/<survey_id>.conf`, including root, companion conventions, archive policy,
  content sniffing, and exclusions;
- `private/generated/<survey_id>/`, including inventory, path mappings, structure results, and
  per-survey archive caches;
- the generated `private/generated/corpus-<survey_id>.tsv` consumed by recovery coverage;
- `research/corpora/<survey_id>/` aggregate deliverables and the survey registry.

It does **not** own `private/generated/recovery_coverage.jsonl`, the coverage report, a class
hypothesis, or repeated analysis of a probe snapshot.

## Hard rules

- Treat the corpus as read-only. Never extract an archive over it, rename its files, or write a
  companion into it unless the user separately performs or authorizes that action.
- Keep paths, filenames, archive names, member names, and document metadata private. Tracked
  output uses `survey_id`, content-derived `corpus_id`, hashes, and aggregates.
- Inventory by content when extensions are unreliable. `--sniff-content` must recognize the
  exact Finale header families rather than a loose `Finale ` prefix: `ENIGMA BINARY FILE`,
  `Finale(R)`, `Finale(TM)`, and `Finale` followed by MacRoman byte `0xAA`.
- Ask for corpus conventions when creating a registration. When refreshing an existing survey,
  read its `.conf`; ask only about missing or apparently stale conventions.
- Keep generators in `scripts/` location-agnostic. Corpus-specific paths belong only in private
  configuration and invocation.
- Print complete-import failures with their private names and paths only to the local console.
  Never copy those diagnostics into tracked output.
- Do not regenerate `tracked-evidence` after each new fixture. Batch fixture additions and
  regenerate only if at least one fixture is new, as the final prerequisite immediately before
  the tracked probe/report cycle that will consume them. Reuse the current inventory when no
  fixture changed, and do not regenerate if no tracked cycle will run.

## Workflow

1. Identify the `survey_id` and whether this is initial discovery or regeneration.
2. For a new corpus, establish: root, companion directory/suffix conventions, archive inclusion,
   content sniffing, exclusions/keeps, DCL decoder path, and publishing intent. Register the survey
   as described in `research/REPRODUCING_THE_SURVEY.md`.
3. For an existing corpus, preflight its `private/corpora/<survey_id>.conf`, mounted root,
   `unar`/`lsar` when archives are enabled, available disk, and current worktree changes.
4. Run `private/regenerate.sh <survey_id>` when the local private driver exists. Its per-stage
   commands are documented in `research/REPRODUCING_THE_SURVEY.md`; use those commands directly
   when reproducing on another machine.
5. Verify the selection funnel: inventory occurrences, distinct `corpus_id` values, origin counts,
   recognized epochs/products, companion match grades, exclusions, and failures. A sudden zero in
   companions usually means a stale convention, not a finding.
6. Run the path and filename leak checks in `research/REPRODUCING_THE_SURVEY.md` before proposing
   tracked output. Fix generators rather than hand-editing generated files.
7. Inspect `git status --short` and `git diff --check`, preserving unrelated changes. Report which
   private artifacts were regenerated and which tracked aggregates changed.

Do not run `recovery_coverage_probe` as part of this workflow. Finishing inventory may make its
existing JSONL stale; say so explicitly and hand the next task to `analyze-recovery-coverage`.
