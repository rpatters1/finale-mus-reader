# Record rows and fields through Finale 2006

**Covers:** Fixed 16-byte other/detail rows, pool shapes, multi-incidence logical objects, field addressing, and the 32-bit word-order question.
**Read when:** Adding a field mapping, or reasoning about incidence and word-slot addressing.
**Confidence:** confirmed for the 16-byte row; word order in 32-bit fields is an untested open hypothesis.

## Record pools and row shapes through Finale 2006

**Confirmed** on 2026-08-09 against the controlled fixtures, seven large corpus files of the
2002-2006 era, three uncompressed-era files, and all 54 Coda-banner files then known.

Every epoch through Finale 2006 stores its records in four pools in the same order, and only the
container framing differs. Pool identity was established by measuring, for each pool, the fraction
of 16-byte rows whose bytes at a given offset are alphanumeric.

| Logical pool | Coda banner | Uncompressed | DCL | Row test |
|---|---|---|---|---|
| others | pool 0 | `0x0001` | `0x000f` | 0.97-1.00 alphanumeric at offset 2 |
| details | pool 1 | `0x0002` | `0x0010` | 0.95-1.00 alphanumeric at offset 4 |
| entries | pool 2 | `0x0003` | `0x0011` | ~0.00 at both, size frequently not a multiple of 16 |
| text | length-prefixed chunks | `0x0004` | `0x0012` | ~0.55 at both, the signature of mixed ASCII |

The text pool is not made of rows at all; it is a byte stream of Enigma text chunks, and the
zlib epoch carries it in block `0x0017`. See [The text pool](../texts/text_pool.md#the-text-pool).

The others and details tests are mutually exclusive in every file examined: a pool scoring high at
offset 2 scores near zero at offset 4 and the reverse. That is a direct consequence of the two row
shapes, which are the only two needed to normalize the whole pre-2007 range:

| Shape | Layout | Payload |
|---|---|---|
| other | comparator (2), tag (2), payload (12) | six 16-bit words |
| detail | comparator 1 (2), comparator 2 (2), tag (2), payload (10) | five 16-bit words |

**Confirmed: the tag is a 16-bit value, not a character pair, and is subject to byte order like
every other field of the row.** A little-endian file stores `FN` as the bytes `NF` and `LA` as
`AL`. Reading the two bytes literally mismatches every tag in such a file, and the failure is
silent: the pool decodes, the rows look structurally valid, and no lookup ever matches. A Finale
3.0 file read that way appears to contain an unfamiliar vocabulary of `NF`, `AL`, `RF`, `SM`, `bs`,
`co` records, which is simply `FN`, `LA`, `FR`, `MS`, `sb`, `oc` reversed, alongside numeric tags
that read as `10`, `40`, `50` instead of `01`, `04`, `05`.

This is worth stating plainly because it is easy to mistake for a format difference. Before the
byte order was applied to tags, every little-endian file in the corpus recovered nothing, and the
failures looked like unframed variants or an older record vocabulary rather than a decoding error.
Correcting it took the Finale 2001-2006 recovery rate from 410 of 426 files to 426 of 426.

A detail carries a second comparator, which displaces its tag by two bytes. This is the same
distinction the ETF makes by argument count: in the 2.6-era ETF `mus-2c0a5e8897b436d5`, all
thirteen tags printed with two arguments (`AS`, `BL`, `CL`, `CN`, `ED`, `GF`, `IM`, `LL`, `MT`,
`ST`, `TN`, `TP`, `Ts`) appear in the binary details pool at offset 4, and none appear in the
others pool.

Entries are neither shape. Their pools are often not a multiple of 16 bytes, consistent with the
32-byte implicit-ID entry rows reported for the early family, so a reader must not apply the
others row stride to them.

Two cautions for anyone reproducing this:

- **The controlled fixtures cannot demonstrate the details pool.** Their `0x0010` block is roughly
  half a kilobyte and scores 0.09, because the fixtures contain almost no detail-bearing content.
  Real corpus files of the same era score 0.98-1.00. Any test of details framing needs a file with
  real content.
- **Coda-banner pools are page-padded, so a naive fraction understates them.** Measured over whole
  pools including padding the details score looks as low as 0.47. Excluding all-zero padding rows
  it is 0.965 across all 54 files then known, in line with the banner eras. The remaining 3.5% are not ASCII
  tags at all but sequential high-bit values, `0x8001`, `0x8002`, `0x8003` and so on, whose meaning
  is **open**. The Coda details pool also carries ten tags the ETF does not print with two
  arguments (`AC`, `BH`, `CD`, `DE`, `DO`, `MM`, `Te`, `UE`, `sB`, `ve`), which is **open** as well.

## Record fields

In the big-endian `0x001a` ordinary variant, the first five words behave as:

1. numeric record type;
2. primary key/`cmper` or `0xfffe` option sentinel;
3. part id;
4–5. payload byte count.

The `0xfffe` records at the front of `0x001a` are singletons with codes beginning at `0x000f` and are strongly interpreted as options. This shows that options are not wholly free-form in 2007+, although their code-to-option and field mappings remain open.

For `0x001b` details, the corresponding fields are numeric type, `cmper1`, `cmper2`, part id,
and payload byte count. Class `0x041d` establishes `cmper1` as staff and `cmper2` as measure for
`MeasureGraphicAssign`. The current importer deliberately consumes only part id 0. Nonzero
part records and the distinction between partial and absent sharing remain outside its scope.

See [RECORD_CATALOG.md](../../corpora/rpatters1-main/RECORD_CATALOG.md) for all observed identifiers. Examples of exact corpus-wide matches include:

| Code | EnigmaXml structure | Evidence |
|---:|---|---|
| `0x0086` | `durAllot` | exact in 375/375 framed files |
| `0x0092` | `frameSpec` | Pearson 0.999; exact in 273/375 (conversion expands some) |
| `0x00a4` | `measNumbRegion` | exact whenever present; 368 files |
| `0x00d9` | `smartShape` | Pearson 0.998; conversion differences observed |
| `0x011a` | `partDef` | exact in 375/375 framed files |
| `0x03ef` | `acciAlter` | strong correlation with conversion differences |
| `0x03f3` | `baselinesExprAboveStaff` | exact in 324/324 detail-framed files |
| `0x0414` | `gfhold` | Pearson 0.999; exact in 291/324 |

### Word order in 32-bit fields — open hypothesis, not tested

Most 32-bit option fields are two 16-bit words with the **high word first**, each word in the
container's byte order (the framework's `MACFOURBYTE`); a minority are plain little-endian longs
(`WINFOURBYTE`). The text-options insert element holds one of each: its two trackings are high word
first in both byte orders, while its Finale 2012 `symChar` is a plain little-endian long. See
[Text options](../options/text_options.md#text-options).

**Hypothesis.** The convention follows *when the field entered the format*, not what it means. The
Enigma era — uncompressed and DCL, when big-endian Macintosh was a first-class target — fixed word
order deliberately so one rule served both byte orders. By the zlib era big-endian was fading, and
by Finale 2012 nobody was thinking about it, so fields added late are simply written in native
little-endian. If this holds, word order is predictable from a field's introduction version and does
not need to be determined per field.

**How to test it, incidentally.** `data/legacy_option_mappings.csv` carries 36 four-byte rows, 31
`MACFOURBYTE` and 5 `WINFOURBYTE`. Each future import that lands a 32-bit field adds a dated data
point: record the field's earliest observed version alongside its observed convention and see
whether the two `WINFOURBYTE` late arrivals stay late and the `MACFOURBYTE` set stays early. Do not
mount a dedicated study for this. One caution for whoever picks it up: the current `WINFOURBYTE`
rows are not uniformly late — `smpteStartTime` is Finale 2014, but the selector-77 page dimensions
are not a late field — so the hypothesis is not confirmed by a glance at the inventory and may need
a narrower statement than "late means little-endian".
