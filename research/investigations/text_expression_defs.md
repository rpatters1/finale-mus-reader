# Text expression definition investigation

**Covers:** Initial epoch samples, synthesized identifiers, and provisional recovery validation.
**Read when:** Refining the open mappings in the [format notes](../format/others/text_expression_defs.md).
**Confidence:** confirmed for tracked byte observations; weak for small private companion samples.

## Initial evidence, 2026-09-05

The user supplied the F2004, F2009, and F2012 semantic boundaries, the `DT` special-other API
tag, and later playback/enclosure/break-rest/hide-measure-number masks. These are contemporary
documentation leads, not independent proofs of their raw positions.
The [public F2000 PDK](../reference/pdk_public_evidence.md) was inspected at its pinned commit
on 2026-09-05. Its `EDTTextExpression` describes the packed font, effects, playback words, flags,
and trailing text; its special-other API classification did not prove physical framing.

| Evidence | Observation |
|---|---|
| `tests/evidence/F263/F263-nodrop-64th.mus` | `DT` expression 1 starts `0018 0000 0058 0000 0000 0002`; text begins `66 00`. Expression 17 starts `120e`; the companion uses font 18, size 14. |
| `tests/evidence/F2000/F2000-lyropts-align-just.mus` | First expression starts `1c00`, and its companion uses font 0, size 28. |
| `mus-564f62c7979b0316` (F2003) | Inline text; source TextBlock maximum 46, first synthesized companion block 47. |
| `mus-b7bd99a606120406` (F2004) | First word is a TextBlock reference, followed by the extended position header. |
| `tests/evidence/F2006/F2006-text-inserts.mus` | Three `DT` definitions reference TextBlocks 19–21; companions preserve those references and assign Misc. |
| `tests/evidence/F2006/F2006-embedded-tiff.mus` | First description starts in incidence 3, after 36 header bytes. |
| `tests/evidence/F2011/F2011-text-inserts.mus` | Class `0x00f1`; category-use flags both set; rehearsal style candidate word 1 is 4 for the lowercase letter-number rehearsal mark. |
| `tests/evidence/F2012/F2012-noteartexp-unlnk-move.mus` | Word 13 is `c001`; baseline and entry offsets are 16 and -72. |
| `mus-84761f30804afb36` (F2012) | Nonempty description uses UTF-16 after the reserved final header word. |

These samples are from one private survey plus tracked evidence. They are not an all-corpus study.

## Identifier and upgrade observations

`mus-b969cf7e22459fd6` (F2000) has existing TextBlocks through 1789; companion expression blocks
start at 693, 1269, 1270, 1735, and 1736. This refutes a universal append-only companion allocator.
`mus-564f62c7979b0316` instead appends sequentially. The reader uses the dependency's allocator
and compares references semantically rather than reproducing unverified gap-reuse behavior.

Pre-F2004 private companions vary positioning between Manual and note-relative settings even
when definitions share the same six-word header shape. The definition alone has not established
the upgrade rule; assignment records may participate.

## Revised hypotheses

- **Superseded:** the older ExpressionText notes described Coda font size as a whole word.
  Expression 17 of the tracked F263 sample distinguishes the packed high-font/low-size layout.
- **Superseded inference:** byte 34 was incorrectly rejected because readable F2006 text
  appeared at byte 36. That text follows a NUL terminator; the
  [description-offset correction](#description-offset-correction) restores byte 34.

## Positioning layout clarification

The user supplied the two historical attachment modes, the eleven-word positioning sequence,
and the distinction between abandoned note-horizontal fields and the repurposed category word.
These are interoperability leads, not a new binary observation or an implemented conversion.
Their durable meaning and the deferred `MeasureExprAssign` work are recorded in
[attachment modes and positioning migration](../format/others/text_expression_defs.md#attachment-modes-and-positioning-migration).
The supplied statement about a nonzero twelfth word reopens the description-start question;
the earlier experiment refutes an unconditional byte-34 interpretation, not that conditional
claim. No positioning decoder changes or assignment synthesis were made for this clarification.

The subsequent user-supplied numeric option lists agree with the existing measure-alignment
and justification helpers and add the older note-specific meanings recorded in the format
notes. Read-only inspection of the owner's authorized private plugin history at its root
revision located compatibility defaults in the expression header. The constructor explains
that old-structure conversion uses Finale's compatibility interface. The relevant history
search of the expression implementation did not locate an
explicit measure/note migration algorithm. No plugin implementation or identifiers were copied.
This is authorized private plugin evidence, not an independent binary proof of conversion.

**Correction after user review:** the note-option lists were mistaken for original F2004–2008
wire values. Revisiting the plugin root revision above shows its expression wrapper requests the
F2009 API layout. Following header renames reaches that same root; the all-history filename
search found no earlier expression-definition path. The old Staff Expression implementation
uses those wrapper values for top/bottom-note positioning, so it does not independently
establish the original numeric fields either. The table is retained as compatibility-interface
evidence in the format notes, with its earlier interpretation explicitly superseded. No
note-specific wire mapping has been implemented from it. Finding the original field values,
not just the combination algorithm, is the immediate unresolved task.

The user subsequently clarified that the plugin intentionally delegates migration to the legacy
API; further searches for a plugin conversion routine are not the prerequisite. Establish the
old note-field meanings first, then use controlled files to measure conversion. The F2009 API
argument alone does not invalidate enums used by a plugin supporting earlier Finale releases.
They remain candidate original-field meanings, not demonstrated replacements or verified wire codes.

The [F2008 Windows manual](https://wpmedia.makemusic.com/kb/pdf_manuals/F2K8_PrintedDoc_Win.pdf),
printed pages 257–258, inspected 2026-09-05, documents the separate Note Positioning tab.
Horizontal positioning separates expression justification, note anchor, and extra offset.
Vertical positioning distinguishes click distance from the top staff line, staff baselines,
top/bottom notes, and entry-related choices. Combined baseline/entry modes expose separate
offsets. The manual names a vertical click option where the plugin candidate names a staff
reference line; that correspondence needs a fixture. **Documentary evidence only:** UI choices
and their meanings do not establish their stored numeric values.

In `tests/evidence/F2006/F2006-embedded-tiff.mus`, definitions 29 and 30 have positioning words
`0 0 18 1 0 0 0 18 7 0 72`. Their raw companions retain the measure anchor and horizontal
adjustment, use baseline adjustment 18, and retain entry adjustment 72. The former measure-Y
word, not the note-baseline word, explains the prior 0-to-18 mismatch. Definition 1 instead
stores measure/manual-horizontal and below-baseline vertical positioning alongside different
note anchors; its companion follows the measure anchors. The three definitions in
`tests/evidence/F2006/F2006-text-inserts.mus` have the compatibility anchor values and manual
measure anchors in the companion. Together with the earlier inline-text fixtures, these support
the narrow measure-path correction and pre-F2004 manual defaults now implemented. Selection or
combination for note-attached uses remains open; entry-detail import remains deferred.
The new assertions have not been built or run, and no capture was performed for this iteration.

The authorized private framework history search also did not locate the migration rule. After
checking the [public class reference](https://pdk.finalelua.com/class_f_c_text_expression_def.html)
on 2026-09-05, the earliest available local revisions of its text-expression declaration and
implementation were inspected at the framework's root revision (2019-11-08). The class already
exposes the merged measure-anchor, baseline-offset, and entry-offset model; its non-Unicode
record access requests the F2010 API layout. Old member spellings persist for some fields,
but they do not expose the abandoned note-position controls. History searches for the old
note fields and alignment/offset/conversion routines found no earlier split interface or
measure/note conversion implementation in these two sources. **Private-framework-derived:**
this limits what the available history can answer; it does not prove where Finale's own
upgrade logic lives. No private framework code or declarations were copied, and no importer
change or build resulted from this search.

## Rehearsal style

The user's category-specific word clarification and rehearsal-style enumeration identify the
word-1 value 4 in `tests/evidence/F2011/F2011-text-inserts.mus` as lowercase letter-number
sequencing, agreeing with the companion. This supersedes the provisional unmapped candidate.
The documentation supplied by the user states that only non-incrementing text existed before F2010;
no controlled boundary pair has yet verified that date. The
[format notes](../format/others/text_expression_defs.md#early-synthesis-and-ids) own the mapping policy.
Source and focused test assertions were updated without building or running tests, as requested.

The subsequently supplied controlled pair `tests/evidence/F2011/F2011-rehearsal-mark.mus`
and `tests/evidence/F2011/F2011-rehearsal-mark-hidemeas.mus` isolates the hide-number flag:
class `0x00f1`, comparator 1, logical word 5 changes from `0400` to `8400`, with the remainder
of the expression payload unchanged. Both records start at block `0200`, decoded offset `23bc`.
Only the second raw companion includes `hideMeasureNum`. Both store word 1 as 6 and the
companion style as `measNum`. This independently binary-verifies another sequencing style
and the hide-number bit in two controlled files from one survey. The user cannot currently
run F2010, so that boundary remains documentary rather than fixture-verified.
These sources and the match-playback variant below still require inventory refresh before the
next tracked capture; their focused assertions have not yet been built or run.

## Match playback

Compared with the rehearsal baseline, `tests/evidence/F2011/F2011-rehearsal-mark-matchplay.mus`
changes word 5 from `0400` to `5401` and word 3 from zero to 1024. The added flag components
are `4000` for matching playback, `1000` for auxiliary-data use, and playback byte `01` for
tempo. Its raw companion explicitly adds `matchPlayback`, `useAuxData`, `playType=time`, and
`auxdata1=1024`. The remainder of the expression payload is unchanged. This is independently
binary-verified against the two controlled sources and their companions, in one survey.
The user approved the provisional F2010 boundary; see the
[format notes](../format/others/text_expression_defs.md#early-synthesis-and-ids) for that policy.

## Independent category flags

The user identified the separate font and positioning bits in the category word. This resolves
the previous ambiguity without changing the lower-bit category reference. The provisional
both-set/both-clear-only implementation is superseded by independent masks; the
[format notes](../format/others/text_expression_defs.md#early-synthesis-and-ids) distinguish
the supplied mapping from controlled evidence still wanted. Builds and tests remain paused.

## SmartMusic normalization

In `tests/evidence/F2006/F2006-embedded-tiff.mus`, definitions 29, 31, and 32 store auxiliary
word 14; definition 30 stores 15. All four store flags `1c0f`. The user-supplied definitions
identify playback byte `0f` as SmartMusic and auxiliary selectors 14/15 as rehearsal/repeat.
These are selector values, not independent bits. The raw Finale 27 companion omits both
`playType` and `auxdata1` while retaining `useAuxData`, enclosure, and break-rest flags.
Tempo definitions 17–22 instead retain auxiliary value 1024 and playback type `time`.
This is independently binary-verified on one file in the tracked-evidence survey.

The user approved import normalization and adjusted provenance. This supersedes the provisional
decision to preserve the auxiliary selector while leaving the unsupported playback type unmapped.
The resulting policy is in the [format notes](../format/others/text_expression_defs.md#coverage-and-remaining-work).
The initial capture below predates this change and is stale for these fields.
Validation after normalization passes all 298 CTest cases, including six expression-focused
cases with 124 assertions. The added fixture test checks original auxiliary/flag words and
offsets, adjusted origins, preserved flags, and an unchanged tempo control. No coverage
capture was rerun for this refinement.

## Initial tracked capture

Historical snapshots below precede the [note-selector experiment](#note-selector-first-iteration).

The first development capture contained 226 source occurrences (224 distinct contents) and 226 successful companions,
with no import failures. Before correcting the description offset, TextExpressionDef had 2,375
unexpected field comparisons across 48 documents. This is a superseded implementation result,
not a recovery claim. The report exposed pre-F2004 Manual/Manual/+7 positioning, later auxiliary
data and position changes, and the unmapped rehearsal style. New difference classifications await
user review. Current validation and remaining scope are recorded in the format notes.

The refreshed tracked capture has the same successful import funnel. TextExpressionDef has 19,329
matching and 2,223 unexpected leaf comparisons; no definition is source-only or companion-only.
The 48 affected source occurrences include the early positioning gaps, the F2011 rehearsal-style
gap, and F2006 `auxData1` values 14/15 becoming zero and baseline adjustment zero becoming 18.
Expression-owned TextBlocks now align by their definition relationship; 24 text-reference
differences remain, including the known Patmm `∞` versus `°` encoding discrepancy. That raw-text
discrepancy already has a text-pool classification, but no additional reference-field classification
has been authorized. Compact examples are capped per source and are not an exhaustive field census.

All 297 CTest cases pass, including five new expression-focused cases with 61 assertions.
The field-count test covers all 23 persisted members and their surveyor origin entries; semantic
reference tests distinguish renumbered equal text from unequal text behind equal IDs. Documentation
links, startup budget, duplication checks, and `git diff --check` pass. No full-corpus capture ran.

## Note-selector first iteration

The user approved a first implementation of the candidate note-selector translations, choosing
vertical code zero as click positioning instead of the earlier proposed reference-line target.
The public F2008 UI names were used for two TU-local enums. No old plugin identifiers were
copied. The experiment's exact scope and field provenance are in the format notes.

The three new F2011 rehearsal fixtures were inventoried as one batch immediately before
capture. `tracked-evidence` now contains 229 occurrences, 227 distinct contents, and 229
adjacent-exact companions. Two instrumented Release captures used the same inventory and
Finale's symbol-font list; every source and companion imported successfully in both.
The first capture includes the pending manual-anchor, measure-Y, hide-number, and match-playback
changes, but precedes the note-selector experiment. This prevents attributing their improvement
to the new selector mapping.

| TextExpressionDef leaf comparisons (occurrences) | Before note selectors | After note selectors |
|---|---:|---:|
| Matching | 20,878 | 20,601 |
| Unexpected | 746 | 1,023 |
| Source-only or companion-only | 0 | 0 |

Distinct-content unexpected counts are 710 before and 987 after. The entire increase is
277 additional leaves across five distinct F2006 files: four previously clean documents gain
67, 67, 73, and 67 differences (`mus-5ab602da3fff05cd`, `mus-4a1b5812b77c79dc`,
`mus-a71f66b69433c38d`, `mus-f3e9167821b468c4`), and `mus-aa5dfb3e175bc5b4` gains three.
No other class's aggregate comparison counts change. This challenges unconditional selection
of the note fields; it does not independently refute their candidate numeric meanings.

The capped unexpected examples show these newly exposed source-to-companion transformations,
all with `LegacyMus` source provenance:

| Field | Imported note-based value | Companion value | Distinct files in examples |
|---|---|---|---:|
| Horizontal anchor | Center of all noteheads | Manual | 4 |
| Horizontal anchor | Right of all noteheads | Manual | 4 |
| Horizontal anchor | Manual | Start of time signature | 4 |
| Vertical anchor | Below baseline or entry | Below baseline | 4 |
| Vertical anchor | Top note | Manual | 1 |

The previously inspected raw F2006 companions support those retained measure-anchor choices.
Examples are capped and do not enumerate every changed leaf. Earlier baseline 0-to-7 gaps and
text-referent differences remain, and no new expected-difference rule was added. The final
snapshot replaces the baseline capture in the standing `private/reports/tracked-evidence.*`
artifact family; this is a controlled-cohort experiment, not private/all-corpus validation.

Both builds succeed and all 300 CTest cases pass. Eight expression-focused cases contain 314
assertions, including all candidate X/Y translations, unknown selectors remaining unmapped,
and F2009+ ignoring the deprecated selectors. The first full run exposed an unsigned expected
flag word in the new hide-number test; its assertion now matches the signed raw-word contract.
No production change was required for that test correction. Documentation checks pass; generated
public CSV line endings were normalized to their tracked LF convention.

## Positioning coverage deferred

The user approved excluding positioning comparisons until assignment recovery is addressed;
the [format notes](../format/others/text_expression_defs.md#coverage-and-remaining-work)
define the exact temporary scope. Imported values and their reader origins are unchanged,
and no expected-difference classification was added.

The refreshed instrumented Release tracked capture contains 229 occurrences (227 distinct
contents), all with successful source and companion imports, using the supplied symbol-font
list. With the six fields excluded, TextExpressionDef has 16,194 matching and 24 unexpected
leaf comparisons, with none source-only or companion-only. The remaining unexpected leaves
are text-reference comparisons; TextBlock has another 24, for 48 overall. This reduced scope
is not complete expression recovery. Surveyor tests check that the six fields and their
origins are absent while the category-position usage flag remains observed.

## Shared text-reference classification

The user approved carrying the common text comparison's classifications through expression
references. This supersedes the separate chunk-equality-only reference check: charset-only
syntax already compared equal, but the reference path lacked the common encoding-glitch
classification. The report examples now show resolved Enigma text and IDs. In the previously
reported case, source `Tempo (∞=120)` and companion `Tempo (°=120)` differ in text as well as
the companion's explicit font-charset argument. Imported text remains unchanged.

The refreshed tracked capture retains the same 229 occurrences, 227 distinct contents, and
successful source/companion funnel. TextExpressionDef has 16,194 matching and 24 classified
encoding-glitch comparisons; the 24 TextBlock reference discrepancies receive that same
classification. No unexpected differences remain in the selected observations. The six
positioning fields remain excluded, and the class's other recovery and evidence gaps remain
listed in the format notes. Focused assertions preserve genuine text differences as `Other`
and verify encoding-glitch classification at both reference paths.
All 300 CTest cases pass after linking the common text surveyor into the focused test target.

## All-corpus capture

The user authorized one instrumented Release capture over the existing `rpatters1-main`,
`rpatters1-installs`, and `tracked-evidence` inventories. It contains 16,326 occurrences of
7,290 distinct source contents: 16,237 occurrences import successfully and 89 are rejected
(58 LIB files and 31 unrecognized documents), matching the previous capture's failure counts.
All 4,828 companion occurrences import successfully, representing 3,241 distinct paired sources.
The supplied Finale symbol-font list was used; no inventory or classification changes were made.

| Unexpected comparisons | Distinct paired source contents | Occurrences |
|---|---:|---:|
| TextExpressionDef | 62,947 | 64,076 |
| TextBlock | 27,803 | 28,162 |
| ExpressionText | 51,087 | 51,724 |
| Total | 141,837 | 143,962 |

All other observed classes have zero unexpected comparisons. Distinct totals count one
available paired comparison per source content; three duplicated sources have differing
comparison payloads, but their unexpected counts are zero in every occurrence. Unpaired
duplicates do not replace a paired observation. Positioning remains excluded.

**Open:** capped examples expose playback, description, enclosure, early rest-breaking,
category, and text-reference differences. They are not an exhaustive field census and may
include definition-identity changes rather than direct field-conversion failures. Three distinct
F2012 sources expose false-to-true `createdByHp` with an unmapped source origin, including
`mus-e4644839e2149421` comparator 14; these are leads for the undocumented flag, not a bit mapping.
No new difference classification was applied. The standing private all-corpus report owns the
full result; the class remains partial with the format notes' remaining fields and epoch gaps.

## Description-offset correction

The user identified the leading zero word before the apparent descriptions. Direct inspection
of `mus-527671f651531470` (F2008), definitions 23, 26, 29, and 30, shows zero bytes at payload
offsets 34–35, followed by `vocal dynamic` at byte 36. The raw companions preserve their
TextBlock references and playback values but omit descriptions. **Superseded:** attributing
this to upgrade loss or positioning migration was premature; the reader skipped the actual
empty-string terminator. The earlier F2006 observation of readable text at byte 36 made the
same mistake. The user confirmed the byte-34/NUL interpretation.

The importer now starts the trailer after 17 words and reports the corresponding byte offset
within a fixed row or variable record. UTF-16 regression inputs start at word 17 and include
a zero-leading trailer with stale data afterward. This correction and the pending partial-hide
conversion are included together in the authorized focused build/test/capture cycle; their
combined result must not be attributed solely to descriptions.

**Contrary result:** both builds and all 302 tests pass, but the focused capture of 1,512
occurrences (1,011 distinct contents) regresses from 143,962 to 229,229 unexpected leaves.
Every source and companion imports successfully. TextExpressionDef rises from 64,076 to
150,563; TextBlock falls from 28,162 to 27,552 and ExpressionText from 51,724 to 51,114.
The quoted F2008 document now has zero unexpected definition leaves, as does the quoted
F2003 document, but the universal description change is contradicted. F2011 accounts for
82,939 additional definition differences, F2012 for 2,936, and F2010 for 1,261; examples
now show empty imported descriptions versus stored companion strings such as
`pianissississimo (velocity = 10)` (`mus-46520e589c4c3a68`, definition 10, F2011).
Some F2006–2008 examples also contradict universal byte-34 termination. No new classification
was applied and no second capture was authorized. Description layout/selection remains open;
the focused snapshot retains that first iteration for review.

The user then asked to revert the description change. Byte 36 and the original description
tests are restored; partial-hidden-text conversion is retained. Read-only checks found both
bytes 34 and 35 zero before descriptions preserved by F2006, F2007, F2008, and F2011 companions.
In particular, `mus-9b5cfc22e1b406f8` definition 26 preserves `vocal dynamic` after the same
leading zero word seen in the omitted-description example. Byte 35 therefore does not resolve
the contradiction. F2012 remains a separate layout question. No build, test, or capture was
run immediately after the revert; that focused report and those binaries were stale.

The subsequently authorized Release rebuild and focused recapture restore the byte-36
baseline while retaining partial-hidden-text conversion. All 1,512 sources and companions
import successfully (1,011 distinct contents). Unexpected leaves total 142,132:
63,466 TextExpressionDef, 27,552 TextBlock, and 51,114 ExpressionText. This is 1,830 fewer
than the pre-hidden-text baseline, a reduction of 610 in each class. The number of fixture
occurrences with `other` text differences falls from 423 to 416. No classifications changed;
positioning remains excluded. No further test run was performed for this report refresh.

## Description conversion loss

The user reproduced loss when upgrading `mus-527671f651531470` directly from F2008 to
F2012, but preservation when saving through F2011 first. Read-only comparison of definitions
23, 26, 29, and 30 with that F2011 intermediate found identical 60-byte record lengths and
identical bytes from offset 34 onward: two zero bytes, description at byte 36, and identical
termination/padding. Only positioning, justification, and category header words changed.
This supersedes the empty/stale-description interpretation above; the exact faulty conversion
step remains open. No public fixture currently reproduces this conversion path.

The existing focused snapshot (1,512 occurrences, 1,011 distinct contents; every source and
companion successful) contains captured nonempty-to-empty description examples in 3 distinct
F2005 documents, 22 F2007, 46 F2008, and one F2011. Counts are lower bounds because examples
are capped. Earlier representatives are `mus-ee1595d7122257a4` (F2005) and
`mus-5d8b3820abbb676b` (F2007). The separate F2011 case is
`mus-94547730f50a1e38`, definition 79. These candidates do not independently establish the
same faulty conversion step. The user approved the
[pre-F2009 classification scope](../format/others/text_expression_defs.md#record-layouts),
leaving F2011 unexpected. Classification and focused boundary tests were added without
building, running tests, or recapturing; existing reports retain the old classifications.

## Ordinal expression correspondence

The user requested coverage-only ordinal pairing, keeping recovered IDs and assignments
unchanged and keeping cross-document text independent of identity selection. The implementation
follows the [coverage correspondence rule](../format/others/text_expression_defs.md#coverage-and-remaining-work).
Synthetic tests exercise definition-ID renumbering, genuine text differences, gapped IDs,
unequal counts with unmatched tails, and exact source-duplicate suppression.

**Contrary to simple gap-closing in the reported example:** raw source
`mus-bf49ac00bf98d299` contains contiguous definition cmpers 1–98; its companion contains
contiguous cmpers 1–101. At definition 39, the source references TextBlock 69 and the companion
references 70. Companion block 69 and raw text 49 still contain the displaced text, but no
companion definition references that block. Source definitions 28 and 39 resolve to identical
`warm` text and have identical definition, TextBlock, and enclosure state after local reference
IDs are excluded. Coverage therefore retains definition 28 for correspondence, skips definition
39 without advancing the companion iterator, and pairs source definition 40 with companion 39.
The appended companion definitions remain unmatched for later assignment investigation; neither
cross-document text matching nor an automatic offset adjustment was introduced.

**Superseded unconditional-skip snapshot:** the authorized recapture of the 1,512-occurrence
unexpected-differences cohort (1,011 distinct
contents) imports every source and companion successfully. Unexpected expression-related leaves
fall from 142,132 to 88,400: TextExpressionDef from 63,466 to 41,072, TextBlock from 27,552 to
16,937, and ExpressionText from 51,114 to 30,391. Occurrences with an `other` text difference
fall from 416 to 301 (292 distinct contents). Both `mus-bf49ac00bf98d299` and its duplicate
`mus-f8d3e3d077a114c3` have zero unexpected leaves in all three classes; each leaves four appended
companion definitions one-sided. Across the cohort, 1,357 occurrences (859 distinct contents)
still have unexpected leaves. Positioning remains excluded, and non-duplicate insertions,
deletions, or assignment-dependent splits may still disturb ordinal correspondence. The aggregate
change combines ordinal pairing, exact source-duplicate suppression, and the separately approved
pre-F2009 description-loss classification; it does not isolate a deduplication-only effect.

A subsequent authorized two-file capture tests conditional duplicate skipping. In
`mus-c2cf68bd12bf7532` (F2007), the current companion text matches the duplicate source expression,
so both iterators advance: unexpected TextExpressionDef, TextBlock, and ExpressionText leaves all
fall to zero, and three companion definitions remain one-sided rather than four. In
`mus-bf49ac00bf98d299` (F2008), the current companion text does not match the duplicate `warm`
expression, so only the source advances; all three classes remain at zero unexpected leaves and
four appended companion definitions remain one-sided. Both sources and companions import
successfully. The 1,512-occurrence report predates this conditional refinement and is now stale.

**Superseded by later correspondence experiments:** the subsequently authorized full recapture
applies conditional duplicate skipping to the same
1,512 occurrences (1,011 distinct contents); every source and companion again imports
successfully. Unexpected expression-related leaves fall from 88,400 to 14,415:
TextExpressionDef from 41,072 to 9,105, TextBlock from 16,937 to 1,944, and ExpressionText from
30,391 to 3,366. Occurrences with an `other` text difference fall from 301 to 82 (73 distinct
contents). Unexpected leaves remain in 1,253 occurrences (755 distinct contents), and all are in
these three classes. Both two-file study cases remain at zero unexpected leaves. This is the last
reliable unexpected-differences baseline before the experiments below.

**Superseded playback-free duplicate identity:** removing playback from duplicate eligibility
while retaining it in the current-pair test makes one studied F2005 source align completely, but
regresses the full 1,512-occurrence cohort. Every source and companion imports. Unexpected
expression-related leaves rise from 14,415 to 47,115: TextExpressionDef 24,609, TextBlock 8,147,
and ExpressionText 14,359. An `other` text difference occurs in 89 occurrences representing 80
distinct contents. Unexpected leaves occur in 1,240 occurrences representing 742 distinct
contents. **Strong for this cohort:** playback-free duplicate eligibility is not a general
correspondence rule.

**Superseded greedy-subsequence experiment:** removing duplicate calculation and advancing only the
source until its complete state matches the current companion reduces unexpected leaves to 12 in
four distinct contents, but this is not recovery progress. Across the same 1,512 occurrences, the
TextExpressionDef comparison has 2,233,404 companion-only leaves and only 1,142,394 same leaves;
TextBlock has 813,542 reader-only and 140,258 companion-only leaves. ExpressionText has 22
reader-only and 1,234 companion-only leaves. Every source and companion imports. **Strong for this
cohort:** strict complete-match subsequence alignment abandons a large population after legitimate
conversion differences and makes the low unexpected count misleading. The experiment does not
establish which smaller set of fields, if any, could safely drive the same scan.

**Superseded one-source-lookahead experiment:** a mismatch skips the current source only when the
immediately following source completely matches the current companion; otherwise the current pair
advances together and retains its differences. Every source and companion in the same 1,512
occurrences imports, but unexpected expression-related leaves rise to 167,081: TextExpressionDef
74,677, TextBlock 31,421, and ExpressionText 60,983. An `other` text difference occurs in 323
occurrences representing 312 distinct contents. Unexpected leaves occur in 1,380 occurrences
representing 882 distinct contents. TextExpressionDef also has 94,374 reader-only and 7,038
companion-only leaves. **Strong for this cohort:** a complete match against only the next source
does not detect enough removals to maintain alignment and performs worse than both the 14,415-leaf
conditional-duplicate baseline and the original 142,132-leaf ordinal result.

Restricting that lookahead predicate to semantic text and playback produces only a modest
improvement: 166,304 unexpected expression-related leaves, with an `other` expression-text
difference in 256 occurrences representing 255 distinct contents. Expanding source lookahead to
16 records and requiring a two-record anchor reduces the total to 69,825 leaves and the `other`
population to 174 occurrences and distinct contents. Every source and companion in both captures
imports successfully. The bounded result remains far above the 14,415-leaf duplicate baseline and
cannot establish that the chosen source skips correspond to Finale's assignment-dependent upgrade.

**Superseded by assignment-based deferral:** the user shelved definition correspondence until
expression assignments can supply attachment context and reliable identity. Recovery coverage now
omits TextExpressionDef, raw expression text, and expression-type TextBlock observations from both
comparison sides after they have been imported and surveyed. The experiments above remain evidence
against retrying an order-only matcher; they are not active comparison policy.

## Partial hidden inline text

The user clarified that the whole-expression hidden flag changes to bracket-delimited spans
in F2002 and remains so through F2003 (correcting an initial pre-F2000 boundary to pre-F2002).
`tests/evidence/F2002/F2002-exp-hidepartial.mus` stores definition 1 as header words
`0c01 0000 0000 0000 0000 c200`, at block `0200`, decoded offset `1530`.
The inline trailer is `test is <hidden>`; its raw companion expression is
`^font(Times,4096)^size(12)^nfx(0)test is ^nfx(128)hidden`.
This independently verifies the F2002 span behavior in one controlled fixture, not the
precise introduction boundary. The user also supplied F2003 companion examples from
`mus-564f62c7979b0316`, definitions 22 and 23, removing brackets around fully hidden text.

The implementation emits hidden-style transitions and restores the base font effects on a
closing bracket. This is source recovery, not an expected-difference rule.
Regression-test code covers the fixture and synthetic F2001–2003 cases, including multiple
spans, flag-clear text, other effects, literal carets/pound signs, and an unmatched opening
bracket. **Superseded:** the first draft required balanced brackets and preserved an unmatched
opening bracket literally. `tests/evidence/F2002/F2002-exp-unbalanced.mus` changes the final
closing bracket of the earlier fixture to a question mark (decoded offset `1553`); the
companion still hides `hidden?` through end of text. The header and flag are unchanged.
This refutes balancing as a prerequisite. The revised conversion treats opening and closing
brackets as independent controls; synthetic assertions also cover repeated opening/closing
controls and a closing control without a preceding opening control. Those additional cases
are user-directed behavior, not yet controlled-fixture findings.
The initial draft remained unbuilt and untested at the user's request. The later authorized
[description correction cycle](#description-offset-correction) includes these changes and
passes all 302 CTest cases. The new fixtures still await batching into the next authorized
tracked inventory/capture cycle; the focused cohort was not expanded.
