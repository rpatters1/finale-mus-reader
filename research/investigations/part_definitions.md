# PartDefinition investigations

**Covers:** How the `partDef` class id, its six words, the default-name sign convention, and the absence of any pre-zlib part definition were established.
**Read when:** Proposing a hypothesis about part definitions or linked-part identity, or before re-opening any of the questions below.

## 2026-09-03 — Confirming the class id and locating the six words

**Question.** The class id `0x011a` was ranked `partDef` on whole-corpus count correlation, and
the tracked-evidence catalog ranked the same id `partGlobals`; both classes carry one 12-byte
record per document in a single-part file, so counting cannot separate them. Which is it, and
where do the twelve musxdom members live?

**Method.** `tests/evidence/F2012/F2012-noteartexp.mus` is the smallest tracked document with a
linked part. Its `0x011a` records were read at byte level and set beside the `partDef` elements of
its Finale 27 companion. The mapping was then checked against every reference-corpus document
with more than one part — 303 documents, 2,331 records, from Finale 2007, 2008, and 2012 — by
decoding each record with `tools/investigations/record_dump` and pairing it with the companion
element of the same comparator.

**Result.** `0x011a` is `partDef`: the two records of the controlled fixture carry comparators 0
and 1, which are the score and the one linked part, whereas `partGlobals` is a comparator-65534
class. The layout is recorded in
[`../format/others/part_definitions.md`](../format/others/part_definitions.md). Every mapped
member agrees with its companion in all 2,331 records.

Three members do not follow any stored bit. `needsRecalc` and `useAsSmpInst` are true in the
companion for every linked part and false for every score, and neither has a Finale interface that
could set it independently, so they are derived from `PartDefinition::isScore` and reported as the
era's behavior rather than left unmapped. `unlinkInsts` is true for five records whose twelve bytes
are byte-identical to siblings where it is false; the record is fully accounted for without it.

The five `unlinkInsts` specimens are, by corpus-relative path, `partDef` comparator, and the part
name its companion `nameID` resolves to:

| Release | `corpus_id` | cmper | Part |
|---|---|---:|---|
| 2008 | `mus-0f4eba682cbab3f7` | 14 of 19 | percussion |
| 2008 | `mus-a767fc45617d6277` | 14 of 22 | percussion |
| 2007 | `mus-b3cdc3de73a1be2c` | 14 of 22 | percussion |
| 2007 | `mus-ddd62e03c5ee4505` | 15 of 17 | a named instrumental part |
| 2007 | `mus-ddd62e03c5ee4505` | 16 of 17 | a named instrumental part |

Three of the five are a percussion part, which is the shape a multi-staff part takes; the flag
words are `0x000e` for those three and `0x000b` for the other two, both of which occur widely on
parts where the member is false. The first three are one part carried between two related
documents rather than three independent observations.

**The record cannot be carrying it.** Every one of the 2,331 payloads is 12 bytes, so there is no
longer form and no continuation to hide a flag in. Each of the 96 bits was then tested for a
perfect split against the companion member, in both polarities: none separates the five true
records from the other 2,326. The bits never set in any record — word 3 bits 4-7, 10 and 12-15,
word 2 bits 1-7 and 9-15, and the rest — are clear in the five true records too, so no unset
candidate remains either.

The four consecutive parts of `mus-ddd62e03c5ee4505` show it without statistics:

| cmper | words | `unlinkInsts` |
|---:|---|---|
| 13 | `0163 000d 0001 000b 0000 0000` | false |
| 14 | `0164 000e 0001 000b 0000 0000` | false |
| 15 | `0165 000f 0001 000b 0000 0000` | **true** |
| 16 | `0166 0010 0001 000b 0000 0000` | **true** |

Identical flag word, identical instrument and default-name words, `nameID` and `partOrder` simply
incrementing, and the member flips between the second and the third. Whatever states it is outside
this record: a per-part or per-staff record not yet reached, or a value Finale 27 computes from
the part's staves.

## 2026-09-03 — Two rules fitted to a population that excluded Finale 2011

**Question.** The first all-corpus regression left 34 unexpected differences, every one of them in
this class and none anywhere else in the reader. Why?

**Method.** The 34 fall in seven zlib documents. Three are Finale 2011 tutorial files shipped in the
Finale 2011 installers, which only the installs corpus holds; the reference corpus has no Finale
2011 document with more than one part at all. Their records were read at byte level beside their
companions, and the two rules the earlier sample had produced were then re-tested against **every**
zlib document with a companion in both corpora — 2,027 documents, 4,095 records.

**Result.** Both rules were wrong, and both had been fitted to a population that could not
distinguish them from the truth.

- `needsRecalc` and `useAsSmpInst` track *being a linked part*, not `extractPart`. In all 303
  multi-part reference-corpus documents the two are indistinguishable, because every record with
  the extraction bit clear there is a score record.
  `mus-2651f85d05d8999e`, a tutorial document shipped with Finale 2011, separates them: its parts
  carry flag word `0x0108`, extraction bit clear, and their companions still set both members.
- The SmartMusic instrument word goes live in Finale **2011**, not 2012.
  `mus-7c817bea45f74b2d`, another Finale 2011 tutorial document, stores zero for its parts against
  a companion of zero, where Finale 2007 and 2008 store zero against a companion of `-1`.

The corrected rules produce zero mismatches across all 4,095 records. Bit 8 predicts the instrument
boundary equally well and was considered as a structural marker for it; why the version gate was
kept instead is recorded with the gate in
[`../format/others/part_definitions.md`](../format/others/part_definitions.md).

**The lesson is the sampling, not the rules.** Both hypotheses were supported by thousands of
records and both were wrong, because the release that would have refuted them was missing from the
sample and its absence was invisible. `research/data/surveys.csv` says what each survey can answer:
the reference corpus has the volume and the companions, the installs corpus is the only place
several releases exist at all. A rule fitted to one of them is not established until the other has
seen it.

**Refuted.** Two readings of the flag word were tried and fail. `needsRecalc` is not bit 3: the
Finale 2007 document `mus-ddd62e03c5ee4505` has a part whose flag word is `0x0003`,
bit 3 clear, against a companion `needsRecalc` of true. `useAsSmpInst` is not bit 8: every part of
that same document has bit 8 clear against a companion `useAsSmpInst` of true. Both members would
therefore have to be supplied by the upgrade even if the bits did mean them, which is what makes
the bits undecidable from companion evidence alone.

## 2026-09-03 — The default-name word carries two comparators

**Question.** Twelve bytes leave one word for two musxdom members, `defaultNameStaff` and
`defaultNameGroup`. How does one word carry both?

**Method.** Records whose companion names a group rather than a staff were selected out of the
303-document sample and their last word compared with the companion value.

**Result.** By sign. `0xfff4` pairs with `defaultNameGroup` 12 and `0xfff3` with 13, in the same
documents whose staff-named parts carry ordinary positive comparators, and zero pairs with
neither member being present. A part names a staff or a group or nothing, never both, so one
signed selector is sufficient.

## 2026-09-03 — No pre-zlib document stores a part definition

**Question.** The record was expected to exist before Finale 2007 under the two-character tag
`pD`, and the translation from that tag to the zlib class id was the open question.

**Method.** `tools/investigations/record_dump --tag=pD` over all 3,981 reference-corpus documents
and all 221 tracked fixtures. The lowercase `pd` family, which is present, was then examined
across a 400-document sample and in the `^pd` rows of the tracked ETF exports.

**Result.** No document of any epoch contains a record tagged `pD`. Linked parts are a Finale 2007
feature and Finale 2007 is the first zlib release, so the tag has no era in which it could have
been written; there is no translation to find, and the class id has to be — and now is —
established directly.

`pd` is a different record and not an early spelling of this one. It occurs in 2,249 of the 3,981
corpus documents, always exactly once, always at comparator 0, and its six words are zero except
the last, which holds a packed value in the `0x08xx` and `0x0c3x` range. Read as this layout it
would give every pre-2007 document `copies` of zero and `printPart` false, while every Finale 27
companion of such a document reports one copy and printed. Its meaning is `open` and it has no
musxdom home yet.

**Consequence.** The score part is supplied from the era's own behavior rather than decoded. The
values are confirmed by the companions: all 187 pre-zlib tracked fixtures produce a score part
agreeing with Finale 27 on every member except `nameId`, which the upgrade synthesizes a "Score"
text block for and this reader leaves at zero.
