# CustomKeys investigations

**Covers:** Experiments behind the custom-key others-record findings.
**Read when:** Investigating custom-key identities, payload widths, or `KeyMapArray` ordering.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-09-04 — Custom-key others identities and layouts

- **Supplied hypothesis:** The nine fixed tags and their field layouts were supplied from an
  independent reverse engineering effort, with the prediction that they were unchanged from
  Finale 1.0 through Finale 2012.
- **Fixed records:** Direct word dumps of Finale 2003 source `mus-b67aad6814dc4059` locate the
  supplied tags and fields. Twelve-word array records contain semantic values only through the
  musxdom capacity; later words can be nonzero. `KeyMapArray` stores each pair as flag then
  harmonic level.
- **Zlib records:** Direct byte dumps of little-endian Finale 2010 source
  `mus-4b3246869b9d07d7` identify classes `0x0072`-`0x0075`, `0x009a`, `0x009b`, and
  `0x00a0`-`0x00a2`. Its `KeyMapArray` payload is 384 bytes for 96 steps.
- **Refuted prediction:** The pair order is not a fixed-record versus class-record distinction.
  Direct record analysis found that it follows byte order across both framing styles: big-endian
  records store flag then harmonic level, while little-endian records store harmonic level then
  flag.
- **Tracked comparison:** The tracked-evidence Release capture read 225 sources and 225 companions
  without error: 77 Coda-banner, 49 uncompressed, 65 DCL, and 34 zlib. It exercised 56 equal
  `KeyAttributes` leaves, 279 equal `KeyFormat` leaves, and 837 equal leaves for each tonal-center
  array. The four accidental arrays and `KeyMapArray` were unrepresented.
- **Conclusion:** **Weak** for the complete fixed and zlib layouts because each is represented by
  one full custom-key specimen. The tracked subset is independently reproducible, but it does not
  raise confidence for unrepresented classes or epochs.

## 2026-09-05 — `KeyMapArray` byte-order distinction

- **Trigger:** The all-corpus comparison showed 224 unexpected `KeyMapArray` leaves. Affected
  sources decoded diatonic steps as `false` and harmonic levels as `32768`, the signature of
  reading each two-word tuple in reverse.
- **Method:** The normalized `KM` and `0x00a1` word streams were classified independently by which
  alternating slot contained only flag values and which contained harmonic levels. The scan
  covered every custom-map occurrence in `rpatters1-main` and `rpatters1-installs` without
  changing either corpus.
- **Result:** **Confirmed** in the represented layout combinations. All 173 distinct big-endian
  documents stored flag then harmonic level; all 36 distinct little-endian documents stored
  harmonic level then flag. Both surveys contained both byte orders, and no contrary record was
  found. Big-endian examples span Coda-banner, uncompressed, and DCL framing; little-endian
  examples span Coda-banner, uncompressed, and zlib framing.
- **Unrepresented combinations:** Little-endian DCL and big-endian zlib custom maps were not
  present. Treating the distinction as byte-order-wide in those two cells is **strong**, supported
  by the cross-framing pattern but not directly observed.
- **Implementation consequence:** Pair selection now uses the source byte order, independently of
  whether the record came from a fixed or class-identified pool. Focused synthetic tests exercise
  both byte orders in every supported epoch.

## 2026-09-05 — refreshed all-corpus comparison

- **Scope:** After regenerating `rpatters1-main` and `rpatters1-installs`, the Release capture read
  16,322 sources and compared 4,824 companions without a companion error. A record-level presence
  scan found custom-mode cmpers (`2..127` or `0x4000..0x4fff`) in 209 documents.
- **Key-map result:** All 9,908 compared `KeyMapArray` leaves agree. This removes the previous 224
  little-endian mismatches and supports the byte-order correction.
- **Other numeric classes:** Every compared `KeyFormat`, accidental-array, tonal-center-array, and
  clef-octave value agrees. The three `KeyAttributes::hasClefOctv` differences are analyzed below.
- **Symbol-list result:** 1,162 `KeySymbolListElement` leaves differ in 29 documents. Finale 3.0,
  98, and 2012 examples consistently decode byte `0x8b` as `Ü` while companions retain U+008B.
  Finale 1.0 and `PC 1.0+` examples additionally disagree in `cmper2`, so their detail identity or
  row layout is not yet correct. These are unresolved reader findings, not classified upgrade
  behavior.

## 2026-09-05 — `hasClefOctv` detail presence

- **Trigger:** `mus-eb3c8bf25589c9ff`, `mus-a6eb2a03862c390e`, and
  `mus-fd30faffac39c5b0`, respectively saved by Finale 3.7, 97, and 98, store a zero
  `KeyAttributes` flags word for key cmper 2 while their Finale 27 companions set `hasClefOctv`.
- **Positive control:** Each source contains both `Cn` and `Cp` detail families for key cmper 2,
  across clef ids 0 through 15. The companion retains the clef-octave objects.
- **Negative control:** The same sources also contain `KeyAttributes` cmper 3 with a zero flags
  word, but no `Cn` or `Cp` records for cmper 3. Their companions leave `hasClefOctv` false there.
- **Conclusion:** **Weak.** Finale's conversion behaves as if it repairs a stale false flag from
  the actual presence of matching clef-octave records. The reader now applies that repair after
  all pools are imported and reports `LegacyMusAdjusted`; it does not infer the reverse from
  missing records.
