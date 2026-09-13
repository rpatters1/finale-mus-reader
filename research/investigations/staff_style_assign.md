# StaffStyleAssign investigation

**Covers:** Binary confirmation of Staff Style assignment rows, Staff `hasStyles` derivation,
pre-Finale-2012 split assignments, and tracked recovery coverage.
**Read when:** Revisiting selector `Sy`, assignment ranges, companion-only assignments, or Staff
Style upgrade splitting. For the earlier `GF` source, read [`gframe_hold.md`](gframe_hold.md).
**Confidence:** `confirmed` for controlled fixtures and the tracked-evidence results described
here.

## Record reconstruction

Question: how are a Staff Style reference and its measure range stored across fixed-row and class
record files?

Method: compare the controlled Finale 2000, 2003, and 2011 assigned-style sources with their
unassigned parents and modern companions, then inspect the source record words and vary byte order
synthetically.

Result: `Sy` uses two six-word rows per assignment. The first row contains only the Staff Style
comparator in word 0. The second contains start measure, 32-bit start EDU, end measure, and 32-bit
end EDU. The Finale 2000 and 2003 fixtures both recover the range measure 1 EDU 1024 through
measure 2 EDU 2047. The Finale 2011 class record has the same 24-byte layout with low-word-first
32-bit values. Defining a style without assigning it does not create `Sy`; assigning it does.

Evidence: `tests/evidence/F2000/F2000-staffstyle-applied.mus`,
`tests/evidence/F2003/F2003-staffstyle-assigned.mus`, and
`tests/evidence/F2011/F2011-staffstyle-assigned.mus`, with their controlled parents and Finale 27
companions.

## Split assignments and tracked coverage

Question: after direct source assignment recovery, which remaining companion-only assignments are
source gaps and which are Finale upgrade normalization?

Method: capture the instrumented Release reader over `tracked-evidence` and
`rpatters1-private-evidence`, inspect every document containing a Staff Style assignment
difference, and separate pre-Finale-2000 documents from a Finale 2011 document whose source
assignment is retained while the companion adds identical-range assignments.

Result: the refreshed 2026-09-12 tracked snapshot selected 326 occurrences: 320 from
`tracked-evidence` and 6 from `rpatters1-private-evidence`. Every source and every companion
imported successfully. StaffStyleAssign comparison has 181 equal leaves, 6 expected leaves, zero
unexpected leaves, zero reader-only leaves, and 720 companion-only leaves. The controlled Finale
98 Slash and Two Bar Repeats assignments align semantically with their companions. The former
conclusion that `GF` required a two-row discriminator is superseded by the user-supplied values
recorded in [`gframe_hold.md`](gframe_hold.md).

The earlier one-shared-mask predicate recognized only one of the two Finale 2011 additions and was
refuted by the fixture. The retained structural rule permits multiple split assignments and asks
only that each added pre-Finale-2012 assignment duplicate a preserved source assignment's Staff
and complete range while referring to a different style.

The remaining companion-only population belongs to the broader pre-Finale-2012 upgrade problem;
it is not evidence about the fixed `GF` flags word. No newly observed difference was classified
during this cycle.

## Semantic assignment comparison

Question: can assignment comparison avoid incidence cascades and avoid labeling every
pre-Finale-2012 one-to-many Staff Style split?

Method: inspect every StaffStyleAssign-unexpected document in the 2026-09-12 all-unexpected ad hoc
capture, compare source and companion range multisets, and decode representative `Sy` records
directly. Every source range survived in its companion; 31 of 32 documents retained range order,
while one reordered equal-start assignments. Some companions inserted assignments before all
source assignments, and some replaced one source style reference with several style references on
the same range. Representative source `Sy` bytes agreed with the imported assignments, excluding
decoder misalignment as the cause.

Result: coverage now uses the complete Staff range as assignment identity. Exactly overlapping
assignments are retained in incidence order and composed as active-mask override patches, matching
the order in which musxdom constructs a `StaffComposite`. This allows one combined style to compare
with several split styles without depending on incidence, style ID, or style name. Note-font IDs
are resolved through the surveyed font definitions. The initial comparison intentionally did not
model the base Staff, generated instrument UUID semantics, or partial-overlap segmentation.

Evidence: 32 distinct documents selected from `rpatters1-main`; representative source
`mus-b92b0148be8cb36e`; controlled split behavior in
`tests/evidence/F2011/F2011-staffstyle-assigned.mus` and its Finale 27 companion. The implementation
has focused unit coverage.

Baseline validation, superseded for assignment-value counts by the refinement below: the
2026-09-12 all-unexpected ad hoc capture processed 596 occurrences representing
372 distinct source contents, with every source and companion importing successfully. All 433
tests passed. Semantic comparison left no unexpected `staff_style` leaves, but it did not reduce
assignment differences to zero: 1,947 unexpected leaves remain across 23 distinct contents, with
17,645 companion-only leaves across 57 distinct contents and four reader-only-score leaves in one
content. The retained unexpected examples are active-value disagreements, principally alternate
notation visibility, aggregate chord/fretboard visibility, note-font identity, and Staff name text
IDs. Separately, the Staff pool retains 1,067 unexpected `has_styles` leaves across 352 distinct
contents. These counts are leaf comparisons after composing equal-range assignments, so they are
not directly comparable with pre-semantic assignment counts.

Refinement question: why did 1,947 assignment leaves remain unexpected after exact-range
composition, and can they be compared without adding fixture-specific classifications?

Method: partition every remaining assignment leaf by field, origin, source version, assignment
count on its exact range, base-Staff value, and the existing standalone StaffStyle classification.
Inspect font and Staff-name referents on both sides rather than their numeric IDs.

Result: 1,828 leaves were already-understood StaffStyle upgrade behavior whose origin and
structural evidence had been discarded by the first assignment patch; 89 were equal font names
represented by different numeric IDs; 24 were equal Staff-name texts represented by different
text-block IDs; and 6 were instrument-field adjustments on two ranges created by a pre-Finale-2012
one-to-many split. The comparison now preserves patch origins and mask evidence, reuses the
standalone attached-item upgrade rules, compares font and Staff-name referents semantically, and
completes only the union of overridden fields from each document's base Staff. Exact overlaps still
compose in incidence order and may contain any number of assignments. Generated instrument UUIDs
and partial-overlap segmentation remain outside this refinement.

Evidence: all 1,947 unexpected assignment leaves across 23 distinct source contents in the
2026-09-12 `rpatters1-main` ad hoc capture. **Strong** for the explanation within that survey.
Focused regression tests cover multiple exact overlaps, base-Staff completion, inherited upgrade
classifications, pre-Finale-2012 instrument splits, and semantic Staff-name references.

Replacement validation: all 437 tests passed. The reduced `all-unexpected` cohort selected 142
previously unexpected occurrences from `rpatters1-main` and 440 from `rpatters1-installs`,
representing 361 distinct source contents; all 582 sources and companions imported successfully.
StaffStyleAssign
now has 181,164 equal, 1,854 expected, 79 unexpected, and 4,497 companion-only leaves. The 79
unexpected leaves occur in 7 distinct contents. Reported examples are dominated by
`vert_tab_num_off: 0 -> -1024`; the other displayed transformations are `no_key: true -> false`
and one six-field Staff-type conversion. Standalone StaffStyle retains zero unexpected leaves.
The separate pre-Finale-2000 Staff `has_styles` backlog remains 1,067 unexpected leaves across 352
distinct contents.
