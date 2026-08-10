# Citing Evidence

Most evidence for this format sits in private collections that no one else will
ever open. A finding is therefore only useful if it travels without its files:
it must say what was observed, how broadly, and against what — so a later reader
can weigh it, and anyone who does hold the file can check it.

This describes how findings cite evidence and how confidence is assigned. The
confidence vocabulary itself is defined in `AGENTS.md`: `confirmed`, `strong`,
`weak`, `open`.

## Evidence tokens

Cite evidence by a stable token, never by a path.

| token | what it is | who can re-check it |
|---|---|---|
| `tests/evidence/<dir>/<file>` | a fixture committed to this repository | anyone |
| `mus-<16 hex>` | a legacy source, by content hash | whoever holds that corpus |
| `arc-<16 hex>` | an archive, by content hash | whoever holds that corpus |
| `<survey_id>` | a whole survey, for aggregate claims | whoever holds that corpus |

`mus-` and `arc-` tokens are derived from file contents, so the same file cited
by two surveys carries the same token. That is the point: agreement between
independent collections is the strongest signal available for a format whose
corpus can never be assembled in one place.

Resolve a token through `data/surveys.csv` to find which surveys observed it,
then through that survey's `data/corpus_manifest.csv` for its size, hash, and
saving product.

Structure is published in aggregate, not per file: `RECORD_CATALOG.md`,
`data/record_correlations.csv`, and `data/version_structure_summary.csv`. A
per-file structural dump was written and then withdrawn — record types, size
distributions, and element counts for every file amount to a detailed portrait
of a person's life's work, which is more than a survey needs to give up to be
useful. A finding therefore cites which files it rests on and how many, and
whoever holds the corpus can re-derive the detail.

Filenames are never published. A filename can name a work, a client, or a
person, and it identifies nothing that the content hash does not identify
better. Whoever holds the corpus resolves an id to a file through their own
ignored `corpus_locations.csv`.

## Writing a finding

State the claim, its confidence, and the evidence line. For example:

> Detail records in 2007-era blocks carry a four-byte zero trailer.
> **Confirmed.** Observed in 411 framed files across `rpatters1-main`;
> reproducible against `tests/evidence/F2005/F2005-baseline.mus`.

An evidence line answers three questions: how many files, in how many
independent surveys, and whether any publicly held fixture demonstrates it.

## What confidence requires

Confidence tracks how checkable a claim is, not how sure its author feels.

- **`confirmed`** — demonstrable against a fixture in `tests/evidence/`, or
  observed consistently in two or more independent surveys. Anyone can verify
  it, or two collections that never met agree.
- **`strong`** — consistent across many files in a single survey, with no
  contrary observation, but not independently reproduced. Most corpus-wide
  statistics land here, and they should not be written as settled fact.
- **`weak`** — a small sample, one era, or a pattern that could plausibly be
  coincidence.
- **`open`** — a hypothesis with a stated test that has not been run.

A claim cannot be `confirmed` by volume alone. One person's ten thousand files
are still one collection, one filing habit, and one version history.

## Contrary findings

A survey that contradicts an existing claim is contributing the most valuable
thing it can. Record the disagreement; do not resolve it by editing.

1. **Never delete or rewrite the original claim**, and never edit another
   survey's published data.
2. Add the contrary observation beside the claim, with its own evidence line.
3. Lower the confidence to reflect the conflict — a `confirmed` claim with a
   credible counter-observation is no longer `confirmed`.
4. If the conflict is explainable, say so and keep both observations. Two eras
   behaving differently is a finding; a survey being wrong is also a finding,
   and which one it is may not be knowable yet.

Format eras are the usual explanation. Before recording a contradiction, check
whether the two observations actually cover the same `saving_product` range —
legacy MUS is a family of formats, and a claim that holds for 2007+ blocks may
be simply inapplicable to a 1997 file rather than contradicted by it.

## Donated fixtures

A fixture in `tests/evidence/` outranks any amount of corpus statistics, because
it makes a claim checkable by everyone forever. Contributors who own their
material may donate files; see `.agents/skills/survey-a-corpus/SKILL.md` for how
that is solicited and recorded.
