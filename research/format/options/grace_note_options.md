# GraceNoteOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Grace note options

**Implemented.** musxdom's `GraceNoteOptions` has six scalar fields. The public
PDK documents their semantics across
[`FCSizePrefs`](https://pdk.finalelua.com/class_f_c_size_prefs.html),
[`FCDistancePrefs`](https://pdk.finalelua.com/class_f_c_distance_prefs.html), and
[`FCMiscDocPrefs`](https://pdk.finalelua.com/class_f_c_misc_doc_prefs.html) (accessed
2026-08-29): the two size percentages, slash thickness, entry spacing, playback duration,
and always-slash switch. Those declarations are **public-PDK-derived** and expose no legacy
record locations. Authorized read-only Framework history supplied the initial locations
below; they remain **private-framework-derived** except where the binary and companion
evidence independently supports them.

| musxdom field | Preference property | Selector | Incidence | Word | Width |
|---|---|---:|---:|---:|---:|
| `tabGracePerc` | `graceTablatureNoteSize` | `14` | 0 | 2 | 2 |
| `gracePerc` | `graceNoteSize` | `23` | 0 | 0 | 2 |
| `playbackDuration` | `gracePlaybackDur` | `27` | 0 | 4 | 2 |
| `entryOffset` (`graceBackup` in XML) | `graceNoteSpacing` | `27` | 0 | 5 | 2 |
| `slashFlaggedGraceNotes` | `graceAlwaysSlash` | `44` | 0 | 4 | 2 |
| `graceSlashWidth` | `graceSlashThickness` | `64` | 0 | 1 | 2 |

The fixed-row mapping is **strong** for the uncompressed and DCL epochs. The tracked Finale
97 baseline carries `85, 70, 128, 30, true, 128` at exactly these locations, and its
independently parsed Finale 27 companion carries the same six semantic values. The Finale
2003 DCL baseline likewise agrees at `85, 50, 128, 24, true, 224`. The zlib-era layout is
also **strong**: the Finale 2012 baseline stores the same six values as Finale 2003 at the
corresponding `numericGlobalClass(selector)` records and byte offsets, in a little-endian
container, and its companion agrees. Synthetic tests exercise the class layout in both byte
orders. Controlled non-default edits would be needed to raise the six individual mappings
to confirmed.

The Coda-banner layout is different. In the original layout, `graceSlashWidth` is the general
Def Line Width stored as the first single-precision value of selector `54`. Finale 2.6.3 adds
the later semantic field at selector `64` word 1 in ten-thousandths of a point; selector `64`'s
presence selects that representation and makes it authoritative. Both forms convert points to
modern Efix. The importer therefore recovers `gracePerc` and `graceSlashWidth`; the other four
fields retain their pinned baseline values and are reported as `Finale27Default`.

The Coda `gracePerc` location is **confirmed** by the controlled
`F100-grace-pct` discriminator. Changing only Grace Note Size from 50 to 49 changes only
selector `23` word 0 in the decoded MUS records; its ETF repeats 49, and the Finale 27
companion changes only `gracePerc` from 50 to 49. The companion's `graceSlashWidth`
remains 224. No adjacent selector-23 word moves, so this edit supplies no candidate for the
separately stored line width; in particular, word 2 remains 512, the same value a Finale 2.6.3
baseline carries beside a companion width of 64.

The controlled migration chain confirms both representations. `F100-deflne-625` moves the
selector-`54` float from 0.5 to 0.625, which recovers as Efix 160. A second Finale 1.0.0 save
stores 3.14159 there. Finale 2.6.3 retains that float and writes 31415 at selector `64` word 1;
Finale 3.7.2 converts it to Efix 804, and both the Finale 2.6.3 and 3.7.2 modern companions
contain `graceSlashWidth = 804`. The direct Finale 1.0.0 companions instead normalize the
width to 224. That direct-upgrade conversion loss is classified only for a Coda source whose
source-owned field came from the original layout, which structurally means selector `64` was
absent. Later Coda documents use the authoritative selector-`64` value and agree with their
companions. **Confirmed.**

The 173-document tracked capture imports every source and companion. GraceNoteOptions has 1,000
equal leaves and 38 expected Finale-upgrade-loss leaves, all `graceSlashWidth` disagreements from
original-layout Coda sources. There are no unexpected GraceNoteOptions differences.
