// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "class_test_support.h"
#include "coverage/registry.h"
#include <set>

namespace finale_mus_reader_tests {
namespace {

using namespace classes;

TEST_CASE("File locator synthesis requires its same-side referents", "[file_path][coverage]")
{
    using namespace finale_mus_reader::coverage;
    using PathType = musx::dom::others::FileDescription::PathType;
    ImportReport report(FormatEpoch::ZlibLegacy);
    const std::string type = "file_descriptions[cmper=1].path_type";
    const std::string graphic = "page_graphic_assigns[cmper=1,inci=0].graphic_cmper";
    const std::string alias = "file_aliases[cmper=1]";
    const std::string bookmark = "file_url_bookmarks[cmper=1]";
    ComparisonLeaves source{
        {type, {Value(static_cast<int>(PathType::DosPath)), "legacy-mus"}},
        {"page_graphic_assigns[cmper=1,inci=0].f_desc_id", {Value(1), "legacy-mus"}},
        {graphic, {Value(2), "legacy-mus"}}};
    ComparisonLeaves companion{
        {type, {Value(static_cast<int>(PathType::MacAlias)), ""}},
        {alias + ".share_mode", {Value(0), ""}}, {alias + ".length", {Value(3), ""}},
        {alias + ".alias_handle", {Value(Value::Blob{0, 1, 255}), ""}},
        {bookmark + ".share_mode", {Value(0), ""}}, {bookmark + ".length", {Value(3), ""}},
        {bookmark + ".url_bookmark_data", {Value(Value::Blob{0, 2, 255}), ""}}};
    const ComparisonLeaves empty;
    const Value absent;
    const auto classify = [&](const std::string& object, std::string_view member,
                              DifferenceCategory category = DifferenceCategory::CompanionOnly) {
        const auto path = object + "." + std::string(member);
        const auto fn = differenceClassifier(object.substr(0, object.find('[')));
        REQUIRE(fn);
        DifferenceContext context{path, category, "", absent, companion.at(path).first,
            empty, empty, FormatEpoch::ZlibLegacy, ByteOrder::LittleEndian, nullptr, report};
        context.sourceDocumentLeaves = &source;
        context.companionDocumentLeaves = &companion;
        return fn(context);
    };
    for (const auto member : {"share_mode", "length", "alias_handle"})
        CHECK(classify(alias, member) == DifferenceClassification::FinaleUpgradeSynthesis);
    for (const auto member : {"share_mode", "length", "url_bookmark_data"})
        CHECK(classify(bookmark, member) == DifferenceClassification::FinaleUpgradeSynthesis);
    source[graphic].first = Value(0);
    CHECK_FALSE(classify(alias, "alias_handle"));
    source.erase(graphic);
    CHECK_FALSE(classify(alias, "alias_handle"));
    source[graphic].first = Value(2);
    source[type].first = Value(static_cast<int>(PathType::MacFsSpec));
    CHECK_FALSE(classify(alias, "alias_handle"));
    source[type].first = Value(static_cast<int>(PathType::DosPath));
    companion[type].first = Value(static_cast<int>(PathType::DosPath));
    CHECK_FALSE(classify(alias, "alias_handle"));
    companion[type].first = Value(static_cast<int>(PathType::MacAlias));
    source[type].second = "unmapped";
    CHECK_FALSE(classify(alias, "alias_handle"));
    source[type].second = "legacy-mus";
    source[alias + ".length"] = {Value(3), "legacy-mus"};
    CHECK_FALSE(classify(alias, "alias_handle"));
    source.erase(alias + ".length");
    source[bookmark + ".length"] = {Value(3), "legacy-mus"};
    CHECK_FALSE(classify(bookmark, "url_bookmark_data"));
    source.erase(bookmark + ".length");
    CHECK_FALSE(classify(bookmark, "url_bookmark_data", DifferenceCategory::Differs));
    CHECK_FALSE(classify(bookmark, "url_bookmark_data", DifferenceCategory::ReaderOnly));
    companion[alias + ".length"].first = Value(4);
    CHECK_FALSE(classify(bookmark, "url_bookmark_data"));
    companion.erase(alias + ".length");
    CHECK_FALSE(classify(bookmark, "url_bookmark_data"));
    companion[alias + ".length"] = {Value(3), ""};
    companion.erase(alias + ".alias_handle");
    CHECK_FALSE(classify(bookmark, "url_bookmark_data"));
}

TEST_CASE("Coverage blobs use whole-vector equality", "[file_path][coverage]")
{
    using finale_mus_reader::coverage::Value;
    const Value blob(Value::Blob{0, 1, 254, 255});
    CHECK(blob == Value(Value::Blob{0, 1, 254, 255}));
    CHECK_FALSE(blob == Value(Value::Blob{0, 2, 254, 255}));
    CHECK_FALSE(blob == Value(Value::Blob{0, 1, 254}));
    CHECK_FALSE(blob.isArray());
    CHECK(blob.toJson() == "[0,1,254,255]");
    const Value empty(Value::Blob{});
    CHECK(empty.isBlob());
    CHECK(empty == Value(Value::Blob{}));
    CHECK(empty.toJson() == "[]");
}

TEST_CASE("Only embedded graphic path differences are classified", "[file_path][coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto classify = differenceClassifier("file_paths");
    REQUIRE(classify);
    ImportReport report(FormatEpoch::ZlibLegacy);
    const Value original("original-image.tiff"), replacement("any-other-filename");
    for (const auto assignment : {"page_graphic_assigns[cmper=1,inci=0]",
             "shape_graphic_assigns[cmper=1,inci=0]",
             "meas_graphic_assigns[cmper1=1,cmper2=2,inci=0]",
             "page_graphic_assigns[part_id=2,cmper=1,inci=0]"}) {
        const std::string path = "file_paths[cmper=7].path";
        const std::string desc = std::string(assignment) + ".f_desc_id";
        const std::string graphic = std::string(assignment) + ".graphic_cmper";
        ComparisonLeaves source{{path, {original, "legacy-mus"}},
            {"file_descriptions[cmper=20].path_id", {Value(7), "legacy-mus"}},
            {desc, {Value(20), "legacy-mus"}}, {graphic, {Value(3), "legacy-mus"}}};
        const ComparisonLeaves companion{{path, {replacement, ""}}};
        const ComparisonLeaves locatorOnly{{path, {original, "legacy-mus"}}};
        DifferenceContext context{path, DifferenceCategory::Differs, "legacy-mus",
            original, replacement, locatorOnly, companion, FormatEpoch::ZlibLegacy,
            ByteOrder::LittleEndian, nullptr, report};
        context.sourceDocumentLeaves = &source;
        CHECK(classify(context) == DifferenceClassification::FinaleUpgradeNormalization);
        source[graphic].first = Value(0);
        CHECK_FALSE(classify(context));
        source.erase(graphic);
        CHECK_FALSE(classify(context));
        source[graphic].first = Value(3);
        source[desc].first = Value(21);
        CHECK_FALSE(classify(context));
        source[desc].first = Value(20);
        source["file_descriptions[cmper=20].path_id"].first = Value(8);
        CHECK_FALSE(classify(context));
        source["file_descriptions[cmper=20].path_id"].first = Value(7);
        context.category = DifferenceCategory::ReaderOnly;
        CHECK_FALSE(classify(context));
        context.category = DifferenceCategory::CompanionOnly;
        CHECK_FALSE(classify(context));
        context.category = DifferenceCategory::Differs;
        context.origin = "unmapped";
        CHECK_FALSE(classify(context));
        context.origin = "legacy-mus";
        context.path = "file_descriptions[cmper=20].path_type";
        CHECK_FALSE(classify(context));
        context.path = path;
        source["page_graphic_assigns[cmper=99,inci=0].f_desc_id"] = {Value(20), "legacy-mus"};
        source["page_graphic_assigns[cmper=99,inci=0].graphic_cmper"] = {Value(0), "legacy-mus"};
        CHECK_FALSE(classify(context));
    }
}

TEST_CASE("Embedded alias normalization excludes linked and missing aliases", "[file_path][coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto classify = differenceClassifier("file_aliases");
    REQUIRE(classify);
    ImportReport report(FormatEpoch::ZlibLegacy);
    Value original(3), replacement(5);
    const std::string lengthPath = "file_aliases[cmper=20].length";
    for (const auto assignment : {"page_graphic_assigns[cmper=1,inci=0]",
             "shape_graphic_assigns[cmper=1,inci=0]",
             "meas_graphic_assigns[cmper1=1,cmper2=2,inci=0]",
             "page_graphic_assigns[part_id=2,cmper=1,inci=0]"}) {
        const std::string graphic = std::string(assignment) + ".graphic_cmper";
        original = Value(3);
        replacement = Value(5);
        ComparisonLeaves source{{lengthPath, {original, "legacy-mus"}},
            {std::string(assignment) + ".f_desc_id", {Value(20), "legacy-mus"}},
            {"file_descriptions[cmper=20].path_id", {Value(7), "legacy-mus"}},
            {graphic, {Value(1), "legacy-mus"}}};
        ComparisonLeaves companion{{lengthPath, {replacement, ""}}};
        const ComparisonLeaves locatorOnly{{lengthPath, {original, "legacy-mus"}}};
        DifferenceContext context{lengthPath, DifferenceCategory::Differs, "legacy-mus",
            original, replacement, locatorOnly, companion, FormatEpoch::ZlibLegacy,
            ByteOrder::LittleEndian, nullptr, report};
        context.sourceDocumentLeaves = &source;
        CHECK(classify(context) == DifferenceClassification::FinaleUpgradeNormalization);
        original = Value(Value::Blob{0, 18, 255});
        replacement = Value(Value::Blob{0, 19, 255, 0, 0});
        context.path = "file_aliases[cmper=20].alias_handle";
        context.origin = "legacy-mus-adjusted";
        CHECK(classify(context) == DifferenceClassification::FinaleUpgradeNormalization);
        context.origin = "unmapped";
        CHECK_FALSE(classify(context));
        context.origin = "";
        context.category = DifferenceCategory::CompanionOnly;
        CHECK_FALSE(classify(context));
        context.category = DifferenceCategory::ReaderOnly;
        context.origin = "legacy-mus-adjusted";
        CHECK_FALSE(classify(context));
        context.category = DifferenceCategory::Differs;
        source[graphic].first = Value(0);
        CHECK_FALSE(classify(context));
        source.erase(graphic);
        CHECK_FALSE(classify(context));
        source[graphic].first = Value(1);
        companion.clear();
        CHECK_FALSE(classify(context));
        companion[lengthPath] = {Value(5), ""};
        source["page_graphic_assigns[cmper=99,inci=0].f_desc_id"] = {Value(20), "legacy-mus"};
        source["page_graphic_assigns[cmper=99,inci=0].graphic_cmper"] = {Value(0), "legacy-mus"};
        CHECK_FALSE(classify(context));
    }
}

TEST_CASE("Only embedded DOS or MacFsSpec to MacAlias changes are classified", "[file_path][coverage]")
{
    using namespace finale_mus_reader::coverage;
    using PathType = musx::dom::others::FileDescription::PathType;
    const auto classify = differenceClassifier("file_descriptions");
    REQUIRE(classify);
    ImportReport report(FormatEpoch::DclLegacy);
    Value original(static_cast<std::int64_t>(PathType::MacFsSpec));
    Value replacement(static_cast<std::int64_t>(PathType::MacAlias));
    const std::string path = "file_descriptions[cmper=20].path_type";
    for (const auto assignment : {"page_graphic_assigns[cmper=1,inci=0]",
             "shape_graphic_assigns[cmper=1,inci=0]",
             "meas_graphic_assigns[cmper1=1,cmper2=2,inci=0]",
             "page_graphic_assigns[part_id=2,cmper=1,inci=0]"}) {
        const std::string graphic = std::string(assignment) + ".graphic_cmper";
        ComparisonLeaves source{{path, {original, "legacy-mus"}},
            {std::string(assignment) + ".f_desc_id", {Value(20), "legacy-mus"}},
            {graphic, {Value(1), "legacy-mus"}}};
        const ComparisonLeaves locatorOnly{{path, {original, "legacy-mus"}}};
        const ComparisonLeaves companion{{path, {replacement, ""}}};
        DifferenceContext context{path, DifferenceCategory::Differs, "legacy-mus",
            original, replacement, locatorOnly, companion, FormatEpoch::DclLegacy,
            ByteOrder::BigEndian, nullptr, report};
        context.sourceDocumentLeaves = &source;
        CHECK(classify(context) == DifferenceClassification::FinaleUpgradeNormalization);
        source[graphic].first = Value(0);
        CHECK_FALSE(classify(context));
        source.erase(graphic);
        CHECK_FALSE(classify(context));
        source[graphic].first = Value(1);
        for (const auto category : {DifferenceCategory::ReaderOnly, DifferenceCategory::CompanionOnly}) {
            context.category = category;
            CHECK_FALSE(classify(context));
        }
        context.category = DifferenceCategory::Differs;
        context.origin = "unmapped";
        CHECK_FALSE(classify(context));
        context.origin = "legacy-mus";
        for (const auto type : {PathType::DosPath, PathType::MacFsSpec,
                 PathType::MacPosixPath, PathType::MacUrlBookmark}) {
            replacement = Value(static_cast<std::int64_t>(type));
            CHECK_FALSE(classify(context));
        }
        replacement = Value(static_cast<std::int64_t>(PathType::MacAlias));
        original = Value(static_cast<std::int64_t>(PathType::DosPath));
        CHECK(classify(context) == DifferenceClassification::FinaleUpgradeNormalization);
        source[graphic].first = Value(0);
        CHECK_FALSE(classify(context));
        source[graphic].first = Value(1);
        original = Value(static_cast<std::int64_t>(PathType::MacPosixPath));
        CHECK_FALSE(classify(context));
        original = Value(static_cast<std::int64_t>(PathType::MacFsSpec));
        context.path = "file_descriptions[cmper=20].path_id";
        CHECK_FALSE(classify(context));
        context.path = path;
        source["page_graphic_assigns[cmper=99,inci=0].f_desc_id"] = {Value(20), "legacy-mus"};
        source["page_graphic_assigns[cmper=99,inci=0].graphic_cmper"] = {Value(0), "legacy-mus"};
        CHECK_FALSE(classify(context));
    }
}

TEST_CASE("Embedded directory IDs may change without other locator changes", "[file_path][coverage]")
{
    using namespace finale_mus_reader::coverage;
    const auto classify = differenceClassifier("file_descriptions");
    REQUIRE(classify);
    ImportReport report(FormatEpoch::ZlibLegacy);
    const Value original(18115899), replacement(18115896);
    const std::string path = "file_descriptions[cmper=2].dir_id";
    for (const auto assignment : {"page_graphic_assigns[cmper=1,inci=0]",
             "shape_graphic_assigns[cmper=1,inci=0]",
             "meas_graphic_assigns[cmper1=1,cmper2=2,inci=0]",
             "page_graphic_assigns[part_id=2,cmper=1,inci=0]"}) {
        const std::string graphic = std::string(assignment) + ".graphic_cmper";
        ComparisonLeaves source{{path, {original, "legacy-mus"}},
            {std::string(assignment) + ".f_desc_id", {Value(2), "legacy-mus"}},
            {graphic, {Value(1), "legacy-mus"}}};
        const ComparisonLeaves locatorOnly{{path, {original, "legacy-mus"}}};
        const ComparisonLeaves companion{{path, {replacement, ""}}};
        DifferenceContext context{path, DifferenceCategory::Differs, "legacy-mus",
            original, replacement, locatorOnly, companion, FormatEpoch::ZlibLegacy,
            ByteOrder::LittleEndian, nullptr, report};
        context.sourceDocumentLeaves = &source;
        CHECK(classify(context) == DifferenceClassification::FinaleUpgradeNormalization);
        source[graphic].first = Value(0);
        CHECK_FALSE(classify(context));
        source.erase(graphic);
        CHECK_FALSE(classify(context));
        source[graphic].first = Value(1);
        for (const auto category : {DifferenceCategory::ReaderOnly, DifferenceCategory::CompanionOnly}) {
            context.category = category;
            CHECK_FALSE(classify(context));
        }
        context.category = DifferenceCategory::Differs;
        context.origin = "unmapped";
        CHECK_FALSE(classify(context));
        context.origin = "legacy-mus";
        context.path = "file_descriptions[cmper=2].vol_ref_num";
        CHECK_FALSE(classify(context));
        context.path = path;
        source["page_graphic_assigns[cmper=99,inci=0].f_desc_id"] = {Value(2), "legacy-mus"};
        source["page_graphic_assigns[cmper=99,inci=0].graphic_cmper"] = {Value(0), "legacy-mus"};
        CHECK_FALSE(classify(context));
    }
}


TEST_CASE("Graphic file locators preserve controlled source values", "[file_path]")
{
    using namespace musx::dom::others;
    const auto classic = readFixture("evidence/F372/F372-page-graphic.mus");
    REQUIRE(classic.document);
    const auto desc = classic.document->getOthers()->get<FileDescription>(0, 1);
    REQUIRE(desc);
    CHECK(desc->version == 256);
    CHECK(desc->pathType == FileDescription::PathType::MacFsSpec);
    CHECK(desc->pathId == 1);
    CHECK(desc->volRefNum == -3);
    CHECK(desc->dirId == 16);
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    const auto& fields = classic.report.fields.at(finale_mus_reader::instanceKey<FileDescription>(0, musx::dom::Cmper(1)));
    std::set<std::string> names;
    for (const auto& [name, info] : fields) {
        names.insert(name);
        CHECK(info.origin == ValueOrigin::LegacyMus);
        CHECK(info.blockOffset == 0x200);
        CHECK(info.decodedOffset == 0x13b0);
        CHECK(info.sourceIdentity == finale_mus_reader::records::packTag("Fd"));
    }
    CHECK(names == std::set<std::string>{"version", "pathType", "pathId", "volRefNum", "dirId"});
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
    const auto path = classic.document->getOthers()->get<FilePath>(0, 1);
    REQUIRE(path);
    CHECK(path->path == "Photo_tiff.tiff");

    const auto dcl = readFixture("evidence/F2006/F2006-linked-tiff.mus");
    REQUIRE(dcl.document);
    const auto alias = dcl.document->getOthers()->get<FileAlias>(0, 1);
    REQUIRE(alias);
    CHECK(alias->length == 462);
    REQUIRE(alias->aliasHandle.size() == alias->length);
    CHECK(std::vector<std::uint8_t>(alias->aliasHandle.begin(), alias->aliasHandle.begin() + 8)
        == std::vector<std::uint8_t>{0, 0, 0, 0, 206, 1, 2, 0});
    CHECK(dcl.document->getOthers()->get<FileDescription>(0, 1)->dirId == 380256);
    CHECK(dcl.document->getOthers()->getArray<FileUrlBookmark>(0).empty());

    const auto zlib = readFixture("evidence/F2012/F2012-graphics-types.mus");
    REQUIRE(zlib.document);
    CHECK(zlib.document->getOthers()->getArray<FileDescription>(0).size() == 6);
    CHECK(zlib.document->getOthers()->get<FileDescription>(0, 1)->pathType
        == FileDescription::PathType::MacAlias);
    CHECK(zlib.document->getOthers()->get<FileDescription>(0, 1)->dirId == 341052);
    CHECK(zlib.document->getOthers()->get<FilePath>(0, 1)->path == "GifSample.gif");
    CHECK(zlib.document->getOthers()->get<FileAlias>(0, 1)->length == 392);
    CHECK(zlib.document->getOthers()->getArray<FileUrlBookmark>(0).empty());
}

TEST_CASE("File locator surveyors expose every persisted member", "[file_path][coverage]")
{
    using namespace musx::dom::others;
    auto session = musx::factory::DocumentFactory::begin();
    const auto document = session.getDocument();
    const auto add = [&]<typename Target>() {
        auto target = std::make_shared<Target>(document,
            musx::dom::SCORE_PARTID, musx::dom::EnigmaBase::ShareMode::All, musx::dom::Cmper(1));
        if constexpr (std::is_same_v<Target, FileAlias>) {
            target->aliasHandle = {0, 1, 254, 255};
            target->length = target->aliasHandle.size();
        } else if constexpr (std::is_same_v<Target, FileUrlBookmark>) {
            target->urlBookmarkData = {0, 1, 254, 255};
            target->length = target->urlBookmarkData.size();
        }
        document->getOthers()->add(Target::XmlNodeName, std::move(target));
    };
    add.operator()<FileAlias>();
    add.operator()<FileDescription>();
    add.operator()<FilePath>();
    add.operator()<FileUrlBookmark>();
    ImportReport report(FormatEpoch::ZlibLegacy);
    const auto result = finale_mus_reader::coverage::runAllSurveyors({document, report});
    const auto check = [&](const char* key, const std::set<std::string>& expected) {
        const auto& rows = result.snapshot.at(key).asArray();
        REQUIRE(rows.size() == 1);
        std::set<std::string> members;
        for (const auto& [name, value] : rows.front().asObject()) {
            if (name != "cmper" && name != "part_id" && name != "share_mode" && name != "origin")
                members.insert(name);
        }
        CHECK(members == expected);
        CHECK(finale_mus_reader::coverage::surveyorPool(key) == "others");
    };
    check("file_aliases", {"length", "origin_length", "alias_handle", "origin_aliasHandle"});
    check("file_descriptions", {"version", "origin_version", "vol_ref_num", "origin_volRefNum",
        "dir_id", "origin_dirId", "path_type", "origin_pathType", "path_id", "origin_pathId"});
    check("file_paths", {"path", "origin_path"});
    check("file_url_bookmarks", {"length", "origin_length", "url_bookmark_data",
        "origin_urlBookmarkData"});
    for (const auto& [key, blob] : {std::pair{"file_aliases", "alias_handle"},
             std::pair{"file_url_bookmarks", "url_bookmark_data"}}) {
        const auto& item = result.snapshot.at(key).asArray().front().asObject();
        REQUIRE(item.at(blob).isBlob());
        CHECK(item.at(blob).asBlob() == finale_mus_reader::coverage::Value::Blob{0, 1, 254, 255});
        CHECK(item.at("length").asInteger() == 4);
    }
}

TEST_CASE("File locators bound opaque lengths and report all fields", "[file_path]")
{
    using namespace musx::dom::others;
    for (const auto epoch : {FormatEpoch::CodaBanner, FormatEpoch::UncompressedLegacy,
             FormatEpoch::DclLegacy, FormatEpoch::ZlibLegacy}) {
        for (const auto order : {ByteOrder::BigEndian, ByteOrder::LittleEndian}) {
            const auto pathType = order == ByteOrder::LittleEndian ? std::int16_t{1} : std::int16_t{3};
            const std::vector<std::int16_t> description{256, pathType, 7, -5,
                order == ByteOrder::BigEndian ? std::int16_t{1} : std::int16_t{2},
                order == ByteOrder::BigEndian ? std::int16_t{2} : std::int16_t{1}};
            const auto parsed = epoch == FormatEpoch::ZlibLegacy
                ? makeClassContainer({SyntheticClassRow{0x008a, description, 7},
                      SyntheticClassRow{0x0089, {3, 0, 0x1234, 0x5678}, 7},
                      SyntheticClassRow{0x0089, {100, 0, 0, 0}, 8},
                      SyntheticClassRow{0x008a, {256}, 8}}, order)
                : makeContainer({{7, "Fd", {description[0], description[1], description[2],
                      description[3], description[4], description[5]}},
                      {7, "Fa", {3, 0, 0x1234, 0x5678, 0, 0}},
                      {8, "Fa", {100, 0, 0, 0, 0, 0}}}, epoch, order);
            const auto index = LegacyRecordIndex::build(parsed);
            auto session = musx::factory::DocumentFactory::begin();
            const auto document = session.getDocument();
            auto referenceSession = musx::factory::DocumentFactory::begin();
            const auto reference = std::move(referenceSession).finish();
            ImportReport report(epoch);
            finale_mus_reader::PendingReferences pending;
            SourceProfile profile(epoch);
            profile.byteOrder = order;
            musx::factory::ConstructionContext construction;
            const finale_mus_reader::ImportContext context{
                index, profile, noSource, document, reference, report, pending, construction};
            finale_mus_reader::others::importFilePath(context);
            if (epoch == FormatEpoch::CodaBanner) {
                CHECK(document->getOthers()->getArray<FileDescription>(0).empty());
                CHECK(document->getOthers()->getArray<FileAlias>(0).empty());
                continue;
            }
            const auto desc = document->getOthers()->get<FileDescription>(0, 7);
            REQUIRE(desc);
            CHECK(desc->dirId == 65538);
            CHECK(desc->volRefNum == -5);
            CHECK(desc->pathType == (pathType == 1 ? FileDescription::PathType::DosPath
                                                  : FileDescription::PathType::MacAlias));
            const auto alias = document->getOthers()->get<FileAlias>(0, 7);
            REQUIRE(alias);
            CHECK(alias->length == 3);
            CHECK(alias->aliasHandle == (order == ByteOrder::BigEndian
                ? std::vector<std::uint8_t>{0x34, 0x12, 0x56}
                : std::vector<std::uint8_t>{0x12, 0x34, 0x78}));
#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
            CHECK(report.fields.at(finale_mus_reader::instanceKey<FileAlias>(0, musx::dom::Cmper(7)))
                .at("aliasHandle").origin == ValueOrigin::LegacyMusAdjusted);
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
            CHECK_FALSE(document->getOthers()->get<FileAlias>(0, 8));
            CHECK(reportedFieldCount(report) == 7);
            CHECK_FALSE(report.diagnostics.empty());
        }
    }
}

} // namespace
} // namespace finale_mus_reader_tests
