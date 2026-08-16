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
- Keep physical framing separate from logical interpretation. Gate layouts and semantic
  changes explicitly, choosing the instrument by the hierarchy below rather than reaching
  for the version first.
- Make the first implementation a narrow vertical slice with stated coverage. Do not
  wait for universal knowledge, and do not advertise an untested epoch as supported.
- **Never leave an epoch entirely uncovered by accident.** A gate that excludes a whole
  epoch must say in a comment at the gate that the exclusion is intended and why: that
  the era does not store the class, that it stores it somewhere still unlocated, or that
  the evidence to place it is missing. An epoch silently absent from a gate is a defect,
  not a scope decision, and it fails quietly: the class populates from reference defaults
  and every field reports as synthesized, which looks exactly like a document that had
  nothing to recover. Cover each epoch with at least one test, so that removing an epoch from
  a gate has to break something.
- **Prefer a structural marker to any gate.** If the record stream states which layout it is in --
  a payload size, a collection length, a field that is quiescent in one era and packed in the next
  -- read that rather than dating the file. Version coverage is permanently incomplete: whole
  releases are missing from every corpus, the earliest era states no version on one platform, and a
  version read without the container's byte order is wrong rather than absent. Prove the marker
  against the corpus before relying on it, state what it costs on an ambiguous file, and say
  whether a version gate would also have worked -- a preference stated as a necessity is a claim
  that will not survive the next specimen. A test over unbounded content is a heuristic rather
  than a marker: nothing distinguishes a header row from the first bytes of every font name a
  user could install, so that boundary rightly stays a version range.
- **Prefer an epoch gate to a version gate, and frame any version gate inside its epoch.** A
  version range excludes eras nobody listed, and it fails closed on a file whose version
  cannot be recovered — the Coda-banner era's Windows documents state a platform where its Mac
  documents state a version, so they have none. Where a boundary really is a version, such as
  a record layout changing at Finale 3.2 inside the uncompressed era, put the range on a table
  already restricted to that epoch and bound it at both ends. Never invent a version to satisfy
  a gate: a file that does not state one is telling you something, and a synthetic version
  would be fabricated evidence.
- Treat ETF and modern MUSX companions as semantic references. They may normalize,
  synthesize, reorder, renumber, or discard source data. Where the reader deliberately
  disagrees with a companion -- keeping a value the upgrade discards, or declining to
  reproduce something the upgrade synthesizes -- say so in a comment at the site and in
  the research notes, so a later companion survey reads it as an intended difference
  rather than a decoding error.
- Keep corpora read-only and paths private. Use controlled tracked evidence or ignored
  `private/` output according to repository policy.
- Record recovered and synthesized values separately in `ImportReport`, and distinguish a
  third case from both. A field that a later Finale exposes as a setting is often fixed
  behavior in an earlier one: the value is known exactly, no record stores it, and the
  version or epoch that introduced the setting is the boundary. Report that as
  `ValueOrigin::LegacyBehavior`, not as a recovered value it has no bytes for and not as a
  synthesized default it is not guessing at. Where a capture pass establishes such a field
  before the mapping tables run, the tables leave its report entry alone rather than
  overwriting it with a default.
  Reserve it for a value the pinned baseline does not already supply, or supplies wrongly.
  Where the baseline already carries what the era's behaviour implies, leave it there: the
  baseline is generated from committed resources with recorded hashes, so asserting the same
  value in code is a second copy of one fact rather than a safeguard against drift.
- Keep project-owned source files unity-build clean. A future build may combine many
  class-specific translation units, and anonymous namespaces do not isolate names from
  one another after CMake amalgamates those files. Use distinctive file-local aliases,
  helpers, and constants; verify with a temporary `CMAKE_UNITY_BUILD=ON` build when the
  implementation or its neighboring source files change.
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

**A fixture added to `tests/evidence/` is not finished until it is in the corpus index.**
Re-survey that tree, which is registered as a corpus in its own right: `./private/regenerate.sh
tracked-evidence` where the private configuration exists, or `scripts/inventory.py` against
`tests/evidence/` with the export convention from `research/data/surveys.csv`, which needs no
private configuration at all. Then update that row — the source count, the date, the
`tool_commit`, and the `corpus_fingerprint`, which changes whenever a file is added — and
re-run any class-coverage cohort that draws on it.

Do this because the fixture tree is a *survey*, not a pile of test inputs, and it is the
only one that is both fully companion-backed and reproducible at all by anyone but its owner.
It is also where the earliest evidence lives: the private corpora begin at Finale 1.8.7, so
every Finale 1.0.0 document with a companion is a tracked fixture. A fixture left out of the
index is invisible to every coverage survey and to every count published from one, which is
exactly how an era comes to look unrepresented when it is not. Keep the fixture cohort in the
scope you agree with the user at Step 7 whenever the class reaches an era the private corpora
cover thinly or not at all.

Nothing about that tree is private, so its conventions still live in
`private/corpora/tracked-evidence.conf` only because that is where `regenerate.sh` looks;
say so rather than treating the file as sensitive.

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
`src/import/details/`, or `src/import/entries/`). Keep the shared table machinery in
`src/import/legacy_mapping.*`; there is no intermediate `mappings/` directory. Concrete
table declarations and implementations should use the corresponding pool namespace
(`others`, `options`, `details`, or `entries`).

**Expose one `ClassImporter` per musxdom class from that pool's `<pool>.h`, and register
that single entry point in the registry in `legacy_mapping.cpp`.** The registry states
which classes are imported and in what order, and nothing else. Everything era-specific
belongs inside the class's own translation unit: how many physical layouts it has, which
epoch or version gate selects each, whether a capture pass must build a collection before
the scalar tables overlay it, and what must be checked once they have run. A class with
four layouts and a capture pass is still one registry entry. Keeping that knowledge in the
class file is what lets a later era be added by editing one translation unit, and what
stops the generic mapping code from accumulating a branch per class.

Order within a pool is free; order between pools is a dependency statement and belongs in
a comment at the registry. Anything that must run after every pool is complete, such as
copying reference objects, is a separate phase after the importers rather than an
importer's responsibility. Tables and helpers that only the importer uses should be
file-local; expose a stage separately only where a test needs to drive it alone.

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

**Name the surveys as well as the cohort, and say what each one can and cannot answer.**
Every registered survey in `research/data/surveys.csv` is in scope, and they are not
interchangeable. The reference corpus has the companions and the volume but starts at Finale
1.8.7. The installs corpus has no authored documents and thin companion coverage, but it is
the only place several releases exist at all, and the only place the Coda-era Windows
documents that state no version are found — the population any version gate silently drops.
The tracked fixtures are tiny and must never be read as a percentage, but every one has a
companion and they hold the controlled one-variable pairs. A claim about a release absent
from the survey you ran is not a finding; check whether another survey has it before calling
an era covered.

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

After the user has approved the completed work and its pull request, identify the exact transient analysis artifacts that are unlikely to help with the next musxdom class, prompt for explicit approval, and clean up only the approved artifacts. Preserve reusable corpus inventories, path and companion mappings, and expensive archive caches.

Finish with a concise account of the implemented class and fields, supported epochs and
version gates, controlled fixtures and tests, corpus coverage, synthesized fallback,
known upgrade variances, remaining open layouts, and the next smallest evidence request.
Call the result partial whenever any epoch or field remains unsupported.

**State every completely uncovered epoch explicitly, every time.** Name the epoch, say
whether the exclusion is intended or merely unevidenced, and give the reason. This is not
covered by the per-field coverage account: a class can look thoroughly implemented, with
high field counts and clean companion agreement, while one whole era recovers nothing at
all. Saying "supported for X, Y and Z" leaves the reader to notice that W is missing.
Say that W is uncovered.
