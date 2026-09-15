# StaffUsed

**Covers:** Legacy `StaffUsed` record layouts, range recovery, list completion, and vertical
normalization.
**Read when:** Working on selector `IU`/`Iu`, class `0x009f`, system staff lists, or Special Part
Extraction.
**Confidence:** `confirmed` for represented layouts and ordinary/system-scaled normalization;
`strong` for compact-list maximum normalization, Special Part Extraction selection,
synthesized-list behavior, and the Coda-era Staff Set remap.

## Identity and layouts

The comparator identifies a staff list. Comparator 0 is the ordinary Scroll View list; positive
comparators can identify authored system lists. Through Finale 3.2, each `IU` row packs two
six-byte entries holding `staffId`, an unused word, and a signed 16-bit `distFromTop`. Finale 3.5
through 98 stores one 12-byte `IU` entry with `staffId` at byte 0 and a signed 32-bit
`distFromTop` at byte 8 in the container's native word order. The row remains 12 bytes on both
sides, so the recovered saving version selects the layout. **Strong** across represented Finale
1.0--3.2 and 3.5--98 files.

A positive comparator below 32768 is imported only when the part has a corresponding `StaffSystem`
or `Document::calcScrollViewCmper` selects it for Special Part Extraction. Other lists in that
range are orphaned layout data. Comparator 0 and the reserved high comparator namespaces do not
require a StaffSystem.

Finale 2000--2006 selector `Iu` and zlib class `0x009f` use 24-byte entries. Their first 12 bytes
retain the later `IU` fields; the second half stores `range.startMeas`, `range.startEdu`,
`range.endMeas`, and `range.endEdu`. The 32-bit fields use the container's native word order.
Earlier layouts do not store a range, and recovery supplies the whole-document range Finale writes
when upgrading them.

## Completion and normalization

Through Finale 2000, lists are normalized by subtracting their maximum `distFromTop` from every
entry. The removed value is added to the system's `top`; a system with Resize Vertical Space enabled
receives the value at `ssysPercent`, rounded to the nearest Evpu, while other systems receive it
unscaled. Recovery preserves every stored distance in DCL and zlib sources and does not adjust the
stored system top. The all-corpus capture validates this epoch boundary across represented sources.

Through Finale 2010, StaffSystem flag `0x4000` selects the system-specific StaffUsed list. When the
flag is clear, the system is unoptimized and receives a copy of its part's Scroll View list even if
a stale same-comparator IU list remains in the file. Finale 2011 and later systems always use their
system-specific lists. The source list for an unoptimized system is comparator 0 unless
`PartGlobals::calcScrollViewCmper()` selects the nonzero Special Part Extraction list. Synthesized
lists use the system comparator, retain the template order, positions, and ranges, and are reported
as legacy behavior. Score lists use `ShareMode::All`, while linked-part lists use `ShareMode::None`.

When Special Part Extraction supplies the template, its maximum stored distance is also the
normalization value added to the synthesized system's top. Thus both the synthesized entries and
the system-top adjustment come from the extraction list; comparator 0 does not participate in that
system's normalization.

The Special Part Extraction selector is already recovered by
[`part_globals.md`](part_globals.md): pre-zlib selector 23 word 4, zlib class `0x0120` byte 6, and
musx-era XML node `<pageViewIUlist>`. `Document::calcScrollViewCmper` delegates to that per-part
value and falls back to `BASE_SYSTEM_ID` when PartGlobals is unavailable.

## Companion layout transformations

Finale may add a staff containing entries to an optimized system list when upgrading a document and
shift the later staff positions. Under active Special Part Extraction, it instead reduces each
optimized system list to the ordered intersection with the extraction list and renormalizes the
first retained staff to zero. Recovery deliberately preserves either authored source list.
Coverage recognizes each transformation only from its corresponding source-list structure and
StaffSystem identities.

## Remaining scope

Active Special Part Extraction is represented for score-owned PartGlobals. **Believed:** Finale
cannot enable Special Part Extraction independently for a linked part. Recovery does not enforce
that belief: a nonzero part-owned Special Part Extraction comparator is honored if encountered.
Such a field fixture would refute the belief and is not expected to be reproducible through
Finale's user interface.

Musxdom defines the modern eight-Staff-Set namespace from `STAFF_SET_1_SYSTEM_ID` (65500) through
that value plus 7. Finale 1.x--2.x instead had four Staff Sets at 65530 through 65533, immediately
above Special Part Extraction (65528) and the temporary system (65529). Finale 3.0 expanded the
feature to eight Staff Sets, for which the old base would exceed the unsigned 16-bit comparator
range, and moved the base to its modern value. Recovery remaps the four Coda-era comparators by
ordinal and preserves all other comparators.
