# StaffUsed investigations

**Covers:** How the `StaffUsed` layouts, normalization, synthesis, and Special Part Extraction
selection were established.
**Read when:** Revisiting selector `IU`/`Iu`, class `0x009f`, an authored-list comparison, or system
top synthesis.
**Confidence:** `confirmed` for represented source layouts and normalization; `strong` for Special
Part Extraction representation and synthesis.

## 2026-09-14 — Layout and normalization recovery

**Question.** How do legacy staff lists map to modern `StaffUsed` arrays, and how does Finale move
their absolute first-staff position into `StaffSystem::top`?

**Method.** Raw rows were checked from controlled Coda, Finale 97, Finale 2000--2006, and zlib
fixtures before implementing the three entry widths. The Coda quartet isolated two entries in one
row. After the all-corpus comparison exposed large early-uncompressed values, raw Finale 3.0 and
3.2 rows were found to retain that two-entry layout, while multi-staff Finale 3.5 rows and the
public Finale 3.7.2 fixtures use one 12-byte entry per row. Existing system-margin and Coda
system-scaling fixtures tested normalization independently, including the 83%, 85%, and 87%
variants. Focused tests then covered both byte orders, the Finale 3.5 boundary, incomplete
payloads, authored comparators, base-list synthesis, and a synthetic nonzero Special Part
Extraction selector.

**Result.** The layouts and rules are recorded in
[`../format/others/staff_used.md`](../format/others/staff_used.md). In particular, the controlled
87% Resize Vertical Space system converts the base offset -188 to top -164, while the 83% and 85%
systems without that option retain -188. Rows through Finale 3.2 can contain two logical entries;
treating a row as one entry skipped every even-numbered staff and interpreted the second entry as
part of a 32-bit distance. Finale 3.5 is the first represented release with the later layout, so
the exact intervening release boundary is **strong** rather than confirmed.

## 2026-09-14 — Tracked companion comparison

**Question.** Does recovery preserve StaffUsed across every represented source epoch, and does it
remove the dependent StaffSystem top gap?

**Method.** The instrumented Release probe selected the registered `tracked-evidence` and
`rpatters1-private-evidence` inventories. It supplied Finale's symbol-font list and all three
installed non-General-MIDI annotation files. The 336 occurrences represent 333 distinct content
IDs: 123/120 Coda occurrences/distinct contents, 79/79 uncompressed, 81/81 DCL, and 53/53 zlib.
All sources and companions imported. Raw `IU(1)` rows were inspected separately for the only
remaining transformation, and selector 23 word 4 was checked across the cohort for active Special
Part Extraction.

**Result.** The final capture produced 26,217 equal leaves, 7,033 generally classified leaves,
2,387 Coda-expected leaves, and zero leaves in every unexpected or one-sided category. The original
54 classified leaves are one authored uncompressed system list repeated in three controlled
occurrences: Finale 27 inserts staff 27 at incidence 10 because it contains entries and shifts the
later positions. Semantic staff matching exposes the position shifts without treating later staffs
as replacements. The classifier does not inspect entries; it requires the source system order to
remain a subsequence of the companion, the companion order to remain a subsequence of the source
base list, and both cmpers to name StaffSystems. The other classified StaffUsed leaves are 4,802
Studio View leaves synthesized by Finale and 2,177 leaves attached to companion-only StaffSystems.
An earlier capture also had 532 reader-only leaves from a low-number list without a corresponding
StaffSystem; recovery now omits such orphaned layout lists. Any recurring one-sided low system-list
case is classified as Finale layout recalculation in either direction.

The same earlier capture had 7 reader-only and 7 companion-only leaves for the identical one-entry
list stored at 65531 by Finale 1.0 and at 65501 by Finale 27. The Finale 3.0
manual establishes that the feature had expanded from four to eight Staff Sets by that release.
Together with the reserved Special Part Extraction and temporary-system comparators at 65528 and
65529, this supports a Coda-era four-value base of 65530 and explains why Finale 3.0 had to move
the expanded namespace to 65500. Recovery's ordinal remap collapses those 14 one-sided leaves into
7 equal comparisons. **Strong:** the one represented Coda Staff Set is publicly reproducible,
while the other three early values remain unrepresented.

StaffSystem produced 12,762 equal leaves, 486 classified differences, and no unexpected
differences; its recovered top values no longer await StaffUsed. No tracked source had a nonzero
Special Part Extraction comparator. **Confirmed** for represented recovery; active extraction
was subsequently established by the all-corpus investigation below.

## 2026-09-14 — Special Part Extraction and native compact-list word order

**Question.** How does an active extraction affect system-list synthesis, and why did three Finale
98 Windows sources produce normalized distances exactly 65,536 times their companion values?

**Method.** The authorized all-corpus capture isolated the only two sources with reader-only
StaffUsed leaves, `mus-2d4d6af7c6f79420` and `mus-50081bda2eac62ff`. Their pre-zlib selector 23,
raw `IU` families, StaffSystems, and Finale 27 EnigmaXML were compared independently. Raw compact
`IU` words were then checked in the three Finale 98 Windows sources `mus-b83837e54efd4140`,
`mus-6767ebeee8a4a1ae`, and `mus-9a00dc9a26252fbf`.

**Result.** Both extraction sources store selector 23 word 4 as unsigned 65528 and contain an
authored `IU(65528)` selecting one staff. Finale 27 retains `<pageViewIUlist>65528</pageViewIUlist>`.
It reduces every authored system list to the selected staff, but recovery preserves the authored
four-staff lists. The initial inference that Finale takes the system-top adjustment from the
selected staff's raw position in `IU(0)` is **superseded**: the Finale 27 companions' page layouts
had diverged from their sources. Source-faithful recovery instead uses the extraction list's own
maximum as both its normalization value and the adjustment to each unoptimized system's top. The
two observed base/extraction pairs were -192/-165 and -216/-80. **Strong:** two independent scores
in one survey agree, but no controlled public extraction fixture or linked-part example is
available.

The Finale 98 sources store little-endian signed longs directly: for example, distance words
`[-188, -1]` and `[-428, -1]` represent -188 and -428, whose normalized difference is -240. Reading
those words as high-first produces -240 multiplied by 65,536. Compact `IU` therefore follows the
container's native long-word order just like the later ranged layouts. **Strong:** all three
Windows sources agree with their Finale 27 companions, while the big-endian behavior remains
covered by the public controlled fixtures.

After the three corrections approved from that evidence, the tracked dual-corpus capture imported
all 336 sources and companions and left StaffUsed with zero unexpected or one-sided leaves. The
initial all-corpus capture covered 16,433 occurrences representing 7,395 distinct
content IDs; 16,343 sources imported, all 4,941 available companions imported, and 90 inputs failed
before comparison. StaffUsed changed from 30,312 to 3,101 unexpected leaves and from 861 to zero
reader-only leaves. The sharing correction moved 25,762 leaves to equality. Native compact-long
order moved another 1,429 StaffUsed leaves to equality and corrected 26 dependent StaffSystem top
leaves. The extraction reduction moved its 861 removed-source leaves and 20 retained-staff distance
leaves to Finale layout recalculation. The now-superseded base-list anchoring also made 20
StaffSystem top leaves equal to companions whose page layouts had diverged. All five evidence
tokens named above then had no unexpected StaffUsed or StaffSystem differences. **Strong:** the
aggregate spans `tracked-evidence`, `rpatters1-main`,
`rpatters1-installs`, and `rpatters1-private-evidence`; at capture time, 3,101 StaffUsed
differences and 40,173 companion-only leaves still awaited investigation. The early-layout
analysis below resolves the identified subset without changing that retained snapshot.

Analysis of that snapshot attributed 360 of the 3,101 unexpected leaves to Finale 3.0 and 3.2
sources decoded with the later one-entry layout. Their raw rows match the earlier two-entry
layout, while represented Finale 3.5 sources use the later layout. This identifies the Finale 3.5
saving-version boundary. **Strong.**

The post-boundary all-corpus capture used the same 16,433 occurrences and 7,395 distinct content
IDs. StaffUsed unexpected leaves fell by exactly 360 to 2,741, while companion-only leaves fell
by 4,284 to 35,889 as the packed second entries became recoverable. All 38 Finale 3.0
occurrences, representing 25 distinct contents, and all 55 Finale 3.2 occurrences, representing
51 distinct contents, now have no unexpected StaffUsed leaves. The remainder spans 27
occurrences and 26 distinct contents from Finale 2.6 and Finale 2000--2012; all 214 retained
examples are `distFromTop` differences, and none has a magnitude at or above 65,536. StaffSystem
has 640 unexpected leaves across 21 occurrences and 19 distinct contents; all 195 retained
examples are `top` differences. Both residual populations remain open. **Strong.**

## 2026-09-15 — Unoptimized system-list selection

**Question.** Why did one Finale 2000 source and its Finale 27 companion render the same system
layout while their same-comparator StaffUsed lists differed?

**Method.** System 17 of `mus-2dd7565ae8920b52` was checked in Finale 2000 and Finale 27, then its
raw `SS`, system `Iu`, and base `Iu` rows were compared with the companion EnigmaXML. The public
API documents `FCStaffSystem::IsOptimized()` and `CreateSystemStaves()` as the optimization
and effective-list interfaces ([public PDK documentation](https://pdk.finalelua.com/class_f_c_staff_system.html),
accessed 2026-09-15). The legacy flag value and Finale 2011 transition were checked against
user supplied information and are **non-public-reference-derived**.

**Result.** The source system's `SS` flags are `0x0001`, so the `0x4000` own-list bit is clear. Its
stale four-staff `Iu(17)` normalizes to `0, -249, -512, -750`, but the six-staff base list normalizes
to `0, -288, -576, -864, -1177, -1409`, exactly the companion's `instUsed(17)` array. The source and
companion StaffSystem geometry is otherwise identical. Through Finale 2010, a clear `0x4000` bit
therefore makes the system use the Scroll View list; Finale 2011 and later treat every system as
optimized. **Strong:** one private source is independently binary-verified against both Finale
versions and its companion, and the selection rule agrees with the documented public API behavior.

## 2026-09-15 — Nonmonotonic ranged-list normalization (superseded)

**Question.** Why did two Finale 2006 sources retain a negative first StaffUsed distance in their
Finale 27 companions instead of moving it into `StaffSystem::top`?

**Method.** The focused cohort was recaptured with every unexpected example retained. Raw system
23 of `mus-6b4ffa135e18f5fd` stores StaffUsed distances `[-98, 0, -206, -530]`, `SS.top = -188`,
and 100% vertical scaling. Unconditional normalization produced `[0, 98, -108, -432]` and top
-286, while the companion preserved the raw list and top. Recovery was changed to abort tentative
normalization of a range-bearing list when a normalized value rises above its predecessor. An
initial unrestricted implementation was also tested against the cohort before the rule was
limited to range-bearing layouts.

**Result.** Both coordinate representations give identical absolute staff positions, but the
order rise from 0 to 98 identifies the representation Finale preserves. Applying that rule to
compact lists created 187 new StaffUsed differences in the represented Coda source, whose
companion still normalizes such lists; the range-bearing structural gate removes that regression.
The final focused capture selected 29 source occurrences representing 27 distinct contents, and
all sources and companions imported. StaffUsed unexpected leaves fell from 743 to 735 and
StaffSystem from 687 to 685; both `mus-6b4ffa135e18f5fd` and `mus-a16b6d7023b06b31` now have zero
differences in either class. All 1,420 remaining unexpected leaves were retained in the snapshot.
**Strong:** two Finale 2006 sources agree and the contrary compact behavior is independently
represented, but the exact introduction boundary within range-bearing releases is not isolated.

This value-based rule is **superseded** by the DCL epoch boundary below. Once every DCL and zlib
list preserves its stored origin, no pre-DCL range remains in the focused evidence for the
monotonic test to distinguish.

## 2026-09-15 — Finale 2011 normalization cutoff (superseded trial)

**Question.** Does the transition to linked-part-era StaffSystem behavior also end legacy IU-list
normalization?

**Method.** Recovery was changed to preserve every stored StaffUsed distance at Finale 2011 and
later while retaining the established Coda and pre-2011 rules. The focused cohort includes one
Finale 2011 content ID in Mac and Windows occurrences and five Finale 2012 content IDs. The
instrumented Release probe retains every unexpected example for this cohort.

**Result.** All 29 source and companion occurrences imported. StaffUsed unexpected leaves fell
from 735 to 283 and StaffSystem leaves from 685 to 254; all 537 remaining unexpected leaves were
retained in the snapshot. No Finale 2011-or-later fixture retains an unexpected difference in
either class. The cutoff remains **strong** rather than confirmed because the focused evidence
does not isolate it with controlled adjacent-version edits.

This trial is **superseded** by the broader Finale 2001 cutoff experiment below.

## 2026-09-15 — Finale 2001 normalization cutoff trial

**Question.** Does preserving the authored IU-list origin beginning with Finale 2001 explain the
remaining constant-offset StaffUsed differences more completely than the Finale 2011 boundary?

**Method.** Recovery preserves stored StaffUsed distances in the DCL and zlib epochs while
retaining the established pre-DCL rules. The epoch transition is itself the Finale 2001 boundary,
so no redundant version gate is used. A focused instrumented Release capture retains every
unexpected example from the 29-occurrence staff-layout cohort.

**Result.** All 29 source and companion occurrences imported. Unexpected StaffUsed leaves fell
from 283 to 7 and StaffSystem leaves from 254 to 48, reducing the focused total from 537 to 55; all
55 were retained. The seven StaffUsed leaves are confined to one Finale 2.6 list, while the
StaffSystem leaves comprise that fixture's paired top plus 47 Finale 2000 tops. Every represented
Finale 2001-or-later StaffUsed leaf agrees, including all three Finale 2006 contents.

The result is **open** rather than strong: this cohort was selected from fixtures unexpected under
the earlier behavior. It cannot reveal regressions in Finale 2001–2010 fixtures that were formerly
clean and therefore absent from the cohort. A fresh broad capture is required to decide whether
the DCL boundary supersedes the Finale 2011 trial.

A follow-up removed the monotonic-order guard and made all pre-DCL lists normalize
unconditionally. The focused capture remained exactly 7 StaffUsed, 48 StaffSystem, and 55 total
unexpected leaves, confirming that the value-based guard has no effect within this cohort once the
DCL boundary is active.

## 2026-09-15 — Compact-list maximum normalization

**Question.** Does Finale normalize a compact IU list from its maximum stored `distFromTop` rather
than incidence zero?

**Method.** Finale 2.6 system 21 stores staff 4 at -184 in incidence 0 and staff 12 at -163 in
incidence 1. Its Finale 27 companion puts staff 12 first, subtracts -163 from every distance, and
adjusts the system top by that value at the stored 88% vertical scale. Recovery was changed to
select the maximum value and the focused capture retained every unexpected example.

**Result.** All 29 source and companion occurrences imported. StaffUsed unexpected leaves fell
from 7 to 0, and StaffSystem leaves from 48 to 47, reducing the focused total from 55 to 47. The
Finale 2.6 content now has no unexpected leaf; all 47 retained differences belong to Finale 2000
StaffSystem tops. **Strong:** the raw list, scaled top calculation, and companion agree exactly,
with no contrary compact list in the focused cohort. A broad capture is still required to test
pre-DCL contents that were clean under incidence-zero normalization and therefore absent here.

The remaining Finale 2000 content, `mus-13e307b184ece4d2`, is a known bad Finale 27 conversion:
Finale 27 visibly destroyed its page layout. Coverage therefore identifies that companion by its
content ID and classifies its StaffUsed value differences and StaffSystem `top` differences as
Finale layout recalculation, without generalizing the damage into a format rule. The subsequent
uncapped focused capture imported all 29 source and companion occurrences and reported zero
unexpected StaffUsed, StaffSystem, or overall leaves.

## 2026-09-15 — Source-faithful Special Part Extraction normalization

A source-faithful Special Part Extraction recapture replaced the superseded comparator-0
anchoring described above. The uncapped all-corpus run imported 16,343 of 16,433 occurrences and
all 4,941 companions. StaffUsed retained zero unexpected leaves. The only 61 unexpected leaves in
the entire capture were `StaffSystem::top` differences: systems 1--31 of
`mus-2d4d6af7c6f79420` and systems 1--30 of `mus-50081bda2eac62ff`. Those are precisely the two
Finale 3.7 Special Part Extraction sources whose Finale 27 page layouts diverged. **Strong:** no
other source or class produced an unexpected leaf under the changed normalization.

For every differing system, the companion top equals the stored StaffSystem top plus the maximum
distance from a stale same-comparator IU list, falling back to IU(0) when that list is absent. The
companion nevertheless replaces the visible system list with the extraction list, so this
cross-list substitution loses the source layout. Coverage classifies a top difference as Finale
upgrade loss only when the companion equals that independently computed counterfactual. The
subsequent uncapped focused capture classified all 61 leaves and retained zero unexpected leaves.
**Strong:** both extraction sources satisfy the value predicate for every system, including both
the stale-list and IU(0)-fallback cases. The final uncapped all-corpus recapture retained the same
16,433 occurrences and 7,395 distinct contents, moved exactly those 61 leaves to Finale upgrade
loss, and reported zero unexpected leaves across all observed classes.
