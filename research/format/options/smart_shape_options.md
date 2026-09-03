# SmartShapeOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Smart Shape options

`SmartShapeOptions` is **implemented**: all 41 scalar fields and all five collections are recovered
or resolved where their source families exist. `maximumShortHairpinLength` and
`articAvoidSlurAmt` postdate MUS and are reported as `MusxOnly`. The remaining Coda-era scalar
locations cannot be narrowed further from controlled fixtures; they are future real-world evidence
opportunities rather than implementation gaps.
Source-owned `ssLineStyleCmp*` values retain the comparators of the imported
`SmartShapeCustomLine` records. Before custom lines existed, the glissando and tab-slide fields resolve
to copied baseline definitions; the tab-bend curve similarly resolves to a copied baseline definition
before its tool arrives in Finale 2003. The remaining external-reference audit is
tracked in [`OPTIONS_EXTERNAL_CMPER_TODO.md`](../../state/OPTIONS_EXTERNAL_CMPER_TODO.md).

The source layout is selected structurally rather than by the document's marketing version:

- A complete selector `52` family contains exactly twelve words. Before selector `97` appears,
  the first nine words supply the short, medium, and long `(span, inset, height)` tuples and the
  last tuple is a zero placeholder. Upgrade behavior constructs the extra-long style by retaining
  the later baseline span and copying the recovered long inset and height. With selector `97`, all
  four tuples are source-owned.
- Selector `97` marks the later scalar slur family at selectors `50`, `51`, and `53`. Earlier files
  reuse parts of those three selectors for a different layout, so none is sufficient by itself.
  The first three words of selector `53` are the stable exception: horizontal break adjustments
  and vertical break adjustment are readable whenever the twelve-word contour family is present.
  The later family adds the avoid-staff-lines flag at word 3, padding at word 4, maximum angle at
  word 5, and its second incidence.
- In that later family, selector `50` word 5 stores a positive staff-line tip-avoidance amount
  one greater than the displayed value. A controlled Finale 2002 edit from 8 to 17 changes only
  that word from 9 to 18, and the modern companion preserves 17. Six distinct paired survey files
  instead store zero while their companions use 8. The meaning of zero is not established; the
  importer treats it as unusable for recovery and retains the pinned baseline value and
  `Finale27Default` origin. The positive transform is **confirmed** and the zero treatment is
  **strong**.
- In the initial enhanced-slur layout, selector `53` has only one incidence. General slur padding
  also supplies accidental padding, and the initial adjustment does not stretch first; both are
  reported as `LegacyBehavior`. A second incidence introduces independent values for those fields.
- Selector `92` marks the four custom-line references and the associated selector `93` slur-tip
  width.
- The four connection-style collections are signed-word triples in collection-enum order:
  `(connection index, horizontal offset, vertical offset)`. The stored connection index is the
  ordinal of musxdom's `ConnectionIndex`: head, stem, and note anchors in the legacy order from
  `HeadLeftTop = 0` through `NoteRightCenter = 13`. Selector `26` carries slur connections,
  selector `90` tab-slide connections, selector `91` glissando connections, and selector `98`
  bend-curve connections. Their zlib identities are respectively `0x0028`, `0x0068`, `0x0069`,
  and `0x0070`, following the numeric-global class bridge.
- The slur collection's payload length states which semantic prefix exists. Finale 2.6.3 stores
  two styles in one incidence. Finale 97 through 2002 store 25 styles followed by one structural
  zero triple in thirteen incidences. Finale 2003 and later store all 29 styles followed by the
  same structural triple in fifteen incidences. The reader overlays that prefix and retains the
  seeded values for unavailable later styles. Tab-slide, glissando, and bend-curve payloads have
  no trailing triple and contain exactly 18, 2, and 8 styles when present. Absent families retain
  seeded values with `Finale27Default` origin.

Finale 27 companions do not necessarily serialize all 29 slur connection styles. Across the
combined companion population they contain 4, 25, or 29 keyed styles even within the same source
version. A discriminating Finale 2003 trio retains all 29 source tuples in each document: the
29-style companion includes every type; the 25-style companion omits the four zero-valued tab
types; and the 4-style companion contains only the four nonzero tab types while omitting 25
zero-valued types. Thus the shorter collection is sparse serialization, not a source-version
layout. The reader continues to construct the complete 29-style options array. Coverage classifies
an omitted object as `finale-upgrade-loss` when any of its source-owned semantic values is nonzero,
and otherwise as `reader-completed-connection-array`; the object's `type`, `connectIndex`, X, and Y
leaves receive the same object-level disposition.

- Selector `10` incidence 0 word 0 stores the default Smart Shape direction from Finale 2002
  through the fixed-row DCL epoch; its zlib class `0x0018` retains the same word. The stored enum
  is exactly `-1 = Under`, `0 = Automatic`, and `1 = Over`. Controlled Finale 2002, 2005, and 2008
  edits agree, and the F2008 companions preserve all three meanings. Earlier epochs retain the
  pinned baseline's Automatic direction. Five old-origin documents later saved by Finale 2008 or
  2012 instead carry the out-of-domain value 103; all five companions use Automatic. The importer
  rejects any value outside the three-member enum and retains `Finale27Default` rather than
  coercing it by sign.
- The named `FI` family at comparators 11 and 12 supplies the hairpin, hook, line, dash, and octava
  settings. In Finale 3.0 through 3.6, comparator 11 word 1 is not an independent crescendo-line
  width: modern companions derive that leaf from the smart-line width in word 4. Word 2 likewise
  does not provide the later hook-length option in this range, so the importer retains the pinned
  baseline hook length. Finale 3.7 makes both words independently meaningful. The importer applies
  these rules only to that version range inside the uncompressed epoch; a missing version does not
  match. In the zlib epoch class `0x008d` preserves those comparators and payloads.

The fixed-row numeric selectors become their established numeric-global classes in the zlib epoch:
`50`–`53` become `0x0040`–`0x0043`, `92`–`93` become `0x006a`–`0x006b`, and `97` becomes
`0x006f`. The named `FI` family is the exception described above. The current authorized framework
provided the initial field locators; that lead is **private-framework-derived**. The word positions,
long-word order, transformations, and fixed-to-class bridge are independently confirmed against the
controlled legacy records and their semantic companions from Finale 3.7.2 through 2012.

The crescendo-width boundary is **strong**. Nineteen paired Finale 3.0, 3.2, and 3.5 sources have
word 1 values of 0, 32, or 72 while their companions use word 4 values of 111 or 118 for both line
widths. Those sources store word 2 as 0 or 8 while every companion uses the baseline hook length
12. A discriminating Finale 3.7 source stores 144 in word 1, 12 in word 2, and 118 in word 4; its
companion preserves those values independently. No paired Finale 3.1, 3.3, 3.4, or 3.6 specimen is
available, and the six-word record has no identified structural change at the boundary.

The printed Finale 3.2 addendum documents “enhanced smart shapes,” including entry-attached slurs,
slur contours and connection types, plus Slur Thickness, Line Thickness, Dash Length, and Dash
Space. The Finale 3.7 addendum identifies the Smart Shape Options dialog, including Hook Length,
as a 3.7 enhancement. This independently confirms the hook-length introduction boundary. The
independent crescendo-line-width boundary remains strong from the binary and companion evidence
rather than confirmed from the addenda.

The selector `97` structural boundary coincides exactly with Finale 2002 in the larger installation
survey: all 63 Finale 2001 documents have a zero fourth selector-`52` tuple and selector-`53` word 3
clear, while all 774 Finale 2002 documents have a nonzero fourth tuple and word 3 set. MakeMusic's
later documentation independently describes the new Engraver Slurs as the conversion boundary for
Finale 2001-and-earlier files. A controlled Finale 97 edit changes all three stored contours while
leaving the final tuple zero; its Finale 27 companion copies the edited long inset and height into
the extra-long contour and supplies span 1152. Separate tie-control edits affect only `TieOptions`.

Finale 2002 has the enhanced family marker but only the first selector-`53` incidence. Across its
seven tracked fixtures, Finale 27 copies general `slurPadding` to `slurAcciPadding` for three
discriminating values: 12, 18, and 37. The controlled 37-EVPU edit also disables accidental
avoidance, confirming that the copy does not depend on that flag. Every companion keeps
`slurDoStretchFirst` false. Finale 2003 adds the second incidence and stores accidental padding and
stretch-first independently. The importer therefore selects the two Finale 2002 behaviors by the
record family's six-word shape rather than by version. This is **confirmed** by the tracked fixture.

A Finale 2000 discriminator rejects the more specific hypothesis that the synthesized thickness
controls come from editable tie thickness. Relative to its untouched parent, it changes selector 84
`thicknessRight` from 6 to -17 and `thicknessLeft` from 6 to 11, but its Finale 27 companion leaves
both Smart Shape thickness-control Y values at 6 and changes only `TieOptions`.

Tie contour insets are excluded separately. A controlled Finale 2000 edit switches the ordinary
contours to fixed insets and changes their two selector-86 inset values from 8/8 to 17/13. Finale 27
preserves all six edited inset positions in `TieOptions` but leaves both Smart Shape thickness-control
Y values at 6.

The source is selector 59 incidence 0 word 5. A controlled Finale 2000 Slur Thickness edit changes
that word alone from 6 to 17, and its Finale 27 companion changes exactly `slurThicknessCp1Y` and
`slurThicknessCp2Y` from 6 to 17. Before selector 97 introduces independent control-point values,
this one stored thickness supplies both vertical controls; the horizontal controls remain at their
seeded defaults. The importer selects this rule structurally with the twelve-word selector-52 contour
family and absence of selector 97. The earlier correlations with tie settings and default music-font
size were properties of two document templates, not source mappings.

The Coda-banner epoch remains mostly **uncovered**. Finale 1.0.0 and 2.6.3 both contain some of
selectors `50`–`53`, but selector `52` has a six-word floating-point-shaped payload rather than the
twelve-word contour family, and most of the other words do not match the later scalar meanings.
Neither file contains `FI`, `92`, `93`, or `97`. Except for the four thickness fields located below,
the reader retains and reports the pinned Finale 27 defaults rather than interpreting the colliding
rows. The remaining Coda Smart Shape locations and meanings remain **open**.

A controlled Finale 2.6.3 curve-options edit now confirms that this epoch does store some of those
preferences. Relative to `mus-ec910badddfa6699`, `mus-e0d207b83365f47d` changes six-word global
selectors `15`, `35`, `50`, and `51`; its ETF independently reports the same words. Finale 27
distributes the edit across several modern option classes. Within `SmartShapeOptions`, the only
changed leaves are `slurThicknessCp1Y` and `slurThicknessCp2Y`, both 6 to 13. A second discriminator
changes selector `51` from `(0, -6, 0, -6, 0, 8)` to `(-13, 17, -15, 19, 3, 5)`, exposing two
`(horizontal, vertical)` thickness-control pairs. Horizontal values map directly; vertical values
map with their signs inverted. A third fixture, whose first four words are
`(131, -171, -173, 179)`, includes a rendered legacy curve and confirms the source-faithful result
`(131, 171, -173, -179)`. Finale 27 instead discards both horizontal values and copies the first
vertical result to both control points, visibly distorting the upgraded curve. The reader preserves
all four source values rather than reproducing that lossy upgrade. A manually corrected modern
companion, SHA-256 `e8f361373c9962ddae60844da1b597ecaf19494c0c05194d85d654333ee26e95`,
stores exactly `(131, 171, -173, -179)` and reproduces the legacy curve's appearance. Tie-end
values and additional non-PostScript tie edits do not populate `SmartShapeOptions`. Coverage records
the resulting Coda `LegacyMus` disagreements in `Cp1X`, `Cp2X`, and `Cp2Y` as
`finale-upgrade-loss`; `Cp1Y` is excluded because Finale upgrades it correctly. That shared
classification names confirmed loss of source-owned values during a Finale upgrade; its executable
rules remain narrowly gated for each independently established case.

The Coda-banner files contain no stored `FI` hook-length preference, but the semantic companions
separate their behavior by version: all surveyed Finale 2.6.3 companions use 8 EVPU, while Finale
1.0.0 companions and later source-owned `FI` values use 12. A controlled Finale 2.6.3 curve edit
also changes selector `51` word 5 from 8 to 5 without changing the companion hook length, excluding
that tempting correlation. The reader therefore assigns 8 as `LegacyBehavior` only when the source
profile is both Coda-banner epoch and version 2.6. This version gate is necessary because the
record structure cannot distinguish the two observed Coda behaviors. Confidence is **strong**.

Legacy formats have one hairpin opening rather than differentiated short and long openings. The
reader therefore assigns `shortHairpinOpeningWidth = crescHeight` unconditionally and reports the
short value as `LegacyBehavior`; where `FI` exists, `crescHeight` comes from comparator 11 word 0,
and where that source remains unlocated it retains the pinned baseline value. The separate short
opening and maximum-short-span settings both arrive in Finale 25.3, after the last MUS format. This
boundary is **private-framework-derived** and agrees with their absence from the Finale 2012 UI.
`maximumShortHairpinLength` remains at the pinned baseline value and is reported as `MusxOnly`, so
that modern threshold cannot create differentiated legacy behavior.

`articAvoidSlurAmt` is also `MusxOnly`. Finale 2012 exposes no such control, no mapping or accessor
for it appears in the authorized legacy Smart Shape preference history, and the Finale 27 baseline
supplies 8 EVPU. Finale's official version 26 feature list says that release added automatic
articulation adjustment around slurs and added a minimum-space control to Smart Slur Options; the
linked dialog documentation names it "Articulations Avoid Slurs By." This establishes Finale 26 as
the introduction boundary. The source is Finale's [New Features in Finale](https://usermanuals.finalemusic.com/FinaleWin/Content/Finale/What_s_new.htm)
and [Smart Slur Options](https://usermanuals.finalemusic.com/FinaleWin/Content/Finale/db-smart-slur-options.htm)
documentation, accessed 2026-08-26. The reader leaves the baseline value untouched and reports its
provenance separately from source recovery. **Confirmed.**

Coverage classifies differing `crescHorizontal`, `crescLineWidth`, `slurAvoidStaffLines`,
`slurLeftBreakHorzAdj`, `smartLineWidth`, and `useEngraverSlurs` values as
`different_defaults` when, and only when, the imported value retains `Finale27Default` origin. This
records the reviewed baseline-versus-upgrade disagreement without treating a source-recovered value
on the same path as expected. The same classification applies to horizontal and vertical offsets in
the four connection-style collections, again only with `Finale27Default` origin; connection indices
and source-recovered offsets remain outside the rule.

Finale 2003 is a separate, confirmed `finale-upgrade-loss` case for bend-curve connection offsets.
Finale 2002 has no bend-connection controls, Finale 2003 presents three, and Finale 2004 presents the
complete six-control interface. Selector `98` nevertheless stores all eight modern semantic tuples
in Finale 2003. Its three controls write the X and Y coordinates of types 0, 1, 2, 3, and 6; the
three top-line support tuples, types 4, 5, and 7, are not exposed and retain their source defaults.
Finale 27 resets every discriminating Finale 2003 X and Y edit during upgrade and also replaces the
three hidden Y values of 48 with 40. The coverage rule is restricted to DCL files whose source major
version is 8, `LegacyMus` bend-connection offsets, and the X and Y leaves. Connection indices are
excluded because no controlled Finale 2003 fixture varies them.
