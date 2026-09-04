# PartDefinition

**Covers:** The `partDef` class record, its six words, the packed flag word, the signed default-name word, the SmartMusic instrument boundary, and the score part every pre-zlib document acquires.
**Read when:** Working on part definitions, linked parts, or interpreting their coverage numbers.
**Confidence:** `confirmed` for the six words and the three mapped flag bits, and for `unlinkInsts` being absent from the record; `strong` for the SmartMusic instrument boundary and the two derived members, each verified against 4,095 records of every zlib release.

## Only the zlib epoch stores the record

**Confirmed.** Linked parts arrived with Finale 2007, which is the release that also moves the
container to zlib, so this class has exactly one physical layout and it is a class record. No
document of any earlier epoch stores a part definition: `tools/investigations/record_dump`
finds the tag `pD` in none of the 3,981 reference-corpus documents and none of the 221 tracked
fixtures. See [`../../investigations/part_definitions.md`](../../investigations/part_definitions.md)
for the lowercase `pd` family, which does occur throughout the pre-zlib eras and is a different
record.

| Epoch | Identity | Addressing |
|---|---|---|
| Coda banner, uncompressed, DCL | none | the score part below is supplied instead |
| Zlib | class id `0x011a` | 12-byte payload, six words |

The comparator is the part id: zero is the score, and a linked part's own id counts up from one.
Every record is owned by the score part (`part` zero), so no part-scoped or continued form of the
class has been observed.

## The six words

**Confirmed** against `tests/evidence/F2012/F2012-noteartexp.mus` and its two siblings, and
against every zlib document in the reference and installs corpora that has a Finale 27 companion —
2,027 documents, 4,095 records — which agree with their companions on every mapped member. The layout is unchanged from Finale 2007
through 2012 and in both byte orders; the payload is 12 bytes in all 905 records sampled at byte
level.

| Byte | musxdom member | Notes |
|---:|---|---|
| 0 | `nameId` | `TextBlock` comparator of the part's name, zero when it has none |
| 2 | `partOrder` | position in Finale's part list; the score is zero |
| 4 | `copies` | number of copies to print |
| 6 | — | packed flags, below |
| 8 | `smartMusicInst` | Finale 2011 and later only, below |
| 10 | `defaultNameStaff` / `defaultNameGroup` | one signed selector, below |

### The flag word

**Confirmed** for bits 0 through 2 across all 2,331 sampled records; bits 3 and 8 are `open`.

| Bit | Mask | musxdom member |
|---:|---:|---|
| 0 | `0x0001` | `printPart` |
| 1 | `0x0002` | `extractPart` |
| 2 | `0x0004` | `applyFormat` |

Bit 3 is set in most records of every release and clear in a minority, and no musxdom member
follows it. Bit 8 is set on every Finale 2011 and 2012 linked part and on nothing else, so it
covaries exactly with the SmartMusic instrument boundary below without being distinguishable from
the release itself.

### The default-name selector

**Confirmed.** The word at byte 10 names one object of either of two kinds by its sign, which is
how one word carries the two comparators musxdom keeps apart. A positive value is a `Staff`
comparator and becomes `defaultNameStaff`; a negative value is a `StaffGroup` comparator negated
and becomes `defaultNameGroup`; zero is neither. The reference corpus supplies both — `0xfff4`
against a companion `defaultNameGroup` of 12, and `0xfff3` against 13 — in documents that also
carry ordinary positive staff selectors.

### The SmartMusic instrument

**Strong.** Finale 2007 and 2008 store zero in the word at byte 8 for the score and for every
linked part alike, while Finale 27 reads `-1` for those parts. Finale 2011 and 2012 store what
Finale 27 then reads back: zero from 2011, `-1` from 2012. The reading is that the word carries the
instrument only from Finale 2011, and that a linked part of an earlier release had none, which is
what `-1` spells; the reader therefore supplies `-1` for a pre-2011 linked part as that era's own
behavior.

The gate is a version gate inside one epoch because nothing in the record states which layout it
uses. Bit 8 covaries with the boundary exactly across all 4,095 records, but a bit is not a
structural marker: a marker is a data item that is absent, whereas this is a value whose meaning is
inferred from the same corpora it would be gating, and it may record something else that merely
arrived in the same release.

## The score part every era has

**Confirmed** for the values, `open` for the name. musxdom requires the score part to exist, and
Finale 27 gives a document of any era exactly one: `partOrder` zero, one copy, printed, and a name
text block. The reader supplies those three as the era's own behavior wherever the source stores
no record for comparator zero — which is every pre-zlib document, and any document whose record
pool failed to frame.

The name is the era's behavior too, and the difference from the companion is deliberate. No
release that wrote a pre-zlib document had a score name object at all — a score acquired one only
when linked parts arrived — so a null reference is what the era held rather than a value this
reader failed to locate. Finale 27's upgrade synthesizes a `TextBlock` holding the literal word
"Score" and points `nameID` at it, allocating whichever comparator is next free, so the companion
value varies by document. This reader does not invent text the source never stored.

Every pre-zlib tracked fixture therefore shows one companion difference on this member and no
other, classified `synthesized-score-name`. The rule is scoped to the score part alone, to a
reader value of zero against a non-zero companion, to `legacy-behavior` provenance, and to the
epochs that store no part definition, so it cannot mask a recovered `nameId` that disagrees.

Finale usually allocates a fresh text block for that name, which the reader simply does not have.
Occasionally it reuses an empty block the source already carries, and then the same synthesis
appears as a *text* difference on a block both sides have: the reader's text is the font commands
the source stores, the companion's ends in `Score`. That case is classified `synthesized score
name` in the text pool. It is deliberately not treated as a part name the reader recovered
wrongly — the reader's score part names no text at all — and the two tests are mutually exclusive
for that reason.

## Members the extraction bit decides

**Strong.** `needsRecalc` and `useAsSmpInst` have no stored bit and no Finale interface of their
own. Finale 27 reports both true for every linked part and false for every score, across all 4,095
records. Being a linked part is therefore what they are, and the reader derives both from
`PartDefinition::isScore` and reports them as the era's behavior: a part Finale can extract is a
part whose layout it recalculates and whose staves it offers as a SmartMusic instrument. Deriving
them rather than reading them is what keeps a bit later found to carry one of them from being
silently overridden.

**Extraction is not what they are**, though it looks that way in an authored document: every
linked part in the reference corpus is also an extracted one, so the extraction bit and being a
linked part are indistinguishable there. Finale's own tutorial files separate them — a part created
but never extracted carries both members with the extraction bit clear — which is why this needed a
corpus holding installer content.

## The member this record does not carry

**Confirmed absent from the record; its own source, if it has one, is elsewhere.** The question of
where `unlinkInsts` lives is not open in this class: the record has been searched exhaustively and
does not hold it. `unlinkInsts` is true for five parts of four reference-corpus
documents, and the companion difference each one produces is classified `possibly-unrecoverable`:
the reader cannot presently reach the value, and *possibly* rather than *is* because it may be
stored in a record not yet decoded. Every part definition payload is 12 bytes — 2,331 of 2,331 — with no longer form and no
continuation, and **no single bit of the 96 separates those five records from the other 2,326**, in
either polarity; the bits that are never set in any record are clear in the five as well. Four
consecutive parts of one document make the point without statistics: identical flag words,
identical instrument and default-name words, `nameID` and `partOrder` merely incrementing, and the
member flips between the second and the third. It is therefore stored outside the class, or
computed by Finale 27 from the part's staves. The specimens and the bit search are in
[`../../investigations/part_definitions.md`](../../investigations/part_definitions.md).
