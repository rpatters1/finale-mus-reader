# TextBlocks investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-22 — TextBlock is stored after Coda and assembled inside Coda

- **Question:** whether public `EDTTextBlock` describes a disk record, and how the same class can
  identify Coda block text when that epoch has no `TX` family.
- **Post-Coda result:** `TX` and zlib class `0x00b7` share the public structure's first twelve
  words. Controlled fixed-row, DCL, and big-endian zlib fixtures reproduce the companions'
  scalar fields, conditional line-spacing member, justification, flags, and high-word-first
  Efix values. The public declaration remains a hint; its first twelve words are now independently
  verified format facts.
- **Coda result:** `EDTTextBlock` is synthetic there. Neither the MUS record index nor the era's
  ETF exports contain `TX`. Each `HS` incidence describes the corresponding ordered `HT` block;
  its low flag bits carry justification, and the companion supplies the era's invariant remaining
  behavior. Text recovery owns allocation of the musxdom text number, and TextBlock recovery reads
  that finished object rather than reimplementing the ordering.
- **Refuted lead:** raw searches initially appeared inconsistent with a direct Coda `TX` record.
  Record dumps and ETF exports settled the issue: there is no such family, and the apparent modern
  record was the companion's synthetic representation.
- **Upgrade behavior:** Finale may insert a shape block, duplicate a page-number text, synthesize
  expression blocks, and assign different modern cmpers to synthetic Coda TextBlocks. Semantic
  text plus layout describes that transformation but is not TextBlock identity. The maintained
  report compares TextBlocks by cmper and consults each side's referenced Enigma text only to
  classify the paired block. Semantically matching referents make a `textId` change equivalent;
  nonmatching referents, zero-id absence, and a combined text-family/id reassignment with one
  resolvable referent classify the block as Finale renumbering and suppress its remaining leaf
  differences. This classification applies across all eras. Text comparison and its detailed
  difference table remain exclusively owned by the texts pool.
- **Broad capture:** the immutable three-survey recovery snapshot contains 15,938 occurrences:
  15,883 imported successfully,
  4,490 had companions, and the represented epochs were 4,247 zlib, 10,302 DCL, 1,028
  uncompressed, 306 Coda-banner, and 55 failed/unclassified. Counts are occurrences unless stated
  otherwise. Among Coda sources, 594 blocks in 83 distinct documents had exact semantic-layout
  matches, 520 with changed IDs; seven blocks in seven documents lacked a semantic match.
- **Follow-up — `textType` is word 12:** Finale 2003 component documents store zero there. Finale
  2004 and 2005 component documents store packed `bl` on block TextBlocks and `xp` on expression
  TextBlocks. The controlled Finale 2006 graphic fixture changes the representation to decimal
  `2004` for all 17 block TextBlocks and `2006` for all 41 expression TextBlocks; the controlled
  Finale 2008 block-only fixture retains `2004`. Thus the user's proposed appended field and
  Finale 2004 boundary are confirmed, while the raw encoding has two generations. The importer
  selects from either recognized stored value rather than from the document version. This
  resolves the expression-text ownership that was previously left for `TextExpressionDef`.
- **Follow-up — corners are legacy behavior, not a decoded location:** Finale 2012 has no
  rounded-corner option, establishing that the feature postdates the legacy MUS era. The
  three-survey capture independently has seven rounded companion TextBlocks, all with radius
  512: six reserved high-comparator objects absent from the source and one expression TextBlock
  added during upgrade. Every companion TextBlock corresponding to a source block has square
  corners and radius zero. The importer now assigns and reports those values as
  `LegacyBehavior`; the discarded `0x2000` and words 13–14 hypothesis was attempting to locate
  fields that the source format never stored.
- **Open Coda reference infrastructure:** the existing shared `HS`/`HT` walk already resolves a
  synthesized TextBlock inward to its BlockText without a separate map. Outward references from
  staff/group names, page or measure text, text expressions, and similar records are different:
  their legacy reference token has not been identified. Those importers will require one
  document-level legacy-token-to-synthesized-TextBlock-cmper map; none may independently count
  blocks or use semantic text as identity. Stored post-Coda `TX` cmpers remain direct identities.
- **Artifacts:** `src/import/others/text_blocks.cpp`,
  `tools/coverage/surveyors/others/text_blocks.cpp`, `tests/mapping_tests.cpp`,
  `tests/text_pool_tests.cpp`, and
  [`text_blocks.md`](../format/others/text_blocks.md#textblock-attributes).
