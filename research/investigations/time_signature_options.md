# TimeSignatureOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-09-02 — TimeSignatureOptions initial recovery

- **Question:** Can all modern `TimeSignatureOptions` members be connected to legacy preference
  records closely enough for an initial importer, and which early fields remain unsupported?
- **Semantic evidence:** The public `FCDistancePrefs` and `FCMiscDocPrefs` documentation and linked
  headers at `pdk.finalelua.com`, accessed 2026-09-02, account for all fourteen members.
  `FCSizePrefs` has no time-signature member. These claims are **public-PDK-derived**.
- **Location evidence:** An authorized read-only private interoperability source was consulted only
  after the public headers. It supplied selector/incidence/word leads for ten distance values and
  three miscellaneous values; the shared selector-44 courtesy word supplies the fourth. These
  leads are **private-framework-derived**. No declaration, comment, or implementation text was
  copied.
- **Independent checks:** The controlled Finale 1 score-spacing fixture confirms selector 18
  incidence 0 words 3--5. The controlled Finale 2008 records confirm that numeric class `0x0020`
  flattens incidence 1 into words 6--10. Synthetic tests exercise all fourteen mapped fields in
  uncompressed, DCL, big-endian zlib, and little-endian zlib records, plus the smaller Coda field
  set and missing-record fallback.
- **Tracked result:** The instrumented Release probe read 217 tracked-evidence occurrences (215
  distinct contents), with 73 Coda, 48 uncompressed, 65 DCL, and 31 zlib sources. All 217 sources
  and companions parsed. An initial capture had 2,996 equal leaves and 42 unexpected parts-field
  leaves while absent incidence-1 fields retained the pinned baseline. After the source-era shared
  behavior was reviewed, copying the five score distances to their parts counterparts eliminated
  every disagreement: all 3,038 leaves now agree, without an expected-difference rule.
- **Confidence and scope:** **Confirmed** for the three controlled Finale 1 locations and
  their shared-parts behavior, and **strong** for the other initial mappings. Lower/abbreviated
  score lift and the courtesy-time switch remain baseline-owned in Coda; single-control fixtures
  would strengthen the other mappings.
- **All-corpus result:** The authorized three-survey capture selected 16,314 occurrences (7,278
  distinct contents). Of these, 16,225 imported successfully and 4,625 had successful companions;
  89 failed before comparison, comprising 58 Finale LIB files and 31 inputs that did not classify
  as Finale MUS documents. All 64,750 companion-backed `TimeSignatureOptions` leaves agree: 2,422
  Coda, 4,774 uncompressed, 28,742 DCL, and 28,812 zlib. No expected-difference rule is involved.
