# Decoder rules

**Covers:** How to choose between a structural marker, an epoch gate, and a version gate, plus the binary-safety rules every decoder must honour.
**Read when:** Writing or changing any gate, layout selection, or binary read.
**Confidence:** these are project rules, not format findings.

Legacy MUS is a family of formats, not one stable binary layout. Classification
must use observed framing, byte order, lengths, and checksum/codec validation;
do not select a decoder from a marketing version alone.

Current broad eras are:

- Finale 1.x-2.x: pre-banner framing with unresolved directory spans.
- Finale 3.x-2000: uncompressed typed pools and platform-dependent byte order.
- Finale 2001-2006: PKWARE DCL-compressed typed blocks. A normal zlib install
  does not provide this decoder; use a pinned Mark Adler `contrib/blast`
  implementation with its original zlib license notice.
- Finale 2007-2012: zlib-related typed blocks, including transition-era byte
  order variation.

Keep physical framing separate from logical record interpretation. In
particular, the verified legacy other/detail rows are 16 **bytes**, not 16
words, and multi-incidence rows may form one logical object.

- Make all binary reads bounds-checked and overflow-safe.
- Preserve original offsets, raw values, selected byte order, format era, and
  confidence/provenance in diagnostics where useful.
- Validate declared sizes, decompressed sizes, CRCs, and complete input/output
  consumption as appropriate to the era.
- **Where the data or the record structure states which layout a file uses, read that instead of
  dating the file.** This outranks both gates below, and not as a stylistic preference: version
  coverage is incomplete and always will be. No Finale 3.3 or 3.4 document exists in either
  surveyed corpus, so a boundary in that window can only be guessed at; the Coda-banner era's
  Windows documents state no version at all; and a version read without the container's byte
  order gives a plausible wrong answer rather than no answer. A structural marker is also one
  step closer to the evidence, because a version boundary is usually inferred from the same
  observation the structure makes directly. Two mappings already work this way: the clef tuple
  width comes from the payload size, and the whole stem family's units come from the size of its
  connection collection; a field that is quiescent in one era and packed in the next serves the
  same purpose. Prove the marker against the corpus before relying on it. Say at the site why the
  marker is trustworthy, what it costs when the file is ambiguous, and whether a version gate
  would also have worked — a preference stated as a necessity is a claim that will not survive
  the next specimen.
  A test over unbounded content is not a marker but a heuristic: font names are whatever a user or system could install, so no rule distinguishes a header incidence from the first bytes of every possible name, and that boundary rightly stays a version range. A marker must be a fact about the record's shape, not a guess about its contents.
- Keep record layouts and option mappings explicitly version-aware, but **prefer an epoch gate
  to a version gate wherever the boundary is really the epoch.** A version gate is the more
  fragile instrument: it fails closed on any file whose version cannot be recovered, and it
  fails silently, leaving the class populated from reference defaults and every field reported
  as synthesized. That looks exactly like a document with nothing to recover. The Coda-banner
  era's Windows documents state a platform where its Mac documents state a version, so they
  have no version at all and a version-gated table skips them without a word.
- **Where a version gate is genuinely required, frame it inside the epoch it is gating.** A
  boundary that falls within one epoch — the font-definition header arriving in Finale 3.2,
  inside the uncompressed era — belongs to that epoch and should be expressed as a version
  range on a table already restricted to it. Listing extra epochs alongside it can only ever be
  satisfied by a misread version, and bounding the range at both ends keeps a wild version from
  landing in the wrong case. Never invent a version to satisfy a gate: a file that does not state
  one is telling you something, and a synthetic version would be fabricated evidence.
- **Never leave an epoch entirely uncovered by accident.** A gate that excludes a whole epoch
  must say in a comment at the gate that the exclusion is intended and why: that the era does not
  store the class, that it stores it somewhere still unlocated, or that the evidence to place it
  is missing. An epoch silently absent from a gate is a defect, not a scope decision, and it
  fails in the quiet way described above. Cover each epoch with at least one test, so that
  removing an epoch from a gate has to break something.
- Treat sharing, tag-specific fields, early directory spans, and other matters
  labeled open in the research as open.
- When evidence conflicts with an assumption, preserve the evidence and update
  the hypothesis rather than forcing the sample through an expected layout.
