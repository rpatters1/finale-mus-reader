# Production Readiness

Everything that must be accounted for before the reader can be presented as a
production legacy MUS importer rather than a verified vertical slice. Items are
prioritized by the damage they do if left unaddressed, not by implementation
cost.

The confidence vocabulary is the project standard: `confirmed`, `strong`,
`weak`, `open`. "Confirmed" here means observed against this repository's code
or evidence on the date recorded, not merely reasoned about.

Status values: **blocker** (imported documents are wrong or unusable until
fixed), **gap** (imported documents are correct but incomplete), **decided**
(analyzed, with a recorded decision that needs no further work), **open**
(needs evidence before it can be prioritized).

---

## P0 — imported documents are incorrect or throw

### P0.1 Font definitions now come from the file

**Status:** done 2026-08-09. **Confidence:** confirmed.

Previously the reader created no `others::FontDefinition` at all, so every font reference
in the seeded options resolved to nothing and `FontInfo::getName` threw.

Font definitions are now decoded from the `FN` records of the source file for every
pre-zlib epoch: 12,759 definitions across all 669 Coda-banner, uncompressed, and DCL corpus
files, every one with a name. Character set bank and value, pitch, family, and name are all
populated from the file. The layout is recorded in
[FORMAT_NOTES.md](FORMAT_NOTES.md#font-definitions).

The zlib era still produces none, because its records are not decoded. See P1.3.

**Open: character set and font platform for files before Finale 3.2.** Those files carry
no header incidence, so the bank and character set value are absent from the record
entirely and the reader leaves them at their constructed defaults. Every font in the 54
Coda-banner and 28 Finale 3.0-3.7 documents is therefore reported as a Mac-bank font with
character set 0, which is a default rather than a reading. Recovering them will have to come
from somewhere other than the `FN` record.

### P0.2 Seeded option font ids are not reconciled

**Status:** blocker, and newly urgent. **Confidence:** confirmed.

This is the hazard that argued against seeding pinned font definitions, now arriving from
the other direction. The pinned Finale 27 options carry Finale 27 font ids, and real font
definitions from the source document now exist under the source document's ids. An option
the reader has not yet mapped therefore resolves its font id against a table it was never
written for, and returns whichever font happens to occupy that id.

The failure mode has changed from loud to silent. Before font definitions existed, such a
reference threw `std::invalid_argument`. It now returns a plausible, wrong font.

Reconciling means one of:

- recovering the legacy font preferences so the option values are themselves from the file,
  which removes the mismatch at its source; or
- pointing every unrecovered font reference at a designated source font and reporting it as
  synthesized, so nothing resolves by coincidence.

Until then, treat any font reference reached through a seeded option as unverified. Font
definitions themselves are trustworthy; the option fields that point at them are not.

### P0.3 Platform-matched baseline selection

**Status:** done 2026-08-09. **Confidence:** confirmed.

Previously only the macOS baseline was embedded, so every import seeded macOS
defaults even when the recovered banner said `WIN` and `recoverHeader` had set
`TextEncoding::Windows`.

Both baselines are now embedded, `scripts/generate_embedded_defaults.py` is
parameterized over them, and `defaults::parseDefault` selects on
`ImportReport::sourcePlatform`, falling back to macOS when the platform is
`Unknown`. `ImportReport::defaultsPlatform` reports the baseline actually used,
and the tests assert the macOS, Windows, and unknown-fallback paths.

Do not read significance into the specific values that differ between the two
committed baselines. They were captured on different machines, so differences
such as font ids and shape ids reflect each machine's installed resources, not a
platform behavior of Finale.

---

## P1 — imported documents are correct but substantially incomplete

### P1.1 Option coverage is a thin slice

**Status:** gap. **Confidence:** confirmed 2026-08-09.

Eight values are recovered: four `layerAtts.restOffset` and four
`MusicSpacingOptions` fields. `research/data/legacy_option_mappings.csv` holds
437 distilled field mappings and
`research/data/legacy_direct_option_blocks.csv` five direct blocks. Everything
not in that slice silently remains a Finale 27 default.

Measured coverage, from running the reader over all 1,218 direct corpus files:

| Epoch | Files | Music spacing 4/4 | Layer offsets 4/4 | Files with fonts | Fonts |
|---|---:|---:|---:|---:|---:|
| Coda banner | 54 | 0 | 0 | 54 | 5,119 |
| Uncompressed | 190 | 90 | 180 | 190 | 2,231 |
| DCL | 426 | 426 | 426 | 426 | 5,418 |
| zlib | 527 | 0 | 0 | 0 | 0 |
| Unrecognized framing | 20 | 0 | 0 | 0 | 0 |

Every file but one produced a document; the single failure is an AppleDouble
metadata artifact, which is the correct outcome. Every pre-zlib file recovers its
font table. Every framed DCL file recovers all eight of the promoted option
values.

The uncompressed shortfalls are era facts, not defects. Selector `94` does not
appear before Finale 2000, so Finale 3.2 through 97 recover layer offsets and no
spacing; the ten files without layer offsets are Finale 97 documents that carry no
`LA` records. Coda-banner files have neither selector, which is why that row is
zero. Per-era detail is in
[LEGACY_OPTION_MAPPINGS.md](LEGACY_OPTION_MAPPINGS.md#corpus-verification-of-promoted-mappings).

This measures recovery, not accuracy. Only the fixtures with ETF counterparts
confirm that recovered values are correct.

`ImportReport::fields` already distinguishes recovered from synthesized values,
which is what keeps this a gap rather than a correctness problem. Promotion of
further mappings should stay evidence-led and version-aware.

### P1.2 No score content is imported

**Status:** gap. **Confidence:** confirmed.

No measures, staves, entries, text, page or system layout, parts, or
instruments. The reader currently produces an options-complete document with a
recovered header. This is the honest current scope and is asserted by
`expectNoScoreContent`, but "MUS reader" will be read by users as meaning score
content.

### P1.3 The zlib era decodes no records

**Status:** gap. **Confidence:** confirmed 2026-08-09.

The Coda-banner half of this item is done. Its pools are decoded into typed
blocks, its others and details pools are indexed like every other epoch, and all
54 corpus files recover their font tables.

The 2007-2012 zlib era still overlays nothing: 527 files classify and inflate but
produce no records, so they receive no fonts and no option values. Its record
framing is undecoded, which is research rather than implementation, and it is now
the largest single block of unserved files.

Twenty of those files additionally log a zlib decompression failure during
inflation, which is a separate defect from the undecoded records.

### P1.4 Twenty zlib-era files are not framed

**Status:** gap. **Confidence:** confirmed 2026-08-09.

This item previously read "thirty-seven banner-era files are not framed" and
attributed them to known variant framings. That diagnosis was wrong twice over,
and both causes are now fixed:

- **Sixteen Finale 2001-2006 files** were not variant framings at all. Their tags
  were being read as raw bytes rather than as byte-order-sensitive values, so
  every tag in a little-endian file mismatched. Every framed DCL file in the
  corpus now recovers all eight promoted values.
- **One Finale 97 file** overran the customary `0x200` body boundary with long
  file-info text. The body offset is a header field, not a constant. See
  [FORMAT_NOTES.md](FORMAT_NOTES.md#banner-era-files); the controlled pair under
  `tests/evidence/F97/` is the evidence and the regression test.

Twenty files remain unrecognized, all of them zlib-era. They are covered by P1.3
rather than by anything specific to framing.

### P1.5 Legacy text encoding is not converted

**Status:** gap. **Confidence:** strong.

`recoverHeader` records `TextEncoding::Mac` or `TextEncoding::Windows`, which
declares the source encoding but converts nothing. Legacy documents carry
Mac OS Roman or Windows code page text, not UTF-8. Any future text import needs
a conversion step, and the declared encoding must agree with the selected
baseline (see [P0.3](#p03-platform-matched-baseline-selection-is-not-implemented)).

---

## P2 — decisions and hygiene

### P2.1 Marking categories: do not seed

**Status:** decided. **Confidence:** confirmed for the musxdom half, `open` for
the version half.

The Finale 27 baseline contains 7 `markingsCategory`, 7 `markingsCategoryName`,
and 16 category staff lists. Finale appears to require them; musxdom does not:

- `resolveExpressions` logs at Info level and continues when an expression's
  category is missing;
- `MeasureExprAssign::getMarkingCategory` returns `nullptr` for an absent or
  zero category; and
- `resolveMarkingCategories` only validates the `categoryType` of categories
  that exist.

Legacy documents predating categories have none, and their text expressions
carry no `categoryId`. Seeding the pinned set would add seven categories the
source never had, which no expression would reference and which
`resolveExpressions` would leave empty — fabricated structure with no
corresponding source fact.

**Decision:** keep them out of the allowlist. Import categories from the source
when expression records are decoded, and let musxdom's existing tolerance handle
documents that have none.

**Open:** the Finale version that introduced marking categories (recollection
suggests 2009) is unverified. Confirm from the corpus before writing
version-aware category decoding.

### P2.2 Dangling shape references in seeded options

**Status:** gap. **Confidence:** confirmed 2026-08-09.

Two baseline clef definitions set `isShape` and point at shape records, and
`MultimeasureRestOptions` points at one more. None are seeded. Unlike fonts,
every `ShapeDef` lookup in musxdom is null-tolerant, so these degrade quietly
rather than throwing: the shape clefs behave as though they have no shape.

The reasoning in [P0.2](#p02-font-definitions-must-come-from-the-mus-file)
applies unchanged. Shape ids share the same single id space, and the two
committed baselines disagree about which shape id a given clef uses, so seeding
pinned shape records would fabricate identity now and collide with source shape
records later. Clear the references instead, so the document does not claim
shapes it lacks, or leave them until shapes are decoded from the source. Lower
priority than fonts only because nothing throws.

### P2.3 Early version ordering

**Status:** resolved 2026-08-09, with two versions unverified. **Confidence:** confirmed.

The version packing, the saving-product to internal-version mapping, and the byte-order
behavior are now decoded and recorded in
[VERSION_MATRIX.md](VERSION_MATRIX.md#decoded-version-packing). The consequences for
gating:

- **Major alone does not order Finale's history**, as suspected. Finale 3.2, 3.5, 3.7
  and Finale 97 all carry major 3; Finale 97 is 3.8. Gates below major 5 must therefore
  state a minor, which `VersionRange` supports.
- **Finale 98 and Finale 2011 are absent from the corpus.** They are presumed to be
  majors 4 and 16. Do not write a gate that depends on either until a sample confirms it.
- **Coda-banner files carry a version, in their product banner.** They have no header
  tuple: the whole 0x60-0x200 region is zero apart from a constant pair at 0x80. The
  reader recovers major and minor from the `Finale(TM) 2.6` text instead, so
  `ImportReport::sourceVersion::raw` is zero for them. Every such file in the corpus is
  Finale 2.6, so no gate below major 3 has been exercised against real evidence.
- **Byte order is settled.** The tuple is stored in the file's own byte order, confirmed
  by the 2007 and 2008 wrapper splits matching the container classification exactly. Both
  paths are covered by tests.

### P2.4 musxdom dependency pin

**Status:** done 2026-08-09. **Confidence:** confirmed.

`CMakeLists.txt` pins musxdom at `3df7602` on `main`, which carries both
interfaces the reader depends on: the factory construction session (musxdom
#157) and `musx::factory::NodeFilter` (musxdom #158). Keep the pin on merged
`main` revisions.

---

## Allowlist reference

The pinned macOS baseline `<others>` holds 127 direct children across 31 tags.
Classification as of 2026-08-09:

| Tag | musxdom type | Seed? | Rationale |
| --- | --- | --- | --- |
| `layerAtts` | `LayerAttributes` | **yes** | Option-like; currently the entire allowlist. |
| `fontName` | `FontDefinition` | never | Referenced by seeded options, but must come from the MUS file; see P0.2. |
| `shapeDef`, `shapeData`, `shapeList` | ShapeDesigner | never | Referenced by seeded options, but the ids are not transferable; see P2.2. |
| `markingsCategory`, `markingsCategoryName`, `categoryStaffListScore`, `categoryStaffListParts` | category set | no | See P2.1. |
| `ssLineStyle` | `SmartShapeCustomLine` | no | Library content; no seeded option references it. |
| `fretInst`, `fretStyle` | fretboard library | no | `ChordOptions` fret ids are absent, so they default to 0. |
| `measSpec`, `staffSpec`, `staffSystemSpec`, `pageSpec`, `frameSpec`, `instUsed`, `partDef`, `partGlobals`, `textBlock`, `measNumbRegion` | score content | never | Fallback score content; must not leak. |

Nine baseline tags have no registered musxdom type and are skipped regardless of
the allowlist: `metaClef`, `durAllot`, `volumeValue`, `bypassFxValue`,
`playbackRoute`, `playbackRouteName`, `playDefs`, `staffPlayData`, `viSetup`.
`durAllot` is the spacing allotment table and has no musxdom home at all, which
is worth remembering before spending evidence effort on the legacy equivalent.

---

## Method

Baseline composition was read from
`resources/defaults/finale27-macos-new-document-without-libraries.enigmaxml`.
Registered-type classification was taken from musxdom's `PoolFactory.cpp` type
registries. The font failure was reproduced against
`tests/evidence/F2002/F2002-baseline.mus` through the reader's public API. Re-run
these checks before trusting the counts in this note after a musxdom update.
