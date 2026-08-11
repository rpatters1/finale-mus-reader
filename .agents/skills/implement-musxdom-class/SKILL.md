---
name: implement-musxdom-class
description: Research, draft, test, and iteratively refine legacy Finale MUS recovery for a specified musxdom DOM class. Use when the user says they want to implement, recover, decode, or map a musxdom class or a tightly related set of classes from legacy .mus files, especially when they provide preliminary selectors, record identifiers, sample edits, structure hints, or epoch-specific evidence. This is an interactive implementation workflow; use survey-class-coverage afterward for broad corpus validation, not as a substitute for the initial evidence-driven draft.
---

# Implement a musxdom class

Drive one class from a user-supplied lead to a narrow working implementation,
then refine it against the corpus. Keep physical discovery, semantic interpretation,
implementation, and validation distinct so that later evidence can revise one layer
without destabilizing the others.

## Hard rules

- Read `AGENTS.md`, `research/README.md`, and `research/FORMAT_NOTES.md` before
  changing a decoder. Follow any more specific repository guidance they identify.
- Treat every locator or structure hint, including one from the user, as a hypothesis
  until supported by bytes. Preserve the hint and its provenance rather than silently
  promoting it to fact.
- Inspect the musxdom class and its EnigmaXML parsing behavior directly. Construct
  that class; never introduce a parallel document model or duplicate behavior already
  implemented by musxdom.
- Keep physical framing separate from logical interpretation. Version-gate layouts
  and semantic changes explicitly, using the file's recovered version and epoch.
- Make the first implementation a narrow vertical slice with stated coverage. Do not
  wait for universal knowledge, and do not advertise an untested epoch as supported.
- Treat ETF and modern MUSX companions as semantic references. They may normalize,
  synthesize, reorder, renumber, or discard source data.
- Keep corpora read-only and paths private. Use controlled tracked evidence or ignored
  `private/` output according to repository policy.
- Record recovered and synthesized values separately in `ImportReport`.
- Do not commit, push, or publish unless the user separately requests it.

## Work interactively

At each evidence boundary, tell the user what is known, what remains a hypothesis,
and which smallest new specimen or fact would discriminate between alternatives.
Ask one focused question when new user evidence is genuinely needed. If the existing
evidence supports a safe narrow implementation, continue without asking the user to
reconfirm the implementation request.

Useful user contributions include:

- a suspected pre-zlib tag, selector, comparator, incidence, or word offset;
- a zlib record identifier or payload shape;
- a public or authorized-private interoperability fact;
- a description of how one UI edit should affect the target field;
- controlled before/after files from a Finale version not otherwise available;
- knowledge of a version boundary, enum reorder, sentinel, or default behavior.

When asking for a controlled fixture, prescribe one minimal edit, the exact source
Finale version and platform desired, and any ETF or modern re-save that would make the
result more useful. Prefer a baseline plus one changed field over a realistic score
whose unrelated changes create noise.

## Step 1 — Define the vertical slice

Identify:

1. The fully qualified musxdom class and pool (`options`, `others`, `details`, or
   another owner), including comparator, incidence, sharing, and part semantics.
2. The first fields or collection elements to recover. List their C++ types, defaults,
   enum meanings, units, bit semantics, and whether they are references.
3. The epochs in scope: early pre-banner/Coda, fixed-row uncompressed, DCL, and zlib.
4. The preliminary physical locators and structure hints supplied by the user.
5. The fallback behavior expected when the source lacks the class or a field.
6. What will count as semantic agreement with ETF or modern EnigmaXML.

Start a working evidence matrix, kept private while exploratory, with one row per
field and layout variant:

| epoch/version | record identity | selector/cmper | incidence | offset/width | transform | target field | evidence | confidence |
|---|---|---|---|---|---|---|---|---|

Do not assume every target field existed in every era, that physical order equals an
enum's modern order, or that one record corresponds to one object.

## Step 2 — Establish the musxdom semantics

Search the configured musxdom source for the class declaration, `XmlNodeName`, XML
field names, enum definitions, constructors, pool accessors, and any custom parse or
serialization logic. Also inspect nearby classes for the idiomatic construction path.

Determine before decoding bytes:

- whether an object is seeded from the reference document or created from source
  records;
- which fields musxdom already transforms, validates, or derives;
- whether booleans are represented as individual XML properties but packed in MUS;
- whether identifiers are document-local cmpers that require referential resolution;
- whether absence differs semantically from zero or an empty collection.

Never restate a musxdom enum or normalization rule in reader code when it can be used
directly. A prose research table may explain the relationship but must point to the
single runtime implementation.

## Step 3 — Locate the records with small epoch samples

Use the smallest available controlled baseline/edit pair in each epoch. Decode through
the repository's existing container, decompression, row indexing, and byte-order code.
For every candidate, retain record identity, comparator, incidence, payload length,
raw words or bytes, block offset, and decoded offset.

Work from the most recognizable representation outward:

1. **Zlib to EnigmaXML.** Compare zlib payloads with the same class in a modern
   EnigmaXML companion. Field sequences, repeated object boundaries, enum values, and
   distinctive edited numbers often expose the logical structure while it still
   resembles older Enigma rows.
2. **Zlib identity to pre-zlib selector.** For numeric global options, explicitly test
   the established `numericGlobalClass(selector)` relationship: the zlib class id is
   the numeric selector plus `0x000e`, and incidences are coalesced into one payload.
   Use the shared helper if it applies. Do not generalize that offset to named tags,
   non-global pools, or a new class until samples verify it.
3. **DCL and uncompressed rows.** Locate the analogous typed rows and determine how a
   payload crosses 16-byte physical rows. A verified logical payload may span several
   incidences or pack several tuples into one row.
4. **Early layouts.** Expect fields to be scattered across selectors and versioned
   independently. Use controlled edits and raw offsets; do not force an early layout
   to resemble the later one.

Use `scripts/structure_probe.py`, the private record catalog, existing correlation
scripts, and disposable `/tmp` probes before inventing a new general-purpose tool.
Preserve a location-agnostic script under `scripts/` only when the final method will be
reused. Research scripts may duplicate a decoding rule for independent verification;
library code may not.

## Step 4 — Infer fields and version boundaries

Diff baseline/edit pairs at decoded-record granularity. Change one semantic value at a
time where possible. Test signedness, endian order, byte versus word width, long-word
order, scaling, enum translation, flag masks, collection count, and padding.

For each proposed mapping, require two kinds of support when practicable:

- a physical observation showing exactly which bytes changed; and
- a semantic observation from the UI, ETF, EnigmaXML, or independently resolved
  referent showing what the change means.

Use surrounding unchanged fields as structural evidence, not proof. Treat terminal
zeroes as padding only when record geometry or contrasting samples demonstrate it.
Take however many complete logical elements the payload supplies unless the format
itself encodes a validated limit.

When versions disagree, distinguish:

- physical relocation of the same semantic field;
- changes in collection order or enum meaning;
- additive fields absent from older files;
- upgrade-time synthesis by Finale;
- an unsupported or ambiguous source layout.

Present the draft evidence matrix and confidence labels to the user before making a
broad semantic assumption. A localized, strongly evidenced mapping can proceed while
other fields remain open.

## Step 5 — Implement the smallest sound mapping

Prefer the existing table-driven mapping framework when a field has a stable source
location and a direct assignment or bit extraction. Add a class-specific mapping file
under the class's musxdom pool directory (`src/import/others/`, `src/import/options/`,
`src/import/details/`, or `src/import/entries/`). Expose only its table or capture entry
from that pool's `<pool>.h`, and register it once in the central mapping registry. Keep
the shared table machinery in `src/import/legacy_mapping.*`; there is no intermediate
`mappings/` directory. Concrete table declarations and implementations should use the
corresponding pool namespace (`others`, `options`, `details`, or `entries`).

Choose target construction deliberately:

- `OptionsSingleton`: overlay a complete seeded options object.
- `OthersByCmper`: overlay reference-seeded objects only when their identities are
  genuinely stable and unmatched source records should not create objects.
- `OthersFromRecords`: construct source-owned pooled objects by source comparator.
- A bespoke capture pass: use for variable-length collections, physical-to-semantic
  reordering, cross-record assembly, reference repair, or transformations requiring
  several fields at once.

If another pool needs a new construction policy, extend the shared framework once;
do not hide a second mapping engine in the new class file.

Use the mapping macros and shared readers for widths, bits, version gates, offsets,
and reporting. Use a finalizer only for work requiring multiple recovered fields.
Route warnings through the report or musxdom logging channel. Preserve unsupported raw
observations in diagnostics when they materially help later refinement.

For options, follow the repository fallback sequence: seed from the fully formed,
platform-appropriate reference document, overlay source values, and leave unsupported
fields at the reference default. Exclude a class from blanket seeding only when it must
be reconstructed as a collection or requires class-specific referential repair.

For cmper fields, never compare or copy ids across documents as identities. Resolve the
referent on each side and define semantic equality for that class. If fallback requires
cloning a reference object, reuse an equivalent target object when safe; otherwise
allocate the next sequential target cmper and preserve the reference object's original
spelling and data. Define cmper-zero semantics explicitly rather than assuming zero is
an ordinary pooled object.

## Step 6 — Prove the draft on controlled evidence

Add focused tests for:

- one exact fixture from every claimed physical layout and byte order;
- each implemented version boundary;
- exact target values plus `ImportReport` origin and offsets;
- packed flags, signed values, longs, enum translations, and reference resolution as
  applicable;
- absent records retaining defaults or producing no object, according to policy;
- malformed, truncated, incomplete-incidence, or trailing-byte behavior where the new
  decoder introduces a boundary;
- no score content leaking from the reference document.

Test the source representation independently of the modern companion. A passing MUSX
comparison alone cannot prove that the original bytes were decoded correctly.

Run the focused test, the full test suite, generated-resource checks when relevant,
`git diff --check`, and `git status --short`. Do not disturb unrelated worktree changes.

## Step 7 — Refine with class coverage

Once the narrow implementation passes controlled fixtures, read and use
`../survey-class-coverage/SKILL.md`. Define the cohort with the user: all fixtures,
loose only, ETF-backed, Finale-27-backed, or an explicit union or intersection.

Make the targeted survey answer at least:

- Does every claimed epoch import the class without failing the document?
- How often is each field physically present, recovered, defaulted, unresolved, or
  rejected by a version gate?
- Do companions preserve, remap, normalize, substitute, synthesize, or remove it?
- Are there new payload sizes, incidences, identities, orderings, or version boundaries?
- Do cmper fields resolve to the expected same-side referents?
- Did the implementation need reference objects, and did it introduce duplicates?

Show contradictions and representative private fixture names/paths to the user only in
the local console as allowed by survey policy. Keep tracked findings aggregate and
sanitized. Revise code, tests, and confidence labels together, then rerun the smallest
cohort that exercises the change before rerunning the broader survey.

## Transferable lessons from FontOptions

Apply these as questions, not universal rules:

- A physical ordinal may stop matching the modern semantic enum at a version boundary.
- Zlib may retain an older plug-in-facing tuple shape even when its container changes.
- A packed effects word may need to become several musxdom booleans through the class's
  own setter or accessors.
- Structural zero fill can complete a fixed-size row without terminating a collection.
- A source reference may point to a missing definition; recovery then needs a semantic,
  same-type fallback rather than a blind numeric copy.
- Name normalization is a matching rule, not an instruction to rewrite source or
  reference spelling.
- A complete modern options collection can legitimately combine source-recovered values
  with explicitly reported reference defaults.

## Completion standard

Finish with a concise account of the implemented class and fields, supported epochs and
version gates, controlled fixtures and tests, corpus coverage, synthesized fallback,
known upgrade variances, remaining open layouts, and the next smallest evidence request.
Call the result partial whenever any epoch or field remains unsupported.
