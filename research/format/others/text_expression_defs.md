# Text expression definitions

**Covers:** `TextExpressionDef`, inline expression-text synthesis, and comparison deferral.
**Read when:** Changing `src/import/others/text_expression_defs.cpp` or expression upgrade behavior.
**Confidence:** partial; confirmed mappings on the named tracked fixtures, with unresolved fields below.

## Record layouts

The fixed-row tag is `DT`; the zlib class is `0x00f1`. Each comparator identifies one definition.
**Confirmed:** the tracked F263, F2000, F2006, F2011, and F2012 samples in
the [investigation](../../investigations/text_expression_defs.md) establish these locations.

| Logical word | Before F2004 | F2004 and later |
|---:|---|---|
| 0 | Packed font and size | TextBlock reference |
| 1 | Font effects | Unused initially; category-specific rehearsal style from F2010 |
| 2 | Constant playback value or executable shape | Same |
| 3 | Auxiliary playback data | Same |
| 4 | Playback pass | Same |
| 5 | Playback type and flags | Playback type and flags |
| 6 | Start of inline text bytes | Horizontal measure alignment |
| 7 | Inline text | Horizontal justification |
| 8 | Inline text | Horizontal measure adjustment |
| 9 | Inline text | Note alignment; unused from F2009 (user-supplied) |
| 10 | Inline text | Note expression alignment; unused from F2009 (user-supplied) |
| 11 | Inline text | Note horizontal adjustment; unused from F2009 (user-supplied) |
| 12 | Inline text | Vertical measure alignment |
| 13 | Inline text | Measure vertical adjustment in F2004–2008 (user-supplied); category ID and category-use bits from F2009 |
| 14 | Inline text | Vertical note-expression alignment (user-supplied) |
| 15 | Inline text | Note baseline adjustment; modern baseline adjustment |
| 16 | Inline text | Note entry adjustment; modern entry adjustment |
| 17 | Inline text | Skipped word; meaning unresolved |
| Byte 36 onward | Inline text | NUL-terminated description; bytes or UTF-16 according to text epoch |

The F2004 gate is inside the DCL epoch. **Weak boundary evidence:** one private F2003 and one
private F2004 source bracket the user-supplied contemporary-documentation boundary. The tracked
F2006 fixture establishes the later layout but does not independently prove the exact F2004 boundary.
No length heuristic selects the layout: the older text trailer has arbitrary length.

**Restored provisional behavior:** the importer starts descriptions at byte 36 and ends at the
first NUL byte (platform text) or zero UTF-16 code unit (Unicode text). The byte-34 experiment
was reverted after substantial regressions. Zero bytes at both 34 and 35 precede descriptions
that some pre-F2012 companions preserve and others drop, so neither leading zero by itself
establishes stale text. The
[description-offset correction](../../investigations/text_expression_defs.md#description-offset-correction)
records the contrary evidence. Synthetic tests exercise byte order and surrogate pairs;
a controlled nonempty F2012 description is still wanted. Description layout/selection remains
open, with F2012 kept separate from the pre-Unicode evidence.

**Strong conversion-loss interpretation; user-approved scope for future comparison:** a recovered
nonempty description becoming empty in a paired companion is upgrade loss for pre-F2009 sources.
Recovery preserves the source description. The F2011 omitted-description case remains a separate
investigation. No executable difference rule currently applies because expression comparison is
deferred. See the
[conversion-path evidence](../../investigations/text_expression_defs.md#description-conversion-loss).

Alignment numbering shares the implementation in `src/import/support/expression_alignment.h`
with [marking categories](marking_category.md). The playback mapping and flag masks have their
single implementation in the expression importer; the public F2000 PDK and user-supplied
later flag definitions are leads, and only the tested subset is currently mapped.

## Attachment modes and positioning migration

**User-supplied interoperability account; conversion rules remain open.** Before F2009,
"Score Expressions" were measure-attached and "Staff Expressions" were entry-attached
details. F2009 discontinued the latter assignment form. When implementing `MeasureExprAssign`,
convert those legacy entry attachments into measure assignments using their entry context.
That assignment conversion is explicitly deferred, not part of the current definition work.

The older definition offered separate measure and note positioning controls. For F2004–2008,
the supplied eleven-word positioning sequence corresponds to logical words 6–16 in the
layout table above (the two fixed-row incidences after incidence 0). The vertical controls
were identified in the supplied account as a 2003-era addition. Do not extrapolate this extended
layout into the earlier inline-text record without evidence.

The immediate recovery problem is to combine the two older positioning sets into the expanded
measure-positioning model used from F2009 onward. Directly copying only the measure controls
does not establish that migration, and the former measure vertical adjustment must not be
interpreted as a category before F2009. Exact enum translations, offset combinations, and any
dependence on assignment type remain to be established with the user and fixtures. Do not
discard the note controls merely because the corresponding assignment form was discontinued.

Definition-level positioning conversion is current work; only importing the entry-detail
assignments themselves is deferred. **Confirmed for the sampled measure-positioning path:**
pre-F2004 horizontal and vertical measure anchors are manual (`LegacyBehavior`). In
F2004–2008, recover modern baseline adjustment from the former measure-Y word (`LegacyMus`),
not the separate note-baseline word. From F2009 use the modern baseline word. This provisional
measure path does not establish how a definition used for note attachments should be converted.
The current first-iteration experiment replaces only the F2004–2008 horizontal/vertical
selectors with translations of words 9 and 14; offsets and justification retain that measure
path. Before F2004 and from F2009 onward, selection is unchanged. TU-local enums own the
candidate legacy codes; recognized selectors report `LegacyMus` with their actual source
words and offsets, and unknown selectors remain `Unmapped`. **Contradicted as a universal
selection rule:** tracked companions disagree more often with this unconditional note-selector
choice. The mapping remains provisional pending review of the
[isolated capture](../../investigations/text_expression_defs.md#note-selector-first-iteration).

**Compatibility-interface evidence, not verified original wire values:** justification and
the supplied measure-anchor values agree with the existing shared helpers. The following
note-position table was initially misidentified as the original F2004–2008 field encoding.
**Superseded certainty:** these have not been established as original wire codes. Conversely,
the F2009 API argument alone does not establish that they are replacements: the plugin also
supports older running versions. Retain the table as candidate meanings for the separate
note fields. Verify the original X/Y controls and stored numbers before measuring conversion;
the plugin delegated that conversion to the legacy API. The
[F2008 manual investigation](../../investigations/text_expression_defs.md#positioning-layout-clarification)
provides original UI descriptions for planning those fixtures.

| Reported horizontal interface value | Anchor meaning |
|---:|---|
| 0 | Left of all noteheads |
| 1 | Manual horizontal position |
| 2 | Stem |
| 3 | Center of primary notehead |
| 4 | Center of all noteheads |
| 5 | Right of all noteheads |
| 6 | Left of primary notehead |

| Reported vertical interface value | Anchor meaning |
|---:|---|
| 0 | Vertical click position (first-iteration choice) |
| 1 | Above staff baseline |
| 2 | Below staff baseline |
| 3 | Top note |
| 4 | Bottom note |
| 5 | Above entry |
| 6 | Below entry |
| 7 | Above baseline or entry |
| 8 | Below baseline or entry |

**Compatibility behavior in authorized plugin history:** the older note defaults are exposed
as horizontal value 1 and vertical value 3; measure defaults are manual in both directions.
These are interface defaults, not proof of stored pre-F2009 note-field encodings.
The old note fields are deprecated and may be unreliable in F2009+; do not let their stale
values override the modern measure-positioning fields. Shape-expression reuse remains a
candidate until its layout and conversion are verified. The history and distinguishing
fixture observations are in the [positioning investigation](../../investigations/text_expression_defs.md#positioning-layout-clarification).

Description conversion loss is distinguished from source layout in [record layouts](#record-layouts).

## Early synthesis and IDs

**Confirmed for the observed style value 4:** logical word 1 stores rehearsal-mark style in
the tracked F2011 sample. The importer explicitly maps stored numbers to musxdom styles;
zero means non-incrementing text. F2010 and later map recognized values with `LegacyMus`
provenance; unknown values remain `Unmapped`. Earlier sources retain non-incrementing text
with `LegacyBehavior` provenance. **Weak boundary evidence:** the F2010 introduction
comes from the user's contemporary-documentation comment, not a controlled F2009/F2010 pair.
See the [investigation](../../investigations/text_expression_defs.md#rehearsal-style).

**Confirmed on the controlled F2011 rehearsal pair:** the upper bit of word 5 controls
`hideMeasureNum`. It reports `LegacyMus` under the same proposed F2010 gate as rehearsal
sequencing; earlier sources retain false with `LegacyBehavior` provenance. The importer owns
the mask. This later meaning must not replace the pre-F2004 inline-text pound-replacement
interpretation. The pair verifies F2011 behavior, not the exact repurposing boundary.

**Confirmed on the F2011 match-playback variant:** word 5 also supplies `matchPlayback` through
its independently mapped bit. Use the same provisional F2010 gate: recovered values report
`LegacyMus`, earlier false values `LegacyBehavior`. **Weak boundary evidence:** the user chose
F2010 based on their online research; no F2010 source or cited search result establishes it here.
The [investigation](../../investigations/text_expression_defs.md#match-playback) records the
isolated changes and distinguishes this control from tempo playback and its beat duration.

**Confirmed on F263:** the font ID occupies the high byte and size the low byte. F2000 uses the
opposite order. `src/import/support/legacy_font.h` owns packed-font assignment. Inline text starts
at byte 12 and ends at its first NUL; row padding can contain nonzero stale bytes after that NUL.
Decode through the stored font, preserve its style state, escape literal carets, and convert
the documented pound replacements to Enigma inserts. **Confirmed on the controlled F2002
partial-hide and unbalanced fixtures:** when the no-print flag is set, an opening angle bracket
starts hidden text, even without a closing bracket. The controls are removed from the
synthesized text. A closing bracket restores the base effects; repeated controls do not nest
(user-supplied interpretation, awaiting contrasting fixture evidence). Text outside the spans retains its font
effects. **User-supplied boundary:** this applies in F2002–2003; before F2002 the flag hides
the entire expression, including literal brackets. Flag-clear text retains brackets literally.
Pound replacement, flag-clear bracket behavior, and the exact F2001/F2002 boundary still need
contrasting controlled fixtures. See the
[partial-hide investigation](../../investigations/text_expression_defs.md#partial-hidden-inline-text).

The importer creates one ExpressionText and one TextBlock per definition, in comparator order,
after ordinary text-pool materialization. It uses musxdom's allocator, which appends after the
highest ID. **Weak:** companion samples sometimes append TextBlocks and sometimes reuse gaps;
matching exact IDs is therefore not the construction contract. Companion ExpressionText numbers
are sequential in the sampled definition order.

**Confirmed on tracked F263 and F2006:** pre-F2009 expressions receive the imported category whose
type is Misc. **Weak:** the broader small private sample supports the same behavior. The category
importer owns the canned category population; the expression importer resolves it after import.

From F2009, the category word's two upper bits independently select category fonts and
positioning; the importer owns the numeric masks. Both fields report `LegacyMus` for every
bit combination. **User-supplied mapping:** the individual bit meanings still need contrasting
controlled fixtures; the observed both-set/both-clear cases alone cannot distinguish them.

## Coverage and remaining work

**Confirmed on one tracked F2006 fixture:** SmartMusic playback is removed on modern upgrade.
Normalize that playback type to None and its auxiliary selector to zero, retaining `useAuxData`
and unrelated fields. Both normalized fields report `LegacyMusAdjusted` with the original words
and offsets; other playback types retain their auxiliary data. The selector's meaning does not
limit the normalization to rehearsal/repeat markers. The user supplied the v27.1 removal date;
the [byte and companion observations](../../investigations/text_expression_defs.md#smartmusic-normalization)
verify removal, not its exact release boundary.

All 23 persisted class fields have reader origins. **Temporary comparison exclusion:** the probe
still imports and surveys TextExpressionDef so reader failures remain visible, but comparison
preparation removes the definitions, all raw expression texts, and expression-type TextBlocks from
both source and companion snapshots. No expression leaf contributes to recovery-report counts.
Restore comparison after expression assignments can provide reliable identity and attachment
context. This does not change imported values; the class remains partial. Earlier correspondence
experiments and their contrary results remain in the
[investigation](../../investigations/text_expression_defs.md#ordinal-expression-correspondence).

Recovery reaches all four container epochs; none is wholly excluded. Remaining scope:

- `createdByHp` in later sources.
- Controlled rehearsal-style variants and the exact F2010 boundary.
- Controlled mixed category-font/category-position flag combinations.
- Pre-F2004 justification and adjustments; early `breakMmRest`.
- Additional playback types absent from the mapped PDK subset.
- Pre-F2009 positioning upgrades.
- Category and executable-shape reference equivalence beyond stable source IDs.
- Exact early-version and platform coverage beyond the controlled samples.

No field is labeled `MusxOnly`. Historical companion results are retained in the investigation,
but current reports deliberately make no expression-recovery comparison claim.
The current tracked capture and ordinary test results are in the
[investigation](../../investigations/text_expression_defs.md#note-selector-first-iteration).
