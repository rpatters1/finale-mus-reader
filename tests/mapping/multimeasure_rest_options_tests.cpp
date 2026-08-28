// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "mapping_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace mapping;

/// @brief A document whose MultimeasureRestOptions carries the pinned baseline's values.
/// @details The three the early era cannot state are what matter: the baseline starts the
/// H-bar 30 Evpu in, ends it 30 Evpu out, and switches automatic updating on.
///
/// `noHorizontalStretch` is seeded **true**, which the pinned baseline is not. That option
/// arrived with Finale 27, so no legacy document can state it and the reader must assert it
/// false rather than take the baseline's word; seeding the baseline's own false would let an
/// implementation that merely inherited pass this test.
musx::dom::DocumentPtr makeMmRestDocument()
{
    using MmRest = musx::dom::options::MultimeasureRestOptions;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto options = std::make_shared<MmRest>(document);
    options->measWidth = 360;
    options->numAdjY = -32;
    options->shapeDef = 3;
    options->numStart = 2;
    options->useSymsThreshold = 9;
    options->symSpacing = 48;
    options->startAdjust = 30;
    options->endAdjust = -30;
    options->autoUpdateMmRests = true;
    options->noHorizontalStretch = true;
    document->getOptions()->add(MmRest::XmlNodeName, options);
    return std::move(session).finish();
}

// Finale 3.5 rewrote the multimeasure-rest record, and the reader decides that boundary from
// the size of the selector-25 family rather than from the version. The boundary falls inside
// the uncompressed epoch, so no epoch gate can express it, and no tracked fixture can exercise
// the uncompressed half of the early layout -- the corpus has no publishable Finale 3.0 or 3.2
// document -- so both shapes are built here, and each is read under versions that would
// contradict the marker if the marker were not what decides.
void testMmRestEarlyLayoutMarker()
{
    using MmRest = musx::dom::options::MultimeasureRestOptions;
    const auto runImport = [](const std::vector<SyntheticRow>& rows,
                               const SourceProfile& profile, ImportReport& report) {
        const auto document = makeMmRestDocument();
        const auto reference = makeMmRestDocument();
        const auto index = LegacyRecordIndex::build(makeContainer(rows));
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importMultimeasureRestOptions(context);
        return document->getOptions()->get<MmRest>();
    };

    // The early record: one incidence, the adjustment and shape in slots 4 and 5. Read through
    // the later table these same words would give a shape of 7 and a "start number at" of -20.
    const std::vector<SyntheticRow> early{{GLOBALS_CMPER, "25", {320, 3, 24, 7, -20, 5}}};
    // The later record: two incidences, everything in the framework's places.
    const std::vector<SyntheticRow> later{
        {GLOBALS_CMPER, "25", {216, 0, -20, 5, 3, 11}},
        {GLOBALS_CMPER, "25", {60, -4, 12, -12, 0, 1}},
    };

    auto coda = profileFor(2, 6);
    coda.epoch = FormatEpoch::CodaBanner;
    coda.version.reset();
    // Finale 3.0 and 3.2 by version, one version from well past the boundary, and a
    // Coda-banner file that states no version at all.
    for (const auto& profile : {profileFor(3, 0), profileFor(3, 2), profileFor(15, 0), coda}) {
        ImportReport report(FormatEpoch::UncompressedLegacy);
        const auto options = runImport(early, profile, report);
        expectMapping(options->measWidth == 320 && options->numAdjY == -20
                && options->shapeDef == 5,
            "The early multimeasure-rest layout was not read from its own slots");
        expectMapping(options->numStart == 2 && options->useSymsThreshold == 9
                && options->symSpacing == 48,
            "The early layout disturbed a field the baseline supplies");
        // The three the baseline gets wrong for this era, plus the Finale 27 option no legacy
        // era has at all.
        expectMapping(options->startAdjust == 0 && options->endAdjust == 0
                && !options->autoUpdateMmRests && !options->noHorizontalStretch,
            "An early document inherited a value its era cannot have stated");
        expectMapping(field(report, "options.multimeasureRestOptions.startAdjust").origin
                == ValueOrigin::LegacyBehavior,
            "An asserted early value was not reported as era behavior");
        expectMapping(field(report, "options.multimeasureRestOptions.symSpacing").origin
                == ValueOrigin::Finale27Default,
            "An early field the baseline supplies was claimed as read");
        // These documents recover shape 5 and define no shapes at all, which is the ordinary
        // Finale case rather than a fault: hundreds of corpus documents name an H-bar shape
        // their own file never carries. It must be noted, and noted at Info, so that a host
        // filtering for real problems does not see several hundred false ones.
        expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                          [](const finale_mus_reader::Diagnostic& entry) {
                              return entry.level == musx::util::Logger::LogLevel::Info
                                  && entry.message.find("H-bar") != std::string::npos;
                          }),
            "A multimeasure-rest H-bar naming an undefined shape was not noted at Info");
    }

    // The same reader on the two-incidence record, including under versions that predate the
    // boundary and under the Coda-banner epoch. Whatever a file claims to be, the record it
    // carries is what decides: the epoch mask says only that these are 16-byte rows.
    for (const auto& profile :
            {profileFor(3, 0), profileFor(3, 5), profileFor(15, 0), coda}) {
        ImportReport report(FormatEpoch::UncompressedLegacy);
        const auto options = runImport(later, profile, report);
        expectMapping(options->measWidth == 216 && options->numAdjY == -20
                && options->shapeDef == 5 && options->numStart == 3
                && options->useSymsThreshold == 11,
            "The later multimeasure-rest layout was not read from its own slots");
        expectMapping(options->symSpacing == 60 && options->numAdjX == -4
                && options->startAdjust == 12 && options->endAdjust == -12
                && options->useSymbols,
            "The later layout's second incidence was not read");
        expectMapping(field(report, "options.multimeasureRestOptions.startAdjust").origin
                == ValueOrigin::LegacyMus,
            "A recovered H-bar adjustment was reported as era behavior");
        // Asserted in every era, not only the early one, because no legacy format has it.
        expectMapping(!options->noHorizontalStretch
                && field(report, "options.multimeasureRestOptions.noHorizontalStretch").origin
                    == ValueOrigin::LegacyBehavior,
            "A later-layout document inherited the baseline's horizontal-stretch setting");
        // Selector 83 is still absent, which is what a Finale 3.5 or 3.7 document looks like.
        expectMapping(!options->autoUpdateMmRests
                && field(report, "options.multimeasureRestOptions.autoUpdateMmRests").origin
                    == ValueOrigin::LegacyBehavior,
            "A document without selector 83 kept the baseline's automatic-update setting");
    }

    // With the selector present, the flag is read rather than asserted, and from word 4.
    ImportReport report(FormatEpoch::UncompressedLegacy);
    auto withUpdate = later;
    withUpdate.push_back({GLOBALS_CMPER, "83", {0, 0, 1, 0, 1, 0}});
    const auto options = runImport(withUpdate, profileFor(5, 0), report);
    expectMapping(options->autoUpdateMmRests
            && field(report, "options.multimeasureRestOptions.autoUpdateMmRests").origin
                == ValueOrigin::LegacyMus,
        "The automatic-update word was not read from selector 83");

}

TEST_CASE("Multimeasure rest layout marker", "[mapping]") { testMmRestEarlyLayoutMarker(); }

} // namespace
} // namespace finale_mus_reader_tests
