# musxdom class coverage

Derived from ../musxdom/src/musx/factory/PoolFactory.cpp. Alphabetic within each pool,
as the registry is. Regenerate when musxdom adds a registered type.

    [x] done    [~] partial    [ ] not started

`[x]` requires both halves: every field the class's known records can supply is recovered, and the
class shows no unexpected companion difference across every registered survey. A field whose known
records provably do not carry it reports `Unmapped` and does not by itself hold a class at `[~]`.

**This file carries no commentary.** It is a checklist: one line per class, the file that
recovers it, and the briefest possible status in the right-hand column. Anything that needs a
sentence belongs in that class's own file under [`research/format/`](../format/); anything that
needs a paragraph belongs in its [`research/investigations/`](../investigations/index.md) file.
Explanation added here has to be removed again.

**62 done, 2 partial, 126 to do, 190 total.** Verified class for class against the registry on
2026-09-04: every registered type appears here and nothing here is unregistered.

## options  (28 done, 0 partial, 0 to do, 28 total)

    [x] AccidentalOptions                       accidental_options.cpp
    [x] AlternateNotationOptions                alternate_notation_options.cpp
    [x] AugmentationDotOptions                  augmentation_dot_options.cpp
    [x] BarlineOptions                          barline_options.cpp
    [x] BeamOptions                             beam_options.cpp
    [x] ChordOptions                            chord_options.cpp
    [x] ClefOptions                             clef_options.cpp
    [x] FlagOptions                             flag_options.cpp
    [x] FontOptions                             font_options.cpp
    [x] GraceNoteOptions                        grace_note_options.cpp
    [x] KeySignatureOptions                     key_signature_options.cpp
    [x] LineCurveOptions                        line_curve_options.cpp
    [x] LyricOptions                            lyric_options.cpp
    [x] MiscOptions                             misc_options.cpp
    [x] MultimeasureRestOptions                 multimeasure_rest_options.cpp
    [x] MusicSpacingOptions                     music_spacing_options.cpp
    [x] MusicSymbolOptions                      music_symbol_options.cpp
    [x] NoteRestOptions                         note_rest_options.cpp
    [x] PageFormatOptions                       page_format_options.cpp
    [x] PianoBraceBracketOptions                piano_brace_bracket_options.cpp
    [x] RepeatOptions                           repeat_options.cpp
    [x] SmartShapeOptions                       smart_shape_options.cpp
    [x] StaffOptions                            staff_options.cpp
    [x] StemOptions                             stem_options.cpp
    [x] TextOptions                             text_options.cpp
    [x] TieOptions                              tie_options.cpp
    [x] TimeSignatureOptions                    time_signature_options.cpp
    [x] TupletOptions                           tuplet_options.cpp

## others  (26 done, 0 partial, 67 to do, 93 total)

    [ ] AcciAmountFlats                                                         L1 library item
    [ ] AcciAmountSharps                                                        L1 library item
    [ ] AcciOrderFlats                                                          L1 library item
    [ ] AcciOrderSharps                                                         L1 library item
    [ ] ArticulationDef                                                         L1 library item
    [ ] BeatChartElement
    [x] ChordSuffixElement                      chord_suffix_elements.cpp
    [x] ChordSuffixPlayback                     chord_suffix_playback.cpp
    [ ] ClefList
    [ ] DrumStaff
    [ ] DrumStaffStyle                                                          L2 satellite
    [ ] FileAlias
    [ ] FileDescription
    [ ] FilePath
    [ ] FileUrlBookmark
    [x] FontDefinition                          font_definitions.cpp
    [ ] Frame
    [x] FretInstrument                          fret_instruments.cpp
    [x] FretboardGroup                          fretboard_groups.cpp
    [x] FretboardStyle                          fretboard_styles.cpp
    [ ] KeyAttributes                                                           L1 library item
    [ ] KeyFormat                                                               L1 library item
    [ ] KeyMapArray                                                             L1 library item
    [x] LayerAttributes                         layer_attributes.cpp
    [x] MarkingCategory                         marking_category.cpp
    [x] MarkingCategoryName                     marking_category.cpp
    [ ] Measure
    [ ] MeasureExprAssign
    [ ] MeasureNumberRegion
    [ ] MultiStaffGroupId
    [ ] MultiStaffInstrumentGroup
    [ ] MultimeasureRest
    [ ] NamePositionAbbreviated
    [ ] NamePositionFull
    [ ] NamePositionStyleAbbreviated                                            L2 satellite
    [ ] NamePositionStyleFull                                                   L2 satellite
    [ ] OssiaBounds
    [ ] OssiaHeader
    [ ] OssiaMusic
    [ ] Page
    [x] PageGraphicAssign                       graphic_assignments.cpp
    [ ] PageOssiaAssign
    [ ] PageTextAssign
    [x] PartDefinition                          part_definitions.cpp
    [x] PartGlobals                             part_globals.cpp
    [ ] PartVoicing
    [ ] PercussionNoteInfo                                                      L1 library item
    [ ] PlaybackRoute
    [ ] PlaybackRouteName
    [ ] RepeatBack
    [ ] RepeatBackIndividualPositioning
    [ ] RepeatEndingStart
    [ ] RepeatEndingStartIndividualPositioning
    [ ] RepeatEndingText
    [ ] RepeatEndingTextIndividualPositioning
    [ ] RepeatPassList
    [x] ShapeData                               shape_definitions.cpp
    [x] ShapeDef                                shape_definitions.cpp
    [ ] ShapeExpressionDef                                                      L1 library item
    [x] ShapeGraphicAssign                      graphic_assignments.cpp
    [x] ShapeInstructionList                    shape_definitions.cpp
    [ ] SmartShape
    [x] SmartShapeCustomLine                    smart_shape_custom_lines.cpp
    [ ] SmartShapeMeasureAssign
    [ ] SplitMeasure
    [ ] Staff
    [x] StaffListCategoryName                   staff_list.cpp
    [x] StaffListCategoryParts                  staff_list.cpp
    [x] StaffListCategoryScore                  staff_list.cpp
    [x] StaffListRepeatName                     staff_list.cpp
    [x] StaffListRepeatParts                    staff_list.cpp
    [x] StaffListRepeatPartsForced              staff_list.cpp
    [x] StaffListRepeatScore                    staff_list.cpp
    [x] StaffListRepeatScoreForced              staff_list.cpp
    [ ] StaffPlayData
    [ ] StaffStyle                                                              L1 library item
    [ ] StaffStyleAssign
    [ ] StaffSystem
    [ ] StaffUsed
    [ ] SystemLock
    [ ] TempoChange
    [x] TextBlock                               text_blocks.cpp
    [ ] TextExpressionDef                                                       L1 library item
    [ ] TextExpressionEnclosure                                                 L2 satellite
    [ ] TextRepeatAssign
    [ ] TextRepeatDef                                                           L1 library item
    [ ] TextRepeatEnclosure                                                     L2 satellite
    [ ] TextRepeatIndividualPositioning
    [ ] TextRepeatText                                                          L2 satellite
    [ ] TimeCompositeLower
    [ ] TimeCompositeUpper
    [ ] TonalCenterFlats                                                        L1 library item
    [ ] TonalCenterSharps                                                       L1 library item

## details  (2 done, 0 partial, 58 to do, 60 total)

    [ ] AccidentalAlterations
    [ ] ArticulationAssign
    [ ] BaselineChords
    [ ] BaselineExpressionsAbove
    [ ] BaselineExpressionsBelow
    [ ] BaselineFretboards
    [ ] BaselineLyricsChorus
    [ ] BaselineLyricsSection
    [ ] BaselineLyricsVerse
    [ ] BaselineSystemChords
    [ ] BaselineSystemExpressionsAbove
    [ ] BaselineSystemExpressionsBelow
    [ ] BaselineSystemFretboards
    [ ] BaselineSystemLyricsChorus
    [ ] BaselineSystemLyricsSection
    [ ] BaselineSystemLyricsVerse
    [ ] BeamAlterationsDownStem
    [ ] BeamAlterationsUpStem
    [ ] BeamExtensionDownStem
    [ ] BeamExtensionUpStem
    [ ] BeamStubDirection
    [ ] Bracket
    [ ] CenterShape
    [ ] ChordAssign
    [ ] ClefOctaveFlats                                                         L1 library item
    [ ] ClefOctaveSharps                                                        L1 library item
    [ ] CrossStaff
    [ ] CustomDownStem
    [ ] CustomUpStem
    [ ] DotAlterations
    [ ] EntryPartFieldDetail
    [ ] EntrySize
    [x] FretboardDiagram                        fretboard_diagrams.cpp
    [ ] GFrameHold
    [ ] IndependentStaffDetails
    [ ] KeySymbolListElement                                                    L1 library item
    [ ] LyricAssignChorus
    [ ] LyricAssignSection
    [ ] LyricAssignVerse
    [ ] LyricEntryInfo
    [x] MeasureGraphicAssign                    measure_graphic_assign.cpp
    [ ] MeasureNumberIndividualPositioning
    [ ] MeasureOssiaAssign
    [ ] MeasureTextAssign
    [ ] NoteAlterations
    [ ] PercussionNoteCode
    [ ] SecondaryBeamAlterationsDownStem
    [ ] SecondaryBeamAlterationsUpStem
    [ ] SecondaryBeamBreak
    [ ] ShapeNote
    [ ] ShapeNoteStyle                                                          L2 satellite
    [ ] SmartShapeEntryAssign
    [ ] StaffGroup
    [ ] StaffSize
    [ ] StemAlterations
    [ ] StemAlterationsUnderBeam
    [ ] TablatureNoteMods
    [ ] TieAlterEnd
    [ ] TieAlterStart
    [ ] TupletDef

## entries  (0 done, 0 partial, 1 to do, 1 total)

    [ ] Entry

## texts  (6 done, 2 partial, 0 to do, 8 total)

    [x] BlockText                               text_pool.cpp, coda_texts.cpp
    [~] BookmarkText                            text_pool.cpp                   pooled eras only
    [~] ExpressionText                          text_pool.cpp                   pooled eras only
    [x] FileInfoText                            text_pool.cpp,
                                                file_info_text.cpp
    [x] LyricsChorus                            text_pool.cpp, coda_texts.cpp
    [x] LyricsSection                           text_pool.cpp, coda_texts.cpp
    [x] LyricsVerse                             text_pool.cpp, coda_texts.cpp
    [x] SmartShapeText                          text_pool.cpp
