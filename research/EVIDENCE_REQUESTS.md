# Evidence Requests

The first ETF evidence set is now present locally under `private/evidence/` (ignored by git).
Requests remain deliberately small and hypothesis-driven. When multiple Finale versions are available, use the
**earliest compatible version** for the export and record the exact application version and maintenance/build number.
Source locations are intentionally omitted from this public document. Resolve each `corpus_id` through the local-only
`private/generated/<survey_id>/corpus_locations.csv` mapping (ignored by git); the public manifest supplies the size and hash, never the filename.

## Coverage gaps

### No Windows document earlier than Finale 3.0 — supplied and analyzed

**Closed 2026-08-11 with specimens.** The Finale 2.2 for Windows install disks were located and
their `.LIF` installer archives extracted, yielding 24 `.MUS` files: eight templates and sixteen
tutorial documents, all Windows-origin and all from inside the Coda-banner era. They are
little-endian, they carry a previously unrecorded banner spelling, and the reader rejects all of
them. Findings are in [FORMAT_NOTES.md](FORMAT_NOTES.md#determining-byte-order) and the reader work
is [P2.6](PRODUCTION_READINESS.md). The specimens live in the installs corpus, not in this
repository: they are Coda-authored sample content and are not publishable evidence.

The reasoning that preceded them is kept below, because it predicted them correctly from a single
Microsoft KB article and is a fair record of how the question was closed.

**Answered in principle 2026-08-11; the specimen is still wanted, and now matters more.**

A proposition floated and then withdrawn on the same day was that Finale for Windows began with
Finale 3.0, which would have made a pre-3.0 Windows document impossible rather than absent. It is
false. Microsoft Knowledge Base article Q107181, the README for Windows Sound System 2.0, names
"Finale 2.2 for Windows from Coda Music Technology" and mentions MusicProse for Windows beside it.
Finale 2.2 falls inside the Coda-banner era, so that era spans both platforms.

The corpus cannot see this: its earliest Windows-origin document is Finale 3.0, out of 20 spanning
3.0 to 2012, and no Coda-banner file carries a platform string at all. Absence in one corpus was not
absence in the world, which is the lesson worth keeping.

This raises the value of the request rather than retiring it. The container asserts big-endian for
the whole Coda-banner era, so a Windows document of that era would be misread, and one is now known
to be a thing that could exist. Any Windows-origin `.mus` from Finale 1.x or 2.x would settle both
the byte order and whether the era's pool structure is even the same across platforms.

Source, accessed 2026-08-11:
[Microsoft KB Q107181, revised 13 June 2001](https://jeffpar.github.io/kbarchive/kb/107/Q107181/).

Every file in the corpus from before Finale 3.x is Mac-origin and big-endian: all 54 Coda-banner then known
files, and the archive-derived Finale 1.8.7, 2.0.1, and 2.6 members. There is no Windows document
of that era at all, so nothing verifies how the reader would treat one.

That may reflect the era rather than the corpus. Binaries of that period are not believed to have
been transportable between platforms; a document moved between Mac and Windows had to travel as
ETF. Byte order is the likely reason, since Mac documents of the era are uniformly big-endian and
Windows documents uniformly little-endian. The two platforms were also on separate release
schedules, so a Windows release may not correspond to any Mac version already catalogued here.
When the schedules unified is not established: possibly as early as Finale 3.0, and certainly by
Finale 97, which is internal 3.8.

The Finale 3.0 files in the corpus are Windows-origin and little-endian while the 3.2 through 3.7
files are Mac-origin and big-endian, so by the 3.x line a single format was demonstrably written on
both platforms with byte order tracking the platform.

Consequences for the reader, none of them yet exercised:

- **Coda-banner byte order is asserted, not detected.** Those files have no block framing to
  trial-decode, so the container declares them big-endian on the strength of all 54 corpus files then known.
  A Windows document of the era, if such a thing exists, would break that assertion silently.
- **The pre-3.2 font table would be affected.** Font names are read as bytes and are byte-order
  independent, but nothing else about such a file has been tested.
- **A Windows document might not use this record vocabulary at all**, given the separate release
  schedules.

What would close it: see the note above. A Windows-origin document predating Finale 3.0 would settle
it directly; the corpus already holds Windows Finale 3.0 files, so the boundary is somewhere at or
before that. A Finale 98
document would additionally settle the two open version questions recorded in
[LEGACY_OPTION_MAPPINGS.md](LEGACY_OPTION_MAPPINGS.md) and
[FORMAT_NOTES.md](FORMAT_NOTES.md#font-definitions).

## Priority requests

### P1 — Analyzed — essential

- **Source:** `mus-d89e8fe12e271440`
- **Operation:** Open with the earliest available Finale version that accepts the file, without resaving first; export as Enigma Transportable/Portable (`.etf`). Record the exact application version/build and whether opening/export succeeds.
- **Preferred output:** An ETF export associated privately with this corpus ID.
- **Why:** This is a 3,351-byte, explicitly Finale 2005 file at the ETF compatibility boundary. Its Finale 27 export contains recognizable nested tuplet/entry data. DCL and physical rows are now solved, and the public Finale 2000 PDK identifies `TP` as the nested-tuplet entry-detail tag. This pair is retained to map the version-expanded `TP` fields and their relationship to the 38-byte entry rows.

### P2 — Analyzed — essential

- **Source:** `mus-3597fd4fce0c272b`
- **Operation:** Attempt to open with the earliest available compatible Finale version and export ETF without deliberately changing the document. Record success and the exact application version/build.
- **Preferred output:** An ETF export associated privately with this corpus ID.
- **Why:** The header says Finale 2000 and the document is small (19,557 bytes) and template-like. It tests defaults and upgrade synthesis with little musical noise. The later exact `mus-3a8b724cf3adba80` pair and corpus-wide probe have now answered the physical question: Finale 3.x–2000 uses uncompressed 16-byte other/detail and 38-byte entry rows.

### P3 — Proposed — important

- **Source:** `mus-a23053bf74c5e11a`
- **Operation:** First try the earliest available compatible Finale version. Export ETF without resaving if it opens; report the exact version/build and, if it fails, the earliest version attempted.
- **Preferred output:** An ETF export associated privately with this corpus ID.
- **Why:** This is the smallest file whose explicit banner says Finale 3.0 (38,565 bytes). The corpus probe now proves it is a Windows-origin, little-endian instance of the four uncompressed pools. Its ETF would test whether field values and logical identities are exactly byte-swapped equivalents of the Mac Finale 2000 pair.

### P4 — Supplied/Analyzed — essential

- **Source:** `mus-3a8b724cf3adba80`
- **Operation:** Open directly in Finale 2000 without resaving and export Enigma Transportable/Portable.
- **Supplied output:** A 28,718-byte ETF export, stored locally under ignored `private/evidence/` (SHA-256 `c02e859d6026de960a44ea07bd0d3154e07e7f85d690cc5eaf5a84b623d3149d`).
- **Why/result:** The musically small file contains entries, note alterations, tuplets, beams, and stem details. Its exact pair proves four uncompressed pools, one-for-one 16-byte other/detail rows, eight 38-byte entries, literal tag ordering, and byte-identical raw text after ETF separator removal. It triggered corpus-wide confirmation across 189 Finale 3.x–2000 files.

### P5 — Proposed — useful

- **Source:** `mus-c7e0faa94df7fc67`
- **Operation:** Open and export ETF in Finale 2004 or Finale 2005; record application version.
- **Preferred output:** An ETF export associated privately with this corpus ID.
- **Why:** At 3,294 bytes, this is an unusually small Finale 2004 file with recognizable beam/detail structures. DCL/framing is already confirmed for Finale 2004; the export would provide strong decoded detail-record candidates and field-level semantic correlations.

## Controlled-difference requests

### A1 — Supplied/Analyzed — essential for earliest banner-era files

The archive survey found explicit pre-banner files labeled `Finale(TM) 1.8.7`, `2.0.1`, and `2.6`. The first three
selected samples were exported and analyzed locally: `mus-7aa45639c14b3864` (1.8.7),
`mus-2c0a5e8897b436d5` (2.0.1), and `mus-bd0042f8e0354192` (2.6). The Finale 2.6 source had to
come from the parallel StuffIt archive because the ZIP copy did not preserve its classic Mac resource fork. These
are the earliest explicit-version binary samples currently identified. Finale 27 counterparts have now been supplied
for all three after copies were given `.mus` suffixes; the extensionless originals were not recognized directly.

### A2 — Analyzed — important archive-format coverage

The installed `unar`/`lsar` 1.10.7 tools were used to inspect all 275 `.sit` archives, preserving archive/member hashes and resource-fork observations. The results are incorporated in `ARCHIVE_SURVEY.md` and the public archive data. No further blanket extraction request remains; future archive work should target named members only.

### A3 — Proposed — useful cross-era binary controls

Use archive candidates `mus-ee1382238443129b` (Finale 2007, validated big-endian framing) and `mus-43c11614815f485c` (Finale 2008, validated little-endian framing) as controlled comparison targets if their documents can be opened. Export ETF where supported or save equivalent copies from the indicated Finale versions. The purpose is to test whether the observed endian transition changes only serialization or also record identities and payload meanings.

### W1 — Proposed — essential platform coverage

Supply additional Windows-origin files, preferably with exact Finale version/build provenance, covering at least one minimal document and one musically rich document from Finale 2001–2006, 2007, 2008, and 2012. For the 2007/2008 pair, save the same document on Mac and Windows where possible. Preserve source hashes, header bytes, platform tuples, and resource-fork absence. Four existing Windows files now prove little-endian serialization of the same Finale 3.x–2000 pools, but later Windows coverage remains insufficient.

### C1 — Proposed — useful for decoded record mapping

Using the earliest available Finale version that supports ETF, create a minimal one-staff/one-measure document and save/export four pairs (`.mus` and `.etf`): empty measure, add one quarter-note middle C, change only that note to C-sharp, and attach one articulation. Do not change layout between saves. DCL decoding is solved; this isolates entry, pitch/alteration, and articulation changes in the decoded pools.

### C2 — Proposed — important for transition

If both an early Finale 2007 and Finale 2008 installation are available, open the same minimal document and save one `.mus` from each without other edits. Record exact maintenance/build numbers. This tests the observed big-/little-endian record serialization transition and whether type codes/payloads stayed stable.

### C3 — Proposed — essential for sharing

In Finale 2012, create a score plus one linked part with one expression and one articulation. Save A with all items shared; save B after unlinking only the expression; save C after changing only the part's expression. Supply all `.mus` files and Finale 27 `.musx` conversions. This isolates the binary fields behind `part`, `shared=true/false`, duplication, and override behavior.

### C4 — Proposed — essential for the Finale 2.6.3 boundary

In Finale 2.6.3, create a new minimal document using that version's ordinary default-document command. Keep one staff
and one measure and enter three quarter notes C4, D4, and E4. Save the document once as `minimal-Fin263.mus`, then
immediately export `minimal-Fin263.etf` without another edit or resave. Record the exact Finale build and platform.
Preserve the resource fork when moving the MUS file. This pair should expose the early ordinary/detail/entry indexes
with very little noise; general Finale 27 compatibility through 2.6.3 is already confirmed.

### C5 — Partly analyzed — Finale 1.0.0 does export ETF

Repeat C4 in Finale 1.0 as `minimal-Fin100.mus` and `minimal-Fin100.etf`. If Finale 1.0 has no ETF export command,
preserve the MUS file and record that fact rather than exporting it through a later Finale version. Try a `.mus`-
suffixed copy in Finale 27 and record the result. This is the highest-value test of whether the 1.8.7–2.6 fixed-row/
indexed model and verified Finale 27 compatibility extend to the first release.

**Answered 2026-08-11 for the export question.** `tests/evidence/F100/F100-clef-baseline.etf` is a Finale 1.0.0 ETF
export, and `tests/evidence/F263/F263-clef-baseline.etf` its Finale 2.6.3 counterpart. Both carry the era's own
banner, `ENIGMA transportable file` followed by `ENIGMA Structures Copyright 1987 by Coda.`, so they are written by
the era rather than by a later Finale reading the file. Finale 1.0 therefore has an ETF export command, and the
Coda era now has a textual reference of its own.

They already earn their keep. Both show `^NN(65534)` globals with the same six words the MUS rows hold, which
independently confirms the clef records at selectors 28 through 35 and their word 4 baseline adjustment. The
Finale 2.6.3 export also shows `^24(65534) 13 69 52 48 65 60`, matching the Finale 1.0.0 census and confirming from
a second era that selector 24 is not the default-font array in the Coda era. The remaining part of C5, a minimal
document paired across the C4 matrix, is still worth having.

### C6 — Proposed — essential for option-map verification

In Finale 2000 or Finale 2005, create one no-libraries baseline document and save both MUS and ETF. From the same
baseline, create five copies that each change exactly one visible option, then save MUS and ETF without allowing other
automatic layout changes:

1. music-spacing minimum width (`^94(65534)`, incident 0, word slot 1);
2. right tie thickness (`^84(65534)`, incident 0, word slot 0);
3. score page width (`^15(65534)`, incident 0, word slots 2–3, Mac high-word first);
4. forward-repeat spacing (`^70(65534)`, incident 0, word slot 4); and
5. show fretboards (`^41(65534)`, incident 0, word slot 2).

Use distinctive nondefault values and record the exact UI value, Finale version/build, and platform for each copy.
Store publishable pairs under `tests/evidence/options/<version>/`. This is the smallest high-value test of the
private-framework-derived mappings and exercises two-byte, four-byte, numeric, boolean, and five separate musxdom
destinations.

### C7 — Analyzed — default-font sequence and early-location verification

Exact Finale 27 companions now accompany every tracked MUS fixture. Their hashes and the Finale 27.4 conversion
provenance are recorded in `tests/evidence/finale27-provenance.txt`. The controlled Finale 2002–2012 pairs confirm
that companions carry a complete modern vector but do not preserve one timeless physical order: Finale 2002 maps
physical 28 to tablature while physical 13 is a holding slot, and Finale 2003–2006 physical 43 is zero structural
fill in the second half of the final fixed row.

Fourteen newly authored Finale 1.0.0 Mac fixtures provide a baseline and thirteen one-variable UI saves. The source files were
created by Robert G. Patterson under Mac OS 9 hosted by SheepShaver, and their Finale 27 companions were made by the
same automated conversion. They locate every font exposed in Finale 1.0.0's Font Preferences UI; the complete field
table is in `FORMAT_NOTES.md` and their provenance and hashes are in `tests/evidence/F100/provenance.txt`. Features
not exposed in that UI remain open; companion-only additions or coupled changes are upgrade synthesis and are not
source-location evidence.

The comparisons resolve IDs independently through normalized font names and compare `(name, size, effects)`, never
numeric cmpers. The detailed method and remaining early-version work are in
[LEGACY_OPTION_MAPPINGS.md](LEGACY_OPTION_MAPPINGS.md#fontoptions-sequence-verification-strategy).

### C8 — Analyzed — the two open ClefOptions fields, both now settled

Two `ClefOptions` questions cannot be settled by any corpus file, because every specimen has the
default value on both sides of the comparison. Both need one controlled toggle. Any Finale from 2003
to 2006 is the most useful source, because that era is fully covered by other evidence; note the
exact version, build, platform, and UI value, and save a Finale 27 companion for each.

1. **The clef baseline word.** From a no-libraries baseline, open the Clef Designer and give one
   clef — index 0 is easiest to find — a distinctive non-default baseline adjustment. This is the
   only word of the clef tuple that is zero in all 1,268 corpus specimens. It settles three things
   at once: that tuple word 3 is where the value lands, whether that era stores it in Efix or in
   harmonic levels, and therefore whether the reader's unit boundary at Finale 2001 is right. A
   second copy from any pre-2001 Finale, with the same edit, would settle the earlier half directly.
2. **The courtesy-clef bit.** From the same baseline, turn off "Display Courtesy Clef at End of
   Staff System" and leave the key and time signature courtesies on. Selector `44` word 3 holds all
   three as bits; the corpus contains only the values `7` and `5`, which eliminates bit 1 and leaves
   bits 0 and 2 as candidates for the clef. One save with only the clef courtesy cleared identifies
   it. A second save clearing only the key courtesy would confirm the assignment rather than infer
   it by elimination.

**Set 1 supplied and analyzed 2026-08-11**, as `F2005-clef-baseline`, `F100-clef-baseline`, and
`F263-clef-baseline` with ETF and Finale 27 companions. Results are in
[FORMAT_NOTES.md](FORMAT_NOTES.md#clef-definitions): the 2001-and-later unit is Efix in a signed
16-bit word, **confirmed**; the pre-2001 slot is word 4 of the clef's own selector, **confirmed**,
which corrected a wrong mapping that had been reading word 1 and affected 90 corpus files. The
harmonic-level-to-Efix conversion stays **weak** and now cannot be settled by a companion at all,
because Finale 27 discards the early value rather than converting it.

**Set 2 supplied and analyzed 2026-08-11**, as `F2005-courtesy-clef-off`, `F2005-courtesy-key-off`
and `F263-courtesy-key-off`. `cautionaryClefChanges` is bit 2 of selector 44 word 3 and
`cautionaryKeySigChanges` is bit 0, **confirmed**; the second save is what distinguishes the clef
bit from a bit that merely happened to be clear. The Finale 2.6.3 save shows the Coda era storing
these as separate boolean words in selector 12 rather than as a packed word, so that era is
excluded and its clef word is the one piece still open. The corpus could not have settled any of
this: all 1,120 companions have the option set.

### C9 — Proposed — useful for the early clef record

Two questions the Set 1 fixtures raised rather than answered. Both are small and neither blocks
anything.

1. **The "use baseline adjustments" checkbox. Answered 2026-08-11 for Finale 3.x.** The Finale
   3.7.2 pair locates it in bit 0 of word 5 of selector 28, governing the document rather than one
   clef, and Finale 97 turns out to have dropped the checkbox and adjust unconditionally. What
   remains open is only the Coda era: the Finale 1.0.0 save left no trace anywhere in the others or
   details pools, so either that era does not persist the setting or it lives in the Coda-banner
   directory region the container does not decode. A save toggling **only** the checkbox, changing
   no values, would settle it; byte-identity with its baseline would be the answer.
2. **Word 1 of the early clef record.** It is populated in the Coda era — `6, 0, -2, -6, 6, -1, -13,
   -4` for selectors 28 through 35 — and zero from Finale 3.0 onward, and no controlled edit so far
   has moved it. If any Clef Designer field remains unaccounted for after the checkbox is settled,
   one save changing it on clef 0 would identify the slot.

### C10 — Proposed — settles the Coda `Name` fan-out

The importer propagates the Coda era's single `Name` font preference to all four modern name
types (see FORMAT_NOTES, "The single `Name` preference reaches all four modern name types"). The
57 Coda-era companions cannot confirm or refute this, because all 57 were upgraded under a
personal Maestro default file: they report `Times 16`, `Monaco 16`, or `Pmusic 12` for the name
types while the source tuple reads `Times 14` in every one of them, so they describe the default
file rather than the source.

**What would settle it:** one Coda-era document upgraded to Finale 27 under a *stock* default
file, with the `Name` preference set to something distinctive — a face and size that appear
nowhere else in the document. Three outcomes are each decisive: the companion carries that face
and size on all four name types (fan-out confirmed), on `StaffNames` only (fan-out should be
dropped), or on none (Finale 27 genuinely discards `Name`, and the divergence is deliberate).

### S1 — Supplied/Analyzed — settles the pre-Finale-3.5 stem lengths

Two controlled Finale 1.0.0 saves are now tracked as `tests/evidence/F100/F100-stemopts-changed.*`
and `F100-stemconn-disabled.*`. The first lengthens the normal and shortened stems by one staff
position each and switches off "Display Reverse Stemming", which moved selector `20(65534)` words
4-5 from 7 and 5 to 8 and 6 and selector `41(65534)` word 1 from 0 to 1. Its companion moves the
lengths to 96 and 72 and gains `<noReverseStems/>`, confirming both the staff-position unit and
the Coda era's own bit for that flag.

### S2 — Supplied/Analyzed — the Finale 1.0.0 stem-connection switch

Settled by `tests/evidence/F100/F100-stemconn-enabled.*`. Enabling stem connections moves selector
`31(65534)` word 5 from 0 to 1 and moves nothing else in the file; the companion gains
`<useStemConnections/>` and the era's own ETF shows the same word. The earlier "Disable" save
remains as the no-op control: it was made on a document that was already disabled, which the
Finale 1.0.0 dialog does not indicate.

### S3 — Supplied/Analyzed — the half-stem length and the early reverse-stemming bit

Settled by `tests/evidence/F372/F372-revstem-halfstem.*`. Two words move and no others:
`03(65534)` word 2 goes 18 -> 19 and `41(65534)` word 1 goes 0 -> 1, with the companion following
on both. That confirms the half-stem location, and shows Finale 3.7.2 spells the flag in bit 0
rather than the framework's bit 2. The Finale 3.0-3.4 question it was also meant to answer is now
moot: the reader dates the spelling from the word's own contents rather than from a version
boundary.

### S4 — Supplied/Analyzed — the packed reverse-stemming bit, and the modern stem length

Settled by `tests/evidence/F2002/F2002-norevstem-len96.*`. Exactly the predicted words move:
`41(65534)` word 1 goes 26 -> 30, a gain of 4, and `20(65534)` word 4 goes 84 -> 96. The companion
carries `<noReverseStems/>` and `<stemLength>96</stemLength>`. Every StemOptions location is now
measured rather than distilled.

### S6 — Proposed — what the Coda "Offset" means

`21(65534)` word 0 is the augmentation-dot upstem-flag adjustment from Finale 3.0 on, confirmed
against companions. A Finale 1.0.0 save reached the same word through a stem dialog labelled
"Offset", and Finale 27 discards the value when converting that era, so the companion cannot
adjudicate. If Finale 1.0.0 or 2.6.3 exposes an augmentation-dot adjustment anywhere else in its
UI, changing it and seeing whether `21(65534)` word 0 moves would settle whether the era shares the
later meaning. Low priority: nothing reads that word today.

### S5 — Supplied/Analyzed — the last stem inference

Closed by `tests/evidence/F100/F100-revstem-25.*`. Setting the reverse stem adjustment to 25 moves
`21(65534)` word 2 from 18, the ETF shows `^21(65534) 4 8 25 6 4 4`, and the companion carries
**300** — twelve times the stored number, matching the factor the two stem lengths establish.
`StemOptions` now has no inferred location, unit or bit anywhere in its four epochs.

### T1 — Supplied/Analyzed — the accidental symbol inserts and eleven text scalars

Settled by nine fixtures: `tests/evidence/F2005/F2005-insert-sharp-font.*`,
`F2005-insert-sharp-font-it-und.*`, `F2005-insert-flat-tracking.*`, `F2005-textopts-scalars.*`, the matching
`F2012-baseline`, `F2012-insert-flat-tracking` and `F2012-textopts-scalars`, and the Coda-era
`F100-dateform-tab.*` and `F263-dateform-tab.*`.

The two font saves put 9, 79 and 0x01 — then 0x06 — into offsets 10, 12 and 14 of the sharp element of
`78(65534)`, with the companion resolving 9 to the source's own `^FN(9) "Petrucci"` and the bits to bold, then
italic plus underline. The tracking save puts 1000, 250 and −25 into offsets 0, 4 and 8 of the flat element,
which fixes both 32-bit widths, the high-word-first order and the sign of the baseline shift. The scalars save
moves five records at once and locates eleven fields in `81`, `82`, `83`, `05` and `13`. The two Coda saves move
`05` word 5 to 1 and 2 and `13` word 0 to 7, showing that date format and tab spacing sit in the same words in
the earliest era as in the latest.

Full results are in [FORMAT_NOTES.md](FORMAT_NOTES.md#text-options).

### T2 — Supplied/Analyzed — the four text fields no corpus document varies

Both saves behaved exactly as predicted, each moving one word and nothing else.

`tests/evidence/F2005/F2005-textvert-center.*` moves `83` word 1 from 0 to 2, with the ETF reading
`^83(65534) 0 2 0 0 0 0` and the companion gaining `<textVertAlign>center</textVertAlign>`. Center at 2, against
the earlier fixtures' `bottom` at 1, fixes the legacy vertical list as `Top, Bottom, Center` — the same
first/opposite/centre order as the two enums already confirmed — and leaves word 3 as `textIsEdgeAligned`.

`tests/evidence/F97/F97-expword-off.*` moves `82` word 5 from 1 to 0, with the ETF reading
`^82(65534) 100 1 1 0 0 0` and the companion losing `<textExpandSingleWord/>`. That fixes word 5 and leaves word
1 as the percent/Evpu line-spacing selector.

`tests/evidence/F2005/F2005-linespace-to-evpu.*` closes the last residue. It moves `82` words 0 and 1 and nothing
else, `[100, 1, 1, 0, 0, 1]` → `[72, 0, 1, 0, 0, 1]`, with the ETF reading `^82(65534) 72 0 1 0 0 1`; the
companion replaces `<textLineSpacingPercent>100</…>` with `<textLineSpacingEvpu>72</…>` and keeps
`<textExpandSingleWord/>`. Word 1 set means percent, clear means Evpu, and word 0 is the value either way.

That save was needed because Finale 27 has no boolean of its own for the mode. It writes one spelling or the
other and never both, so there is nothing on the companion side to compare a recovered flag against; before this
fixture the word was identified only by elimination against word 5, which the Finale 2012 scalars save had moved
at the same time. The reader needs word 1 solely to choose which musxdom member receives `82` word 0.

### T3 — Proposed — the Finale 3.7–2000 insert layout

The introduction boundary is no longer part of this request. The repository owner's inspection of
the Finale 3.7 addendum confirms that configurable text inserts first appeared in that release's
UI, agreeing with the record family's absence through Finale 3.5 and presence from Finale 3.7.

No controlled save exists for the 17-byte layout, and no companion can stand in for one: Finale 27 mis-converts
that era on all 179 documents that have the record. One save from Finale 97 or 2000, changing the flat insert's
tracking before to 1000, tracking after to 250 and baseline shift to −25 — the same edit as the Finale 2005
fixture — would confirm the two 32-bit widths there directly rather than by inference from the later layout.

**A Windows-origin Finale 3.x–2000 document would answer a second question** that no Mac file can. The era's
structure reads correctly only when the payload's 16-bit words are swapped, and every observed file of the era
is big-endian, so "stored opposite to the container" and "always little-endian" cannot be distinguished. This is
low priority: the reader gives the same answer under either explanation, and no such document is known to be
available.

### L1 — Proposed — the eight lyric fields nothing varies

**The highest-value single request in the class, and the smallest.** Eight `LyricOptions` fields have no located
storage: `useSmartWordExtensions`, `wordExtNeedUnderscore`, `lyricPunctuationToIgnore`, `lyricAutoNumType`, and the
three `showAutoNumbersOn…` flags. Every one is invariant across all 69 tracked fixtures *and* absent from the
framework's lyrics preference map, so neither source can locate them and no corpus survey of any size will either —
this is the `82`/`83` situation from
[T2](#t2--suppliedanalyzed--the-four-text-fields-no-corpus-document-varies) again.

**Three fields were on this list and have been struck off without a fixture**, because the repository owner supplied
the boundary instead: `hyphenChar`, `useAltHyphenFont` and `altHyphenFont` all postdate Finale 2012, the last
release this reader opens, so no legacy file can state them and no save could ever have found them. It is worth
noticing how much cheaper the answer was than the save it replaced: the corpus could only ever have shown that
nothing varies, which is what an absent option and an unfound one look like alike.

One Finale 2005 or 2006 save, from the same baseline as the existing `F2005-*` fixtures, changing **only** the
Lyric Options dialog and moving as many of the remaining eight as the dialog exposes at once: switch off "Use Smart
Word Extensions", switch on "Only Create on Lyrics With Underscores", and set automatic lyric numbers to "Keep with
First Syllable" so `lyricAutoNumType` leaves `align`. Save the MUS and the ETF. Moving several at once is
acceptable here rather than sloppy, because none of them is currently located at all: the first pass only needs to
find which selectors move, and a second single-field save can separate any two that land in one word. **Check
first which of the eight that era's dialog actually exposes** — any it does not is a candidate for the same
after-2012 answer the three struck-off fields got, and asking is cheaper than saving.

### L2 — Supplied/Analyzed — the edge punctuation bit and the punctuation set, both closed

**Both fields are now mapped, and neither could have been closed by a corpus.**

`lyricUseEdgePunctuation` is selector `57` word 4, class `0x0047` byte 8, live from Finale 2012. That one came from
the reference corpus rather than a fixture: Finale 2012 is the only release whose documents vary, thirteen record
bits partition that cohort exactly because the split is really born-in-2012 against upgraded-into-2012, and a
negative control in Finale 2008 cuts those thirteen to one candidate.

`lyricPunctuationToIgnore` needed the fixture, and the request predicted the wrong shape. It is **not** a cmper into
a text pool but a **variable-length tail on the same selector 57 record**, written only when the list differs from
the stock set — which is exactly why the corpus was silent: the element takes two values across all 1,189
companions, absent or stock, and the stock set appears in no Finale 2012 file at all.
`tests/evidence/F2012/F2012-lyric-punct.mus` sets it to `#@%&` and the record grows from twelve bytes to
twenty-four, the six scalars unchanged and the four characters following as 16-bit code units and a zero. An absent
tail means the stock list, which musxdom's `integrityCheck` already supplies.

The fixture repaid itself twice over. Finale rewrote the word-extension connection table as the dialog closed,
giving five of nine styles a vertical offset of 5 and a sixth an offset of 1, so it is also a second non-default
specimen for a collection that otherwise rested on one Finale 2006 document.

**Supplied and analyzed:** `F2012-lyropts-noign-punct.mus`, the checkbox cleared and nothing else. It confirms the
mapping the corpus had predicted before the fixture existed — byte 8 of class `0x0047` moves 0 to 1, no other word
of that record moves — and it is the only published document anywhere with the switch cleared, so it is the sole
fixture exercising the word set rather than clear. It also shows the switch and the tail are independent: ignoring
is off and the list is stock, and the record stays twelve bytes.

**One question this pair leaves open.** Two corpus documents ignore punctuation while carrying no
`<lyricPunctuationToIgnore>` element at all, which neither fixture explains. A Finale 2012 save that clears the
punctuation list *entirely*, with ignoring left on, would say whether an empty list is representable at all or
whether those two documents mean something else.

### L3 — Analyzed — the pre-2004 vertical offset, and a correlation that was a coincidence

**Closed, and the hypothesis it rested on was wrong.** The request supposed that Finale 27 synthesized the starting
connection's vertical offset as 1 or 4 according to the word-extension syllable positioning bit, because six fixture
groups agreed and nothing else separated them. There was no rule: those documents simply store the value in
selector `29` word 5, which this study had not yet found.

`tests/evidence/F100/F100-wext-push-6-lift-5.*` names it, along with its horizontal partner on selector `30`. Both
exist in every era including the Coda banner, and they are the only lyric values that era stores — Lift and Push
are the whole of its dialog. `tests/evidence/F372/F372-lyricopts-changed.*` confirms them again in the uncompressed
epoch alongside hyphen spacing and line thickness, the two settings Finale 3.7.2 adds.

Worth keeping as a caution: a correlation across six groups looked like a rule for as long as the real field was
missing. The fix was not a better statistical test but a controlled save in the era that had the fewest options to
confound it.

**One residue.** Where the era stores no connection table, Finale 27 also moves the `oneEntryEnd` element's
horizontal offset with Push — 42 to 44 in the Finale 3.7.2 pair. One specimen cannot distinguish that from several
formulas that fit it, so the reader keeps the pinned baseline's 42 and the difference is intended. A second save
moving Push to a different value would settle it, and is low priority.

### X1 — Proposed — the two lyric keywords nothing has produced

The text pool names each chunk by keyword, and `verse` is confirmed by `F2007-lyric-hyphens.mus`. `chorus` and
`section` are inferred from it and from musxdom's own naming; no fixture contains either. Any document with a
chorus or a section lyric, in any epoch from Finale 97 on, would replace the inference with an observation. One
document can carry both.

The cost of being wrong is small but silent in the wrong direction: an unrecognized keyword is reported by name,
so the *right* spelling would announce itself the first time such a document is read. That makes this a cheap
request rather than an urgent one.

### X2 — Partly closed — the pre-Finale-2012 bookmark object

The Finale 3.5 addendum confirms that bookmarks were introduced in that release. Controlled Finale
3.7.2 and 2012 saves locate the earlier text in the `BK` others family and the later UTF-8 text in
the `bookmark` text-pool record. Text recovery from `BK` remains deliberately deferred until the
bookmark class itself is imported; a Finale 3.5 specimen would establish whether its object and
text representation already match Finale 3.7.2.

### X3 — Proposed — where expression text moved into the text pool

Finale 2000 keeps expression display text inside the `DT` record; Finale 2006 keeps it in the text pool and uses
`DT` for the expression's description instead. No tracked fixture between them defines an expression at all, so
the reader gates the `DT` reading to the uncompressed epoch and recovers no expression text from a Finale
2001–2005 document that still uses the old layout.

**One document per release from Finale 2001 to Finale 2005 with a single text expression defined would close it**,
and a Finale 2002 save alone would narrow it usefully. A save that also fills in the expression's description
would show whether the two strings ever coexist, which is what would decide between a version gate and a
structural marker.

### X4 — Partly analyzed — File Info after Finale 97

The four header offsets that carry title, composer, copyright, and description are confirmed only on the Finale 97
pair. Every other tracked fixture leaves all four empty, which is consistent with the offsets still holding and
equally consistent with their having moved. The reader applies them to every banner-bearing epoch, so a later file
that keeps something else there would produce nonsense rather than nothing.

**A document with all four File Info fields filled in, saved by Finale 2006 and by Finale 2012**, would confirm or
refute the offsets in both compressed epochs at once.

**Half of this closed on 2026-08-18, from the other direction.** `F2006-text-inserts.mus` shows that Finale 2006's
Text Tool offers a `^description` insert and no `^lyricist`, `^arranger` or `^subtitle`, so File Info still has
exactly four fields at that release. The three musxdom also names are therefore not a gap in the mapping for any
era this reader covers, and the request for "the first evidence for lyricist, arranger and subtitle" was asking
for something that does not exist yet in that era. What is still wanted is a later release that has them, which is
the same fixture X5 asks for.

### X5 — Supplied/Analyzed — the binary command codes, and what is left of them (complete)

**Closed for 25 codes on 2026-08-18 by `F2006-text-inserts.mus`.** From Finale 2006 the MUS stores Enigma commands
in a binary form — a caret, a one-byte code, and a hexadecimal-digit argument — that no public source describes and
that no plug-in could ever see, because both the ETF export and the PDK hand back the spelled-out form.

Finale 2006 is the only release that can decode it: the first to write the codes and the last to export ETF, so one
document exists in both representations. The fixture holds one text block per insert its Text Tool offers, each
containing that insert alone, so the block number keys the two files to each other. Its Finale 27 companion agrees
with the ETF on all 21 records, so each pairing has two independent witnesses. The table is in
[FORMAT_NOTES.md](FORMAT_NOTES.md#enigma-commands-in-the-compressed-epochs).

A second pass over the same document added the three style commands, and with them the two facts about the
argument encoding that nothing else could have supplied: `^superscript(15)` is the only place a nibble of `0xf`
is stored anywhere, and `^baseline(-13)` is the only negative argument. Both had been extrapolated, and the
signedness had been extrapolated wrongly.

**Closed for five more codes on 2026-08-18 by `F2008-BE-text-inserts.mus`**, which placed `^partname` at `0xa0`
and `^lyricist`, `^arranger` and `^subtitle` at `0xa1` to `0xa3` — appended, not slotted into the alphabetical run
where this request predicted them — and `^time` at `0x98`, the one prediction that held.

**Closed for the last four on 2026-08-18 by `F2011-text-inserts.mus`.** Finale 2009 introduced marking categories
and Finale 2010 automatic rehearsal marks, so Finale 2011 is the earliest available release able to write
`^fontTxt`, `^fontMus`, `^fontNum` and `^rehearsal` in one document; they are at `0xa4` to `0xa7`. **Every command
`musx/util/EnigmaString.h` documents now has a code**, and no fixture in the tracked corpus reports an unread code.

**Still open, and no longer predictable:** `0x80`, `0x82`, `0x83`, `0x89`, `0x93`, `0x97`, and anything from `0xa8`
up. Both predictions this request made from the ordering of the table were refuted by the first fixture able to
test them, and the second refutation is the more general one: appending is what every release after the original
set does, for style commands as much as for inserts, so neither group has grown in place since. A slot inside
either group is an unexplained hole, not a reserved one. Filling any of the six needs a document that uses the
command, not an argument about where it would sort — and there is no longer a named command left to look for.

**Worth having anyway:** the same one-insert-per-block document from **Finale 2012**, which crosses the Unicode
boundary. The reader accepts the two-byte code spelling structurally rather than by version, but that has been
exercised on three codes, not thirty.

### X6 — Proposed — text in a Coda-banner document

The container's pool walk is fixed: an empty pool no longer ends the chain, so a Finale 1.0.0
document now reports all three of its pools and the text region behind them. What is still
undecoded is the `HT` others family, where that era keeps its block text as a stream alternating
NUL-terminated strings with binary layout data across incidences.

**The evidence available is thinner than the fixture count suggests.** Twenty-six Coda-banner
documents are tracked. All 21 Finale 1.0.0 ones contain no text whatsoever — their companions
carry only the `Score` part name, which lives in `PartDefinition` and is not yet imported — and the five Finale 2.6.3 ones are five
saves of a single document, so the era has exactly **one** text specimen. Its Finale 27
companion recovers eleven block texts and seventeen expressions, which is a target to decode
against but not a way to separate text bytes from the layout bytes woven through them.

**The request, in priority order:**

1. **A controlled Coda-banner pair differing in one block's text.** One document saved twice,
   the second with a single block's string changed to a different length — six characters to
   twelve, say, so any length or offset field moves visibly. This is the single highest-value
   item: the interleaving is the whole unknown, and a one-variable edit is what isolates it.
   Either release will do, though 2.6.3 is the one already known to store text.
2. **A Finale 1.0.0 document with block text**, one short distinct string per block so each is
   identifiable by content. The 1.0.0 layout is entirely unexercised, and it cannot be assumed
   to match 2.6.3's: the two releases spell the text-region markers differently, `^text \0` and
   `^lyric \0` against `^text()` and `^lyrics()`.
3. **A Coda-banner document with lyrics.** No tracked document of the era has a verse, chorus or
   section, so nothing shows whether lyric text lives in `HT` alongside block text, in the
   `^lyric` region that is empty everywhere, or somewhere else again. The three fixtures named
   for lyrics are lyric *option* edits and carry no syllables.

A fourth would settle the argument encoding rather than the framing: this era writes binary
commands with a shorter argument form than the later epochs, so the digit rule the compressed
eras follow does not apply. Any of the above exercises it incidentally.

### X7 — Partly answered — where File Info begins

`texts::FileInfoText` is recovered from header offsets in Finale 97 and from the text pool in
Finale 2008. Neither end of the range is established. The lower end may be around Finale 3.7
rather than earlier, which would mean the header offsets this reader reads are meaningless for
the releases before it.

**No corpus can answer this at any size.** A File Info dialog nobody filled in leaves exactly
the same empty offsets as a File Info dialog that does not exist, and only three tracked
documents anywhere have the fields filled: two purpose-built Finale 97 fixtures and the Finale
2008 one. Every other document in every era is silent.

**The lower end is bounded, not answered.** `F372-fileinfo-text.mus` carries all four fields at
the header offsets with a companion that recovers all four, so File Info exists by Finale 3.7.2,
and that release is also the earliest that writes the modern inserts reading those fields. It is
the earliest release *available here* whose dialog offers them, which makes it a ceiling on the
arrival rather than the arrival itself. Testing an earlier release needs an earlier release.

**The upper end is answered by `F2002-fileinfo-text.mus`:** the header offsets still hold in the
DCL epoch, all four fields recovered and confirmed by its companion, so the move to the text pool
happens somewhere after Finale 2002. Narrowing it further would need a Finale 2005, 2006 or 2007
document with the fields filled, and the reader needs no answer to work — it fills from the
header only what the pool did not supply.

**A correction to how this request was framed.** It claimed no corpus could speak to the DCL
placement because no *fixture* had File Info filled in. That confused the tracked fixtures with
the surveyed corpora, which do contain documents of that range carrying at least some of the
fields. The reasoning holds only for the lower bound, where an unfilled dialog and an absent
dialog leave identical bytes; it never held for the placement question, which any filled document
of the era settles.

## Status legend

- **Proposed:** documented but not yet requested/supplied.
- **Requested:** user has initiated the evidence creation.
- **Supplied:** file is present but not analyzed.
- **Analyzed:** incorporated into the notes and catalog.
- **No longer needed:** superseded by stronger evidence.
