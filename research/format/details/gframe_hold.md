# GFrameHold

**Covers:** Legacy `GF` identity, its presumed flags locations, and the pre-Finale-2000
alternate-notation projection.
**Read when:** Working on `GFrameHold`, `GF`, entry frames, clef lists, or alternate notation before
Staff Styles existed.
**Confidence:** `confirmed` for both observed alternate-notation locations and their companion
projections; `strong` for the Finale 98 boundary between them; user-supplied for the Finale 98
flag mask and value names; `open` for the record structure and complete length history.

## Current recovery boundary

`GF` is a Details record keyed by Staff and measure. Two alternate-notation layouts are controlled:

| Source range | Alternate-notation source | Evidence |
|---|---|---|
| before Finale 98 | low nibble of word 4 | Finale 2.6.3, 3.7.2, and 97 fixtures |
| Finale 98 through Finale 2000 | low nibble of first-row word 1 | Finale 98 fixtures and user-supplied values |

The presumed flags are in word 4 before Finale 98. User-supplied values identify word 1 as flags,
and the controlled Finale 98 fixture places the alternate-notation selector there. The record's
structural discriminator and variable-length behavior remain unknown, so the preliminary importer
uses the recovered source version. Every other word and the relationship among same-key rows are
intentionally uninterpreted until entry recovery establishes the frame structure.
`importGFrameHolds` therefore does not yet
construct `musx::dom::details::GFrameHold`; it recovers only the removed legacy behavior described
below. **Confirmed** for the observed alternate-notation locations; **strong** for the Finale 98
boundary; **user-supplied** for the Finale 98 location and mask. The current public Framework
describes [`FCCellFrameHold`](https://pdk.finalelua.com/class_f_c_cell_frame_hold.html) as a
cell-attached frame and clef-change class (accessed 2026-09-12); the fixed prefix and mask come
from values supplied for this investigation.

| `flags & 0x000f` | Legacy meaning | Modern projection | Evidence |
|---:|---|---|---|
| 0 | normal notation | no assignment | user-supplied value |
| 1 | slash-beats notation | Slash Notation | user-supplied value + controlled fixtures |
| 2 | rhythmic notation | Rhythmic Notation | user-supplied value + controlled fixtures |
| 3 | one-bar repeat symbol | One Bar Repeats | user-supplied value + controlled fixtures |
| 4 | two-bar repeat symbol | Two Bar Repeats | user-supplied value + controlled fixtures |
| 5 | blank end of two-bar repeat | Two Bar Repeats | user-supplied value + controlled fixtures |
| 6 | invisible-beats notation | Blank Notation | user-supplied value + controlled fixtures |

Value 5 is an internal phase of the two-bar repeat representation, not the later
`BlankNotationWithRests` choice. No numbering shift is applied in the pre-Finale-2000 projection.
The corresponding legacy command operates on whole measures, so consecutive cells with the same
effective notation become one full-measure Staff Style assignment. Values 4 and 5 coalesce as the
same effective notation. **Confirmed** for all six nonzero values by the parallel
`tests/evidence/F263/F263-altnotation.mus`, `tests/evidence/F372/F372-altnotation.mus`,
`tests/evidence/F97/F97-altnotation.mus`, and `tests/evidence/F98/F98-altnotation.mus` fixtures.
The older Finale 98 range fixtures independently exercise values 1, 4, and 5.

## Upgrade projection

Finale 27 creates a complete six-style canonical palette when upgrading represented
pre-Finale-2000 documents: Normal Notation, Slash Notation, Rhythmic Notation, One Bar Repeats,
Two Bar Repeats, and Blank Notation. Recovery mirrors that upgrade and creates assignments for
nonzero `GF` notation values. Synthesized style contents and assignment comparison are documented
with [`StaffStyleAssign`](../others/staff_style_assign.md).

The tracked development capture contains 330 occurrences representing 327 distinct contents; all
sources and companions import successfully, with no unexpected differences. In each new Finale 97
and Finale 98 fixture, StaffStyle has 840 equal leaves and StaffStyleAssign has 100 equal leaves,
with no differences in either class. **Confirmed** within that cohort. Method and superseded
interpretations:
[`../../investigations/gframe_hold.md`](../../investigations/gframe_hold.md).

## Remaining scope

The clef representation, frame references, sharing, complete record-length history, and every other
`GFrameHold` member remain open. They are deferred until entries are recovered. No `GFrameHold`
objects are emitted or surveyed yet.
