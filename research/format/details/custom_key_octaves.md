# Custom-key clef octave arrays

**Covers:** `ClefOctaveFlats` and `ClefOctaveSharps` identities and payloads.
**Read when:** Working on `src/import/details/custom_key_octaves.cpp` or custom-key accidental
placement.
**Confidence:** weak; the fixed sharp family and both zlib families are independently
binary-verified.

`ClefOctaveFlats` uses fixed tag `Cn` and zlib detail class `0x0408`.
`ClefOctaveSharps` uses fixed tag `Cp` and zlib detail class `0x0409`. Cmper1 is the custom-key
cmper and cmper2 is the clef id. Each payload contains seven signed octave values followed by
three physical padding words; only the first seven words belong to the DOM array.

The fixed `Cp` layout is exercised by Finale 2003 source `mus-b67aad6814dc4059`; fixed `Cn` rests
on the supplied independent reverse engineering. Both zlib layouts are exercised by
little-endian Finale 2010 source `mus-4b3246869b9d07d7`. No controlled Coda-banner or
uncompressed specimen containing either family is available.

Experiment history:
[`../../investigations/custom_key_octaves.md`](../../investigations/custom_key_octaves.md).

Implementation: `src/import/details/custom_key_octaves.cpp`.
