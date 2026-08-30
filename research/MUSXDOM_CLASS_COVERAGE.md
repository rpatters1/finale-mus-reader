# musxdom class coverage

Derived from ../musxdom/src/musx/factory/PoolFactory.cpp. Alphabetic within each pool,
as the registry is. Regenerate when musxdom adds a registered type.

    [x] done    [~] partial    [ ] not started

**This file carries no commentary.** It is a checklist: one line per class, the file that
recovers it, and the briefest possible status in the right-hand column. Anything that needs a
sentence belongs in [FORMAT_NOTES.md](FORMAT_NOTES.md); anything that needs a paragraph belongs
in [EXPERIMENT_LOG.md](EXPERIMENT_LOG.md). Explanation added here has to be removed again.

**38 done, 3 partial, 149 to do, 190 total.** Verified class for class against the registry on
2026-08-24: every registered type appears here and nothing here is unregistered.

## options  (19 done, 0 partial, 9 to do, 28 total)

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
    [ ] LineCurveOptions
    [x] LyricOptions                            lyric_options.cpp
    [ ] MiscOptions
    [x] MultimeasureRestOptions                 multimeasure_rest_options.cpp
    [x] MusicSpacingOptions                     music_spacing_options.cpp
    [ ] MusicSymbolOptions
    [ ] NoteRestOptions
    [ ] PageFormatOptions
    [x] PianoBraceBracketOptions                piano_brace_bracket_options.cpp
    [x] RepeatOptions                           repeat_options.cpp
    [x] SmartShapeOptions                       smart_shape_options.cpp
    [ ] StaffOptions
    [x] StemOptions                             stem_options.cpp
    [x] TextOptions                             text_options.cpp
    [ ] TieOptions
    [ ] TimeSignatureOptions
    [ ] TupletOptions

## others  (11 done, 1 partial, 81 to do, 93 total)

    [ ] AcciAmountFlats
    [ ] AcciAmountSharps
    [ ] AcciOrderFlats
    [ ] AcciOrderSharps
    [ ] ArticulationDef
    [ ] BeatChartElement
    [ ] ChordSuffixElement
    [ ] ChordSuffixPlayback
    [ ] ClefList
    [ ] DrumStaff
    [ ] DrumStaffStyle
    [ ] FileAlias
    [ ] FileDescription
    [ ] FilePath
    [ ] FileUrlBookmark
    [x] FontDefinition                          font_definitions.cpp
    [ ] Frame
    [x] FretInstrument                          fret_instruments.cpp
    [x] FretboardGroup                          fretboard_groups.cpp
    [x] FretboardStyle                          fretboard_styles.cpp
    [ ] KeyAttributes
    [ ] KeyFormat
    [ ] KeyMapArray
    [~] LayerAttributes                         layer_attributes.cpp            stub; completeness audit excluded
    [ ] MarkingCategory
    [ ] MarkingCategoryName
    [ ] Measure
    [ ] MeasureExprAssign
    [ ] MeasureNumberRegion
    [ ] MultiStaffGroupId
    [ ] MultiStaffInstrumentGroup
    [ ] MultimeasureRest
    [ ] NamePositionAbbreviated
    [ ] NamePositionFull
    [ ] NamePositionStyleAbbreviated
    [ ] NamePositionStyleFull
    [ ] OssiaBounds
    [ ] OssiaHeader
    [ ] OssiaMusic
    [ ] Page
    [x] PageGraphicAssign                       graphic_assignments.cpp
    [ ] PageOssiaAssign
    [ ] PageTextAssign
    [ ] PartDefinition
    [ ] PartGlobals
    [ ] PartVoicing
    [ ] PercussionNoteInfo
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
    [ ] ShapeExpressionDef
    [x] ShapeGraphicAssign                      graphic_assignments.cpp
    [x] ShapeInstructionList                    shape_definitions.cpp
    [ ] SmartShape
    [x] SmartShapeCustomLine                    smart_shape_custom_lines.cpp
    [ ] SmartShapeMeasureAssign
    [ ] SplitMeasure
    [ ] Staff
    [ ] StaffListCategoryName
    [ ] StaffListCategoryParts
    [ ] StaffListCategoryScore
    [ ] StaffListRepeatName
    [ ] StaffListRepeatParts
    [ ] StaffListRepeatPartsForced
    [ ] StaffListRepeatScore
    [ ] StaffListRepeatScoreForced
    [ ] StaffPlayData
    [ ] StaffStyle
    [ ] StaffStyleAssign
    [ ] StaffSystem
    [ ] StaffUsed
    [ ] SystemLock
    [ ] TempoChange
    [x] TextBlock                              text_blocks.cpp
    [ ] TextExpressionDef
    [ ] TextExpressionEnclosure
    [ ] TextRepeatAssign
    [ ] TextRepeatDef
    [ ] TextRepeatEnclosure
    [ ] TextRepeatIndividualPositioning
    [ ] TextRepeatText
    [ ] TimeCompositeLower
    [ ] TimeCompositeUpper
    [ ] TonalCenterFlats
    [ ] TonalCenterSharps

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
    [ ] ClefOctaveFlats
    [ ] ClefOctaveSharps
    [ ] CrossStaff
    [ ] CustomDownStem
    [ ] CustomUpStem
    [ ] DotAlterations
    [ ] EntryPartFieldDetail
    [ ] EntrySize
    [x] FretboardDiagram                        fretboard_diagrams.cpp
    [ ] GFrameHold
    [ ] IndependentStaffDetails
    [ ] KeySymbolListElement
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
    [ ] ShapeNoteStyle
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
