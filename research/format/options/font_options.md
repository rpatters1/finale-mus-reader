# FontOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Later pre-zlib default-font array

**Private-framework-derived; strongly ETF-supported for Finale 2002–2005.** In those versions the `FontOptions`
array is selector `24`, comparator `65534`. It is a packed array of three-word font tuples rather than one font per
physical incidence:

| Physical tuple index `n` | Incidence | Font ID | Size | Effects |
|---:|---:|---:|---:|---:|
| even | `n / 2` | slot 0 | slot 1 | slot 2 |
| odd | `n / 2` | slot 3 | slot 4 | slot 5 |

The framework's zero-based default-font preference numbers are versioned rather than one timeless musxdom enum
order. From at least Finale 98 **through Finale 2011**, index 13 is a legacy drawing-time tablature holding slot and
index 28 is the actual default tablature font; earlier versions remain open. **Beginning with Finale 2012**, tablature
uses index 13 and index 28 is percussion. Indices
40–42 are also present beginning in Finale 2003. The controlled Finale 2002 ETFs contain 20 incidences, enough for
indices 0–39. The Finale 2003–2005 ETFs contain 22, enough for indices 0–43; index 43 is the zero-filled second tuple
required to complete the final fixed 16-byte row. Each baseline/changed pair has
an identical selector-24 array, as expected because the controlled edit changed a note rather than a document font.

### The 13/28 boundary is Finale 2012, not Finale 2003

**Confirmed** by measurement, and it **contradicts the private-framework history** these notes previously followed,
which places the transition at Finale 2003. Where the two disagree, the measurement governs and is what the importer
implements; see the comment on `semanticType` in `src/import/options/font_options.cpp`.

The earlier claim that the boundary "independently fits the exact-pair corpus" was true but not discriminating. It
was tested against Finale 2002 sources, and Finale 2002 precedes *both* candidate boundaries, so those documents are
consistent with either. Nothing in the corpus distinguished them until Finale 2011 specimens existed, because
Finale 2011 is the only version whose behavior differs between the two hypotheses.

Across 1,211 documents whose Finale 27 companion assigns tablature and percussion different values — the only
documents that can discriminate — the arrangement is:

| Source version | Documents | Arrangement |
|---|---|---|
| Finale 2003–2010 (majors 8–15) | 405 | index 28 is tablature; 13 is a holding slot |
| **Finale 2011 (major 16)** | **597** | index 28 is tablature; 13 is a holding slot |
| Finale 2012 (major 17) | 209 | index 13 is tablature; index 28 is percussion |

No document contradicts this on either side of the boundary, on either platform. Coding the boundary at Finale 2003
cost every Finale 2003–2011 document its tablature font and gave it a percussion font it never stored: 2,516 of the
2,629 FontOptions disagreements then present in the corpus were this single rule.

The Finale 2011 specimens that settled it came from the Finale 2011 install DVD; no Finale 2011 document existed in
any survey before that. Comparison must normalize font names with musxdom's `normalizeFontName`: `EngraverTextT` and
`Engraver Text T` are one face, and comparing raw spellings produced 324 false disagreements that made Finale 2011
look internally inconsistent.

Across 42 distinct
Finale 2002 sources, index 28 carries the value upgraded into modern tablature, while index 13 is not an independent
modern default. Across all 329 distinct Finale 2003–2006 exact-pair sources, physical index 43 is `(0, 0, 0)`. This
is structural row fill, not a terminator or a version-encoded collection limit. Physical capture therefore walks
every complete tuple; semantic insertion ignores only a zero-filled second tuple at the end of a fixed row.

The selector and packing are established well enough to locate every stored font ID, size, and effects word in this
verified range. Their semantic values remain **strong**, not `confirmed`, until a controlled file changes one
default font at a time. This layout must not be selected merely because a file predates zlib; Finale 1.0.0 proves
that selector meanings and layouts changed inside the broad pre-zlib era.

The same array is present, with the same packing, well before Finale 2002. The Finale 97 and Finale 2000 fixtures
each carry 20 incidences of selector `24`, and their tuples agree with their exact Finale 27 companions type for
type: `(0, 28)`, `(0, 28)`, `(0, 24)`, `(0, 26)`, `(2, 12)` for music, key, clef, time, and chord. Finale 3.7.2
shows the same array with that document's own sizes.

The reader recovers these. Selector 24 is the default-font array in every fixed-row epoch except the Coda banner, so
the layout is selected by epoch rather than by a version range, and 6,100 recovered sizes across 173 uncompressed
files agree with their exact companions. The physical-to-semantic quirks of the era are the same ones the DCL era
has before Finale 2003: physical slot 13 is not an independent value and physical slot 28 carries tablature.

## Finale 1.0.0 fonts

**Confirmed across the installation survey.** All 22 distinct Finale 1.0.0 specimens contain exactly five `FN`
families, with font-definition cmpers 0–4. Every specimen has the same five-name table. Consequently, importing any
modern fallback `FontInfo` whose font ID is outside 0–4 can silently resolve to an unrelated definition if comparator
spaces differ, or remain dangling; neither result is a safe fallback.

Every surveyed specimen also contains exactly one `24(65534)` row, but its two apparent triples are `(13, 69, 52)`
and `(48, 65, 60)`. Both putative font IDs exceed the definition range, and the values do not resemble font sizes or
effect masks. Selector 24 therefore is **not** the later default-font array in Finale 1.0.0.

**Confirmed by newly authored controlled Finale 1.0.0 fixtures.** Thirteen one-variable UI saves locate every font
preference exposed by Finale 1.0.0. Twelve map directly to modern FontOptions; the historical `Name` preference is
treated as the predecessor of `StaffNames` under the additive-only early-version hypothesis:

| Semantic type | Font ID | Size | Effects |
|---|---|---|---|
| Music | `02(65534)` word 0 | word 1 | word 2 |
| Key | `03(65534)` word 3 | word 4 | word 5 |
| Clef | `04(65534)` word 0 | `39(65534)` word 4 | `39(65534)` word 5 |
| Time | `03(65534)` word 0 | word 1 | word 2 |
| Chord | `02(65534)` word 3 | word 4 | word 5 |
| ChordAcci | `37(65534)` word 0 | word 1 | word 2 |
| Ending | `05(65534)` word 0 | word 1 | word 2 |
| Tuplet | `36(65534)` word 0 | word 1 | word 2 |
| TextBlock | `26(65534)` word 0 | word 1 | word 2 |
| LyricVerse | `26(65534)` word 3 | word 4 | word 5 |
| LyricChorus | `27(65534)` word 0 | word 1 | word 2 |
| LyricSection | `27(65534)` word 3 | word 4 | word 5 |
| StaffNames (historical `Name`) | `04(65534)` word 3 | word 4 | word 5 |

Clef is physically split across two records. The Tuplet save also changes the ChordAcci tuple, and Finale 27
preserves both changes. Raw effects `0x08` and `0x10` occur in controlled saves but are not among musxdom's six
represented Enigma style bits; reporting retains the raw mask while `setEnigmaStyles` expands the supported bits.
Finale 27 drops the controlled historical `Name` change, so its continuation as `StaffNames` is **strong**, not
confirmed. It also changes some unedited categories, demonstrating upgrade synthesis and shared-preference
behavior; those changes are not evidence for additional source locations.

### The single `Name` preference reaches all four modern name types

The Coda-banner era exposes one `Name` font preference. Finale 3.0 replaced it with four — `StaffNames`,
`AbbrvStaffNames`, `GroupNames`, `AbbrvGroupNames` — which that era stores as separate tuples at physical ordinals
31, 32, 33 and 39. The importer therefore propagates the one recovered Coda tuple to all four types, and this
fan-out is gated on the Coda-banner epoch alone so that it can never overwrite the three independently recovered
values of any later epoch.

Recovering `StaffNames` alone would emit a document whose staff names use one face and size while its group and
abbreviated names use the Finale 27 default of Times New Roman 14 — a split neither the source nor the Finale 27
baseline has, since that baseline sets all four identically. The Finale 3.7 `F372-baseline` fixture, which never
touched these preferences, likewise carries all four as Times 12.

The three propagated types report as `ValueOrigin::LegacyBehavior` rather than `LegacyMus`: the bytes are read from
the source, but the assignment restores an era behavior rather than an option the source stored.

**This is a deliberate, revisitable divergence from the Finale 27 companions.** Across the 57 Coda-era documents
with companions, every companion disagrees with the recovered value: 39 report `Times 16`/`Times 14`, 17 report
`Monaco 16`/`Monaco 14`, and one reports `Pmusic 12` throughout, while the source tuple reads `Times 14` in all 57.
Those companion values track the personal default file the upgrade was performed under rather than the source
document, so they do not settle what Finale 27 does with a stored `Name`. Settling it needs a companion produced
under a stock default file.

The same `02`, `03`, `04`, `05`, `26`, `27`, `36`, `37`, and `39` global families persist through the Finale 1.8.7,
2.0.1, and 2.6 corpus. Under the working hypothesis that this interval only adds font preferences, the importer
uses the Finale 1.0.0 mappings through 2.6 and leaves any later additions at safely remapped Finale 27 defaults.
This extension is **strong**: 188 distinct readable early sources preserve the relevant physical records, and 53
adjacent-exact Finale 2.6 companions independently support Chord, Ending, and Tuplet. These newly authored Finale
1.0.0 documents contain 23 font definitions, unlike the five-definition installation cohort, so they establish
locations without changing that earlier census.

This rules out keeping the pinned `FontOptions` as a completeness skeleton, because its numeric IDs belong to the
baseline's font-definition table. The reader should filter that whole object out and create a fresh, fully populated
`FontOptions`. Era-verified source tuples are copied directly. Each missing type is synthesized from a separate,
fully populated platform-matched baseline document: baseline font cmper 0 remains 0; every nonzero baseline font is
matched to a target definition by normalized name, or its complete `FontDefinition` is cloned at the next target
cmper when no match exists. The normalization removes whitespace and folds case, matching musxdom's existing font
normalization so PostScript and family spellings compare equal. Normalization is used only to find a match: when a
definition must be cloned, its name retains the selected platform reference document's exact spelling. This
new definition receives the next sequential comparator after the target's highest existing font comparator; the
reference comparator is never copied into the target id space. This preserves all 45 keys without allowing a
baseline ID to resolve accidentally to an unrelated legacy font.

This completion is deliberately not an attempt to reproduce every choice Finale makes while upgrading a document.
For example, Finale 27 derives a Percussion preference from Music in controlled pre-2003 upgrades, but no separate
source location has been found. The importer therefore takes Percussion from the selected reference document. More
generally, a legacy MUS import will remain less complete than a native MUSX document; coverage improves only as
additional source fields are identified with sufficient confidence.

## Zlib default-font array

**Strong.** The same array is the singleton class record `0x0026(65534)`, incidence 0, in the `0x001a` block. Both
controlled zlib fixtures carry a 276-byte payload: 46 consecutive six-byte tuples in file byte order. Tuples 0–44
map to musxdom's 45 `FontType` values and tuple 45 is the zero-filled second tuple of the final 12-byte pair:

| Logical `FontType` index `n` | Font ID | Size | Effects |
|---:|---:|---:|---:|
| 0–44 | byte `6n` | byte `6n + 2` | byte `6n + 4` |

The 23 tuple pairs are consistent with the fixed-row representation's two tuples per Enigma record and with Finale
continuing to expose that record model to plug-ins, even though the zlib class record itself is length-governed.
Each member is a 16-bit word. Font ID is a font-definition cmper, size is the point size, and effects is an Enigma
style mask. The effects word must be expanded through musxdom `FontInfo::setEnigmaStyles`, yielding the `bold`,
`italic`, `underline`, `strikeout`, `absolute`, and `hidden` booleans rather than being assigned as one scalar field.

The identification is supported by continuity, not merely payload size. The first 43 tuples in the big-endian
Finale 2007 fixture exactly equal the corresponding Finale 2005 ETF tuples. Finale 2005 then has an unused zero
half-incidence; Finale 2007 fills indices 43 and 44 and moves the zero tuple to index 45. The little-endian Finale
2012 fixture has the same tuple organization with document-specific font IDs, sizes, and effects. The public record
catalog observes class `0x0026` with length 276 in every represented zlib product year from 2006 through 2012.

This also exposes a general options bridge. For every numeric global selector present in the Finale 2005 ETF, the
controlled Finale 2007 options prefix contains class ID `selector + 0x000e`; it adds selector 47 and omits none.
The Finale 2012 fixture follows the same transform, with the expected version-specific additions/removals. For
example, selector 24 becomes class `0x0026`, selector 94 becomes `0x006c`, and selector 98 becomes `0x0070`.
Therefore zlib did not replace the option schema wholesale: it coalesced each old selector family's incidences into
one length-governed class payload, while retaining comparator `65534` and the payload's logical word sequence.

The physical location and organization are strong enough to implement. The semantic claim remains **strong**, not
`confirmed`, until a controlled zlib file changes one default font at a time and a trusted conversion verifies the
result.

The mapping is incomplete: it covers 61 numeric globals while current ETFs contain 35 additional numeric globals not
mapped by the framework. Historical and Finale 26.2 replacement locations also prove that mappings can be
version-dependent. Earlier statements that option code names and layouts were wholly unknown are superseded by this
partial map.
