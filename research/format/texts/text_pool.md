# Text pool

**Covers:** Pool framing, initial formatting state, section markers before Finale 97, the binary Enigma command table, and the parenthetical value after a font name.
**Read when:** Working on any pooled text, or on Enigma command parsing.
**Confidence:** confirmed for the command table in the compressed epochs.

## The text pool

**Confirmed** for the Coda-banner, uncompressed, DCL, and zlib epochs against the controlled
fixtures and their Finale 27 companions.

The text pool is the one pre-2007 pool that is not made of records, and the one place the
format describes itself in words. It is a byte stream of `^keyword(n) ... ^end` chunks packed
end to end, with nothing between them — the same shape ETF prints in its own text section.
The keyword names the class, and the number in parentheses is the comparator musxdom keys the
texts pool by.

| Epoch | Block | Text form of commands | Binary form |
|---|---|---|---|
| Coda banner | not in a block; see below | — | — |
| Uncompressed | `0x0004` | `^font(Name)`, `^efx(name)` | not observed |
| DCL | `0x0012` | `^font(Name,charset)`, `^nfx(n)` | yes |
| Zlib | `0x0017` | same as DCL | yes, UTF-8 encoded from Finale 2012 |

Keywords observed, with the musxdom class each names:

| Keyword | Class | Evidence |
|---|---|---|
| `block` | `texts::BlockText` | every epoch's fixtures |
| `verse` | `texts::LyricsVerse` | `F2007-lyric-hyphens.mus` |
| `chorus` | `texts::LyricsChorus` | **inferred** from `verse`; no fixture has one |
| `section` | `texts::LyricsSection` | `F2001Win-section-lyric.mus` |
| `smartshape` | `texts::SmartShapeText` | `F2006-embedded-tiff.mus` |
| `expression` | `texts::ExpressionText` | `F2006-embedded-tiff.mus` |
| `fileInfo` | `texts::FileInfoText` | `F2008-BE-text-inserts.mus` |
| `bookmark` | `texts::BookmarkText` | `F2012-bookmarks.mus` |

The reader reports any keyword it does not recognize, by name, which is how a missing spelling
is meant to be found.

The little-endian Finale 2001 text fixtures confirm both framing and encoding at the DCL lower
boundary. `F2001Win-meastext.mus` recovers `Measure-attachéd text: €2.99` through Windows-1252,
and `F2001Win-section-lyric.mus` recovers its `section` record exactly. Their ETF exports and
Finale 27 companions agree with both strings.

### Initial formatting state

**Strong for formatted text.** A legacy text record can begin with literal text or with only
part of a face, size, and effects state. Finale's upgraded EnigmaXML completes missing initial
settings in block and lyric text from the document's corresponding `FontOptions` default. The
reader does the same before the first literal byte, while preserving every explicit command,
its order, and any literal whitespace between later commands. It spells a synthesized face as
`^font(name)`, not `^fontid(cmper)`, so the result remains stable if font-definition comparators
are renumbered.

The defaults are TextBlock for block, bookmark, File Info, and smart-shape text; Expression for
expression text; and LyricVerse, LyricChorus, or LyricSection for the corresponding lyric kind.
Smart-shape text was verified in Finale as the line text of a smart-shape line and follows the
TextBlock default. Finale does not use font information on File Info or bookmark text, and its
companions may therefore omit their formatting state; the reader nevertheless completes them
so each is a valid self-contained Enigma string.

This makes `FontDefinitions` followed by `FontOptions` a bootstrap dependency ahead of the
ordinary pool order: options, others, details, entries, then texts. The reader exposes one text-
family import stage; whether a text came from the text stream, fixed header offsets, or a Coda-
banner store remains an implementation detail of that family.

Finale also removes an exact adjacent duplicate complete state during upgrade. Section lyric 2
in `F372-fileinfo-text` contains the same face, size, and effects commands twice in both its MUS
text stream and ETF, while its Finale 27 companion contains them once. The reader preserves the
source commands; companion-quality reporting treats that no-op normalization as equal.

### Section markers before Finale 97

**There are three framings, and each stream states its own.**

Finale 3.7.2 and earlier open the pool with the ETF section markers themselves. A Finale 3.7.2
document carries one stream beginning `^text`, then its `^block(n)` records, then `^lyrics` and
whatever lyric records follow; an empty document is the bare bytes `^text^lyrics`, two markers
with nothing between them. **A record in this framing has no terminator** — it runs to the start
of the next record, to the marker that opens the next section, or to the end of the stream.

`F372-fileinfo-text.mus` is what shows it. Its ETF export carries the same records in the same
order and the same spelling, differing only in the carriage returns ETF puts between sections,
so a MUS of this era stores very nearly what ETF prints.

Finale 97 and later drop the markers and close each record with `^end`. Each record names its
own kind, so a section header has nothing left to say.

The Coda-banner epoch is different again: two length-prefixed chunks behind the last record
pool, opening `^text ` and `^lyric ` (Finale 1.0.0) or `^text()` and `^lyrics()` (Finale 2.6.3).
Its lyric records live there, in the unterminated form; its block text does not — see
[The Coda-banner epoch](coda_texts.md#the-coda-banner-epoch).

The reader tells the first two apart by reading the opening bytes rather than by dating the
file: a stream that opens with a section marker is unterminated, and one that opens with a
record is not. The boundary sits inside the uncompressed epoch, so an epoch gate cannot express
it, and the marker is something every stream carries.

### Enigma commands in the compressed epochs

From the DCL epoch Finale writes most commands in a binary form: a caret, a one-byte code, and
an argument spelled as hexadecimal digits one byte per digit, each digit stored one greater
than its value. `^\x86\x01\x01\x02\x09` is `0x0018`, which the companion writes as
`^size(24)`. The run ends at the first byte outside `0x01`–`0x10`, which is what lets literal
text follow an argument directly.

**Confirmed** by `tests/evidence/F2006/F2006-text-inserts.mus`, a document written to answer
this and nothing else: one text block per insert the Finale 2006 Text Tool offers, each
holding that insert alone, so the block number identifies the pairing. Finale 2006 is the only
release that can settle it — the first to write these codes and the last to export ETF — so
the same records exist encoded in the `.mus` and spelled out in the `.etf`. The Finale 27
companion agrees with the ETF on all 21 records, giving every row below two witnesses.

| Code | Command | Argument |
|---|---|---|
| `0x81` | `^baseline` | 8 digits |
| `0x84` | `^nfx` | 4 digits |
| `0x85` | `^font` | 4 digits — a comparator |
| `0x86` | `^size` | 4 digits |
| `0x87` | `^superscript` | 8 digits |
| `0x88` | `^tracking` | 8 digits |
| `0x8a` | `^composer` | — |
| `0x8b` | `^copyright` | — |
| `0x8c` | `^date` | 4 digits |
| `0x8d` | `^fdate` | 4 digits |
| `0x8e` | `^dbflat` | — |
| `0x8f` | `^dbsharp` | — |
| `0x90` | `^description` | — |
| `0x91` | `^filename` | — |
| `0x92` | `^flat` | — |
| `0x94` | `^natural` | — |
| `0x95` | `^page` | 8 digits |
| `0x96` | `^sharp` | — |
| `0x98` | `^time` | 1 digit |
| `0x99` | `^title` | — |
| `0x9a` | `^totpages` | — |
| `0x9b` | `^perftime` | 4 digits |
| `0x9c` | `^cprsym` | — |
| `0x9d` | `^value` | — |
| `0x9e` | `^control` | — |
| `0x9f` | `^pass` | — |
| `0xa0` | `^partname` | — |
| `0xa1` | `^lyricist` | — |
| `0xa2` | `^arranger` | — |
| `0xa3` | `^subtitle` | — |
| `0xa4` | `^fontTxt` | 4 digits — a comparator |
| `0xa5` | `^fontMus` | 4 digits — a comparator |
| `0xa6` | `^fontNum` | 4 digits — a comparator |
| `0xa7` | `^rehearsal` | — |

Three properties of the argument are fixed by three of that fixture's blocks, and by nothing
else in any survey:

- **It is one value, not a sequence of shorter ones.** Block 6's page offset is stored
  `01 01 01 01 01 02 02 04`, whose eight digits read together are `0x113`, and both witnesses
  write `^page(275)`. Read as two four-digit arguments the same bytes are 0 and 275, so only a
  non-zero offset could have told the two apart.
- **The digit range runs the full 0 to 15.** Block 21's `^superscript(15)` is
  `01 01 01 01 01 01 01 10` — the only place a nibble of `0xf` appears anywhere, and therefore
  the only evidence that the `+1` offset holds across the whole range rather than only up to
  the `0xe` that everything else stops at. No argument byte is ever `0x00`, which is
  presumably the point: a zero byte would end a C string.
- **Widths are 1, 4, or 8 digits, and belong to the command rather than to the value.**
  `^nfx(0)` still spends four digits on a zero, so nothing about a value shortens its
  argument. `^time` is the only one-digit command known, and its single digit is the
  seconds flag.
- **An argument is signed, in two's complement at whatever width it occupies.** Block 19's
  `^baseline(-13)` is `10 10 10 10 10 10 10 04`, or `0xfffffff3`; read unsigned that is
  4294967283. No four-digit argument has been observed negative — the commands that use that
  width carry a font comparator, a point size, a style mask, or a format ordinal — but both
  widths are read signed, and Finale itself is the reason. Its page offset and tracking dialogs
  both run 0 to 32767, the signed 16-bit maximum, in fields whose argument is eight digits and
  so has 32 bits to spend: the value behind each dialog is a signed short widened on the way
  out. Baseline and superscript write that same eight-digit argument and are routinely
  negative, so the floor belongs to each dialog while the width belongs to the encoding. The
  one case where the two readings
  would differ is a font comparator above 32767, which needs a document with 32768 font
  definitions; nothing is done about that until one appears.

Several of these are resolved by a parsing context rather than by the general Enigma parser.
`^value`, `^control` and `^pass` come from the context a `TextExpressionDef` supplies, all three
being playback properties of the expression the text belongs to. `^rehearsal` comes from the
context a `MeasureExprAssign` supplies, since the mark it produces depends on where in the score
the expression is assigned. `^filename` is resolved by neither, and is for the client to
resolve: only the client knows the name it saved the document under, or is about to save it
under. The reader writes all of them out regardless, because each is what the document says,
and an insert this reader cannot resolve is left for whoever can.

**Cross-referenced against every command `musx/util/EnigmaString.h` documents**, the table is
now complete: every command named there has a code. What remains is the reverse question, the
slots no observed command occupies.

**Still open:** `0x80`, `0x82`, `0x83`, `0x89`, `0x93`, `0x97`, and anything from `0xa8` up.

The codes fall into two groups. `0x81` to `0x88` are the style commands, in no order this
reader can see. From `0x8a` up they are inserts, in alphabetical order, with `fdate` at `0x8d`
the one member out of place — which it would not be if its internal name began with `date` —
and with `perftime`, `cprsym`, `value`, `control` and `pass` following `totpages` in the order
a later release appended them.

**A prediction made from that ordering was half right, and the wrong half is worth recording.**
The Finale 2006 table left four gaps inside the alphabetical run, at `0x89`, `0x93`, `0x97` and
`0x98`, exactly where `arranger`, `lyricist`, `subtitle` and `time` would sort. Finale 2008 has
all four inserts, so it settles them: `^time` is indeed at `0x98`, but `^lyricist`, `^arranger`
and `^subtitle` are at `0xa1`, `0xa2` and `0xa3`, **appended** past the end alongside
`^partname` at `0xa0`. That is the only thing a release can do when it adds an insert, since
renumbering would change the meaning of every document already saved — so a name absent from an
earlier release will never appear inside the alphabetical run, and the run reflects one
original alphabetized set rather than a rule still in force.

`0x89`, `0x93` and `0x97` are therefore unexplained holes rather than reserved slots, and
`0x98` was a member Finale 2006 simply did not expose. **None of the predictions was ever put
into the reader**, which is why the refutation cost nothing: a wrong name resolves to the wrong
document field and reads as recovered content, where an unlisted code is reported by number and
reads as what it is.

**One standing hypothesis, deliberately untested:** that `0x82` and `0x83` are `^fontid` and
`^fontNum`, or the two the other way round. It is recorded here as a tripwire — so that a
document which turns out to carry either code has something to be checked against — and not
because any evidence supports it. No corpus has been surveyed for it, and none should be on its
account alone. It stays out of the reader's table until a document states it, for the reason the
next paragraph gives.

**A second prediction from the same shape was also refuted, the same way.** Three of the free
slots — `0x80`, `0x82`, `0x83` — sit inside the style group between `^baseline` at `0x81` and
`^nfx` at `0x84`, which is exactly where the three font-category commands would belong.
`F2011-text-inserts.mus` puts them at `0xa4` to `0xa6` instead, appended past the last insert
alongside `^rehearsal` at `0xa7`. Appending is evidently what every release after the original
set does, whether the command is an insert or a style command, and the style group is as closed
as the alphabetical run is. The six remaining slots are left empty in the reader rather than
filled by inference, for the same reason as before.

From Finale 2012 the pool is UTF-8, so the same code arrives as its two-byte encoding:
`^\xc2\x85` rather than `^\x85`. The reader accepts both spellings structurally rather than
gating on the version.

### The parenthetical value after a font name

`^font(Times,4096)` and `^font(Engraver Text T,8194)` carry the same packed character-set word
the `FN` record's own header holds: the high nibble is the bank, 1 for Mac and 2 for Windows,
and the low twelve bits are the character set value. `4096` is `0x1000`, Mac with no character
set, and `8194` is `0x2002`, the Windows symbol character set — and both agree exactly with
the `FN` header of the font they name in `F2006-embedded-tiff.mus`. The value is therefore
redundant with the font definition, and the reader does not carry it forward. It is not
redundant while reading the command itself, however: `mus-f2884c8c17e584a9` stores
`ヒラギノ明朝 Pro W3` as Shift-JIS bytes and gives the command the packed value `4097`
(`0x1001`, Mac Japanese). Decoding that name as the document's Mac Roman default produces
mojibake and prevents it from resolving to the correctly decoded font definition. The
parenthetical therefore decodes the stored font name before resolution; the document platform
is only the fallback for a command that omits it.

A font referenced by id is spelled `Font` followed by the id, with a character set of `0`:
`^font(Font0,0)`. musxdom reads that spelling natively.

**A companion's font table is not stable evidence.** Finale 27 matches fonts against the ones
installed on the machine doing the upgrade, and the path taken — a scripted upgrade or a manual
one — can change the result too. Re-exporting the same source document has been observed to
renumber its whole `fontName` table, drop a face and shift every `FontOptions` comparator with
it. So a companion is a good witness to *which face* a text is set in and a poor one to *which
comparator*, and two companions of the same document generated at different times may disagree
about the comparator without either being wrong.

The source's own font table has no such elasticity: it is what the document stores. That is the
reason the rule below reads in the direction it does.

**The reader names the font wherever the document defines one.** Every font command resolves to
a comparator first — `^fontid` and every binary code state one outright, `Font` followed by
digits is the same thing under Finale's convention for a font it knows only by id, and a name is
matched back to a definition — and the comparator is then written out under the name that
definition carries. This is why `importFontDefinitions` runs before any text importer.

`^font` is the spelling for a plain reference, whatever command the source used, and
`^fontMus`, `^fontTxt` and `^fontNum` keep theirs: the marking category they name is the one
thing `^fontid` cannot carry.

**`^fontid` is the fallback, not the normal form.** It is the one spelling that needs no
definition to exist, so it is what a comparator the document does not define becomes. That
fallback also drops the marking category a categorized command names, which is the lesser harm
against inventing a name for a definition that is not there. A *name* that resolves to nothing
is different again: there is no comparator to fall back to, so the command and the name are both
kept and the absence stays visible.

The character-set parenthetical is not carried forward in any of these forms, for the reason
above: the definition states it, and musxdom reads it only from there.
