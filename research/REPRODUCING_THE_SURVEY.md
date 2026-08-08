# Reproducing the Survey

## Local evidence mappings

Published findings identify each source by a stable content-derived `corpus_id`, such as
`mus-65aa1de01997b781`, plus its basename, size, and SHA-256. The public manifest is
[`data/corpus_manifest.csv`](data/corpus_manifest.csv).

The original path mapping is deliberately not tracked. If the corpus is available locally, create
`private/corpus_locations.csv` with `scripts/publish_manifest.py --private-output ...`; the repository `.gitignore`
excludes that directory. This exact filename is the canonical local key for resolving a public `corpus_id` back to
the evidence file. Existing raw inventory outputs containing paths are working data and should likewise remain local.

Archive members use the same public `member_id` convention. Their ignored local archive/member path mapping is
`private/archive_locations.csv`, produced with `archive_probe.py --private-output ...`.

For StuffIt archives, install the `unar` package so that both `unar` and `lsar` are available. Use `lsar` for a
non-destructive member listing and `unar -o <temporary-directory> <archive>` for extraction; never extract over the
source corpus.

The local ETF evidence directory is `private/evidence/`. Controlled publishable MUS/ETF pairs for Finale 2002–2005
are tracked under `tests/evidence/`.

## Commands

Run from the finale-mus-reader repository root:

```bash
python3 scripts/inventory.py \
  '<local-corpus-root>' \
  --output-dir private

python3 scripts/archive_probe.py \
  '<local-corpus-root>' \
  --output research/data/archive_members.csv \
  --summary research/ARCHIVE_SURVEY.md \
  --private-output private/archive_locations.csv

python3 scripts/publish_manifest.py \
  private/corpus_inventory.csv \
  --public-output research/data/corpus_manifest.csv \
  --private-output private/corpus_locations.csv

python3 scripts/structure_probe.py \
  private/corpus_inventory.csv \
  --output-dir private

python3 scripts/dcl_probe.py \
  private/corpus_locations.csv \
  research/data/corpus_manifest.csv \
  --blast '<path-to-blast-compatible-executable>' \
  --output private/dcl_probe.json

python3 scripts/uncompressed_probe.py \
  private/corpus_locations.csv \
  research/data/corpus_manifest.csv \
  --output private/uncompressed_probe.json

python3 scripts/musx_semantics.py \
  private/corpus_inventory.csv \
  --output private/musx_semantics.csv

python3 scripts/correlate_records.py \
  private/corpus_inventory.csv \
  private/musx_semantics.csv \
  --output research/data/record_correlations.csv

python3 scripts/render_record_catalog.py \
  private/record_catalog.csv \
  research/data/record_correlations.csv \
  --inventory private/corpus_inventory.csv \
  --output research/RECORD_CATALOG.md

python3 scripts/render_corpus_inventory.py \
  private/corpus_inventory.csv \
  private/structure_probe.csv \
  private/musx_semantics.csv \
  --output research/CORPUS_INVENTORY.md
```

The scripts are read-only with respect to the evidence corpus. `musx_semantics.py` uses the public symmetric
`score.dat` recoding algorithm documented in the MIT-licensed sibling denigma project and keeps decoded XML in
memory.

