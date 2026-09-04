# Current state

Compact working state. Every line links to its detail; none of the analysis lives here.
[`state/`](state/) holds the long-form versions and is the authority when they disagree.

## What is implemented

- **55 done, 2 partial, 133 to do, 190 registered musxdom classes** (verified against the registry
  2026-09-04). Options are complete: 28 of 28. Per-class status:
  [`state/MUSXDOM_CLASS_COVERAGE.md`](state/MUSXDOM_CLASS_COVERAGE.md).
- Container classification, byte-order trialling, and framing work for all four epochs; DCL and
  zlib blocks validate against stored CRC-32.
- Others/details/texts recovered so far: font definitions, shape definitions, fret records,
  graphic assignments, custom smart-shape lines, marking-category staff lists, text blocks, part
  definitions, part globals, and the text pool classes.
- Score content — measures, staves, entries, and their details — is **not** imported.

## Current priorities

From [`state/PRODUCTION_READINESS.md`](state/PRODUCTION_READINESS.md), which records each item's
status and date:

- **P0.2 Remaining seeded option font ids are not reconciled** — blocker.
- **P1.1 Option coverage is a thin slice** — gap.
- **P1.2 No score content is imported** — gap; the largest single piece of remaining work.
- **P1.3 The zlib era decodes only supported record classes** — gap.
- **P1.5 Legacy text encoding is not converted** — gap.

P0.1, P0.3, and P1.4 are recorded as done or resolved.

## Open questions

- No byte-order marker has been found in the 3.x-and-later header; order is inferred from framing.
  The six bytes at `0x062` remain uninterpreted.
  ([`format/container/byte_order.md`](format/container/byte_order.md))
- The header text region's field sequence past `0x178` is `open`, which is what blocks excising
  document metadata by field rather than by scanning.
  ([`format/container/header.md`](format/container/header.md))
- Word order within 32-bit fields is an untested hypothesis.
  ([`format/container/record_rows.md`](format/container/record_rows.md))
- Coda-banner index and directory spans, and generic pool boundaries, are unresolved.
  ([`format/container/coda_banner.md`](format/container/coda_banner.md))
- Sharing is confirmed present but its pre-zlib encoding is unresolved; a controlled link/unlink
  test is the wanted experiment. ([`format/container/sharing.md`](format/container/sharing.md))
- Coda-banner byte order is asserted rather than detected, and Windows Finale existed then
  (P2.6 in [`state/PRODUCTION_READINESS.md`](state/PRODUCTION_READINESS.md)).

## Known gaps and regressions

- `LayerAttributes` is complete and agrees with its companion on every layer of every tracked
  document. A layer with no stored record takes its era's behavior, and playback and music
  spacing are supplied below Finale 2002, where neither setting exists.
  ([`format/others/layer_attributes.md`](format/others/layer_attributes.md))
- 94 residual `FontOptions` disagreements remain classified but unexplained (P3.2).
- Two companion differences are recorded rather than suppressed; the instrument errors behind them
  are worth reading before trusting a comparison script's field lookup.
  ([`investigations/regression_open_questions.md`](investigations/regression_open_questions.md))
- `BookmarkText` and `ExpressionText` recover in pooled eras only.

## Wanted evidence

Priorities and the full request list are in
[`state/EVIDENCE_REQUESTS.md`](state/EVIDENCE_REQUESTS.md); several requests there are marked
proposed rather than supplied, including the controlled link/unlink pair for sharing (C3), the
Finale 2.6.3 boundary pair (C4), and the option-map verification set (C6).

## Maintaining this file

When a question here is answered, the answer moves to the owning file under
[`format/`](format/), its evidence to [`investigations/`](investigations/index.md), and the bullet
leaves this file. See the `maintain-documentation` skill.
