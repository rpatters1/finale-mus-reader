# TimeSignatureOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Time signature options

**Partially implemented.** The public
[`FCDistancePrefs`](https://pdk.finalelua.com/class_f_c_distance_prefs.html) documentation identifies
the score and linked-parts top, bottom, and abbreviated-symbol vertical positions and the spaces
before and after a time signature as EVPU values. The public
[`FCMiscDocPrefs`](https://pdk.finalelua.com/class_f_c_misc_doc_prefs.html) documentation identifies
the common- and cut-time abbreviation switches, composite-meter decimal count, and courtesy time
signature at a system end. `FCSizePrefs` exposes no corresponding time-signature member. These
semantics are **public-PDK-derived** from the pages and linked public headers, accessed 2026-09-02;
the public declarations do not expose the legacy record locations.

An authorized read-only private interoperability source supplied the initial locations below.
They are **private-framework-derived** leads. Fixed-row epochs address selector, incidence, and
word; zlib coalesces the six-word incidence rows into a class-record word stream reached through
`numericGlobalClass(selector)`.

| musxdom field | Selector | Incidence | Word | Stored form |
|---|---:|---:|---:|---|
| `timeUpperLift` | 18 | 0 | 5 | signed word |
| `timeFront`, `timeBack` | 18 | 0 | 3, 4 | signed words |
| `timeUpperLiftParts`, `timeLowerLiftParts`, `timeAbrvLiftParts` | 18 | 1 | 0, 1, 2 | signed words |
| `timeFrontParts`, `timeBackParts` | 18 | 1 | 3, 4 | signed words |
| `timeSigDoAbrvCommon`, `timeSigDoAbrvCut` | 19 | 0 | 2, 3 | Boolean words |
| `numCompositeDecimalPlaces` | 23 | 0 | 1 | word |
| `cautionaryTimeChanges` | 44 | 0 | 3 bit 1 | packed bit |
| `timeLowerLift`, `timeAbrvLift` | 67 | 0 | 0, 1 | signed words |

The controlled Finale 1 score-spacing edit independently confirms selector 18 incidence 0 words
3--5: 24, 12, 0 become 23, 11, 1 in both MUS and ETF, and Finale 27 preserves the three values.
The Finale 2008 class `0x0020` independently confirms the coalesced shape: score values occupy
words 3--5, linked-parts verticals words 6--8, and linked-parts spaces words 9--10. All five parts
fields in that fixture agree with its companion. Confidence is **confirmed** for the controlled
Finale 1 fields and **strong** for the remaining mapped fields pending single-control
discriminators.

The Coda layout has no supported source for lower/abbreviated score lift or the packed
courtesy-time switch; those three members retain the pinned baseline. A fixed-row selector-18
family shorter than the eleven words needed for independent parts distances structurally identifies
the earlier shared layout. In that layout the five parts members copy their corresponding score
values as `LegacyBehavior`; a complete second incidence supplies independent parts values instead.
The zlib epoch never interprets a short class payload as the earlier layout. The refreshed
217-occurrence tracked-evidence capture imported every source and
companion. All 3,038 `TimeSignatureOptions` leaf comparisons agree, including the controlled
Finale 1 edit whose score values 23, 11, and 1 propagate to the parts front, back, and top fields.
No expected-difference classification is needed.

The all-corpus capture selects 16,314 occurrences across all three registered surveys: 16,225
import successfully, 4,625 have successful companions, and 89 fail before comparison. Its
`TimeSignatureOptions` comparison is likewise exact: all 64,750 leaves agree, comprising 2,422
Coda, 4,774 uncompressed, 28,742 DCL, and 28,812 zlib leaves. The 89 source failures are 58 Finale
LIB files and 31 inputs that do not classify as Finale MUS documents; none enters the companion
comparison.
