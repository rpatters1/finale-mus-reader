# Reproducing the Survey

## Local evidence mappings

Published findings identify each source by a stable content-derived `corpus_id`, such as
`mus-65aa1de01997b781`, plus its size and SHA-256. Filenames are not published. That identifier is derived from the file's bytes, so
the same file surveyed from two different corpora carries the same `corpus_id` on purpose: it is what lets separate
surveys corroborate each other.

Which survey a deliverable came from is therefore a separate dimension, never part of the identifier. Each survey
publishes under `research/corpora/<survey_id>/`, where `survey_id` is `<github-user>-<corpus-slug>`, and registers a
row in [`data/surveys.csv`](../data/surveys.csv). The reference corpus is `rpatters1-main`, so its manifest is
[`corpora/rpatters1-main/data/corpus_manifest.csv`](../corpora/rpatters1-main/data/corpus_manifest.csv).
Agents should read `.agents/skills/inventory-a-corpus/SKILL.md`, which covers registration and the pre-publication
checks.

The original path mapping is deliberately not tracked. If the corpus is available locally, create
`private/generated/<survey_id>/corpus_locations.csv` with `scripts/publish_manifest.py --private-output ...`; the
repository `.gitignore` excludes that directory. This exact filename is the canonical local key for resolving a
public `corpus_id` back to the evidence file. Existing raw inventory outputs containing paths are working data and
should likewise remain local.

Private output is namespaced by `survey_id` the same way public deliverables are, so results from several corpora
coexist without overwriting each other:

| Path | Holds |
|---|---|
| `private/corpora/<survey_id>.conf` | that corpus's root and conventions |
| `private/generated/<survey_id>/` | its path-bearing CSVs and its archive cache |
| `private/evidence/<survey_id>/` | fixtures taken from it |
| `research/corpora/<survey_id>/` | its publishable deliverables |

The archive cache is per-survey deliberately: `build_cache` rewrites `cache_index.csv` with only the archives seen on
that run, so a cache shared between corpora would drop the other corpus's index entries and re-extract everything
next time.

Archive members use the same public `member_id` convention. Their ignored local archive/member path mapping is
`private/generated/<survey_id>/archive_locations.csv`, produced with `archive_probe.py --private-output ...`.

Every path below is a local convention rather than a property of the format. `inventory.py` therefore requires
`--export-dir-name` and `--export-suffix` and supplies no defaults, so a corpus organized differently is inventoried
by passing its own convention. Pass leading-dash values with `=`, as `--export-dir-name=-exports`, or argparse reads
the value as a flag.

Two further options exist for corpora that are not somebody's document folder:

- `--sniff-content` inventories a loose file whose header is Finale content even when its name is not `*.mus`.
  Classic Mac Finale kept the file type in the resource fork, so its documents commonly have no extension at all and
  an extension-only scan misses every one of them, including whole eras. Each inventory row records a `discovery`
  column of `suffix`, `sniffed`, or `archive-member`, so sniffed specimens stay separable from named ones.
- `--exclude=GLOB` (repeatable) skips anything matching the glob or living beneath a directory that matches it,
  relative to the corpus root. It applies to sources, exports, and archives alike. Use it for material that is
  ENIGMA-framed but is not a document — Finale library and font-annotation files, or another application's files
  that carry a Finale banner — and for subtrees a survey must not read. Matching is case-insensitive: a corpus
  spanning classic Mac, DOS and Windows filesystems carries `.mus`, `.MUS`, `.lib`, `.LIB` and `.Lib` at once, and a
  case-sensitive exclusion would quietly cover only part of what it names. Exclusions are recorded in
  `inventory_summary.json`, because a survey run with them covers less than the tree it names and its counts are
  only comparable with another run that used the same ones.

Recognizing a specimen is a matter of the banner, whose three spellings are tabulated in
[`format/container/header.md`](../format/container/header.md#the-three-banner-spellings).
`inventory.py` and `archive_sources.py` each carry that pattern and must stay in step: a spelling missing from
`archive_sources.py` means matching members are never cached, so nothing downstream ever sees them.

For StuffIt archives, install the `unar` package so that both `unar` and `lsar` are available. Use `lsar` for a
non-destructive member listing and `unar -o <temporary-directory> <archive>` for extraction; never extract over the
source corpus.

The local ETF evidence directory is `private/evidence/<survey_id>/`. Controlled publishable MUS/ETF pairs for
Finale 2002–2005 are tracked under `tests/evidence/`.

## Commands

Run from the finale-mus-reader repository root, with `SURVEY=<survey_id>` and `GEN=private/generated/$SURVEY`.
Script output goes under `private/generated/`, which is ignored because every file in it is reproducible from these
commands. Whoever holds a corpus locally will normally drive this through `private/regenerate.sh <survey_id>`, which
keeps each corpus's root and conventions in `private/corpora/<survey_id>.conf` rather than repeating them here.

Archive members are surveyed by adding `--include-archives` to the first command, not by a separate pass. Members
are extracted once into `$GEN/archive_cache/` and recorded as ordinary inventory rows carrying `origin=archive`, so
every later step covers them exactly as it covers loose files. Only `.mus` members and extensionless members that
carry a Finale banner are cached. Reruns reuse the cache; `--refresh-archive-cache` forces re-extraction.

```bash
SURVEY=<survey_id>
GEN="private/generated/$SURVEY"
OUT="research/corpora/$SURVEY"
mkdir -p "$GEN" "$OUT/data"

python3 scripts/inventory.py \
  '<local-corpus-root>' \
  --output-dir "$GEN" \
  --export-dir-name='<export-dir-name>' \
  --export-suffix='<export-suffix>' \
  --include-archives          # optional; slow on the first run
  # --sniff-content           # optional; required for extensionless classic Mac documents
  # --exclude='<glob>'        # optional; repeatable

# Renders the archive deliverables from that inventory. Only meaningful when the
# inventory was built with --include-archives; it no longer opens archives itself.
python3 scripts/archive_probe.py \
  "$GEN/corpus_inventory.csv" \
  --output "$OUT/data/archive_members.csv" \
  --summary "$OUT/ARCHIVE_SURVEY.md" \
  --private-output "$GEN/archive_locations.csv"

python3 scripts/publish_manifest.py \
  "$GEN/corpus_inventory.csv" \
  --public-output "$OUT/data/corpus_manifest.csv" \
  --private-output "$GEN/corpus_locations.csv"

python3 scripts/structure_probe.py \
  "$GEN/corpus_inventory.csv" \
  --output-dir "$GEN"

python3 scripts/dcl_probe.py \
  "$GEN/corpus_locations.csv" \
  "$OUT/data/corpus_manifest.csv" \
  --blast '<path-to-blast-compatible-executable>' \
  --output "$GEN/dcl_probe.json"

python3 scripts/uncompressed_probe.py \
  "$GEN/corpus_locations.csv" \
  "$OUT/data/corpus_manifest.csv" \
  --output "$GEN/uncompressed_probe.json"

# The next three steps compare each source against its modern re-save. Skip them
# for a corpus without exports and pass render_corpus_inventory.py two arguments.
python3 scripts/musx_semantics.py \
  "$GEN/corpus_inventory.csv" \
  --output "$GEN/musx_semantics.csv"

python3 scripts/correlate_records.py \
  "$GEN/corpus_inventory.csv" \
  "$GEN/musx_semantics.csv" \
  --output "$OUT/data/record_correlations.csv"

python3 scripts/render_record_catalog.py \
  "$GEN/record_catalog.csv" \
  "$OUT/data/record_correlations.csv" \
  --inventory "$GEN/corpus_inventory.csv" \
  --output "$OUT/RECORD_CATALOG.md"

python3 scripts/render_corpus_inventory.py \
  "$GEN/corpus_inventory.csv" \
  "$GEN/structure_probe.csv" \
  "$GEN/musx_semantics.csv" \
  --output "$OUT/CORPUS_INVENTORY.md"
```

`structure_probe.py` also writes `version_structure_summary.csv` into `$GEN`; the published copy at
`$OUT/data/version_structure_summary.csv` is placed there by hand.

`inventory.py` also writes a superseded, path-bearing `CORPUS_INVENTORY.md` into its `--output-dir`; the copy
written by `render_corpus_inventory.py` in the last step is the publishable one.

The scripts are read-only with respect to the evidence corpus. `musx_semantics.py` uses the public symmetric
`score.dat` recoding algorithm documented in the MIT-licensed sibling denigma project and keeps decoded XML in
memory.
