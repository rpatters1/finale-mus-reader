# Staff investigations

**Covers:** Evidence behind the legacy `Staff` layouts and companion-comparison boundaries.
**Read when:** Revising `staff.cpp`, interpreting an unsupported Staff field, or investigating the
six-word Coda record.
**Confidence:** `confirmed` for observed structure; `open` for unedited fields and upgrade
transformations not yet reviewed.

## 2026-09-09 — base and extension reconstruction

Question: which portion of the public Finale-2000 `EDTStaffSpec` is raw, and how does it align with
legacy `IS`/`0x00e7` payloads?

Method: compare the public declaration with exact fixed-row and class payload bytes in controlled
Finale 1.0, 2.6, 3.7, 2001, 2003, 2008, 2011, and 2012 fixtures; decode both byte orders; then parse
the independently saved Finale 27 companion EnigmaXML.

Result: the declaration's first 18 words align with the raw record from Finale 3.7 onward. Its
post-padding staff ID, related-record selectors, and range are synthesized PDK data and have no
corresponding bytes in the 18-word record. Finale 2003 samples expand to 36 words, represented
Finale 2008 samples use 36-word zlib payloads, Finale 2011 uses 42 words, and Finale 2012 uses 48.
The 32-bit fields at bytes 36 and 40 initially looked like platform-dependent word slots; comparing
the big- and little-endian fixtures refuted that reading and established ordinary native 32-bit
storage. The 48-word layout adds the UUID at byte 80.

Evidence: public fixtures `tests/evidence/F372/F372-baseline.mus`,
`tests/evidence/F2001/F2001Win-empty.mus`, `tests/evidence/F2003/F2003-baseline.mus`,
`tests/evidence/F2003/F2003Win-empty.mus`, `tests/evidence/F2008/F2008-percussion-staff.mus`,
`tests/evidence/F2011/F2011-baseline.mus`, and `tests/evidence/F2012/F2012-bookmarks.mus`.

## 2026-09-09 — Coda predecessor and tracked comparison

Question: can the six-word Finale 1.0/2.6 `IS` record be assigned to modern Staff members, and which
source/companion differences remain after the verified slice?

Method: enumerate the `IS` payloads in both registered controlled surveys, compare 270 source
occurrences representing 268 distinct contents with their Finale 27 companions, and inspect raw
companion EnigmaXML for representative single- and multi-staff documents. Then edit the available
Staff controls in controlled Finale 1.0 and 2.6.3 documents and compare their `IS`, `IA`, `IN`, and
`in` rows with ETFs and independently saved Finale 27 companions.

Result: all 270 sources and companions imported. The controlled edits identify `IS` word 0 as the
default clef, word 4 as the later packed transposition, and word 5 bit `0x0010` as time-signature
hiding. Finale 1.0 writes the full name in uppercase `IN` rows and exposes neither custom staff
lines nor an abbreviation. Finale 2.6.3 adds an `IA` row whose word 1 packs Base Key in the low byte
and the vertical tablature-number offset in the high byte, words 3–4 carry notehead font ID and
size/effects, and word 5 carries the fret-instrument ID, tablature notation, and a version-specific
note-font enable bit. Its word 2 low bit and `IS` word 2 together produce custom staff line 13. The
full name remains in `IN`, while the new abbreviation uses lowercase `in`. Each name row contributes
12 NUL-terminated or padded bytes.

The same conversion synthesizes five modern tablature visibility values: show clefs on every
system and hide rests, augmentation dots, stems, and tuplets. The companion leaves the remaining
tablature visibility values false, so recovery treats only those five as Coda tablature legacy
behavior rather than attributing them to unidentified bits.
The recovered single-line custom Staff likewise receives the Coda legacy barline extents of -48
below and 48 above its reference line.
The imported name preference supplies the source font for decoding these eight-bit strings.
Recovery creates both layers of the modern reference chain—a `BlockText` containing the converted
name and a regular `TextBlock` pointing to it—and assigns the TextBlock comparator to the Staff.
Creation waits until the independent `HT`/`HS` text pass has materialized its original ordinal
identifiers, because page and measure assignments refer to those TextBlock IDs. The name pair can
then take the next free identifiers without disturbing any stored relationship.

The companions' 24-EVPU line spacing, four -4 rest offsets, -4 stem reversal, and -5/-3 repeat-dot
offsets have no location in either six-word record and match the pinned Finale 27 ordinary Staff.
The remaining unidentified values cannot yet be assigned uniquely.

Before these controlled Coda edits, recovery of the expanded rows, the narrowly selected
post-Coda defaults, and temporary deferral of Coda `Unmapped` leaves gave 48,286 equal matched
leaves, 798 expected differences, 67 unexpected matched leaves, 4,160 reader-only score leaves,
and 22,155 companion-only leaves. After resolving the remaining compatibility classifications,
the refreshed comparison had 48,290 equal matched leaves, 861 expected differences, no unexpected
matched leaves, and unchanged reader-only and companion-only counts. That capture included 716
temporary Coda deferrals.

The final 67 unexpected post-Coda leaves resolved as follows:

- 55 disabled note-font sizes stored as zero in Finale 3.7, 97, 98, and 2000 become the dormant
  24-point Finale 27 default and are classified as `DifferentDefaults`;
- eight false `useNoteFont` values—four Finale 97 percussion Staff occurrences and four Finale 2005
  tablature fixtures—become true and are classified as `FinaleUpgradeLoss` through Finale 2008;
- four Finale 97 `hideTimeSigsInParts` values now match after legacy behavior was corrected to follow
  the resolved `hideTimeSigs` value;
- assignment-derived `hasStyles` values awaiting `StaffStyleAssign` recovery are classified as
  dependent recovery rather than unexpected.

The complete alternate-notation defaults and aggregate-control mappings remove 229 prior
unexpected leaves: 181 pre-Finale-2000 slash-dot defaults and 48 Finale 2000/2001 other-layer
values.

The Finale 2009 compatibility correction removes every unexpected Staff difference from the
represented Finale 2007 and 2008 sources. No zlib Staff differences remain unexpected.

The temporary Coda triage rule was subsequently removed. Differing Coda Staff leaves whose reader
origin is `Unmapped` now remain unexpected until their values are recovered correctly. The
refreshed comparison has 48,290 equal matched leaves, 149 expected differences, 712 unexpected
matched leaves, 4,160 reader-only score leaves, and 22,155 companion-only leaves. The four Coda
expected differences that remain belong to the separately authorized UUID-default rule rather than
the removed blanket classification.

The snapshot's 272 pre-Finale-2012 UUID comparisons were reviewed separately. Finale 27 writes
`uuid::Unknown` in 212 cases, which now compare equal. The other 60 are named companion UUIDs: four
in the two public Finale 1.0 quartet fixtures and 56 in four private Finale 97 documents. Since none
of these layouts stores a UUID, the reader supplies `uuid::Unknown` as `LegacyBehavior`. A
disagreement is `DifferentDefaults` only for that origin before Finale 2012; a mismatch in the
96-byte stored layout remains unexpected.

The pinned macOS and Windows Finale 27 baselines carry the same ordinary Staff values. For the
post-Coda layouts that structurally end before them, its 24-EVPU line spacing and `-5`/`-3`
repeat-dot offsets account for 771 newly equal comparisons and are retained as
`Finale27Default`. The fallback is deliberately field-specific: the baseline's instrument UUID is
score identity, its
tablature-number offset disagrees with companions upgraded from shorter layouts, and its absent
note-font object cannot supply the font size those companions synthesize. Coda remains outside
this fallback because its six words are undecoded rather than proven not to contain the values.

Evidence: `tracked-evidence` selected 264 occurrences and `rpatters1-private-evidence` selected 6;
the source/companion values were checked against public representatives
`tests/evidence/F100/F100-baseline.mus`, `tests/evidence/F100/F100-quartet.mus`,
`tests/evidence/F100/F100-staffprops.mus`, `tests/evidence/F263/F263-timecomp.mus`,
`tests/evidence/F263/F263-staffopts.mus`, `tests/evidence/F372/F372-baseline.mus`,
`tests/evidence/F2001/F2001Win-empty.mus`, and
`tests/evidence/F2008/F2008-percussion-staff.mus`.

## 2026-09-09 — expanded Staff rows

Question: do the proposed incidence 2–6 extension locations agree with the controlled Finale 27
companions?

Method: decode each Staff payload in `tracked-evidence`, preserving native byte and 32-bit word
order, and compare the proposed values independently with the corresponding companion Staff.

Result: 141 expanded Staff records agree exactly for repeat-dot offsets, line spacing, vertical
tablature-number offset, the extended flag word, and all six stem-offset values. The two observed
stem-direction values agree with the companions, and four controlled tablature variants exercise
independent switches in that word. The 2011/2012 second alternate-notation word agrees for 64
records. Four controlled records carry nonzero fret instrument ID 2 and agree with their
companions. All six stem offsets remain zero throughout the cohort, so those locations still lack
a non-default semantic discriminator. The 76 shorter Finale 2000–2002 records retain zero in the
last base word while their companions write synthesized repeat-dot defaults.

Evidence: `tracked-evidence`; publicly reproducible structure and byte order in
`tests/evidence/F2003/F2003-baseline.mus`, `tests/evidence/F2003/F2003Win-empty.mus`,
`tests/evidence/F2008/F2008-percussion-staff.mus`, `tests/evidence/F2011/F2011-baseline.mus`, and
`tests/evidence/F2012/F2012-baseline.mus`.

## 2026-09-09 — Finale 2012 staff-line hiding

Question: when does display word bit `0x0004` acquire the meaning represented by Staff
`hideStaffLines`?

Method: compare the 42-word Finale 2011 Staff, the 48-word Finale 2012 baseline, and a controlled
Finale 2012 edit that hides the first staff's lines. Check the independently converted Finale 27
document against the raw source-bit result.

Result: the edit sets only bit `0x0004` in Staff word 11 for the setting under study. Opening the
Staff dialog also normalizes the font-size word and creates unrelated records, so those changes are
not mapping evidence. The Finale 27 companion independently writes `hideStaffLines` for staff 1.
The mapping is therefore gated by the 48-word structure: the 2012 baseline reports a stored false
value, the edited source reports true, and the 42-word 2011 layout leaves the property unmapped
because the same bit formerly participated in whole-staff hiding.

Evidence: `tests/evidence/F2011/F2011-baseline.mus`,
`tests/evidence/F2012/F2012-baseline.mus`, and
`tests/evidence/F2012/F2012-hidestafflines.mus`.

## 2026-09-09 — Finale 2005 tablature settings

Question: do controlled tablature settings distinguish the proposed extended flags and locate the
two remaining tablature-position fields?

Method: create four Finale 2005 variants that respectively break staff lines at numbers, hide
tuplets, show clefs on every system, and use letters. Compare their `IS` records and ETF exports,
then parse each independently converted Finale 27 Staff.

Result: the ETF reproduces every `IS` word exactly, and the companion semantics agree. In word 22,
the four variants respectively isolate `0x0800` set, `0x2000` clear, `0x0001` clear, and `0x0200`
set, corroborating the existing boolean transformations. Word 1 is independently discriminated:
`0x0b17` accompanies `capoPos` 23 and `lowestFret` 11, while `0x0d05` accompanies 5 and 13. It is
therefore an unsigned low/high byte pair. All four companions also agree with word 20's vertical
number offset of -1088 and word 23's fret-instrument comparator 2.

Evidence: `tests/evidence/F2005/F2005-tab-breaklines.mus`,
`tests/evidence/F2005/F2005-tab-notuplets.mus`,
`tests/evidence/F2005/F2005-tab-showclefs.mus`, and
`tests/evidence/F2005/F2005-tab-showletters.mus`, together with their tracked ETF and Finale 27
companions.

## 2026-09-09 — Finale 2012 automatic name numbering

Question: where does the Finale 2012 Staff store its newly introduced automatic name-numbering
setting and style?

Method: compare the Finale 2012 baseline with a controlled edit that enables ordinal-prefix
numbering, then inspect the independently converted Finale 27 Staff.

Result: Staff word 37 is the only raw word that changes, from `0x0000` to `0x8002`. The companion
writes enabled automatic numbering with the ordinal-prefix style. The high bit is therefore the
enable switch and the remaining bits carry the musxdom enum value. The word exists inside the
96-byte layout that first appears in Finale 2012; shorter layouts report the disabled feature as
legacy behavior with numeric style value zero.

Evidence: `tests/evidence/F2012/F2012-baseline.mus` and
`tests/evidence/F2012/F2012-staff-autonum.mus`, with their Finale 27 companions.

## 2026-09-09 — note-attached items

Question: does the legacy “Show Note-attached Items” setting correspond to one modern Staff field
or expand into several fields during upgrade?

Method: uncheck the single setting in controlled Finale 2000, 2006, and 2008 documents, compare
each Staff record with its untouched parent and available ETF, and inspect the independently
converted Finale 27 Staff. The Finale 2006 and 2008 comparisons use their `empty` fixtures as
parents.

Result: word 2 moves from `0x0700` to `0x0600` in Finale 2000 and from `0x6700` to `0x6600` in
Finale 2006 and 2008. The alternate-notation style and layer subfields remain zero, so the changed
`0x0100` bit is the setting. Word 4 also moves from `0x0000` to `0x1800` in all three, but the Finale
2005 tablature fixtures establish that word 4 is the note-font size/effects value; the edit has
materialized a 24-point default rather than set two more flags. All three companions set
`altHideArtics`, `altHideLyrics`, `altHideSmartShapes`, `altHideExpressions`, `hideFretboards`, and
`hideChords`. This confirms the one-to-many conversion through Finale 2008. Finale 2009's removal
of entry-attached expressions is the compatibility boundary; its representation can be checked
when the broader corpus next supplies that version.

Two further Finale 2000 edits distinguish the other aggregate controls. Unchecking “Show Notes in
Other Layers” clears only word 2 bit `0x0200`, and its companion changes only
`altHideOtherNotes`. Unchecking “Show Note-attached Items in Other Layers” instead clears only
`0x0400`, and its companion changes `altHideOtherArtics`, `altHideOtherLyrics`,
`altHideOtherSmartShapes`, and `altHideOtherExpressions`. Both edits also materialize word 4 as
`0x1800`. The 74-byte extension supplies independent storage for the latter three properties, so
its presence is the structural boundary between this aggregate conversion and the later direct
interpretation. The represented F2008 layout is shorter, while the represented F2011 layout
contains the extension.

A separate Finale 2008 notehead-font edit changes only Staff words 3, 4, and 5 to `0x0009`,
`0x1104`, and `0x0020`. Its companion resolves font ID 9 to American Typewriter, preserves size 17
and underline, and enables `useNoteFont`, independently confirming all three locations. The two
controlled Finale 2008 percussion Staff records instead carry a 24-point note font with word 5 bit
`0x0020` clear, while Finale 27 enables `useNoteFont`. Coverage treats only a stored false to
companion true change on a pre-Finale-2009 zlib percussion Staff as upgrade loss; other styles,
directions, origins, epochs, and versions remain unexpected.

The Finale 3.7.2 notehead-font edit already uses the same representation: Staff words 3, 4, and 5
become `0x0006`, `0x1700`, and `0x0020`. The companion identifies font 6 as Apple Chancery,
preserves its plain 23-point size, and enables `useNoteFont`. Additional `CS`, `NS`, numeric-global,
and font-definition changes are not needed to establish the Staff mapping. There is therefore no
Finale-2000 gate on these three Staff fields.

The controlled Staff Style pairs resolve how `hasStyles` should be recovered. In Finale 2000 and
2003, defining and assigning a style leave `IS` byte-identical; the assigned companion nevertheless
adds one `staffStyleAssign` and `staffSpec.hasStyles`. In Finale 2011, defining the style still leaves
`IS` unchanged, but assigning it sets only Staff word 5 bit `0x1000`, and the companion again writes
the assignment and `hasStyles`. The 36-word Finale 2003 layout therefore refutes payload expansion
as the raw-bit gate. Locating the later introduction would not affect recovery: once the separately
scoped `StaffStyleAssign` class is recovered, assignment presence will refresh `hasStyles` for every
source version. The Staff slice leaves it `Unmapped` until then, and coverage defers only that
unmapped difference as awaiting dependent recovery. A calculated value with any other provenance
remains subject to ordinary comparison.

The separate Studio View Staff is byte-identical across the Finale 2006 pair. It is application-owned
rather than authored score content. The reader continues to recover it on a best-effort basis, while
the coverage surveyor excludes the entire instance on both sides of comparison.

Evidence: `tests/evidence/F2000/F2000-baseline.mus`,
`tests/evidence/F2000/F2000-staff-unshownoteitems.mus`,
`tests/evidence/F2000/F2000-staff-hideothernotes.mus`,
`tests/evidence/F2000/F2000-staff-hideotheritems.mus`,
`tests/evidence/F2000/F2000-staff-style.mus`,
`tests/evidence/F2000/F2000-staffstyle-applied.mus`,
`tests/evidence/F2003/F2003-staffstyle.mus`,
`tests/evidence/F2003/F2003-staffstyle-assigned.mus`,
`tests/evidence/F2006/F2006-empty.mus`, and
`tests/evidence/F2006/F2006-staff-nonoteitems.mus`, together with their ETFs and Finale 27
companions; `tests/evidence/F2008/F2008-empty.mus`,
`tests/evidence/F2008/F2008-staff-nohidenoteitems.mus`,
`tests/evidence/F2008/F2008-notehead-font.mus`, and the two Finale 2008 percussion Staff fixtures
with their Finale 27 companions; and `tests/evidence/F2011/F2011-staffstyle.mus` and
`tests/evidence/F2011/F2011-staffstyle-assigned.mus` with their Finale 27 companions. The early
note-font corroboration is `tests/evidence/F372/F372-notehead-font.mus` with its ETF and Finale 27
companion.

## 2026-09-09 — pre-Finale-2000 alternate notation ranges

Question: are the alternate-notation values seen after upgrading an early document properties of
its raw Staff record?

Method: apply alternate notation to one entire Finale 98 Staff and, separately, to its first three
measures; compare each document with the untouched parent and inspect the Finale 27 companions.

Result: neither edit changes the `IS` Staff record. Both add `GF` records keyed by staff and
measure. The full-staff edit writes paired records through the document's 44 measures, while the
partial edit writes an operative record for measures 1–3 and zero-valued records thereafter.
Finale 27 represents the full edit as a synthesized Two Bar Repeats Staff Style assignment over
measures 1–44, and the partial edit as a synthesized Slash Notation assignment over measures 1–3.
The modern Staff alternate-notation properties therefore have no pre-Finale-2000 raw Staff source.
Across the controlled and tracked companions, the compatible Staff defaults are `Normal`, layer 0,
slash dots enabled, and every other alternate-notation boolean disabled. Recovery supplies those
values as `LegacyBehavior`; recovery of `GF` and its synthesized style runs remains future work.

Evidence: `tests/evidence/F98/F98-baseline.mus`,
`tests/evidence/F98/F98-altnotation-full.mus`, and
`tests/evidence/F98/F98-altnotation-partial.mus`, together with their ETFs and Finale 27
companions.
