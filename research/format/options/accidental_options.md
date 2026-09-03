# AccidentalOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Accidental options

**Implemented in full.** The accidental preferences are fields of the
omnibus distance-preferences class rather than one legacy record. The Framework mapping supplied for
interoperability identified the numeric globals below; it was initially `private-framework-derived`. The
zlib epoch reaches the same word slots through the usual `numericGlobalClass` relationship. The public
[`FCDistancePrefs`](https://pdk.finalelua.com/class_f_c_distance_prefs.html) documentation (accessed
2026-08-28) exposes `GetAccidentalCrossLayerPositioning` and
`SetAccidentalCrossLayerPositioning`, with Lua property support dated to Framework 0.59, but gives no Finale
product-version boundary or physical record location.

| musxdom field | Selector | Incidence | Word | Width | Availability |
|---|---:|---:|---:|---:|---|
| `minOverlap` | `21` | 0 | 3 | 2 | located epochs |
| `multiCharSpace` | `21` | 0 | 5 | 2 | located epochs |
| `crossLayerPositioning` | `22` | 0 | 0 | 2 | Finale 2004 onward |
| `acciNoteSpace` | `59` | 0 | 3 | 2 | located epochs |
| `acciAcciSpace` | `59` | 0 | 4 | 2 | located epochs |
| `startMeasureSepar` | `41` | 2 | 2 | 2 | when the incidence is present |

The Framework-derived cross-layer row is `current_only`: it does not occur in the original
pre-Finale-2014-oriented mapping line and therefore cannot date itself backward. The controlled
`F2004-no-xlayer-accis` save supplies the missing boundary evidence. Switching off “Use Cross-Layer
Accidental Positioning” moves selector 22 word 0 from 1 to 0 while leaving its other five words
unchanged; the ETF repeats that row and the Finale 27 companion leaves `<useNewAcciPositioning>` absent.
Finale 2004 and later read the word as a boolean. Earlier sources report false as `LegacyBehavior` and
do not read the word: the controlled `F372-beams-resthalfstems` save proves that the same location is
BeamOptions' `dispHalfStemsOnRests` in Finale 3.7.2.

`startMeasureSepar` (`<frontAcciSepar>`, "Adjustment at Start of Measure") is independently located by the
controlled F2005 save: changing 24 to 27 moves only selector `41`, incidence 2, word 2; its ETF and Finale 27
companion carry 27 as well. The mapping is structural rather than version-gated: a source that contains that
incidence and slot recovers it, while an absent or short selector family leaves the pinned baseline untouched.
This resolves the 41 former source/companion differences, and all 792 field comparisons now agree.

The broader three-survey pass exposes 11 distinct Finale 3.0 documents in which selector `59` words 3 and 4
are both zero while their Finale 27 companions store 8. The reader supplies the effective value 8 as `LegacyBehavior`
only when selector `59` exists, both words are zero, and the source predates 3.5.
A nonzero source value is always recovered normally. The pre-fixture three-survey capture had all 27,240
`AccidentalOptions` comparisons agreeing, but its indiscriminate selector-22 mapping was falsified by the
controlled F3.7.2 half-stem save. A later tracked-only capture isolated that collision as its sole
AccidentalOptions difference. The current tracked-evidence capture contains 194 source occurrences
and 194 exact, successfully parsed companions across all four readable epochs; all 1,164
AccidentalOptions leaves agree, with no expected or unexpected differences and no missing or extra
leaves. Both the enabled F2004 baseline and the controlled disabled save agree on all six fields.
That tracked cohort has no Finale 3.5 source and therefore does not exercise the corrected gate.

The printed Finale 3.5 addendum identifies accidental spacing as a new adjustable setting in that
release. This user-supplied manual evidence establishes the version boundary independently of the
record values. **Weak working hypothesis:** the earlier zeroes are not stored preference values;
Finale 3.0 through 3.2 allocate the two slots without parameterizing the settings and use a
hard-coded spacing of 8. The behavior rule models that pre-parameter behavior, corroborated by
every Finale 27 companion, without claiming that zero encodes 8. A future direct specimen can
revise this interpretation without changing the located later mapping.

Word 5 belongs to another class. Before Engraver Slurs introduce independent controls, it is the shared slur
thickness that supplies both `SmartShapeOptions` vertical thickness-control values. The controlled Finale 2000
edit locating that field is described in the Smart Shape section below. Its zero in the Finale 3.0 rows is
consistent with the same early layout, but is not part of the accidental-spacing behavior gate.
