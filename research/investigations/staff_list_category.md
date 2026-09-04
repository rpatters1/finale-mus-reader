# Marking-category staff-list investigation

**Covers:** Record identities, layouts, text width, introduction boundary, and baseline fill for
the three category staff-list classes.
**Read when:** Revising any category staff-list decoding or investigating forced-list overrides.
**Confidence:** user-supplied selectors with targeted independent binary verification.

## 2026-09-04 — Identities and layouts

The supplied legacy selectors identify category-list name `('C','L')`, score `('C','s')`,
parts `('C','p')`, score override `('c','o')`, and parts override `('C','o')`. Their zlib class
identities are respectively `0x012f`, `0x0130`, `0x0132`, `0x0131`, and `0x0133`.

Private corpus token `mus-da9e72c0a8ce7f8c` independently verifies the Finale 2009 score and parts
classes: cmpers 1–8 each have a twelve-byte payload beginning with `ffff` and followed by zero
fill. A big-endian installed Finale 2009 template verifies the corresponding word-packed name
class and the required within-word byte restoration; it also contains an exercised parts list
with values `-1, 14`.

**Strong.** The first all-corpus comparison exposed the counterintuitive target mapping that the mostly
sentinel-only samples could not distinguish. Across 21 distinct Finale 2012 documents from
`rpatters1-main`, all 68 differing value leaves cross-matched exactly: each `0x0130` value matched
the companion's `categoryStaffListParts`, and each `0x0132` value matched its
`categoryStaffListScore`. Direct inspection of `mus-559cbb2d3a3ca516` confirms the raw pair:
`0x0130` stores `-1, 2` and `0x0132` stores `-1, 3`, while the decoded Finale 27 companion stores
those sequences under parts and score respectively. The importer therefore preserves the legacy
selector names as physical identities but maps their contents to the opposite-named modern class.

Private corpus token `mus-7241e6dc648b8b9d` independently verifies the Finale 2012 name width.
Its twelve-byte payload spells `Tempo Staves` with one byte per character. Finale did not change
category-list names to Unicode storage within the legacy MUS saving range, so all supported
versions use platform conversion rather than the separate Finale 2012 symbol-codepoint rule.

**Strong.** The all-corpus recapture also rejects a fixed twelve-byte name limit. Token
`mus-76ef4d96d05bedff` stores four Finale 2009 names ranging from 6 through 18 characters, including
payloads longer than twelve bytes. Its Finale 27 companion contains no category-list name object;
the four companion-backed occurrences of three distinct Finale 2009 contents account for the 32
initially reader-only name leaves in the aggregate report. Each source's last-saver tuple has beta
development status 2 and exactly four legacy score and parts list cmpers. The comparison therefore
classifies the missing name leaves as beta discrepancies only when all three structural conditions
hold: Finale 2009 beta, reader-only legacy names, and exactly four source-owned lists of each kind.
A focused recapture of the four companion-backed occurrences classified all 32 leaves as beta
discrepancies, with no remaining reader-only or unexpected category-name leaf.

No `0x0131` or `0x0133` record occurs in either registered private record catalog. Their payload
meaning is therefore **open**; the importer reports and ignores any future occurrence rather than
guessing.

## 2026-09-04 — Introduction and baseline behavior

No category-list class occurs before internal major version 14 (Finale 2009). Existing Finale 27
companions of earlier files contain eight score lists and eight parts lists even though the source
cannot contain this record family. Both pinned Finale 27 New Document Without Libraries resources
contain exactly the same eight-plus-eight set, with value `-1` in every list and no stored names.

User-supplied product history states that early Finale 2009 releases exposed only four category
staff lists and that a later update expanded the set to eight. The exact update boundary is
**open**. Recovery does not need that version boundary: source-owned cmpers are imported and
tracked first, after which the reader requests each untracked cmper from 1–8 from the baseline.
Musxdom also refuses an occupied cmper atomically if a caller does request one.

The 2026-09-04 `tracked-evidence` recovery-coverage snapshot compared 224 sources with companions:
76 Coda-banner, 49 uncompressed, 65 DCL, and 34 zlib. All 224 sources and companions succeeded.
The parts and score observations each produced 3,584 exact leaves with no expected, unexpected,
reader-only, or companion-only leaves. Manual inspection of one result from every epoch found the
same eight baseline-filled lists in the three pre-F2009 epochs and exact category-list recovery in
the zlib sample. Category-list names produced zero leaves, so that snapshot does not strengthen
the focused name evidence above.

**Confirmed.** After correcting the score/parts target inversion, the authorized all-corpus recapture processed
16,321 occurrences representing 7,285 distinct source contents across `rpatters1-main`,
`rpatters1-installs`, and `tracked-evidence`. Of 16,232 successful source rows, 4,632 had successful
companions. Category names produced 378 exact leaves and 32 reader-only leaves; parts produced
74,207 exact leaves and score produced 74,267, with no unexpected or unmatched parts/score leaves.
The report had no unexpected difference in any surveyed class. The 89 failed occurrences were 58
Finale LIB files and 31 inputs that did not classify as Finale MUS documents, rather than failures
in category-list recovery.
