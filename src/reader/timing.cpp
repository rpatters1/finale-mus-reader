// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "reader/timing.h"

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
#include <stdexcept>
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

namespace finale_mus_reader {
namespace timing {

std::string_view phaseName(Phase phase)
{
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    switch (phase) {
    case Phase::FileIo: return "file_io";
    case Phase::ContainerParse: return "container_parse";
    case Phase::SourceReport: return "source_report";
    case Phase::EmbeddedGraphics: return "embedded_graphics";
    case Phase::ConstructionBegin: return "construction_begin";
    case Phase::DefaultsParse: return "defaults_parse";
    case Phase::DefaultsSeed: return "defaults_seed";
    case Phase::HeaderRecovery: return "header_recovery";
    case Phase::MacSymbolFonts: return "mac_symbol_fonts";
    case Phase::RecordIndex: return "record_index";
    case Phase::ImportFontDefinitions: return "font_definitions";
    case Phase::ImportFontOptions: return "font_options";
    case Phase::ImportAccidentalOptions: return "accidental_options";
    case Phase::ImportAlternateNotationOptions: return "alternate_notation_options";
    case Phase::ImportAugmentationDotOptions: return "augmentation_dot_options";
    case Phase::ImportBarlineOptions: return "barline_options";
    case Phase::ImportBeamOptions: return "beam_options";
    case Phase::ImportChordOptions: return "chord_options";
    case Phase::ImportClefOptions: return "clef_options";
    case Phase::ImportFlagOptions: return "flag_options";
    case Phase::ImportGraceNoteOptions: return "grace_note_options";
    case Phase::ImportKeySignatureOptions: return "key_signature_options";
    case Phase::ImportLineCurveOptions: return "line_curve_options";
    case Phase::ImportLyricOptions: return "lyric_options";
    case Phase::ImportMiscOptions: return "misc_options";
    case Phase::ImportMultimeasureRestOptions: return "mmrest_options";
    case Phase::ImportMusicSpacingOptions: return "spacing_options";
    case Phase::ImportMusicSymbolOptions: return "music_symbol_options";
    case Phase::ImportNoteRestOptions: return "note_rest_options";
    case Phase::ImportPageFormatOptions: return "page_format_options";
    case Phase::ImportPianoBraceBracketOptions: return "piano_brace_bracket_options";
    case Phase::ImportRepeatOptions: return "repeat_options";
    case Phase::ImportSmartShapeOptions: return "smart_shape_options";
    case Phase::ImportStaffOptions: return "staff_options";
    case Phase::ImportStemOptions: return "stem_options";
    case Phase::ImportTextOptions: return "text_options";
    case Phase::ImportTieOptions: return "tie_options";
    case Phase::ImportTimeSignatureOptions: return "time_signature_options";
    case Phase::ImportFretInstruments: return "fret_instruments";
    case Phase::ImportFretboardGroups: return "fretboard_groups";
    case Phase::ImportFretboardStyles: return "fretboard_styles";
    case Phase::ImportLayerAttributes: return "layer_atts";
    case Phase::ImportPageGraphicAssignments: return "page_graphic_assigns";
    case Phase::ImportShapeGraphicAssignments: return "shape_graphic_assigns";
    case Phase::ImportShapeDefinitions: return "shape_definitions";
    case Phase::ImportSmartShapeCustomLines: return "ss_line_styles";
    case Phase::ImportFretboardDiagrams: return "fretboard_diagrams";
    case Phase::ImportMeasureGraphicAssignments: return "meas_graphic_assigns";
    case Phase::ImportTexts: return "texts";
    case Phase::ImportTextBlocks: return "text_blocks";
    case Phase::TextLaterPool: return "text_later_pool";
    case Phase::TextPoolFraming: return "text_pool_framing";
    case Phase::TextConversion: return "text_conversion";
    case Phase::TextLiteralEncoding: return "text_literal_encoding";
    case Phase::TextFontState: return "text_font_state";
    case Phase::TextObjectConstruction: return "text_object_construction";
    case Phase::TextReportConstruction: return "text_report_construction";
    case Phase::TextDomInsertion: return "text_dom_insertion";
    case Phase::TextHeaderFileInfo: return "text_header_file_info";
    case Phase::TextCodaStored: return "text_coda_stored";
    case Phase::DeferredReferences: return "deferred_references";
    case Phase::ConstructionFinish: return "construction_finish";
    case Phase::DiagnosticDelivery: return "diagnostic_delivery";
    case Phase::Count: break;
    }
    throw std::logic_error("invalid reader timing phase");
#else
    static_cast<void>(phase);
    return {};
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

std::string_view counterName(Counter counter)
{
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    switch (counter) {
    case Counter::TextRecords: return "text_records";
    case Counter::TextRecordBytes: return "text_record_bytes";
    case Counter::TextCommands: return "text_commands";
    case Counter::TextLiteralRuns: return "text_literal_runs";
    case Counter::TextLiteralBytes: return "text_literal_bytes";
    case Counter::TextCacheHits: return "text_cache_hits";
    case Counter::TextCacheMisses: return "text_cache_misses";
    case Counter::TextCacheAvoidedBytes: return "text_cache_avoided_bytes";
    case Counter::TextFontResolutionCacheHits: return "text_font_resolution_cache_hits";
    case Counter::TextFontResolutionCacheMisses: return "text_font_resolution_cache_misses";
    case Counter::TextInitialFontCacheHits: return "text_initial_font_cache_hits";
    case Counter::TextInitialFontCacheMisses: return "text_initial_font_cache_misses";
    case Counter::Count: break;
    }
    throw std::logic_error("invalid reader timing counter");
#else
    static_cast<void>(counter);
    return {};
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

std::string_view containerCandidateName(ContainerCandidate candidate)
{
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    switch (candidate) {
    case ContainerCandidate::Uncompressed: return "uncompressed";
    case ContainerCandidate::Dcl: return "dcl";
    case ContainerCandidate::Zlib: return "zlib";
    }
    throw std::logic_error("invalid container timing candidate");
#else
    static_cast<void>(candidate);
    return {};
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

std::string_view containerAttemptResultName(ContainerAttemptResult result)
{
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    switch (result) {
    case ContainerAttemptResult::Accepted: return "accepted";
    case ContainerAttemptResult::InputBounds: return "input_bounds";
    case ContainerAttemptResult::FirstType: return "first_type";
    case ContainerAttemptResult::Framing: return "framing";
    case ContainerAttemptResult::Inflate: return "inflate";
    case ContainerAttemptResult::Checksum: return "checksum";
    case ContainerAttemptResult::Incomplete: return "incomplete";
    }
    throw std::logic_error("invalid container timing result");
#else
    static_cast<void>(result);
    return {};
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

namespace {

thread_local Session* activeSession{};

} // namespace

Session::Session()
    : previous_(activeSession)
{
    activeSession = this;
}

Session::~Session()
{
    activeSession = previous_;
}

void Session::record(Phase phase, double durationMs)
{
    durations_[static_cast<std::size_t>(phase)] += durationMs;
}

void Session::record(ContainerAttemptMeasurement measurement)
{
    containerAttempts_.push_back(std::move(measurement));
}

void Session::increment(Counter counter, std::size_t amount)
{
    counters_[static_cast<std::size_t>(counter)] += amount;
}

std::vector<Measurement> Session::measurements() const
{
    std::vector<Measurement> result;
    for (std::size_t index = 0; index < durations_.size(); ++index) {
        if (durations_[index] != 0.0) {
            result.push_back({static_cast<Phase>(index), durations_[index]});
        }
    }
    return result;
}

std::vector<ContainerAttemptMeasurement> Session::containerAttempts() const
{
    return containerAttempts_;
}

std::vector<CounterMeasurement> Session::counters() const
{
    std::vector<CounterMeasurement> result;
    for (std::size_t index = 0; index < counters_.size(); ++index) {
        if (counters_[index] != 0) {
            result.push_back({static_cast<Counter>(index), counters_[index]});
        }
    }
    return result;
}

Scope::Scope(Phase phase)
    : session_(activeSession), phase_(phase),
      started_(session_ ? std::chrono::steady_clock::now()
                       : std::chrono::steady_clock::time_point{})
{
}

Scope::~Scope()
{
    if (!session_) return;
    const std::chrono::duration<double, std::milli> elapsed =
        std::chrono::steady_clock::now() - started_;
    session_->record(phase_, elapsed.count());
}

ContainerAttempt::ContainerAttempt(ContainerCandidate candidate, bool bigEndian)
    : session_(activeSession), measurement_{.candidate = candidate, .bigEndian = bigEndian},
      started_(session_ ? std::chrono::steady_clock::now()
                       : std::chrono::steady_clock::time_point{})
{
}

ContainerAttempt::~ContainerAttempt()
{
    if (!session_ || finished_) return;
    finish(ContainerAttemptResult::Incomplete);
}

void ContainerAttempt::beginDecompression(std::size_t compressedBytes)
{
    ++measurement_.decompressionCalls;
    measurement_.compressedInputBytes += compressedBytes;
}

void ContainerAttempt::decompressionSucceeded(std::size_t decompressedBytes)
{
    ++measurement_.decompressedBlocks;
    measurement_.decompressedBytes += decompressedBytes;
}

void ContainerAttempt::finish(ContainerAttemptResult result)
{
    if (!session_ || finished_) return;
    measurement_.result = result;
    const std::chrono::duration<double, std::milli> elapsed =
        std::chrono::steady_clock::now() - started_;
    measurement_.durationMs = elapsed.count();
    session_->record(std::move(measurement_));
    finished_ = true;
}

void increment(Counter counter, std::size_t amount)
{
    if (activeSession) activeSession->increment(counter, amount);
}

#else

Session::Session() = default;
Session::~Session() = default;

std::vector<Measurement> Session::measurements() const
{
    return {};
}

std::vector<ContainerAttemptMeasurement> Session::containerAttempts() const
{
    return {};
}

std::vector<CounterMeasurement> Session::counters() const
{
    return {};
}

void increment(Counter, std::size_t)
{
}

#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

} // namespace timing
} // namespace finale_mus_reader
