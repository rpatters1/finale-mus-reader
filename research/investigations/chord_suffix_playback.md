# Chord suffix playback investigation

**Covers:** Evidence behind the `ChordSuffixPlayback` identities and trailing-fill handling.
**Read when:** Revising playback-array recovery or interpreting zero values.
**Confidence:** public-PDK-derived plus targeted independent binary verification.

## 2026-09-04 — Array identity and termination

The public Finale 2000 PDK `EDATA.H` defines `IK` as a null-terminated array of 16-bit chord-playback
values whose cmper matches the suffix definition. The immutable source was accessed 2026-09-04 at
[`edata.h`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/edata.h).
The maintained API documentation describes the collection as chord-suffix key offsets in
[`FCChordSuffixKeyNumberOffsets`](https://pdk.finalelua.com/class_f_c_chord_suffix_key_number_offsets.html).

**Confirmed through Finale 2005.** `mus-9ce47f4111aac6e7` and `mus-d0856f8164ebd47e` contain signed
`IK` words followed by zero fill. **Strong for zlib.** `mus-8fc93fa852c6c878` stores the corresponding
arrays in class `0x007e`; its identity also has perfect companion-count correlation in both registered
private surveys.

**Confirmed for Coda-banner.** `mus-800f4949a5b6a7ee` stores `IK(1)` as `3, 7, 10, 0, 0, 0`;
its ETF repeats those six words and its Finale 27 companion retains a six-value playback array.

The final 2026-09-04 `tracked-evidence` recovery-coverage snapshot compared 224 sources with
companions. Retaining every stored word produced 5,005 exact interval matches with no expected,
unexpected, reader-only, or companion-only leaves.
