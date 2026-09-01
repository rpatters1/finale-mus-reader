// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace options {
namespace {

using NoteRestOptionsTarget = musx::dom::options::NoteRestOptions;

constexpr const char *noteRestOptionsReportPrefix = "options.noteRestOptions";
constexpr std::uint16_t noteColorSelector = 99;
constexpr std::size_t noteColorCount = music_theory::STANDARD_12EDO_STEPS;
constexpr std::size_t noteColorFirstWord = 1;
constexpr std::size_t noteColorTrailerWords = 5;

struct NoteColorChannel {
  const char *name;
  std::uint16_t NoteRestOptionsTarget::NoteColor::*member;
};
constexpr NoteColorChannel noteColorChannels[] = {
    {"red", &NoteRestOptionsTarget::NoteColor::red},
    {"green", &NoteRestOptionsTarget::NoteColor::green},
    {"blue", &NoteRestOptionsTarget::NoteColor::blue},
};
constexpr std::size_t noteColorPayloadWords =
    noteColorFirstWord + noteColorCount * std::size(noteColorChannels) +
    noteColorTrailerWords;

bool storesRestPositionAdjustments(const records::LegacyRecordIndex &index,
                                   const SourceProfile &profile) {
  constexpr std::uint16_t restPositionSelector = 44;
  return readGlobalWords(index, profile, restPositionSelector).present;
}

const FieldMapping codaNoteRestOptionFields[] = {
    MUS_WORD(NoteRestOptionsTarget, "12", GLOBALS_CMPER, 0, 4,
             doCrossStaffNotes),
};

const FieldMapping uncompressedNoteRestOptionFields[] = {
    MUS_BIT(NoteRestOptionsTarget, "CS", 1, 0, 5, 7, doShapeNotes),
    MUS_WORD(NoteRestOptionsTarget, "12", GLOBALS_CMPER, 0, 4,
             doCrossStaffNotes),
    MUS_WORD(NoteRestOptionsTarget, "41", GLOBALS_CMPER, 0, 5,
             scaleManualPositioning),
};

const FieldMapping dclNoteRestOptionFields[] = {
    MUS_WORD(NoteRestOptionsTarget, "01", GLOBALS_CMPER, 0, 1, doShapeNotes),
    MUS_WORD(NoteRestOptionsTarget, "12", GLOBALS_CMPER, 0, 4,
             doCrossStaffNotes),
    MUS_WORD(NoteRestOptionsTarget, "41", GLOBALS_CMPER, 0, 5,
             scaleManualPositioning),
};

const FieldMapping fixedRowRestPositionFields[] = {
    MUS_WORD(NoteRestOptionsTarget, "44", GLOBALS_CMPER, 0, 0, drop8thRest),
    MUS_WORD(NoteRestOptionsTarget, "44", GLOBALS_CMPER, 0, 1, drop16thRest),
    MUS_WORD(NoteRestOptionsTarget, "44", GLOBALS_CMPER, 0, 2, drop32ndRest),
    MUS_WORD(NoteRestOptionsTarget, "41", GLOBALS_CMPER, 0, 3, drop64thRest),
    MUS_WORD(NoteRestOptionsTarget, "41", GLOBALS_CMPER, 0, 4, drop128thRest),
};

const FieldMapping classNoteRestOptionFields[] = {
    MUS_CLASS_WORD(NoteRestOptionsTarget, numericGlobalClass(1), GLOBALS_CMPER,
                   classWordOffset(1), doShapeNotes),
    MUS_CLASS_WORD(NoteRestOptionsTarget, numericGlobalClass(12), GLOBALS_CMPER,
                   classWordOffset(4), doCrossStaffNotes),
    MUS_CLASS_WORD(NoteRestOptionsTarget, numericGlobalClass(41), GLOBALS_CMPER,
                   classWordOffset(5), scaleManualPositioning),
};

const FieldMapping classRestPositionFields[] = {
    MUS_CLASS_WORD(NoteRestOptionsTarget, numericGlobalClass(44), GLOBALS_CMPER,
                   classWordOffset(0), drop8thRest),
    MUS_CLASS_WORD(NoteRestOptionsTarget, numericGlobalClass(44), GLOBALS_CMPER,
                   classWordOffset(1), drop16thRest),
    MUS_CLASS_WORD(NoteRestOptionsTarget, numericGlobalClass(44), GLOBALS_CMPER,
                   classWordOffset(2), drop32ndRest),
    MUS_CLASS_WORD(NoteRestOptionsTarget, numericGlobalClass(41), GLOBALS_CMPER,
                   classWordOffset(3), drop64thRest),
    MUS_CLASS_WORD(NoteRestOptionsTarget, numericGlobalClass(41), GLOBALS_CMPER,
                   classWordOffset(4), drop128thRest),
};

const MappingTable &codaNoteRestOptionsTable() {
  static const MappingTable table{
      .reportPrefix = noteRestOptionsReportPrefix,
      .epochs = EpochMask::CodaBanner,
      .targetKind = TargetKind::OptionsSingleton,
      .enumerateTargets = &enumerateOptionsTarget<NoteRestOptionsTarget>,
      .fields = codaNoteRestOptionFields,
      .fieldCount = std::size(codaNoteRestOptionFields)};
  return table;
}

const MappingTable &uncompressedNoteRestOptionsTable() {
  static const MappingTable table{
      .reportPrefix = noteRestOptionsReportPrefix,
      .epochs = EpochMask::Uncompressed,
      .targetKind = TargetKind::OptionsSingleton,
      .enumerateTargets = &enumerateOptionsTarget<NoteRestOptionsTarget>,
      .fields = uncompressedNoteRestOptionFields,
      .fieldCount = std::size(uncompressedNoteRestOptionFields)};
  return table;
}

const MappingTable &dclNoteRestOptionsTable() {
  static const MappingTable table{
      .reportPrefix = noteRestOptionsReportPrefix,
      .epochs = EpochMask::Dcl,
      .targetKind = TargetKind::OptionsSingleton,
      .enumerateTargets = &enumerateOptionsTarget<NoteRestOptionsTarget>,
      .fields = dclNoteRestOptionFields,
      .fieldCount = std::size(dclNoteRestOptionFields)};
  return table;
}

const MappingTable &classNoteRestOptionsTable() {
  static const MappingTable table{
      .reportPrefix = noteRestOptionsReportPrefix,
      .epochs = EpochMask::Zlib,
      .encoding = RecordEncoding::ClassRecord,
      .targetKind = TargetKind::OptionsSingleton,
      .enumerateTargets = &enumerateOptionsTarget<NoteRestOptionsTarget>,
      .fields = classNoteRestOptionFields,
      .fieldCount = std::size(classNoteRestOptionFields)};
  return table;
}

const MappingTable &fixedRowRestPositionTable() {
  static const MappingTable table{
      .reportPrefix = noteRestOptionsReportPrefix,
      .epochs = EpochMask::CodaBanner | EpochMask::FixedRow,
      .applies = &storesRestPositionAdjustments,
      .targetKind = TargetKind::OptionsSingleton,
      .enumerateTargets = &enumerateOptionsTarget<NoteRestOptionsTarget>,
      .fields = fixedRowRestPositionFields,
      .fieldCount = std::size(fixedRowRestPositionFields)};
  return table;
}

const MappingTable &classRestPositionTable() {
  static const MappingTable table{
      .reportPrefix = noteRestOptionsReportPrefix,
      .epochs = EpochMask::Zlib,
      .encoding = RecordEncoding::ClassRecord,
      .targetKind = TargetKind::OptionsSingleton,
      .enumerateTargets = &enumerateOptionsTarget<NoteRestOptionsTarget>,
      .fields = classRestPositionFields,
      .fieldCount = std::size(classRestPositionFields)};
  return table;
}

bool captureNoteColors(const ImportContext &context,
                       NoteRestOptionsTarget &target) {
  if (!sourceMatches(context.profile, EpochMask::Zlib) ||
      target.noteColors.size() != noteColorCount) {
    return false;
  }
  const auto source =
      readGlobalWords(context.index, context.profile, noteColorSelector);
  if (!source.present) {
    return false;
  }
  if (source.words.size() != noteColorPayloadWords) {
    context.report.diagnostics.push_back(
        {musx::util::Logger::LogLevel::Warning,
         "The notehead-color record has " +
             std::to_string(source.words.size()) + " words; its layout requires " +
             std::to_string(noteColorPayloadWords) +
             ", so its values remain at the Finale 27 defaults."});
    return false;
  }
  for (const auto &color : target.noteColors) {
    if (!color) {
      return false;
    }
  }

  // The color record stores the outline switch first, then twelve red values,
  // twelve green values, and twelve blue values. Five trailing words are not
  // interpreted.
  target.drawOutline = source.words.front() != 0;
  FINALE_MUS_READER_REPORT_FIELD(
      context.report, instanceKey<NoteRestOptionsTarget>(), "drawOutline",
      {ValueOrigin::LegacyMus, source.blockOffset, source.decodedOffset,
       source.words.front(), numericGlobalClass(noteColorSelector)});

  for (std::size_t channelIndex = 0;
       channelIndex < std::size(noteColorChannels);
       ++channelIndex) {
    const auto &channel = noteColorChannels[channelIndex];
    for (std::size_t colorIndex = 0; colorIndex < noteColorCount;
         ++colorIndex) {
      const auto wordIndex = noteColorFirstWord +
                             channelIndex * noteColorCount + colorIndex;
      const auto value = static_cast<std::uint16_t>(source.words[wordIndex]);
      const auto &color = target.noteColors[colorIndex];
      color.get()->*channel.member = value;
      const auto member = std::string("noteColors[") +
                          std::to_string(colorIndex) + "]." + channel.name;
      FINALE_MUS_READER_REPORT_FIELD(
          context.report, instanceKey<NoteRestOptionsTarget>(), member,
          {ValueOrigin::LegacyMus, source.blockOffset,
           source.decodedOffset + wordIndex * 2, value,
           numericGlobalClass(noteColorSelector)});
    }
  }
  return true;
}

void reportDefaultedNoteRestFields(const ImportContext &context,
                                   const NoteRestOptionsTarget &target,
                                   bool recoveredNoteColors) {
  if (!recoveredNoteColors) {
    FINALE_MUS_READER_REPORT_FIELD(
        context.report, instanceKey<NoteRestOptionsTarget>(), "drawOutline",
        {ValueOrigin::Finale27Default, 0, 0, target.drawOutline});
  }
  if (sourceMatches(context.profile, EpochMask::CodaBanner)) {
    for (const auto [member, value] : {
             std::pair{"doShapeNotes",
                       static_cast<musx::dom::Evpu>(target.doShapeNotes)},
         }) {
      FINALE_MUS_READER_REPORT_FIELD(
          context.report, instanceKey<NoteRestOptionsTarget>(),
          std::string(member),
          {ValueOrigin::Finale27Default, 0, 0, value});
    }
  }
  if (recoveredNoteColors) {
    return;
  }
  for (std::size_t index = 0; index < target.noteColors.size(); ++index) {
    const auto &color = target.noteColors[index];
    if (!color)
      continue;
    const auto prefix =
        std::string("noteColors[") + std::to_string(index) + "].";
    FINALE_MUS_READER_REPORT_FIELD(
        context.report, instanceKey<NoteRestOptionsTarget>(), prefix + "red",
        {ValueOrigin::Finale27Default, 0, 0, color->red});
    FINALE_MUS_READER_REPORT_FIELD(
        context.report, instanceKey<NoteRestOptionsTarget>(), prefix + "green",
        {ValueOrigin::Finale27Default, 0, 0, color->green});
    FINALE_MUS_READER_REPORT_FIELD(
        context.report, instanceKey<NoteRestOptionsTarget>(), prefix + "blue",
        {ValueOrigin::Finale27Default, 0, 0, color->blue});
  }
}

} // namespace

void importNoteRestOptions(const ImportContext &context) {
  applyMappingTables(
      {&codaNoteRestOptionsTable(), &uncompressedNoteRestOptionsTable(),
       &dclNoteRestOptionsTable(), &classNoteRestOptionsTable(),
       &fixedRowRestPositionTable(), &classRestPositionTable()},
      context.index, context.profile, context.document, context.report);

  const auto target =
      context.document->getOptions()->get<NoteRestOptionsTarget>();
  if (!target)
    return;
  const auto mutableTarget =
      std::const_pointer_cast<NoteRestOptionsTarget>(target);
  const auto recoveredNoteColors = captureNoteColors(context, *mutableTarget);
  if (sourceMatches(context.profile, EpochMask::CodaBanner)) {
    // Finale 1.x-2.x always scale manual note positioning. The stored
    // preference begins with Finale 3.0, so the whole Coda-banner epoch uses
    // the earlier behavior.
    mutableTarget->scaleManualPositioning = true;
    FINALE_MUS_READER_REPORT_FIELD(
        context.report, instanceKey<NoteRestOptionsTarget>(),
        "scaleManualPositioning", {ValueOrigin::LegacyBehavior, 0, 0, 1});
  }
  reportDefaultedNoteRestFields(context, *target, recoveredNoteColors);
}

} // namespace options
} // namespace finale_mus_reader
