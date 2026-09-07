# Graphic file locator investigation

**Covers:** Selector discovery and source/companion differences for graphic file locators.
**Read when:** Extending the shared file-path importer or reviewing locator differences.
**Confidence:** confirmed controlled byte observations; open for unrepresented flavors.

## 2026-09-07: controlled records

Used `record_dump` for normalized source records, CR-normalized ETF lines for pre-zlib
semantics, and the existing `musx_semantics.decode_score_dat` for raw companion XML.
The durable layout is recorded once in [the format reference](../format/others/file_path.md).

- `tests/evidence/F372/F372-page-graphic.mus`: decoded others offset `0x13b0` has
  `Fd(1)` bytes `010000020001fffd00000010`. The ETF and companion identify version 256,
  MacFsSpec, path ID 1, volume -3, directory 16. The next two rows hold the TIFF filename.
- `tests/evidence/F372/F372-measure-graphic.mus`: `Fd(1)` has volume -1 and directory 57;
  its ETF path spans two rows. This supplies a measure-assignment locator independently
  of the page specimen.
- Private controlled source `mus-1402a29f106e2832`: decoded others offset `0x1640` has
  `Fd(1)` bytes `000101000100000000000000`. ETF and companion agree on DOS path type.
  The path occupies nine rows and contains 107 bytes before NUL. The Windows path retains
  its drive prefix and backslashes.
- `tests/evidence/F2006/F2006-linked-tiff.mus`: `Fa(1)` starts at decoded others offset
  `0x47f0`, bytes `01ce00000000000001ce0002`. ETF declares 462 alias bytes, locating
  the low-word-first length and four-byte prefix. The source alias starts
  `0000000001ce0002`; the companion alias starts `00000000ce010200`, showing pairwise
  byte swapping in this conversion. The initial raw-byte preservation policy is superseded
  by the normalized representation in the format reference.
- `tests/evidence/F2012/F2012-graphics-types.mus`: the six alias, description, and path
  records share comparators. The first alias declares 392 bytes. Descriptions have
  volume -100 and directory 341052, and the first stored display path is `GifSample.gif`.
  Raw companion XML instead contains 500-byte aliases and temporary `EnigmaTemp...` names.
  Therefore companion filenames and aliases cannot identify the original source bytes by
  direct equality. The companion also has six URL bookmarks that were not located in the
  source records inspected. No URL-bookmark selector is inferred from their presence.

The [public Finale 2000 headers in GRAME GUIDOLib](https://github.com/grame-cncm/guidolib/tree/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin)
(accessed 2026-09-07) were searched for `FilePathSpec`;
the consulted `edata.h`, `EEDDATA.H`, `EXTYPES.H`, `FINEXTND.H`, and `egraf.h` did not expose
that declaration. No PDK-derived numeric mapping is claimed. A newer public declaration
or a locator-specific controlled fixture would settle the remaining path-type hypotheses.

The first focused test run exposed unsigned widening of the volume word; explicitly
sign-extending it fixed the error across both byte orders. Tests also reject oversized
aliases and short class descriptions and verify that Coda data is not interpreted using
the later layout.

The initial description of Coda storage as merely unlocated is superseded by Robert
Patterson's feature-availability clarification; see the
[epoch scope](../format/others/file_path.md#remaining-scope). The existing exclusion is
unchanged, but it no longer represents missing recovery work.

## Tracked coverage before alias normalization (superseded)

The instrumented Release capture including `mus-c53758b97dfa5f7e` imported all 241 source
occurrences (239 distinct contents) and all 241 companions. This is the `tracked-evidence`
cohort only; private-corpus regression was not run. The new surveyors appear under `others`
in the maintained report. No locator payload warnings occurred.

Unclassified differences are concentrated in six distinct graphic sources. The DCL embedded
fixtures `mus-5ab602da3fff05cd`, `mus-4a1b5812b77c79dc`, and `mus-a71f66b69433c38d` account
for five description `pathType` changes from `MacFsSpec` to `MacAlias`; raw companion XML
confirms the new discriminator. Together with the zlib specimen `mus-80f8c631de435726`,
they account for eleven stored paths replaced with temporary names. These four sources and
the linked DCL specimen `mus-f3e9167821b468c4`, plus the linked zlib specimen
`mus-c53758b97dfa5f7e`, account for thirteen changed aliases and thirteen companion-only
bookmarks. The source origin of recovered fields is `LegacyMus`.

The report counts 4,587 differing alias leaves, 768 companion-only alias leaves, five
description differences, eleven path differences, and 14,410 companion-only bookmark leaves.
These are scalar/byte comparison counts, not document counts. They remain unexpected;
no classification rule was added. The Windows specimen `mus-1402a29f106e2832` preserves
both its description and its path. The remaining implementation/evidence scope is listed
in the [format reference](../format/others/file_path.md#remaining-scope).

The linked zlib addition matches all description and path values. It adds 470 differing
alias bytes (for example, offsets 10 and 11 exchange values 12 and 77, origin `LegacyMus`)
and 702 companion-only bookmark leaves, comprising the 700-byte blob, its length, and its
sharing mode. No other class has unexpected matched-value differences in this snapshot.

## Linked Finale 2012 discriminator

Private controlled source `mus-c53758b97dfa5f7e` independently confirms the linked zlib
representation. Its description at decoded others offset `0x1ab8` is
`0001030001005cffe5000000`: version 256, MacAlias, path ID 1, volume -164, directory 229.
The path at `0x1ad2` retains the same JPEG filename in both source and companion.
The alias payload occupies 600 bytes, declares 592 data bytes, and excludes four trailing
padding bytes after the count and blob. Swapping every adjacent byte pair of the entire
source alias produces the companion's 592-byte alias exactly.

Compared with `F2012-baseline`, newly present others keys belong to the three established
locator classes, two font definitions, and the page graphic assignment. No new bookmark
selector is exposed. Raw companion XML nevertheless contains a 700-byte URL bookmark.
This strengthens the observed linked-alias conversion behavior but does not establish a
legacy bookmark layout. The subsequent provisional MUSX scope decision and all-corpus
watch conditions are in the [format reference](../format/others/file_path.md#remaining-scope). A focused source
regression test initially covered the stored locator and absence of a fabricated bookmark;
it was removed when the fixture moved to the private corpus because of its SMB metadata.
Synthetic zlib tests retain alias byte-order and length-boundary coverage.

## Normalized reader and combined controlled evidence

After moving the path-bearing Windows specimen to the private controlled corpus, the
development capture includes 240 public occurrences (238 distinct contents) and one private
occurrence, `mus-1402a29f106e2832`. All 241 sources and companions imported successfully;
239 distinct contents were compared. The source, ETF, and companion hashes were preserved
through the move. Public tests no longer require the private specimen; synthetic records
retain DOS-discriminator coverage.

Both linked aliases now match exactly. Before whole-blob comparison, the report classified
3,979 embedded alias leaves and 11 embedded paths as upgrade normalization. The approved extension
also classifies the five embedded `MacFsSpec -> MacAlias` description changes. The converse
is represented by three linked sources retaining `MacFsSpec` and their filenames:
`F372-measure-graphic`, `F372-page-graphic`, and `F2006-linked-tiff`. The rule does not require
a simultaneous path or blob difference; embedding and the exact type transition define it.
The former 14,410 companion-only bookmark leaves represented 13 bookmarks, not 14,410 objects.
That per-byte counting is superseded by whole-vector comparison at Robert Patterson's request:
each bookmark contributes one blob, one length, and one sharing-mode leaf. Bookmarks remain
observable under the provisional out-of-scope decision. This controlled capture preceded the
all-corpus review below.
Focused tests include class-local comparison contexts with cross-class source referents supplied
separately, and reject linked, unresolved, mixed-use, and other-discriminator cases.

## All-corpus locator review

The first four-survey capture compared 16,338 occurrences (7,302 distinct sources): 16,249
imports succeeded, 58 LIB inputs and 31 non-MUS inputs were rejected, and all 4,840 attempted
companions imported successfully. Four matched-value discrepancies remained, all descriptions:

- `mus-bed39b5c1515d4f4`, descriptions 2 and 6: `dirId` 18115899 to 18115896.
  Companion XML explicitly stores 18115896.
- `mus-3837e439ea81eb51`, descriptions 1 and 2: `DosPath` to `MacAlias`, with two
  companion-only aliases. The source page assignments reference embedded graphics 1 and 2;
  stored DOS paths are temporary filenames. Companion XML stores `macAlias` and embedded
  page references. No private filename or personal path is published here.

Robert Patterson approved embedded directory-ID changes and the additional DOS-to-MacAlias
transition, but explicitly deferred changes toward DOS paths. These extend the existing
normalization category. At this stage, companion-only aliases remained unclassified objects;
the platform experiment below supersedes that interpretation for embedded DOS conversions.
The authorized all-corpus refresh supersedes the pre-extension snapshot: the selection and
import totals are unchanged, all four discrepancies classify as expected, and no unexpected
matched-value differences remain. Two companion-only aliases remain visible (six leaves),
as do 237 companion-only bookmark occurrences (711 leaves). No unknown locator-discriminator or malformed locator-payload
warnings occurred; this does not establish historical absence of unlocated POSIX/bookmark data.

## Windows versus Mac companion synthesis

Robert Patterson supplied a separate WinFin27 companion for `mus-3837e439ea81eb51`.
Direct companion XML inspection finds two `DosPath` descriptions and no `FileAlias` or
`FileUrlBookmark` objects in the Windows save. Its embedded temporary filenames change.
The Mac save instead has two `MacAlias` descriptions, two 500-byte `FileAlias` blobs,
and two `FileUrlBookmark` objects. The source contains no alias records. Neither source nor
companion-selection conventions were changed during this experiment.

This supports platform-conversion synthesis, not failed alias recovery. The approved rules
are specified in the [format reference](../format/others/file_path.md#remaining-scope).
They also classify a companion-only `FileUrlBookmark` paired with a companion `FileAlias`,
consistent with the provisional MUSX-only scope. No claim is made that `FileAlias` itself is
MUSX-only. The pre-synthesis all-corpus snapshot is stale; these new classifications require
fresh authorization before another all-corpus capture.
