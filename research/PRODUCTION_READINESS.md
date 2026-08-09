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

### P0.1 Seeded font references resolve to nothing

**Status:** blocker. **Confidence:** confirmed 2026-08-09.

The pinned Finale 27 `<options>` pool references font definitions by cmper. The
allowlist admits only `layerAtts`, so no `others::FontDefinition` reaches the
imported document and every one of those references dangles.

Observed on `tests/evidence/F2002/F2002-baseline.mus` by calling
`FontOptions::getFontInfo` and then `FontInfo::getName`:

```
fontId=0 -> THROWS: font definition not found for font id 0
fontId=1 -> THROWS: font definition not found for font id 1
```

`FontInfo::getName`, `calcIsSymbolFont`, `setFontIdByName`, and
`calcIsSameTypeface` all throw `std::invalid_argument` when the definition is
absent. Nothing in document finalization touches them, so import succeeds and
the failure surfaces later, inside consumer code.

Baseline reference counts: `fontID` 1 appears 19 times, plus 3, 5, and 13 once
each; every `<font>` node that omits `<fontID>` means id 0, the document default
music font. The baseline defines cmpers 0–14.

This is the visible symptom of [P0.2](#p02-font-definitions-must-come-from-the-mus-file),
which is the actual work item and which rules out the obvious quick fix.

**Seeding the pinned font definitions is rejected.** It looks like the cheapest
interim — allowlist `fontName`, report the definitions as synthesized — but a
document has a single `fontId` space. Once the real font table is imported, the
seeded option ids would stop dangling and start *resolving*, into whichever
source record happens to share that cmper. A loud `std::invalid_argument`
becomes silent misattribution, and it happens exactly when the correct fix
lands. The interim would be a landmine armed by its own replacement.

The only acceptable interim is one that fabricates nothing: report at import
that font references do not resolve, so a consumer learns it before calling
`getName`. Until then, the throw is the honest behavior.

### P0.2 Font definitions must come from the MUS file

**Status:** blocker. Top priority once record decoding begins; not queued before
then. **Confidence:** strong.

Font definitions must be decoded from the source. The pinned baseline's table
cannot substitute for it: the tables are unrelated, and the ids are not
transferable. Ids are assigned per capture machine — the two committed baselines
disagree about which id holds which typeface purely because they were captured
on different machines — so a baseline id carries no meaning that survives being
copied into another document.

Production requires both halves:

- decode the legacy font table and create `others::FontDefinition` from it; and
- reconcile every `fontId` in the imported document with that table.

The second half is not automatic once the first lands. Options the reader never
recovered still carry baseline ids, and those must be resolved deliberately —
by recovering the corresponding legacy font preferences, or by pointing them at
a designated source-derived font and reporting them as synthesized. Letting them
resolve by cmper coincidence is the one outcome to avoid.

**Invariant to enforce once implemented:** every `fontId` in an imported
document resolves to a `FontDefinition` decoded from the MUS file. This is
checkable as a post-import pass and should be asserted by tests.

**Evidence already in hand.** The controlled ETF exports carry the font table in
the clear, as `^FN(cmper) "name"` records — `tests/evidence/F2002/F2002-baseline.etf`
holds 19 of them, beginning `^FN(0) "Maestro"`, `^FN(1) "Times"`,
`^FN(2) "Helvetica"`. Each has a matching `.mus` file for 2002–2005, so the DCL
era has ground truth to decode against. For the zlib era,
[RECORD_CATALOG.md](RECORD_CATALOG.md) already lists `0x0090` as `fontName` at
`weak` confidence across 2007–2012. This work starts from evidence, not a survey.

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

### P1.3 Pre-banner and zlib-era logical records are unresolved

**Status:** gap. **Confidence:** confirmed.

Both epochs classify and frame correctly but overlay nothing; the reader emits
warnings saying so. Pre-banner pool directories remain unresolved
(`FORMAT_NOTES.md`), and 2007–2012 variable logical records are not decoded.

### P1.4 Legacy text encoding is not converted

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

### P2.3 Early version ordering is ambiguous, and pre-banner files have no version

**Status:** open. **Confidence:** confirmed for the mechanism, `open` for the boundaries.

Mapping rows are gated on the version embedded in the file, which is decoded from the
banner header tuple as major.minor.maint.build. Two limits are known:

- **Major alone does not order Finale's history.** Finale 98 is believed to be major 4
  and Finale 97 major 3, which collides with the Finale 3.x line. The gate therefore
  orders on major *then minor*, but the actual early boundaries are unverified. Confirm
  what Finale 97 and 98 record before writing any gate below major 5. The corpus holds
  Finale 97 files; Finale 98 may be absent entirely.
- **Pre-banner files carry no recoverable version.** They have no banner, so
  `ImportReport::sourceVersion` is absent and only ungated rows can apply to them.
  Whether a version lives elsewhere in those files is unknown and needs its own
  investigation.

A third limit is untested rather than unknown: every controlled fixture is big-endian,
so whether a little-endian file stores the header tuple byte-reversed has never been
exercised. The reader warns when the major version falls outside 0-27 and notes when the
low byte holds a plausible major instead, which is the signature that case would produce.

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
