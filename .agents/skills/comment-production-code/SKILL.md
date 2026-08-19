---
name: comment-production-code
description: Write or revise comments in this repository's production C++ (src/, include/, tools/coverage/). Use when adding a decoder, reviewing a diff for comment quality, or cleaning up comments that have drifted into narrating how a finding was made. Covers what a production comment may state, how to label a claim this repository is not certain of, and where derivation history belongs instead.
---

# Comment production code

A production comment states **how the code works**, and **how we believe it ought
to work** where belief is all we have. Nothing else.

This applies to `src/`, `include/`, and `tools/coverage/`. It does not apply to
the rest of `tests/`, `tools/`, or `scripts/`, and it does not apply to the
research documents, which exist precisely to hold what this rule keeps out of
the source.

`tools/coverage/` is the one probe meant to stay comprehensible and
regression-safe over time rather than rewritten on demand the way the rest of
`tools/` is, so it earns the same comment discipline as `src/`/`include/` even
though — unlike them — its code is still free to repeat itself, per `AGENTS.md`.

## The rule

A comment in production code may contain:

- **Behavior.** What this code does, in terms a reader of the code can check
  against the code.
- **Mechanism.** How the format works, where that is what makes the code make
  sense — a layout, an encoding, an ordering constraint, a lifetime.
- **Belief, labelled as belief.** A theory about the format that is untested or
  only partly established, stated as a theory and marked with how far it is
  trusted. This repository decodes an undocumented format; a comment that admits
  uncertainty is worth more than one that hides it.
- **Consequence.** What breaks if the code is changed in an obvious-looking way,
  when the reason is not visible locally.

A comment in production code may **not** contain:

- **How the behavior was derived.** Which fixture established it, which
  companion corroborated it, which survey it came out of, how many documents
  agreed. This is the single most common drift, and it is what the research
  documents are for.
- **What was believed before.** A refuted prediction, a hypothesis that did not
  hold, "this turned out not to be", "the first attempt did". A reader needs the
  conclusion, not the path.
- **Project narrative.** When something was added, what it replaced, which
  release or conversation prompted it, what is planned next.
- **Self-reference.** "this reader", "we", "I", and the like, narrating the
  author's process rather than describing the code.

## Confidence labels

Where a statement about the format is not certain, say so in the comment and say
how uncertain. Use one of these, consistently:

| Label | Meaning |
|---|---|
| *(unlabelled)* | Established. The code depends on it and nothing known contradicts it. |
| **Believed** | Consistent with everything seen, with no case that would distinguish it from a near alternative. |
| **Unverified** | A theory that fits, with nothing yet confirming it. Recorded so that a document which contradicts it is recognized as contradicting something. |

An unverified theory earns its place only when it would change what a reader
does — most often as a tripwire, where the code deliberately fails loudly rather
than guessing, and the comment says what a future observation would settle.
Never let an unverified theory read as fact, and never act on one in code that
could instead report the gap.

## Where the excluded material goes

Nothing is deleted; it moves.

| Material | Home |
|---|---|
| How a mapping was established, and from what | `research/FORMAT_NOTES.md` |
| What a specific fixture proves, and what it cost | `tests/evidence/<era>/provenance.txt` |
| The investigation itself, including refuted predictions | `research/EXPERIMENT_LOG.md` |
| What evidence is still wanted | `research/EVIDENCE_REQUESTS.md` |

Before deleting a sentence from a comment, check it survives somewhere in that
table. A refuted prediction is worth keeping — in the log, where the next person
tempted by the same prediction will find it.

## Doxygen

`AGENTS.md` already governs Doxygen: it documents the contract, not its history.
Everything above applies to Doxygen as well, and Doxygen additionally excludes
mechanism a caller does not need. A caller arriving at the published API has none
of this repository's context.

## Revising an existing comment

Work sentence by sentence, not comment by comment. Most comments that violate
this rule are half compliant: a correct statement of mechanism followed by a
sentence naming the fixture that established it. Keep the first, move the second.

Two checks before finishing:

1. **Would this sentence still be true and useful to someone who has never seen
   this repository's research documents?** If it only makes sense as a reference
   to work that happened here, it belongs in those documents.
2. **Does the comment claim more confidence than the evidence supports?**
   Stripping the derivation makes a hedged claim read as fact. Where that
   happens, add the label rather than restoring the history.
