---
name: implement-musxdom-class
description: Research, draft, test, and iteratively refine legacy Finale MUS recovery for a specified musxdom DOM class. Use when the user says they want to implement, recover, decode, or map a musxdom class or a tightly related set of classes from legacy .mus files, especially when they provide preliminary selectors, record identifiers, sample edits, structure hints, or epoch-specific evidence. This is an interactive implementation workflow; use analyze-recovery-coverage afterward for broad corpus validation, not as a substitute for the initial evidence-driven draft.
---

# Implement a musxdom class

Drive one class from a user-supplied lead to a narrow working implementation,
then refine it against the corpus. Keep physical discovery, semantic interpretation,
implementation, report integration, and validation distinct so that later evidence can
revise one layer without destabilizing the others.

## Hard rules

- Read `AGENTS.md`, `research/ORIENTATION.md`, and `research/STATE.md` before changing a
  decoder, then open only the target class's own file: `src/import/<pool>/<name>.cpp` is
  documented in `research/format/<pool>/<name>.md`, with its experiment history in
  `research/investigations/<name>.md`. Use `research/INDEX.md` for anything else, and do not
  read `research/` recursively. Follow any more specific repository guidance they identify.
- Treat every locator or structure hint, including one from the user, as a hypothesis
  until supported by bytes. Preserve the hint and its provenance rather than silently
  promoting it to fact.
- Inspect the musxdom class and its EnigmaXML parsing behavior directly. Construct
  that class; never introduce a parallel document model or duplicate behavior already
  implemented by musxdom.
- Keep physical framing separate from logical interpretation. Gate layouts and semantic
  changes explicitly, choosing the instrument by the hierarchy below rather than reaching
  for the version first.
- Make the first implementation a narrow vertical slice of **recovery**, but a complete
  vertical slice of the musxdom class's **field surface**. The reader mapping and coverage
  surveyor must enumerate every field from the first implementation, even when only a few
  have located source bytes. Do not wait for universal recovery knowledge, and do not
  advertise an untested epoch as supported.
- **Choose the gating instrument by the hierarchy in
  [`research/reference/decoder_rules.md`](../../../research/reference/decoder_rules.md):**
  structural marker first, then epoch gate, then a version gate framed inside its epoch. That
  file also states the rule that no epoch may be silently uncovered. Read it before writing a
  gate; do not restate it here. What this skill adds is when to apply it: you are choosing the
  instrument at Step 4, from the epoch samples of Step 3, before any table is written.
- Treat ETF and modern MUSX companions as semantic references. They may normalize,
  synthesize, reorder, renumber, or discard source data. Where the reader deliberately
  disagrees with a companion -- keeping a value the upgrade discards, or declining to
  reproduce something the upgrade synthesizes -- say so in a comment at the site and in
  the research notes, so a later companion survey reads it as an intended difference
  rather than a decoding error.
- Keep corpora read-only and paths private. Use controlled tracked evidence or ignored
  `private/` output according to repository policy.
- Before any recovery-coverage capture, read and follow
  [Full-corpus authorization](../analyze-recovery-coverage/SKILL.md#full-corpus-authorization).
  That section is the sole authority for the capture boundary in this workflow.
- **Report value origin per
  [`research/reference/options_fallback.md`](../../../research/reference/options_fallback.md),**
  which defines `LegacyMus`, `LegacyBehavior`, `Finale27Default`, `Unmapped`, and `MusxOnly`, and
  the rule that a value the pinned baseline already supplies is not asserted again in code. A
  field a later Finale exposes as a setting is often fixed behavior in an earlier one; that is
  `LegacyBehavior`, and finding its introducing version or epoch is part of Step 4.
- **An implemented class is field-complete before it is recovery-complete.** Inventory every
  persisted musxdom field, including leaves of contained objects and every fixed collection
  element, directly from the class and its XML mapping. Give every leaf an initial reader/report
  entry and emit every leaf from the class surveyor. A field is never omitted because its MUS
  source is unknown, because its current value equals a constructor or Finale 27 default, or
  because no controlled fixture changes it. Unknown fields must therefore participate in generic
  companion comparison and produce unexpected differences when their values disagree.

  Which origin each unrecovered field reports — `Unmapped`, `MusxOnly`, or `Finale27Default` — is
  defined in [`research/reference/options_fallback.md`](../../../research/reference/options_fallback.md).
- Keep project-owned source files unity-build clean, per
  [`research/reference/code_conventions.md`](../../../research/reference/code_conventions.md).
  Use the instrumented development build for implementation and coverage work. When the work is
  ready for pull-request delivery, follow
  [`prepare-pull-request`](../prepare-pull-request/SKILL.md) for final local validation and delivery.
- Do not commit, push, or publish unless the user separately requests it.

## Work interactively

At each evidence boundary, tell the user what is known, what remains a hypothesis,
and which smallest new specimen or fact would discriminate between alternatives.
Ask one focused question when new user evidence is genuinely needed. If the existing
evidence supports a safe narrow implementation, continue without asking the user to
reconfirm the implementation request.

**Write the provisional implementation as soon as one epoch's mapping is supported by
bytes, before anything else.** Not after the other epochs are located, not after the
open fields are closed, not after the research notes are written, and above all not
after a corpus survey -- Step 8 exists to refine an implementation and cannot be run
before there is one. A class that has been studied for hours with nothing in `src/` is
the failure this workflow is shaped to prevent, and it does not announce itself: every
individual step looks like diligence.

The trap is that evidence compounds. Each located field makes the next one cheaper, so
gathering never reaches a natural stopping point and the moment to start writing code
never arrives on its own. Treat the first supported epoch as that moment. Ship the
narrow slice with the other epochs reporting synthesized defaults, then widen it. Field
mappings landing in a table one at a time is normal and cheap; a first implementation
deferred until the picture is complete is neither.

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

**Do not re-survey `tests/evidence/` when each fixture is added.** Batch all fixture additions
made during the implementation. Re-survey this tree only when it contains new fixtures, and
only as the final prerequisite immediately before the next `tracked-evidence` probe/report
cycle. If no fixture was added, reuse the current inventory. If no tracked probe/report cycle
will run, do not re-survey merely to refresh the index.

For that final prerequisite, use `./private/regenerate.sh tracked-evidence` where the private
configuration exists, or `scripts/inventory.py` against `tests/evidence/` with the export
convention from `research/data/surveys.csv`, which needs no private configuration at all. Then
update that survey row — the source count, the date, the `tool_commit`, and the
`corpus_fingerprint`, which changes whenever a file is added — and immediately proceed to the
tracked probe/report cycle.

Do this because the fixture tree is a *survey*, not a pile of test inputs, and it is the
only one that is both fully companion-backed and reproducible at all by anyone but its owner.
It is also where the earliest evidence lives: the private corpora begin at Finale 1.8.7, so
every Finale 1.0.0 document with a companion is a tracked fixture. A fixture left out of the
index is invisible to every coverage survey and to every count published from one, which is
exactly how an era comes to look unrepresented when it is not. Keep the fixture cohort in the
scope you agree with the user at Step 8 whenever the class reaches an era the private corpora
cover thinly or not at all.

Nothing about that tree is private, so its conventions still live in
`private/corpora/tracked-evidence.conf` only because that is where `regenerate.sh` looks;
say so rather than treating the file as sensitive.

## Step 1 — Define the vertical slice

Identify:

1. The fully qualified musxdom class and pool (`options`, `others`, `details`, or
   another owner), including comparator, incidence, sharing, and part semantics.
2. Every persisted field and fixed collection element in the musxdom class. List its C++ type,
   default, XML name or contained path, enum meaning, units, bit semantics, whether it is a
   reference, and its initial mapping status. Separately identify the first subset whose source
   recovery will be attempted.
3. The epochs in scope: early pre-banner/Coda, fixed-row uncompressed, DCL, and zlib.
4. The preliminary physical locators and structure hints supplied by the user.
5. The fallback behavior expected when the source lacks the class or a field.
6. What will count as semantic agreement with ETF or modern EnigmaXML.

Start a working evidence matrix, kept private while exploratory, with at least one row per
musxdom field. Add rows for each layout variant as evidence is found; a field with no located
source still has an explicit `unmapped` row; a field known to postdate every supported layout has
an explicit `musx-only` row instead:

| epoch/version | record identity | selector/cmper | incidence | offset/width | transform | target field | mapping status | evidence | confidence |
|---|---|---|---|---|---|---|---|---|---|

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

Registry order is not a contract. Apart from the bootstrap pair the registry calls out, an
importer must build the same document wherever it appears in the list, and entries are kept
alphabetical within each pool so there is nothing to read into their order. An importer that
needs another class's objects -- to resolve a recovered comparator against the pool that owns
its referent, most often -- registers a check on `PendingReferences::checks`, which runs after
every importer. Consulting another pool inline instead makes the class depend on a neighbor
having run first, and nothing verifies that. Anything that must run after every pool is complete, such as
copying reference objects, is a separate phase after the importers rather than an
importer's responsibility. Tables and helpers that only the importer uses should be
file-local; expose a stage separately only where a test needs to drive it alone.

**Give every new reader class importer minimum timing instrumentation in the same change.**
Add one aggregate `timing::Phase` for the importer to `src/reader/timing.h`, give that phase
its stable structured-output name in `src/reader/timing.cpp`, and use it on the importer's
single `FINALE_MUS_READER_IMPORTER` registry row. The registry owns the timed scope, so do not
also time the whole importer inside its class translation unit. This class-level total is the
minimum: add nested phases or counters only when the implementation has a meaningful internal
stage or repeated operation that a later performance investigation may need to distinguish.
Use the existing timing macros for those additions so a configuration without
`FINALE_MUS_READER_ENABLE_INSTRUMENTATION` excludes their storage and measurement code completely.

**`<pool>.h` declares importers and nothing else.** A stage a test drives on its own goes in
`<pool>/test_access.h`, which no library code may include; a stage no test drives is
file-local, in the class's own anonymous namespace. Do not export a stage for symmetry with a
neighboring class -- check who calls it. The distinction is worth keeping mechanical, because
a pool header that mixes the registry's contract with a test seam stops being readable as
either, and an unused export looks like API to whoever reads it next. The same rule applies to
the shared framework header: a helper whose only caller lives in its own translation unit
belongs in that file's anonymous namespace, however general it looks.

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

Before adding source mappings, create the class's complete field manifest in the reader/report
path. Initialize every unresolved source-owned leaf to its musxdom default with `Unmapped`
provenance; for options, retain the seeded value while marking the unresolved leaf `Unmapped`,
subject to the instrumentation compatibility rule above. Mark a field `MusxOnly` only when evidence
establishes that it postdates every supported legacy layout. A later mapping changes only the fields it establishes. For a field
mapped in some layouts, explicitly report `Finale27Default` in layouts where the baseline is the
chosen result. Do not let blanket initialization overwrite `LegacyMus` or `LegacyBehavior`, and do
not promote an untouched field from `Unmapped` merely because its default happens to match a
companion.

For cmper fields, never compare or copy ids across documents as identities. Resolve the
referent on each side and define semantic equality for that class. If fallback requires
cloning a reference object, reuse an equivalent target object when safe; otherwise
allocate the next sequential target cmper and preserve the reference object's original
spelling and data. Define cmper-zero semantics explicitly rather than assuming zero is
an ordinary pooled object.

**Write this class's surveyor in the same pass, not afterward.** Add a
`Value observe<Class>(const SurveyContext&)` function in
`tools/coverage/surveyors/<pool>/<class>.cpp` -- one file per pool directory, matching
`src/import/`'s layout one for one -- and register its pool, key, and observer together with
`COVERAGE_SURVEYOR` at
namespace scope (see `tools/coverage/registry.h`). Emit **every persisted field**, including
every currently unmapped field. The serialized field paths must exhaust the musxdom class surface
established in Step 2; mapping uncertainty changes provenance, not presence. Name a contained
object rather than flattening it: flattening a variant
block loses which variant the record chose. `recovery_coverage_probe` is registry-driven
and self-registering, so nothing else has to change for the new class to appear in every
future corpus run -- but nothing makes the surveyor itself appear automatically either. A
class implemented without one is silently untested across every corpus and every
companion comparison, however thoroughly its own fixtures and unit tests pass, and Step 8
has nothing to run it against. Treat the importer and its surveyor as one unit of work: a
change is not finished until both land together, and a later change that adds, renames,
or reinterprets a field on the importer side updates the surveyor in the same commit --
the two are two views of the same document, and they drift the moment only one of them is
told about a change.

**Make surveyor leaf names mechanical, never editorial.** For an ordinary musxdom member,
the serialized leaf key is the exact snake-case spelling of that C++ member and the origin
key names that exact member after `origin_`. For example,
`drawFinalBarlineOnLastMeas` pairs `draw_final_barline_on_last_meas` with
`origin_drawFinalBarlineOnLastMeas`. Do not expand abbreviations, improve wording, substitute
an XML alias, or otherwise paraphrase either side. `tools/coverage/comparison.cpp` converts the
leaf key back to camel case to locate its origin; a plausible alias silently loses that
association. For contained or synthetic leaves that cannot follow this rule, establish the
explicit pairing in the production comparison model and cover it with a focused test rather
than relying on a naming resemblance.

## Step 6 — Prove the draft on controlled evidence

Add focused tests for:

- an exact comparison between the inventoried musxdom field manifest and the reader/report plus
  surveyor field sets, so adding or overlooking a field cannot silently reduce coverage;
- every initially unresolved or MUSX-only field appearing in probe output with its
  default-initialized or seeded value and the corresponding `Unmapped` or `MusxOnly` status,
  rather than being absent;
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

The leaf/origin name inspection is a prerequisite for **every** recovery-coverage probe cycle,
including `tracked-evidence`. Before invoking the probe, inspect only surveyor source files changed
or untracked relative to the current branch's `HEAD`; unchanged committed surveyors require no
repeat review. In those files, inspect each `field("snake_case", &Target::memberName)` beside its exact
`origin_memberName`; verify the leaf is the mechanical snake-case spelling of that C++ member and
that neither list has an orphan or duplicate. This is a required agent check, not a requirement to
add a focused test. A corpus capture is never the mechanism for discovering or validating a
surveyor spelling.

## Step 7 — Integrate the class into the maintained coverage report

The registry makes a surveyor appear in `recovery_coverage_probe`; it does not make the
class semantically complete in `scripts/recovery_coverage_report.py`. Integrate the class
there before treating corpus comparison as validation:

1. Confirm the class's single `COVERAGE_SURVEYOR` registration names the correct musxdom pool.
   The probe serializes class counts nested under that registered pool, and the Python report
   renders that structure directly. Do not add a second class-to-pool table or registry anywhere.
2. Inspect how the report keys every emitted collection. The generic musx identity is enough
   only when that identity survives a Finale upgrade. If cmpers, incidences, numbering, or
   order can change, align the two sides semantically before recursive leaf comparison,
   resolving each side's referents within its own document. Reuse an existing report
   normalization or referent pairing when it answers the same question; do not create a
   second spelling, text, font, or object-equivalence implementation.
3. If correct alignment needs information the probe does not emit, add an explicit structured
   field to the class surveyor or a cross-pool relationship surveyor. Do not re-derive the fact
   in Python from incidental fields merely because they happen to correlate in current samples.
4. Leave every newly observed difference unclassified until the user has reviewed and explicitly
   approved the proposed interpretation. First present its counts, representative source and
   companion values, origins, epochs, and any pattern that supports an explanation. Do not add or
   broaden an expected-difference rule merely because the importer deliberately retains a default,
   because Finale appears to normalize or synthesize the companion, or to make the report clean;
   those are hypotheses the report exists to expose. After approval, scope the rule by class path,
   category, origin, epoch, and value condition narrowly enough that it cannot hide a recovered-value
   disagreement. Re-run the unmasked comparison whenever a probe metadata defect is fixed, so the
   user reviews the actual differences rather than a classification inferred before the fix.
5. Exercise the maintained report with controlled source/companion observations covering every
   applicable outcome: preserved plus any remapped, normalized, substituted, synthesized,
   removed, or unresolved cases the class is known to have. Add a focused deterministic report
   test when matching or classification code changes; at minimum, run the script on a small
   synthetic or controlled JSONL and inspect the class table and transformation totals.

Run `python3 -m py_compile scripts/recovery_coverage_report.py`. Report integration is complete
only when the class appears under the correct pool and its differences describe semantic recovery
and upgrade behavior rather than raw identifier churn. A disposable analysis script may explore
a hypothesis, but it does not satisfy this step; move the settled rule into the maintained report.

## Step 8 — Refine with class coverage

Once the narrow implementation and its surveyor (Step 5) both exist and pass controlled
fixtures, read and use `../analyze-recovery-coverage/SKILL.md`. **Do not run recovery coverage
before that point.** A survey answers questions about an implementation -- which fields
it recovers, which epochs it fails, where companions disagree with it -- and none of
those questions has a meaning yet if the class exists only as notes, or if it has an
importer but no surveyor to report what the importer did. A survey run early is not a
head start; it is Step 8 performed on nothing, and it has to be run again afterwards
anyway. Start with `tracked-evidence` and iterate there as needed. After all planned tracked
fixtures are present, re-survey them exactly once as the final prerequisite to the first
probe/report cycle that needs those new fixtures; do not re-survey between cycles unless more
fixtures have been added. Define any broader cohort with the user—such as all fixtures, loose only,
ETF-backed, Finale-27-backed, or an explicit union or intersection—then follow
[Full-corpus authorization](../analyze-recovery-coverage/SKILL.md#full-corpus-authorization)
immediately before launching its probe.

**Name the surveys as well as the cohort, and say what each one can and cannot answer.**
When the user authorizes a broader cohort, account for every included survey registered in
`research/data/surveys.csv`; surveys are not interchangeable. The reference corpus has the
companions and the volume but starts at Finale
1.8.7. The installs corpus has no authored documents and thin companion coverage, but it is
the only place several releases exist at all, and the only place the Coda-era Windows
documents that state no version are found — the population any version gate silently drops.
The tracked fixtures are tiny and must never be read as a percentage, but every one has a
companion and they hold the controlled one-variable pairs. A claim about a release absent
from the survey you ran is not a finding; check whether another survey has it before calling
an era covered.

Run `tools/coverage/recovery_coverage_probe` over the chosen cohort, on a corpus TSV that
declares a `#companion:` convention for it (see `recovery_coverage_probe.cpp`'s
`readCorpusRows()`) so every row also surveys its Finale 27 companion, through the same
surveyor written in Step 5. Summarize the JSONL with `scripts/recovery_coverage_report.py`.
Step 7 must already have placed the class in the correct report pool and established any
semantic alignment its identities require. Source and companion still come from the identical
`runAllSurveyors()` JSON shape, so ordinary leaf comparison remains generic; class-aware report
code is reserved for identity alignment, cross-pool equivalence, and characterized upgrade
transformations that the generic comparison cannot express correctly.

Make the targeted survey answer at least:

- Does every persisted musxdom field appear in both source and companion observations, including
  fields whose source mapping is still unknown?
- Does every claimed epoch import the class without failing the document?
- How often is each field unmapped, physically present, recovered, defaulted, unresolved, or
  rejected by a version gate?
- Do companions preserve, remap, normalize, substitute, synthesize, or remove it?
- Are there new payload sizes, incidences, identities, orderings, or version boundaries?
- Do cmper fields resolve to the expected same-side referents?
- Did the implementation need reference objects, and did it introduce duplicates?

Show contradictions and representative private fixture names/paths to the user only in
the local console as allowed by survey policy. Keep tracked findings aggregate and
sanitized. Revise code, tests, confidence labels, and the surveyor together -- the same
"one unit of work" rule from Step 5 applies to every later revision, not only the first
draft -- then rerun the smallest cohort that exercises the change before rerunning the
broader survey.

Coverage review is an evidence boundary. Stop after presenting newly observed differences
and wait for the user's interpretation before classifying any of them as expected. An existing
approved rule may continue to classify the behavior it already describes, but a new class must
not be fitted to those rules by analogy without review.

Before calling the work complete, offer the regression over every registered survey and apply
[Full-corpus authorization](../analyze-recovery-coverage/SKILL.md#full-corpus-authorization).
If the capture is run, present the per-survey import table from
`recovery_coverage_report.py`'s output. Otherwise report that broad regression was not performed
and base the handoff on tracked evidence and ordinary tests. The regression is only as wide as its
surveyor list, and a class recovering correctly for its own fixtures can still break another class
for every document in a corpus -- a shared decoder touched along the way (the text encoding, a font
lookup, the record index) is exactly what an authorized full-survey run can catch and what per-class
tests cannot.

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

Before calling the implementation complete, update `research/state/MUSXDOM_CLASS_COVERAGE.md`:
mark the class `[x]` when complete or `[~]` when partial, name its importer file, and adjust both
the pool heading and whole-file totals. Keep explanation out of the checklist; its own preamble
defines the terse format and routes findings to the research notes.

Record the class's durable findings in `research/format/<pool>/<name>.md` and the investigation
behind them in `research/investigations/<name>.md`, using the `maintain-documentation` skill.
Remove any question it answered from `research/STATE.md`.

After the user has approved the completed work and its pull request, identify the exact transient analysis artifacts that are unlikely to help with the next musxdom class, prompt for explicit approval, and clean up only the approved artifacts. Preserve reusable corpus inventories, path and companion mappings, and expensive archive caches.

Finish with a concise account of the implemented class and fields, supported epochs and
version gates, controlled fixtures and tests, corpus coverage, synthesized fallback,
known upgrade variances, every still-unmapped field, every MUSX-only field, remaining open layouts, and the next
smallest evidence request.
State whether the full regression in Step 8 was run under
[Full-corpus authorization](../analyze-recovery-coverage/SKILL.md#full-corpus-authorization),
and include its per-survey import table only when it was run.
Call the result partial whenever any epoch or field remains unsupported.

**State every completely uncovered epoch explicitly, every time.** Name the epoch, say
whether the exclusion is intended or merely unevidenced, and give the reason. This is not
covered by the per-field coverage account: a class can look thoroughly implemented, with
high field counts and clean companion agreement, while one whole era recovers nothing at
all. Saying "supported for X, Y and Z" leaves the reader to notice that W is missing.
Say that W is uncovered.
