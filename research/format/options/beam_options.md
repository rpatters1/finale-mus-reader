# BeamOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Beam options

**Partially implemented.** All fourteen fields have mappings across the four readable epochs,
but some early canned boolean behaviors remain open. The public PDK documents
the semantic fields across
[`FCDistancePrefs`](https://pdk.finalelua.com/class_f_c_distance_prefs.html),
[`FCSizePrefs`](https://pdk.finalelua.com/class_f_c_size_prefs.html), and
[`FCMiscDocPrefs`](https://pdk.finalelua.com/class_f_c_misc_doc_prefs.html) (accessed
2026-08-28). Those declarations are **public-PDK-derived** and do not expose the legacy
record locations. Authorized read-only Framework history supplied the initial locations;
the mappings below remain **private-framework-derived** except where the binary evidence
described here independently confirms them.

| musxdom field | Selector | Incidence | Word | Width |
|---|---:|---:|---:|---:|
| `beamStubLength` | `03` | 0 | 3 | 2 |
| `maxSlope` | `20` | 0 | 0 | 2 |
| `beamSepar` | `20` | 0 | 1 | 2 |
| `maxFromMiddle` | `20` | 0 | 2 | 2 |
| `beamingStyle` | `41` | 0 | 0 | 2 |
| eight boolean fields | `41` | 0 | 1 | packed bits |
| `beamWidth` | `62` | 0 | 4 | 4, high word first |

The zlib epoch reaches the same locations through `numericGlobalClass(selector)` and byte
offsets. The stored beaming-style order is end notes, always flat, standard note, extreme
note; the importer translates it to musxdom's end, standard, extreme, always-flat enum order.
The packed flag word is:

| Bit | BeamOptions field |
|---:|---|
| 0 | `extendBeamsOverRests` |
| 1 | `incRestsInFourGroups` |
| 3 | `beamFourEighthsInCommonTime` |
| 4 | `beamThreeEighthsInCommonTime` |
| 5 | `oldFinaleRestBeams` |
| 6 | `dispHalfStemsOnRests` |
| 7 | `spanSpace` |
| 8 | `extendSecBeamsOverRests` |

Bit 2 is StemOptions' `noReverseStems`, so the two importers share one structural test for
the word's layout. The word is zero through Finale 97 and packed from Finale 2000 in the
tracked evidence. Any bit above bit 0 selects the packed layout; DCL and zlib select it by
epoch. A packed uncompressed document whose word is zero or exactly bit 0 remains physically
ambiguous and is treated as the early layout, the same bounded ambiguity documented for
StemOptions. `tests/evidence/F2012/F2012-bookmarks.mus` independently confirms bit 5: its
little-endian class record carries `0x00b2`, including bit 5, and its raw Finale 27 companion
explicitly carries `<oldFinaleRestBeams/>`.

Before the packed layout, the options use a mixture of separate switches and canned behavior.
Selector 41 word 0 still stores `beamingStyle`, and selector 16 word 4 controls both
`extendBeamsOverRests` and `extendSecBeamsOverRests` in the Coda-banner and uncompressed
lone-flag layouts. The controlled `F263-beams-inclrests` and `F372-beams-inclrests` edits each
move that word from zero to one while selector 41 word 1 remains zero; both companions add
`<incEdgeRestsInBeamGroups/>` and `<extendSecBeamsOverRests/>`. The controlled
`F97-beams-inclrests` edit repeats the same isolated selector 16 word 4 change and the same
two-property conversion, without changing `incRestsInFourGroups`. The controlled
`F263-flatbams` edit independently moves selector 41 word 0 from zero to one and its companion
stores `<beamingStyle>alwaysFlat</beamingStyle>`. The reader reports these values as `LegacyMus`.
Selector 22 word 0 separately controls `dispHalfStemsOnRests`, established by
`F372-beams-resthalfstems`; the reader imports it as `LegacyMus` only in the uncompressed
lone-flag layout. Finale 2.6.3 exposes no half-stem control, so Coda selector 22 is not claimed.
Selector 22 is reused for
AccidentalOptions' `crossLayerPositioning` beginning in Finale 2004, after the half-stem switch
has moved into selector 41's packed flag word.

Finale 3.7 through Finale 98 store `beamFourEighthsInCommonTime` separately at selector 09
word 5. The controlled `F98-beams-no4-8thcommon` edit moves that word from one to zero. Finale 98
also writes the “Include Rests in Groups of Four” UI state to selector 23 word 5: the controlled
`F98-beams-inclrestsin4` edit moves it from zero to one. Each MUS differs from the common baseline
at exactly that one byte, each ETF prints the same isolated row change, and selector 41 remains
zero in all three. But Finale 98 clears the setting when the edited document is closed and
reopened, and every later version opens it as false. Selector 23 therefore records a written UI
state rather than persistent document behavior. The earlier F3.7 and Finale 97 ETFs carry zero at
selector 09 word 5, agreeing with their disabled companions. Finale 2000 moves the four-eighths
field into selector 41 bit 3 and supplies the persistent four-groups field at bit 1.

The reader still asserts `oldFinaleRestBeams` and `spanSpace` true for all lone-flag documents.
Finale 2.6.3 exposes only the two controls above and stores no four-eighths switch; its legacy
behavior is false, while Finale 27 enables the modern option during upgrade. Because the setting
affects only newly created beams and cannot alter existing ones, the reader deliberately accepts
the later true default rather than reproducing the historical Coda behavior. It therefore leaves
`beamFourEighthsInCommonTime` at the pinned baseline with origin `Finale27Default` only for Coda
sources. Finale 3.7 through 98 recover the separate selector 09 word, and Finale 2000 and later
recover selector 41 bit 3. Finale 98 ignores selector 23 and reports `incRestsInFourGroups` false
as `LegacyBehavior`, matching how the source reopens; earlier sources retain the pinned baseline,
and Finale 2000 and later recover selector 41 bit 1. Other values without a located switch likewise
remain at the pinned baseline. Finale 2000 is the first represented packed layout.

The same 32-versus-128 stem-connection count that dates StemOptions' unit transition also
dates two BeamOptions fields. In the 32-element layout, `maxSlope` and `maxFromMiddle` are
staff positions and are multiplied by musxdom's `EVPU_PER_STAFF_POSITION`; later layouts
store Evpu directly. `beamSepar` does not change unit. For Coda `beamWidth`, when selector 62
is present, the observed conversion is 2,500 stored units per Evpu. The tracked Coda evidence
contains nine raw `30000` values corresponding to `12 Evpu` or `768 Efix`, and one raw `28750`
corresponding to `11.5 Evpu` or `736 Efix`; no contrary value occurs. Interpreting that ratio as
10,000 stored units per point is a **strong** theory: it is arithmetically equivalent because
musxdom defines four Evpu per point, and it accounts for every observed value. Neither the public
PDK nor musxdom documents the source unit that way, however, so the interpretation remains
unproven rather than a confirmed format fact. Coda `beamStubLength` is absent or inoperative --
the slot is zero in 38 documents and four in one while every companion carries 18 -- so it
retains the pinned baseline's matching 18 rather than reading the later location.

An initial tracked-evidence capture compared all fourteen fields in 140 successful source/companion
pairs: 39 Coda-banner, 29 uncompressed, 48 DCL, and 24 zlib. All 1,960 leaves agree, with no
expected or unexpected differences, no missing or extra leaves, and no failed documents. No
expected-difference rule was needed for that cohort; it predates the three controlled F3.7.2
beam fixtures.

The pre-selector-16 implementation's all-corpus capture selected 16,237 occurrences across `rpatters1-main`,
`rpatters1-installs`, and `tracked-evidence`; 16,148 parsed successfully and 4,548 had successful
companions. Across those companion occurrences, 63,525 of 63,672 BeamOptions leaves agree and
147 remain unexpected. The 147 candidates account exactly for four transformations:

- `beamFourEighthsInCommonTime`: false from `LegacyBehavior`, true in the companion, in 57
  distinct uncompressed documents (Finale 3.0: 14, 3.2: 3, 3.5: 3, Finale 98: 37);
- `extendBeamsOverRests`: false from `Finale27Default`, true in the companion, in 25 distinct
  documents (20 Finale 3.7 uncompressed and five PC 1.0+ Coda-banner);
- `extendSecBeamsOverRests`: the same false-to-true transformation in the same 25 documents;
- `incRestsInFourGroups`: true from `Finale27Default`, false in the companion, in 40 distinct
  uncompressed documents (39 labeled Finale 98 and one with no recovered saving product).

Raw companion inspection confirms the corresponding elements are present or absent in
representatives `mus-254b77cdb2ab24f4`, `mus-20e1b564fb5bce1a`,
`mus-c67acda870867d92`, `mus-611884d2d6a3577a`, and `mus-5ddec0a12409afbd`; these are not
musxdom default-construction artifacts. Their selector 41 flag words are zero, but the new
controlled evidence shows that selector is not the only source location. A direct companion
scan finds selector 16's two target fields false in 63 and true in 20 distinct Finale 3.7
documents; the 20 true documents exactly match the two 20-document Finale 3.7 difference
populations above. The new mapping therefore targets 40 of the original 147 unexpected leaves.

A tracked-only capture after adding selector 16 and the three controlled fixtures selected 143
successful source/companion pairs. Of 2,002 BeamOptions leaves, 2,001 agreed; the sole difference
was the new half-stem fixture because selector 22 had not yet been promoted to BeamOptions. The
same source produced the sole AccidentalOptions difference among 858 leaves, exposing the old
reader's incorrect assumption that selector 22 word 0 meant `crossLayerPositioning` in every
fixed-row layout. The reader now recovers the half-stem switch through Finale 3.7 and gates the
accidental switch at its Finale 2004 introduction.

The pre-compatibility-choice F2.6.3 tracked-evidence capture selected 146 distinct source documents with 146 exact,
successfully parsed companions: 41 Coda-banner, 32 uncompressed, 49 DCL, and 24 zlib. Of 2,044
BeamOptions leaves, 2,003 agree and 41 are unexpected. Every difference has the one path and
transformation `beamFourEighthsInCommonTime: false -> true`, with source origin `LegacyBehavior`:
29 Finale 1.0 and 12 Finale 2.6 documents. Raw companion inspection confirms that representative
Finale 1.0 and 2.6 conversions, including both new controlled fixtures, explicitly store
`<beamFourEighthsInCommonTime/>`; the companion value is Finale upgrade synthesis rather than a
musxdom default artifact. The other thirteen BeamOptions leaves agree everywhere, including both
new F2.6.3 controls. There are no expected BeamOptions differences and no missing or extra leaves.
The reader now accepts that synthesized true value as the harmless later default, so these 41
differences describe the superseded implementation.

The pre-F98-fixture tracked-evidence capture covers the same 146 distinct sources and companions, all
successfully parsed. All 2,044 BeamOptions leaves agree, with no expected or unexpected
differences and no missing or extra leaves: 574 Coda-banner, 448 uncompressed, 686 DCL, and 336
zlib comparisons. Recovering the independently available four-eighths bit resolves the prior 24
Finale 3.7 and Finale 97 differences without changing the accepted Coda upgrade behavior. Both
controlled Finale 2.6.3 beam edits and all three controlled Finale 3.7 beam edits agree on all
fourteen fields. The controlled Finale 2004 cross-layer edit also agrees on all six
AccidentalOptions fields. `incRestsInFourGroups` remains **open**.

The corresponding pre-F98-fixture three-inventory capture selects 16,243 occurrences. Of those, 16,154
sources parse successfully and 4,554 have successful companions. Across the companions, 63,679
of 63,756 BeamOptions leaves agree and 77 are unexpected; none are expected, missing, or extra.
All 77 differences occur in 40 distinct uncompressed Finale 98 documents. Thirty-eight have
source version `4.0.0.10`; two have `4.0.3.2`, one of which has no recovered marketing-product
label. The transformations are:

- `beamFourEighthsInCommonTime`: false from `LegacyMus`, true in the companion, in 37 documents
  (36 version `4.0.0.10`, one version `4.0.3.2`);
- `incRestsInFourGroups`: true from `Finale27Default`, false in the companion, in all 40
  documents.

The 37 four-eighths documents are a subset of the four-groups population; the other three have
only the four-groups difference. Direct companion inspection confirms both the present
four-eighths element and the absent four-groups element in a representative with both findings,
and confirms the absent four-eighths element in representatives with only the four-groups
finding. The former therefore records Finale's upgrade of a recovered source value rather than
default leakage from the reader. Coda-banner, DCL, and zlib BeamOptions comparisons all agree.
The new controlled Finale 98 evidence explains both formerly differing fields. Its baseline and
four-eighths edit show that the 37 broad-corpus four-eighths differences came from reading the
later packed location too early. Its four-groups edit locates the written UI state at selector 23
word 5, but Finale 98 does not restore that state after reopening the document and later versions
likewise show the option false. A tracked capture over 149 sources made before that reload behavior
was applied had 2,085 agreeing BeamOptions leaves and one unexpected difference: the edited source
was recovered as true from `LegacyMus`, while its companion was false. The reader now models the
reload behavior instead. The post-correction `tracked-evidence` capture covers 150 distinct sources
and 150 successfully parsed companions: all 2,100 BeamOptions leaves agree, including every leaf in
the ten Finale 97 fixtures and three Finale 98 fixtures. No expected-difference rule is involved.

The corresponding post-correction capture across `rpatters1-main`, `rpatters1-installs`, and
`tracked-evidence` selects 16,247 occurrences: 16,158 sources parse successfully, 4,558 have
successfully parsed companions, and 89 fail before comparison. All 63,812 companion-backed
BeamOptions leaves agree, with no expected differences, unexpected differences, or one-sided
leaves. By survey, that is 16,870 agreeing leaves from 1,205 `rpatters1-main` companions, 44,842
from 3,203 `rpatters1-installs` companions, and 2,100 from the 150 tracked companions. The
companion-backed Finale 98 population contains 42 distinct documents across internal versions
`4.0.0.10` and `4.0.3.2`; all 588 of their BeamOptions leaves agree. No Finale 99 source in the
selected surveys has a companion, so the exact-version behavior remains untested there.
