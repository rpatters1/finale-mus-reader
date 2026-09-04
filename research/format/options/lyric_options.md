# LyricOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Lyric options

**Partially implemented, and partial in a way worth stating plainly: eleven of this class's twenty-three fields have
no located storage at all.** What is implemented is `src/import/options/lyric_options.cpp`, covering the two
collections and five scalars across every epoch that stores them. The mapping is in
[`data/lyric_options_mapping.csv`](../../data/lyric_options_mapping.csv). Counts below are the 69 tracked fixtures and
their exact Finale 27 companions unless stated otherwise; **no corpus survey has been run for this class yet**, so
every confidence label here rests on the fixtures alone.

The class is spread over six numeric globals at comparator `65534`, reached in the zlib epoch through the usual
`numericGlobalClass` rule. They do not arrive together, which is why each is gated by its own record rather than by
one boundary:

| Selector | Zlib class | Arrives | Carries |
|---|---|---|---|
| `15` | `0x001d` | present in every era; word 1 usable from Finale 3.x | `maxHyphenSeparation` |
| `29` | `0x002b` | **every era, including the Coda banner** | `wordExtVertOffset`, the dialog's "Lift" |
| `30` | `0x002c` | **every era, including the Coda banner** | `wordExtHorzOffset`, the dialog's "Push" |
| `34` | `0x0030` | present in every era; word 5 usable from Finale 2004 | `useSmartWordExtensions` |
| `35` | `0x0031` | present in every era; word 5 usable from Finale 2004 | `useSmartHyphens` |
| `55` | `0x0045` | Finale 2004 | eight, later nine, word-extension connection styles |
| `57` | `0x0047` | Finale 2004 | `smartHyphenStart`, `wordExtNeedUnderscore`, `wordExtMinLength`, `wordExtOffsetToNotehead` |
| `58` | `0x0048` | Finale 2011, and the record grows to say so | the three `showAutoNumbersOn…` flags and `lyricAutoNumType` |
| `67` | `0x0051` | Finale 3.x | `wordExtLineWidth` |
| `87` | `0x0065` | Finale 2000 | the four syllable position styles |

Only three of the six are in the private framework study, which names selectors `15`, `35`, `57`, `67` and `87` for
this group. Selectors `55` and the whole connection table are **not in it at all** and were located from the corpus.

**`wordExtLineWidth` is the one scalar the fixtures vary, and it is what proves the group is read rather than
inherited.** Finale 3.7.2, 97, 2000 and 2002 store 118; Finale 2003 through 2007 store 224; one Finale 2012 document
stores 115. Each companion carries exactly that number, and the pinned baseline says 115, so a document inheriting
the baseline would be right once by accident and wrong everywhere else.

### Two collections, and the two orders that do not match musxdom

The **word-extension connection table** at selector `55` begins as eight three-word elements — connection point,
then the horizontal and vertical offsets — laid out in the first eight positions of musxdom's own
`WordExtConnectStyleType` order. Its Finale 2004 payload is four fixed rows and has no `zeroOffset` element. A later
layout adds `zeroOffset` as the ninth element and grows to five fixed rows, thirty words, of which the last three
are padding. Payload length therefore selects the layout without a version gate.

Its connection point is numbered on a scale this class does not own. The values run `0x10` to `0x15`, continuing a
wider entry-connection numbering that begins at note and stem attachments, and their order is
`lyricRightBottom, headRightLyrBaseline, dotRightLyrBaseline, durationLyrBaseline, systemLeft, systemRight`.
musxdom puts the two system attachments third and fourth, so the value cannot be cast and is translated through a
table. `tests/evidence/F2006/F2006-embedded-tif.*` fixes the whole mapping in one document: it carries all six
numbers with five distinct vertical offsets beside them, and its companion names each connection point next to the
same offsets. The framework corroborates the base and the tail from an unrelated place — its smart-shape entry
connection enum reaches lyric-right-bottom at exactly `0x10` and ends with the two system attachments — with one
difference: that enum has no dotted entry, which is why its `duration` sits one place earlier than the records put
it. The records and the companions agree with each other, so they govern.

musxdom also keeps the **starting connection's two offsets twice**, once as that connection's own and once as the
class-level `wordExtHorzOffset` and `wordExtVertOffset` the Lyric Options dialog shows. The Finale 2006 fixture
shows they are one value rather than two that happen to agree: it stores 8 where every other tracked document
stores 4, and its companion moves both spellings together.

### Lift and Push, the only lyric values the earliest era stores

**Confirmed on three representations, in every epoch.** The dialog's word-extension "Lift" and "Push" have numeric
globals of their own — selector `29` word 5 and selector `30` word 5 — and unlike everything else in this class
those exist in **every** era including the Coda banner. `tests/evidence/F100/F100-wext-push-6-lift-5.*` names them:
a Finale 1.0.0 save moving Push to 6 and Lift to 5 moves selector 29 word 5 from 4 to 5 and selector 30 word 5 from
4 to 6, moves no other word in the file, and its companion reads 5 and 6. Its ETF prints the same two rows.
`tests/evidence/F372/F372-lyricopts-changed.*` repeats it in the uncompressed epoch. Every tracked fixture agrees
with its companion on both fields across all four epochs.

Three things follow.

**The Coda-banner epoch is no longer empty for this class.** It supplies exactly these two fields and nothing else,
which is what its dialog exposes: Lift and Push are the whole of its lyric options. Finale 3.7.2 adds hyphen
spacing and word extension line thickness to them, and the four-option fixture confirms all four at once — it is
also the only tracked document anywhere that varies the hyphen separation, which promotes selector `15` word 1
from consistent-everywhere to confirmed.

**A recorded open correlation was wrong and is retired.** Pre-Finale-2004 documents whose companions show a
vertical offset of 1 rather than 4 had looked as though the value tracked the word-extension positioning bit. It
never did: those documents store 1 in selector 29. The correlation held across six fixture groups purely because
nothing else in the sample separated them, which is a fair warning about how convincing a coincidence can look
when the real field has not been found yet.

**Where selector 55 does not exist, the starting connection takes the dialog's values.** That is what Finale 27
does with such documents and what both new companions show. One further synthesis is deliberately *not*
reproduced: the companions also move the `oneEntryEnd` connection's horizontal offset from 42 to 44 in both
Finale 1.0 and Finale 3.7 documents. These observations establish the upgrade result but not its formula, so the
reader keeps the pinned baseline's 42 and the difference is intended.

The **syllable position table** at selector `87` is four three-word positions across two fixed rows, again in
musxdom's own order: others (`default`), word extension, first syllable, start of system. Selector 87 arrives with
**Finale 2000**, the first release with a dedicated Lyric Options dialog, and these four positions are the whole of
what that dialog adds to the four settings Finale 3.x already had.

That order is corpus-confirmed rather than inferred from the framework's field names, which matters because the
last two positions carry identical values in almost every document and so discriminate nothing on their own. Five
reference-corpus documents are the exception and all five agree: a Finale 2004 document stores `(1, 2)` in words
6-8 against `(1, 1)` in words 9-11, with a companion whose `first` is center/left and whose `systemStart` is
center/center, and four documents from Finale 2000 and 2010 carry the pair the other way round. Both directions are
observed, so the two cannot be swapped.

Its alignment and justification use a third numbering — `1 = center, 2 = left, 3 = right`, from the framework's
`LYRICS_ALIGN_` constants — which is neither musxdom's `Left, Right, Center` nor its reverse. Every fixture that
carries the selector stores 1 where its companion says `center` and 2 where it says `left`, for all four positions.
**No surveyed document stores 3**: across all 1,189 companion pairs not one carries `right` as either an alignment
or a justification, for any position, so the corpus could never have settled that member.
`tests/evidence/F2000/F2000-lyropts-align-just.*` does, and supplies it twice over — the first syllable's alignment
set to Right and the system-start syllable's justification set to Right, so a mapping that translated only one of
the two fields would fail on it. Its companion reads `first` as right/left and `systemStart` as center/right.
Against its parent the only options record that moves is selector 87's second incidence, where word 6 goes 1 to 3
and word 10 goes 2 to 3. **All three members of the legacy alignment list are now verified against Finale's own
conversion**, and the framework's `LYRICS_ALIGN_` constants are corroborated rather than relied on.

That fixture also carries both states of the flag bit in one record: setting a position's alignment enables it, so
`first` and `systemStart` gain 0x8000 while the word extension position stays off.

Bit 15 of each position's third word is musxdom's `on`. `tests/evidence/F2000/F2000-multilayer.mus` is the one
tracked document that clears it for the three optional positions, and it is the one whose companion omits their
`<on/>`; every other fixture sets all three and every companion emits them. The framework calls the first
position's flag a placeholder with no UI, and musxdom likewise documents `on` as meaningless for `default`.

### Three values an era fixed rather than stored

- **The three optional syllable positions are off before selector `87` exists.** The pinned baseline switches all
  three on, and all 37 tracked fixtures from Finale 1.0.0, 2.6.3, 3.7.2 and 97 -- every document in the set without the
  selector -- convert with all three off.
  The alignments are left to the baseline, which already carries what those same conversions produce.
- **The word-extension line width is 224 where selector `67` does not exist.** All 25 Coda-banner fixtures convert
  to 224, and the baseline says 115. This is one of the few cases where the baseline supplies a value and supplies
  it wrongly.
- **Syllable edge punctuation is not ignored before Finale 2012**, where the setting does not exist. The pinned
  baseline says the opposite, so this is asserted rather than inherited. See the section below: the corpus settles
  both the boundary and the word, and the assertion covers every release through Finale 2010.

### Syllable edge punctuation, and a confound that nearly named nine wrong answers

**Confirmed for Finale 2012 and for every earlier release, against all 1,189 adjacent-exact companion pairs of the
reference corpus.** `lyricUseEdgePunctuation` is **selector `57` word 4**, class `0x0047` byte 8 in the zlib epoch —
the fourth field of the six-word row that already holds `smartHyphenStart`, `wordExtMinLength` and
`wordExtOffsetToNotehead`. The reader recovers it from Finale 2012 and asserts the era's behavior before that; both
branches agree with their companions on all 1,189 documents, with no read failure.

The boundary is **Finale 2011**, and getting there took a correction worth recording, because the reference corpus
alone gave the wrong answer:

| Release | Selector 57 word 4 | Companions: not ignored | ignored | Survey |
|---|---|---:|---:|---|
| Coda-banner through Finale 2003 | record absent | 454 | 0 | reference |
| Finale 2004 through Finale 2010 | 0 in every document | 487 | 0 | reference |
| **Finale 2010** | **0** in all 22 | 22 | 0 | installs |
| **Finale 2011** | **1** in all 597 | 597 | 0 | installs |
| Finale 2012 | 0 | 0 | 198 | reference |
| Finale 2012 | 1 | 50 | 0 | reference |

So the word exists for seven releases before it means anything, and reading it early would switch edge punctuation
off for all 941 of those documents against every one of their companions. That is what the version gate prevents.

**The reference corpus contains no Finale 2011 document at all**, so a boundary read off it is an interpolation
across that gap — and this reader had the gate at Finale 2012 until MakeMusic's own manuals contradicted it. The
Finale 2010 Document Options-Lyrics dialog has neither "Ignore Syllable Edge Punctuation" nor a "Punctuation to
Ignore" field; the Finale 2011 dialog has both; and the Finale 2012 manual's "Finale 2011 Interface Changes" page
states the feature arrived in 2011 outright. The installs survey then confirms it at exactly that line, all 22 of
its companion-backed Finale 2010 documents carrying 0 and all 597 Finale 2011 ones carrying 1.

The lesson is not that the corpus was wrong but that it was *silent*, and silence read as a boundary. The gap had
even been written down at the time — "Finale 2011 is absent from this corpus" — and the gate was still coded to the
nearest release that was present.

A generalization to avoid here, because the corpus invites it and a fixture contradicts it: all 597 companion-backed
Finale 2011 documents carry 1, and it is tempting to read that as the release's default. It is not.
`tests/evidence/F2011/F2011-baseline.mus`, created new in Finale 2011, carries **0** — a document made in that
release ignores edge punctuation by default. The 597 are Finale's own sample and template files, authored earlier
and converted into the release, and conversion switches ignoring off to preserve the older look. That is the same
behavior the Finale 2012 cohort shows, where the 50 documents carrying 1 are the upgraded ones and the 198 born in
2012 carry 0. **A corpus of one vendor's shipped content is not a sample of what a release writes for a new
document.**

`tests/evidence/F2012/F2012-lyropts-noign-punct.mus` closes it with a controlled one-variable save, and confirms the
mapping predicted from the corpus before the fixture existed: clearing the checkbox moves byte 8 of class `0x0047`
from 0 to 1, no other word of that record moves, and the companion gains `<lyricUseEdgePunctuation/>`. It is the only
published document anywhere with the switch cleared, so it is the sole fixture exercising the word **set** rather
than clear; every other tracked Finale 2012 fixture sits on the ignored side. It also shows the switch and the tail
are independent — ignoring is off there and the list is stock, so the record stays twelve bytes.
**It is a version gate rather than a marker because nothing structural distinguishes the two cases**: the record is
twelve bytes in Finale 2007 and in Finale 2012 alike, so its shape says nothing and only the release does. It is
bounded inside the zlib epoch and fails closed onto the pre-2012 behavior, which is right for every release but one.

The route to that word is worth recording, because the obvious search produced nine wrong answers first. Finale 2012
is the only release whose documents vary, so the 248 companion-backed Finale 2012 documents are the only cohort that
can locate the word — and a search for any record bit that partitions those 248 exactly returns **thirteen** of them,
across fonts, stems, clefs and beams. The split is not a punctuation split at all but a **lineage** split: the 198
that ignore punctuation are documents born in Finale 2012, carrying that release's own new defaults, and the 50 that
do not are documents upgraded into it from older files. Fields already mapped prove it — the same partition falls
across `wordExtLineWidth` (115 for the born documents against 118 or 224 for the upgraded ones), the syllable
positioning bits, and the starting connection's vertical offset.

What separates an arriving field from a lineage artifact is a **negative control in an earlier release**. A
pre-existing field already varies among Finale 2008 documents; a field that arrives with Finale 2012 is identically
zero throughout them. Applying that leaves four of the thirteen, and only one is a boolean in a lyric record:

| Candidate | Finale 2012 values | Finale 2008 values | Reading |
|---|---|---|---|
| `24` w120/123/126, `29` w5, `55` w2, `67` w5, `69` w1, `87` w8/w11 | — | already vary | lineage artifacts |
| `41` w15 | 0, −8 | all 0 | a 2012 arrival, but a coordinate |
| `48` w5 | 0, 100 | all 0 | a 2012 arrival, reads as a percent |
| `48` w17 | 0, 127 | all 0 | a 2012 arrival, reads as a mask |
| **`57` w4** | **0, 1** | **all 0** | **a 2012 arrival, boolean, in the lyrics record** |

`lyricPunctuationToIgnore` is **confirmed too, and it is a variable-length tail on the same selector `57` record**.
The corpus could not have found it and said so clearly: the element takes exactly two values across all 1,189
companions — absent in 993 and the stock set `,.?!;:'"“”‘’` in 196 — so not one document customises the list, and the
stock set appears nowhere in any Finale 2012 fixture's records or inflated blocks. **Finale writes the tail only when
the list differs from the stock one**, which is why searching for the default found nothing.

`tests/evidence/F2012/F2012-lyric-punct.mus` settles it: the list set to `#@%&`, four characters sharing nothing with
the stock set, grows the record from twelve bytes to twenty-four. The six scalars are unchanged and the characters
follow as 16-bit code units terminated by a zero:

| Words 0–5 | Words 6+ |
|---|---|
| the six scalars, unchanged | `0x23 0x40 0x25 0x26`, then `0x0000` |

The reader takes the tail from word 6 to the first zero word and converts it with musxdom's own
`EnigmaString::toU8`, so it is agnostic to how the record grows — the observed payload is consistent with expansion
in twelve-byte chunks, and the terminator rather than the chunking is what the decode depends on. Astral characters
would arrive as UTF-16 surrogate pairs and are handled, though nothing exercises that: every character of the stock
set and of the fixture is in the basic multilingual plane.

**The tail has two layouts, and the Unicode release is the boundary.** Finale 2012 stores one 16-bit code unit per
character; **Finale 2011 stores packed 8-bit bytes in the saving platform's code page**. Neither can be read as the
other: both controlled containers are little-endian, so decoding the Finale 2011 byte string through the word path
would transpose every pair of characters.

`tests/evidence/F2011/F2011-lyric-punct.*` settles the older layout and its encoding together, and it exists because
a Finale 2011 installation was obtained for the question — no survey had an authored document of that release, and
0 of the 597 shipped ones carries a tail at all. Its list is `#@%&«»` and its tail is `23 40 25 26 c7 c8`, six bytes
in string order rather than three words. **The last two bytes are what make the code page a measurement rather than
an assumption**: `0xc7 0xc8` is the guillemet pair in Mac Roman and `ÇÈ` in Windows-1252, and the companion reads the
guillemets. The reader takes the bank from the document's own platform, as it does for a font definition carrying no
charset of its own — the only source available, since this text belongs to no font record — and this is the one
specimen anywhere that tests that choice for this field.

**Finale writes the tail on the list, not on the switch.** `F2012-lyropts-noign-punct.mus` turns ignoring off and
leaves the list stock, and its record stays twelve bytes; the tail appears only when the list itself differs.

**A document with no tail keeps the stock list, and the reader does nothing about it.** The pinned baseline states no
`<lyricPunctuationToIgnore>` either, and musxdom's `LyricOptions::integrityCheck` supplies exactly that set for an
empty one, so writing it here would be a second copy of a default musxdom already owns. Two corpus documents ignore
punctuation while carrying no element at all, which is still unexplained and recorded rather than smoothed over.

The fixture also carries an unasked-for change worth keeping: Finale rewrote the word-extension connection table as
the dialog closed, giving five of its nine styles a vertical offset of 5 and a sixth an offset of 1. That makes it a
second non-default specimen for a collection that otherwise rests on one Finale 2006 document.

### Automatic lyric numbering, and a record that states its own layout

**Confirmed.** Automatic lyric numbering arrives with **Finale 2011** — the Finale 2010 lyric dialog has no such
option and the Finale 2011 one does — and its four fields occupy words 6 to 9 of **selector `58`**: the three
`showAutoNumbersOn…` switches and then `lyricAutoNumType`. musxdom's `AutoNumberingAlign` is in the legacy order,
`None` then `Align`, so the type passes through untranslated.

**This one needs no version gate, because the record states its own layout.** Selector 58 is six words in every era
before Finale 2011 and twelve from it on. Every fixed-row fixture carries six; the Finale 2007 class record carries
twelve bytes; the Finale 2011 and 2012 ones carry twenty-four. The installs survey confirms the marker exactly at
the boundary: **all 22 companion-backed Finale 2010 documents carry twelve bytes and all 597 Finale 2011 ones carry
twenty-four.** That contrast with the edge-punctuation setting a few paragraphs up is worth keeping — that field
had to take a version range because its record does not change shape, and the range was wrong for a year of
releases until the manuals corrected it. This field's record changes shape, so nothing has to be inferred about
which release a file came from.

Three booleans and an enum cannot be separated by saves that each move one thing, because four fields need four
distinct signatures. The three tracked Finale 2011 saves carry a binary code instead — the type moves only in the
first, Verses in the first and second, Choruses in the first and third, Sections in all three — so every field has
a unique pattern and none can be mistaken for another.

A document with the short record shows no automatic numbers at all, and the three switches are asserted false on the
same footing as the smart-lyric group. `lyricAutoNumType` is left to the baseline, because the numbering type of a
document that displays no numbers means nothing and the baseline already carries what every companion of every era
shows.

All three saves also add one class-detail record, `0x0455` at cmper 1/1, identical in each. It is not part of this
class — the switches are entirely in selector 58 — and appears to be per-lyric state that numbering being in use
creates. It is noted for whichever class reaches it, not read here.

### Two boundaries that are gates rather than markers, and why

Neither collection has a size that states which layout a file is in, because neither has two layouts: a document
either carries the record or does not. Presence is therefore the instrument throughout, which reaches the
Coda-banner era's Windows documents that state no version at all. **The one place presence is not safe is selector
`55` in the Coda-banner epoch**, where the number is reused by an unrelated option: the Finale 1.0.0 and 2.6.3
fixtures store values such as 16128 and 16448 in it, and one controlled Finale 1.0.0 stem-options save moves its
first two words. Reading that as the connection table would fabricate nine styles out of another option's bytes, so
the epoch is excluded outright and the word count guards it a second time.

Selectors `34` and `35` word 5 are the other qualified cases, and they behave identically. The word exists in every era and is zero in every document before
Finale 2004 and set in every one after, while every companion of every era switches smart hyphens on. That is what
an option arriving with a default of *on* looks like from before it existed, so reading the word on an older
document would turn smart hyphens off for the whole pre-2004 corpus. The reader gates both on selector `57`, which arrives with the same release. The Finale 2.6.3 fixture removes any
doubt about selector `34`: it stores **12** in that word, which is no boolean at all.

Two controlled Finale 2004 saves name the switches. `F2004-lyropts-nosmart-wext.*` moves selector 34 word 5 from 1
to 0 and its companion loses `<useSmartWordExtensions/>`; `F2004-lyropts-needuscore.*` moves selector 57 **word 1**
from 0 to 1 and its companion gains `<wordExtNeedUnderscore/>`. Both are confirmed on record, ETF and companion
together. The Finale 2004 dialog words the underscore setting the opposite way round and unclearly, and Finale had
reworded it to match the modern boolean by Finale 2012; the companion settles the sense rather than the wording, and
the stored value needs no inversion. That also fills the last unexplained word of the selector 57 record — word 1
had been noted as zero everywhere and was once a candidate for a punctuation-list reference — leaving only **word
5** of that record unassigned.

### Why all three smart-lyric switches are forced off before Finale 2004

**Smart hyphens, smart word extensions and the underscore requirement arrive together with Finale 2004**, so all
three are false for an earlier document rather than merely unstated, and the reader asserts them rather than
inheriting the baseline. The words that carry them are zero in every earlier document whether the option is off or
has not been invented yet, so reading them would be reading nothing.

**The reason is about what this reader can produce, and it is worth stating plainly because it is the class's only
deliberate disagreement with Finale 27.** Smart hyphens and smart word extensions are not standalone settings:
each is implemented as smart shapes — hyphen smart shapes for the one, word-extension smart shapes for the other.
Finale 27 manufactures those during its upgrade, which is exactly why every pre-2004 companion comes back with
`<useSmartHyphens/>` and
`<useSmartWordExtensions/>`, and why the pinned baseline switches both on. **This reader does not manufacture
them.** Leaving the switches on would describe a document it has not built — an option claiming a rendering that
nothing in the imported pools can draw. The honest value is false, and a future coverage survey will report a
systematic companion mismatch on every pre-2004 document as a result. That is intended.

The underscore requirement is the quiet member of the group, since baseline and companions already leave it false;
it is asserted on the same footing as `MultimeasureRestOptions::noHorizontalStretch`, because the era's behavior
is known rather than inherited.

### What the Coda-banner epoch recovers, and what remains open

**The Coda-banner epoch recovers nothing from its records for this class**, and that exclusion is intended: it
stores none of the six selectors in a usable form. Its selector `15` word 1 is zero where every companion says 144,
and its selector `55` is a different option entirely. What it does get is the three assertions above, so a
Coda-banner document is not left silently claiming Finale 27 settings.

Eleven fields are not read from any era, and they divide into two quite different cases. Every companion agrees
with the pinned baseline on all eleven, so the fixture set cannot tell them apart on its own; what separates them
is knowing which Finale release introduced each option.

**Three of them postdate Finale 2012 and are therefore settled rather than open**: `hyphenChar`,
`useAltHyphenFont` and `altHyphenFont`. Finale 2012 is the last release this reader opens, so no `.mus` file of any
era has anywhere to put them. All three are nonetheless handled differently from each other, and the differences
are the useful part.

`useAltHyphenFont` is **asserted false and reported as `LegacyBehavior`**, exactly as
`MultimeasureRestOptions::noHorizontalStretch` is, and for the same reason: the pinned baseline also says false,
but the two statements are not the same statement. The baseline saying false is one Finale 27 document's setting,
which a later pinned resource could legitimately change; the setting postdating every legacy format is a fact about
the formats, and it is what makes the value *known* rather than synthesized. `LegacyBehavior` marks a value the
reader asserts on the strength of era knowledge, whether or not the baseline happens to agree — not merely one that
contradicts the baseline.

`hyphenChar` is **left exactly as seeded and reported as `MusxOnly`**. It postdates every supported legacy layout,
so there is no source value to recover. `MusxOnly` distinguishes this settled absence from `Unmapped`, which marks
a field that could have a legacy source whose mapping has not yet been established.

`altHyphenFont` is the one that needed a decision. **It is absent from the pinned `<lyricOptions>` element
entirely**, so the seeded member holds a null pointer, and musxdom populates it only from an `<altHyphenFont>`
element and otherwise synthesizes one in `integrityCheck` — which runs at the end of construction, after every
importer. A null pointer during the import therefore means exactly one thing, that the baseline carried no such
element, **and it means it without reading the baseline's XML**, which no importer has access to in any case. The
reader imports no value, reports the three persisted `FontInfo` leaves as `MusxOnly`, and musxdom fills the member
in afterwards as it does for any document that omits it.

The obvious-looking alternative is wrong and worth naming, because it was tried first. A `FontInfo` the baseline
*did* seed would carry the baseline's font numbering rather than the imported document's and would need
`musx::dom::importFontDefinitionInto` before it named anything here. But a member the baseline never filled in is
not a seeded value needing repair — it is an absent one, and copying the reference document's own synthesized
placeholder into it would put a value in the document that no document ever stated.

`tests/mapping_tests.cpp` pins all three behaviors with a `LyricOptions` seeded to contradict every assertion the
reader makes, `hyphenChar` set to `~` among them. Removing the boolean assertion fails it, and so does hard-coding
the hyphen character; both mutations were checked.

**Eight remain genuinely open**: `useSmartWordExtensions`, `wordExtNeedUnderscore`, `lyricPunctuationToIgnore`,
`lyricAutoNumType`, and the three `showAutoNumbersOn…` flags. There is one lead:
`tests/evidence/F2012/F2012-upstem-flags.mus` is the only document anywhere whose companion omits
`<lyricAutoNumType>`, and it differs from its sibling in no lyric record this study located. The framework's lyrics
preference map names none of the eleven, and explicitly has no field for the alternate hyphen font, which
[`data/legacy_option_font_id_locations.csv`](../../data/legacy_option_font_id_locations.csv) records as
`not_identified` — correctly, and now for a stated reason rather than as an unfinished search.

One correlation is recorded and **not** implemented. In documents older than selector `55`, Finale 27 synthesizes
the starting connection's vertical offset as 1 when the word-extension syllable positioning bit is set and 4 when
it is not — six fixture groups agree, and no other record separates the two cases. Whether that is a real legacy
behavior or an artifact of the converter is unsettled, so the reader leaves those documents at the baseline's 1
and the question stays **open**.
