---
name: maintain-documentation
description: Rules for changing anything under research/. Use when adding a format finding, recording an experiment, updating project status, creating or splitting a research document, repointing links, or reorganizing documentation. Preserves the progressive-disclosure structure that keeps automatic session context small while leaving the full research history retrievable.
---

# Maintain the documentation

The documentation is a hierarchy: a tiny startup layer, topical reference files, and an evidence
and history archive. It is organized for retrieval, not for reading through. These rules keep it
that way. `research/INDEX.md` and `research/ORIENTATION.md` describe the structure itself; do not
restate the structure here.

## 1. The startup budget is fixed

The startup set is `AGENTS.md`, `research/ORIENTATION.md`, `research/STATE.md`, and
`research/INDEX.md`. **Combined ceiling: 4,000 words**, enforced by
`python3 scripts/check_doc_budget.py`. Exceeding it means move material down into the tree. It
never means raising the ceiling.

## 2. Every fact has one destination

| What you have | Where it goes |
|---|---|
| A durable format fact about a class | `research/format/<pool>/<name>.md` — named after `src/import/<pool>/<name>.cpp` |
| A durable fact about the container, framing, or encoding | `research/format/container/<subject>.md` |
| How it was established: method, bytes, counts, refuted predictions | `research/investigations/<topic>.md` |
| A status, priority, blocker, or open question | one linked line in `research/STATE.md`, detail in `research/state/` |
| A binding project rule | `research/reference/` |

Nothing else belongs in a startup file. Startup files carry durable facts and navigation, never
evidence.

## 3. State a fact once

Every fact, rule, and format finding has exactly one home. **A second copy is a defect even when
both copies are currently correct**, because the two diverge silently — nobody adding a sentence
to one copy thinks of themselves as duplicating a fact. This is `AGENTS.md`'s code rule applied to
prose, and prose needs it stated more firmly: copying a paragraph meets none of the resistance
that copying code does, and reads as helpful. Duplication is not paid for by being convenient, by
being small, or by the copies being in different trees — distance makes it worse, because the
copies are never read together.

**Before writing a paragraph that explains a rule or states a format fact, grep for a distinctive
phrase from it.** If it exists already, link instead. This is the counterpart of "before adding a
literal, grep for it," and it is the step that actually catches the problem.
`python3 scripts/check_doc_duplication.py --since main` does it systematically for a whole change.
It is advisory: a conclusion and the experiment behind it legitimately share wording, so read its
output rather than obeying it.

Elsewhere, name the fact and link to it. An index says what a file contains and when to read it;
it never reproduces the file. A redirect stub holds no content. An exception needs its
justification stated on the line, naming why one home was not possible.

The standing example: the gate-selection rules sat in both `AGENTS.md` and
`implement-musxdom-class` until each had acquired sentences the other lacked — one naming the
clef-tuple and stem-collection markers, the other requiring a marker be proved against the corpus.
Both were right, neither complete, and nothing had gone visibly wrong.

## 4. Consolidate; do not append

A new observation about a field that already has a paragraph **updates that paragraph**. Session
transcripts never go into documentation. A dated experiment entry goes in the topical
investigation file for its subject, not into a growing chronological log.

## 5. Preserve uncertainty

Use the project vocabulary: `confirmed`, `strong`, `weak`, `open`, `superseded`, plus
`public-PDK-derived`, `private-framework-derived`, `independently binary-verified`. Never promote
a label to make a document read better. Cite evidence per
`research/reference/CITING_EVIDENCE.md`.

## 6. Preserve failure

A refuted hypothesis is kept — in its investigation file with the refutation, or in
`research/history/FAILED_HYPOTHESES.md` if it is a broad structural dead end. It exists so an
expensive experiment is not repeated. Deleting one is a defect.

## 7. Mark; do not contradict

Superseded material is labeled `superseded` with a pointer to what replaced it, or moved to
`research/history/`. **Do not silently resolve a conflict between two claims.** Preserve the
conflict, state it as a conflict, and surface it in `research/STATE.md` until it is investigated.

## 8. Update the index in the same change

A file `research/INDEX.md` does not name is invisible. A new investigation file also gets a line
in `research/investigations/index.md`.

## 9. Retire from STATE

When an open question is answered, the answer moves into `research/format/`, its evidence into
`research/investigations/`, and the bullet leaves `research/STATE.md`. `STATE.md` describes what
is live, not what has been.

## Checklist before finishing a documentation change

- [ ] `python3 scripts/check_doc_budget.py` passes.
- [ ] `python3 scripts/check_doc_links.py` passes.
- [ ] Each new fact has exactly one home; everywhere else links to it. You grepped a distinctive
      phrase from every new explanatory paragraph, or ran
      `python3 scripts/check_doc_duplication.py --since <base>` and read the result.
- [ ] Every new or moved file has its **Covers / Read when / Confidence** header.
- [ ] `research/INDEX.md` (and `investigations/index.md`) name every file you added.
- [ ] Every claim carries a confidence label and its evidence is citable.
- [ ] Resolved items removed from `research/STATE.md`; superseded items marked or moved.
- [ ] No conflict was resolved by deletion.
