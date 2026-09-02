// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/options.h"

#include <array>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>

#include "musx/musx.h"

namespace finale_mus_reader
{
namespace options
{
namespace
{

using StaffOptionsTarget = musx::dom::options::StaffOptions;
using StaffNamePositioning = musx::dom::others::NamePositioning;
using StaffNamePositionMember = std::shared_ptr<StaffNamePositioning> StaffOptionsTarget::*;

constexpr std::uint16_t staffScalarSelector = 97;
constexpr std::size_t staffScalarWordCount = 15;
constexpr musx::dom::Evpu preFinale2008StaffSeparation = -320;
constexpr musx::dom::Evpu codaStaffNameHorzOff = -192;
constexpr musx::dom::Evpu codaStaffNameVertOff = -27;

bool sourceStoresStaffScalars(const records::LegacyRecordIndex& index,
                              const SourceProfile& profile)
{
    return profile.epoch == FormatEpoch::ZlibLegacy &&
           readGlobalWords(index, profile, staffScalarSelector).words.size() >=
           staffScalarWordCount;
}

const FieldMapping staffScalarFields[]{
    MUS_CLASS_WORD(StaffOptionsTarget, numericGlobalClass(staffScalarSelector), GLOBALS_CMPER,
                   classWordOffset(12), staffSeparation),
    MUS_CLASS_WORD(StaffOptionsTarget, numericGlobalClass(staffScalarSelector), GLOBALS_CMPER,
                   classWordOffset(13), staffSeparIncr),
    MUS_CLASS_WORD(StaffOptionsTarget, numericGlobalClass(staffScalarSelector), GLOBALS_CMPER,
                   classWordOffset(14), autoAdjustStaffSepar),
};

const MappingTable& staffScalarTable()
{
    // The record payload states whether this zlib-era layout carries the scalar tail.
    static const MappingTable table{.reportPrefix = "options.staffOptions",
                                    .epochs = EpochMask::Zlib,
                                    .applies = &sourceStoresStaffScalars,
                                    .encoding = RecordEncoding::ClassRecord,
                                    .targetKind = TargetKind::OptionsSingleton,
                                    .enumerateTargets =
                                        &enumerateOptionsTarget<StaffOptionsTarget>,
                                    .fields = staffScalarFields,
                                    .fieldCount = std::size(staffScalarFields)};
    return table;
}

struct StaffNamePositionSource
{
    std::uint16_t selector;
    StaffNamePositionMember target;
    std::string_view member;
    std::size_t flagSlot;
    std::uint8_t justificationBitCount;
    std::uint8_t alignmentFirstBit;
};

constexpr std::array staffNamePositionSources{
    StaffNamePositionSource{4, &StaffOptionsTarget::namePos, "namePos", 5, 2, 4},
    StaffNamePositionSource{66, &StaffOptionsTarget::namePosAbbrv, "namePosAbbrv", 5, 2, 4},
    StaffNamePositionSource{79, &StaffOptionsTarget::groupNameFullPos, "groupNameFullPos", 2, 3, 3},
    StaffNamePositionSource{80, &StaffOptionsTarget::groupNameAbbrvPos, "groupNameAbbrvPos", 2, 3,
                            3},
};

constexpr std::uint8_t namePositionAlignmentBitCount = 2;
constexpr std::uint8_t namePositionExpandBit = 15;
constexpr std::int16_t earlyStaffNameVerticalEfixPerPoint = 3;

std::int64_t extractStaffNamePositionBits(std::uint16_t value, std::uint8_t firstBit,
                                          std::uint8_t bitCount)
{
    const auto mask = (std::uint16_t{1} << bitCount) - 1U;
    return static_cast<std::int64_t>((value >> firstBit) & mask);
}

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
void reportStaffNamePositionField(const ImportContext& context, std::string member,
                                  ValueOrigin origin, std::int64_t rawValue,
                                  const GlobalSelectorWords* source = nullptr,
                                  std::uint16_t selector = 0)
{
    FieldInfo info{origin, source ? source->blockOffset : 0, source ? source->decodedOffset : 0,
                   rawValue};
    if (source)
    {
        info.sourceIdentity = context.profile.epoch == FormatEpoch::ZlibLegacy
                                  ? numericGlobalClass(selector)
                                  : numericGlobalTag(selector);
    }
    context.report.setField(instanceKey<StaffOptionsTarget>(), std::move(member), std::move(info));
}
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

void reportStaffNamePositionUnavailable(const ImportContext& context,
                                        const StaffNamePositionSource& descriptor,
                                        const StaffNamePositioning& position)
{
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    const auto reportDefault = [&](std::string_view leaf, std::int64_t value)
    {
        reportStaffNamePositionField(context,
                                     std::string(descriptor.member).append(".").append(leaf),
                                     ValueOrigin::Finale27Default, value);
    };
    reportDefault("horzOff", position.horzOff);
    reportDefault("vertOff", position.vertOff);
    reportDefault("justify", static_cast<std::int64_t>(position.justify));
    reportDefault("hAlign", static_cast<std::int64_t>(position.hAlign));
    reportDefault("expand", position.expand);
#else
    static_cast<void>(context);
    static_cast<void>(descriptor);
    static_cast<void>(position);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

void importStaffNamePosition(const ImportContext& context, StaffOptionsTarget& target,
                             const StaffNamePositionSource& descriptor, bool recoverSource = true,
                             bool earlyStaffLayout = false)
{
    auto& position = target.*descriptor.target;
    if (!position)
        position = std::make_shared<StaffNamePositioning>(context.document);

    const auto source = recoverSource
                            ? readGlobalWords(context.index, context.profile, descriptor.selector)
                            : GlobalSelectorWords{};
    if (!source.present || source.words.size() <= descriptor.flagSlot)
    {
        reportStaffNamePositionUnavailable(context, descriptor, *position);
    }
    else
    {
        position->horzOff = source.words[0];
        const auto flags = static_cast<std::uint16_t>(source.words[descriptor.flagSlot]);
        const auto justification =
            extractStaffNamePositionBits(flags, 0, descriptor.justificationBitCount);
        const auto lastAlignJustify = static_cast<std::int64_t>(musx::dom::AlignJustify::Center);
        const auto previousAlignment = position->hAlign;
        const auto alignment = earlyStaffLayout
                                   ? justification
                                   : extractStaffNamePositionBits(flags,
                                         descriptor.alignmentFirstBit,
                                         namePositionAlignmentBitCount);
        if (earlyStaffLayout)
        {
            // The earlier record stores a font tuple rather than packed alignment and expand
            // flags. Its vertical conversion depends on unavailable font metrics, so use a
            // uniform point-size approximation.
            position->vertOff = source.words[1] +
                                earlyStaffNameVerticalEfixPerPoint * source.words[3];
        }
        else
        {
            position->vertOff = source.words[1];
        }
        if (justification <= lastAlignJustify)
        {
            position->justify = static_cast<musx::dom::AlignJustify>(justification);
        }
        if (alignment <= lastAlignJustify)
        {
            position->hAlign = static_cast<musx::dom::AlignJustify>(alignment);
        }
        if (!earlyStaffLayout)
        {
            position->expand = extractStaffNamePositionBits(flags, namePositionExpandBit, 1) != 0;
        }

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        const auto reportRecovered = [&](std::string_view leaf, std::int64_t value)
        {
            reportStaffNamePositionField(
                context, std::string(descriptor.member).append(".").append(leaf),
                ValueOrigin::LegacyMus, value, &source, descriptor.selector);
        };
        reportRecovered("horzOff", source.words[0]);
        reportStaffNamePositionField(context,
                                     std::string(descriptor.member).append(".vertOff"),
                                     earlyStaffLayout ? ValueOrigin::LegacyMusAdjusted
                                                      : ValueOrigin::LegacyMus,
                                     position->vertOff, &source, descriptor.selector);
        if (justification <= lastAlignJustify)
        {
            reportRecovered("justify", justification);
        }
        else
        {
            reportStaffNamePositionField(context, std::string(descriptor.member).append(".justify"),
                                         ValueOrigin::Finale27Default,
                                         static_cast<std::int64_t>(position->justify));
        }
        if (alignment <= lastAlignJustify)
        {
            auto alignmentOrigin = ValueOrigin::LegacyMus;
            if (earlyStaffLayout)
            {
                alignmentOrigin = previousAlignment == position->hAlign
                                      ? ValueOrigin::Finale27Default
                                      : ValueOrigin::LegacyBehavior;
            }
            reportStaffNamePositionField(
                context, std::string(descriptor.member).append(".hAlign"),
                alignmentOrigin, alignment, earlyStaffLayout ? nullptr : &source,
                earlyStaffLayout ? 0 : descriptor.selector);
        }
        else
        {
            reportStaffNamePositionField(context, std::string(descriptor.member).append(".hAlign"),
                                         ValueOrigin::Finale27Default,
                                         static_cast<std::int64_t>(position->hAlign));
        }
        if (earlyStaffLayout)
        {
            reportStaffNamePositionField(context,
                                         std::string(descriptor.member).append(".expand"),
                                         ValueOrigin::Finale27Default, position->expand);
        }
        else
        {
            reportRecovered("expand", position->expand);
        }
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    }

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    reportStaffNamePositionField(context,
                                 std::string(descriptor.member).append(".indivPos"),
                                 ValueOrigin::Finale27Default, position->indivPos);
    reportStaffNamePositionField(context,
                                 std::string(descriptor.member).append(".hidden"),
                                 ValueOrigin::Finale27Default, position->hidden);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
}

void applyCodaStaffNameBehavior(const ImportContext& context, StaffOptionsTarget& target)
{
    // Coda has no editable name-position preferences. The fixed staff-name
    // behavior replaces the apparent selector values; matching seeded leaves remain defaults.
    for (const auto member : {&StaffOptionsTarget::namePos, &StaffOptionsTarget::namePosAbbrv})
    {
        auto& position = target.*member;
        position->horzOff = codaStaffNameHorzOff;
        position->vertOff = codaStaffNameVertOff;
        position->justify = musx::dom::AlignJustify::Left;
        position->hAlign = musx::dom::AlignJustify::Left;

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        const auto prefix = member == &StaffOptionsTarget::namePos ? "namePos." : "namePosAbbrv.";
        reportStaffNamePositionField(context, std::string(prefix).append("horzOff"),
                                     ValueOrigin::LegacyBehavior, position->horzOff);
        reportStaffNamePositionField(context, std::string(prefix).append("vertOff"),
                                     ValueOrigin::LegacyBehavior, position->vertOff);
        reportStaffNamePositionField(context, std::string(prefix).append("justify"),
                                     ValueOrigin::LegacyBehavior,
                                     static_cast<std::int64_t>(position->justify));
        reportStaffNamePositionField(context, std::string(prefix).append("hAlign"),
                                     ValueOrigin::LegacyBehavior,
                                     static_cast<std::int64_t>(position->hAlign));
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    }
}

} // namespace

void importStaffOptions(const ImportContext& context)
{
    const auto pooled = context.document->getOptions()->get<StaffOptionsTarget>();
    if (!pooled)
        return;
    const auto target = std::const_pointer_cast<StaffOptionsTarget>(pooled);

    applyMappingTables({&staffScalarTable()}, context.index, context.profile, context.document,
                       context.report);

    // Before the scalar tail exists, Finale uses a fixed staff separation. A
    // structurally present tail supersedes this source-era behavior.
    if (!sourceStoresStaffScalars(context.index, context.profile))
    {
        target->staffSeparation = preFinale2008StaffSeparation;
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
        context.report.setField(instanceKey<StaffOptionsTarget>(), "staffSeparation",
                                {ValueOrigin::LegacyBehavior, 0, 0,
                                 preFinale2008StaffSeparation});
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    }

    const auto recoverNamePositionSource = context.profile.epoch != FormatEpoch::CodaBanner;
    const auto hasGroupNamePositionLayout =
        readGlobalWords(context.index, context.profile, 79).present &&
        readGlobalWords(context.index, context.profile, 80).present;
    for (const auto& descriptor : staffNamePositionSources)
    {
        const auto earlyStaffLayout =
            context.profile.epoch == FormatEpoch::UncompressedLegacy &&
            !hasGroupNamePositionLayout &&
            (descriptor.target == &StaffOptionsTarget::namePos ||
             descriptor.target == &StaffOptionsTarget::namePosAbbrv);
        importStaffNamePosition(context, *target, descriptor, recoverNamePositionSource,
                                earlyStaffLayout);
    }
    if (!recoverNamePositionSource)
    {
        applyCodaStaffNameBehavior(context, *target);
    }
}

} // namespace options
} // namespace finale_mus_reader
