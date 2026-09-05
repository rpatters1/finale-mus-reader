# Custom-key others records

**Covers:** The nine others-pool classes that define custom-key formats, maps, attributes,
accidental arrays, and tonal-center arrays.
**Read when:** Working on `src/import/others/custom_keys.cpp` or custom-key interpretation.
**Confidence:** mixed; each claim is labeled below.

All nine classes are source-owned. The importer constructs an object only when its record family
is present; it does not copy a built-in or Finale 27 baseline instance. musxdom treats the custom
key's cmper as its identity and already supplies fallbacks when `KeyFormat` or `KeyMapArray` is
absent, so no member of this group is used as a prerequisite for constructing another one.

| musxdom class | Fixed tag | Zlib class | Logical payload |
|---|---|---:|---|
| `AcciAmountFlats` | `An` | `0x0072` | first 7 signed words |
| `AcciAmountSharps` | `Ap` | `0x0073` | first 7 signed words |
| `AcciOrderFlats` | `On` | `0x0074` | first 7 unsigned words |
| `AcciOrderSharps` | `Op` | `0x0075` | first 7 unsigned words |
| `KeyFormat` | `KF` | `0x00a0` | semitone count, diatonic-step count |
| `KeyMapArray` | `KM` | `0x00a1` | two words per chromatic step |
| `KeyAttributes` | `KA` | `0x00a2` | six-word tuple described below |
| `TonalCenterFlats` | `Tn` | `0x009a` | first 8 unsigned words |
| `TonalCenterSharps` | `Tp` | `0x009b` | first 8 unsigned words |

`KeyAttributes` stores `harmRefer`, `middleCKey`, `fontSym`, `gotoKey`, `symbolList`, and flags in
that order. Flag `0x0001` is `hasClefOctv`. `KeyFormat` stores `semitones` followed by
`scaleTones`; its remaining four fixed-row words are not members of the DOM class.

After all pools are imported, a false `hasClefOctv` is normalized to true when either clef-octave
detail family exists for the same part and key cmper. The adjustment retains the stored false in
the field report and changes its origin to `LegacyMusAdjusted`. A stored true is not cleared when
no matching detail is imported, because absence does not establish that the source flag is stale.
This one-way relationship is **weak**, observed consistently in three uncompressed sources:
`mus-eb3c8bf25589c9ff`, `mus-a6eb2a03862c390e`, and `mus-fd30faffac39c5b0`. See the
[investigation](../../investigations/custom_keys.md#2026-09-05--hasclefoctv-detail-presence).

The array records occupy two physical other rows, or twelve words in the observed class records.
Only the DOM class's first seven or eight values are semantic. Bytes beyond that capacity include
nonzero unrelated data in fixed-row specimens and must not be imported.

`KeyMapArray` pair order follows the document byte order rather than its epoch or record framing.
Big-endian records store the `0x8000` diatonic flag first and `hlevel` second; little-endian records
store `hlevel` first and the flag second. This distinction is **confirmed** for big-endian
Coda-banner, uncompressed, and DCL records and for little-endian Coda-banner, uncompressed, and
zlib records. Little-endian DCL and big-endian zlib custom maps remain unrepresented, so extending
the byte-order rule to those combinations is **strong** rather than confirmed. The map is trimmed
to `KeyFormat::semitones` after all importers run, which removes fixed-row padding without making
registry order significant. If the matching format is absent, every complete pair is retained.

The fixed identities and layouts other than key-map pair order and the one-way `hasClefOctv`
normalization are **weak**, independently
binary-verified in one Finale 2003 DCL source, `mus-b67aad6814dc4059`. The zlib identities and
layouts other than key-map pair order are **weak**, independently binary-verified in one
little-endian Finale 2010 source, `mus-4b3246869b9d07d7`. The
tracked-evidence comparison independently exercises `KeyFormat` and both tonal-center arrays in
Coda-banner, uncompressed, and DCL sources, and `KeyAttributes` in DCL sources; those exercised
leaves agree with their companions. Pair order was independently binary-verified in 209 distinct
documents across `rpatters1-main` and `rpatters1-installs`: 173 big-endian documents store the flag
first and 36 little-endian documents store `hlevel` first, with no contrary observation. The
remaining families and the zlib epoch still need broad semantic coverage.

Related details:
[`../details/custom_key_octaves.md`](../details/custom_key_octaves.md) and
[`../details/key_symbol_list.md`](../details/key_symbol_list.md).

Experiment history: [`../../investigations/custom_keys.md`](../../investigations/custom_keys.md).

Implementation: `src/import/others/custom_keys.cpp`.
