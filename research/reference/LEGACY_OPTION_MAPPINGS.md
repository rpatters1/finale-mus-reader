# PDK Framework Option Mappings

## Scope and provenance

This note records interoperability facts distilled from two privately supplied histories of a third-party PDK
Framework. The sources were inspected read-only with the owner's authorization. No framework source, declarations,
comments, or implementation code is stored in this repository, and neither history is identified here by
repository, branch, path, or author: only by content hash, in the same way corpus evidence is identified.

Three snapshots are distinguished below, because their mappings differ:

| Alias used here | What it is |
|---|---|
| **current snapshot** | The principal source of the published mapping, snapshot `44650a9a…`. |
| **historical snapshot** | Used for comparison, snapshot `d8a4c778…`. |
| **original-distribution snapshot** | That history's initial source import, snapshot `37326071…`, the one closest to Finale 2012. |

The inspected framework implementation file was unmodified in each snapshot; unrelated local changes in the
historical worktree were not consulted.

The `StemOptions` scalar locations were taken from the original-distribution snapshot, which is
the one closest to Finale 2012. Its inspected implementation file has SHA-256
`9fa4ab822adb503fbc3aa92cf499309904f5e556633f627271304b6aa1c82a91`, and the flag-bit meanings
come from the corresponding `ff_prefs.h`. The framework distributes one logical options class
across several omnibus preference structures, so those eight fields are spread over
`SizePrefs`, `DistancePrefs` and `MiscDocPrefs` and over five numeric globals.

These findings are labeled **private-framework-derived**. They are not clean-room findings and should not be
described as public-PDK-derived. The provenance is historically mixed: the compatibility routines are described as
originating in an earlier PDK source, while the user reports that the framework's original author wrote routines
later adopted by MakeMusic. This note makes no ownership determination. It records only the field-location facts needed to read user-owned documents.

The complete distilled field table is
[`data/legacy_option_mappings.csv`](../data/legacy_option_mappings.csv). Direct multi-incidence option
blocks that do not use the field map are cataloged in
[`data/legacy_direct_option_blocks.csv`](../data/legacy_direct_option_blocks.csv). These are factual
schemas, not translations of the C++ source.

The separate [`data/legacy_option_font_id_locations.csv`](../data/legacy_option_font_id_locations.csv)
audits every musxdom options-pool field that contains a font-definition cmper. It was checked against
the same historical snapshot; the inspected implementation file has SHA-256
`f38fa25ad08e41130c36b228794bbea306b5f96e0db58cef4315e5e55dbaf683`. The table contains no private
path and records only distilled interoperability facts.

## Central finding

Before Finale 26.2 exposed regular preference structures, the framework reconstructed synthetic preference objects
from ordinary Enigma records. Numeric tags such as `^94` normally select comparator `65534`; this is the same form
printed by ETF as `^94(65534)`. Each incidence supplies six 16-bit payload words. A mapping identifies:

- the two-character tag or numeric global number;
- comparator and incidence;
- a zero-based word slot within the six-word payload;
- a one-, two-, or four-byte value width;
- any word-order or boolean conversion;
- a semantic field and preference group; and
- for a few fields, a minimum Finale version.

This explains the otherwise opaque ETF `^NN(65534)` records. They are not arbitrary blobs: many are shared backing
records whose individual words feed fields in several logical options objects. Conversely, one logical options object
can draw fields from many global numbers and incidences. A reader must therefore index physical option records first
and assemble logical musxdom options afterward.

The framework's four-byte conversion rules distinguish two word orders:

- `MACFOURBYTE`: first payload word is the high word and the next is the low word;
- `WINFOURBYTE`: first payload word is the low word and the next is the high word.

It also identifies an inverted-boolean conversion. One-byte fields remain a binary-validation target because the
framework selects them through a 16-bit payload slot and then narrows the value.

## Mapping inventory

The current snapshot yields 435 mapping rows across 24 groups: 424 baseline rows and 11 Finale 26.2 compatibility
overrides. The original branch preserves two legacy locations that no longer occur in the current table, making the
published union 437 rows. The union contains 385 two-byte fields, 36 four-byte fields, and 16 one-byte fields. It
references 61 distinct numeric globals and nine nonnumeric tags.

The available ETF set contains 59 of the 61 numeric global selectors; only `^47(65534)` and `^48(65534)` are absent.
Across numeric and nonnumeric selectors, 386 of 437 published rows refer to a selector observed in at least one available ETF.
This verifies selector existence only. It does not independently prove the semantic field, slot, width, signedness, or
conversion of any individual row. ETFs also contain 35 numeric `^NN(65534)` selectors not covered by the framework
table, so this is substantial partial coverage rather than a complete options map.

| Group | Rows | Principal selectors | Candidate musxdom target |
|---|---:|---|---|
| `ChordPrefs` | 14 | `^37`, `^41`, `^44`, `^45`, `^76` | `ChordOptions` |
| `DistancePrefs` | 44 | 14 numeric globals plus `^OA(0)` | Multiple geometry option classes |
| `ExtraTupletPrefs` | 4 | `^14`, `^23`, `^69` | `TupletOptions` |
| Group/staff-name positions | 12 | `^04`, `^66`, `^79`, `^80` | `StaffOptions` |
| `LayoutDistancePrefs` | 6 | `^01`, `^76`, `^77`, `^93` | `PageFormatOptions` or `MiscOptions` |
| `LyricsPrefs` | 19 | `^15`, `^35`, `^57`, `^67`, `^87`, `^OL(2)` | `LyricOptions` |
| `MMRestDefaultsPrefs` | 13 | `^25`, `^83` | `MultimeasureRestOptions` |
| `MiscDocPrefs` | 50 | 20 numeric globals plus five named records | `MiscOptions` and related classes |
| Music-character groups | 11 | `^11`, `^12`, `^19`, `^22`, `^69` | `MusicSymbolOptions` |
| `MusicSpacingPrefs` | 17 | `^41`, `^94` | `MusicSpacingOptions` |
| Page-format score/parts | 44 | score globals and `^77` for parts | `PageFormatOptions` |
| `PartExtractPrefs` | 25 | `^77` | Page format or unsupported extraction policy |
| `PartScopePrefs` | 1 | `^PG(65534)` | Part metadata or `MiscOptions` |
| `PianoBracePrefs` | 11 | `^45`, `^60`, `^61`, `^64`, `^65` | `PianoBraceBracketOptions` |
| `PlaybackPrefs` | 53 | five numeric globals plus `^FI`, `^OP`, and `^pd` | Currently unsupported playback/document settings |
| `RepeatPrefs` | 25 | `^05`, `^20`, `^69`–`^72`, `^76` | `RepeatOptions` |
| `SizePrefs` | 21 | 14 numeric globals | Multiple size-related option classes |
| `SmartShapePrefs` | 39 | `^50`, `^51`, `^53`, `^92`, `^93`, `^97`, `^FI` | `SmartShapeOptions` or `LineCurveOptions` |
| `TiePrefs` | 26 | `^41`, `^84`, `^97` | `TieOptions` |

The candidate musxdom targets are routing guidance, not verified one-to-one class mappings. Aggregates such as
`DistancePrefs`, `SizePrefs`, and `MiscDocPrefs` distribute fields across multiple modern options classes.

## Historical differences

A preserved original-mapping branch, snapshot `982939e1…`, holds the original author's
pre-Finale-2014-oriented mapping line: 343 rows in 22 groups. Of those, 341 remain byte-for-byte equivalent as factual
mapping tuples in the current table. The 2022 historical head contained 373 rows, and the current snapshot contains
435. Later work primarily expanded playback data, added chord preferences, and added a small number of newer Finale
fields and compatibility overrides. Current-only rows are not automatically applicable to old files merely because
they were discovered later.

Two directly relevant original-branch locations no longer appear in the current table. `specialExtractedPartCmper`
was originally mapped to numeric
global `^23(65534)`, incidence 0, word slot 4. A later version moved it to `^PG(65534)`, incidence 0, word slot 3.
The original `score_in_c` mapping used `^PG(0)`, incidence 0, word slot 0 with inverted-boolean interpretation; the
current mapping uses `^PG(65534)` at the same incidence and slot. These are probably version-dependent and must not
be resolved by blindly applying the newest table to every MUS era. The CSV preserves all alternatives with an
explicit `mapping_lineage` column.

Eleven current rows are runtime replacements used by the framework's Finale 26.2 compatibility path. They relocate
selected distance, lyrics, playback, and miscellaneous-document fields to named records such as `OA`, `OL`, `OP`,
`OY`, and `pd`. These overrides describe the newer compatibility layer; the baseline numeric-global locations are the
starting point for Finale 2001–2006 MUS files.

## Direct multi-incidence option blocks

Five legacy option structures bypass the synthetic field map but still have explicit physical locations:

| Logical structure | Legacy selector | Organization | Candidate target |
|---|---|---|---|
| Slur contours | `^52(65534)` | two fixed incidences | `SmartShapeOptions` or `LineCurveOptions` |
| Tie placement | `^85(65534)` | four fixed incidences | `TieOptions` |
| Tie contours | `^86(65534)` | five fixed incidences | `TieOptions` |
| Grids and guides | `^88(65534)` | eleven fixed incidences | currently unsupported document settings |
| Stem connections | `^40(65534)` | variable incidence collection | `StemOptions` |

All five selectors occur in the available ETF evidence. Four of the five field layouts remain source-derived.

The stem-connection block is now decoded and implemented: one connection per incidence, six words in the order
`fontId`, `symbol`, `upStemVert`, `downStemVert`, `upStemHorz`, `downStemHorz`, with the symbol widening to a long
at Finale 2012 and the adjustments stated in Evpu rather than Efix before Finale 3.5. The framework's claim that the
font is the element's first 16-bit member is **independently binary-verified**, and its Finale 2012 layout caveat is
confirmed. See [stem_options.md](../format/options/stem_options.md#stem-connections) for the per-era table, the terminator rule, and
the stale pre-Unicode copy that Finale 2012 leaves in the record.

The eight `StemOptions` scalars around that collection are distilled and implemented as well; their locations,
per-era caveats and verification counts are in
[stem_options.md](../format/options/stem_options.md#the-eight-stem-scalars-and-the-marker-that-dates-them). Six are corpus-confirmed
against non-default companions. Two — the half-stem length and the normal stem length — are consistent everywhere
and exercised nowhere, because no companion in either corpus states anything but 18 and 84 for them, and the
reverse-stemming bit is likewise never set. Those three remain **strong**.

Twenty-two `RepeatOptions` scalars are likewise promoted from the seven `RepeatPrefs` numeric
globals. Fixed-row and zlib-class layouts agree from Finale 3.7 onward; Coda-banner and Finale
3.0–3.2 remain uncovered. The two document staff-display fields are not among the promoted rows.
See [repeat_options.md](../format/options/repeat_options.md#repeat-options-the-document-staff-list-reference).

## Options-pool font IDs

Two of the five musxdom options fields containing font-definition cmpers have legacy locations identified by the
framework study. In the verified Finale 2002–2005 range, `FontOptions` uses the direct default-font preference family
at `24(65534)`. Each incidence packs two three-word font tuples: `(font ID, size, effects)` in slots 0–2 and another
in slots 3–5. For zero-based physical tuple index `n`, the incidence is `n / 2`; the font-ID slot is 0 when `n` is
even and 3 when it is odd. The physical index is translated through a versioned semantic map rather than cast to
the modern `FontType` enum. This layout is not valid for Finale 1.0.0, whose single selector-24 row contains values that
cannot reference its five font definitions.
Controlled Finale 1.0.0 fixtures instead confirm Music at words 0–2 of `02(65534)`, TextBlock at words 0–2 of
`26(65534)`, and LyricVerse at words 3–5 of `26(65534)`. No other early category is inferred from the Finale 27
companions because the upgrade synthesizes and couples additional values.
In zlib files the same logical stream is coalesced into singleton class `0x0026(65534)`: the tuple for `n` begins at
byte `6n`, with font ID, size, and effects at offsets `6n`, `6n + 2`, and `6n + 4`. Both controlled endian variants
have 45 live tuples followed by one zero tuple. The effects word is a bitmask and must be expanded with musxdom's
`FontInfo::setEnigmaStyles` into its six style booleans.
`StemOptions` uses the direct stem-connection block at `40(65534)`; before Finale 2012 each connection occupies one
incidence and its font ID is word slot 0. Available ETFs independently support the existence and organization of both
selectors, but no controlled one-option-at-a-time font change has yet promoted either semantic mapping to
`confirmed`.
Neither is a row in the framework's fragmented `PREFERENCE_LOCATION_MAPRECORD` arrays: both use direct preference
loaders. Thus the strict count found in those arrays is zero, while the broader framework source identifies two
direct legacy locations.

No preference-table location was found for the clef-specific override, alternate lyric-hyphen font, or text inserted-
symbol font. Absence from the framework tables is not evidence that a legacy document cannot store them: the
framework models clef overrides through clef definitions outside its preference maps, and it may simply not expose
the other two legacy fields. The complete five-row audit, including these open results and the Finale 2012 stem-layout
caveat, is in [`data/legacy_option_font_id_locations.csv`](../data/legacy_option_font_id_locations.csv).

The text inserted-symbol font is no longer open. It is word slot 5 of each element of the direct block at
`78(65534)`, and it is a source-document `FontDefinition` comparator like the others; see
[TextOptions](#textoptions-three-distilled-rows-and-a-class-the-framework-does-not-model) below. That location came
from the corpus rather than from the framework, which confirms the audit's own caution: the framework not exposing a
field says nothing about whether the file stores it.

### FontOptions implementation plan

The implementation follows this staged data flow: filter the unsafe baseline object, recover every era-verified
source tuple, translate physical positions through a versioned semantic map, then complete all absent modern types
from a separate baseline document with font-definition remapping.

1. Exclude the direct `<fontOptions>` child when loading the pinned `<options>` element. musxdom's existing
   `OptionsFactory::create` filter supports this directly. A modern fallback font ID is unsafe when the legacy file
   has a smaller, unrelated font-definition table; an absent map key is preferable to a silently wrong typeface.
   Retain a separate, fully populated baseline `DocumentPtr` for the completion pass, but never insert an object
   owned by that document into the imported document.
2. Give the numeric-option class transform one code home: zlib class ID is the old selector plus `0x000e`. Derive
   the FontOptions class from selector 24 rather than hard-coding two unrelated identities.
3. Add one bounds-checked font-tuple reader with a six-byte stride. Feed it the concatenated `24(65534)` fixed-row
   words before Finale 2007 and the `0x0026(65534)` class payload in the zlib epoch; numeric reads follow the file's
   established byte order.
4. Construct one fresh, document-owned `FontOptions`. For each complete source tuple, construct a fresh `FontInfo`,
   assign `fontId`, assign signed `fontSize`, and pass the unsigned effects word to `setEnigmaStyles`. Insert it only
   through the era-specific semantic map: through Finale 2011, skip physical 13 and map physical 28 to `Tablature`;
   from Finale 2012 onward, map 13 to `Tablature` and 28 to `Percussion`. Decide which layout applies by epoch first:
   the uncompressed and DCL epochs are entirely pre-2012 and need no version test, and only the zlib epoch spans the
   boundary. Major 12 occurs in both the DCL epoch (Finale 2006) and the zlib epoch (Finale 2007), so no version
   range alone separates them. Do not encode a tuple-count limit: walk
   what the file carries, ignoring an all-zero second tuple at the end of the final two-tuple unit as structural
   fill. This applies to fixed rows and to the tuple-pair grouping preserved in the zlib payload. Add
   the `FontOptions` object to the pool exactly once. Do not duplicate the six effect bit constants in this
   project.
5. Walk every complete tuple to the physical end of the family or payload; do not encode an expected tuple count.
   Report holding and structural-fill tuples even when the era map does not insert them. Keep the modern enum as
   the model bound, including reporting the terminal zlib zero tuple without inventing an out-of-range enum value.
6. Report the three recovered source values independently for each present `FontType`, including raw effects plus
   its legacy origin and offsets. Never partially construct or report a truncated tuple. Validate each recovered
   font ID against the source document's `FontDefinition` pool. During source capture, preserve and report a dangling
   numeric reference exactly as stored; never replace it silently with a default.
7. Complete all remaining `FontType` keys from the platform-matched baseline. Clone the baseline `FontInfo` into a
   fresh target-owned instance, preserving its size and effects, and remap its font ID by this exact procedure:
   - baseline cmper 0 transfers as cmper 0 without a lookup or validation;
   - for a nonzero cmper, obtain its `FontDefinition` in the baseline and compare its normalized name with the target
     definitions, using the matching target cmper if found. Normalization removes whitespace and folds case, so
     PostScript and family spellings such as `FinaleMaestro` and `Finale Maestro` compare equal;
   - if no name matches, clone every `FontDefinition` field into a fresh target-owned record at one past the greatest
     target font cmper, then use that new cmper; and
   - cache the remapping by normalized name so every later option using that baseline face reuses the same target
     definition. If more than one existing definition has the normalized name, choose the lowest cmper
     deterministically. Detect 16-bit cmper exhaustion rather than wrapping.
   musxdom already implements this normalization privately for font recognition. Promote that one implementation to
   a reusable public comparison/key helper and call it here; do not create a second normalization rule in the reader.
8. Report completed entries as synthesized Finale 27 defaults. The resulting target font ID shows whether an
   existing record was reused or a new comparator was allocated. The baseline document is a read-only template and need
   live only through completion; the finished import owns every resulting `FontOptions`, `FontInfo`, and
   `FontDefinition` instance.
9. Test synthetic truncation and both fixed-row packing halves, then assert exact real values from Finale 2002 and
   2005, big-endian Finale 2007, and little-endian Finale 2012. Include effects values `3`, `2`, and `1` so the tests
   prove conversion to multiple booleans and verify that no source-recovered or synthesized `FontInfo` instance is
   shared between documents. Add Finale 1.0.0 tests proving that there are five source font definitions, that its
   selector-24 row is not decoded with the later layout, that all 45 option keys are populated, that cmper 0 remains
   0, that case/whitespace variants reuse the normalized-name match, and that unmatched baseline definitions are
   copied once at consecutive cmpers beyond the source maximum.
10. Keep the mapping at **strong** confidence until exact legacy/Finale 27 pairs verify that the physical ordinal
    sequence upgrades to the same named `FontType` sequence. Use controlled one-font-at-a-time fixtures only for
    positions that remain ambiguous because several categories have identical defaults. Those fixtures should
    change a text typeface, point size, and each exposed style in separate saves and compare Finale's EnigmaXML
    conversion.

### FontOptions sequence-verification strategy

The physical sequence has not always used the current `FontType` semantics. Private framework history places the
tablature transition at Finale 2003; **measurement places it at Finale 2012, and the measurement governs.** From at
least Finale 98 through **Finale 2011**, physical 13 is a holding slot and physical 28 is default tablature;
**beginning in Finale 2012**, 13 is tablature and 28 is percussion. See [`format/options/font_options.md`](../format/options/font_options.md#the-1328-boundary-is-finale-2012-not-finale-2003), "The 13/28 boundary is
Finale 2012, not Finale 2003", for the 1,211 discriminating documents behind this and for why the corpus appeared to
agree with the framework history until Finale 2011 specimens existed. Physical 43 is
reserved through Finale 2006 and becomes `TimeParts` in Finale 2007. The sequence before Finale 98 and the physical
location before the verified Finale 2002 layout remain **open**.
Finale 27 upgrades of exact legacy files remain the primary semantic reference for observing the associated upgrade
transformations and font substitutions.

The source mappings are in
[`data/font_options_mapping.csv`](../data/font_options_mapping.csv). It deliberately has one row per physical layout,
plus targeted rows for the three confirmed Finale 1.0.0 categories. Variable arrays are walked to their physical
end without discarding holding or structural-fill tuples. Semantic completion is a separate pass and always
produces 45 keys. The selector-24 layout does not apply to Finale 1.0.0.

For each exact legacy/Finale 27 pair, construct two canonical views:

1. Resolve every candidate legacy tuple's font cmper through that source's `FontDefinition` pool and represent the
   tuple as `(normalized font name, signed size, effects mask)`. Keep its physical family, incidence, word/byte
   offset, and ordinal alongside it.
2. Parse the upgraded EnigmaXML through musxdom. For each named `FontType`, resolve its upgraded font cmper through
   the upgraded document's font definitions and construct the same canonical tuple. Treat omitted XML members as
   the values musxdom assigns, rather than comparing XML spelling or presence.

Do not compare font cmpers directly: Finale 27 may renumber definitions. Use the same case-folded,
whitespace-insensitive font-name key required by the importer. Compare the effects mask both as raw legacy bits and
as the booleans produced by `FontInfo::setEnigmaStyles`. Record conversion warnings and reject a font-name comparison
when Finale substituted a face rather than merely renumbering it.

For source ordinal `i`, first obtain its candidate semantic type from the era descriptor, then form the set of
upgraded `FontType` values whose canonical tuples equal that source tuple. Holding and reserved physical slots have
no modern semantic candidate and remain source-only observations.
Intersect those candidate sets over multiple, varied documents from the same saving product, then solve the
remaining one-to-one mapping while preserving physical order. This avoids claiming that an ordinal is `music`,
`clef`, or another category merely because several unchanged defaults happen to share the same font, size, and
effects. It also makes a disagreement visible instead of forcing the current enum order onto the evidence.

Run the comparison in three passes:

1. Add Finale 27 upgrades for the existing controlled Finale 2002–2005, 2007, and 2012 fixtures. These verify the
   already located arrays and the zlib transition against small, publishable sources.
2. Apply the analyzer to the existing exact source/export corpus pairs at every represented saving-product
   boundary. Use this breadth to find reordered positions, inserted categories, shorter arrays, and conversion
   outliers. An upgraded tail that has no source tuple is synthesized data and says nothing about the historical
   array.
3. Use controlled source-version edits for the remaining coverage gaps. The Finale 1.0.0 control and three edits
   have now confirmed Music, TextBlock, and LyricVerse; repeat that method for other exposed categories and then for
   1.8.7, 2.0.1, 3.8, 98, and 99 as needed. Scan plausible global records rather than presuming selector 24.

If repeated defaults leave an ordinal unresolved, make one controlled file in the originating Finale version and
give exactly one exposed default-font category a distinctive face, size, and effect combination. Save the legacy
file and upgrade that exact save in Finale 27. One changed source position paired with one changed named XML category
settles the ordinal; reset to the unchanged baseline before testing another category. Also save an unchanged control
twice so incidental save-time changes can be excluded. For Finale 1.0.0 this is the fallback, not the first step:
the paired stock fixtures may already contain enough variation to identify the representation without operating the
old application.

Record a separate era descriptor containing the physical location, available ordinal count, and ordinal-to-
`FontType` map. Promote a position to **confirmed** only when a publishable exact pair or controlled pair makes it
unambiguous; consistent private pairs are **strong**. Matching lengths, a stable prefix, or framework history alone
is structural evidence and cannot establish semantic order. The importer must not use one timeless ordinal map
until every era it claims to support is covered by such a descriptor.

## Consequences for the reader

The options problem is no longer wholly unmapped. A practical first implementation can:

1. create every musxdom options instance from a pinned Finale 27 new-document-without-libraries baseline;
2. decode and index every available legacy option record by `(tag, cmper, incident)`;
3. apply only table rows appropriate to the classified Finale era and platform;
4. overlay confidently decoded values into the candidate musxdom classes;
5. retain baseline defaults for absent, unverified, or unsupported fields; and
6. emit a capability report distinguishing recovered values from synthesized defaults.

The mapping must be data-driven and versioned. It should not be compiled into assumptions that every table row works
unchanged across Finale 2000, 2001–2006, 2007–2012, and the 26.2 compatibility API.

## Corpus verification of promoted mappings

**Confirmed** by running the reader over all 1,218 direct corpus files, last measured
2026-08-09. Eight option mappings are promoted: four `MusicSpacingOptions` fields from selector
`94(65534)` and the four `layerAtts` objects from `LA`, whose rest offset was the only recovered
member when this was measured; the class is now complete, and its own layout, flag word, and
release coverage are in
[`../format/others/layer_attributes.md`](../format/others/layer_attributes.md). Font definitions are recovered
separately and are not counted here; see
[font_definitions.md](../format/others/font_definitions.md#font-definitions).

| Era | Files | Music spacing 4/4 | Layer offsets 4/4 |
|---|---:|---:|---:|
| Coda banner, 1.8.7-2.6 | 54 | 0 | 0 |
| Finale 3.0-3.7 | 28 | 0 | 28 |
| Finale 97 | 70 | 0 | 60 |
| Finale 2000 | 92 | 90 | 92 |
| Finale 2001-2006 | 426 | 426 | 426 |

Three era facts follow:

- **Every framed DCL file recovers all eight values.** The sixteen that previously did not were
  failing on tag byte order, not on framing.
- **`LA` is present from Finale 3.0 and absent from the Coda-banner era, because layers were
  introduced in Finale 3.x.** The absence is therefore a fact about the era rather than a
  shortfall in the reader: a document written before layers existed has no layer attributes to
  recover, and the four `layerAtts` objects correctly keep their Finale 27 default rest offsets.
  This is the fallback strategy working as intended, and `ImportReport` records those four values
  as synthesized rather than recovered.
  **Refined 2026-09-03:** a release writes the row only once a layer setting leaves its default,
  which is why the 24 purpose-built Finale 3.7.2 fixtures in `tracked-evidence` carry none while
  these 28 authored files all do. The reader now supplies the era's own behavior for a layer with
  no row rather than the Finale 27 default, so the sentence above about synthesized rest offsets
  no longer describes what it does; see
  [`../format/others/layer_attributes.md`](../format/others/layer_attributes.md).
- **Ten Finale 97 documents carry no `LA` records** even though layers existed by then. That is a
  separate and unexplained case, and is **open**.
- **Selector `94` is not observed before Finale 2000.** Finale 3.0 through 97 recover layer
  offsets and no spacing values, and the Finale 1.8.7 file `mus-7aa45639c14b3864` carries
  comparator 65534 records under many other selectors but not `94`. The introduction point lies
  after Finale 97, internal 3.8, and no later than Finale 2000, internal 5.0. Finale 98, internal
  major 4, would settle it but is absent from the corpus and its release notes are not online.
  The likeliest explanation is that these spacing options were themselves introduced in Finale
  2000, which was a large feature release; that is a **weak** hypothesis, consistent with the
  observation but not independently established.

No version gate was added for the spacing mappings. A gate protects against reading a field that
*moved*; a field that is simply absent needs none, because the record lookup fails and the value
correctly reports as a synthesized default. Adding one would encode an unverified boundary while
changing no behavior. Font definitions are gated, because there the layout genuinely differs: files
before Finale 3.2 carry no header incidence.

This measures recovery, not accuracy. Only the fixtures with ETF counterparts independently confirm
that the recovered values are correct.

### Clef scalars: the same selector does not always mean the same thing

**Confirmed 2026-08-11** against all 1,120 adjacent-exact Finale 27 companions. Eight distilled rows
name locations for `ClefOptions` scalars: `clefDefault`, `clefReduction`, `clefDefaultOffset`,
`clefBefore`, `clefAfter`, `clefKeySpace`, `clefTimeSpace`, and `clefOnlyOnFirstSys`. Checking each
against its companion, per era rather than in aggregate, shows the distilled table is right for
Finale 3.0 onward and wrong for three rows before it:

| Rows | Coda banner | Finale 3.0–2000 | Finale 2001–2006 | Finale 2007+ |
|---|:--:|:--:|:--:|:--:|
| `01` w0, `13` w2, `13` w3, `19` w0, `19` w1 | 57/57 | 173/173 | 374/374 | 497/497 |
| `38` w5, `39` w4, `27` w1 bit 0 | **0/57** | 173/173 | 374/374 | 497/497 |
| `44` w3 bit 2, the courtesy clef | **absent** | consistent | confirmed by fixture | consistent |

The courtesy row is a ninth location, not one of the distilled eight: `courtesyFlags` packs the clef, key and time
courtesies into selector `44` word 3, at bits 2, 0 and 1. Controlled Finale 2005 saves identify the clef and key
bits. The Coda era stores the same three as separate boolean words in selector `12`, which is a further instance of
the same renumbering.

In the Coda era selector `27` word 1 and selector `39` word 4 hold font sizes, and are already read
as such by the FontOptions mapping; selector `38` word 5 disagrees with the companion on every Coda
file with a non-default value. The reader gates those three to Finale 3.0 and later.

Two general lessons follow, and both apply to the 429 rows still unpromoted:

- **A distilled row is an era-scoped claim, not a universal one.** These eight came from one
  preference-table snapshot. Five hold across every era observed; three do not, and nothing in the
  distilled CSV marks the difference. Verification must be per era.
- **A wrong location is quiet.** All three bad Coda rows read a real record and return a plausible
  number. Only comparison with a companion distinguishes that from a correct read, which is why
  aggregate agreement counts hide it: the three rows still show 54/57 overall agreement, because
  most files have the default value on both sides.

The zlib era needs no separate distillation. The same eight logical options are reached through the
established `numericGlobalClass` rule, comparator `65534` and byte offsets in place of word slots,
and all eight agree on all 497 compared files.

### One row verified in passing: the augmentation-dot upstem-flag adjustment

`DistancePrefs.dotFlagAdjust`, selector `21(65534)` word 0, is musxdom's
`AugmentationDotOptions::dotUpFlagOffset`. It is **corpus-confirmed from Finale 3.0 onward**: across
every sampled file with an exact companion, from Finale 3.0 through Finale 2012, the stored word and
the companion's `<dotUpFlagOffset>` agree, on values that vary between documents (0, 4, 8 and 24).
Nothing is implemented for it yet; this is recorded so that whoever implements
`AugmentationDotOptions` starts from a checked location rather than a distilled one.

The Coda era is **open**. Its files store 4 there while every companion reports 0, so Finale 27
discards the value as it discards that era's clef baselines. A controlled Finale 1.0.0 save reached
the word through a dialog offering "Offset" beside the two stem lengths
(`tests/evidence/F100/F100-stemopts-changed.mus`, 4 -> 5), which is consistent with the same option
under an older label but does not establish it.

`MiscDocPrefs.dotAdjustForMultipleVoices`, selector `27(65534)` word 0, maps to
`AugmentationDotOptions::adjMultipleVoices` beginning with the evidenced Finale 3.5 layout. The
three-survey recovery capture found 17 distinct companion-backed Finale 3.0-3.2 documents in which
the source location did not supply the field: the importer retained the pinned `true` default while
Finale 27 wrote `false`. This reviewed transformation is classified as `different_defaults` only
for the uncompressed epoch before Finale 3.5, with `Finale27Default` origin and the observed
`true`-to-`false` direction. Three Finale 3.5 documents are the earliest agreeing evidence. Finale
3.3 and 3.4 are absent from every survey, so the exact introduction release inside that bracket
remains **open**.

`DistancePrefs.dotOffset`, selector `21(65534)` word 1, is also present in the Coda-banner
epoch. The controlled Finale 2.6.3 fixture changes only that option from 8 to 13; its era ETF
preserves selector 21 word 1 as 13 and its Finale 27 companion carries `dotOffset` 13. No other
Coda-era `AugmentationDotOptions` field is located by this fixture.

### MMRestDefaultsPrefs: right for Finale 3.5 on, and a different record before it

**Confirmed 2026-08-16** against every adjacent-exact Finale 27 companion in the reference corpus. The group's
thirteen distilled rows name twelve locations on selector `25` and one on selector `83`, and all of them hold from
Finale 3.5 onward: 1,130 companion-backed documents agree on all nine scalars and on the character-rest-style flag,
with no disagreement in any era. The group covers every field of `MultimeasureRestOptions` except
`noHorizontalStretch`, which is a Finale 27 option no legacy format stores.

Before Finale 3.5 the record is a different shape. Selector `25` carries **one** incidence of six words rather than
two, and two of the three fields it still holds have moved: `vertNumAdj` is word 4 and `shapeID` word 5, where the
later layout puts them at words 2 and 3. All 264 Finale 1.8.7 through 3.2 documents of the reference corpus are on
that side of the line and all 3,458 later ones on the other; no file carries any other word count. That corpus has no
Finale 1.0.0 document at all, so the era's lower bound comes from the `tracked-evidence` survey, whose 19 Finale 1.0.0
fixtures carry the six-word record and are the only companion-backed documents of that version anywhere. This is the third instance of the same
lesson the clef scalars taught — a distilled row is an era-scoped claim — and the second boundary found at Finale
3.5, after the stem family's units.

The boundary cannot be expressed as an epoch gate, because it falls inside the uncompressed epoch, and a version
range would have to guess a cut point between 3.2 and 3.5 that no corpus can narrow. The reader reads the family's
own word count instead. See
[multimeasure_rest_options.md](../format/options/multimeasure_rest_options.md#multimeasure-rest-defaults) for both per-era tables.

Two further findings about the group:

- `mmautoupdate` is selector `83` **word 4**, as distilled, and word 2 of the same record is a different thing. 468
  companion-backed documents carry word 2 set with word 4 clear and none of their conversions has
  `<autoUpdateMmRests/>`; all 73 that carry word 4 do. The selector first appears in Finale 97.
- The two words the framework leaves as `AAAA` and `BBBB` are zero in all 3,458 later-layout documents, so nothing
  in the corpus can name them. They stay **open**.
- `noHorizontalStretch` is not open and is not a gap in the distilled table. "Stretch Horizontally" is a Finale 27
  feature, so no legacy format has a bit for it and the framework had none to name. Bit 0 is the only bit of the
  flags word any document uses, and the reader asserts the option false in every era.

### TextOptions: three distilled rows, and a class the framework does not model

**Confirmed 2026-08-16** against all 1,189 adjacent-exact Finale 27 companions of the reference corpus. This is the
first class where the framework survey was mostly a negative result and the corpus supplied the rest, so it is worth
recording what each source was actually good for.

The framework has no text-preferences class at all — 31 `__FCPrefsBase` subclasses and none for text — and the
437-row union contains exactly three rows that reach `TextOptions`: `MiscDocPrefs.secondsInTimeStamp` at `05` word 4,
`MiscDocPrefs.dateFormat` at `05` word 5, and `MiscDocPrefs.textTabChars` at `13` word 0. All three are now
corpus-confirmed on every era including Coda-banner, with no per-era exception of the kind the clef scalars turned
up. The framework also supplies the `DATEFORMAT_SHORT/LONG/MACLONG` values, and they match musxdom's `DateFormat`
one-for-one. Nothing in the tables or anywhere in the framework tree mentions the accidental symbol inserts.

The remaining eleven scalars and the whole insert array were located instead by searching the record stream for the
Finale 27 default values as a byte pattern, then diffing controlled one-variable saves. That method cost about the
same as reading the framework and produced more: five numeric globals for the scalars, and a direct five-element
block at `78(65534)` for the inserts. Full locations, per-era layouts and verification counts are in
[text_options.md](../format/options/text_options.md#text-options) and
[`data/text_options_mapping.csv`](../data/text_options_mapping.csv).

Four lessons generalize to the 429 rows still unpromoted:

- **A defaults fingerprint locates a record without any fixture.** The Finale 27 baseline states what the values
  should be; searching for that tuple across the record stream found the insert block in one pass. This works
  wherever a class has distinctive defaults, and it is cheaper than a controlled save.
- **Finale orders its alignment enums first, opposite, center.** `textJustify` stores `Left, Right, Center, Full,
  ForcedFull` where musxdom has `Left, Center, Right, Full, ForcedFull`, and `textVertAlign` stores `Top, Bottom,
  Center` where musxdom has `Top, Center, Bottom`; both need positions 1 and 2 exchanged. `textHorzAlign` needs
  no change only because `AlignJustify` already uses Finale's order. Each was settled by exactly one specimen —
  two corpus documents and one controlled save. Assume no enum matches, and check the rest of the enum-valued
  rows the same way.
- **A companion can be wrong in a way that is stable and era-specific.** Finale 27 mis-converts the Finale 3.7–2000
  insert layout on all 179 documents that have one, so aggregate companion agreement would have scored that era at
  zero and looked like a decoding failure. Eight later documents carry the same corruption frozen into the file,
  which is what proves the direction of the error.
- **The corpus is nearly useless for options nobody changes.** All 1,108 documents carrying selectors `82` and `83`
  hold identical values in four of their words. Two one-word fixtures closed three of those four and left the last
  identified by elimination; the corpus could not have closed any of them at any size. Volume does not substitute
  for variation.

### LyricOptions: nineteen distilled rows, two collections the framework does not model

**Located 2026-08-17 against the 69 tracked fixtures and their exact Finale 27 companions. No corpus survey has been
run for this class**, so nothing below carries the corpus weight the `TextOptions` and `MultimeasureRestOptions`
entries do.

`_FCEDTLyricsPrefs` is the framework's most useful lyric artifact, and the nineteen `LyricsPrefs` rows in
[`data/legacy_option_mappings.csv`](../data/legacy_option_mappings.csv) are all of it. They give five of the class's
locations — `15` word 1, `35` word 5, `57` word 0, `67` word 5, and the four syllable positions across two
incidences of `87` — and two numberings the corpus alone could not have supplied: the `LYRICS_ALIGN_` constants
(`1 = center, 2 = left, 3 = right`) and the `0x8000` "use this positioning" bit. Every one is corroborated by the
fixtures where a fixture varies at all.

What it does not have is as informative. **The framework models thirteen of the class's twenty-three fields and
nothing else**, and the two collections it misses are the two that matter most:

- **Selector `55`, the nine word-extension connection styles, is absent from the framework entirely** — no
  preference row, no direct-block entry, no accessor. It was located from the corpus, and one Finale 2006 fixture
  fixed the whole element layout and the connection-point numbering in a single document.
- **`hyphenChar`, `useSmartWordExtensions`, the alternate hyphen font, `wordExtNeedUnderscore`, edge punctuation,
  automatic numbering and the three auto-number flags are named nowhere in the framework tree**, and are also
  invariant across every companion in the fixture set — the worst case for locating a field, and the one a
  controlled save is normally for. Three of them turned out not to need one: `hyphenChar`, `useAltHyphenFont` and
  `altHyphenFont` **postdate Finale 2012**, so the framework had nothing to name and no save could have found them.
  The framework's silence was the right answer rather than a gap, which is worth remembering before commissioning a
  fixture for anything else it omits. They still land in three different places: the switch is asserted false, the
  hyphen character is inherited from the baseline because restating it would duplicate the pinned resource, and the
  font is not imported at all.

Two lessons add to the `TextOptions` list:

- **An enum numbering can come from a class the framework does not connect to the option.** The word-extension
  connection point is numbered on the smart-shape entry-connection scale, which reaches lyric-right-bottom at
  `0x10`. The framework has that enum under `FCSmartShapeEntryConnectStyle`, nowhere near its lyrics preferences,
  and it independently confirmed the base and the tail of an order the corpus had already fixed. It is also
  *incomplete* — it has no dotted-attachment entry — so it corroborates without governing.
- **A field can be stored once and modeled twice.** musxdom keeps the starting connection's offsets both as that
  connection's own and as the dialog-level `wordExtHorzOffset`/`wordExtVertOffset`. Only a fixture that moves the
  value off its default shows the two are one thing; two fields agreeing at their defaults proves nothing.
- **A null seeded member is a readable signal, and the right answer to it is usually to do nothing.**
  `altHyphenFont` has no element in the pinned `<lyricOptions>`, so the seeded object is a null pointer until
  musxdom's `integrityCheck` synthesizes one at the end of construction. That null is a reliable statement that the
  baseline omitted the element, available without reading the baseline's XML — but the conclusion to draw from it
  is that there is nothing to import, not that something must be built. A `FontInfo` the baseline *had* seeded
  would carry the baseline's font numbering and would need `importFontDefinitionInto`; a member it never filled in
  is absent rather than wrong, and filling it from the reference's own placeholder would put a value in the
  document that no document stated.

Full locations, per-era gates and the eleven open fields are in
[lyric_options.md](../format/options/lyric_options.md#lyric-options) and
[`data/lyric_options_mapping.csv`](../data/lyric_options_mapping.csv).

## Confidence and validation plan

| Claim | Status |
|---|---|
| Numeric tags with implicit comparator select `65534` | private-framework-derived; strongly ETF-supported |
| An incidence contains six 16-bit payload words | private-framework-derived; consistent with known 12-byte payload rows |
| Selector exists in ETF | independently text-verified where CSV column says `yes` |
| Semantic field and word slot | private-framework-derived; generally not independently verified |
| Four-byte word-order conversions | private-framework-derived; binary verification pending |
| Finale 26.2 replacement locations | private-framework-derived; outside the immediate 2001–2006 target |
| Candidate musxdom class | architectural inference; field-by-field verification pending |

Highest-value verification is a controlled Finale 2000 or 2005 document in which one visible option is changed at a
time and both MUS and ETF are saved. Tests should begin with `MusicSpacingPrefs`, `TiePrefs`, `PageFormatPrefsScore`,
`RepeatPrefs`, and `ChordPrefs`: each has a direct musxdom destination and compact, well-localized selectors.
