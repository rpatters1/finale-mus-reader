# AGENTS.md

Guidance for coding agents and contributors working in this repository.

## Project purpose and boundaries

Legacy Finale MUS Reader is an MIT-licensed C++ library for reading legacy
proprietary Finale `.mus` files and constructing a `musxdom::Document`.

- This repository is a client of `musxdom` and must remain separate from it.
- Construct musxdom objects directly. Do not add a second public document model.
- Do not add legacy MUS decoding code to musxdom.
- Do not implement MUSX package extraction here. musxdom already accepts raw
  EnigmaXML through `DocumentFactory`.
- Do not bundle Finale fonts, Finale libraries, or proprietary source material.
- Keep the importer removable as an independent component if provenance or
  licensing concerns change.

The project is currently at the research and initial-implementation stage.
Implement narrow, verified vertical slices and do not present partial format
coverage as a universal reader.

## Skills

Task-specific procedures live in `.agents/skills/<name>/SKILL.md`, kept
tool-neutral so any agent can use them. Read the relevant one before starting
that task rather than reconstructing the procedure:

- `.agents/skills/inventory-a-corpus/`: discover or rediscover what a local
  corpus contains and regenerate its private inventories, mappings, caches, and
  sanitized public aggregates. Read this before inventorying a corpus or
  refreshing `private/generated/<survey_id>/`; it does not run reader coverage.
- `.agents/skills/analyze-recovery-coverage/`: run
  `recovery_coverage_probe` over already-inventoried corpora and analyze its
  JSONL snapshot, including repeated class, field, regression, and Finale-upgrade
  studies that should not rerun the probe.
- `.agents/skills/implement-musxdom-class/`: research, draft, test, and refine
  recovery of one musxdom DOM class from legacy MUS records. Read this when the
  user asks to implement a class; it moves from small epoch samples to a narrow
  implementation and then hands broad validation to
  `analyze-recovery-coverage`.
- `.agents/skills/maintain-documentation/`: the rules for changing anything under `research/`.
  Read this before adding a finding, recording an experiment, updating status, or creating a
  research document.
- `.agents/skills/comment-production-code/`: what belongs in a code comment and what belongs in
  the research documents instead.

## Documentation

Read `research/ORIENTATION.md`, `research/STATE.md`, and `research/INDEX.md` at the start of a
session. That is the whole startup set.

**Do not read `research/` recursively, and do not preload `research/format/`,
`research/investigations/`, `research/history/`, `research/reference/`, or
`research/corpora/`.** Those hold the detailed reference notes, the experiment history, and the
corpus inventories; open one file when the task calls for it.

The naming rule that locates a class's notes from its source file, and the rest of the navigation
contract, are stated once in `research/ORIENTATION.md`. `research/INDEX.md` maps every other
subject to a file.

Before editing any documentation, use the `maintain-documentation` skill.

## Repository map

- `.agents/skills/`: procedures for agents working in this repository.
- `include/finale_mus_reader/`: public C++ API.
- `src/container/`: headers, format classification, byte order, compression,
  framing, checksums, and bounds validation.
- `src/defaults/`: embedded Finale-default resources and options seeding.
- `src/import/`: direct construction and overlay of musxdom objects.
- `src/reader/`: reader orchestration and document-construction coordination.
- `src/records/`: version-aware wire-record decoding.
- `resources/defaults/`: authoritative Finale 27 New Document Without Libraries
  fallback resources for macOS and Windows.
- `tests/evidence/`: controlled, publishable MUS/ETF fixtures and provenance.
- `research/`: findings, confidence labels, provenance, evidence inventories,
  and open questions, organized for retrieval rather than for reading through.
  See **Documentation** above. Survey results are registered in
  `research/data/surveys.csv`, one row per surveyed corpus.
- `scripts/`: reproducible maintenance and corpus-read-only analysis tools.
  Location-agnostic by design: every path is a CLI argument, so corpus-specific
  conventions belong in the invocation and never in the code.
- `private/`: ignored local evidence and path mappings. Never publish it.
  Results are namespaced per surveyed corpus: conventions in
  `private/corpora/<survey_id>.conf`, script output in
  `private/generated/<survey_id>/`, fixtures in `private/evidence/<survey_id>/`.
  `private/regenerate.sh <survey_id>` rebuilds one corpus, `--all` rebuilds every
  configured one.
- `third_party/`: permitted location for pinned, license-compatible codec code. A tool that
  vendors a file verbatim keeps it in its own `third_party/` beside the code that uses it,
  as `tools/coverage/third_party/` does; either way the contents are not ours to edit.

Some implementation directories may be absent until the initial CMake
scaffolding creates them.

## Nothing is implemented more than once

Every line of code is a liability that must be checked and maintained in perpetuity. Prefer
solutions that centralize reuse and avoid boilerplate without introducing overly complex
abstractions. A change with more red deletions than green insertions is a win. Every line of
code should earn its place.

In library and reader code, every fact and every behavior has exactly one
implementation. A second copy is a defect even when both copies are currently
correct, because the two will diverge and the divergence will be silent.

This is close to absolute. Treat an exception as needing extraordinary
justification, stated in a comment at the site, rather than as a judgment call.
Duplication is not paid for by being convenient, by being small, or by the copies
being far apart — distance makes it worse.

In practice:

- If a constant, a spelling, a table, or a rule is needed in two places, give it
  one home and include it. Do not restate it, not even in a `case` label.
- If musxdom already implements something, call it. Do not reimplement it here,
  and do not restate the values it is built on.
- When a fact must be recorded twice for humans, record it once as code and once
  as prose that points at the code. A comment is documentation, not a second
  implementation.
- Before adding a literal, grep for it. Three separately hardcoded spellings of
  the product banner are what let Finale 1.0.0 go unread: the survey scripts
  learned the third spelling and the reader did not, because the reader had two
  copies and only one was updated.

Comments in `src/` and `include/` state how the code works, and how the format is believed to
work where belief is all there is, with the confidence labeled. They do not carry how a
behavior was derived, which fixture or survey established it, or what was believed before. That
material belongs in the class's file under `research/format/`, its experiment history under
`research/investigations/`, and the fixture `provenance.txt` files. See the
`comment-production-code` skill.

Doxygen comments document the contract, not its history. State what a caller must know:
behavior, parameters, return values, and what is thrown. Do not explain why a function or
property was added, what it replaced, or which investigation prompted it. A reader arriving at
the published API has none of that context. The same rule applies to Doxygen written in
musxdom.

The rule also applies across the recovery-coverage probe/report pair, and it does not apply to
the rest of `tools/`, `scripts/`, or test code. The exact scope is in
[`research/reference/duplication_scope.md`](research/reference/duplication_scope.md).

## Rules that live in the research tree

These are binding project rules, kept next to the material they govern so there is one copy of
each. Read the relevant one before the work it covers, not at session start.

| Rule | Read before |
|---|---|
| [`research/reference/decoder_rules.md`](research/reference/decoder_rules.md) | Writing or changing a layout gate, or any binary read. Structural markers outrank version gates; epoch gates outrank version gates. |
| [`research/format/container/text_encoding.md`](research/format/container/text_encoding.md) | Any text or symbol conversion. Never re-encode pre-Finale-2012 text without that text's own font. |
| [`research/reference/options_fallback.md`](research/reference/options_fallback.md) | Adding an option overlay or reporting a `ValueOrigin`. |
| [`research/reference/embedded_defaults.md`](research/reference/embedded_defaults.md) | Touching `src/defaults/`, `resources/defaults/`, or the resource generator. |
| [`research/reference/CITING_EVIDENCE.md`](research/reference/CITING_EVIDENCE.md) | Writing any finding, or publishing anything derived from a corpus or from PDK material. |
| [`research/reference/build_invariants.md`](research/reference/build_invariants.md) | Changing CMake or adding a dependency. |
| [`research/reference/code_conventions.md`](research/reference/code_conventions.md) | Writing or reviewing any project-owned C++ source file. |
| [`research/reference/duplication_scope.md`](research/reference/duplication_scope.md) | Deciding whether a repetition in `tools/`, `scripts/`, or tests is a defect. |

The CSV files under `research/data/` are evidence, not runtime schemas. Promote only validated
mappings into versioned compiled or generated constant tables; do not make the library load
research CSV files at runtime.

## Build, code, and tests

The build uses CMake. Keep these properties intact when extending it:
- Provide a normal CMake library target and deliberate musxdom dependency
  integration suitable for downstream clients.
- Require C++20. `std::format` is permitted where it improves clarity.
- Require zlib for gzip and zlib-era decoding. Reuse a zlib target supplied by
  a parent project, otherwise fetch the pinned source by default; retain the
  option to use an installed system zlib.
- Keep the library independent of XML implementations. Do not select, fetch,
  link, or enable pugixml, RapidXML, TinyXML2, QtXML, or another backend for the
  library target. Accept a concrete `musx::xml::IXmlDocument` implementation
  through the public reader template, parallel to musxdom's `DocumentFactory`.
  Tests may fetch and instantiate pugixml without propagating its backend
  definition to the library.
- Keep committed generated resource sources synchronized with their gzip inputs
  and verify them with `scripts/generate_embedded_defaults.py --check`.
- Follow the surrounding musxdom C++ conventions where this repository has not yet established a
  local style. Naming, file headers, namespaces, preprocessor comments, Windows macro safety, the
  MSVC directory-wide flags, and unity-build cleanliness are in
  [`research/reference/code_conventions.md`](research/reference/code_conventions.md).
- Keep public APIs small and keep wire-format details out of public interfaces
  unless callers need them for diagnostics or capability reporting.
- Route project-owned runtime warnings and errors through musxdom's logging
  channel; do not write them directly with `fprintf` or to stderr. Unmodified,
  inactive test harnesses in pinned third-party sources are exempt.

Add focused tests with every implemented format fact. Prefer controlled fixtures
and exact byte/offset assertions. Include malformed and truncated inputs for
container and decompression work. Run the relevant build and tests before
finishing; until CMake exists, run affected scripts directly and perform at
least syntax checks such as `python3 -m py_compile scripts/<script>.py`.

For a local sibling musxdom checkout, configure and test with:

```bash
cmake -S . -B build \
  -DFINALE_MUS_READER_MUSXDOM_SOURCE_DIR=../musxdom
cmake --build build
ctest --test-dir build --output-on-failure
```

Unity compilation is a project-owned-target invariant and must never be imposed on code this
repository does not own; a dependency is opted out by calling
`finale_mus_reader_keep_out_of_unity()` at its site. The rationale, the MSVC directory-wide flags,
and the one deliberate Catch2 exception are in
[`research/reference/build_invariants.md`](research/reference/build_invariants.md).

Do not modify large evidence or default fixtures unless the task requires it.
If exact-source files change intentionally, verify and document their hashes and
provenance.

## Git hygiene

- Never commit or push directly to `main`. Work on a feature branch and deliver
  changes through a pull request.
- Do not push a feature branch, create a pull request, or commit unless the user
  requests that action.
- Preserve unrelated user changes in a dirty worktree.
- Keep implementation changes focused; avoid unrelated cleanup.
- Before handing off, inspect `git status --short` and `git diff --check`. Note
  that the authoritative CRLF EnigmaXML resources are the known exception to
  `git diff --check` whitespace reports.
