# Legacy Finale MUS Reader

Legacy Finale MUS Reader is an exploratory C++ library for reading legacy Finale
`.mus` files and constructing [`musxdom`](https://github.com/rpatters1/musxdom)
documents.

The project is intentionally separate from musxdom. Legacy Finale MUS Reader is a
client of musxdom; musxdom does not depend on this library and contains no
legacy `.mus` decoding code.

## Status

This project is at the research and scaffolding stage. It does not yet provide
a complete `.mus` importer. Legacy Finale files span several binary format
eras, and support is expected to grow incrementally as individual structures
are verified.

The initial implementation target is a narrow vertical slice that can:

1. Classify a file's probable Finale format era and byte order.
2. Decode the applicable container or compression layer.
3. Recover selected header values and record pools.
4. Create a complete musxdom options pool from a pinned Finale 27
   new-document-without-libraries baseline.
5. Overlay every legacy option value that has been confidently decoded.
6. Report which values were recovered and which remain synthesized defaults.

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

## Building

The build system and library targets have not yet been added.

## License

Legacy Finale MUS Reader is available under the [MIT License](LICENSE).
