# RepeatOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Repeat options: the document staff-list reference

The reader now recovers 22 scalar fields from selectors `05`, `20`, `69`–`72`, and `76` in
the fixed-row epochs and from their numeric-global classes in the zlib epoch. A stored zero
maximum-pass count has the older twenty-pass meaning; the later files state 20 directly.
`bracketEndAnchorThinLine` postdates the located legacy family and is asserted false as
`LegacyBehavior` only when that family is present. Selector `72` (class `0x0056` in the zlib
epoch) is the structural marker: every located RepeatOptions layout contains it, while it is
absent throughout the Coda-banner evidence and the observed Finale 3.0–3.2 files. Those earlier
files use fixed legacy behavior for `addPeriod` false, both repeat-dot vertical positions zero,
the thin repeat line and ending-bracket line widths 224, and the ending anchor on the thick line;
the remaining unsupported fields retain the pinned Finale 27 defaults. Finale 2.6.3's UI likewise
exposes no settings corresponding to RepeatOptions; extending that conclusion to the early
uncompressed epoch remains **strong** rather than confirmed because no source application between
2.6.3 and 3.7.2 is available for a direct UI check.

An unmasked three-survey recovery capture before those six assertions found 136 successful
source/companion pairs without the structural marker. Each pair showed the same six differences,
for 816 uniform leaves. Their imported values were the pinned Finale 27 defaults; their companions
all produced the fixed values above. These are treated as **strong** legacy behavior rather than
recovered MUS fields because no early RepeatOptions family exists to store them.

`bracketHeight` remains separate and **open**. The same 136 pairs compare the pinned value 96 with
144 in 116 companions but with 72 in 20 companions. All 20 exceptions are Windows Finale 2.2
tutorial documents, yet four other documents from the same installation upgrade to 144, so neither
epoch, version, nor platform distinguishes the result. The reader does not synthesize either value
until a source structure or a characterized upgrade rule explains the split. Coverage classifies
these reviewed baseline/companion disagreements under the `different_defaults` umbrella; that
classification records their nature without choosing which companion value is legacy behavior.

**The current preference mapping locates `RepeatOptions::showOnStaffListNumber`, but no legacy MUS in scope stores
that location.** The current private framework maps it to numeric global selector `72`, comparator `65534`,
incidence 1, word 2. That is a **private-framework-derived** lead, not a legacy finding. In the zlib epoch the
established numeric-global bridge would make the corresponding location class `0x0056`, byte offset 16 of its
coalesced payload.

A read-only scan of all 15,941 inventoried paths in the three registered surveys, representing 6,984 distinct
content identities, found no second incidence. Every observed `72(65534)` family and every `0x0056` class payload
was exactly 12 bytes: incidence 0 only. Selector `72` is absent altogether through the Coda-banner epoch and the
observed Finale 3.0–3.2 documents. The result covers both byte orders where each later epoch supplies them, including
the Windows Coda-banner population that states no parseable version.

All 4,493 available Finale 27 companions likewise compare with `showOnStaffListNumber` unset. They carry the repeat staff-list
families in many documents, so those objects do not by themselves imply that this document option selects one.
The companions generally state `<showOnTopStaffOnly/>`, including the pinned baseline, but the source record has no
second incidence from which to recover it.

The controlled Finale 2005 top-staff/staff-list pair sharpens that negative result. Its seven
RepeatOptions selector rows are byte-for-byte identical, including the single selector `72`
incidence. The corrected staff-list MUS instead adds exactly five physical other rows: one `DC`
score-membership row, two `Dc` name incidences, one `dc` parts-membership row, and one `io`
parts-override row. These are the four components of a repeat staff-list family, not ending and
text-repeat assignment records; the ETF reports the same additions and no changed repeat object.
Its checked-in modern companion predates the corrected save and is not a valid semantic pair. The
fixture author independently confirmed that Finale 27 does not upgrade the legacy selection into
the document option. Thus the edit creates staff list 1 without storing a RepeatOptions pointer to
it. Whether the list identity or its presence implicitly represented the legacy selection remains
open.

The Finale 2012 baseline/staff-list pair supplies the same shape in the zlib epoch. Its
`0x0056` RepeatOptions class remains byte-for-byte identical and exactly 12 bytes long. The
edit adds only three semantic class records, all at cmper 1: `0x00e1` names `Staff List 1`,
`0x00e2` contains parts member `-3`, and `0x00e4` contains the score's floating top and bottom
members `-1` and `-2`. The companion identifies these respectively as `repeatStaffListName`,
`repeatStaffListParts`, and `repeatStaffListScore`. It nevertheless writes
`<showOnTopStaffOnly/>` and no document staff-list number. The fixture author reports that
Finale 2006–2012 converted this old selection to All Staves, while a later release drifted to
Top Staff Only; neither conversion describes the source semantics.

The three-list Finale 2012 discriminator rules out the apparent cmper-1 convention. A save with
three lists and list 2 selected leaves `0x0056` unchanged and differs from the one-list/list-1 save
only by the seven records required to define the two added lists. These are names `0x00e1`, parts
memberships `0x00e2`, score memberships `0x00e4`, and a score override `0x00e5`; the companion
identifies the last of these as `repeatStaffListScoreOverride`, not a selection marker. No record
outside those list families changes.

The apparent persistence in the source application is an application-state bug, not a document
representation. In both Finale 2005 and Finale 2012, a selected repeat staff list survives closing
and reopening the document while the same Finale process remains running. After Finale itself is
quit and relaunched, reopening that document loses the selection. This exactly explains why
repeated saves and same-session checks appeared to preserve a value while controlled MUS record
comparisons found none. It also means that a converter's All Staves or Top Staff Only result is a
fallback chosen after the source session state has disappeared, not recovery of document data.

Together with the corrected Finale 2005 pair, this is **strong** evidence that the legacy MUS
representation in scope does not persist the document-level selected list number at all. List
presence and cmper must not be used to synthesize one. A repeat staff-list importer can therefore
recover the list objects independently without choosing one for `showOnStaffListNumber`.

The practical conclusion is deliberately narrow. Selector `72` itself is legacy: its incidence 0
holds six verified ending-bracket fields and becomes the 12-byte class `0x0056` in the zlib epoch.
What is absent is incidence 1. Classifying its word-2 locator as a MUSX-era preference mapping
beyond this reader's scope is **strong**, not confirmed: no legacy file carries it, but no observed
file directly establishes when it was introduced. The reader intentionally has no source mapping
for `showOnStaffListNumber`; it and the related display-mode field remain at the pinned Finale 27
default rather than following any converter fallback.

Finale 2.6.3 supplies the direct Coda-banner UI observation. The next available source application
is Finale 3.7.2, whose fixed-row RepeatOptions family is already present, so no direct UI
discriminator is available for Finale 3.0–3.2. The structural gate makes that missing version
coverage harmless: absence of the family selects the fixed behavior above and pinned defaults for
the still-unsupported fields.
