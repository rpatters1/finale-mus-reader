# Staff field recovery checklist

**Covers:** Staff leaves with outstanding recovery questions.
**Read when:** Choosing or completing the next Staff recovery task.
**Confidence:** Current implementation status; field interpretations remain governed by the
[Staff format note](../format/others/staff.md).

An unchecked item means that at least one supported source structure may contain a recoverable
value whose location or interpretation remains unresolved. Expected structural absence alone does
not put a field on this list. `[x]` marks a recovered field; `[~]` marks a field established as
unrecoverable in the named structure and supplied through legacy behavior or the pinned baseline.
Retain resolved items rather than removing them.

Current status: `hasStyles` is the only unresolved Staff field. Every `[~]` item below has an
intentional fallback disposition and is not outstanding recovery work.

## All structures

- [ ] `hasStyles` — calculate from Staff Style assignments once they are recovered.

## Before the Finale 2012 extension

- [~] `hideStaffLines` — Finale 27 default before the 2012 extension.

## Layouts without the extended flag, fret-instrument, or stem-offset tail

- [~] `fretInstId` — synthesize a one-string instrument from tablature Base Key; Finale 27 default otherwise.
- [~] `showTabClefAllSys` — tablature legacy behavior; Finale 27 default otherwise.
- [~] `hideRests` — tablature legacy behavior; Finale 27 default otherwise.
- [~] `hideTies` — Finale 27 default.
- [~] `hideDots` — tablature legacy behavior; Finale 27 default otherwise.
- [~] `hideStems` — tablature legacy behavior; Finale 27 default otherwise.
- [~] `stemDirection` — Finale 27 default.
- [~] `stemStartFromStaff` — Finale 27 default.
- [~] `stemsFixedEnd` — Finale 27 default.
- [~] `useTabLetters` — Finale 27 default.
- [~] `hideBeams` — Finale 27 default.
- [~] `breakTabLinesAtNotes` — Finale 27 default.
- [~] `stemsFixedStart` — Finale 27 default.
- [~] `hideTuplets` — tablature legacy behavior; Finale 27 default otherwise.
- [~] `horzStemOffUp` — Finale 27 default.
- [~] `horzStemOffDown` — Finale 27 default.
- [~] `vertStemStartOffUp` — Finale 27 default.
- [~] `vertStemStartOffDown` — Finale 27 default.
- [~] `vertStemEndOffUp` — Finale 27 default.
- [~] `vertStemEndOffDown` — Finale 27 default.

## Six-word Staff layouts

- [x] `notationStyle`
- [x] `useNoteShapes`
- [x] `blankMeasure`
- [x] `vertTabNumOff`
- [x] `customStaff`
- [x] `transposedClef`
- [~] `capoPos` — Finale 27 default; the stored legacy value is MIDI Base Key, not capo position.
- [~] `lowestFret` — Finale 27 default; the stored legacy value is MIDI Base Key, not lowest fret.
- [x] `floatTime`
- [x] `rbarBreak`
- [~] `showNameInParts` — Finale 27 default.
- [~] `showNoteColors` — Finale 27 default.
- [~] `hideRepeatBottomDot` — Finale 27 default.
- [~] `hideRepeatTopDot` — Finale 27 default.
- [~] `flatBeams` — Finale 27 default.
- [~] `hideFretboards` — Finale 27 default.
- [~] `hideLyrics` — Finale 27 default.
- [~] `noOptimize` — Finale 27 default.
- [~] `hideBarlines` — Finale 27 default.
- [~] `hideRptBars` — Finale 27 default.
- [x] `hideKeySigs`
- [~] `hideChords` — Finale 27 default.
- [x] `noKey`
