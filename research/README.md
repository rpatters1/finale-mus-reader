# Legacy Finale MUS Feasibility Study

This is a public-source, independently verified exploratory study of legacy Finale `.mus` files. It assesses whether a future reader could populate the existing musxdom model; it is not a reader implementation.

Three corpora have been surveyed, all registered in [`data/surveys.csv`](data/surveys.csv).

`rpatters1-main` is the reference corpus of authored documents: 1,544 loose legacy files, 1,367 of them distinct by content, plus 2,437 archived occurrences. It has 2,044 Finale 27 exports under its primary convention: exact adjacent `-exports/<name>.fin27.musx` counterparts for 1,193 loose files. Fifteen more classic-Mac documents have exact `-finale27/<name>.musx` companions under the fallback convention used by recovery coverage, leaving 336 loose files without a companion. Pairing is by path alone: an earlier run also matched by basename search, and every one of those pairings turned out to be a guess rather than a pair. Finale 27 exports are semantic references, not byte-for-byte representations: conversion changes the modified header, normalizes data, and can synthesize or expand records.

`rpatters1-installs` is Finale application installations for macOS and Windows, 12,116 files across products 1.0.0 through 2011, only 4,536 of them distinct: the same shipped content recurs in every version directory by design. It now has exports and the semantic half runs, with 3,090 adjacent-exact companions. It contributes version coverage more than converted references, holding the only Finale 3.8, 98 and 99 material in any survey. A quarter of its files carry no extension, because classic Mac Finale kept the file type in the resource fork. Its documents are Finale's own samples and templates, converted into a release rather than authored in it, so they show what a release could open and not what it wrote for a new document.

`tracked-evidence` is the controlled fixture set committed to this repository, surveyed as a corpus in its own right: 173 files, every one of them paired adjacent-exact with a Finale 27 companion. It is tiny and must never be read as a percentage, but it supplies three things the other two cannot. Both of them begin at Finale 1.8.7, so its 38 Finale 1.0.0 fixtures are the **only companion-backed documents of that version in any survey**; it holds the controlled one-variable edit pairs, which an authored corpus cannot produce; and it holds documents written to exercise one feature deliberately, which is how the text inserts, the Coda-banner text families and the bookmark eras were decoded — none of them appears in an ordinary score often enough for any corpus to settle. It is also the **only** survey anyone can reproduce: the corpus ships with the repository, so `scripts/inventory.py` run against `tests/evidence/` re-derives this row from a clone with no private configuration. The other two describe private drives and can be re-run only by their owner, so their numbers must be taken on trust in a way this one's need not be.

## Corpus identifiers

Published findings identify each source by a stable content-derived `corpus_id`, such as `mus-65aa1de01997b781`, plus its size and SHA-256. Filenames, paths, and drive names are never published: a filename can name a work, a client, or a person, and the hash identifies the evidence better. This permits future maintainers to recognize the same evidence without publishing anything about where it lives or what it is called. The public manifest is
[corpora/rpatters1-main/data/corpus_manifest.csv](corpora/rpatters1-main/data/corpus_manifest.csv), one directory per
surveyed corpus; [data/surveys.csv](data/surveys.csv) registers them.

To survey your own corpus, paste [SURVEY_PROMPT.md](SURVEY_PROMPT.md) into a coding agent; the procedure it follows
is [`.agents/skills/inventory-a-corpus/SKILL.md`](../.agents/skills/inventory-a-corpus/SKILL.md), and
[REPRODUCING_THE_SURVEY.md](REPRODUCING_THE_SURVEY.md) gives the underlying commands.

For StuffIt archives, install the `unar` package so that both `unar` and `lsar` are available. Use `lsar` for a non-destructive member listing and `unar -o <temporary-directory> <archive>` for extraction; never extract over the source corpus.

The evidence set includes controlled MUS/ETF pairs for Finale 2002–2005 under `tests/evidence/F2002/` through
`tests/evidence/F2005/`. There are now fifteen ETF exports plus eight controlled-test MUS files in total. The public notes record
provenance and hashes.

Finale 27 opened three targeted ETF-backed 1.8.7, 2.0.1, and 2.6 sources after `.mus` was appended to each
extensionless filename; the 2.6 document had non-blocking font
issues. These conversions disprove the suspected 2.6.x parser cutoff for the tested files while demonstrating that
classic Mac type/creator discovery and modern filename-extension recognition are separate from format support.

## Terminology note: "pre-banner"

Earlier survey notes call the Finale 1.x-2.6 family "pre-banner". That is inaccurate and the term is
retained only where it records what was observed at the time. Those files do carry a banner, a
plain-text `Finale(TM) 2.6 Copyright 1987 by Coda.` product string at offset 0; what they lack is the
`ENIGMA BINARY FILE` signature. The reader calls the era `CodaBanner`, and the corrected structural
description is in [FORMAT_NOTES.md](FORMAT_NOTES.md#coda-banner-files).

## Public-source provenance policy

The investigation began as a strict clean-room study. On 2026-08-08, the project broadened that boundary to permit
consulting historically public copies of Finale PDK material for factual format information. PDK-derived findings must:

- cite an immutable public URL and access date;
- be labeled `public-PDK-derived` until independently checked;
- record only facts needed for interoperability, such as identifiers, sizes, field order, and flag meanings;
- be restated in the project's own terminology rather than copying declarations, comments, or implementation code;
- be checked against locally owned MUS, ETF, or MUSX evidence wherever practical; and
- become `independently binary-verified` only when the corpus confirms them.

No PDK source or header is stored in this repository. This is a public-source provenance boundary, not a claim of
strict clean-room isolation. Corpus-derived conclusions recorded before the boundary change retain their original
provenance.

## Current conclusion

**Feasible with substantial reverse engineering.** Finale 3.0–2012 is now tractable at the container and physical-
record layers. Finale 3.x–2000 uses four uncompressed typed pools; Finale 2001–2006 wraps the same fixed physical rows
in big-endian PKWARE Data Compression Library (DCL) blocks with CRC-32 checks; Finale 2007–2012 uses typed zlib
blocks and a later variable record frame. Mark Adler's open-source `blast` decompressor
successfully decoded all 1,603 candidate compressed members encountered in the 2001–2006 corpus, and every decoded
result matched its stored CRC-32. Of the files tested, 410 traverse cleanly as complete typed-block sequences; some
additional files yield valid leading DCL members before their outer framing stops matching the current probe. Public
Finale 2000 PDK facts plus independent corpus checks establish fixed 16-byte other/detail rows and 38-byte entry rows
from Finale 3.0 through Finale 2006; the former “16-word” hypothesis was a byte/word unit error. The exact Finale 2000
`mus-3a8b724cf3adba80` MUS/ETF pair additionally proves one-for-one pool counts, ordered ordinary tags, selected detail and entry
values, and byte-identical text after removing ETF section separators. Tag-specific fields, later entries, options,
and sharing remain incomplete. Finale 1.8.7–2.6 is now known to share the 16-byte logical-record cadence, exact
32-byte entry bodies, tag vocabulary, and raw text with Finale 3.0, but its index/directory spans and generic pool
boundaries remain unresolved. A universal reader is not yet justified, but a version/format-era strategy is.

## Reproduction

The survey workflow and commands are documented in
[REPRODUCING_THE_SURVEY.md](REPRODUCING_THE_SURVEY.md).

## Research documents

- [CORPUS_INVENTORY.md](corpora/rpatters1-main/CORPUS_INVENTORY.md): all examined files, sizes, hashes, header products, and counterpart matches.
- [ARCHIVE_SURVEY.md](corpora/rpatters1-main/ARCHIVE_SURVEY.md): archive and extensionless-member findings, including the Finale 2.6 samples.
- [FORMAT_NOTES.md](FORMAT_NOTES.md): headers, format eras, blocks, record framing, entries, text, options, and sharing.
- [LEGACY_OPTION_MAPPINGS.md](LEGACY_OPTION_MAPPINGS.md): distilled legacy global-to-option mappings, provenance, confidence, and validation plan.
- [RECORD_CATALOG.md](corpora/rpatters1-main/RECORD_CATALOG.md): every numeric record identifier observed in successfully framed 2007+ blocks.
- [VERSION_MATRIX.md](VERSION_MATRIX.md): corpus versions and proposed format eras.
- [EVIDENCE_REQUESTS.md](EVIDENCE_REQUESTS.md): precise ETF and controlled-difference requests.
- [EXPERIMENT_LOG.md](EXPERIMENT_LOG.md): commands, observations, failed hypotheses, and follow-ups.
- [FEASIBILITY_ASSESSMENT.md](FEASIBILITY_ASSESSMENT.md): direct recommendation, risks, architecture, and next steps.
- [PRODUCTION_READINESS.md](PRODUCTION_READINESS.md): prioritized blockers and gaps between the current vertical slice and a production importer.
- [REPRODUCING_THE_SURVEY.md](REPRODUCING_THE_SURVEY.md): corpus mapping conventions and reproducible commands.

Public references used in the initial clean-room search include Mark Adler's permissively licensed `blast` decoder, the
Library of Congress description of legacy MUS and ETF,
Finale's historical help/glossary stating that ETF creation ended after Finale 2006, and the independent LilyPond
`etf2ly` ETF subset reader. `blast` supplies the independently verified 2001–2006 payload decoder; the other sources
document ETF's role and grammar but do not describe the MUS container:
[`blast` source](https://github.com/madler/zlib/tree/master/contrib/blast),
[`blast.h` format/API notes](https://github.com/madler/zlib/blob/master/contrib/blast/blast.h),
[Library of Congress MUS description](https://www.loc.gov/preservation/digital/formats/fdd/fdd000632.shtml),
[Finale ETF glossary](https://finale.jetzt/finalehelp/Finale26Win/Content/Finale/glossary.htm),
[`etf2ly` manual](https://manpages.ubuntu.com/manpages/stable/man1/etf2ly.1.html), and
[historical Finale format notes](https://preservation.tylerthorsted.com/2024/02/09/finale/).

The later public-PDK-informed phase uses the Finale 2000 PDK copy exposed in GRAME's GUIDOLib repository at immutable
commit `9f74ba9b3e287f240bbd454c2259fc3f7737c6ad`. Its
[`FinalePlugin-ReadMe.txt`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/FinalePlugin-ReadMe.txt)
identifies the included version. Exact consulted header links and the independently verified findings are recorded
in [FORMAT_NOTES.md](FORMAT_NOTES.md).
