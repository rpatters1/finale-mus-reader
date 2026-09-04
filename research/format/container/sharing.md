# Sharing and linked parts

**Covers:** Zlib part ownership, structural sharing, the Finale 2002 controlled pair, and what Finale 27 references show about part scope.
**Read when:** Working on linked parts, part-scoped records, or the `shared` attribute.
**Confidence:** confirmed present; the pre-zlib encoding is open.

## Sharing and linked parts

**Confirmed present; encoding unresolved.** Finale 27 references show:

- 301 matched documents with more than one `partDef`;
- 315 documents with part-scoped records;
- 2,188,767 part-scoped converted elements;
- 1,156,180 `shared="true"` and 1,032,587 `shared="false"` attributes.

Code `0x011a` is `partDef`, now confirmed at byte level rather than by count; the class and its layout are in [`../others/part_definitions.md`](../others/part_definitions.md). Part scope and sharing are likely encoded in the remaining key/flag fields and/or duplicated records inside the ordinary “other” and detail pools, not a named sharing block. Finale 27 may expand shared relationships during conversion, so the counts do not prove the legacy representation is duplicated. A controlled link/unlink test is essential.

### Zlib part ownership and sharing

**Strong across controlled Finale 2012 edits and score/part companion comparisons.** The stable
class-record header fields correspond directly to the EnigmaXML object attributes: numeric class
id selects the XML class, the comparator fields select `cmper`/`cmper1`/`cmper2`, and the header
part id selects `part`. Score records have part zero and are always `ShareMode::All`.

Part sharing is stated by the physical record form rather than by a separate header policy value:

- a part record with a same-sized continuation is `ShareMode::Partial`; its primary payload is a
  physical part snapshot and its continuation selects the unlinked bits that replace score bits;
- a compact-layout part record is `ShareMode::Partial` and supplies only its compact fields over
  the corresponding score object;
- a full standalone part record without a continuation is `ShareMode::None`.

Each class importer supplies its compact layouts as score/part payload-size pairs. Multiple pairs
may describe one class if its representation changed during the zlib epoch; matching remains
structural rather than version-gated. The first strong example is others class `0x00b0`
(`measSpec`): 26 bytes per score tuple and 8 bytes per compact part tuple. This remains test-only
until the Measure importer owns the table. Other observed standalone layouts include `pageSpec`
(`0x00bb`, 24 bytes) and `staffSystemSpec` (`0x00df`, 36 bytes); both correspond to
`shared="false"`. Continued `pageTextAssign` (`0x00c2`) and `smartShape` (`0x00d9`) part records
correspond to `shared="true"`.

The controlled `F2012-noteartexp*` trio isolates the continuation behavior for others class
`0x00b1`. The linked score has only the score record. Unlinking the expression creates a part-1
record with a 24-byte primary payload and a 24-byte continuation. Moving the unlinked expression
24 EVPU changes its primary horizontal-offset word and changes the aligned continuation word from
zero to `0xffff`. Thus unlink creates the physical part snapshot with its mask clear, while an edit
marks the changed bits. Page-graphic evidence narrows this further: the `hidden` bit can be selected
independently from the page-selection bits in the same packed word. Applying the continuation as a
bit mask before class parsing produces exact companion values across the tracked and all-corpus
comparisons.

This later variable frame does not retain the earlier fixed 16-byte physical rows. Many payload sizes are multiples
of the old 12-byte other capacity, suggesting that later versions coalesced successive incidences into one
variable-length payload, but that historical relationship still needs field-level verification.

### Finale 2002 controlled pair

**Strong for this sample; broader version coverage pending.** The F2002 baseline and one-pitch-change pair were saved
by Finale `2002a.r1` under Mac OS 9.0.4 in SheepShaver. Beginning at `0x200`, both files expose a simple outer stream
of big-endian variable-length records:

| Offset | Type | Total record length | Baseline / changed |
|---:|---:|---:|---:|
| `0x0200` | `0x000f` | 1,976 | unchanged |
| `0x09b8` | `0x0010` | 158 | unchanged |
| `0x0a56` | `0x0011` | 53 / 57 | changed |
| `0x0a8b` / `0x0a8f` | `0x0012` | 6 | empty/end-pool marker moved with preceding record |

The six-byte outer header is two bytes of type followed by a four-byte **total record length including the header**.
Records are not constrained to 16 words or 32-byte alignment; the terminal record begins at an odd offset in the
changed file. Changing the pitch from C to D changes 45 observed bytes overall, including the length field and a
four-byte extension, while the complete `0x000f` and `0x0010` records remain identical. The ETF changes only the
`eE` entry payloads, supporting the interpretation that `0x0011` is an entry-related pool and that the edit is
localized at the outer-record level.

Subsequent DCL testing solved the payload codec and confirmed the same checked container across all recognized
2001–2006 samples. The fixed rows inside `0x000f`–`0x0011` are now established, while tag-specific logical field
layouts and `0x0012` text organization remain incomplete. F2003, F2004, and F2005 independently confirm the same
outer framing and broad type sequence.
