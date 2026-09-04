# TextPool investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-18 — The text pool: a format that spells its own class names

- **Question:** Where does each musxdom `texts` class come from, and what has to change about a legacy text block
  before musxdom can read it?
- **Method:** Dumped the fourth typed block of one fixture per epoch below the record layer, then held each chunk
  against the same-numbered element in that file's Finale 27 companion. Compared all 81 companion-backed tracked
  fixtures element by element once the reader existed.
- **Observation — the pool is prose, not records.** Every epoch from Finale 97 on stores it as `^keyword(n) ...
  ^end` chunks packed end to end, which is the shape ETF prints in its own text section. The keyword names the
  class and the number is the comparator. That is why one importer covers six classes: the file states which is
  which, so nothing has to be inferred from position or comparator range. The zlib epoch keeps it in block
  `0x0017`, which the notes had described only as "decoded strings ... and binary control data".
- **Observation — three things stop it being modern Enigma already.** The uncompressed epoch spells styles as a run
  of `^efx(name)`, which has to become one `^nfx(bits)`; the compressed epochs write commands in a binary form with
  a hexadecimal-digit argument offset by one; and the bytes are in a code page named by whichever font is in force,
  while EnigmaXML is UTF-8. Legacy line breaks are carriage returns and must become line feeds.
- **The parenthetical after a font name is the `FN` header word.** `^font(Engraver Text T,8194)` is `0x2002`, bank
  2 with character set 2, which is exactly the `FN(9)` header in the same file; `^font(Times,4096)` is `0x1000`
  and matches `FN(1)`. The value is redundant with the font definition it names, which is what makes it safe to
  drop rather than merely convenient.
- **Two encoding rules, and a fixture that separates them.** `F97-fileinfo-short.mus` sets fourteen expressions in
  font 0 and one in font 16, `Patmm`. Finale 27 converts the font-0 characters byte for byte and the `Patmm` `0xb0`
  to an infinity sign — so the music font's bytes are glyph numbers and `Patmm`'s are Mac Roman, even though the
  file records the same character set for both. Font id 0 is therefore treated as a symbol font on the strength of
  its id, which is the only statement a Finale 97 file makes about it.
- **Expression text used to live in its own record.** In the uncompressed epoch `DT` packs point size and font
  comparator into the two bytes of its first payload word, the style bits into the next, and the display text into
  every incidence after. By Finale 2006 that same embedded string is the expression's description and the display
  text has moved into the pool. Reading `DT` as display text in the wrong era would fill the texts pool with
  category descriptions, so the pass is gated to the uncompressed epoch; the fixtures say nothing about Finale
  2001 through 2005, which define no expressions at all.
- **Result against the companions.** All 81 fixtures agree on every recovered character. What remains is one
  spelling difference Finale makes inconsistently with itself — it normalizes a font command to `^fontid(n)` in a
  block text and a smart shape text but passes `^font(Name,charset)` through in an expression, from the same
  stored bytes — plus the Coda-banner epoch, which recovers nothing, and four Finale 2006 block texts for staves
  the document no longer has, which the reader keeps and Finale 27 discards.
- **Next evidence:** a document containing lyric chorus or section text, to replace the inferred keywords with
  observed ones; a bookmark, whose keyword is unknown; a Finale 2002-2005 document defining a text expression, to
  place the boundary where expression text moved into the pool; and a File Info string in any epoch after Finale
  97, to show whether the header offsets still hold.
- **Artifacts:** `src/import/texts/text_pool.cpp`, `src/import/texts/expression_text.cpp`,
  `src/import/texts/file_info_text.cpp`, `src/import/support/enigma_text.cpp`, `tests/text_pool_tests.cpp`,
  [`text_pool.md`](../format/texts/text_pool.md#the-text-pool).

## 2026-08-18 — Twenty-two binary command codes, from the one release that can tell

- **Question:** From Finale 2006 the MUS stores Enigma commands as a caret, a one-byte code, and a digit
  argument. Seven codes had been read off incidental fixtures. What are the rest?
- **Method:** Noticed that Finale 2006 is simultaneously the first release to write the binary form and the last
  to export ETF, and that its ETF spells every command out. That makes one document a crib for the other. Asked
  for `F2006-text-inserts.mus`: one text block per insert the Text Tool offers, each holding that insert alone so
  the block number keys the `.mus` and the `.etf` to each other. Its Finale 27 companion agrees with the ETF on
  all 21 records, so every pairing has two witnesses rather than one.
- **Observation — 25 codes, in one pass.** `0x81` baseline, `0x84` nfx, `0x85` fontid, `0x86` size, `0x87`
  superscript, `0x88` tracking, `0x8a` composer, `0x8b` copyright, `0x8c` date, `0x8d` fdate, `0x8e` dbflat,
  `0x8f` dbsharp, `0x90` description, `0x91` filename, `0x92` flat, `0x94` natural, `0x95` page, `0x96` sharp,
  `0x99` title, `0x9a` totpages, `0x9b` perftime, `0x9c` cprsym, `0x9d` value, `0x9e` control, `0x9f` pass. Before
  the fixture the reader dropped fifteen of those and named every one in a diagnostic, which is what made the
  request precise.
- **Three properties of the argument, each resting on one block and nothing else.** It is one value, not a
  sequence of shorter ones: block 6's page offset `01 01 01 01 01 02 02 04` is `0x113` and both witnesses write
  `^page(275)`, where two four-digit arguments would be 0 and 275 — so every earlier specimen, all offset by zero,
  was consistent with both readings. The digit range runs the full 0 to 15: block 21's `^superscript(15)` is
  `01 01 01 01 01 01 01 10`, the only nibble of `0xf` anywhere. And an argument is signed: block 19's
  `^baseline(-13)` is `0xfffffff3`, which read unsigned is 4294967283.
- **The signedness was a live defect, not a nicety.** A negative baseline is ordinary, and the first
  implementation would have written 4294967283 into every document that had one, with nothing to notice it. The
  reader now treats both widths as signed, including the four-digit one, where no negative has been observed: the
  encoding is one encoding, and a dialog clamping a field at zero is a fact about the dialog.
- **Two of these came from asking whether an assumption had a witness.** The `+1` digit offset had been
  extrapolated from digits 0 to 14; a scan of every fixture showed the highest byte ever stored was `0x0f`, so
  `0x10` was an inference. Chasing it produced the three style commands, and with them the only `0xf` nibble and
  the only negative arguments in any survey. It also exposed a second problem the scan alone had shown: run
  lengths are only ever 4 or 8, so the width belongs to the command, and a greedy scan for digit-range bytes would
  eat a literal `0x10` — perfectly possible in symbol-font text — as a fifth digit, losing both the character and
  the value.
- **The ordering is nearly alphabetical and then is not.** `0x8a` to `0x9a` run in alphabetical order with `fdate`
  the single exception, which it would not be if its internal name began with `date`; `perftime`, `cprsym`,
  `value`, `control` and `pass` follow in the order a later release would have appended them. `0x81` to `0x88` are
  the style commands and follow no order this reader can see. On that reading the four gaps inside the
  alphabetical run fall where `arranger` (`0x89`), `lyricist` (`0x93`), `subtitle` (`0x97`) and `time` (`0x98`)
  belong. **None of the four is in the reader.** A wrong
  name resolves to the wrong document field and reads as recovered content; an unlisted code is reported by number
  and reads as what it is, which is how this fixture came to be asked for in the first place.
- **A second finding the same fixture settled.** Finale 2006's Text Tool offers a `^description` insert and no
  `^lyricist`, `^arranger` or `^subtitle`, so File Info still has exactly four fields at that release. The three
  further types musxdom names are therefore not a gap in the header mapping for any era this reader covers.
- **Why none of this was ever visible.** The ETF export and the PDK both hand back the spelled-out form, so no
  plug-in could see the encoding: it exists only between Finale and its own `.mus`. The one place a binary command
  reaches an ETF at all is the Coda era, where `HT` is dumped as raw quoted payloads and its own — different,
  two-byte-argument — form leaks through because ETF is printing bytes rather than text.
- **Next evidence:** the same one-insert-per-block document from a release with the fuller File Info. It needs no
  ETF; the Finale 27 companion is the crib, and produced the first seven codes before this fixture existed.
- **Artifacts:** `src/import/support/enigma_text.cpp`, `tests/text_pool_tests.cpp`,
  `tests/evidence/F2006/F2006-text-inserts.mus`, [`text_pool.md`](../format/texts/text_pool.md#the-text-pool),
  [`EVIDENCE_REQUESTS.md`](../state/EVIDENCE_REQUESTS.md).

## 2026-08-18 — Finale 2008 finishes the code table's shape, and refutes half a prediction

- **Question:** The Finale 2006 code table left four gaps inside its alphabetical run, at `0x89`, `0x93`, `0x97`
  and `0x98`, exactly where `arranger`, `lyricist`, `subtitle` and `time` would sort. Finale 2006 has none of
  those inserts, so it could not test that. Does a release that has them put them there?
- **Method:** `F2008-BE-text-inserts.mus`, the same one-insert-per-block document from Finale 2008, with all
  seven File Info fields filled in. Finale 2008 writes no ETF, so the Finale 27 companion is the only witness —
  which is what produced the first seven codes before either insert fixture existed.
- **Observation — no, and the reason generalizes.** `^time` is at `0x98` as predicted, but `^lyricist`,
  `^arranger` and `^subtitle` are at `0xa1`, `0xa2` and `0xa3`, appended past the end of the table alongside
  `^partname` at `0xa0`. Appending is the only thing a release can do when it adds an insert: renumbering would
  change what every already-saved document says. So the alphabetical run reflects one original alphabetized set
  and is not a rule still in force, and `0x89`, `0x93` and `0x97` are unexplained holes rather than reserved
  slots. `0x98` was a member Finale 2006 simply did not expose.
- **The refutation cost nothing, which was the point.** None of the four predictions was ever written into the
  reader. An unlisted code is reported by number and reads as a gap; a wrongly named one resolves to the wrong
  document field and reads as recovered content. This is the case that shows the difference is not academic.
- **Observation — a third argument width, and the one insert Finale 27 throws away.** `^time` takes a *single*
  digit where every previously observed argument was four or eight. Finale 27 emits no `^time` at all on
  conversion, so the companion cannot name it; the evidence is the fixture's own controlled pair, two blocks
  labeled "Time" and "Time with seconds" differing in that one byte, which is exactly musxdom's `^time` flag.
  The reader carries it forward. Finale 27 dropping an insert says what that conversion does, not what the
  document contains, and recovering the latter is the whole job: discarding a command the file states and musxdom
  can spell, on no better authority than a converter's choice, would be deleting content. The mapping is labeled
  strong rather than confirmed because of the missing witness, not because the decision is in doubt.
- **Observation — File Info leaves the header.** All seven fields are `^fileInfo(n)` records in the text pool,
  numbered by musxdom's own `FileInfoText::TextType` and confirmed field for field, while the header offsets that
  carry them in Finale 97 are empty. The reader needed no version boundary for this: the header pass now fills in
  only the types the pool did not supply, so each document states for itself which way it stores them. Where
  between Finale 2006 and Finale 2008 the move happened stays open, and does not have to be answered.
- **Two companion differences, both intended.** The fixture's first block is the bare text `FULL SCORE` with no
  style commands, and Finale 27 writes `^fontid(1)^size(12)^nfx(0)FULL SCORE`; the reader keeps what the file
  says rather than synthesizing a font the document never stated. And Finale 27 drops `^time`, where the reader
  carries it forward, for the reason above.
- **A byte-order specimen as a side effect.** This is the first big-endian Finale 2008 document in any survey.
  Its author produced it under Mac OS X 10.4 in QEMU because the Intel-era save crashed, which is a fair summary
  of why that half of the transition era is thin everywhere.
- **Next evidence:** whatever occupies `0x89`, `0x93`, `0x97`, `0x80`, `0x82`, `0x83`, and anything from `0xa4`
  up. Those are no longer predictable from the ordering, so they need a document that uses them rather than an
  argument about where they would sort.
- **Artifacts:** `src/import/support/enigma_text.cpp`, `src/import/texts/text_pool.cpp`,
  `src/import/texts/file_info_text.cpp`, `tests/text_pool_tests.cpp`,
  `tests/evidence/F2008/F2008-BE-text-inserts.mus`, [`text_pool.md`](../format/texts/text_pool.md#the-text-pool).

## 2026-08-18 — The last four codes, and a second prediction refuted the same way

- **Question:** four commands `musx/util/EnigmaString.h` documents had no binary code located: `^rehearsal` and
  the three font-category commands `^fontMus`, `^fontTxt` and `^fontNum`. Where are they?
- **Evidence:** `F2011-text-inserts.mus` with its Finale 27 companion. Finale 2009 introduced marking categories
  and Finale 2010 automatic rehearsal marks, so Finale 2011 is the earliest available release that can write all
  four in one document. One block text and four expressions, each carrying one command.
- **Result — the table is complete.** `0xa4` is `^fontTxt`, `0xa5` `^fontMus`, `0xa6` `^fontNum`, each with a
  four-digit argument, and `0xa7` is `^rehearsal` with none. Every command musxdom documents now has a code, and
  no fixture in the tracked corpus reports an unread code any more.
- **Refuted — the font-category commands are not in the style group.** Three free slots, `0x80`, `0x82` and
  `0x83`, sit between `^baseline` at `0x81` and `^nfx` at `0x84`, which is precisely where three font-category
  commands would belong; this reader said so in writing and did not act on it. They are appended past the last
  insert instead. That is the second prediction from the shape of the code table to be refuted by the first
  fixture able to test it, after the Finale 2008 one, and it generalizes the earlier lesson: **appending is what
  every release after the original set does, for style commands as much as for inserts.** Both groups are closed.
  Six slots stay empty in the reader.
- **Observation — a categorized font command states a comparator, not a name.** The three stored arguments are
  9, 11 and 11 where the companion writes `Times New Roman` and `Engraver Text T`, which are font definitions 9
  and 11. `^rehearsal` takes no argument, and the fixture shows it directly rather than by absence: the byte
  after the code is `0x20`, the leading space of the literal " Rehearsal", and `0x20` is not a digit byte.
- **Decision — a font reference is written under the font's name, and `^fontid` is the fallback.** Every font
  command resolves to a comparator first, and the comparator is written out as the name the document's own
  `FontDefinition` gives it; `^fontid` is what a comparator with no definition behind it becomes, being the one
  spelling that needs none. `^font` covers a plain reference whatever command the source used, and the three
  categorized commands keep their spelling, because the marking category they name is the one thing `^fontid`
  cannot carry. This makes the ordering constraint load-bearing rather than incidental: font definitions must be
  imported before any text, which the importer registry already does.
- **Follow-on:** every text fixture's expected strings changed with it, from `^fontid(4)` to `^font(Times)` and
  the like. The fallback had no fixture, so the synthetic cases now cover both halves of it: a comparator the
  document defines, one it does not, and a categorized command in each case.
- **The tripwire earned its keep.** Before the fix the reader reported `0xa4 0xa5 0xa6 0xa7` by number and
  dropped their text, rather than guessing a width and swallowing the following characters. The width-per-command
  table added for the Finale 2006 work is what made that possible.
- **Next evidence:** `0x80`, `0x82`, `0x83`, `0x89`, `0x93`, `0x97`, and anything from `0xa8` up. Nothing about
  the ordering predicts these, and two refutations say not to try.
- **Artifacts:** `src/import/support/enigma_text.cpp`, `tests/text_pool_tests.cpp`,
  `tests/evidence/F2011/F2011-text-inserts.mus`, [`text_pool.md`](../format/texts/text_pool.md#the-text-pool).

## 2026-08-18 — The Coda-banner pool walk, and how little text that era actually gives us

- **Defect:** the container's Coda-banner walk ended on the first pool with zero pages. A Finale 1.0.0 document
  reported **one** block where it has three, because its details pool is empty; its entries pool and the text
  region behind it were unreachable.
- **Fix:** an empty pool is an ordinary pool. The page size is the only thing that identifies a prologue, and the
  chain needs no terminator of its own — what follows the last pool is the text region, whose first four bytes
  are a chunk length rather than 0x200, so the page-size test ends the walk there anyway. The offset advances by
  the prologue even for an empty pool, so a run of them cannot spin.
- **Result:** `F100-baseline.mus` goes from 1 block to 3. No diagnostic changed on any fixture, all 84 still
  parse, and no existing test asserted the old count. A new synthetic case builds pools of {1, 0, 1} and {0, 0,
  0} and fails without the fix.
- **The finding that matters more than the fix.** The era's text region is two length-prefixed chunks, and they
  are empty in all 26 tracked documents — but that is not why recovery is blocked. Block text lives in the `HT`
  others family, and `F263-baseline.mus` holds it plainly: `TITLE`, `Licensed by ASCAP`, `One Lincoln Plaza`,
  `New York, NY  10023`, `All rights reserved.` and a composer and copyright line, in NUL-terminated runs
  alternating with binary layout rows. Finale 27 recovers all eleven blocks and seventeen expressions from it.
- **The corpus is thinner here than its count suggests, and this is the reason to ask for fixtures rather than
  keep looking.** All 21 Finale 1.0.0 documents contain no text at all — their companions carry only Finale 27's
  `Score` part name, which is PartDefinition's and is not yet imported — and the five Finale 2.6.3 documents are five saves of one document. The era has
  **one** text specimen. A companion names what the answer should be but cannot separate the text bytes from the
  layout bytes woven through them; only a controlled one-variable edit does that.
- **Observation — the two releases spell the region markers differently.** Finale 1.0.0 writes `^text \0` and
  `^lyric \0`; Finale 2.6.3 writes `^text()` and `^lyrics()`. Singular against plural, and a space-plus-NUL
  against parentheses. That is reason enough not to assume one `HT` layout covers both.
- **Observation — File Info's lower boundary is unanswerable from any corpus.** An unfilled dialog leaves the
  same empty header offsets as a dialog that does not exist, and only three tracked documents anywhere have the
  fields filled. The arrival may be around Finale 3.7 rather than earlier, which would make the header offsets
  meaningless for every release before it.
- **Next evidence:** X6 and X7 in [`EVIDENCE_REQUESTS.md`](../state/EVIDENCE_REQUESTS.md).
- **Artifacts:** `src/container/mus_container.cpp`, `tests/reader_tests.cpp`,
  [`coda_texts.md`](../format/texts/coda_texts.md#the-coda-banner-epoch).

## 2026-08-18 — Four fixtures open the Coda-banner text, and a third pool framing appears

- **Question:** the previous entry ended with the pool walk fixed and the `HT` framing undecoded, and with the
  observation that the era had exactly one text specimen. Four Finale 1.0.0 documents were authored to answer it.
- **Result — block text is `HT` plus `HS`, one pair per block.** `HT` holds the characters in four consecutive
  incidences, 48 bytes, ending at the first NUL; `HS` holds the style, keyed so that incidence n describes the nth
  `HT` record of the same comparator. `HS` word 2 packs the font comparator above the point size, word 3 is the
  `nfx` mask, word 4 is an insert argument and word 5 selects the insert.
- **What proved style is not in `HT`:** two blocks of the Finale 2.6.3 document differ in size and style — 14/1
  against 12/0 — while sharing a byte-identical `HT` trailer. Whatever that trailer holds, it is not this.
- **The controlled pair did the work a companion cannot.** `F100-short-text` and `F100-long-text-w-insert` differ
  in one block's string, and exactly two records move between them. That is what says a block is those two records
  and nothing else, and what showed the record is a fixed four incidences either way, so a shortened string leaves
  the previous save's bytes behind the terminator.
- **One insert character, three inserts.** `#` converts to `^page`, `^date` or `^time` depending on `HS` word 5,
  with word 4 as the argument in each case. `F100-text-other-inserts` is what separates them. **Believed** rather
  than established: three observations cannot distinguish a two-bit field from two independent flags, so an
  unlisted value keeps the character and reports it.
- **Lyric text is elsewhere again** — in the text region behind the pools, spelled out, where the `^text` chunk
  beside it is empty even in documents that plainly have block text. `F100-lyric-text` is the only document of the
  era anywhere with lyrics, and it forced one converter fix: an `^efx` run written spaced apart is still one run
  and still one `^nfx`, with the spaces belonging before the command.
- **A third pool framing, from `F372-fileinfo-text`.** Finale 3.7.2 keeps one pool stream divided by the ETF
  section markers themselves, `^text` and `^lyrics`, and **a record has no terminator**: it runs to the next
  record, to the next marker, or to the end of the stream. Finale 97 drops the markers and closes records with
  `^end`. The reader tells them apart by reading the opening bytes, because the boundary falls inside the
  uncompressed epoch and no epoch gate can express it.
- **Observation — File Info is bounded, not dated.** That release carries all four fields at the header offsets
  and is the earliest available whose dialog offers them. That is a ceiling on the arrival and not the arrival:
  no earlier release is available to test, and no corpus can settle it at any size, because an unfilled dialog
  leaves the same empty offsets as a dialog that does not exist.
- **A caution the fixture earned.** An intermediate save of that document lost both block texts, from the MUS and
  the ETF alike, leaving five lyric records where seven text items had been entered. The file was structurally
  valid and nothing announced the loss. Finale of this vintage under emulation can corrupt a document silently.
- **Artifacts:** `src/import/texts/coda_texts.cpp`, `src/import/texts/text_pool.cpp`,
  `src/import/support/enigma_text.cpp`, `tests/text_pool_tests.cpp`,
  [`coda_texts.md`](../format/texts/coda_texts.md#the-coda-banner-epoch).
