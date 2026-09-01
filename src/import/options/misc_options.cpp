// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options.h"

#include <iterator>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace options {
namespace {

using MiscOptionsTarget = musx::dom::options::MiscOptions;

constexpr const char *miscOptionsReportPrefix = "options.miscOptions";
constexpr std::uint16_t showRepeatsForPartsSelector = 16;
constexpr std::uint16_t pickupSelector = 17;

const FieldMapping fixedMiscFields[] = {
    MUS_NUMERIC_WORD(MiscOptionsTarget, pickupSelector, 0, 5, pickupValue),
};

const FieldMapping fixedLayerMiscFields[] = {
    MUS_WORD(MiscOptionsTarget, figureTag, 10, 0, 4, showActiveLayerOnly),
};

const FieldMapping fixedRepeatPartMiscFields[] = {
    MUS_NUMERIC_WORD(MiscOptionsTarget, showRepeatsForPartsSelector, 0, 5,
                     showRepeatsForParts),
};

const FieldMapping classMiscFields[] = {
    MUS_CLASS_WORD(MiscOptionsTarget,
                   numericGlobalClass(showRepeatsForPartsSelector),
                   GLOBALS_CMPER, classWordOffset(5), showRepeatsForParts),
    MUS_CLASS_WORD(MiscOptionsTarget, numericGlobalClass(pickupSelector),
                   GLOBALS_CMPER, classWordOffset(5), pickupValue),
    MUS_CLASS_WORD(MiscOptionsTarget, zlibFigureClass, 10, classWordOffset(4),
                   showActiveLayerOnly),
};

const MappingTable &fixedMiscTable() {
  static const MappingTable table{
      .reportPrefix = miscOptionsReportPrefix,
      .epochs =
          EpochMask::CodaBanner | EpochMask::Uncompressed | EpochMask::Dcl,
      .targetKind = TargetKind::OptionsSingleton,
      .enumerateTargets = &enumerateOptionsTarget<MiscOptionsTarget>,
      .fields = fixedMiscFields,
      .fieldCount = std::size(fixedMiscFields)};
  return table;
}

const MappingTable &fixedLayerMiscTable() {
  // The Coda epoch predates layers and has no Show Active Layer Only setting;
  // its seeded Finale 27 default is retained deliberately.
  static const MappingTable table{
      .reportPrefix = miscOptionsReportPrefix,
      .epochs = EpochMask::Uncompressed | EpochMask::Dcl,
      .targetKind = TargetKind::OptionsSingleton,
      .enumerateTargets = &enumerateOptionsTarget<MiscOptionsTarget>,
      .fields = fixedLayerMiscFields,
      .fieldCount = std::size(fixedLayerMiscFields)};
  return table;
}

const MappingTable &fixedRepeatPartMiscTable() {
  static const MappingTable table{
      .reportPrefix = miscOptionsReportPrefix,
      .epochs = EpochMask::Uncompressed | EpochMask::Dcl,
      .targetKind = TargetKind::OptionsSingleton,
      .enumerateTargets = &enumerateOptionsTarget<MiscOptionsTarget>,
      .fields = fixedRepeatPartMiscFields,
      .fieldCount = std::size(fixedRepeatPartMiscFields)};
  return table;
}

const MappingTable &classMiscTable() {
  static const MappingTable table{
      .reportPrefix = miscOptionsReportPrefix,
      .epochs = EpochMask::Zlib,
      .encoding = RecordEncoding::ClassRecord,
      .targetKind = TargetKind::OptionsSingleton,
      .enumerateTargets = &enumerateOptionsTarget<MiscOptionsTarget>,
      .fields = classMiscFields,
      .fieldCount = std::size(classMiscFields)};
  return table;
}

void applyLegacyMiscBehavior(const ImportContext &context) {
  const auto pooled = context.document->getOptions()->get<MiscOptionsTarget>();
  if (!pooled) {
    return;
  }
  const auto target = std::const_pointer_cast<MiscOptionsTarget>(pooled);

  // These settings postdate the supported legacy formats, whose fixed
  // behavior differs from the pinned Finale 27 defaults.
  target->consolidateRestsAcrossLayers = false;
  target->alignMeasureNumbersWithBarlines = false;
  FINALE_MUS_READER_REPORT_FIELD(
      context.report, instanceKey<MiscOptionsTarget>(),
      "consolidateRestsAcrossLayers", {ValueOrigin::LegacyBehavior, 0, 0, 0});
  FINALE_MUS_READER_REPORT_FIELD(context.report,
                                 instanceKey<MiscOptionsTarget>(),
                                 "alignMeasureNumbersWithBarlines",
                                 {ValueOrigin::LegacyBehavior, 0, 0, 0});
}

void reportRemainingMiscFields(
    const ImportContext &context,
    const std::shared_ptr<const MiscOptionsTarget> &target) {
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
  const auto instance = instanceKey<MiscOptionsTarget>();
  FINALE_MUS_READER_REPORT_FIELD(
      context.report, instance, "shapeDesignerDashLength",
      {ValueOrigin::Finale27Default, 0, 0, target->shapeDesignerDashLength});
  FINALE_MUS_READER_REPORT_FIELD(
      context.report, instance, "shapeDesignerDashSpace",
      {ValueOrigin::Finale27Default, 0, 0, target->shapeDesignerDashSpace});
  FINALE_MUS_READER_REPORT_FIELD(
      context.report, instance, "restWidthAdjust",
      {ValueOrigin::Finale27Default, 0, 0, target->restWidthAdjust});
  FINALE_MUS_READER_REPORT_FIELD(
      context.report, instance, "dblWholeVertAdjust",
      {ValueOrigin::Finale27Default, 0, 0, target->dblWholeVertAdjust});
  FINALE_MUS_READER_REPORT_FIELD(
      context.report, instance, "keepWrittenOctaveInConcertPitch",
      {ValueOrigin::Finale27Default, 0, 0,
       target->keepWrittenOctaveInConcertPitch});
  if (context.profile.epoch == FormatEpoch::CodaBanner) {
    FINALE_MUS_READER_REPORT_FIELD(
        context.report, instance, "showActiveLayerOnly",
        {ValueOrigin::Finale27Default, 0, 0, target->showActiveLayerOnly});
  }
#else
  static_cast<void>(context);
  static_cast<void>(target);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

} // namespace

void importMiscOptions(const ImportContext &context) {
  applyMappingTables({&fixedMiscTable(), &fixedLayerMiscTable(),
                      &fixedRepeatPartMiscTable(), &classMiscTable()},
                     context.index, context.profile, context.document,
                     context.report);
  applyLegacyMiscBehavior(context);
  if (const auto target =
          context.document->getOptions()->get<MiscOptionsTarget>()) {
    reportRemainingMiscFields(context, target);
  }
}

} // namespace options
} // namespace finale_mus_reader
