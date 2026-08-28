// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "mapping_test_support.h"

namespace finale_mus_reader_tests {
namespace {

using namespace mapping;

/// @brief A document whose StemOptions already carries connections from somewhere else.
/// @details Seeded deliberately, because the capture pass must drop them: stem connections
/// belong to the document that stated them and name that document's fonts.
musx::dom::DocumentPtr makeStemDocument()
{
    using StemOptions = musx::dom::options::StemOptions;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    auto options = std::make_shared<StemOptions>(document);
    options->stemLength = 84;
    for (char32_t symbol : {U'΄', U'΅', U'Ά'}) {
        auto connection = std::make_shared<StemOptions::StemConnection>();
        connection->symbol = symbol;
        connection->fontId = 99;
        options->stemConnections.push_back(std::move(connection));
    }
    document->getOptions()->add(StemOptions::XmlNodeName, options);
    return std::move(session).finish();
}

std::shared_ptr<musx::dom::options::StemOptions> captureStems(
    const finale_mus_reader::container::ParsedContainer& parsed, const SourceProfile& profile,
    const musx::dom::DocumentPtr& document, ImportReport& report)
{
    finale_mus_reader::options::captureStemOptions(
        LegacyRecordIndex::build(parsed), profile, document, report);
    return std::const_pointer_cast<musx::dom::options::StemOptions>(
        document->getOptions()->get<musx::dom::options::StemOptions>());
}

// Finale 3.5 changed every unit in the stem family at once, and the reader decides that
// boundary from the size of the connection collection rather than from the version. The
// versions below therefore include one outside the era entirely: whatever a file claims to
// be, the collection it carries is what dates it. No tracked fixture can exercise this --
// the corpus has no publishable Finale 3.0 through 3.4 document -- so both shapes are built
// here.
void testStemPreFinale35Units()
{
    const auto tableWith = [](std::size_t slots) {
        std::vector<SyntheticRow> rows{
            {GLOBALS_CMPER, "20", {0, 0, 0, 0, 7, 5}},   // stem length and shortened length
            {GLOBALS_CMPER, "21", {0, 0, 18, 0, 0, 0}},  // reverse stem adjustment
        };
        // The connection family, whose size is the era marker. Only the first carries data.
        rows.push_back({GLOBALS_CMPER, "40", {0, 192, 12, -12, 0, 0}});
        for (std::size_t i = 1; i < slots; ++i) {
            rows.push_back({GLOBALS_CMPER, "40", {0, 0, 0, 0, 0, 0}});
        }
        return makeContainer(rows);
    };
    const auto runImport = [](const finale_mus_reader::container::ParsedContainer& parsed,
                               const SourceProfile& profile, ImportReport& report) {
        const auto document = makeStemDocument();
        const auto reference = makeStemDocument();
        const auto index = LegacyRecordIndex::build(parsed);
        finale_mus_reader::PendingReferences pending;
        musx::factory::ConstructionContext construction;
        const finale_mus_reader::ImportContext context{
            index, profile, noSource, document, reference, report, pending, construction};
        finale_mus_reader::options::importStemOptions(context);
        return document->getOptions()->get<musx::dom::options::StemOptions>();
    };

    constexpr std::size_t earlySlots = 32;
    constexpr std::size_t modernSlots = 128;
    // Staff positions become Evpu and Evpu becomes Efix, whatever the version says. The third
    // profile is a version from well outside the era, which the marker must override.
    auto coda = profileFor(0, 0);
    coda.epoch = FormatEpoch::CodaBanner;
    coda.version.reset();
    for (const auto& profile : {profileFor(3, 0), profileFor(3, 2), profileFor(15, 0), coda}) {
        ImportReport report(FormatEpoch::UncompressedLegacy);
        const auto options = runImport(tableWith(earlySlots), profile, report);
        expectMapping(options->stemLength == 7 * 12 && options->shortStemLength == 5 * 12
                && options->revStemAdj == 18 * 12,
            "A pre-Finale-3.5 stem length was not converted from staff positions");
        expectMapping(options->stemConnections.size() == 1
                && options->stemConnections[0]->upStemVert == 12 * 64,
            "A pre-Finale-3.5 stem adjustment was not converted from Evpu");
        // The half-stem length is not stored in that era, so it keeps the seeded default.
        expectMapping(field(report, "options.stemOptions.halfStemLength").origin
                == ValueOrigin::Finale27Default,
            "A pre-Finale-3.5 document claimed a half-stem length it does not store");
    }
    // The same words in the later shape are already in the modern units, and the version that
    // would have said otherwise is ignored.
    for (const auto& profile : {profileFor(3, 0), profileFor(5, 0), profileFor(15, 0)}) {
        ImportReport report(FormatEpoch::UncompressedLegacy);
        const auto options = runImport(tableWith(modernSlots), profile, report);
        expectMapping(options->stemLength == 7 && options->shortStemLength == 5
                && options->revStemAdj == 18,
            "A Finale 3.5 stem length was scaled as though it were staff positions");
        expectMapping(options->stemConnections[0]->upStemVert == 12,
            "A Finale 3.5 stem adjustment was scaled as though it were Evpu");
    }
}

// Nothing about the collection may come from the pinned baseline, and a source that states
// no connections must produce none rather than inheriting three.
void testStemConnectionsAreSourceOwned()
{
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const auto document = makeStemDocument();
    const auto options = captureStems(
        makeContainer({{GLOBALS_CMPER, "94", {1, 2, 3, 4, 5, 6}}}),
        profileFor(5, 0), document, report);
    expectMapping(options->stemConnections.empty(),
        "A document with no stem-connection record kept the seeded connections");
    expectMapping(options->stemLength == 84,
        "Clearing the collection disturbed the seeded scalars");
}

// A Finale 2012 record frequently holds a stale copy of the pre-Unicode default table, which
// through the widened element reads as one implausible symbol. Recovering nothing is what
// Finale sees; recovering the old table would assert a layout the era does not use.
void testStemStaleUnicodeRecord()
{
    using finale_mus_reader::numericGlobalClass;
    // The bytes an untouched Finale 2012 document carries: the pre-Unicode table, whose
    // first two words the widened element reads as the single symbol 0x030000c0.
    const auto parsed = makeClassContainer(numericGlobalClass(40),
        {0, 192, 768, -768, 0, 0, 0, 131, -2304, 2304, -1024, 1024, 0, 0},
        ByteOrder::LittleEndian);
    SourceProfile profile(FormatEpoch::ZlibLegacy);
    profile.byteOrder = ByteOrder::LittleEndian;
    SourceVersion version;
    version.major = 17;
    profile.version = version;

    ImportReport report(FormatEpoch::UncompressedLegacy);
    const auto document = makeStemDocument();
    const auto options = captureStems(parsed, profile, document, report);
    expectMapping(options->stemConnections.empty(),
        "A stale pre-Unicode stem record was read as though it were the widened layout");
    expectMapping(std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                      [](const auto& entry) {
                          return entry.message.find("not a Unicode codepoint")
                              != std::string::npos;
                      }),
        "The stale stem-connection record produced no diagnostic");
    expectMapping(anyMappingReportedField(report, [](const auto& member, const auto& value) {
                      return member == "stemConnections[0].symbol"
                          && value.rawValue == 0x030000c0;
                  }),
        "The out-of-range symbol was not reported as raw evidence");

    // The same words in a pre-Unicode file are the table they always were.
    SourceProfile earlier = profile;
    SourceVersion narrow;
    narrow.major = 13;
    earlier.version = narrow;
    ImportReport narrowReport(FormatEpoch::UncompressedLegacy);
    const auto narrowDocument = makeStemDocument();
    const auto narrowOptions = captureStems(parsed, earlier, narrowDocument, narrowReport);
    expectMapping(narrowOptions->stemConnections.size() == 2
            && narrowOptions->stemConnections[0]->symbol == 192
            && narrowOptions->stemConnections[1]->symbol == 131,
        "The twelve-byte zlib element stopped decoding before Finale 2012");
}

// A connection names its font by comparator. A dangling one is preserved rather than replaced,
// because a default would invent a typeface the source never named; registering it is what lets
// musxdom mint and log a placeholder for it at the end of construction instead of leaving the
// comparator unusable, which is what testDanglingFontComparatorRequiresRegistration verifies.
void testStemFontReferenceValidation()
{
    ImportReport report(FormatEpoch::UncompressedLegacy);
    const auto document = makeStemDocument();
    const auto options = captureStems(
        makeContainer({{GLOBALS_CMPER, "40", {7, 192, 768, -768, 0, 0}},
            {GLOBALS_CMPER, "40", {0, 0, 0, 0, 0, 0}}}),
        profileFor(5, 0), document, report);
    musx::factory::ConstructionContext construction;
    finale_mus_reader::options::validateStemOptions(document, construction);
    expectMapping(options->stemConnections.size() == 1
            && options->stemConnections[0]->fontId == 7,
        "A stem connection did not keep the font comparator the source stated");
}

// No real Finale save can stand in for a comparator that resolves to nothing: Finale always
// writes some definition for every comparator it uses, even a placeholder of its own. The
// state this reader must survive -- a hand-edited or otherwise malformed source naming a font
// id its own table never defines -- has to be built synthetically instead. This constructs it
// directly against musxdom's own construction session, twice: once exactly as an importer that
// forgot to register the comparator would leave it, and once as the reader actually does.
//
// Both documents give a TextOptions symbol insert a font id no FontDefinition in the document
// answers. The only difference is whether that id is registered with the session's own
// ConstructionContext before the session finishes -- which is what
// options::registerSymbolInsertFonts does in the real pipeline -- and that difference is the
// whole story: unregistered, FontInfo::getName throws exactly as it did before musxdom offered
// a placeholder; registered, the same call resolves to musxdom's "Missing Font (n)" spelling.
void testDanglingFontComparatorRequiresRegistration()
{
    using TextOptions = musx::dom::options::TextOptions;
    using Insert = musx::dom::options::AccidentalInsertSymbolType;
    constexpr musx::dom::Cmper danglingFontId = 909;

    const auto buildDocument = [](bool registerComparator) {
        auto session = musx::factory::DocumentFactory::begin();
        const auto document = session.getDocument();
        auto options = std::make_shared<TextOptions>(document);
        options->textLineSpacingPercent = 100;
        auto insert = std::make_shared<TextOptions::InsertSymbolInfo>(options);
        auto font = std::make_shared<musx::dom::FontInfo>(document, /*sizeIsPercent*/ true);
        font->fontId = danglingFontId;
        font->fontSize = 12;
        insert->symFont = std::move(font);
        options->symbolInserts[Insert::Sharp] = std::move(insert);
        document->getOptions()->add(TextOptions::XmlNodeName, options);
        if (registerComparator) {
            session.getConstructionContext().registerFontId(danglingFontId);
        }
        return std::move(session).finish();
    };

    const auto getSymFont = [](const musx::dom::DocumentPtr& document) {
        return document->getOptions()->get<TextOptions>()
            ->symbolInserts.at(Insert::Sharp)->symFont;
    };

    const auto unregistered = buildDocument(false);
    expectMapping(getSymFont(unregistered)->fontId == danglingFontId,
        "An unregistered dangling font comparator did not survive construction");
    bool threw = false;
    try {
        (void)getSymFont(unregistered)->getName();
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expectMapping(threw,
        "An unregistered dangling font comparator no longer throws out of FontInfo::getName; "
        "the contrast this test relies on is gone, not just the registered half of it");

    const auto registered = buildDocument(true);
    expectMapping(getSymFont(registered)->fontId == danglingFontId,
        "A registered dangling font comparator did not survive construction");
    std::string name;
    bool registeredThrew = false;
    try {
        name = getSymFont(registered)->getName();
    } catch (const std::exception&) {
        registeredThrew = true;
    }
    expectMapping(!registeredThrew,
        "A registered dangling font comparator threw out of FontInfo::getName instead of "
        "resolving to musxdom's placeholder");
    expectMapping(name == "Missing Font (" + std::to_string(danglingFontId) + ")",
        "A registered dangling font comparator did not resolve to the placeholder spelling "
        "Finale's own conversions use");
}


TEST_CASE("Stem pre-Finale-3.5 units", "[mapping]") { testStemPreFinale35Units(); }
TEST_CASE("Stem connections are source owned", "[mapping]") { testStemConnectionsAreSourceOwned(); }
TEST_CASE("Stale Unicode stem record", "[mapping]") { testStemStaleUnicodeRecord(); }
TEST_CASE("Stem font reference validation", "[mapping]") { testStemFontReferenceValidation(); }
TEST_CASE("Dangling font comparator requires registration", "[mapping]") { testDanglingFontComparatorRequiresRegistration(); }

} // namespace
} // namespace finale_mus_reader_tests
