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
entirely and the reader leaves them at their constructed defaults. Every font in the 54 then known
Coda-banner and 28 Finale 3.0-3.7 documents is therefore reported as a Mac-bank font with
character set 0, which is a default rather than a reading. Recovering them will have to come
from somewhere other than the `FN` record.

### P0.2 Remaining seeded option font ids are not reconciled

**Status:** blocker, narrowed 2026-08-11. **Confidence:** confirmed.

This is the hazard that argued against seeding pinned font definitions, now arriving from
the other direction. The pinned Finale 27 options carry Finale 27 font ids, and real font
definitions from the source document now exist under the source document's ids. An option
the reader has not yet mapped therefore resolves its font id against a table it was never
written for, and returns whichever font happens to occupy that id.

The direct `FontOptions` case is now reconciled. The pinned object is filtered out; confirmed
source tuples are inserted through an era-specific physical-to-semantic map, and every missing
modern type is cloned from the separate baseline document. Baseline id 0 passes unchanged.
Every nonzero baseline id is matched to the lowest target comparator by normalized font name,
or its complete `FontDefinition` is cloned at the next nonzero comparator with the selected
platform reference's exact name spelling. The reference comparator itself is never retained.
Finale 1.0.0 Music,
TextBlock, and LyricVerse are recovered from confirmed locations; other early categories safely
remain synthesized.

`ClefOptions` is the second class removed from this hazard, for the same reason and by the
same means: every one of its eighteen clef definitions can carry a font id, live whenever the
own-font flag bit is set. It is filtered out of the seeding and rebuilt, so a clef font id in
an imported document is always the source's own. The two pinned baselines give no clef its own
font, so extending a short collection from the baseline introduces no font id at all, and the
reader asserts that rather than assuming it.

The blocker remains for the other seeded option classes containing font ids. Those objects are
still copied from the baseline with numeric references that have not been reconciled.

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

A legacy MUS import is not expected to become as complete as a native MUSX document. The reader
recovers only source representations established by evidence and fills structurally required
gaps from reported defaults; this section records the remaining distance so coverage can improve
incrementally without presenting inferred upgrade behavior as source data.

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

The shortfalls are era facts, not defects. Selector `94` does not appear before
Finale 2000, so Finale 3.2 through 97 recover layer offsets and no spacing.
Coda-banner files recover neither, because layers were introduced in Finale 3.x
and that era has no layer attributes to recover: those four values correctly keep
their Finale 27 defaults and are reported as synthesized. The ten uncompressed
files without layer offsets are Finale 97 documents that carry no `LA` records
despite layers existing by then, which is unexplained. Per-era detail is in
[LEGACY_OPTION_MAPPINGS.md](LEGACY_OPTION_MAPPINGS.md#corpus-verification-of-promoted-mappings).

This measures recovery, not accuracy. Only the fixtures with ETF counterparts
confirm that recovered values are correct.

**FontOptions now reads the uncompressed era. Fixed 2026-08-11.** Selector `24` is the
default-font array in every fixed-row epoch except the Coda banner, but the layout row had
been gated to `EpochMask::Dcl`, so all 208 uncompressed-era corpus files reported 45 of 45
font options as Finale 27 defaults while the source held 40 of them. No test covered the era,
which is why it went unnoticed.

Both the layout row and the semantic layout now key on the epoch. `EpochMask::FixedRow`
excludes exactly the one era where selector 24 means something else, so no version range is
needed and none is used; and `semanticType` takes the earlier layout for the whole
uncompressed epoch rather than asking for a major version, which keeps it true of any file
the container classifies, including one whose header version cannot be recovered.

Verified against every adjacent-exact companion in the era: 6,100 recovered font sizes across
173 files, with no disagreement. Finale 3.7.2, Finale 97 and Finale 2000 fixtures cover the
span, and a synthetic file with an out-of-range major version covers the Finale 3.0 shape.

`ClefOptions`, added 2026-08-11, is the first mapping to cover every epoch including
zlib, and the first verified against the whole adjacent-exact companion set rather
than against fixtures alone. Over the 1,150 distinct direct corpus files:

| Epoch | Files | With source clef definitions | Definitions recovered |
|---|---:|---:|---:|
| Coda banner | 62 | 62 | 496 |
| Uncompressed | 177 | 177 | 1,416 |
| DCL | 388 | 388 | 6,892 |
| zlib | 522 | 503 | 9,054 |

The nineteen zlib files that recover nothing are the unframed files of
[P1.4](#p14-twenty-zlib-era-files-are-not-framed), not a clef gap. Accuracy was
measured separately against all 1,120 adjacent-exact Finale 27 companions: of the
source-supplied definitions every field agrees except four, in three Finale 2.6 files
the companion itself rewrote. Seven of the eight scalar options agree on every
compared file in every era where the reader reads them. Detail is in
[FORMAT_NOTES.md](FORMAT_NOTES.md#clef-definitions).

`ImportReport::fields` already distinguishes recovered from synthesized values,
which is what keeps this a gap rather than a correctness problem. Promotion of
further mappings should stay evidence-led and version-aware.

### P1.2 No score content is imported

**Status:** gap. **Confidence:** confirmed.

No measures, staves, entries, text, page or system layout, parts, or
instruments. The reader currently produces a fallback-heavy document whose `FontOptions`
is structurally complete but may synthesize many categories when their source representation
is not identified. It also contains a recovered header. This is the
honest current scope and is asserted by
`expectNoScoreContent`, but "MUS reader" will be read by users as meaning score
content.

### P1.3 The zlib era decodes only supported record classes

**Status:** gap, narrowed 2026-08-11. **Confidence:** confirmed.

The Coda-banner half of this item is done. Its pools are decoded into typed
blocks, its others and details pools are indexed like every other epoch, and all
54 corpus files then known recover their font tables.

The 2007-2012 class-record framing is now decoded. The reader recovers zlib-era
font definitions and captures the complete class `0x0026` default-font tuple stream
in either byte order. Most other record classes remain unidentified or unmapped, so
the era is still the largest single block of unserved score content.

Twenty of those files additionally log a zlib decompression failure during
inflation, which is a separate defect from the undecoded records.

### P1.4 Twenty zlib-era files are not framed

**Status:** resolved 2026-08-11. **Confidence:** confirmed.

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

**Resolved 2026-08-11.** The remaining twenty files, all zlib-era and nineteen
distinct by content, were not unrecognized framing either, and were not covered by
P1.3. Every one of them framed correctly and decoded four blocks with valid CRCs,
including `0x001a`, the record block that holds the whole options pool. The parse
then threw all four away, because the container assumed any block longer than the
six-byte empty marker was a compressed member and returned nothing on the first
failure.

The block that failed holds an **embedded graphic**, stored uncompressed: twelve
files carry a DOS/binary EPSF header, six carry `%!PS-Adobe`, and one carries a PNG
signature. See
[FORMAT_NOTES.md](FORMAT_NOTES.md#which-blocks-are-compressed). The container now
decides compression from an allowlist of block types rather than assuming it, so an
unknown type is preserved verbatim instead of failing the document. All 1,150
distinct corpus files now parse, and the nineteen recover their options like any
other file.

The bytes are preserved and reported but not imported; that is
[P3.1](#p31-embedded-graphics-are-preserved-but-not-imported).

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

**Status:** gap, narrowed 2026-08-11. **Confidence:** confirmed.

Two baseline clef definitions set `isShape` and point at shape records, and
`MultimeasureRestOptions` points at one more. None are seeded. Unlike fonts,
every `ShapeDef` lookup in musxdom is null-tolerant, so these degrade quietly
rather than throwing: the shape clefs behave as though they have no shape.

`ClefOptions` is no longer seeded, so a shape clef now reaches an imported document by
one of two routes. When the source stores its own clef table, which every Finale 2001
and later file does, the shape comparator is the source's own and will resolve once
shape records are decoded. Only when the collection has to be extended from the
baseline — every pre-2001 file, whose eight stored clefs cannot include indices 16 and
17 — is a baseline comparator carried, and the reader emits a warning saying so. The
controlled companions show Finale's own upgrade assigning a different shape comparator
per document, which is independent confirmation that these ids are not portable
identities.

The reasoning in [P0.2](#p02-font-definitions-must-come-from-the-mus-file)
applies unchanged. Shape ids share the same single id space, and the two
committed baselines disagree about which shape id a given clef uses, so seeding
pinned shape records would fabricate identity now and collide with source shape
records later. Clear the references instead, so the document does not claim
shapes it lacks, or leave them until shapes are decoded from the source. Lower
priority than fonts only because nothing throws.

### P2.6 Coda-banner byte order is asserted, and Windows Finale existed then

**Status:** blocker. **Confidence:** confirmed 2026-08-11, with specimens.

`parseCodaBanner` hardcodes big-endian, and 24 documents that need the other order are
now in hand: the `.MUS` templates and tutorial files from the Finale 2.2 for Windows
install disks. This item was written as a gap awaiting a specimen; the specimens exist,
so it is a blocker for that population.

Two independent fixes are needed, and one marker answers both. The era's banner states
its platform: Windows documents carry the product `PC 1.0+` and Mac documents carry a
bare version. So the classifier's `hasNumericProduct()` test should admit a `PC` product
instead of rejecting it, and the same token should select little-endian rather than the
hardcoded big-endian.

Test the leading `PC` token alone. The rest of the field is a version string and must not
be part of the test: `1.0+` is the only value seen, but the token exists precisely because
the platform is stated separately from the version, and matching the whole string would
reject a Windows document from any other release. Note that `1.0+` will not parse as a
version either, so these files will carry no recovered version and every version-gated
mapping will skip them; the era's mappings are gated on the epoch, so that is survivable.

The marker discriminates perfectly over both corpora — 24 `PC` documents, all
little-endian; 252 numeric-product documents, all big-endian; no other product string
anywhere contains `PC`. The pool prologue remains available as a cross-check, since its
page-size word reads `0x200` in one order and `0x0002` in the other, but it is no longer
the primary test.

Both directions are now verifiable against real files, which is what this entry was
waiting for.

The original entry follows.

`parseCodaBanner` hardcodes big-endian. That was recorded as an untested assumption
supported by all 62 corpus files; it is now known to be incomplete. Microsoft KB
Q107181, the Windows Sound System 2.0 README, names "Finale 2.2 for Windows from Coda
Music Technology", and Finale 2.2 falls inside this era's 1.8.7-to-2.6 range, so the
era spans both platforms even though no surveyed file does.

A little-endian document of this era would decode with transposed words and tags that
are not text. It would most likely present as an unrecognized variant rather than as
plausible wrong data, which is the better failure of the two, but it would not be read.
No such file is in any surveyed corpus, so nothing is misread today.

Unlike the other eras this one has no block framing to trial, but it is not untestable:
each pool prologue holds a page-size word that reads `0x200` in one order and `0x0002`
in the other. Trialling the prologue would replace the assertion with a test and would
handle a Windows document of the era correctly if one ever appears. That is a small,
self-contained container change, deliberately not made without a specimen to verify it
against; the risk of writing an untested second path is comparable to the risk it
removes.

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

### P2.4 Header fields beyond the versions are unpopulated

**Status:** gap. **Confidence:** confirmed 2026-08-09.

`recoverHeader` fills word order, text encoding, the creation and last-save dates,
the application string, the platform, and all three version tuples. Three fields of
musxdom's `header::FileInfo` are never written:

- **`devStatus`** is decoded but discarded. The version packing carries a
  development-status code in bits 15-8, and `SourceVersion::devStatus` holds it:
  Finale 2002 stores 3, Finale 2012 stores 2 and 4. musxdom's field is a string
  such as `dev` or `release`, and the mapping from code to name has not been
  established, so nothing is written rather than a guess being recorded.
- **`appRegion`** is never attempted. Finale 27 EnigmaXML carries `<appRegion>US</appRegion>`.
- **`modifiedBy`** is never attempted. Finale 27 EnigmaXML carries the element, and
  musxdom groups it with the date, which suggests the legacy file-info block is
  where to look.

The 2007-2012 era is the place to start. Its records are class-identified and map
closely onto the EnigmaXML element model, so whatever carries this information is
likely a record with its own class id rather than a packed header field, and
reading it should be a table rather than an investigation. Confirming the values
there would also supply the `devStatus` mapping for every earlier era, because a
Finale 27 conversion of the same document states the name that a numeric code
corresponds to.

Coda-banner files are a separate case and are already as complete as their format
allows: they record no date, application, or platform anywhere, and their only
version is the one in the product banner, which the reader now writes to the
last-saver block.

### P2.5 musxdom dependency pin

**Status:** done 2026-08-09. **Confidence:** confirmed.

`CMakeLists.txt` pins musxdom at `3df7602` on `main`, which carries both
interfaces the reader depends on: the factory construction session (musxdom
#157) and `musx::factory::NodeFilter` (musxdom #158). Keep the pin on merged
`main` revisions.

---

## P3 — deferred until everything above is done

### P3.1 Embedded graphics are preserved but not imported

**Status:** open, near-last priority. **Confidence:** confirmed 2026-08-11.

Nineteen distinct corpus files embed a graphic in a stored block: twelve a binary
EPSF, six a `%!PS-Adobe` EPS, one a PNG. The container keeps the bytes and
`ImportReport` names them, so nothing is silently dropped, but nothing consumes
them. See [FORMAT_NOTES.md](FORMAT_NOTES.md#which-blocks-are-compressed).

**Corrected 2026-08-12.** This entry previously listed two blockers and both were
wrong. musxdom does have a destination: `DocumentFactory::CreateOptions` carries
`EmbeddedGraphicFiles`, a vector of `{filename, EmbeddedGraphicBlob}` supplied at
document creation. That also disposes of the filesystem objection, since the blobs
are in-memory and the filename is just a label. Nothing about WebAssembly prevents
this, and no second document model is needed.

What actually remains is decoding, and it is a self-contained cycle of work rather
than an open question:

- **The inner framing differs by epoch** and has to be worked out per era, the way
  every other structure here has been.
- **The framing is only partly read.** The stored payload begins with a nested
  `type` and `size` header — `0x000f` and `0x0043` both observed — before the image
  signature, and a block may hold more than one graphic. That is the first step
  whenever this is picked up, and it needs no new evidence; the corpus already has
  the specimens.

The expectation is that these will eventually be carried across. Until then the
bytes are preserved, the report names them at info level, and the document is
usable without them.

A related observation, also deferred: **208 zlib files carry a non-empty `0x001d`
block** that the reader never reaches, because the walk stops at the first terminal
marker and those bytes fall into `trailingByteCount`. They are not graphics — the
payload shape differs — and what they hold is unexamined. If embedded audio or
other attachments exist, that is where to look first.

### P3.2 The 94 residual FontOptions disagreements

**Status:** open, low priority. **Confidence:** measured 2026-08-12 across 2,700
adjacent-exact companions.

After the 13/28 gate correction and the whole-tuple font substitution, 94 recovered
FontOptions values still disagree with their companion, against 111,131 that agree.
They are recorded here so they are not rediscovered as if new. Three groups, and
only the last is genuinely unexplained:

- **77 `staffNames`.** The Coda-banner era stores one `Name` preference where
  Finale 3.0 and later store four, and the importer propagates it to all four. The
  companions disagree among themselves in a way that tracks the default file each
  upgrade ran under rather than the source document, so they cannot settle it. See
  FORMAT_NOTES, "The single `Name` preference reaches all four modern name types",
  and evidence request C10, which names the one file that would decide it.
- **12 `ending` and 1 `tuplet`.** Not disagreements at all on inspection: ours reads
  `New Century Schlbk` and the companion `New Century Schoolbook`, at identical
  sizes. Classic Mac truncated file and font names to 31 characters, and
  `normalizeFontName` strips punctuation without expanding abbreviations, so the two
  spellings of one face compare unequal. Any future name comparison across this
  boundary needs to account for truncation, not just normalization.
- **4 `fretboard`.** Unexplained. Worth knowing that every anomalous font size in
  the corpus — 16 negative and 10 above 200, out of 282,983 recovered sizes — falls
  on this one ordinal and nowhere else. That concentration is the thread to pull if
  fretboard misbehaves again.

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
