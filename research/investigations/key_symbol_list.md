# KeySymbolList investigations

**Covers:** Experiments behind `KeySymbolListElement` storage and conversion.
**Read when:** Investigating accidental symbol strings, signed slots, or their font.
**Confidence:** confirmed for Coda fixed records; weak for the later-epoch sample.

## 2026-09-04 — Symbol-list identity and string boundary

- Finale 2003 source `mus-b67aad6814dc4059` contains fixed `KS` detail records. Their first null
  terminates the accidental glyph string; later bytes are nonzero and do not appear in the
  companion value.
- Little-endian Finale 2010 source `mus-4b3246869b9d07d7` identifies detail class `0x0416`.
  Its 259 instances each have a ten-byte payload. One list runs from comparator 0 through 32 and
  from unsigned comparators 65505 through 65535, which are signed values -31 through -1.
- **Refuted prediction:** Symbol-list comparators are not confined to 1 through 7 and -1 through
  -7. The importer preserves every stored comparator and lets musxdom expose its signed value.
- **Font dependency:** musxdom documents the string as using `KeyAttributes::fontSym` when nonzero
  and the key font otherwise. Conversion is deferred so importer registry order cannot change the
  result.
- **Conclusion (superseded in part):** The later-epoch evidence remains **weak**. The Coda identity,
  signed comparator interpretation, and ten-byte boundary are now confirmed by the controlled
  fixture below. Uncompressed storage remains unrepresented.

## 2026-09-05 — Finale 27 corrupts legacy Petrucci double sharps

- Ten sources spanning Finale 3.0, Finale 98, and Finale 2012 store Petrucci byte `0xdc` in
  otherwise matching symbol-list strings. Their Finale 27 companions store U+008B at precisely
  those positions. The decoded EnigmaXML contains UTF-8 bytes `c2 8b`, so neither the XML parser
  nor the surveyor introduced the value.
- Visual inspection of the companion for `mus-7af59089d5cae405` established that U+008B is
  mojibake, while the source `0xdc` is the intended Petrucci double sharp.
- Coda-banner sources are excluded from this classification: their differences include record
  alignment and comparator changes, and their companions do not apply the same substitution.
- **Conclusion (strong):** classify a non-Coda companion difference as a Finale encoding error
  only when its recovered `accidentalString` is otherwise identical after replacing every
  U+00DC with U+008B. Evidence: ten distinct sources across `rpatters1-main` and
  `rpatters1-installs`; semantic inspection of `mus-7af59089d5cae405`.

## 2026-09-05 — Controlled Finale 1.0 key-signature library conversion

- `tests/evidence/F100/F100-keysiglib.mus` was made by loading Finale 1.0's key-signature library
  into the controlled baseline. Its ETF independently prints the same `KS` comparator pairs and
  payload words.
- The source has 63 physical `KS` rows representing 46 unique `(cmper1, cmper2)` pairs. Seventeen
  pairs have a second, identical physical incidence. All use symbol-list id 1; custom key cmper 2
  references that list.
- The Finale 27 companion contains 40 `keySymList` elements. Every surviving element retains its
  source comparator pair. It omits the six elemental slots 1, 2, 4, -4, -2, and -1, each of which
  has a one-character source string. This omission, rather than comparator renumbering, caused
  positional array comparison to misalign the rest of the class.
- The companion rewrites 26 of the 40 surviving strings, so those content changes remain a
  separate conversion question. Its U+00DC, U+00BA, and U+00F5 characters are byte-preserving
  forms of source bytes `dc`, `ba`, and `f5`: companion font 0 remains Petrucci with symbol charset
  4095. A plain-text rendering resembles `Ü`, `º`, and `õ`, but this is distinct from the U+008B
  corruption above.
- In `F100-keysiglib-maybefixed.musx`, changing the Petrucci preference from 71 to 24 points and
  opening and saving the list through a Lua key-signature editor leaves all 40 serialized
  `keySymList` elements byte-for-byte unchanged. The editor creates controls for all 15 modern
  alteration values, but the six missing records appear as blank controls. Inspection of an
  independent list-loader implementation confirms that `Load()` returns only physical records;
  it does not consult `GetDefaultList()`. The save also creates unrelated custom key cmper 3 and
  normalizes numerous options, so it is not a replacement semantic companion.
- **Conclusion (confirmed):** compare this class by exact `(cmper1, cmper2)` identity. Classify a
  missing Coda elemental-slot record as Finale upgrade loss at the serialized-record level, while
  leaving changed strings visible until their conversion is characterized. Evidence: the tracked
  MUS, ETF, original Finale 27 companion, and edited Finale 27 save for
  `tests/evidence/F100/F100-keysiglib.mus`.

## 2026-09-05 — Coda Mac symbol strings are word-packed low-byte-first

- The controlled Finale 1.0 ETF prints the same logical payload words as the MUS. For example, a
  word whose raw big-endian bytes are `$#` converts to the companion string `#$`; swapping each
  adjacent byte pair before null termination accounts for every surviving string.
- Six other Coda Mac sources in the `rpatters1-installs` survey show the same transformation. Coda
  Windows records do not: their little-endian storage already places the logical low byte first.
- Later big-endian `KS` records remain ordinary byte strings and do not show pair transposition.
- **Conclusion (confirmed):** restore adjacent bytes only for big-endian Coda-banner `KS` payloads.
  Evidence: `tests/evidence/F100/F100-keysiglib.mus`, its ETF and companion, plus six distinct
  sources in `rpatters1-installs`.

## 2026-09-05 — Finale 27 drops trailing whitespace controls

- After correcting Coda Mac word order, all 106 remaining symbol-string differences across the 19
  Coda sources in the retained unexpected-difference cohort differ only by a terminal control:
  87 end in `0x01` and 19 end in `0x06`.
- Removing that trailing suffix makes each source string exactly equal to its companion. Internal
  controls and any other string difference remain unclassified.
- **Conclusion (strong):** classify this exact companion transformation as a whitespace-control
  difference while preserving the recovered source bytes. Evidence: 19 distinct sources in
  `rpatters1-installs`, covering six Mac Finale 1.0.0 and thirteen `PC 1.0+` documents.
