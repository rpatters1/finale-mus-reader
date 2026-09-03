# NoteRestOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Note/rest options

**Implemented.** `NoteRestOptions` has nine scalar leaves and a twelve-element RGB collection.
The public Framework documentation places Use Shape Notes, Display Cross-Staff Notes in
Original Staff, and Scale Manual Note Positioning in `FCMiscDocPrefs`; this establishes their
semantics but no disk locations. The consulted immutable documentation is
[`class_f_c_misc_doc_prefs.html`](https://github.com/finale-lua/pdk-framework-docs/blob/c3c5ebf0335432812b286e79c9757ad023eb48f1/html/class_f_c_misc_doc_prefs.html)
(accessed 2026-08-31). `FCMusicCharacterPrefs` identifies the five vertical-rest values as
EVPU adjustments in eighth-through-128th order; the consulted immutable documentation is
[`class_f_c_music_character_prefs.html`](https://github.com/finale-lua/pdk-framework-docs/blob/c3c5ebf0335432812b286e79c9757ad023eb48f1/html/class_f_c_music_character_prefs.html)
(accessed 2026-09-01). These descriptions are **public-PDK-derived**. Authorized read-only
Framework history supplied the following initial locations, which remain
**private-framework-derived** except where the source and companion evidence below corroborates
them:

| musxdom field | Preference property | Selector | Incidence | Word | Width |
|---|---|---:|---:|---:|---:|
| `doShapeNotes` | `useNoteShapes` | `1` | 0 | 1 | 2 |
| `doCrossStaffNotes` | `crossStaffInOriginal` | `12` | 0 | 4 | 2 |
| `scaleManualPositioning` | `scaleManualPos` | `41` | 0 | 5 | 2 |

The selector-1 location applies to DCL and later storage, not to the whole fixed-row family.
The DCL importer retains it as **private-framework-derived** preliminary coverage; all 53
distinct DCL sources in the current tracked-evidence scan store zero there and have false semantic
companions, so none provides an independent positive test. The zlib importer derives classes
`0x000f`, `0x001a`, and `0x0037` from the three selectors.

**Confirmed in Finale 3.7.2:** the earlier uncompressed Use Shape Notes switch is `CS`
comparator 1, word 5, bit 7. Enabling it in `F372-noteopts` changes that word from `0` to
`128`, while selector `01` remains six zero words. The edit also adds eight `HI` detail rows
that define the individual note shapes; those rows are data enabled by the switch rather than
an alternative location for the switch. The ETF repeats the `CS` change and the Finale 27
companion writes `doShapeNotes`. When Finale 2012 upgrades the same document, class `0x000f`
word 1 contains `1`, independently confirming the later class location and showing that the
upgrade translates the older representation. The controlled inverse `F372-noteopts-noshapes`
edit clears only that `CS` bit among the note-shape records: selector `01` remains zero and all
eight `HI` definitions remain byte-identical. Two unrelated selector values undergo a one-unit
save normalization. The false Finale 27 companion confirms the inverse semantic value, making
the switch location **confirmed** independently of the unavailable original baseline. A
deduplicated scan found the `CS` bit and the semantic companion both false in the other 40
tracked uncompressed sources; the controlled fixture is the sole positive pair.

The stored cross-staff word maps directly to musxdom's `doCrossStaffNotes` member: despite the
Framework property's UI-facing “in original staff” name, every tracked source agrees with its
independently parsed companion under the direct interpretation. A controlled inverse edit is
still needed to distinguish the stored flag's precise UI polarity independently of Finale's
upgrade.

**Confirmed in Finale 2.6.3 and the uncompressed era:** all five Rest Positioning fields are
stored as signed EVPU values. Selector `44` words 0--2 hold the eighth-through-32nd values, and
selector `41` words 3--4 hold the 64th and 128th values. All 22 distinct Finale 2.6 tracked sources match their
semantic companions exactly across the five-word tuple despite predating the UI that exposes
the first three values. The controlled baseline stores `12, -12, -12, -24, -48`. Finale 2.6.3
presents the last two as toggles, but stores their realized EVPU values: disabling only the
64th-rest drop changes word 3 to zero, and disabling only the 128th-rest drop changes word 4 to
zero. Each is the sole decoded change in its controlled source and is repeated by the ETF. The
Finale 27 companions preserve the corresponding values, so no boolean-to-EVPU conversion is
required.

The DCL importer uses the same locations as **private-framework-derived** preliminary coverage,
but this is not independently confirmed: all 53 tracked DCL sources have raw tuples that differ
from their Finale 27 companions, usually raw zeroes upgraded to `0, 0, 0, -24, -48`. That may be
upgrade-time default synthesis, but a controlled DCL edit is needed to distinguish it from a
physical relocation. The Finale 2012 staged fixture independently confirms the later class-record
locations.

The controlled Finale 3.7.2
`F372-noteopts` edit changes the two source rows from `0, 0, 0` and `-24, -48` to
`3, -5, 7` and `-23, -47`; its ETF repeats the changes and its Finale 27 companion carries all
five semantic values. Opening that document in Finale 2012 preserves the values at the same word
positions in classes `0x003a` and `0x0037`, confirming the class-record replacements as well.
Synthetic tests cover fixed rows in the Coda, uncompressed, and DCL epochs and class records in
both byte orders.

**Confirmed earliest fixed behavior:** Finale 1.0.0 has no selector `44`; selector `41` is
present but all six words are zero. Its effective rest-drop tuple is `0, 0, 0, -24, -48`, the
same tuple supplied by both pinned Finale 27 baselines, so the importer leaves all five values
at `Finale27Default` rather than duplicating them as `LegacyBehavior`. Thirty-nine of 40
distinct tracked Finale 1.0.0 companions carry that tuple. The remaining companion, for the
controlled default-music-font edit, writes zero for the last two values; this is treated as
upgrade-time font-default substitution rather than evidence that the zero source words store
the behavior. Selector `44` presence therefore distinguishes a stored tuple from the earliest
fixed behavior without relying on a recoverable version; its absence requires no value overlay
because the pinned baseline already agrees.

Selector `75`, the straight-flag coordinate row, instead marks the independently editable
music-character layout. A deduplicated census of two already-inventoried corpora found it absent from all 76
intact Finale 3.0/3.2 sources and present in all 312 intact Finale 3.5--97 sources. Ten additional
sources omit selectors `41`, `44`, and `75` together and therefore do not test the boundary. The
stored rest-position tuple predates that marker and is recovered independently of it. The printed
Finale 3.5 addendum confirms that release introduced the editable rest-position values. That UI
boundary does not gate `NoteRestOptions` recovery because earlier documents already store their
realized values.

This separation is consistent with Finale's published development history, but that history is
context rather than record evidence. Steve Peha recalled that the original Adobe Sonata-based
implementation needed many hidden parameters and offsets for alignment, and that symbol
placement was tuned for months before Finale 1.0 shipped. See [“Meet Steve Peha, creator of
Petrucci, Finale's first music font”](https://www.finalemusic.com/blog/meet-steve-peha-creator-of-petrucci-finales-first-music-font/)
(published 2010-02-18; accessed 2026-09-01).

The Coda layout is narrower. Selector `12` word 4 continues to agree with every companion, but
selector `1` word 1 cannot be treated as Use Shape Notes: one tracked Finale 1.0.0 source stores
one while its raw companion omits `doShapeNotes`. With no applicable Coda source mapping, that
field retains its seeded value as `Finale27Default`. Selector `41` word 5 is also not the later
scale-manual flag there: all 64 tracked Coda
occurrences store zero while their raw companions explicitly store `<scaleManualPositioning/>`.
**Confirmed:** scaling manual note positioning is fixed true behavior before Finale 3.0, when the
preference was introduced. The importer therefore supplies true as `LegacyBehavior` throughout
the Coda epoch and does not read selector `41` word 5. Finale 3.0 and every subsequent legacy
version read the stored option instead; its ordinary value is off. The 64 tracked Coda occurrences,
representing 62 distinct source ids and both Coda product generations, agree with that boundary.

**Confirmed in Finale 2008:** numeric-global class `0x0071` (selector `99`) is a 42-word
notehead-color record. Word 0 stores `drawOutline`; words 1--12 are the twelve red channels,
words 13--24 the green channels, and words 25--36 the blue channels. Words 37--41 are zero in
both controlled sources. The existing `F2008-empty` source stores outline true and Finale's
standard color set. The controlled `F2008-notehead-colors` source stores outline false and 36
distinct edited channel values. Every value in both source records agrees exactly with its
Finale 27 companion, establishing the planar order, unsigned 16-bit channels, and leading
boolean independently of defaults. Finale 2007's controlled source has no class `0x0071`, in
agreement with the **private-reference-derived** Finale year-to-year product summary identifying
notehead colors as a Finale 2008 feature (SHA-256
`7d25ac230626f98d63f5b91bb5034232a55c876886fafc6a16ecb24e95b55b0c`, accessed
2026-09-01). The importer therefore uses record presence and the exact 42-word shape rather than
a version gate. Earlier sources and a malformed or absent color record retain the pinned
baseline and report `Finale27Default`.

The 2026-09-01 tracked-evidence capture contains 191 successful source occurrences and 191
successful companions, representing 189 distinct `corpus_id` values: 66 Coda, 42 uncompressed,
53 DCL, and 30 zlib. Of 8,595 observed `NoteRestOptions` leaves, 8,593 compare equal. The two
remaining leaves are the controlled Finale 1.0.0 default-music-font edit's 64th- and 128th-rest
positions: the pinned defaults remain `-24, -48`, while Finale 27's companion omits both elements
and therefore reads as zero. Both source files have a zero selector `41`, omit selector `44`, and
differ in the numeric-global row selecting the default music font rather than in a stored
rest-position value. These two leaves are classified as `different_defaults`. The rule covers any
of the five rest-position leaves only with `Finale27Default` origin, so a disagreement recovered
from source bytes remains unexpected. No selected document failed before comparison, and no
`NoteRestOptions` leaf remains unexpected.
