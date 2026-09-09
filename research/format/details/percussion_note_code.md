# PercussionNoteCode

**Covers:** Legacy note-level percussion-map assignments.
**Read when:** Resolving a percussion note to `PercussionNoteInfo` or changing detail class
`0x0451`.
**Confidence:** `confirmed` for the modern layout; `strong` for the observed pre-2010 implicit
assignment rule.

## Identity and layout

Finale 2010–2012 zlib documents store this entry detail as class `0x0451`. The class-detail
comparators hold the high and low words of the 32-bit entry number. Each payload contains one or
more 10-byte elements, and each element becomes one incidence of
`musx::dom::details::PercussionNoteCode`:

| Field | Offset | Type |
|---|---:|---|
| `noteId` | 0 | unsigned word |
| `noteCode` | 2 | unsigned word |
| unused | 4 | three zero words |

An incomplete element or nonzero unused word is diagnosed while complete elements remain
recoverable. The class is present in Finale 2010, 2011, and 2012 records in the registered
catalogs. Coverage matches records by entry number and matches incidences within an entry by
`noteId`; incidence order is not semantic.

## Pre-2010 assignment

No `nC` or `NC` detail occurs in the 32 pre-2010 percussion-map documents in the focused cohort.
The only detail selector beginning with `N` or `n` is the unrelated `NG` staff-group record.

In the seven documents that Finale upgrades with note codes, the assignment is implicit. A legacy
percussion note's encoded pitch selects the `DF` row by input key (`cmper2`), provided that the
staff's `DS` bitmap selects that row. Finale then writes the row's playback MIDI value into the
upgraded entry pitch and attaches the generated percussion note type. Distinct input keys
therefore remain distinguishable when their `DF` rows share one playback MIDI value.

A controlled Finale 97 pair proves the direction. Input MIDI 71 and 79 are stored in the legacy
entry note as words 96 and 176. Both selected `DF` rows play MIDI 54, but Finale 27 converts them
to different note codes, 7 and 4103, while rewriting both upgraded notes to MIDI 54. A combined
fixture produces both results in one entry stream.

Across the broader seven-document comparison, 4,769 upgraded notes receive codes and have pitches
matching a selected row's playback MIDI value. Another 53 real notes whose upgraded pitches are
absent from the selected map receive no code. Rest entries can retain unused note slots; they do
not receive codes.

## Coverage

The native importer currently recovers zlib class `0x0451`. Pre-zlib recovery remains partial
because the entry pool is not yet recovered. Coverage classifies the resulting companion-only
legacy note codes as `awaits-dependent-recovery`; `--strict-deferred` reports them as unexpected.
The evidence and cohort tokens are in
[`../../investigations/percussion_note_code.md`](../../investigations/percussion_note_code.md).
