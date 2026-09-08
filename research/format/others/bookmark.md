# Bookmark

**Covers:** The bookmark record in all three of its forms, and what of it the reader uses.
**Read when:** Working on bookmarks, or adding a musxdom class for the bookmark object.
**Confidence:** confirmed, field by field, including each flag bit alone; only the name is recovered.

## The record

**Confirmed.** A bookmark is one record per bookmark at comparators from `0x8000` up. Where it
lives changes with the epoch and the name eventually moves out of it, but the words never change
their meaning:

| Words | Meaning | EnigmaXML |
|---|---|---|
| 0..23 | the name, 48 bytes ending at the first NUL | `bookmarkText`, via `nameRawTextID` |
| 24 | 1 scroll view, 0 page view | `scrollView` / `pageView` |
| 25 | view percent, stored as written | `viewPerc` |
| 26 | whether the view percent applies | `changePercent` |
| 27..29 | unused | — |
| 30 | measure number in scroll view, page number in page view | `meas` / `page` |
| 31 | staff comparator in scroll view, horizontal position in page view | `inst` / `horz` |
| 32 | staff-set comparator in scroll view, vertical position in page view | `vert` |
| 33 | flags: `0x0001` word 31 applies, `0x0002` word 32 applies | `changeInst` / `changeHorz`, `changeVert` |
| 34..35 | unused | — |

**Each flag bit is established alone rather than inferred from a pair.** `F2008-bookmarks.mus` has
one page-view bookmark carrying only a horizontal position, which stores flags `1`, and one
carrying only a vertical position, which stores flags `2`; their companions carry only
`<changeHorz/>` and only `<changeVert/>` respectively. The same fixture separates words 31 and 32
by giving them distinct values, 40 and 200, which every earlier specimen had held equal.

**Word 25 is a percentage as written, and survives its own enable being clear.** That fixture's
`Percent 75 off` bookmark stores 75 in word 25 with word 26 clear, and its companion carries
`<viewPerc>75</viewPerc>` and no `<changePercent/>`; only the bookmark that sets word 26 gets that
element. The two words are independent, so a reader must not treat word 25 as meaningful only when
word 26 is set — Finale keeps both.

In scroll view the first flag bit is `changeInst`. `F2001Win-bookmarks.mus` sets the staff
comparator with flags `1` and its companion carries `<changeInst/>`, while `F2008-bookmarks.mus`
has a scroll bookmark with a staff comparator and flags `0`, whose companion has none.

## The three forms

**Which form a record uses is read from its own length, never from a version.** The named forms
are 72 bytes and the nameless one is 24, so a document states for itself where its bookmark names
are.

| Epoch | Where | Size | Name |
|---|---|---|---|
| Uncompressed, DCL | `BK` in the tagged others pool, six incidences | 72 | words 0..23 |
| Zlib through Finale 2011 | class `0x007b` in the class-others pool | 72 | words 0..23 |
| Finale 2012 | class `0x007b` | 24 | in the text pool; word 27 becomes its comparator |

The move into the text pool is **Finale 2012's, not the zlib era's**. `F2008-bookmarks.mus` is a
zlib document whose records still carry their names inline, and so are the two Finale 2008 and two
Finale 2011 documents of the reference and installs corpora, all four of which recovered nothing
while the reader read only the text pool and all four of which recover every bookmark now. Since
the form is chosen by record length rather than by release, that is four documents of two releases
confirming the reading generalizes off the one fixture built for it.

In the Finale 2012 form the retained word 27 is the `nameRawTextID` the companion states.
`F2012-bookmarks.mus` stores 3 and 2 there for its two bookmarks, and its companion names
`nameRawTextID` 3 and 2.

## What the reader recovers

**Only the name, as a `texts::BookmarkText`.** musxdom has no class for the bookmark object, so the
view state has nowhere to go; it is documented above so that adding the object later is a matter of
reading words this file already describes. `src/import/others/bookmark.cpp` reads the two named
forms and the text-pool importer reads the Finale 2012 one.

The name is 8-bit and carries no font, so it is converted through the source platform's encoding
and stored as those characters alone. **No initial formatting state is synthesized for this class,
where every other text class gets one.** Nothing in the document asks for it: a `^bookmark` chunk
of the pooled era holds nothing between its header and its `^end`, a named record holds a bare
string, and Finale 27 writes a bookmark's text with no font commands either. A `^font`/`^size`/
`^nfx` prefix would therefore be the reader's invention rather than anything the source states.

That is a deliberate divergence from [File Info](../texts/file_info_text.md), which does take the
`TextBlock` font. The difference is what the text is for: File Info is engraved on the page and its
font is simply not restated, while a bookmark name is a UI label — the same reason musxdom
documents the Enigma inserts in `BookmarkText` as meaningless.

Comparators are the text pool's own, allocated in record order, and they agree with the companion's
`nameRawTextID` for the document they came from. They are not stable across an upgrade: the same
two bookmarks are 1 and 2 in the Finale 3.7.2 companion and 2 and 3 in the Finale 2012 one.

### The reader disagrees with one companion, and is right

`F2001Win-bookmarks.mus` is a Windows document whose names hold `ab bb` and `e5` — the guillemet
pair and the a-ring in Windows-1252. Finale 27 running on macOS re-read those bytes as Mac Roman
and wrote `Scroll ´ª 9, 3` and `PÂge Bookmark`. The reader converts through the platform the source
states and recovers `Scroll «» 9, 3` and `Påge Bookmark`.

The coverage report classifies this under its existing wrong-platform encoding rule, so it stays
visible as a characterized upgrade defect rather than being hidden or counted against recovery. It
is the standing example that a companion is a semantic reference and not an oracle; see
[Text encoding](../container/text_encoding.md).

A second, milder disagreement sits in the reference corpus: one Finale 2008 bookmark name ends in
a stored line break that the reader keeps and Finale 27 drops. That is
[a deliberate divergence](../container/text_encoding.md#encoding) rather than anything specific to
bookmarks.

## Coverage

| Epoch | State |
|---|---|
| Coda banner | No bookmarks. The Finale 3.5 addendum introduces them, after that era ends. |
| Uncompressed | Recovered. `F372-bookmarks.mus`; Finale 3.5 and 3.6 unobserved. |
| DCL | Recovered. `F2001Win-bookmarks.mus`, the only bookmarked document of the epoch in any survey. |
| Zlib through Finale 2011 | Recovered. `F2008-bookmarks.mus`; the form is chosen by record length, so it is not gated to that release. |
| Finale 2012 | Recovered, from the text pool rather than from the record. |
