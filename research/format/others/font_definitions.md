# FontDefinition

**Covers:** The font table, unresolvable comparators, the `Missing Font (n)` placeholder, and companion face comparison.
**Read when:** Working on fonts, font ids, or `companion-face-missing` diagnostics.
**Confidence:** confirmed.

## Font definitions

**Confirmed** against the controlled Finale 2002 fixture and its ETF, and applied to all 669
pre-zlib corpus files, which yield 12,759 definitions with no empty name.

A font definition is one `FN` family in the others pool, comparator equal to the font id used
throughout the document.

| Incidence | Contents |
|---|---|
| 0 | header: character set word, then pitch and family word, then four unused words |
| 1 onwards | the font name, as many rows as it needs |

The header's first word packs the character set: the high nibble is the bank, 1 for a Mac font and
2 for a Windows font, and the remaining twelve bits are the character set value. The bank describes
where the *font* came from, not where the document was written. The values agree with musxdom's own
documentation of the field, which records `0xfff` as the macOS symbol character set and 2 as the
Windows one. The fixture stores `0x1fff` for Maestro and `0x1000` for Times, and its ETF prints the
same headers as `^FN(0) 8191 0 0 0 0 0` and `4096`.

The header's second word carries the pitch in its low nibble and the family in its high nibble.
Across 7,622 header incidences it is non-zero only for Windows-bank fonts: all 7,355 Mac-bank fonts
store zero, while Windows-bank fonts take pitch 1, 2, or 7 and family 0, 1, 2, 4, or 5. Family 4
selects exactly the script and blackletter faces in the corpus, which is independent support for
the nibble boundary. The remaining four header words are unused; not one is non-zero anywhere in
the corpus.

The name is read as bytes, not as words, because character payloads are not byte-order sensitive,
and it ends at the first NUL. Rows are fixed width, so a name shorter than the row is padded and a
longer one simply continues into the next incidence.

**Before Finale 3.2 there is no header incidence**: the family opens with the name and no character
set is recorded. Observed in every Coda-banner file and in the Finale 3.0 files, against Finale 3.2
and later where the header is always present. The exact boundary is **open**: the corpus holds no
Finale 3.1, and its only Finale 3.0 files are Windows-origin, so a platform explanation cannot be
excluded.

### Unresolvable comparators and the `Missing Font (n)` placeholder

**Confirmed.** Finale names a font it cannot resolve with a placeholder definition called
`Missing Font (n)`, where `n` is the comparator itself. Across 2,756 Finale 27 companions the record
is invariant in all 641 occurrences spanning 354 documents: bank `Mac`, `charsetVal` 0, `pitch` 0,
`family` 0, and the name formed from the comparator in every single case. It does not vary with era,
product, or comparator. The commonest comparators are 131, 512 and 100, at 193, 193 and 182
occurrences.

**It is not a conversion artifact.** Finale wrote the same placeholder into the legacy font table at
save time as far back as Finale 2009, so most documents carrying one recover it as `legacy-mus` and
agree with the companion with nothing done on our side. Finale 27 applies the rule unconditionally,
including to comparators that are plainly garbage: one corpus document has a corrupt font table whose
comparators decode as ASCII character pairs — 12596 (`"14"`), 25203 (`"bs"`), 26990 (`"in"`) and
similar — and each one still receives a correctly formed placeholder.

musxdom synthesizes the same record for any comparator registered during construction that the
finished document does not define. That is what keeps `FontInfo::getName` — and `calcIsSameTypeface`
and `calcIsSMuFL`, which route through it — from throwing on a dangling reference. The reader's whole
obligation is to register the comparators it leaves in the document, and to register **what a field
finally holds rather than every value it passes through**: `FontOptions` replaces a recovered
comparator whose definition is absent, and registering the discarded one would mint a placeholder
that nothing refers to. musxdom also registers font ids off `SetFont` instructions in every
`ShapeDef` once the document is built, which the reader does not need to duplicate.

Two Finale `14.0.0.11` documents reach this through the accidental-symbol inserts: their flat and
dblSharp inserts name comparator 100, the source's own table defines fourteen fonts and none of them
is 100, and the placeholder now makes the reader agree with the companion exactly where it
previously left the reference dangling.

### A shape naming a font the source never defines — third deliberate disagreement

**Confirmed**, on five DCL documents, all Finale 2002 `7.0.1.2`. Take `mus-0bb3c333c0f80358`. Its
`FN` families run 0–13 and 22 and stop there. Shapes 14 and 15 are custom tab clefs whose `SetFont`
instruction names comparator **14**, which the file never defines — a dangling reference in the
source document itself, not something conversion introduced.

Finale 27 resolves it, but only by accident. Its converter prunes the source's duplicate font
entries — 0 and 6 are both `Pmusic`, 3 and 10 both `Symbol`, 8 and 13 both `Petrucci`, 9 and 11 both
`Sonata` — and injects its own defaults into the vacated numbers, which lands `Maestro Percussion`
at comparator 14. The shape's dangling reference then silently resolves to it. The strings `Maestro`
and `Engraver` appear nowhere in the source file's records, so those faces are Finale 27's, not the
document's.

That resolution is wrong on the merits. Both shapes draw plain ASCII at 12 point — shape 14 emits
`T`, `1`, `2`, then `-`, space, `0`; shape 15 emits `T`, `2`, then `-`, space, `2` — so the font
wanted is a text face, reported to be Helvetica or Arial with Times for these symbols, and never a
percussion music font. Finale 27 renders tab-clef letterforms as percussion glyphs. The reader keeps
the comparator as stored and lets it read `Missing Font (14)`, which is both truthful and safer to
render, since a name that resolves to nothing falls back to a system face and still produces letters.

**This is the one class of companion difference where matching Finale 27 would be the defect.** It is
recorded so a later coverage run does not re-open it as a regression.

### Reading `companion-face-missing`

That metric counts faces the companion has and the reader does not, and it must **not** be read as a
recovery deficit. Across the reference corpus it is dominated by faces Finale 27 injects during
conversion rather than anything the source named: of 2,956 occurrences over distinct documents,
`Lucida Grande` (1,065), `Engraver Text T` (799), `Times New Roman` (449) and `Maestro` (327) are
2,640 of them, or 89%. Not reproducing those is correct — the reader does not seed font definitions,
because a pinned definition would collide with the source record sharing its comparator.

The interesting residue is small: 165 occurrences of `Missing Font (100)` where the companion carries
a placeholder and the reader does not. Those are comparators referenced only from option classes the
reader has not imported yet, so nothing registers them and no placeholder is minted. The count should
fall as those classes land, and it is a coverage measure rather than a font-table one.
