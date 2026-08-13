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

- `.agents/skills/survey-a-corpus/`: inventory a local `.mus` corpus and publish
  the results. Read this before running anything in `scripts/` against a corpus.
  It covers the conventions you must ask the user for, how survey results are
  namespaced per corpus, and the checks that keep local paths out of published
  files.
- `.agents/skills/survey-class-coverage/`: compare one class or related set of
  classes across a selected cohort of already-inventoried fixtures, optionally
  using ETF or Finale 27 companions. Read this for targeted decoder-coverage and
  upgrade-behavior studies; it keeps per-fixture observations private and
  publishes only aggregate results.
- `.agents/skills/implement-musxdom-class/`: research, draft, test, and refine
  recovery of one musxdom DOM class from legacy MUS records. Read this when the
  user asks to implement a class; it moves from small epoch samples to a narrow
  implementation and then hands broad validation to `survey-class-coverage`.

## Repository map

- `.agents/skills/`: procedures for agents working in this repository.
- `include/finale_mus_reader/`: public C++ API.
- `src/container/`: headers, format classification, byte order, compression,
  framing, checksums, and bounds validation.
- `src/defaults/`: embedded Finale-default resources and options seeding.
- `src/import/`: direct construction and overlay of musxdom objects.
- `src/records/`: version-aware wire-record decoding.
- `resources/defaults/`: authoritative Finale 27 New Document Without Libraries
  fallback resources for macOS and Windows.
- `tests/evidence/`: controlled, publishable MUS/ETF fixtures and provenance.
- `research/`: findings, confidence labels, provenance, evidence inventories,
  and open questions. Start with `research/README.md` and
  `research/FORMAT_NOTES.md` before changing a decoder. Survey results are
  registered in `research/data/surveys.csv`, one row per surveyed corpus.
- `scripts/`: reproducible maintenance and corpus-read-only analysis tools.
  Location-agnostic by design: every path is a CLI argument, so corpus-specific
  conventions belong in the invocation and never in the code.
- `private/`: ignored local evidence and path mappings. Never publish it.
  Results are namespaced per surveyed corpus: conventions in
  `private/corpora/<survey_id>.conf`, script output in
  `private/generated/<survey_id>/`, fixtures in `private/evidence/<survey_id>/`.
  `private/regenerate.sh <survey_id>` rebuilds one corpus, `--all` rebuilds every
  configured one.
- `third_party/`: permitted location for pinned, license-compatible codec code.

Some implementation directories may be absent until the initial CMake
scaffolding creates them.

## Nothing is implemented more than once

In library and reader code, every fact and every behaviour has exactly one
implementation. A second copy is a defect even when both copies are currently
correct, because the two will diverge and the divergence will be silent.

This is close to absolute. Treat an exception as needing extraordinary
justification, stated in a comment at the site, rather than as a judgement call.
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

Doxygen comments document the contract, not its history. State what a caller must know:
behavior, parameters, return values, and what is thrown. Do not explain why a function or
property was added, what it replaced, or which investigation prompted it. A reader arriving at
the published API has none of that context. This constrains Doxygen only; ordinary
implementation comments in this repository are expected to carry that reasoning, because the
evidence behind a mapping is the thing hardest to reconstruct later. The same rule applies to
Doxygen written in musxdom.

This applies to `src/`, `include/`, and `tests/evidence/` fixtures. It does not
apply to `scripts/`, `tools/`, or test code, which may repeat themselves as
freely as makes sense — a probe is meant to be written quickly while a question
is live, and a test that spells out its own expectations is clearer than one that
shares a helper with the code under test.

## Format and decoder rules

Legacy MUS is a family of formats, not one stable binary layout. Classification
must use observed framing, byte order, lengths, and checksum/codec validation;
do not select a decoder from a marketing version alone.

Current broad eras are:

- Finale 1.x-2.x: pre-banner framing with unresolved directory spans.
- Finale 3.x-2000: uncompressed typed pools and platform-dependent byte order.
- Finale 2001-2006: PKWARE DCL-compressed typed blocks. A normal zlib install
  does not provide this decoder; use a pinned Mark Adler `contrib/blast`
  implementation with its original zlib license notice.
- Finale 2007-2012: zlib-related typed blocks, including transition-era byte
  order variation.

Keep physical framing separate from logical record interpretation. In
particular, the verified legacy other/detail rows are 16 **bytes**, not 16
words, and multi-incidence rows may form one logical object.

- Make all binary reads bounds-checked and overflow-safe.
- Preserve original offsets, raw values, selected byte order, format era, and
  confidence/provenance in diagnostics where useful.
- Validate declared sizes, decompressed sizes, CRCs, and complete input/output
  consumption as appropriate to the era.
- **Where the data or the record structure states which layout a file uses, read that instead of
  dating the file.** This outranks both gates below, and not as a stylistic preference: version
  coverage is incomplete and always will be. No Finale 3.3 or 3.4 document exists in either
  surveyed corpus, so a boundary in that window can only be guessed at; the Coda-banner era's
  Windows documents state no version at all; and a version read without the container's byte
  order gives a plausible wrong answer rather than no answer. A structural marker is also one
  step closer to the evidence, because a version boundary is usually inferred from the same
  observation the structure makes directly. Two mappings already work this way: the clef tuple
  width comes from the payload size, and the whole stem family's units come from the size of its
  connection collection. Say at the site why the marker is trustworthy, and what it costs when
  the file is ambiguous.
  A test over unbounded content is not a marker but a heuristic: font names are whatever a user or system could install, so no rule distinguishes a header incidence from the first bytes of every possible name, and that boundary rightly stays a version range. A marker must be a fact about the record's shape, not a guess about its contents.
- Keep record layouts and option mappings explicitly version-aware, but **prefer an epoch gate
  to a version gate wherever the boundary is really the epoch.** A version gate is the more
  fragile instrument: it fails closed on any file whose version cannot be recovered, and it
  fails silently, leaving the class populated from reference defaults and every field reported
  as synthesized. That looks exactly like a document with nothing to recover. The Coda-banner
  era's Windows documents state a platform where its Mac documents state a version, so they
  have no version at all and a version-gated table skips them without a word.
- **Where a version gate is genuinely required, frame it inside the epoch it is gating.** A
  boundary that falls within one epoch — the font-definition header arriving in Finale 3.2,
  inside the uncompressed era — belongs to that epoch and should be expressed as a version
  range on a table already restricted to it. Listing extra epochs alongside it can only ever be
  satisfied by a misread version, and bounding the range at both ends keeps a wild version from
  landing in the wrong case.
- Treat sharing, tag-specific fields, early directory spans, and other matters
  labeled open in the research as open.
- When evidence conflicts with an assumption, preserve the evidence and update
  the hypothesis rather than forcing the sample through an expected layout.

## Legacy text encoding

Legacy MUS stores text in whatever encoding the machine that saved it used; EnigmaXML and
musxdom are always UTF-8. Converting between the two is this project's job and not
musxdom's, which is why `src/import/text_encoding.*` exists here.

The encoding is named per font rather than per document: `charsetBank` selects the
platform's charset numbering and `charsetVal` selects within it, so a Mac font in a document
saved on Windows still decodes correctly. Before Finale 3.2 the font record carries no
charset at all, and the bank is synthesized from the document's own platform instead.

**Aim for the best result obtainable on the machine that is running.** Conversion need not
be bit-identical across platforms, and insisting on that would mean giving up real accuracy:
Windows can name encodings iconv cannot, so it gets the more faithful code page rather than
being held to a common subset. Where a platform must fall back, say so next to the fallback
and record what evidence shows the fallback is adequate.

Choices that no observed file settles are starting positions, not findings. Label them as
such and revise them when a file demands it, rather than defending them.

## Options fallback strategy

musxdom expects a structurally complete options pool. Implement fallback options
with this sequence:

1. Select the pinned Finale 27 macOS or Windows baseline explicitly, preferring
   a source-platform match when reliable.
2. Inflate the embedded gzip bytes with zlib and parse the raw EnigmaXML with
   the configured musxdom XML backend.
3. Seed or clone only the complete options pool into the imported document.
4. Overlay every confidently recovered legacy option value.
5. Leave absent, unknown, or unsupported values at the Finale 27 default. Prefer this even
   where the source era's behaviour is known, whenever the baseline already carries the value
   that behaviour implies. The baseline is generated from committed resources whose hashes
   this document records, so it is effectively as fixed as a constant, and a value asserted
   in code beside a baseline that already agrees is a second copy of the same fact.
6. Report recovered values separately from synthesized defaults, and separately again from
   values determined by how the source version behaved when it had no option to store them.
   `ValueOrigin` names the three: `LegacyMus`, `LegacyBehavior`, and `Finale27Default`.
   Reserve `LegacyBehavior` for a value the baseline does **not** already supply, or supplies
   wrongly — an era that always did something later versions let you turn off, where reading
   the later location would assert the opposite.

Never leak fallback measures, staves, entries, text, document identity, header
values, or other score content into an imported document. The fallback document
must not remain the owner of options placed in the imported document.

The CSV files under `research/data/` are evidence, not runtime schemas. Promote
only validated mappings into versioned compiled or generated constant tables;
do not make the library load research CSV files at runtime.

## Embedded default resources

Keep the raw `.enigmaxml` files as authoritative, inspectable source artifacts.
They are Finale-generated files with intentional CRLF endings; preserve their
exact bytes even though `git diff --check` reports carriage returns as trailing
whitespace. Use the committed deterministic `gzip -n -9` files as the inputs to
the resource-generation script.

Generate and commit the C++ byte arrays under `src/defaults/` with
`scripts/generate_embedded_defaults.py`; do not generate them during a normal
CMake build and do not edit them by hand. Run the script with `--check` to detect
stale output. Do not wrap the XML in ZIP or MUSX containers. Tests should cover
inflation, expected byte counts or hashes, platform selection, musxdom parsing,
and the presence of required option instances.

Expected SHA-256 values:

| Resource | SHA-256 |
| --- | --- |
| macOS EnigmaXML | `cebcc5af8d625979e1baa11c7350a1fc1cbb8475c776bdb5c34aea059e9a9120` |
| macOS gzip | `c58e69ab810451f7b295b3fe1e5545f9e1dd9d064b10e84c9253fe7a90a1ff66` |
| Windows EnigmaXML | `b151b38bd48580db7dd64a73b1364323936391abb19d74e424f27d35070fd2cb` |
| Windows gzip | `745444c37c44c13b17c72e1c6aad9f05e3e04ac2ab04bce027a2f55850201a5f` |

## Evidence, privacy, and provenance

Use the confidence vocabulary already established by the research documents:
`confirmed`, `strong`, `weak`, and `open`. Clearly separate source-derived facts
from independently corpus-verified behavior.

- Public notes use stable content-derived aliases such as `corpus_id`; never add
  private survey paths, drive names, archive locations, or directory layouts.
- Keep local corpus mappings and unpublished evidence under ignored `private/`.
- Controlled public evidence belongs under `tests/evidence/` with provenance and
  hashes where appropriate.
- Analysis scripts must remain read-only with respect to the evidence corpus.
- Treat later Finale/MUSX conversions and ETF exports as semantic references;
  resaving may normalize, synthesize, reorder, or expand records.
- Public PDK facts must cite an immutable public URL and access date, use the
  project's own terminology, and remain labeled `public-PDK-derived` until
  independently verified.
- Authorized local PDK Framework history is read-only. Do not copy its source,
  declarations, comments, or implementation into this repository or musxdom.
  Record only independently useful interoperability facts, numeric mappings,
  behavior, caveats, and provenance. Label such facts
  `private-framework-derived` until independently verified.

Do not make ownership claims about historically mixed PDK/framework material.

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
- Follow the surrounding musxdom C++ conventions where this repository has not
  yet established a local style.
- Match musxdom's code naming: use `camelCase` for methods, properties, and
  variables, and `PascalCase` for classes and enums. Match denigma's filename
  convention by using `snake_case` rather than kebab-case for new source files.
- Begin every project-owned C++ header and source file with
  `Copyright (c) 2026 Robert G. Patterson` and the SPDX identifier `MIT`.
  Preserve original copyright and license notices in third-party sources.
- Use explicit nested namespace blocks rather than concatenated namespace
  declarations.
- Do not require `NOMINMAX`. Protect standard-library `min` and `max` tokens
  from the Windows macros with parentheses, such as `(std::min)(a, b)` and
  `(std::numeric_limits<T>::max)()`.
- Compile every C and C++ object with `/bigobj` under MSVC. Keep this as a
  directory-wide build invariant so template-heavy musxdom factory
  instantiations cannot exceed the default COFF section limit in any target.
- Compile every object with `/utf-8` under MSVC, also directory-wide. Sources
  carry UTF-8 string literals and no byte-order mark; without the flag MSVC reads
  them in the machine's active code page and silently produces different bytes.
- Keep every project-owned translation unit unity-build clean even though unity
  builds are not the default. CMake may combine unrelated source files into one
  translation unit, so an anonymous namespace does not make file-local names
  unique after amalgamation; use distinctive names for aliases, helpers, and
  constants when needed. Do not let unity-only fixes change runtime behavior.
- Periodically smoke-test this invariant in a fresh temporary build directory
  with `-DCMAKE_UNITY_BUILD=ON` and the normal test suite. The smoke test applies
  to project-owned targets; external dependencies retain their own build policy.
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

For a unity-build smoke test, use a separate temporary build directory and retain
the same local musxdom source override, for example:

```bash
cmake -S . -B /tmp/finale-mus-reader-unity -G Ninja \
  -DCMAKE_UNITY_BUILD=ON \
  -DFINALE_MUS_READER_BUILD_TESTING=ON \
  -DFINALE_MUS_READER_MUSXDOM_SOURCE_DIR=../musxdom
cmake --build /tmp/finale-mus-reader-unity
ctest --test-dir /tmp/finale-mus-reader-unity --output-on-failure
```

This runs against the fetched zlib, not an installed one, so the smoke test covers the
default dependency configuration. **A unity build is this project's invariant and must never
be imposed on code this repository does not own.** `CMakeLists.txt` states that rule once, as
`finale_mus_reader_keep_out_of_unity()`, and calls it at each dependency: the zlib targets,
musxdom, pugixml, Catch2's companion target, and the pinned `blast` source. zlib is the
standing example of why -- its `inftrees.h` is included by several of its own sources and is
not idempotent -- but the rule is about ownership, not about zlib, and a dependency added
later is opted out by calling that function rather than by remembering a policy written
elsewhere. Catch2 itself is the one deliberate exception: `tests/CMakeLists.txt` amalgamates
it in *both* configurations for build speed, which is a choice about one dependency rather
than a consequence of the flag.

That leaves `-DCMAKE_UNITY_BUILD=ON` a single-flag test of project-owned translation units.
Verify it by configuring the smoke build and confirming that no dependency target has a
`Unity/` source: only `finale_mus_reader`, `finale_mus_reader_tests` and the deliberate
Catch2 should appear.

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
