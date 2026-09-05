# KeySymbolListElement

**Covers:** `KeySymbolListElement` identity, comparators, string boundary, and font selection.
**Read when:** Working on `src/import/details/key_symbol_list.cpp` or custom accidental glyphs.
**Confidence:** confirmed for fixed-record identity and string storage; weak for the zlib class
association.

The fixed detail tag is `KS`; the zlib detail class is `0x0416`. Cmper1 is the symbol-list id
referenced by `KeyAttributes::symbolList`. Cmper2 is a signed alteration value stored in the
unsigned comparator field. Real records are not limited to alterations -7 through 7: the observed
zlib list spans -31 through 32.

The payload is a ten-byte, null-terminated byte string. In Coda-banner records the bytes are packed
low-byte-first within logical words, so a big-endian container requires adjacent-byte restoration
before locating the null; Coda little-endian and every later layout are already byte streams. Bytes
after the first null are physical tail data and are not part of `accidentalString`. Conversion to
UTF-8 is deferred until all others records exist. It uses a referencing `KeyAttributes::fontSym`
when nonzero and otherwise the key font from `FontOptions`; unresolved font metadata takes the
reader's symbol-font fallback.

Some Coda strings fill the glyph portion without a null and retain a trailing Finale whitespace
control in the recovered value. Finale 27 removes that suffix when upgrading. Coverage treats the
change as a whitespace-control difference only when removing trailing bytes `0x01` through `0x07`
from the source makes the strings exactly equal; the importer preserves the source value.

Controlled Coda-banner evidence confirms the fixed tag, comparator identity, and string storage.
Some comparator pairs have duplicate physical incidences with identical payloads; together they
still represent one logical list element. The available DCL and zlib evidence is the same source
pair catalogued for the neighboring [custom-key clef octave arrays](custom_key_octaves.md).
Uncompressed storage has no controlled specimen.

Experiment history: [`../../investigations/key_symbol_list.md`](../../investigations/key_symbol_list.md).

Implementation: `src/import/details/key_symbol_list.cpp`.
