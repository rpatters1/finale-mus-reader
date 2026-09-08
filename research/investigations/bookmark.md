# Bookmark investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-09-07 — The `BK` layout, read from the ETF and confirmed against the companion

- **Question:** the user supplied a 36-word structure for the `BK` record. Do the bytes support it,
  and can the name be recovered well enough to match a companion in the eras that predate the
  text-pool form?
- **Result — supported in full, from a source that prints the record directly.**
  `F372-bookmarks.etf` spells both bookmarks of `F372-bookmarks.mus` as six incidences of six
  words: four of characters, then `1 100 0 0 0 0` and `1 1 0 0 0 0` for the scroll-view bookmark,
  `0 100 0 0 0 0` and `1 -80 -80 3 0 0` for the page-view one. Every word the hint names lands
  where it says. The Finale 27 companion supplies the semantics from the other side, and the
  `3` of the second record is `<changeHorz/><changeVert/>` — the two flag bits, in that order.
  The layout is in [`bookmark.md`](../format/others/bookmark.md).
- **Refuted — "the string occupies the first four incidences as 48 bytes ending at the first NUL,
  and two numeric incidences follow it" understated the numeric half.** That reading, recorded on
  2026-08-18 when only the shape was wanted, is right about the string and treats the remaining
  twelve words as opaque. They are not: they are the whole view state, and eight of them carry
  values the companion names.
- **The recovered names match their companions exactly**, characters and comparators alike, in
  both the `BK` era and the pooled one. `bookmark_texts[number=1]` of the Finale 3.7.2 fixture is
  `Scroll «» Bookmark` and its companion's bookmark 32768 names `nameRawTextID` 1; the u-umlaut
  and the guillemets survive the Mac Roman conversion and read identically to the UTF-8 spelling
  the Finale 2012 fixture stores.
- **The first draft's only difference was a font state the reader invented, and it was removed.**
  Giving the name the document's `TextBlock` font, as File Info takes it, recovered
  `^font(Times)^size(12)^nfx(0)Page über` where the companion has `Page über`, and accounted for
  all twelve classified `bookmark_texts` differences over the tracked cohort — four each of font,
  size and effects across four texts. The pooled Finale 2012 path had behaved that way since it
  was written; the `BK` path reproduced it rather than introducing it.
- **Nothing in any document asked for that prefix.** Inflating the Finale 2012 text pool shows
  its whole 101 bytes as `^block(1)^«85»…^«86»…^«84»…Score^end^bookmark(2)Page über^end`
  `^bookmark(3)Scroll «» Bookmark^end`: the `^block` chunk carries three binary font commands and
  the two `^bookmark` chunks carry none. The `BK` record holds a bare string, and Finale 27 writes
  no font either. Both paths now store the characters alone, and `bookmark_texts` reports 8 leaves
  compared, 8 the same, and no difference of any kind in either era.
- **Open at the time — the view percent and its enable flag.** Words 25 and 26 held `100` and
  clear in both specimens while both companions carried `<viewPerc>100</viewPerc>`, which was
  consistent with more than one conversion. **Closed the same day** by the commissioned fixtures
  below.
- **Observation — the tracked and private controlled corpora hold exactly two documents with
  bookmarks**, one per era, so the DCL epoch has no specimen at all even though it uses the same
  tagged others pool.
- **Artifacts:** `src/import/others/bookmark.cpp`, `tests/classes/texts/text_pool_tests.cpp`,
  [`bookmark.md`](../format/others/bookmark.md),
  [`bookmark_text.md`](../format/texts/bookmark_text.md#bookmarks).

## 2026-09-07 — Two commissioned fixtures close the record

- **Question:** the fields the first pass could not separate — words 31 and 32, the two flag bits,
  and the percent pair — plus the Finale 2007-2011 window the all-corpus run showed the reader
  missing in four documents.
- **Result — all of them, from `F2008-bookmarks.mus` and `F2001Win-bookmarks.mus`.** The layout is
  now confirmed field by field in [`bookmark.md`](../format/others/bookmark.md), and the fields the
  earlier note called `open` are closed there rather than repeated here.
- **The zlib bookmark is class `0x007b`, and it still carries its name.** The Finale 2008 record is
  72 bytes: the same 48-byte name followed by the same twelve words. The Finale 2012 record of the
  same class is 24 bytes and holds only those twelve, with word 27 — previously unused — carrying
  the text-pool comparator, matching that fixture's companion `nameRawTextID` of 3 and 2. So the
  reader selects the form by record length rather than by version, and the move into the text pool
  is Finale 2012's rather than the zlib era's.
- **Refuted — the first draft's claim that the F372 companion "identifies the two flag bits."** It
  does not. That fixture stores `-80` in both words 31 and 32 and sets both bits together, so
  neither the word-to-field assignment nor the bit-to-word assignment could be read from it; both
  came from the user's structure hint. `F2008-bookmarks.mus` was built to break the symmetry and
  now supports each independently.
- **Observation — the reader is right and the companion is wrong, on one fixture.** The Windows
  fixture's Windows-1252 guillemets and a-ring were re-read as Mac Roman by Finale 27 on macOS,
  which wrote `Scroll ´ª 9, 3` and `PÂge Bookmark`. The comparison's existing
  `wrongPlatformEncodingGlitch` rule catches it; no rule was added or widened for it.
- **Coverage after the change, over the tracked cohort:** `bookmark_texts` compares 24 leaves
  across twelve texts in four documents — 22 identical, 2 classified as that encoding glitch, none
  unexpected. All twelve texts are recovered, where six of them recovered nothing before.
- **Confirmed over all four surveys.** A second authorized all-corpus capture across 16,340 sources
  reports `bookmark_texts` as 42 leaves over 21 texts in nine documents: 39 identical, 3 classified,
  none unexpected, and **no companion-only leaf**. The four Finale 2008 and Finale 2011 documents
  that the first capture showed recovering nothing now recover every bookmark, which is what
  establishes that selecting the form by record length generalizes off the single fixture built for
  it. `ALL POOLS` reports no unexpected difference across 182,778,590 leaves.
- **One new classified difference, and it is intended.** A Finale 2008 bookmark of the reference
  corpus is named with a trailing line break, which the reader keeps and Finale 27 drops. Trimming
  it was considered and rejected: the reader upgrades what the source stores absent a reason not to.
  Recorded once, with line-break handling, in
  [`text_encoding.md`](../format/container/text_encoding.md#encoding).
- **Artifacts:** `src/import/others/bookmark.cpp`, `tests/classes/texts/text_pool_tests.cpp`,
  `tests/evidence/F2008/provenance.txt`, `tests/evidence/F2001/provenance.txt`,
  [`bookmark.md`](../format/others/bookmark.md).
