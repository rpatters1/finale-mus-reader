# GFrameHold investigation

**Covers:** Identification of `GF`, correction of the alternate-notation discriminator, and the
pre-Finale-2000 Staff Style projection.
**Read when:** Revisiting `GF` layout, its flags word, two-bar-repeat phase 5, or the decision not to
construct `GFrameHold` before entries.
**Confidence:** `confirmed` for values 1 through 6 at both observed flags locations and the
focused companion comparison; `strong` for the Finale 98 boundary; user-supplied for the mask and
value names; `open` for the record structure.

## Two presumed flags locations

Question: is alternate notation a distinct two-row `GF` form, or a field of every `GFrameHold`?

Method: compare parallel controlled Finale 2.6.3, 3.7.2, 97, and 98 records and their ETFs, then
reconcile those observations with values supplied by the user. The current public Framework class
reference was checked on 2026-09-12 to confirm that its corresponding class is the cell-attached
frame and clef holder.

Result: `GF` itself is `GFrameHold`. The Finale 2.6.3, 3.7.2, and 97 fixtures store values 1
through 6 in word 4. The parallel Finale 98 fixture stores the same values in first-row word 1,
the user-supplied flags location. The preliminary decoder therefore reads word 4 before
Finale 98 and word 1 from Finale 98 onward before applying `0x000f`. This identifies only the
presumed flags location; it does not interpret the record structure or the relationship among
rows. **Confirmed** for the two observed locations; **strong** for the boundary.

Record length is deliberately not the discriminator. The possibility that layers, clef lists, or
other unresolved frame content may increase the incidence count remains open. Seven Finale 1.0.0
sources in `rpatters1-installs` and nine distinct Finale 1.8.7 sources in `rpatters1-main` also
contain repeated same-key rows while retaining a zero low nibble in word 4. What any repeated rows
represent remains open. **Strong.**

The user-supplied table assigns 0 through 6 to Normal, Slash Beats, Rhythmic, One-Bar Repeat,
Two-Bar Repeat, Two-Bar Repeat Blank End, and Invisible Beats. Both short-layout fixtures store the
complete 1--6 sequence; controlled Finale 98 Two Bar Repeats alternate 4 and 5 by measure. Together
they confirm that 5 is the blank phase of the same notation rather than
`BlankNotationWithRests`. Finale 98
allows partial music selection but applies alternate notation only to complete measures, matching
the measure-keyed `GF` representation. **User-supplied product behavior; not yet isolated in a
controlled selection-boundary fixture.**

An earlier used-styles-only projection left 994 canonical styles companion-only, or 139,160 leaves
at 140 leaves per style. Creating the complete six-style palette removed that population. A later
two-row-only projection avoided false assignments by record shape, but the user-supplied
identification showed why that success was accidental: it excluded complete flags words instead
of masking their low nibble. Both superseded approaches are retained here so neither shortcut is
reintroduced.

Evidence: `tests/evidence/F263/F263-altnotation.mus`,
`tests/evidence/F372/F372-altnotation.mus`, `tests/evidence/F97/F97-altnotation.mus`,
`tests/evidence/F98/F98-altnotation.mus`, `tests/evidence/F98/F98-baseline.mus`,
`tests/evidence/F98/F98-altnotation-partial.mus`, and
`tests/evidence/F98/F98-altnotation-full.mus`, with their Finale 27 companions; values supplied by
the user; public class identity at
[`FCCellFrameHold`](https://pdk.finalelua.com/class_f_c_cell_frame_hold.html), accessed 2026-09-12.
The value names remain user-supplied; all numeric values are independently exercised.

## Projection and validation

The implementation deliberately stops short of constructing `musx::dom::details::GFrameHold`.
`importGFrameHolds` owns the fixed-prefix read and materializes the legacy alternate-notation
projection before Staff Style assignment auditing. `staff_style_assign.cpp` now handles only the
direct `Sy` family.

The refreshed tracked development capture selected 330 occurrences representing 327 distinct
source contents. Every source and companion imported, with no unexpected differences. The new
Finale 97 and Finale 98 fixtures each contribute 840 equal StaffStyle leaves and 100 equal
StaffStyleAssign leaves, with no expected, unexpected, reader-only, or companion-only differences
in either class.
