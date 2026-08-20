# Survey Your Own Corpus

If you have a collection of legacy Finale `.mus` files, you can inventory it and
contribute the findings back. The work is done by a coding agent: you paste the
prompt below, answer a few questions about how your files are organized, and the
agent runs the survey.

**The survey is meant to be contributed, and that is the default.** The point of
this project is to accumulate evidence about a format whose files are scattered
across private collections that no one else will ever see. A survey that stays on
your disk helps you; a contributed one is what makes the format readable for
everyone. The agent will prepare a contribution unless you tell it not to, and it
will show you everything before anything leaves your machine.

You can decline. Say so when the agent asks and it will run the survey locally
and stop. Nothing is pushed without your say-so either way.

Nothing about your filesystem is published. The survey identifies every file by
a hash of its contents, and the mapping back to real paths stays in an ignored
local directory. See [What gets shared](#what-gets-shared) below for the specifics.

## Before you start

1. **Clone the repository** and open your agent in the clone.
2. **Python 3.10 or newer** on `PATH`.
3. **`unar` and `lsar`**, only if your corpus contains `.zip` or `.sit`
   archives you want scanned (`brew install unar`).
4. **Know where your corpus is.** The agent will ask; it does not go looking.
5. **Expect it to take a while.** A corpus of a few thousand files hashes in
   minutes. The archive pass is much slower and is optional.

You do not need to know anything about the file format. You do need to answer
questions about your own filing conventions, because nothing in the repository
can infer them.

## What gets shared

Contributed, if you agree:

- One row per file: a content-derived ID, size, SHA-256, and the Finale version
  string read from the file's header.
- Structural observations **totalled across the corpus**, not file by file —
  which record identifiers appear in which Finale versions, how compression is
  used, how often each record type occurs. No bytes of your musical content,
  ever.
- Findings written into the shared research notes, citing your files by their
  content hash so a later reader can weigh the evidence without holding it.

Never contributed:

- Your `.mus` files themselves.
- **A per-file profile of your work.** Record types, size distributions, and
  element counts for each individual file were considered and rejected: totalled
  across a corpus they describe a format, but file by file they describe a
  composer's output. Aggregates answer the format questions.
- **Filenames.** Not the file's name, not an archive's name, not a member's
  name. A filename can name a work or a client, and the content hash identifies
  the evidence better anyway.
- Absolute paths, drive names, your account name, or your directory layout.
  These stay in `private/generated/`, which git ignores.

Also true regardless:

- The corpus is only ever read. No script writes inside it, renames anything, or
  extracts an archive over it.
- The agent runs a check for leaked paths before anything is committed, and you
  see the diff before it goes anywhere.
- Nothing is pushed or opened as a pull request without you asking.

Optional, and only if you say yes:

- **Fixture files you wrote yourself**, donated under this repository's license.

Your files' header bytes are **never** published, not even with text blanked
out. Finale stores the title, composer, and description there, and the header's
remaining fields are not yet mapped well enough to strip metadata by field
rather than by guesswork.

**Only contribute what you have the right to share.** The survey publishes no
names and no file contents, so an ordinary contribution reveals nothing about
what your music is or who it is for. Donated fixtures are the exception, since
those are whole files: never donate one you did not write.

## The prompt

Copy everything below the line into your agent.

---

I have a collection of legacy Finale `.mus` files that I would like to inventory
using this repository's survey pipeline, and I may contribute the results back.

Please run the survey for me. Before doing anything else, read `AGENTS.md` and
then `.agents/skills/inventory-a-corpus/SKILL.md`, and follow that skill — it is the
procedure for this task, and it covers details that are not repeated here.

Work in this order:

1. **Ask me about my corpus before running anything.** You cannot infer these
   from the code, and guessing produces a survey that quietly reports nothing:
   - where the corpus is;
   - whether I have modern Finale re-saves of these files, and if so, what
     directory they live in relative to each source and what suffix they use;
   - whether I want the `.zip`/`.sit` archive pass, which is the slow one.

   Assume I intend to contribute the results upstream, since that is the point
   of the exercise, but tell me plainly what would be published and give me a
   clear chance to decline. Do not treat my silence as consent for the push
   itself.

   Ask these as a short batch, not one at a time. If an answer is ambiguous,
   check a couple of real paths in my corpus and confirm what you inferred
   rather than assuming.

2. **Check prerequisites** and tell me if something is missing before starting a
   long run rather than failing partway through.

3. **Run the pipeline** as the skill describes, writing path-bearing output to
   `private/generated/` and published output under
   `research/corpora/<survey_id>/`. Register the survey in
   `research/data/surveys.csv`. Warn me before the archive pass and give me a
   rough time estimate.

4. **Verify before publishing.** Run the leak check from the skill. Confirm the
   file counts are plausible and tell me if the export match rate looks wrong —
   a sudden collapse to zero matches almost always means a mistaken suffix
   rather than a corpus without exports.

5. **Tell me what my corpus says about the format**, not just how many files it
   has. Read `research/CITING_EVIDENCE.md`, then compare my results against the
   existing research notes and propose citing edits: claims my files corroborate,
   claims they contradict, Finale versions no previous survey covered, and files
   whose structure still does not parse. Contradictions and unrepresented
   versions are the most valuable things I can contribute, so surface them
   prominently rather than burying them. Never quietly rewrite an existing
   finding — record disagreement beside it.

6. **Show me the results**, including the proposed note changes, and let me
   decide what to include before anything is committed.

7. **Ask whether I want to donate any fixtures**, but only after the survey
   works, and only for files I plausibly wrote myself. Explain that a committed
   file is checkable by everyone forever, unlike statistics about a corpus only
   I can see, and that two saves of one short piece differing by exactly one
   change are the most useful thing of all. Take no file without asking about
   authorship and licensing first.

Constraints, which matter more than speed:

- **Never write inside my corpus.** Read-only, always. Extract archives to a
  temporary directory, never over the source.
- **Never publish an absolute path**, a drive name, or my account name.
- **Do not edit another survey's files** under `research/corpora/`. If my
  results disagree with an existing survey about the same file, that is a
  finding worth reporting, not a conflict to resolve by editing.
- **Do not modify the scripts** to hardcode my layout. They take every path as
  an argument on purpose; my conventions belong in the invocation.
- **Commit on a branch, never on `main`**, and do not push or open a pull
  request unless I ask.
- **Report what you actually observed.** Legacy MUS is a family of formats, and
  my corpus probably covers only some of it. Do not generalize a finding from
  one era to the whole corpus, and do not present a weak filename-based export
  match as an exact one.

If something is genuinely ambiguous, ask me. If something fails, show me the
actual error rather than working around it silently.
