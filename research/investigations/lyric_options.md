# LyricOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-17 — LyricOptions: six selectors, four arrival dates, and eleven fields nobody stores

- **Question:** Where does each field of `LyricOptions` live, and how much of it does the distilled framework's
  `LyricsPrefs` group actually cover?
- **Method:** Dumped the six candidate selectors from the 69 tracked fixtures with `tools/record_dump` and compared
  each against the fixture's exact Finale 27 companion, working outward from the zlib era as the most recognizable
  representation. Read the framework's `_FCEDTLyricsPrefs` struct and `FCLyricsPrefs` accessors read-only for the
  numberings the corpus could not supply. **No corpus survey was run**; every claim below rests on the fixtures.
- **Observation — the layout.** Six numeric globals at `65534`, none of them a direct block in the framework's
  sense, and each arriving at a different release: `15` (word 1, hyphen separation) and `67` (word 5, line width)
  from Finale 3.x; `87` (four syllable positions over two incidences) from Finale 2000; `55` (nine word-extension
  connection styles over five incidences) and `57` (three smart-lyric scalars) from Finale 2004; `35` word 5
  (smart hyphens) usable only from Finale 2004 though the record exists in every era. The zlib era coalesces each
  through the usual `numericGlobalClass` rule, and both byte orders are exercised by the Finale 2007 and 2012
  fixtures.
- **Observation — three orderings, none of which matches musxdom.** The syllable alignment list is
  `1 = centre, 2 = left, 3 = right` (the framework's `LYRICS_ALIGN_` constants, corroborated by every fixture for
  1 and 2 and by no fixture at all for 3). The connection point runs `0x10`–`0x15` on the *smart-shape entry
  connection* scale and orders lyric/head/dot/duration/systemLeft/systemRight, where musxdom puts the two system
  attachments third and fourth. Bit 15 of each position's flags word is musxdom's `on`.
- **The two decisive fixtures were already in the tree.** `F2006-embedded-tif.mus` carries all six connection
  numbers with five distinct vertical offsets and a horizontal offset of 8 where every other fixture has 4; its
  companion names each point beside the same offsets, which fixes the element layout, the whole numbering, and the
  fact that `wordExtHorzOffset`/`wordExtVertOffset` are the starting connection's own offsets rather than separate
  fields. `F2000-multilayer.mus` is the one document that clears the 0x8000 bit for the three optional positions,
  and the one whose companion omits their `<on/>`.
- **Why presence rather than a version, and the one place presence is unsafe.** Neither collection has two layouts,
  so nothing states a layout the way the multimeasure-rest word count does; a document either carries the record or
  does not, and presence reaches the Coda-era Windows documents that state no version. The exception is selector
  `55`, which the **Coda-banner era reuses for an unrelated option** — the Finale 1.0.0 and 2.6.3 fixtures store
  16128 and 16448 in it, and a controlled 1.0.0 stem-options save moves its first two words. That epoch is excluded
  outright, with the word count as a second guard. Selector `35` word 5 is the other qualified case: it is 0 in
  every pre-2004 document and 1 after, while every companion of every era says smart hyphens are on, which is what
  an option arriving switched on looks like from before it existed. Reading it early would have switched smart
  hyphens off for the whole pre-2004 corpus.
- **Conclusion: partial, and the negative half is the interesting half.** Twelve fields and both collections are
  recovered; three more are asserted as `LegacyBehavior` (the optional syllable positions off before selector `87`,
  the line width 224 before selector `67`, edge punctuation not ignored before Finale 2012). **Eleven fields are
  read from no era**, and they are invariant across every companion *and* absent from the framework tree, so
  neither source can separate them. The **Coda-banner epoch recovers nothing from its records for this class**,
  which is intended: it stores none of the six selectors usably.
- **Three of the eleven were then settled without a fixture, by the repository owner supplying a version
  boundary.** `hyphenChar`, `useAltHyphenFont` and `altHyphenFont` all postdate **Finale 2012**, the last release
  this reader opens, so no `.mus` file of any era has anywhere to put them. Nothing is read for them and nothing is
  overwritten, so they keep the baseline's values and are reported as `Finale27Default`. This is the second time a
  boundary the owner knew has closed a question the corpus could only ever have shown as "nothing varies" — which
  is what an absent option and an unfound one look like alike. Eight remain genuinely open.
- **Three owner corrections, and the rule they converge on.** `ValueOrigin::LegacyBehavior` marks a value the
  reader *asserts* on the strength of era knowledge — whether or not the pinned baseline happens to agree, because
  the baseline states one Finale 27 document's setting while the assertion states a fact about the formats.
  `Finale27Default` marks a value the reader inherits because it has nothing better. What decides between them is
  whether the reader writes the value, not whether the value differs from the baseline; `noHorizontalStretch` has
  always been the worked example and agrees with the baseline.
- **But a value the reader cannot state without duplicating the baseline stays inherited.** `useAltHyphenFont` is
  asserted false, because a boolean that is false through the non-existence of its feature can be written in code
  without restating anything. `hyphenChar` is not: writing U+002D beside a pinned resource that already says 45
  would be a second copy of one fact, which is the case the repository's rule is aimed at. So the two post-2012
  fields land on opposite sides — asserted `LegacyBehavior` and seeded `Finale27Default` — and the line between
  them is "can this be said without repeating the resource", not "is this known".
- Second, **`altHyphenFont` needed no work at all, and the first attempt at it was wrong.** It was built from the
  reference document's object and its comparator remapped through `musx::dom::importFontDefinitionInto`, which
  produced plausible results — each fixture resolving to its own music font, Petrucci in the Finale 1.0.0 and 3.7.2
  documents, Pmusic in the Finale 2.6.3 one, Maestro from Finale 2005 on. But the pinned baseline carries no
  `<altHyphenFont>` element either, so what was being copied was the reference's *own* `integrityCheck` placeholder:
  a value no document ever stated, imported into a document that had not asked for it. The reader now declines, and
  musxdom synthesizes the member after construction as it does for any document that omits it.
- **The detection question that settled it is reusable.** A seeded sub-object member is null during the import
  exactly when the baseline omitted its element, because musxdom populates such a member only from the element and
  otherwise synthesizes it in `integrityCheck`, which runs at `finish()` — after every importer. So "did the
  baseline state this?" is answerable from the pointer alone, with no access to the baseline's XML, which no
  importer has anyway.
- **Loose end deliberately not implemented:** in pre-2004 documents Finale 27 synthesizes the starting connection's
  vertical offset as 1 when the word-extension positioning bit is set and 4 when it is not. Six fixture groups agree
  and nothing else separates them; whether that is legacy behaviour or converter invention is **open**, so those
  documents keep the baseline's 1.
- **Next evidence:** [L1](../state/EVIDENCE_REQUESTS.md) — one Finale 2005/2006 save moving as many of the eight still
  unlocated Lyric Options fields at once as the dialog exposes, after checking which of them that era's dialog
  offers at all: any it does not is a candidate for the same after-2012 answer the three struck-off fields got.
  Then a corpus survey, which this class has not had.
- **Artifacts:** `src/import/options/lyric_options.cpp`, `tests/reader_tests.cpp`,
  [`lyric_options.md`](../format/options/lyric_options.md#lyric-options),
  [`data/lyric_options_mapping.csv`](../data/lyric_options_mapping.csv).

## 2026-08-17 — Syllable edge punctuation, and the lineage confound that mimicked nine flags

- **Question:** How far back does "Ignore Syllable Edge Punctuation" go, and where does a file store it? The owner
  reported that Finale 2012 has the setting and Finale 2000 does not, leaving 2001-2011 unaccounted for.
- **Method:** Extracted `<lyricUseEdgePunctuation/>` and `<lyricPunctuationToIgnore>` from all 1,189 adjacent-exact
  companions of the reference corpus, tabulated by saving product, then searched the record stream of the cohort
  that varies for any word or bit partitioning it exactly. Deliverables in
  `private/generated/rpatters1-main/class_coverage/lyric_edge_punctuation/`.
- **Observation — the boundary is Finale 2012 itself.** Every release from Coda-banner through **Finale 2010**
  converts with punctuation not ignored, 941 documents with no exception, which extends the owner's Finale 2000
  observation by ten releases. Only Finale 2012 varies: 50 not ignored against 198 ignored. Finale 2011 is absent
  from this corpus; the installs survey holds 1,295 such documents and has not been run for this class.
- **Observation — the search returned thirteen exact answers, and nine were wrong.** Any bit partitioning the 248
  Finale 2012 documents is a candidate, and thirteen do, spread across fonts, stems, clefs and beams. The split is
  not about punctuation but about **document lineage**: the 198 that ignore it are born-in-2012 documents carrying
  that release's new defaults, and the 50 that do not are upgrades from older files. Already-mapped fields prove it,
  because `wordExtLineWidth`, the syllable positioning bits and the starting connection's vertical offset all
  partition the cohort identically and none of them is this setting.
- **What broke the confound was a negative control in an earlier release.** A pre-existing field already varies
  among Finale 2008 documents; a field arriving with Finale 2012 is identically zero throughout them. That cut
  thirteen candidates to four, of which only one is a boolean and only one sits in a lyric record: **selector `57`
  word 4**, class `0x0047` byte 8, the fourth field of the row already holding `smartHyphenStart`,
  `wordExtMinLength` and `wordExtOffsetToNotehead`. The other three survivors are a coordinate and two values
  reading as a percent and a mask.
- **Conclusion: confirmed.** The reader recovers the word from Finale 2012 and asserts the era's behaviour before
  it. Verified end to end through the public reader over the whole companion-backed corpus: **1,189 of 1,189 agree,
  zero read failures**, including all 248 Finale 2012 documents. This also corrects a real defect rather than only
  improving provenance -- the previous version gate left every Finale 2012 document at the baseline's *ignored*,
  which was wrong for the 50 that do not ignore.
- **Why it stays a version gate.** The record is twelve bytes in Finale 2007 and Finale 2012 alike, so its shape
  states nothing and only the release distinguishes them. The gate is bounded inside the zlib epoch and fails closed
  onto the pre-2012 behaviour, which is the right answer for every release but one.
- **Incidental:** the same sweep confirms selector `57` arrives with Finale 2004 across 941 documents, a gate
  previously resting on a handful of fixtures.
- **Follow-up, same day: `lyricPunctuationToIgnore` closed by the requested fixture, which refuted the prediction
  it was requested on.** The request predicted a cmper in selector 57 word 1 or 5, the two words zero in all 1,189
  companion-backed documents. It is instead a **variable-length tail on the selector 57 record itself**: with the
  list set to `#@%&` the record grows from twelve bytes to twenty-four, six scalars unchanged, then the characters
  as 16-bit code units and a zero. Finale writes the tail **only when the list differs from the stock set**, which
  is the whole reason the corpus was silent and the stock set appeared in no Finale 2012 file. An absent tail means
  the stock list, and the reader does nothing about it because musxdom's `integrityCheck` already owns that default.
  The decode reads word 6 to the first zero, so it is agnostic to how the record grows; the owner's guess that it
  expands in twelve-byte chunks fits the specimen, and the terminator rather than the chunking is what the decode
  depends on.
- **The fixture paid for itself twice.** Finale rewrote the word-extension connection table as the dialog closed,
  giving five of nine styles a vertical offset of 5 and a sixth an offset of 1 — a second non-default specimen for a
  collection that otherwise rested on one Finale 2006 document. It also renumbered two font definitions. Neither was
  asked for, and both are recorded in the fixture's provenance rather than treated as noise.
- **Follow-up: the checkbox-cleared save arrived and confirmed the corpus prediction exactly.**
  `F2012-lyropts-noign-punct.mus` moves byte 8 of class `0x0047` from 0 to 1 and moves no other word of that record,
  and its companion gains `<lyricUseEdgePunctuation/>`. This is the rarer kind of confirmation: the mapping was
  derived from 1,189 companions and a negative control *before* any fixture could exercise it, and the controlled
  save then landed on the predicted byte. It is also the only published document anywhere with the switch cleared,
  so the word-set path now has a real fixture instead of a synthetic record.
- **The pair separates two things that looked like one.** Ignoring is off in the new fixture and its list is stock,
  so its record stays twelve bytes, while the earlier fixture keeps ignoring on and grows to twenty-four. Finale
  writes the tail on the **list** differing from stock, not on the switch being touched.
- **Next evidence:** two corpus documents ignore punctuation while carrying no `<lyricPunctuationToIgnore>` element,
  which neither fixture explains. A Finale 2012 save clearing the list entirely, with ignoring left on, would say
  whether an empty list is representable.
- **Artifacts:** `src/import/options/lyric_options.cpp`, `tests/reader_tests.cpp`, `tests/mapping_tests.cpp`,
  [`lyric_options.md`](../format/options/lyric_options.md#lyric-options),
  [`data/lyric_options_mapping.csv`](../data/lyric_options_mapping.csv).

## 2026-08-17 — Lift and Push, and a six-group correlation that was a coincidence

- **Question:** The Coda-banner epoch recovered nothing at all for `LyricOptions`. The repository owner reported
  that its dialog exposes exactly two lyric settings, word extension "Lift" and "Push", and that Finale 3.7.2 adds
  hyphen spacing and line thickness to them. Where are they?
- **Method:** Two controlled one-variable saves, `F100-wext-push-6-lift-5.mus` and `F372-lyricopts-changed.mus`,
  each with a Finale 27 companion and an ETF, diffed at record granularity against their baselines.
- **Observation:** Lift is **selector `29` word 5** and Push is **selector `30` word 5**, and both exist in *every*
  epoch including the Coda banner. The Finale 1.0.0 pair moves those two words and no other word in the file; its
  companion reads 5 and 6 and its ETF prints the same two rows. The Finale 3.7.2 pair moves four words -- adding
  selector `15` word 1 for hyphen spacing and selector `67` word 5 for line thickness -- and its companion agrees
  with all four. Every previously tracked fixture agrees with its companion on both fields across all four epochs.
- **Conclusion: confirmed on three independent representations**, and the Coda-banner epoch now recovers exactly
  what its dialog exposes rather than nothing. The Finale 3.7.2 save is also the only tracked document anywhere
  that varies the hyphen separation, which promotes selector `15` word 1 from consistent-everywhere to confirmed.
- **A recorded correlation was a coincidence, and this is the lesson worth keeping.** Pre-Finale-2004 documents
  whose companions show a vertical offset of 1 rather than 4 had looked as though the value tracked the
  word-extension syllable positioning bit: six fixture groups agreed and no other record separated them. It was
  never a rule. Those documents store 1 in selector 29. The correlation was convincing for exactly as long as the
  real field was missing, and it was retired by a controlled save in the era with the fewest settings to confound
  it -- not by a better test on the same data.
- **It also replaced a derivation with a read.** The class-level offsets had been taken from selector `55`'s first
  element, which does not exist before Finale 2004; they now come from 29 and 30 in every era, and where selector 55
  is absent the starting connection takes those values, as Finale 27 does.
- **One synthesis deliberately not reproduced:** the companion moves the `oneEntryEnd` element's horizontal offset
  with Push, 42 to 44. A single specimen cannot distinguish that formula from others that fit, so the baseline's 42
  stands and the difference is intended.
- **Unexplained and recorded rather than smoothed over:** the Finale 3.7.2 save also moves selector `13` word 1 from
  1024 to 4096, which belongs to no field this reader maps, and respaces the lyric baseline details from 40 to 48.
- **Follow-up: the syllable positioning table closed too, and in two steps neither of which was a guess.** Finale
  2000 is the first release with a dedicated Lyric Options dialog, and that table is exactly what it adds to the
  four settings Finale 3.x already had. Two questions were open about it. The **order** of the last two positions
  was untested, because `first` and `systemStart` carry identical values in almost every document; five
  reference-corpus documents differ, and all five confirm the order in both directions, so no fixture was needed.
  The **`right` member** could never have been settled by a corpus at all: across all 1,189 companion pairs not one
  document uses it as an alignment or a justification, for any position.
  `F2000-lyropts-align-just.mus` supplies it twice over -- align 3 on the first syllable, justify 3 on the system
  start -- so a mapping that translated only one of the two fields would fail on it. Against its parent the only
  options record that moves is selector 87's second incidence. **All three members of the legacy alignment list are
  now verified against Finale's own conversion.**
- **A reminder that a corpus and a fixture answer different questions.** The order was a corpus question, because
  it needed documents that happened to disagree, and five existed among 1,189. The `right` value was a fixture
  question, because no number of documents helps when nobody ever chose the setting. Asking which kind a question
  is, before reaching for either tool, is the cheap step.
- **Artifacts:** `src/import/options/lyric_options.cpp`, `tests/reader_tests.cpp`,
  [`lyric_options.md`](../format/options/lyric_options.md#lyric-options),
  [`data/lyric_options_mapping.csv`](../data/lyric_options_mapping.csv).

## 2026-08-17 — The edge-punctuation boundary was Finale 2011, and the corpus never said 2012

- **Question:** The reader gated syllable edge punctuation at Finale 2012. The repository owner found release notes
  saying the new lyrics features arrived in **Finale 2011**, which would make the gate wrong and, worse, would put
  the punctuation list in a pre-Unicode release.
- **Method:** Corroborated online against MakeMusic's own manuals before looking at any data, at the owner's
  direction, then ran the Finale 2011 cohort of the installs survey — the population the reference corpus lacks.
- **Observation — the manuals bracket it from both sides.** The Finale 2010 Document Options-Lyrics dialog has
  neither "Ignore Syllable Edge Punctuation" nor a "Punctuation to Ignore" field; the Finale 2011 dialog has both,
  plus "Automatic Lyrics Numbers"; and the Finale 2012 manual's "Finale 2011 Interface Changes" page says the
  punctuation feature arrived in 2011 outright. The same page records that "Create Automatically When Notes Follow
  Without Lyrics" was renamed "Only Create on Lyrics with Underscores", which is the rewording the owner had
  described. The Finale 2011 What's New page independently confirms **Smart Hyphens and Word Extensions arrived in
  Finale 2004**, which is the boundary this reader already had from fixtures.
- **Observation — the installs survey confirms it at exactly that line.** All 22 companion-backed Finale 2010
  documents carry 0 in selector 57 word 4; all 597 companion-backed Finale 2011 documents carry 1. The gate moved
  to major 16.
- **How the wrong boundary got in, which is the part worth keeping.** The reference corpus contains **no Finale
  2011 document at all**. It has Finale 2010 and Finale 2012 and nothing between, so "the boundary is Finale 2012"
  was an interpolation across a gap presented as a measurement. The gap had even been written down at the time --
  the notes said "Finale 2011 is absent from this corpus" -- and the gate was still coded to the nearest release
  that happened to be present. A corpus that is silent about a release does not say the release is on either side
  of a line, and silence is easy to read as evidence when every document that *is* present agrees.
- **The punctuation list could not follow the switch, until the owner installed Finale 2011 and made the specimen.**
  0 of the 597 shipped Finale 2011 documents carries a tail, so the release's encoding was unknowable from any
  survey. `F2011-lyric-punct.mus` settles it: **the tail is packed 8-bit bytes in the platform code page**, where
  Finale 2012 writes one 16-bit code unit per character. Both containers are little-endian, so the two layouts are
  not variants of one rule -- reading the 2011 byte string through the word path transposes every pair of
  characters.
- **The two non-ASCII bytes are what turn the code page from an assumption into a measurement.** The list is
  `#@%&«»`; the tail is `23 40 25 26 c7 c8`; `0xc7 0xc8` is the guillemet pair in Mac Roman and `ÇÈ` in
  Windows-1252, and the companion reads the guillemets. The reader takes the bank from the document's own platform,
  as it does for a font definition carrying no charset, and this is the only specimen anywhere that tests that
  choice for this field. A fixture with an ASCII-only list would have confirmed the packing and left the encoding
  exactly as open as before.
- **A second corpus generalization corrected by the same fixture.** All 597 shipped Finale 2011 documents carry the
  switch set, and reading that as the release's default would have been wrong: `F2011-baseline.mus`, created new in
  Finale 2011, carries it clear. The 597 are Finale's own sample content, authored earlier and converted into the
  release, and conversion switches ignoring off to preserve the older look -- the same behaviour the Finale 2012
  cohort's 50 upgraded documents show. A corpus of one vendor's shipped content is not a sample of what a release
  writes for a new document.
- **Automatic lyric numbering, located the same day by three coded saves.** Neither survey could reach it --
  `lyricAutoNumType` is `align` in all 597 Finale 2011 documents and the three `showAutoNumbersOn...` flags are set
  in no document of any era, anywhere -- so it was always going to be a fixture question. Three booleans and an enum
  cannot be separated by saves that each move one thing, because four fields need four distinct signatures, so the
  three saves carried a binary code instead: the type moves only in the first, Verses in the first and second,
  Choruses in the first and third, Sections in all three. All four fields fell out unambiguously in **words 6 to 9
  of selector 58**.
- **And that record states its own layout, so it needs no version gate.** Selector 58 is six words in every era
  before Finale 2011 and twelve from it on; the installs survey splits at exactly that line, twelve bytes in all 22
  companion-backed Finale 2010 documents and twenty-four in all 597 Finale 2011 ones. The contrast with edge
  punctuation in the same class is the useful part: that field had to take a version range because its record does
  not change shape, and the range was wrong for a year of releases until the manuals corrected it. A record that
  changes shape asks nothing about which release wrote it.
- **Artifacts:** `src/import/options/lyric_options.cpp`, `tests/mapping_tests.cpp`,
  [`lyric_options.md`](../format/options/lyric_options.md#lyric-options),
  [`data/lyric_options_mapping.csv`](../data/lyric_options_mapping.csv).
