# Graphic file locators

**Covers:** `FileAlias`, `FileDescription`, `FilePath`, and MUSX-only locator scope.
**Read when:** Changing `src/import/others/file_path.cpp` or comparing graphic locator records.
**Confidence:** confirmed for the controlled layouts below; open for other locator flavors.

The shared TU is orchestrated by one `ImportFilePath` registry/timing entry. Each recovered
record creates a source-owned others object with its stored comparator and part identity.
Graphic assignments name the description; description `pathId` names the path. Comparator
renumbering across conversions is not yet characterized. No fallback file objects are copied.

## Verified representation

| Class | Fixed-row tag | Zlib class | Payload |
|---|---|---|---|
| `FileAlias` | `Fa` | `0x0089` | Low-word-first 32-bit byte count, then opaque bytes; final row padding excluded |
| `FileDescription` | `Fd` | `0x008a` | Six words, described below |
| `FilePath` | `Fp` | `0x008b` | NUL-terminated bytes concatenated across incidences |
| `FileUrlBookmark` | unlocated | unlocated | Neither `length` nor `urlBookmarkData` has a located legacy source |

Description words are `version`, `pathType`, `pathId`, signed `volRefNum`, and the two words
of signed `dirId`. The directory long follows container byte order. The version is a single
unsigned word despite the wider DOM property. Verified path-type values are 1 (`DosPath`),
2 (`MacFsSpec`), and 3 (`MacAlias`). Unknown discriminators retain the DOM default with
`Unmapped` provenance and a diagnostic. See the POSIX and URL-bookmark scope below.

Recovered fields report `LegacyMus`, except normalized alias bytes (`LegacyMusAdjusted`). Alias lengths are checked
against the available payload before copying. Malformed records produce diagnostics and no
object. Adjacent alias bytes are swapped to the DOM representation, independently of container
endianness; an odd final byte is retained. **Strong:** both linked Mac fixtures match their
companions byte-for-byte after this conversion (big-endian DCL and little-endian zlib).
The Windows fixture contains no alias, so it does not establish Windows blob byte order.
No OS resolution is performed.

Paths have no font. They use the shared fontless text conversion before Finale 2012 and
preserve Unicode-era text. **Weak:** all current path fixtures contain ASCII, so non-ASCII
path encoding and the Unicode boundary still need discriminating fixtures. See the shared
[text encoding rules](../container/text_encoding.md).

Evidence: `tests/evidence/F372/F372-page-graphic.mus` and `F372-measure-graphic.mus` with ETFs
and companions; private controlled source `mus-1402a29f106e2832` with ETF and companion;
`tests/evidence/F2006/F2006-linked-tiff.mus` with ETF and companion; and
`tests/evidence/F2012/F2012-graphics-types.mus` and private source `mus-c53758b97dfa5f7e` with companions.
These controlled sources
cover uncompressed big-endian, DCL in both byte orders, and little-endian zlib storage.
Details and conversion differences are in the [investigation](../../investigations/file_path.md).

## Remaining scope

Coda is not an unresolved recovery epoch for these classes: it had no graphic tool, so
graphic locator import is intentionally inapplicable. This feature-availability fact was
supplied by Robert Patterson on 2026-09-07.

- `FileUrlBookmark` (`length`, `urlBookmarkData`), `MacPosixPath`, and `MacUrlBookmark`
  are treated as MUSX out of scope by Robert Patterson's 2026-09-07 direction. This is a
  provisional scope decision, not proof of historical absence. No bookmark object is fabricated.
  During the eventual all-corpus run, explicitly inspect unknown path-type diagnostics,
  unhandled graphic-associated record families, and bookmark observations for contrary legacy
  evidence. Keep the bookmark surveyor and unknown-discriminator diagnostics active; reopen
  the scope if a legacy source supplies POSIX or bookmark data.
- Non-ASCII paths, big-endian zlib locators, large alias counts, and unlinked part locators
  need controlled source evidence. Synthetic tests exercise both byte orders and truncation.
- Embedded graphic paths, paired alias blobs (including length changes), `FileDescription.dirId`,
  and exactly `FileDescription.pathType: MacFsSpec -> MacAlias` or `DosPath -> MacAlias`
  may differ after conversion; the approved coverage rule marks these as Finale upgrade
  normalization. Source assignments resolve aliases by description ID and paths by the
  description's path ID. A locator also referenced by a linked or unresolved assignment is
  excluded. Linked locators must match.
- Companion-only `FileAlias` objects are expected upgrade synthesis only for source embedded
  DOS locators whose corresponding companion description uses `MacAlias`. Other unmatched
  aliases remain unclassified. `FileAlias` is not MUSX-only.
- Companion-only `FileUrlBookmark` objects are expected upgrade synthesis when a corresponding
  companion `FileAlias` exists. This implements the provisional MUSX-only scope; unpaired
  objects remain visible for investigation. Both rules require intact companion blob/length
  pairs and never hide differences in source-owned objects.
- Coverage compares `aliasHandle` and `urlBookmarkData` as whole byte vectors, including empty
  vectors, via an opaque `Value::Blob` leaf. Blob contents and declared `length` are separate
  comparisons; byte-array JSON rendering does not expand the blob into comparison leaves.
- Conversions to `DosPath` and other description discriminator differences remain unclassified.
  The directory-ID rule does not require another locator field to change simultaneously.
  The synthesis rules use `finale-upgrade-synthesis`, distinct from matched-value normalization.
