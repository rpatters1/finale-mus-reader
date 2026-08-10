# Reproducing the Survey

## Local evidence mappings

Published findings identify each source by a stable content-derived `corpus_id`, such as
`mus-65aa1de01997b781`, plus its size and SHA-256. Filenames are not published. That identifier is derived from the file's bytes, so
the same file surveyed from two different corpora carries the same `corpus_id` on purpose: it is what lets separate
surveys corroborate each other.

Which survey a deliverable came from is therefore a separate dimension, never part of the identifier. Each survey
publishes under `research/corpora/<survey_id>/`, where `survey_id` is `<github-user>-<corpus-slug>`, and registers a
row in [`data/surveys.csv`](data/surveys.csv). The reference corpus is `rpatters1-main`, so its manifest is
[`corpora/rpatters1-main/data/corpus_manifest.csv`](corpora/rpatters1-main/data/corpus_manifest.csv).
Agents should read `.agents/skills/survey-a-corpus/SKILL.md`, which covers registration and the pre-publication
checks.

The original path mapping is deliberately not tracked. If the corpus is available locally, create
`private/generated/corpus_locations.csv` with `scripts/publish_manifest.py --private-output ...`; the repository
`.gitignore` excludes that directory. This exact filename is the canonical local key for resolving a public
`corpus_id` back to the evidence file. Existing raw inventory outputs containing paths are working data and should
likewise remain local.

Archive members use the same public `member_id` convention. Their ignored local archive/member path mapping is
`private/generated/archive_locations.csv`, produced with `archive_probe.py --private-output ...`.

Every path below is a local convention rather than a property of the format. `inventory.py` therefore requires
`--export-dir-name` and `--export-suffix` and supplies no defaults, so a corpus organized differently is inventoried
by passing its own convention. Pass leading-dash values with `=`, as `--export-dir-name=-exports`, or argparse reads
the value as a flag.

For StuffIt archives, install the `unar` package so that both `unar` and `lsar` are available. Use `lsar` for a
non-destructive member listing and `unar -o <temporary-directory> <archive>` for extraction; never extract over the
source corpus.

The local ETF evidence directory is `private/evidence/`. Controlled publishable MUS/ETF pairs for Finale 2002–2005
are tracked under `tests/evidence/`.

## Commands

Run from the finale-mus-reader repository root. Script output goes to `private/generated/`, which is ignored
because every file in it is reproducible from these commands.

Archive members are surveyed by adding `--include-archives` to the first command, not by a separate pass. Members
are extracted once into `private/generated/archive_cache/` and recorded as ordinary inventory rows carrying
`origin=archive`, so every later step covers them exactly as it covers loose files. Only `.mus` members and
extensionless members that carry a Finale banner are cached. Reruns reuse the cache; `--refresh-archive-cache`
forces re-extraction.

```bash
python3 scripts/inventory.py \
  '<local-corpus-root>' \
  --output-dir private/generated \
  --export-dir-name='<export-dir-name>' \
  --export-suffix='<export-suffix>' \
  --include-archives          # optional; slow on the first run

# Renders the archive deliverables from that inventory. Only meaningful when the
# inventory was built with --include-archives; it no longer opens archives itself.
python3 scripts/archive_probe.py \
  private/generated/corpus_inventory.csv \
  --output research/corpora/rpatters1-main/data/archive_members.csv \
  --summary research/corpora/rpatters1-main/ARCHIVE_SURVEY.md \
  --private-output private/generated/archive_locations.csv

python3 scripts/publish_manifest.py \
  private/generated/corpus_inventory.csv \
  --public-output research/corpora/rpatters1-main/data/corpus_manifest.csv \
  --private-output private/generated/corpus_locations.csv

python3 scripts/structure_probe.py \
  private/generated/corpus_inventory.csv \
  --output-dir private/generated

python3 scripts/dcl_probe.py \
  private/generated/corpus_locations.csv \
  research/corpora/rpatters1-main/data/corpus_manifest.csv \
  --blast '<path-to-blast-compatible-executable>' \
  --output private/generated/dcl_probe.json

python3 scripts/uncompressed_probe.py \
  private/generated/corpus_locations.csv \
  research/corpora/rpatters1-main/data/corpus_manifest.csv \
  --output private/generated/uncompressed_probe.json

python3 scripts/musx_semantics.py \
  private/generated/corpus_inventory.csv \
  --output private/generated/musx_semantics.csv

python3 scripts/correlate_records.py \
  private/generated/corpus_inventory.csv \
  private/generated/musx_semantics.csv \
  --output research/corpora/rpatters1-main/data/record_correlations.csv

python3 scripts/render_record_catalog.py \
  private/generated/record_catalog.csv \
  research/corpora/rpatters1-main/data/record_correlations.csv \
  --inventory private/generated/corpus_inventory.csv \
  --output research/corpora/rpatters1-main/RECORD_CATALOG.md

python3 scripts/render_corpus_inventory.py \
  private/generated/corpus_inventory.csv \
  private/generated/structure_probe.csv \
  private/generated/musx_semantics.csv \
  --output research/corpora/rpatters1-main/CORPUS_INVENTORY.md
```

`structure_probe.py` also writes `version_structure_summary.csv` into `private/generated/`; the published copy at
`research/corpora/rpatters1-main/data/version_structure_summary.csv` is placed there by hand.

`inventory.py` also writes a superseded `CORPUS_INVENTORY.md` to the parent of its `--output-dir`; the copy written
by `render_corpus_inventory.py` in the last step is the real one.

The scripts are read-only with respect to the evidence corpus. `musx_semantics.py` uses the public symmetric
`score.dat` recoding algorithm documented in the MIT-licensed sibling denigma project and keeps decoded XML in
memory.

