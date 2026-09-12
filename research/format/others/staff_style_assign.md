# StaffStyleAssign

**Covers:** Legacy Staff Style assignment identity, addressing, range layout, and derived Staff
state.
**Read when:** Working on `StaffStyleAssign`, selector `Sy`, class `0x00e9`, Staff `hasStyles`, or
pre-Finale-2012 Staff Style splitting.
**Confidence:** `confirmed` for the represented Finale 2000, 2003, and 2011 layouts; `open` for
whether any source before Finale 2000 can contain this record family.

## Identity and layout

The fixed-row selector is `Sy`; the zlib class is `0x00e9`. The comparator is the Staff ID, and
each logical assignment occupies 24 bytes, represented as two consecutive 12-byte Others rows in
fixed-row files. Recovery constructs a source-owned `musx::dom::others::StaffStyleAssign` and uses
its zero-based assignment position as the incidence. Multiple 24-byte tuples under one Staff
comparator become consecutive assignments. **Confirmed.** Evidence and method:
[`../../investigations/staff_style_assign.md`](../../investigations/staff_style_assign.md).

| Byte offset | Size | musxdom member |
|---:|---:|---|
| 0 | 2 | `styleId` |
| 2–11 | 10 | unused |
| 12 | 2 | `startMeas` |
| 14 | 4 | `startEdu` |
| 18 | 2 | `endMeas` |
| 20 | 4 | `endEdu` |

The 16-bit values follow the file's byte order. Each 32-bit EDU uses native word order as well:
high word first in big-endian files and low word first in little-endian files. The record family
is selected structurally, with no saving-version gate. An incomplete trailing tuple is diagnosed
and not constructed. **Confirmed** for both byte orders and all three represented format epochs.

After assignment import, `Staff::hasStyles` is recalculated from effective assignment presence for
every source Staff and reported as `LegacyMusAdjusted`. Thus defining a Staff Style alone does not
set the property. Pre-Finale-2000 alternate-notation records are not synthesized in this cycle, so
their Staffs still report false until that later recovery pass. **Confirmed** for represented
source assignments; the earlier synthesis is open work.

## Upgrade behavior and remaining scope

When Finale upgrades a pre-Finale-2012 Staff Style, one source assignment can become multiple
assignments over the same Staff and measure range. A source may also contain several assignments
with exactly the same range. Coverage therefore matches by source part, Staff, and complete range,
not by incidence or Staff Style ID. Within one exact range, it applies the assigned styles in
incidence order and compares the resulting active-mask override patch; later values under the same
mask replace earlier values, while disjoint masks accumulate. For fields overridden on either
side, the comparison completes the other side from that document's base Staff. The masks remain
classifier evidence rather than compared values. Raw style names and IDs are not part of the
comparison; note-font IDs and Staff-name text IDs compare through their semantic referents. A
one-to-many upgrade split needs no difference classification when its effective values are
unchanged. Established pre-Finale-2009 attached-item losses remain classified at the assignment
level, and residual instrument-field changes from a pre-Finale-2012 one-to-many split are upgrade
normalization.
**Confirmed** that multiple assignments may occupy one exact range and that the controlled Finale
2011 mixed-style fixture gains two same-range assignments on upgrade. Semantic comparison is a
coverage policy rather than a claim about the binary layout.

This comparison does not infer an instrument from a generated UUID or divide partially overlapping
ranges into effective time segments. Unmatched complete ranges and any remaining effective-value
differences stay visible for investigation.

No represented source before Finale 2000 contains `Sy`, but its Finale 27 companion may contain
Staff Styles and assignments synthesized from older alternate-notation records. Those objects
remain companion-only deliberately. The later synthesis cycle is expected to remove that entire
companion-only population. The decoder has no pre-Finale-2000 version exclusion: if an earlier
`Sy` family is encountered, it remains visible for investigation rather than being discarded.
