# Marking categories

**Covers:** The `MarkingCategory` and `MarkingCategoryName` class records, their layouts,
alignment numbering, flag word, text encoding, and pre-Finale-2009 fallback.
**Read when:** Working on marking categories, category names, or expression-category defaults.
**Confidence:** `strong` for the record layout, names, fallback classification, and source-owned
font upgrade classification.

## Record families and boundary

**Strong.** Finale 2009 introduces both class records. `MarkingCategory` is class `0x012d` and
`MarkingCategoryName` is class `0x012e`; both use the category cmper. No source record from before
Finale 2009 has been observed. The proposed fixed-row identities `MC` and `mn` are therefore not
used: the fixed-row epochs predate the feature, and no independent record observation supports
those identities.

For Finale 2009 and later, categories are source-owned. An absent category stays absent; the
reader does not fill gaps from the Finale 27 baseline. For earlier sources only, cmpers 1 through
7 are copied from the selected baseline document as one category/name operation. The seventh is
the Miscellaneous category; Finale's public category API calls only cmpers 1 through 6 predefined
category IDs, but also identifies Miscellaneous as a standard Finale-created category.

## Category payload

**Strong.** The payload is 36 bytes in both Finale 2009 and Finale 2012. All values are signed or
unsigned 16-bit words in the container byte order as appropriate.

| Byte | Size | musxdom member |
|---:|---:|---|
| 0 | 2 | `categoryType` |
| 2 | 6 | `textFont`: id, signed size, Enigma effects |
| 8 | 6 | `musicFont`: id, signed size, Enigma effects |
| 14 | 6 | `numberFont`: id, signed size, Enigma effects |
| 20 | 2 | `justification` |
| 22 | 2 | `horzAlign` |
| 24 | 2 | signed `horzOffset` |
| 26 | 2 | `vertAlign` |
| 28 | 2 | signed `vertOffsetEntry` |
| 30 | 2 | signed `vertOffsetBaseline` |
| 32 | 2 | category flags |
| 34 | 2 | `staffList` |

The category type is the predefined category ID on which the category is based. The public API
defines Dynamics through Rehearsal Marks as values 1 through 6; musxdom additionally represents
Miscellaneous as 7.

## Alignment numbering

**Strong; public-PDK-derived and independently binary-verified for values stored by the surveyed
canned categories.** Public category-definition documentation supplies the complete stored
numbering ([public API reference](https://pdk.finalelua.com/class_f_c_category_def.html), accessed
2026-09-04). These values require translation because musxdom enums use semantic declaration
order.

| Stored horizontal | musxdom value |
|---:|---|
| 0 | `LeftBarline` |
| 1 | `StartTimeSig` |
| 2 | `AfterClefKeyTime` |
| 3 | `Manual` |
| 4 | `CenterOverBarlines` |
| 5 | `CenterOverMusic` |
| 6 | `RightBarline` |
| 7 | `StartOfMusic` |
| 9 | `LeftOfAllNoteheads` |
| 10 | `Stem` |
| 11 | `CenterPrimaryNotehead` |
| 12 | `CenterAllNoteheads` |
| 13 | `LeftOfPrimaryNotehead` |
| 14 | `RightOfAllNoteheads` |

Vertical values are `0 AboveStaff`, `1 BelowStaff`, `2 Manual`, `3 RefLine`, `4 TopNote`,
`5 BottomNote`, `6 AboveEntry`, `7 BelowEntry`, `8 AboveStaffOrEntry`, and
`9 BelowStaffOrEntry`. Justification is stored as `0 Left`, `1 Center`, `2 Right`.

## Flag word

**Strong.** Source values and companion fields agree on these masks. The correspondence for
`userCreated` is also exercised by custom categories outside the canned range.

| Mask | musxdom member |
|---:|---|
| `0x0001` | `usesTextFont` |
| `0x0002` | `usesMusicFont` |
| `0x0004` | `usesNumberFont` |
| `0x0008` | `usesPositioning` |
| `0x0010` | `usesStaffList` |
| `0x0040` | `usesBreakMmRests` |
| `0x0080` | `userCreated` |
| `0x0400` | `breakMmRest` |

## Category-name payload

**Strong.** Through Finale 2011 the effective payload is an 8-bit platform string. Finale 2012
stores UTF-16 code units in the container byte order. The decoder therefore uses the same
Finale-2012 Unicode boundary as other legacy text and does not infer width from payload length.

## Comparison classification

Differences between pre-Finale-2009 baseline font leaves and companion values are classified as
different defaults only when their origin is `finale27-default`. Source-owned Finale 2009 beta
differences are beta discrepancies. A positive source font ID that has no source-owned font
definition, but resolves to a concrete font in the companion, is Finale upgrade loss. Other
category differences remain visible.

`MarkingCategory` and `MarkingCategoryName` have no known remaining recovery or comparison scope.
