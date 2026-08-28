// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options.h"

#include <iterator>

#include "musx/musx.h"

namespace finale_mus_reader {
namespace options {
namespace {

using AccidentalOptionsTarget = musx::dom::options::AccidentalOptions;

constexpr std::uint16_t accidentalShapeSelector = 21;
constexpr std::uint16_t accidentalCrossLayerSelector = 22;
constexpr std::uint16_t accidentalSpacingSelector = 59;
constexpr std::uint16_t accidentalStartMeasureSelector = 41;

// The located fields occupy the same numeric-global word slots in every epoch. Fixed-row
// encodings address them by selector and the zlib encoding coalesces each selector into the
// class id derived by numericGlobalClass.
const FieldMapping fixedRowAccidentalFields[] = {
    MUS_WORD(AccidentalOptionsTarget, "21", GLOBALS_CMPER, 0, 3, minOverlap),
    MUS_WORD(AccidentalOptionsTarget, "21", GLOBALS_CMPER, 0, 5, multiCharSpace),
    MUS_WORD(AccidentalOptionsTarget, "22", GLOBALS_CMPER, 0, 0, crossLayerPositioning),
    MUS_WORD(AccidentalOptionsTarget, "59", GLOBALS_CMPER, 0, 3, acciNoteSpace),
    MUS_WORD(AccidentalOptionsTarget, "59", GLOBALS_CMPER, 0, 4, acciAcciSpace),
    MUS_WORD(AccidentalOptionsTarget, "41", GLOBALS_CMPER, 2, 2, startMeasureSepar),
};

const FieldMapping classRecordAccidentalFields[] = {
    MUS_CLASS_WORD(AccidentalOptionsTarget, numericGlobalClass(accidentalShapeSelector),
        GLOBALS_CMPER, classWordOffset(3), minOverlap),
    MUS_CLASS_WORD(AccidentalOptionsTarget, numericGlobalClass(accidentalShapeSelector),
        GLOBALS_CMPER, classWordOffset(5), multiCharSpace),
    MUS_CLASS_WORD(AccidentalOptionsTarget,
        numericGlobalClass(accidentalCrossLayerSelector), GLOBALS_CMPER,
        classWordOffset(0), crossLayerPositioning),
    MUS_CLASS_WORD(AccidentalOptionsTarget, numericGlobalClass(accidentalSpacingSelector),
        GLOBALS_CMPER, classWordOffset(3), acciNoteSpace),
    MUS_CLASS_WORD(AccidentalOptionsTarget, numericGlobalClass(accidentalSpacingSelector),
        GLOBALS_CMPER, classWordOffset(4), acciAcciSpace),
    MUS_CLASS_WORD(AccidentalOptionsTarget,
        numericGlobalClass(accidentalStartMeasureSelector), GLOBALS_CMPER,
        classWordOffset(14), startMeasureSepar),
};

const MappingTable& fixedRowAccidentalTable()
{
    static const MappingTable table{.reportPrefix = "options.accidentalOptions",
        .epochs = EpochMask::CodaBanner | EpochMask::FixedRow,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<AccidentalOptionsTarget>,
        .fields = fixedRowAccidentalFields, .fieldCount = std::size(fixedRowAccidentalFields)};
    return table;
}

const MappingTable& classRecordAccidentalTable()
{
    static const MappingTable table{.reportPrefix = "options.accidentalOptions",
        .epochs = EpochMask::Zlib, .encoding = RecordEncoding::ClassRecord,
        .targetKind = TargetKind::OptionsSingleton,
        .enumerateTargets = &enumerateOptionsTarget<AccidentalOptionsTarget>,
        .fields = classRecordAccidentalFields,
        .fieldCount = std::size(classRecordAccidentalFields)};
    return table;
}

void applyEarlyAccidentalSpacingBehavior(const ImportContext& context)
{
    // Believed: before 3.7 these settings are not parameterized; when both slots remain zero,
    // Finale uses the hard-coded legacy spacing of 8. A nonzero word remains source data.
    if (!sourcePredatesVersion(context.profile,
            FormatEpoch::UncompressedLegacy, versions::finale3_7)) return;

    const auto family = readNumericGlobalWords(context.index, accidentalSpacingSelector);
    if (!family.present || family.words.size() <= 4
        || family.words[3] != 0 || family.words[4] != 0) {
        return;
    }

    const auto pooled = context.document->getOptions()->get<AccidentalOptionsTarget>();
    if (!pooled) return;
    const auto target = std::const_pointer_cast<AccidentalOptionsTarget>(pooled);
    target->acciNoteSpace = 8;
    target->acciAcciSpace = 8;

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    for (const auto* member : {"acciNoteSpace", "acciAcciSpace"}) {
        if (auto* info = context.report.findField(
                instanceKey<AccidentalOptionsTarget>(), member)) {
            info->origin = ValueOrigin::LegacyBehavior;
            info->rawValue = 0;
        }
    }
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

} // namespace

void importAccidentalOptions(const ImportContext& context)
{
    applyMappingTables({&fixedRowAccidentalTable(), &classRecordAccidentalTable()},
        context.index, context.profile, context.document, context.report);
    applyEarlyAccidentalSpacingBehavior(context);
}

} // namespace options
} // namespace finale_mus_reader
