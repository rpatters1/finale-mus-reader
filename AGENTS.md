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

## Repository map

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
  `research/FORMAT_NOTES.md` before changing a decoder.
- `scripts/`: reproducible maintenance and corpus-read-only analysis tools.
- `private/`: ignored local evidence and path mappings. Never publish it.
- `third_party/`: permitted location for pinned, license-compatible codec code.

Some implementation directories may be absent until the initial CMake
scaffolding creates them.

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
- Keep record layouts and option mappings explicitly version-aware.
- Treat sharing, tag-specific fields, early directory spans, and other matters
  labeled open in the research as open.
- When evidence conflicts with an assumption, preserve the evidence and update
  the hypothesis rather than forcing the sample through an expected layout.

## Options fallback strategy

musxdom expects a structurally complete options pool. Implement fallback options
with this sequence:

1. Select the pinned Finale 27 macOS or Windows baseline explicitly, preferring
   a source-platform match when reliable.
2. Inflate the embedded gzip bytes with zlib and parse the raw EnigmaXML with
   the configured musxdom XML backend.
3. Seed or clone only the complete options pool into the imported document.
4. Overlay every confidently recovered legacy option value.
5. Leave absent, unknown, or unsupported values at the Finale 27 default.
6. Report recovered values separately from synthesized defaults.

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
