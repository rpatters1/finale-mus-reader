# KeySignatureOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Key signature options

**Implemented.** musxdom's `KeySignatureOptions` has twelve scalar fields. The public PDK
[`FCDistancePrefs`](https://pdk.finalelua.com/class_f_c_distance_prefs.html) documentation
(accessed 2026-08-29) identifies the five distances as space before the key signature,
space after a canceled key, space after the key signature, space between its accidentals,
and extra space between key and time signatures, all in EVPUs. These semantics are
**public-PDK-derived**; the public declarations do not expose legacy record locations.
Authorized read-only Framework history supplied the initial locations below. They remain
**private-framework-derived** except where the controlled evidence independently confirms
them.

| musxdom field | Selector | Incidence | Word/bit | Coda | Uncompressed | DCL/zlib |
|---|---:|---:|---:|---|---|---|
| `doKeyCancel` | `12` | 0 | word 1 | recovered | recovered | recovered |
| `doCStart` | `12` | 0 | word 2 | recovered | recovered | recovered |
| `redisplayOnModeChange` | `12` | 0 | word 3 | recovered | recovered | recovered |
| `keyFront` | `18` | 0 | word 0 | recovered | recovered | recovered |
| `keyMid` | `18` | 0 | word 1 | recovered | recovered | recovered |
| `keyBack` | `18` | 0 | word 2 | recovered | recovered | recovered |
| `acciAdd` | `21` | 0 | word 4 | recovered | recovered | recovered |
| `showKeyFirstSystemOnly` | `27` | 0 | word 2 | unlocated | recovered | recovered |
| `keyTimeSepar` | `39` | 0 | word 5 | unlocated | recovered | recovered |
| `simplifyKeyHoldOctave` | `41` | 2 | word 1 | behavior false | behavior false | recovered |
| `cautionaryKeyChanges` | `44` | 0 | word 3 bit 0 | behavior true | recovered | recovered |
| `doKeyCancelBetweenSharpsFlats` | — | — | — | behavior true | behavior true | behavior true |

The Coda selector-12 layout uses the same three key-behavior words as the later fixed rows.
The controlled Finale 2.6.3 `nokeycxl` edit changes word 1 from 1 to 0 and the companion
clears `doKeyCancel`; the `key-restrikeC` edit changes word 2 from 0 to 1 and the companion
adds `doCStart`. Word 3 is 1 in the baseline and its companion carries `doBankDiff`; the
`key-trackbank` save records the same enabled UI state but is byte-identical to the baseline,
so that third mapping remains **strong** rather than a before/after confirmation. The older
`F263-courtesy-key-off` source is byte-identical to `nokeycxl` and had been misattributed.

The controlled Finale 1.0.0 key-distance fixture confirms selector `18` words 0..2 as 23, -7,
and 11 through both ETF and companion. A second controlled fixture changes the time-signature
distances in selector `18` words 3..5 and the clef distances in selector `19` words 0..1.
Selector `39` word 2 remains 100 in both files, so it is not evidence for any of those edited
distances and the previously considered Coda `keyTimeSepar` mapping is rejected. The Coda
locations of `keyTimeSepar` and `showKeyFirstSystemOnly` remain **open**; both fields retain the
pinned baseline.

The Coda UI has no separately stored modern courtesy-key field. Its source behavior is to
show the courtesy key, reported as `LegacyBehavior`; from Finale 3.0 onward the three courtesy
switches are packed into selector `44` word 3, with the key signature at bit 0. The controlled
Finale 2005 edit changes that word from 7 to 6, so the later mapping is **confirmed**.

The common fixed-row locations apply to the uncompressed and DCL epochs. The preserve-octave
field is not stored before DCL and the earlier behavior is false; DCL recovers selector `41`
incidence 2 word 1. Zlib retains the same
logical word streams under `numericGlobalClass(selector)`, with incidences coalesced into one
payload; the importer therefore derives the class identities and byte offsets from the same
selector and word numbers. Synthetic tests cover the uncompressed, DCL, and Coda fixed-row
forms and both zlib byte orders. Controlled Finale 1.0.0, 2.6.3, and 2005 edits exercise the
early distances, early behavior words, and later courtesy layout through the complete reader.

`doKeyCancelBetweenSharpsFlats` first appears in the authorized Framework mapping at Finale
25.4, after Finale 2012 and the end of the MUS format. The same Framework interface reports
the earlier behavior as always true, which the user accepted as `LegacyBehavior`; this matters
because the pinned baseline is false. Finale exposes no UI for the hidden setting through 2012,
so source/companion disagreements cannot be interpreted as an editable MUS value. Recovery
coverage classifies the observed `LegacyBehavior` true versus companion false transformation
as `different_defaults` in every epoch. No epoch is uncovered: Coda intentionally recovers its
smaller field set and early behavior, uncompressed uses fixed rows plus the pre-DCL behavior,
DCL uses the complete fixed rows, and zlib uses the corresponding class records.
