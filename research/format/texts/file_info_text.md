# FileInfoText

**Covers:** The File Info strings and their relationship to the header text region.
**Read when:** Working on document metadata text.
**Confidence:** confirmed.

## File Info

**Confirmed.** See [Document metadata in the header text region](../container/header.md#document-metadata-in-the-header-text-region)
for the offsets. Title, composer, copyright, and description map to
`texts::FileInfoText` types 1 through 4.

**Four fields is the whole of File Info through Finale 2006.** `F2006-text-inserts.mus`
establishes it from the other side: that release's Text Tool offers a `^description` insert and
no `^lyricist`, `^arranger` or `^subtitle`.

**By Finale 2008 all seven exist and File Info has left the header entirely.**
`F2008-BE-text-inserts.mus` carries every one as a `^fileInfo(n)` record in the text pool,
numbered by musxdom's own `FileInfoText::TextType` and confirmed field for field by its
companion, while the header offsets that carry them in Finale 97 are empty. Where between the
two releases the move happened is **open**, and does not need answering: the reader fills from
the header only what the pool did not supply, so each document states for itself which way it
stores them.

**The header offsets do still hold across the DCL epoch.** `F2002-fileinfo-text.mus` carries all
four fields there and its companion recovers all four, so the move to the pool happens somewhere
after Finale 2002 rather than at the epoch boundary.

**The lower end is bounded above by Finale 3.7.2 and is otherwise open.**
`F372-fileinfo-text.mus` carries all four fields at the header offsets and its companion recovers
all four, so File Info exists by that release, which is also the earliest that writes the modern
inserts reading them. Finale 3.7.2 is the earliest release *available here* whose dialog offers
the fields; whether an earlier release offers them is untested, because no earlier release is
available to test. It is a ceiling on the arrival, not the arrival.

Whether an earlier release stores the fields cannot be settled by any corpus at any size, since
an unfilled dialog leaves the same empty offsets as a dialog that does not exist. It would need
a release whose dialog offers the fields, and no earlier one is known to.
