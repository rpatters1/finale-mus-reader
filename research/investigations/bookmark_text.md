# BookmarkText investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-18 — Bookmarks, and two deferrals stated as deferrals

- **Question:** `BookmarkText` was the one text class with no located source. Does Finale 2012 pool it, and is the
  pooled text Unicode?
- **Result — yes to both.** From Finale 2012 a bookmark is an ordinary text-pool record, keyword `bookmark`,
  `^end`-terminated and carrying no style commands. Its text is UTF-8, shown rather than inferred: `c3 bc` is the
  u-umlaut of `Page über` and `c2 ab c2 bb` the guillemet pair of `Scroll «» Bookmark`. The same characters are
  one byte each in every earlier era, which is what makes it a measurement — the guillemets settled the lyric
  punctuation code page the same way, as `c7 c8` in Mac Roman.
- **Before that it is the `BK` others family**, comparators from 0x8000 up, in the same shape the Coda era uses
  for block text: 48 bytes of string across four incidences, then two numeric incidences. The text pool of such a
  document holds no bookmark at all.
- **Two deferrals, both asserted rather than assumed.** `BK` is not read until the bookmark class is imported, and
  the `DT` expression text of the fixed-row eras is not read until `TextExpressionDef` is. In both cases the text
  without the class behind it would claim more coverage than it has. The synthesis that existed for `DT` was
  removed rather than switched off, and tests assert that both eras produce nothing, so reinstating either is a
  deliberate act.
- **Unverified: that the move into the pool belongs to the Unicode project.** It fits, but the boundary has not
  been tested inside Finale 2012, and a point release may have changed it. Nothing turns on it while the reader
  takes whichever form the document presents — a missing answer is possible, a wrong one is not.
- **Observation — comparators are not stable across an upgrade.** The same two bookmarks are 1 and 2 before and
  2 and 3 after.
- **Artifacts:** `src/import/texts/text_pool.cpp`, `tests/text_pool_tests.cpp`,
  [`bookmark_text.md`](../format/texts/bookmark_text.md#bookmarks).
