# Legacy Finale MUS Reader

Legacy Finale MUS Reader is an exploratory C++ library for reading legacy Finale
`.mus` files and constructing [`musxdom`](https://github.com/rpatters1/musxdom)
documents.

The project is intentionally separate from musxdom. Legacy Finale MUS Reader is a
client of musxdom; musxdom does not depend on this library and contains no
legacy `.mus` decoding code.

## Status

This project provides an intentionally narrow initial reader. It creates an
empty musxdom document with a complete Finale 27 options pool, recovers the
banner-era MUS header, validates known container formats, and overlays a small
set of verified legacy values. It is not yet a complete score importer.

The initial implementation can:

1. Classify a file's probable Finale format era and byte order.
2. Decode the applicable container or compression layer.
3. Recover selected header values and legacy "other" records.
4. Create a complete musxdom options pool from a pinned Finale 27
   new-document-without-libraries baseline.
5. Overlay every legacy option value that has been confidently decoded.
6. Report which currently supported values were recovered and which remain
   synthesized defaults.

The importer will construct musxdom objects directly. It will not introduce a
second public document model.

## Repository layout

| Path | Purpose |
| --- | --- |
| `include/finale_mus_reader/` | Public C++ API. |
| `src/container/` | File headers, format classification, byte order, and compression. |
| `src/defaults/` | Pinned fallback data used to create structurally complete documents. |
| `src/import/` | Direct construction and overlay of musxdom objects. |
| `src/records/` | Wire-level decoding of globals, others, details, entries, and text. |
| `tests/evidence/` | Publishable controlled MUS, ETF, and MUSX fixtures. |
| `research/` | Format notes, experiment logs, inventories, and feasibility findings. |
| `scripts/` | Small reproducible analysis and maintenance tools. |

The historical feasibility study moved from musxdom with its public commit
history preserved. Its entry point is [research/README.md](research/README.md).

## Evidence and provenance

Research claims distinguish source-derived information from results verified
against `.mus`, ETF, MUSX, or controlled Finale output. The research notes
record provenance and confidence explicitly.

The repository includes a distilled option/global mapping derived from
authorized read-only inspection of historical PDK Framework versions. It
labels every unverified result accordingly; see
[research/LEGACY_OPTION_MAPPINGS.md](research/LEGACY_OPTION_MAPPINGS.md).

## Current format coverage

| MUS epoch | Current useful output |
| --- | --- |
| Finale 1.x–2.x pre-banner | Strong structural classification; complete default options; explicit warning that pool directories remain unresolved. |
| Finale 3.x–2000 uncompressed | Validated typed blocks, byte order, banner header, legacy other-record index, layer rest offsets, and selected music-spacing options. |
| Finale 2001–2006 DCL | Validated typed blocks, PKWARE DCL inflation, CRC-32, banner header, layer rest offsets, and selected music-spacing options. |
| Finale 2007–2012 zlib | Validated typed blocks, zlib inflation, CRC-32, byte order, and banner header; logical option overlays remain pending. |

Unknown banner-era framing still produces an empty options-complete document
and recovered header rather than being mislabeled as a known epoch.

## Default-document design

The library does not parse the full default document and then clear its score
pools. Instead, it inflates the embedded macOS baseline and assembles a small
EnigmaXML document in memory from:

- the complete `<options>` element;
- an empty header that is replaced with recovered MUS header data; and
- an explicit allowlist of option-like `<others>`, currently the four
  `layerAtts` elements.

musxdom's `DocumentFactory` constructs that reduced document once. This avoids
reparenting DOM objects and prevents fallback measures, staves, entries, text,
parts, layouts, or derived instrument state from leaking into the result.

The embedded byte array is generated only when its authoritative gzip resource
changes. Regenerate it with:

```bash
python3 scripts/generate_embedded_defaults.py
```

Use `python3 scripts/generate_embedded_defaults.py --check` in validation or CI
to verify that the committed generated files are current.

## Building

The default build fetches the pinned musxdom revision:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

For development with a sibling musxdom checkout:

```bash
cmake -S . -B build \
  -DFINALE_MUS_READER_MUSXDOM_SOURCE_DIR=../musxdom
```

The project requires C++20 and zlib. By default it fetches the pinned zlib 1.3.1
source and builds its static target. Set
`FINALE_MUS_READER_USE_SYSTEM_ZLIB=ON` to use an installed zlib instead.

When a parent build already provides `ZLIB::ZLIB`, `zlibstatic`, or `zlib`, the
reader reuses that target before considering either option. Its FetchContent
dependency is also named `zlib` and uses the same release and hash as denigma,
so a parent that fetches both projects builds zlib only once.

The library does not select, fetch, link, or enable an XML implementation.
`Reader::read` is templated on a concrete implementation of
`musx::xml::IXmlDocument`, in parallel with musxdom's `DocumentFactory`. Only
the tests fetch pugixml and instantiate the reader with its musxdom adapter.

Set `FINALE_MUS_READER_BUILD_TESTING=OFF` when consuming the library without
its tests.

## API

```cpp
#include <finale_mus_reader/reader.h>

// XmlDocument must derive from musx::xml::IXmlDocument.
auto result = finale_mus_reader::Reader::read<XmlDocument>("legacy_score.mus");
auto document = result.document;
const auto& report = result.report;
```

`ImportReport` identifies the selected epoch, byte order, source platform,
validated blocks, warnings, and the origin of each currently supported option
overlay. The physical record index and overlay layer are separate so additional
record mappings can be added without changing container decoding.

## License

Legacy Finale MUS Reader is available under the [MIT License](LICENSE).
