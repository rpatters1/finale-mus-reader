# Chord suffix playback

**Covers:** Legacy recovery of `musx::dom::others::ChordSuffixPlayback`.
**Read when:** Changing `src/import/others/chord_suffix_playback.cpp` or interpreting `IK`/class `0x007e`.
**Confidence:** confirmed across all four physical epochs.

`ChordSuffixPlayback` is a source-owned signed 16-bit interval array. Its cmper matches the related
`ChordSuffixElement` suffix definition. Through Finale 2006 it uses null-terminated-array selector
`IK`; zlib files use class `0x007e`. The evidence and public-PDK provenance are in
[the investigation](../../investigations/chord_suffix_playback.md).

The payload supplies as many signed interval values as it contains. Every stored word is retained,
including internal and trailing zeroes; an absent record alone creates no object. Finale 27 preserves
fixed-row trailing zeroes in its modern playback array, so they remain part of the source-faithful DOM
even though they have no audible effect.

The controlled Finale 1.0 fixture confirms the same `IK` selector, signed-word layout, and retained
trailing zeroes in the Coda-banner epoch.
