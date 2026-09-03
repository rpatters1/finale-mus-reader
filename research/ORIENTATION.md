# Orientation

**Read this, [STATE.md](STATE.md), and [INDEX.md](INDEX.md) at the start of a session. Nothing else.**

## What this project is

An MIT-licensed C++ library that reads legacy proprietary Finale `.mus` files and constructs a
`musxdom::Document`. It is a *client* of musxdom: it constructs musxdom objects directly, adds no
second document model, and puts no legacy decoding into musxdom. Research and initial
implementation stage — implement narrow, verified vertical slices.

## The pipeline

1. **Classify** the container: trial byte orders against framing until one validates.
2. **Decompress** per epoch — none, PKWARE DCL (`blast`), or zlib.
3. **Frame** typed blocks and pools; recover the banner header.
4. **Decode** records as word streams addressed by `(tag/selector, cmper, incidence, word slot)`.
5. **Overlay** decoded values onto a document seeded from the pinned Finale 27 baseline, then
   `finish` hands it to musxdom.

The reader builds exactly one document. There is no fallback document that could leak content.

## Epochs

| Epoch | Versions | Container |
|---|---|---|
| `CodaBanner` | 1.x–2.6 | Chained pools, no `ENIGMA BINARY FILE` signature |
| Uncompressed | 3.x–2000 | Four uncompressed typed pools, platform byte order |
| DCL | 2001–2006 | PKWARE DCL blocks, CRC-32 |
| Zlib | 2007–2012 | Typed zlib blocks, CRC-32, later variable record frame |

Details: [`format/container/eras.md`](format/container/eras.md).

## Terminology

**Epoch** — one of the four container families above. **Selector/tag** — the numeric record family
key (ETF spells it `^NN`). **Cmper** — the record's comparator key; `65534`/`0xfffe` is the globals
cmper. **Incidence** — one 16-byte row of a multi-row logical record. **Word slot** — a word index,
absolute across incidences. **Companion** — a Finale 27 `.musx` re-save used as a semantic
reference, never a byte-for-byte one. **`corpus_id`** — a content hash naming evidence without
naming a file. **`ValueOrigin`** — `LegacyMus`, `LegacyBehavior`, or `Finale27Default`.

## Invariants that change decisions

- Other/detail rows are **16 bytes**, not 16 words, from Finale 3.0 through 2006.
  ([`format/container/record_rows.md`](format/container/record_rows.md))
- **Where the data states its own layout, read that instead of dating the file.** A structural
  marker outranks a version gate. ([`reference/decoder_rules.md`](reference/decoder_rules.md))
- **Prefer an epoch gate to a version gate.** A version gate fails silently on any file whose
  version cannot be recovered — including every Coda-banner Windows document, which states a
  platform where its Mac siblings state a version.
- **Never re-encode pre-Finale-2012 text without that text's own font**, down to a single stored
  character. ([`format/container/text_encoding.md`](format/container/text_encoding.md))
- **Nothing is implemented more than once.** A second copy of a fact is a defect. See `AGENTS.md`.

## Confidence vocabulary

`confirmed` (reproduced across the stated sample) · `strong` (multiple independent observations
agree) · `weak` (working hypothesis) · `open` (not interpreted) · `superseded` (replaced; the
pointer says by what). Provenance labels: `public-PDK-derived`, `private-framework-derived`,
`independently binary-verified`. Do not promote a label to make a document read better.

## Navigation contract

**Do not read `research/` recursively, and do not preload `format/`, `investigations/`,
`history/`, `reference/`, or `corpora/`.**

- **Class notes are named after their source file.** `src/import/<pool>/<name>.cpp` is documented
  in `research/format/<pool>/<name>.md`, and its experiment history in
  `research/investigations/<name>.md`. Open that one file.
- For anything else, open [INDEX.md](INDEX.md) and follow one entry.
- Before forming a new hypothesis about a field labeled `open`, `weak`, or `superseded`, read the
  matching [`investigations/`](investigations/index.md) file first — the experiment has probably
  already been run, and possibly refuted.
- Before proposing that framing, record width, byte order, or the 2001–2006 payload works some
  way, check [`history/FAILED_HYPOTHESES.md`](history/FAILED_HYPOTHESES.md).
- **Before editing any documentation, use the `maintain-documentation` skill.**
