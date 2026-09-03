# Early-version evidence

**Covers:** Archive-derived early-version evidence, the ETF evidence set, and the Finale 1.8.7-2.6 to 3.0 correlation.
**Read when:** Investigating a Finale 1.x-2.6 question, or judging what evidence exists for that era.
**Confidence:** strong for the correlation; several spans remain open.

## Archive-derived early-version evidence

The expanded archive survey changes the earliest-version picture. StuffIt extraction with `unar` 1.10.7 produced explicit Coda-banner samples labeled Finale 1.8.7, 2.0.1, and 2.6, plus additional 3.0–3.7 files. These files should not be conflated with the 55 direct Coda-banner/unknown files. Public IDs and hashes are in `data/archive_members.csv`; original archive/member locations are private.

**Superseded.** This section previously stated that 1.8.7 was the earliest explicit product observed and that no
explicit Finale 1.0 sample had been found. The `rpatters1-installs` survey holds 22 loose Finale 1.0.0 files — 14
samples, 6 templates and 2 tool demonstrations — so the earliest explicit product is now 1.0.0, and it is not
archive-derived. They were invisible to earlier surveys for two compounding reasons, both since fixed in
`scripts/`: they carry no extension, and their banner uses a third spelling, `Finale` followed by a MacRoman
trademark sign (0xAA) and terminated by `ENIGA Structures` (sic) rather than a copyright notice, which neither the
`Finale(R)` nor the `Finale(TM)` pattern matched.

Finale 27 successfully opened the selected 1.8.7, 2.0.1, and 2.6 files after `.mus` was appended to their
filenames and produced private `.fin27.musx` references. Thus there is no parser compatibility cutoff among these
tested versions. The initial failure mode was file recognition for extensionless classic-Mac documents, not rejection
of their data format. The 2.6 conversion reported font issues, which affect rendering fidelity but did not block
conversion. Finale 1.0 remains untested.

### ETF evidence set

**Confirmed.** Fifteen locally retained ETF exports now provide a semantic record-level reference for the older
families. The original six archival exports remain under ignored `private/evidence/`; the controlled F2002–F2005 pairs
are tracked under `tests/evidence/` because they contain no private source layout. Their hashes and provenance are recorded
in the experiment log.

| Evidence | Source era | ETF size | Observed sections | Selected observations |
|---|---|---:|---|---|
| `mus-d89e8fe12e271440` ETF | Finale 2005 | 16,893 bytes | header, others, details, entries, text, lyrics | Explicit ETF header identifies Finale 2005; six `eE` entry records and tuple/detail records expose the high-entropy era's logical model. |
| `mus-3597fd4fce0c272b` ETF, Finale 2000 exporter | Finale 2000 | 27,945 bytes | header, others, details, entries, text, lyrics | Explicit header identifies Finale 2000; no `eE` entries; compact options/defaults and text blocks. |
| `mus-3597fd4fce0c272b` ETF, Finale 2005 exporter | Finale 2000 source, Finale 2005 saver | 34,029 bytes | header, others, details, entries, text, lyrics | Same source document but Finale 2005 header; adds `&f`, `PD`, `XA`, expressions, and other records. This is direct evidence that resaving can synthesize/upgrade records. |
| `mus-3a8b724cf3adba80` ETF | Finale 2000 | 28,718 bytes | header, others, details, entries, text, lyrics | Exact source-version pair: 1,107 others, 64 details, eight entries, and raw text match the uncompressed binary pools; includes `CN`, `GF`, and `TP`. |
| `mus-7aa45639c14b3864` ETF | Finale 1.8.7 | 123,084 bytes | others, details, entries, text, lyrics | 1,094 `eE` entry lines and 891 detail lines; no modern binary-style header section. |
| `mus-2c0a5e8897b436d5` ETF | Finale 2.0.1 source, Finale 2.6.3 exporter | 74,040 bytes | others, details, entries, text, lyrics | 549 `eE` entry lines and 497 detail lines; old ETF uses the same broad logical sections despite the Coda-banner binary family. |
| `mus-bd0042f8e0354192` ETF | Finale 2.6 | 1,272,164 bytes | others, details, entries, text, lyrics | 9,446 `eE` entry lines and 6,814 detail lines; the large sample is suitable for testing whether early records scale regularly. The StuffIt copy was necessary because the ZIP copy lacked the resource fork. |
| `F2002-baseline.etf` | Finale 2002a.r1 | 14,068 bytes | header, others, details, entries, text, lyrics | Three `eE` entries; exact pair with `F2002-baseline.mus`. |
| `F2002-changed-C-to-D.etf` | Finale 2002a.r1 | 14,075 bytes | header, others, details, entries, text, lyrics | Same three entries, with localized pitch-related field changes; exact pair with `F2002-changed-C-to-D.mus`. |
| `F2003-baseline.etf` | Finale 2003a.r1 | 16,033 bytes | header, others, details, entries, text, lyrics | Three `eE` entries; exact pair with `F2003-baseline.mus`. |
| `F2003-changed-C-to-D.etf` | Finale 2003a.r1 | 16,034 bytes | header, others, details, entries, text, lyrics | Same pool layout; one-byte length increase in the entry record and localized entry-payload change. |
| `F2004-baseline.etf` | Finale 2004c.r1 | 16,334 bytes | header, others, details, entries, text, lyrics | Three `eE` entries; same typed pool sequence, with a larger `0x0010` record than F2002/03. |
| `F2004-changed-C-to-D.etf` | Finale 2004c.r1 | 16,334 bytes | header, others, details, entries, text, lyrics | Entry pitch changes plus `BC` records; automatic note spacing is the leading explanation for the derived changes. |
| `F2005-baseline.etf` | Finale 2005b.r1 | 16,381 bytes | header, others, details, entries, text, lyrics | Three `eE` entries; same typed pool sequence. |
| `F2005-changed-C-to-D.etf` | Finale 2005b.r1 | 16,381 bytes | header, others, details, entries, text, lyrics | Entry pitch changes plus the same `BC` dependency pattern seen in F2004; automatic note spacing is the leading explanation. |

Targeted Finale 27 conversions of the three earliest ETF-backed sources are retained locally:

| Private semantic reference | Size | SHA-256 | Finale 27 pool counts (others/details/entries) |
|---|---:|---|---:|
| `mus-7aa45639c14b3864` | 60,732 | `4742e6fc35dd6892a6548c45c75c19dd139a6eb1be302733d8725a73d49ccd06` | 2,018 / 787 / 1,320 |
| `mus-2c0a5e8897b436d5` | 38,937 | `07d9f18fae4973f31fc69c8f0ecc551cfb8aa79b2131d2ff6dbdbeb818ab1328` | 1,347 / 478 / 659 |
| `mus-bd0042f8e0354192` | 392,491 | `bea52ad03c93c6ba5c7d6dd41c7a2c3ca57675ad5510bd68795963172cc1f311` | 14,913 / 6,095 / 11,153 |

The counts differ from ETF and binary physical rows, so these remain normalized semantic references rather than
serialization maps. Finale 27 reported font issues for the Finale 2.6 source, but generated a valid MUSX container.

ETF records are textual and use explicit section names plus two-character (or extended) structure identifiers, with
`(cmper)` for “other” records and `(cmper,inci)`-like keys for details. `eE` is the prominent entry structure in the
older exports. This confirms that the conceptual “other/detail/entry/text” decomposition predates the 2007 typed-zlib
container. ETF **by itself** did not identify the pre-2007 binary codec because it contains no binary offsets,
compressed bytes, or direct byte-for-byte rendering guarantee. Now that PKWARE DCL decoding is solved for 2001–2006,
an ETF paired with the exact `.mus` saved by the same Finale version is valuable as known semantic input for decoded
record and field correlation, especially with controlled edits.

The two exports of the same Finale 2000 template are especially important: the Finale 2005 export adds records and
changes some defaults, while the header changes from Finale 2000 to Finale 2005. A later-version ETF must therefore be
treated as a normalized semantic reference, not as a lossless reconstruction of the original binary save.

### Correlation between Finale 1.8.7–2.6 and Finale 3.0

**Strong physical and semantic continuity; framing still differs.** Re-extraction of the three ETF-backed early
binaries shows that all begin their main record region at `0x20a`, ten bytes after the common `0x200` body boundary,
and maintain a 16-byte cadence. They do not use Finale 3.0's four typed/length pool headers.

The early ordinary layout places the two-byte tag first, followed by the 12-byte payload and 16-bit comparator. Finale
3.x moves the comparator to the front but retains the same 16-byte capacity. Early detail tags occur ten bytes into
each 16-byte row; ordinary two-character tags and the compact pseudo-detail identifiers are directly recognizable.
The mapping `#v1`–`#v10` → `0x8001`–`0x800a`, `#c*` → `0x9001`–`0x900a`, and `#s*` →
`0xa001`–`0xa00a` is exact in the three samples.

Tag-order correlations are unusually strong:

| Source | Ordinary correlation | Detail correlation | Entries | Text |
|---|---|---|---|---|
| Finale 1.8.7 `mus-7aa45639c14b3864` | all 1,447 ETF ordinary tags match from the first binary row | all 891 tags match | all 1,094 32-byte rows reconstruct byte-for-byte from `eE` | ETF text is an exact 405-byte prefix of the raw binary tail |
| Finale 2.0.1 `mus-2c0a5e8897b436d5` | first 38 tags match; Finale 2.6.3 export then adds/reorders ordinary data | all 497 tags match | all 549 rows reconstruct byte-for-byte | exact 936-byte text prefix |
| Finale 2.6 source class | first 80 tags match in the ZIP data fork used for comparison; source-copy/build normalization remains possible | all 6,814 tags match | all 9,446 rows reconstruct byte-for-byte | exact 1,401-byte text prefix |

The early entry ID is implicit in row position. Each entry occupies 32 bytes and corresponds directly to one `eE`
record. Finale 3.x's 38-byte entry row is the same 32-byte entry data with a four-byte explicit entry ID prepended and
a two-byte zero/reserved suffix appended. This is a direct structural bridge across the 2.x/3.0 boundary.

Unexplained index/directory spans remain between the ordinary, detail, entry, and text regions. Thus a 1.x–2.x reader
cannot yet locate every pool generically, even though its core rows and semantics are closely related to Finale 3.0.
Finale 27 opened all three tested sources once their extensionless names were given a `.mus` suffix. This separates
container compatibility from classic Mac file discovery: suffix handling and missing type/creator metadata can make
a readable data fork appear unsupported. The three conversions are semantic references, not proof of losslessness.
Their Finale 27 pool counts differ substantially from ETF, confirming normalization: 1.8.7 converts to
2,018 others/787 details/1,320 entries; 2.0.1 to 1,347/478/659; and 2.6 to 14,913/6,095/11,153.

## 2026-08-08 — Finale 1.8.7–2.6 correlation with Finale 3.0

- **Question:** Are the explicit early archive files structurally unrelated to Finale 3.0, or do they contain an earlier serialization of the same records?
- **Files examined:** Re-extracted data forks for `mus-7aa45639c14b3864` (Finale 1.8.7), `mus-2c0a5e8897b436d5` (Finale 2.0.1), and the Finale 2.6 `Score` source class, paired with their existing private ETFs. ZIP and several parallel StuffIt `Score` copies were distinguished by hash rather than assumed identical.
- **Method:** Inspect from `0x200`; test 16-byte cadence from `0x20a`; compare literal and compact tag sequences; reconstruct each old `eE` record as four 32-bit values, two packed 16-bit values, and three 32-bit flag values; compare all reconstructed 32-byte rows against the binary; locate raw `^block` text and compare ETF content.
- **Observation:** All three binaries begin 16-byte records at `0x20a`. Finale 1.8.7 matches all 1,447 ordinary ETF tags; the 2.0.1 and 2.6 comparisons match initial prefixes before later-export/source-copy normalization changes ordinary ordering. Detail tags match completely: 891/891, 497/497, and 6,814/6,814, including compact encodings `0x80xx`, `0x90xx`, and `0xa0xx` for ETF `#v*`, `#c*`, and `#s*` pseudo-tags.
- **Entry observation:** Entry IDs are implicit array positions. Every ETF entry reconstructs its corresponding 32-byte binary row exactly: 1,094/1,094, 549/549, and 9,446/9,446. Finale 3.x extends that same entry body to 38 bytes by prepending the explicit four-byte entry ID and appending two zero/reserved bytes.
- **Text observation:** The ETF text payloads are exact prefixes of the raw binary tails: 405, 936, and 1,401 bytes respectively. Each binary has 13 additional trailing bytes outside the ETF text payload.
- **Conclusion:** **Strongly confirmed continuity.** Finale 1.8.7–2.6 and Finale 3.0 share the logical record vocabulary, 16-byte ordinary/detail cadence, entry field representation, and raw text. Finale 3.0 adds explicit typed/length pool framing, rotates key fields within ordinary/detail rows, and expands entries from implicit-ID 32-byte rows to explicit-ID 38-byte rows. The early index/directory spans and generic pool-boundary algorithm remain unresolved.
- **Finale 27 result:** Finale 27 opened all three sources—1.8.7, 2.0.1, and 2.6—after `.mus` was appended to the extensionless filenames. It produced valid MUSX containers with decoded pool counts of 2,018/787/1,320, 1,347/478/659, and 14,913/6,095/11,153 for others/details/entries. The 2.6 file had font issues but converted. This disproves a parser cutoff at 2.6.x for the tested data forks and identifies filename recognition as the original barrier.
- **Follow-up:** Request exact minimal MUS/ETF pairs saved and exported by Finale 2.6.3 and, if supported, Finale 1.0; test Finale 1.0 separately because it remains outside the verified compatibility range.
