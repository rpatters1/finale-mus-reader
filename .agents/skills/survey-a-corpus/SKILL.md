---
name: survey-a-corpus
description: Inventory a local corpus of legacy Finale .mus files and publish the results as reviewable public deliverables. Use when asked to survey, inventory, or catalog a .mus corpus, to reproduce the corpus study on a different machine, to contribute survey results upstream, or to refresh the existing deliverables after the corpus changes. Covers prompting the user for their corpus conventions, keeping paths private, and namespacing results per survey so several corpora can coexist.
---

# Survey a corpus

Run the analysis pipeline in `scripts/` over a local corpus of legacy Finale
`.mus` files, then publish sanitized deliverables that can be reviewed and
merged without exposing anyone's filesystem.

Two things make this more than "run the scripts":

1. **The corpus conventions are unknowable from the code.** Where the corpus
   lives, how exports are named, and where they sit relative to their sources
   describe one person's filing habits, not the file format. You must ask.
2. **Results from different corpora must stay distinguishable.** Deliverables
   are namespaced by a `survey_id` and registered in `research/data/surveys.csv`.

## Hard rules

- **The corpus is read-only.** Every script reads and never writes inside it.
  Never extract an archive over the source corpus; `unar` extracts to a
  temporary directory. Never move, rename, or "tidy" corpus files.
- **Names and paths are private.** Public deliverables carry content-derived
  IDs, sizes, hashes, and structure — never a filename, member name, archive
  name, or path. A filename can name a work, a client, or a person. Anything
  bearing a name goes to `private/generated/`, which is gitignored. Step 5 is a
  mandatory check, not a formality.
- **Print total import failures locally.** Whenever any survey stage attempts a
  complete document import and a fixture fails, print that fixture's name and
  private source path to the console, together with the error. Print every
  failed occurrence, including archive members. This local diagnostic is the
  intentional exception to path suppression; never copy it into tracked output.
- **Do not move the generators.** `scripts/` is deliberately public and
  location-agnostic: every path is a CLI argument. Keep it that way. Anything
  corpus-specific belongs in the invocation, not in the code.
- **`research/REPRODUCING_THE_SURVEY.md` is authoritative** for the pipeline. If
  it and this skill disagree, follow that document and fix this one.

## Step 0 — Ask the user for their conventions

Ask before running anything, and do not guess. Missing answers produce a survey
that silently reports zero exports.

1. **Corpus root** — absolute path to the directory to scan recursively. If it
   is on removable media, confirm it is mounted now.
2. **Export directory name** — the subdirectory beside each source holding its
   modern Finale re-saves, e.g. `-exports`. Ask whether it sits directly beside
   the sources or nested (`out/musx` works; depth is handled).
3. **Export suffix** — what identifies an export, e.g. `.fin27.musx`.
4. **Does the corpus have exports at all?** If not, see Limitations; the survey
   still works but the semantic half of the pipeline is skipped.
5. **Scan archives?** Members of `.zip`/`.sit` archives can be surveyed
   alongside loose files, and they carry real weight: they are often the only
   copies of the oldest Finale versions. It is opt-in because the first run
   extracts them, which is the slowest part of a survey, and it needs `unar` and
   `lsar` on PATH (`brew install unar`). Warn about the runtime before starting,
   and mention the cache: extracted members land in an ignored directory under
   `private/generated/<survey_id>/`, so later runs are fast but disk is used —
   the reference corpus caches roughly 720 MB.
6. **Are the sources named `*.mus`?** Ask, and do not assume. Classic Mac Finale
   kept the file type in the resource fork, so a corpus containing Mac
   installations or Mac-authored documents holds documents with no extension at
   all, and an extension-only scan reports a confident zero for whole eras.
   `--sniff-content` inventories any loose file whose header is Finale content.
   Prefer it whenever the corpus predates OS X or came off a Mac; the cost is
   reading 256 bytes per file.
7. **Anything to exclude?** `--exclude=GLOB` is repeatable and skips a loose
   path, an individual archive-member path, or anything beneath a matching
   directory. Two kinds of answer matter: material
   that is ENIGMA-framed but is not a document (`*.lib` library files, `*.fan`
   font annotations), and subtrees a survey must not read at all. Ask rather
   than guess, then verify the guess: excluding a whole directory because its
   name suggests non-documents is how a survey loses real files, since Finale
   moved `Maestro Font Default` and `Jazz Font Default` into `Libraries/` at
   version 2007. Check what an exclusion actually drops before adopting it.
8. **DCL probe?** Needs a `blast`-compatible executable; ask for its path or
   skip that step.
9. **Publishing intent** — are they surveying privately, or contributing
   deliverables upstream? If private only, do Steps 1–5 and stop.

Record the answers verbatim in the survey registry (Step 2) so a later reader
can tell what the numbers describe.

**Passing a leading-dash value:** `argparse` reads `-exports` as a flag. Always
use the `=` form: `--export-dir-name=-exports`.

## Step 1 — Preflight

- `python3 --version` (3.10+; the scripts use `X | Y` type syntax).
- Corpus root exists and is readable.
- `unar`/`lsar` present, if scanning archives.
- The working tree is clean, or the user knows what is already modified — the
  pipeline rewrites tracked files.
- Free disk for `private/generated/`: a few MB of CSV per thousand sources, plus
  the archive cache when archives are included, which holds one copy of every
  distinct member (roughly 720 MB for the reference corpus).

## Step 2 — Choose and register a survey_id

`survey_id` is `<github-user>-<corpus-slug>`, e.g. `rpatters1-main`. The GitHub
handle is a namespace that is already unique and already the review channel; the
slug distinguishes several corpora belonging to one person. A contributor who
would rather not use their handle may pick any slug unused in the registry —
uniqueness is enforced by review, not by the name's origin.

Do **not** fold the survey into `corpus_id`. `corpus_id` is `mus-<sha256[:16]>`
of the file's bytes, so the same file surveyed by two people gets the same ID on
purpose: that is what makes cross-corpus corroboration possible. Provenance is a
separate column, never a change to the identifier.

Append one row to `research/data/surveys.csv`:

| column | meaning |
|---|---|
| `survey_id` | `<github-user>-<corpus-slug>`; frozen once merged, even if the handle changes |
| `github_user` | handle at time of contribution, or blank |
| `corpus_slug` | short label for this particular corpus |
| `surveyed_date` | ISO date the pipeline was run |
| `tool_commit` | `git rev-parse --short HEAD` of this repo at run time |
| `source_count` | `.mus` files found |
| `export_dir_name`, `export_suffix` | the answers from Step 0 |
| `corpus_fingerprint` | see below |
| `notes` | anything a reader needs to interpret the numbers |

The fingerprint identifies the corpus *content* independently of who surveyed
it, so two survey rows describing the same underlying corpus are detectable, and
a corpus that changed between runs is visible. It is evidence, not a name — it
changes whenever a file is added, so never use it as a label:

Compute it over **loose files only**. Archive members are included or not
depending on a flag, so hashing every row would make one corpus fingerprint two
different ways and destroy the comparison the field exists for:

```bash
python3 - <<'PY'
import csv, hashlib
with open(f"private/generated/{SURVEY}/corpus_inventory.csv", newline="", encoding="utf-8") as fh:
    digests = sorted({row["source_sha256"] for row in csv.DictReader(fh)
                      if row.get("origin", "filesystem") == "filesystem"})
print("corpus-" + hashlib.sha256("\n".join(digests).encode()).hexdigest()[:16])
PY
```

Record archive coverage separately in `notes`, not in the fingerprint.

Deliverables go under `research/corpora/<survey_id>/`, with CSVs in
`research/corpora/<survey_id>/data/`. Every survey uses this layout, including
the reference corpus at `research/corpora/rpatters1-main/`. **Write only inside
your own survey directory.** Never modify another survey's files: two surveys
disagreeing about the same `corpus_id` is a finding to report, not a conflict to
resolve by editing.

`research/data/` holds cross-survey material only — the registry, plus analyses
not produced by this pipeline. Do not add per-survey output there.

## Step 3 — Private output directory

All path-bearing output goes to `private/generated/<survey_id>/`, which the
repository `.gitignore` excludes. Create it if absent. Private output is
namespaced per survey exactly as public deliverables are, so rebuilding one
corpus never overwrites another's results:

| Path | Holds |
|---|---|
| `private/corpora/<survey_id>.conf` | that corpus's root and conventions |
| `private/generated/<survey_id>/` | its path-bearing CSVs and archive cache |
| `private/evidence/<survey_id>/` | fixtures taken from it |

The archive cache is per-survey deliberately: `build_cache` rewrites
`cache_index.csv` with only the archives it saw on that run, so a shared cache
would lose the other corpus's index entries and re-extract everything next time.

If the user wants their own local history of this, `private/` can be its own
local-only git repository with a `pre-push` hook that refuses every push; never
give `private/` a remote. When it is set up that way, `private/regenerate.sh
<survey_id>` drives everything below from the corpus's `.conf`, and that is the
normal way to run a survey; the commands in Step 4 are what it executes.

## Step 4 — Run the pipeline

From the repository root, with `SURVEY=<survey_id>`. Order matters: later steps
consume earlier outputs.

```bash
GEN="private/generated/$SURVEY"
OUT="research/corpora/$SURVEY"
mkdir -p "$OUT/data" "$GEN"

# Add --include-archives only if the user agreed. Members then appear as
# ordinary inventory rows with origin=archive, and every step below covers them.
# Add --sniff-content if sources are not all named *.mus, and --exclude=GLOB
# (repeatable) for anything the survey must skip.
python3 scripts/inventory.py '<corpus-root>' \
  --output-dir "$GEN" \
  --export-dir-name='<export-dir-name>' \
  --export-suffix='<export-suffix>' \
  --include-archives

# Only when the inventory included archives. This renders deliverables from that
# inventory; it does not open archives itself.
python3 scripts/archive_probe.py "$GEN/corpus_inventory.csv" \
  --output "$OUT/data/archive_members.csv" \
  --summary "$OUT/ARCHIVE_SURVEY.md" \
  --private-output "$GEN/archive_locations.csv"

python3 scripts/publish_manifest.py "$GEN/corpus_inventory.csv" \
  --public-output "$OUT/data/corpus_manifest.csv" \
  --private-output "$GEN/corpus_locations.csv"

python3 scripts/structure_probe.py "$GEN/corpus_inventory.csv" \
  --output-dir "$GEN"

# Optional; needs a blast-compatible executable.
python3 scripts/dcl_probe.py "$GEN/corpus_locations.csv" \
  "$OUT/data/corpus_manifest.csv" \
  --blast '<path-to-blast>' \
  --output "$GEN/dcl_probe.json"

python3 scripts/uncompressed_probe.py "$GEN/corpus_locations.csv" \
  "$OUT/data/corpus_manifest.csv" \
  --output "$GEN/uncompressed_probe.json"

# The three steps below need exports; skip them for a corpus without any and
# call render_corpus_inventory.py with two positional arguments instead of three.
python3 scripts/musx_semantics.py "$GEN/corpus_inventory.csv" \
  --output "$GEN/musx_semantics.csv"

python3 scripts/correlate_records.py "$GEN/corpus_inventory.csv" \
  "$GEN/musx_semantics.csv" \
  --output "$OUT/data/record_correlations.csv"

python3 scripts/render_record_catalog.py "$GEN/record_catalog.csv" \
  "$OUT/data/record_correlations.csv" \
  --inventory "$GEN/corpus_inventory.csv" \
  --output "$OUT/RECORD_CATALOG.md"

python3 scripts/render_corpus_inventory.py "$GEN/corpus_inventory.csv" \
  "$GEN/structure_probe.csv" \
  "$GEN/musx_semantics.csv" \
  --output "$OUT/CORPUS_INVENTORY.md"
```

`structure_probe.py` writes `version_structure_summary.csv` into `$GEN` rather
than to a public path; copy it to `$OUT/data/` by hand if publishing it.

`inventory.py` additionally writes a superseded, path-bearing
`CORPUS_INVENTORY.md` into its `--output-dir`. Ignore it; the copy from
`render_corpus_inventory.py` is the publishable one.

## Step 5 — Leak check before publishing

Mandatory. Run against everything about to be committed:

```bash
grep -rnE '/Users/|/home/|/Volumes/|[A-Za-z]:\\\\|/private/generated/' \
  "research/corpora/$SURVEY" && echo "LEAK — do not commit" || echo "clean"
```

Also grep for the corpus root string itself and for the user's account name,
which the pattern above will not catch if the corpus lives somewhere unusual.

Paths are the easy half. **Filenames must also be checked**, and no path pattern
finds them. Write a short script that collects every basename and stem from
`private/generated/<survey_id>/corpus_inventory.csv` (columns `source_relative`,
`export_relative`, `member_path`), drops names shorter than 8 characters and
obvious dictionary words such as `score` or `text`, and then reports any that
appear in a published file under `research/corpora/<survey_id>/`. Short and
generic names produce false positives — `Score` collides with the XML tag
`repeatStaffListScore` — so inspect anything it reports in context rather than
trusting the match.

If anything matches, fix the generator rather than hand-editing the output —
hand-edits are silently undone by the next run.

Sanity-check the numbers too: `source_count` should match the manifest row
count, and a sudden collapse to zero exports almost always means a wrong
`--export-suffix` rather than a corpus without exports.

## Step 6 — Record what the survey found

CSVs are not the contribution; the findings are. A survey that adds numbers to a
directory and says nothing about the format has not paid for its review.

Read `research/CITING_EVIDENCE.md` first — it defines evidence tokens, what each
confidence level requires, and how to record a contradiction. Then compare this
survey against the existing notes, chiefly `research/FORMAT_NOTES.md` and
`research/VERSION_MATRIX.md`, and propose edits for:

- **Corroboration.** A claim previously seen in one survey and now independently
  observed in another can rise to `confirmed`. This is the single most valuable
  thing an additional corpus provides, and it is easy to overlook because
  nothing looks new.
- **Contradiction.** Anything this corpus does that the notes say it should not.
  Record it beside the existing claim and lower that claim's confidence; never
  edit it away. Check first whether the two observations cover the same era —
  inapplicable is not the same as contradicted.
- **New coverage.** Eras or products this corpus contains that no survey had.
  A single file from an unrepresented version is worth more than ten thousand
  from a well-covered one.
- **Unresolved framing.** Files whose records do not parse are a finding, not a
  gap to stay quiet about. Report the count by `saving_product`.

Cite by token, never by path. Show the user the proposed edits and let them
decide what to include; do not silently rewrite established findings.

**Per-file structural detail is not published.** Record types, size
distributions, and element counts, file by file, add up to a detailed portrait
of someone's life's work, and a survey does not need to give that up to be
useful. Structure is published in aggregate — `RECORD_CATALOG.md`,
`data/record_correlations.csv`, `data/version_structure_summary.csv` — and a
finding says which files it rests on and how many, by `corpus_id`. Whoever holds
the corpus can re-derive the detail from `private/generated/`. Do not add a step
that dumps per-file structure into the survey directory; that was tried and
withdrawn deliberately.

**File headers are not published either, and this is not a preference to offer
the user.** The header holds the document's title, composer, and description, and
the surrounding fields are not yet mapped, so metadata cannot be excised by
field. Do not add a flag for it, do not paste header bytes into a finding, and
do not raise it as a tradeoff for the user to weigh. See
`research/FORMAT_NOTES.md` on the header text region for the offsets known so
far; extending that mapping is the work that would reopen the question.

## Step 7 — Offer to donate fixtures

Ask only after the survey succeeds, and only about files the user plausibly owns
— their own compositions, not published works they merely have a copy of.

A committed fixture is worth more than any quantity of statistics, because it
makes a claim checkable by everyone permanently. The most valuable donations are
**controlled-difference pairs**: the same short piece saved twice with exactly
one change, which isolates the bytes that encode that change. Follow the
`tests/evidence/F2003/` pattern.

Ask for, and record in `tests/evidence/<survey_id>/provenance.txt`:

- who authored the music, and confirmation they are licensing it under this
  repository's license;
- the exact Finale version and the operating system it ran on;
- what differs between paired files, if they are a controlled pair.

Never accept a file the user did not author, and never commit one before the
provenance record exists. If they decline, say so in the survey notes and move
on — declining is normal and the survey is still worth contributing.

## Step 8 — Review and contribute

- Show the user the diff, including the registry row and any proposed changes to
  the shared research notes.
- Commit on a branch, never directly on `main`.
- A contribution is one survey directory, one registry line, cited edits to the
  research notes, and optionally donated fixtures. Keep unrelated changes out
  of it.

## Limitations and known traps

- **`inventory.py` requires `--export-dir-name` and `--export-suffix`** and has
  no defaults, by design. A corpus genuinely without exports needs a suffix that
  matches nothing (e.g. `--export-suffix=.no-exports`); every row then reports
  `missing`, and the semantic steps should be skipped. If this case becomes
  common, making export matching properly optional is the better fix.
- **Duplicate files share a `corpus_id`.** The reference corpus has 1,218
  sources but only 1,140 distinct hashes. Row counts and ID counts legitimately
  differ; do not "fix" this.
- **`export_match` values are graded**: `adjacent-exact` means the convention
  matched; `fallback-unique` and `fallback-ambiguous` mean a basename search
  found it elsewhere and the pairing is weaker evidence. Never present fallback
  matches as exact.
- **The archive pass is the long pole**, and StuffIt handling depends on `unar`
  1.10.7 or compatible.
- Legacy MUS is a family of formats. Report what was observed for the versions
  actually present; do not generalize a finding from one era to the corpus.
