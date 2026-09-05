# CustomKeyOctaves investigations

**Covers:** Experiments behind the custom-key clef-octave detail findings.
**Read when:** Investigating clef-specific custom-key accidental placement.
**Confidence:** weak; two epoch specimens, one source per epoch.

## 2026-09-04 — Clef octave identities and capacity

- Finale 2003 source `mus-b67aad6814dc4059` contains fixed `Cp` arrays. Concatenating the two
  five-word incidences gives seven octave values followed by three words that can be nonzero.
- Little-endian Finale 2010 source `mus-4b3246869b9d07d7` contains detail classes `0x0408` and
  `0x0409` for 18 clefs. Every payload is 20 bytes and its first seven signed words correspond to
  the companion's `ClefOctaveFlats` or `ClefOctaveSharps` array.
- **Conclusion:** Both zlib identities, fixed `Cp`, and the seven-value capacity are **weak**
  pending a controlled or independent-survey fixture. Fixed `Cn` currently rests on the supplied
  independent reverse engineering. Coda-banner and uncompressed storage remain unrepresented.
