---
name: survey-class-coverage
description: Test recovery coverage for one legacy Finale class or a related set of classes across a selected cohort of already-inventoried fixtures. Use for targeted class, field, record-layout, fallback, semantic-reference, or upgrade-behavior studies over all fixtures, loose fixtures, ETF-backed fixtures, Finale-27-backed fixtures, or explicit combinations of those cohorts. Do not use to inventory a new corpus.
---

# Survey class coverage

Run a hypothesis-driven extraction over legacy fixtures already registered by
`survey-a-corpus`, optionally comparing each fixture with an ETF or modern Finale
re-save. Keep source observations, companion observations, and the interpretation
between them separate. A companion is a semantic reference with its own provenance,
not proof that Finale preserved the source representation.

## Hard rules

- Treat every corpus as read-only.
- Never put a source path, export path, filename, archive member name, or other
  identifying label in tracked output. Identify evidence by `survey_id` and
  content-derived `corpus_id` only.
- Keep per-fixture observations and path-bearing output under
  `private/generated/<survey_id>/class_coverage/<analysis_id>/`.
- Whenever a fixture fails to import at all, print its fixture name, private
  source path, and error to the local console. Print every failed occurrence,
  including archive members. Console-only failure diagnostics are the
  intentional exception to path suppression; never put them in tracked output.
- Publish aggregate results, not a per-file structural dump. Cite a small number
  of relevant `corpus_id` values in findings when reproducibility requires it.
- Extract the legacy fixture and each companion independently before comparing
  them. Do not infer a missing source value from a companion or treat a companion's
  comparator as the source comparator.
- Treat an ETF as Finale-generated text that may normalize its source, and a
  modern re-save as the result of Finale's upgrade policy. Finale may reorder,
  renumber, synthesize, remove, canonicalize, or substitute values according to
  the fonts and other resources installed on the exporting or upgrading system.
- Preserve contradictory observations. Lower confidence or narrow the claim
  instead of forcing the evidence through the expected mapping.
- Do not modify the reader while performing a survey unless the user separately
  asks for implementation.

## Step 1 — Define the question

Record an `analysis_id` in lowercase snake case and state:

1. The musxdom class or related classes under study.
2. The fields and collection identities to compare.
3. The legacy epochs and version ranges in scope.
4. The registered `survey_id` values to include.
5. The fixture cohort: `all`, `loose`, `etf`, `fin27`, or an explicit union or
   intersection of those selectors.
6. Whether the question concerns physical capture, semantic recovery against an
   ETF, upgrade behavior against a modern re-save, or some combination.
7. The equivalence rule for each field. For references, define how referents are
   matched independently of their cmpers.

Do not begin with an assumed record count or a presumed one-to-one ordering.
Those are potential findings.

## Step 2 — Select the fixture cohort

For each survey, require:

- `private/generated/<survey_id>/corpus_inventory.csv` for pair quality and
  local paths;
- `private/generated/<survey_id>/corpus_locations.csv` when a probe accepts the
  canonical private mapping;
- `research/corpora/<survey_id>/data/corpus_manifest.csv` for public identities
  and hashes.

If these are missing or stale, stop and use `survey-a-corpus`; do not rebuild an
inventory inside this skill.

Build a private `selection.csv` for the analysis. Give each selected legacy
fixture one row with `survey_id`, `corpus_id`, source hash, origin, selector names,
source path, and separate optional ETF and Finale-27 path, hash, and pairing-quality
columns. Derive it from the existing inventory and private companion mappings; do
not rescan the corpus.

Apply these selectors:

- `all`: every inventoried legacy fixture, including loose files and archive
  members when the registered survey includes archives.
- `loose`: only rows whose `origin` is `filesystem`; do not silently include
  extracted archive-cache copies.
- `etf`: only fixtures with a privately resolved ETF companion. Use an existing
  private mapping when available. If none exists, ask for the ETF naming and
  location convention, then create an analysis-local private companion mapping;
  never guess or publish it.
- `fin27`: only fixtures with a readable modern Finale export. Select by
  `export_match` as described below.

When combining selectors, state whether the operation is an intersection or a
union. For example, `loose AND fin27` excludes archive members even if they have
exports, while `etf OR fin27` admits either companion and may give some fixtures
both. Never let the availability of a companion silently change an `all` or
`loose` denominator.

For `fin27`, grade pair quality:

- Use `adjacent-exact` by default.
- Include `fallback-unique` only as a separately reported, weaker cohort.
- Exclude `fallback-ambiguous` unless the user resolves the pairing.
- Exclude rows with no readable export.

Grade ETF pair quality independently using its recorded provenance or naming
rule. Exclude ambiguous ETF matches unless the user resolves them.

Count both source occurrences and distinct `corpus_id` values. Use distinct
content as the primary statistical unit so duplicate files do not overstate the
evidence, while retaining occurrence counts to describe corpus coverage.

## Step 3 — Build one targeted extractor

Prefer a reproducible, location-agnostic probe under `scripts/` when the study is
likely to be repeated. It must accept all paths as command-line arguments and
write only to an explicit output path under `private/generated/`. A disposable
probe under `/tmp` is acceptable for exploration, but preserve the final method
before publishing results.

For each fixture, emit one private observation containing at least:

- `survey_id`, `corpus_id`, selectors, source hash, and source origin;
- detected source era, byte order, and saving product;
- extraction status for the legacy fixture and each requested companion;
- the requested raw source records or tuples, including physical ordinal,
  comparator, incidence, and raw value where applicable;
- independently extracted ETF or modern-export objects and fields, when selected;
- semantic referents needed for comparison, such as a referenced font name;
- companion kind, hash, provenance, and pairing quality;
- warnings and unsupported conditions.

Use existing framing, decompression, indexing, and MUSX `score.dat` decoding
code where available. Do not create a second implementation of a format fact in
library code. A research script may repeat a rule for an independent check, but
label which implementation produced each result.

## Step 4 — Compare semantics, representation, and upgrade behavior

Compare in this order:

1. **Physical capture:** Did the source extractor find the record, tuple, and raw
   field values that the proposed layout predicts?
2. **Referential resolution:** If a field is a cmper, resolve it on its own side.
   Never compare cross-file cmpers directly.
3. **Semantic equivalence:** When a companion exists, compare the resolved objects
   using the class-specific rule. Normalize names only with the project's canonical
   normalizer and retain every original spelling in private output.
4. **Companion transformation:** For an ETF, characterize any export-time change.
   For a modern re-save, determine whether Finale preserved, renumbered,
   canonicalized, substituted, synthesized, or removed the value during upgrade.
5. **Reader coverage:** Independently determine whether the current reader recovers
   the physical and semantic source facts needed to reproduce the intended result.

Assign one primary comparison outcome to each comparable field or collection
element when a companion exists:

- `preserved`: same semantic value and compatible representation;
- `remapped`: same semantic referent under a different cmper or ordering;
- `normalized`: spelling or equivalent representation changed without changing
  the referent;
- `substituted`: Finale selected a different semantic referent during upgrade;
- `synthesized`: the export contains a value absent from the source;
- `removed`: a source value is absent from the export;
- `unresolved`: extraction or equivalence is insufficient to classify it.

Do not call `substituted`, `synthesized`, or `removed` a reader failure unless an
independent source fact shows that the reader lost required information.

Record `reader-gap` independently whenever source evidence exists but the reader
does not recover it. For fixtures without a companion, report physical extraction
status and reader coverage without inventing a comparison outcome.

## Step 5 — Validate the survey

- Manually inspect at least one fixture from every source era and selector cohort
  represented, plus every non-preservation outcome.
- Verify raw source values at byte offsets for a small sample rather than trusting
  only the reader under evaluation.
- Check that each selector's candidate count equals its selected count plus
  explicitly reported exclusions. For unions, report overlap; for intersections,
  report the filtering sequence.
- Check that every tracked aggregate can be regenerated from private observations.
- Run syntax checks and focused tests for any added probe.
- Inspect `git status --short` and `git diff --check`; preserve unrelated changes.

## Step 6 — Report and publish

Lead with the selection funnel, then a coverage table grouped by selector cohort,
era, companion kind, and outcome. Report both distinct content IDs and occurrences,
pair-quality cohorts, extraction failures, and unsupported layouts. State explicitly
which denominators do and do not require a companion, and that Finale exports may
reflect the exporting or upgrading system rather than the original system.

Use the project's confidence vocabulary: `confirmed`, `strong`, `weak`, and
`open`. Read `research/CITING_EVIDENCE.md` before changing shared findings.

If the user requests tracked deliverables, publish only:

- an aggregate CSV under `research/data/` for cross-survey facts; and
- a concise research note that identifies the method, limitations, outcome
  counts, and evidence tokens.

Before publishing, read `../survey-a-corpus/SKILL.md` and run its Step 5 path
and filename leak checks against every proposed tracked file. Fix the generator
if output leaks; do not hand-edit generated results.

## Font-reference example

For a font-id field, record the numeric id and resolve its `FontDefinition` on
the source side. When an ETF or modern export is selected, resolve its id against
that companion's own font definitions. Compare normalized names, but retain raw
names and charset metadata privately. Classify equal normalized names with
different ids as `remapped`; a different resolved name is `substituted`, not
automatically a decoder error. Treat id zero according to the class's documented
default-font semantics rather than searching for a concrete font record.
