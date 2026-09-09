# DrumStaff

**Covers:** A percussion staff's map association and the pre-2010 row-selection bitmap.
**Read when:** Working on `DrumStaff`, selector `DS`, class `0x0084`, or legacy percussion-map
selection.
**Confidence:** `confirmed` for `whichDrumLib` and the class payload boundary; `strong` for the
fixed-row selection bitmap.

## Identity and layout

The availability boundary is documented with the map class in
[`percussion_note_info.md`](percussion_note_info.md).

The fixed-row selector is `DS`; its comparator is the staff ID. Its incidence array is one logical
word stream. Word 0 is `whichDrumLib`, the comparator of the associated `DF` percussion map.

Before Finale 2010, the rest of the stream selects which map definitions are usable on the staff.
Each following word contributes 16 LSB-first bits, and bit number *n* selects the `DF` row whose
second comparator is *n*. The musxdom `DrumStaff` class has no member for this bitmap, but the
percussion-map upgrade uses it to decide which `PercussionNoteInfo` objects to create.

The zlib class is `0x0084`, still keyed by staff ID. Finale 2007–2009 class payloads are 24 bytes
and retain the old selection data. Finale 2010 and later payloads are 12 bytes. Word 0 remains
`whichDrumLib` in both layouts; the remaining modern words are not mapped.

## Evidence

The 24-byte `0x0084` layout occurs consistently in Finale 2007–2009 records in both
`rpatters1-main` and `rpatters1-installs`; both surveys switch to 12 bytes in their represented
Finale 2010+ records. Seventeen Finale 2012 sources in the focused project cohort store staff 32
with word 0 equal to map 3, exactly matching all seventeen companions.

Seven uncompressed documents in `rpatters1-main` independently exercise fixed-row `DS`. Their
bitmaps select 15 or 16 `DF` rows each. Finale upgraded exactly those 107 rows, in bitmap-key
order, with no selection or appearance mismatch. The stable tokens and note-level comparison are
recorded in
[`../../investigations/percussion_note_info.md`](../../investigations/percussion_note_info.md).

The shared fixed-row recovery path and its remaining DCL evidence scope are documented in
[`percussion_note_info.md`](percussion_note_info.md).

The corrected all-corpus capture reports 696 equal leaves, 120 expected legacy-map replacements,
zero unexpected differences, and 118 reader-only score leaves. The class remains partial because
the trailing words in the 12-byte modern layout are still unmapped.
