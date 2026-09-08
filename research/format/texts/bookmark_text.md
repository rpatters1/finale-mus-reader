# BookmarkText

**Covers:** Bookmark storage across the bookmark eras.
**Read when:** Working on bookmarks.
**Confidence:** confirmed for the eras with a specimen; the pre-2012 zlib window is uncovered.

## Bookmarks

**From Finale 2012 a bookmark's text is an ordinary text-pool record**, keyword `bookmark`, with
the `^end` terminator of that era and no style commands of its own — `^bookmark(2)Page über^end`.
musxdom documents any Enigma insert appearing in one as meaningless, but the bytes still need
decoding, so the record goes through the same converter as every other.

**Its text is UTF-8, and `F2012-bookmarks.mus` shows it directly rather than by inference:**
`c3 bc` is U+00FC for the `ü` of `Page über`, and `c2 ab c2 bb` is the guillemet pair of
`Scroll «» Bookmark`. The same pair is 8-bit in the pre-Unicode eras — `c7 c8` in Mac Roman —
which is what makes it a measurement.

**Before that the name is in the `BK` others family**, whose layout, coverage, and the reader's
treatment of the name are in [Bookmark](../others/bookmark.md). The text there is 8-bit -- `9f` for
`ü` under Mac Roman -- and the text pool of such a document holds no bookmarks at all, which is
what keeps the two readings from ever meeting in one document.

**Unverified: that the move into the text pool belongs to the Unicode project.** It fits — the
pooled form is UTF-8 and the era is the one that converted stored text — but the boundary has not
been tested inside Finale 2012. It is possible that the first release of that version still wrote
`BK` and that a point release changed it, in which case a version gate on the major alone would
be wrong. Nothing turns on this while the reader takes whichever form the document presents:
each store is read where it exists, and neither is inferred from a version.

The Finale 3.5 addendum supplied for this investigation identifies bookmarks as a 3.5 enhancement.
The earliest binary specimen available is Finale 3.7.2, so the Finale 3.5 representation remains
unobserved.
