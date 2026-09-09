# PercussionNoteCode investigations

**Covers:** Native class `0x0451` and Finale's synthesis of note codes from pre-2010 percussion
maps.
**Read when:** Revisiting selectors `nC` or `NC`, note-level percussion assignment, or native
note-code differences.
**Confidence:** `confirmed` for observed native and controlled bytes; `strong` for the focused
upgrade rule.

## 2026-09-08 — Native layout and pre-2010 selector search

**Question.** Is a note-level detail stored before Finale 2010, and how does the modern class
encode one?

**Method.** Detail selectors `nC` and `NC` were checked first in all 32 pre-2010 documents with
`DF` maps in the 85-document focused cohort. Every remaining selector beginning with `N` or `n`
was then enumerated. Native class `0x0451` payloads from Finale 2010–2012 records in
`rpatters1-installs` and `rpatters1-main` were compared with their Finale 27 EnigmaXML.

**Result.** Neither candidate selector occurs. Only `NG`, an unrelated staff-group family, occurs:
8,058 rows in 16 of the 32 files. Native class `0x0451` stores one or more five-word elements under
the high and low comparator words of an entry number. The words are `noteId`, `noteCode`, and
three zeroes. The smallest focused Finale 2012 source contains six elements and its companion
contains the same six. Two larger sources contain four more source elements than their companions,
consistent with Finale removing details for entries or notes that do not survive conversion; the
cleanup rule remains open.

The 85-document orchestra cohort expanded that population to ten Finale 2012 documents. Matching
by entry number and `noteId` leaves four source-only objects in each document and no value
differences. The previous array-position comparison produced 180 false differences after those
omissions.

## 2026-09-08 — Implicit legacy assignment

**Question.** Which real notes receive a synthesized `PercussionNoteCode` when no legacy detail
exists?

**Method.** Seven pre-2010 sources and their Finale 27 companions were joined through `DS` staff
IDs, frame holders, frame entry chains, entry note IDs, and the `DS` selection bitmap. Companion
entry harmonic levels and alterations were converted to chromatic MIDI pitch and compared with
the playback word of each selected `DF` row. The evidence tokens are `mus-5e377325bf2a68bb`,
`mus-7be555f3c12df760`, `mus-7e52db7272aa3564`, `mus-83764909dd8b43c9`,
`mus-c37089222a7f68a4`, `mus-c6a556d06a15e981`, and `mus-d3614a4d793f3201`.

**Result.** The percussion staves contain 4,822 real notes. Finale generated codes for all 4,769
notes whose pitch occurs in the selected map and none for the 53 notes whose pitch does not.
Another 559 rest entries carry stale note slots but no `isNote` flag and receive no codes. Every
generated code resolves to the upgraded row with the same playback pitch. Duplicate selected rows
with one playback pitch require the controlled experiment below to determine the input row.

## 2026-09-08 — Duplicate-playback input keys

**Question.** How does Finale distinguish selected `DF` rows 71 and 79 when both have playback
MIDI 54 but different staff positions and noteheads?

**Method.** The Finale 97 baseline `mus-c37089222a7f68a4` was copied into the private controlled
corpus. MIDI notes 71 and 79 were entered on the preceding ordinary staff and copied to percussion
staff 43, separately and together. Finale 97 ETF and Finale 27 companions were saved for the
baseline and three variants. The variant tokens, in alphabetic fixture order, are
`mus-39bcd6e74bfcdfa2`, `mus-648fa445fd9f7414`, and `mus-1e164b8adbbe30b7`.

**Result.** Input MIDI 71 is word 96 in the legacy note and input MIDI 79 is word 176. On the
ordinary staff, Finale 27 retains harmonic levels 6 and 11. On the percussion staff, it rewrites
both notes to harmonic level -4, alteration 1 (MIDI 54), while assigning note codes 7 and 4103.
The combined fixture produces both conversions in one stream. The source entry pitch therefore
selects `DF.cmper2`; `DF` word 0 supplies the upgraded pitch, and the generated row type supplies
`PercussionNoteCode.noteCode`. No separate legacy note-detail record is required.

## 2026-09-08 — Deferred recovery classification

**Question.** How can tracked coverage retain the unresolved legacy note-code synthesis without
making the ordinary report noisy or allowing the gap to disappear from strict validation?

**Method.** The tracked public and private corpora were captured together with deferred recovery
enabled and again with `--strict-deferred`. The capture contained 247 documents: 81 Coda-banner,
71 DCL, 55 uncompressed, and 40 zlib. A hidden failing regression was also added for constructing
legacy note codes from decoded entries; Catch2 excludes it from ordinary discovery.

**Result.** All source and companion reads succeeded. The ordinary report classifies the 128
companion-only legacy note-code leaves as `awaits-dependent-recovery`. Strict mode reclassifies
the same 128 leaves as unexpected. The class checklist remains partial until entry decoding can
supply the entry number and note identity required to construct each detail.
