// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "import/document_factory.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "defaults/default_document.h"
#include "records/legacy_others.h"
#include "musx/factory/DocumentFactory.h"
#include "musx/factory/PoolFactory.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace {

constexpr std::size_t headerSize = 0x200;
constexpr std::string_view bannerSignature = "ENIGMA BINARY FILE";

std::string fixedString(const std::uint8_t* data, std::size_t size)
{
    const auto* end = std::find(data, data + size, std::uint8_t{0});
    return std::string(reinterpret_cast<const char*>(data),
        static_cast<std::size_t>(end - data));
}

std::string savingProductFromBanner(const std::string& banner)
{
    constexpr std::string_view prefix = "Finale(R) ";
    const auto begin = banner.find(prefix);
    if (begin == std::string::npos) {
        return {};
    }
    const auto productBegin = begin + prefix.size();
    auto productEnd = banner.find(" Copyright", productBegin);
    if (productEnd == std::string::npos) {
        productEnd = banner.find(" File Converter", productBegin);
    }
    if (productEnd == std::string::npos) {
        productEnd = banner.size();
    }
    return banner.substr(productBegin, productEnd - productBegin);
}

SourcePlatform parsePlatform(const std::string& platform)
{
    if (platform == "MAC") {
        return SourcePlatform::MacOS;
    }
    if (platform == "WIN") {
        return SourcePlatform::Windows;
    }
    return SourcePlatform::Unknown;
}

musx::dom::header::Platform toDomPlatform(SourcePlatform platform)
{
    switch (platform) {
    case SourcePlatform::MacOS:
        return musx::dom::header::Platform::Mac;
    case SourcePlatform::Windows:
        return musx::dom::header::Platform::Windows;
    case SourcePlatform::Unknown:
        return musx::dom::header::Platform::Other;
    }
    return musx::dom::header::Platform::Other;
}

void populateVersion(const std::uint8_t* raw, musx::dom::header::FinaleVersion& version)
{
    // The first two bytes are consistently the internal major/minor pair. The
    // remaining tuple packing is still open, so do not label it maint/build.
    version.major = raw[0];
    version.minor = raw[1];
}

musx::dom::header::FileInfo parseFileInfo(
    const std::uint8_t* data, std::size_t dateOffset, std::size_t tupleOffset)
{
    musx::dom::header::FileInfo info;
    const int month = data[dateOffset + 1];
    const int day = data[dateOffset + 2];
    if (month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        info.year = 1900 + data[dateOffset];
        info.month = month;
        info.day = day;
    }

    populateVersion(data + tupleOffset, info.finaleVersion);
    info.application = fixedString(data + tupleOffset + 4, 4);
    const auto platform = parsePlatform(fixedString(data + tupleOffset + 8, 4));
    info.platform = toDomPlatform(platform);
    populateVersion(data + tupleOffset + 12, info.appVersion);
    populateVersion(data + tupleOffset + 16, info.fileVersion);
    return info;
}

musx::dom::header::HeaderPtr recoverHeader(
    const std::uint8_t* data, std::size_t size, const ImportReport& report)
{
    auto header = std::make_shared<musx::dom::header::Header>();
    if (report.byteOrder == ByteOrder::BigEndian) {
        header->wordOrder = musx::dom::header::WordOrder::BigEndian;
    } else {
        header->wordOrder = musx::dom::header::WordOrder::LittleEndian;
    }
    if (report.sourcePlatform == SourcePlatform::MacOS) {
        header->textEncoding = musx::dom::header::TextEncoding::Mac;
    } else if (report.sourcePlatform == SourcePlatform::Windows) {
        header->textEncoding = musx::dom::header::TextEncoding::Windows;
    }

    if (hasBanner(data, size) && size >= 0x0a6) {
        header->created = parseFileInfo(data, 0x066, 0x06c);
        header->modified = parseFileInfo(data, 0x08c, 0x092);
    }
    return header;
}

// Seeds the structurally complete Finale 27 options pool and the four option-like
// layer attributes. No other pinned content may reach the imported document.
//
// The seeded options reference font and shape records by cmper, and those records
// are deliberately not seeded: a document has one id space per record type, so a
// pinned definition would later collide with the source record sharing its cmper.
// Those references therefore do not resolve, and font lookups throw until the
// definitions are decoded from the MUS file. See research/PRODUCTION_READINESS.md.
void seedPinnedDefaults(
    const musx::dom::DocumentPtr& document, XmlParser parseXml, ImportReport& report)
{
    // The parsed baseline owns both elements, so it must outlive the pool creation below.
    const auto pinned = defaults::parseDefault(parseXml, report.sourcePlatform);
    report.defaultsPlatform = pinned.platform;
    document->getOptions() = musx::factory::OptionsFactory::create(pinned.options, document);
    document->getOthers() = musx::factory::OthersFactory::create(
        pinned.others, document, pinned.optionLikeOthers);
}

FieldInfo makeDefaultField(const std::string& target, std::int64_t value)
{
    FieldInfo result;
    result.target = target;
    result.rawValue = value;
    return result;
}

void initializeSupportedFields(const musx::dom::DocumentPtr& document, ImportReport& report)
{
    for (musx::dom::Cmper layerId = 0; layerId < 4; ++layerId) {
        const auto layer = document->getOthers()->get<musx::dom::others::LayerAttributes>(
            musx::dom::SCORE_PARTID, layerId);
        if (!layer) {
            throw std::runtime_error("Pinned default is missing a layer attribute");
        }
        report.fields.push_back(makeDefaultField(
            "others.layerAtts[" + std::to_string(layerId) + "].restOffset",
            layer->restOffset));
    }

    const auto spacing = document->getOptions()->get<musx::dom::options::MusicSpacingOptions>();
    if (!spacing) {
        throw std::runtime_error("Pinned default is missing music spacing options");
    }
    report.fields.push_back(makeDefaultField("options.musicSpacing.minWidth", spacing->minWidth));
    report.fields.push_back(makeDefaultField("options.musicSpacing.maxWidth", spacing->maxWidth));
    report.fields.push_back(makeDefaultField("options.musicSpacing.minDistance", spacing->minDistance));
    report.fields.push_back(makeDefaultField(
        "options.musicSpacing.minDistTiedNotes", spacing->minDistTiedNotes));
}

void markRecovered(ImportReport& report, const std::string& target,
    const records::LegacyOther& record, std::int64_t value)
{
    const auto found = std::find_if(report.fields.begin(), report.fields.end(),
        [&](const FieldInfo& field) { return field.target == target; });
    if (found == report.fields.end()) {
        throw std::logic_error("Recovered field was not registered as a supported overlay");
    }
    found->origin = ValueOrigin::LegacyMus;
    found->blockOffset = record.blockOffset;
    found->decodedOffset = record.decodedOffset;
    found->rawValue = value;
}

void overlayLegacyOthers(const container::ParsedContainer& parsed,
    const musx::dom::DocumentPtr& document, ImportReport& report)
{
    const auto others = records::decodeLegacyOthers(parsed);
    auto spacingInstance = document->getOptions()->get<musx::dom::options::MusicSpacingOptions>();
    if (!spacingInstance) {
        throw std::runtime_error("Pinned default is missing music spacing options");
    }
    auto* spacing = const_cast<musx::dom::options::MusicSpacingOptions*>(spacingInstance.get());

    for (const auto& record : others) {
        if (record.tag == "LA" && record.incident == 0 && record.cmper < 4) {
            auto layerInstance = document->getOthers()->get<musx::dom::others::LayerAttributes>(
                musx::dom::SCORE_PARTID, record.cmper);
            if (!layerInstance) {
                throw std::runtime_error("Pinned default is missing a layer attribute");
            }
            const auto value = records::readPayloadWord(record, 0, parsed.byteOrder);
            auto* layer = const_cast<musx::dom::others::LayerAttributes*>(layerInstance.get());
            layer->restOffset = value;
            markRecovered(report,
                "others.layerAtts[" + std::to_string(record.cmper) + "].restOffset",
                record, value);
            continue;
        }

        if (record.tag == "94" && record.cmper == 0xfffe
            && record.incident == 0) {
            const auto minWidth = records::readPayloadWord(record, 1, parsed.byteOrder);
            const auto maxWidth = records::readPayloadWord(record, 2, parsed.byteOrder);
            const auto minDistance = records::readPayloadWord(record, 3, parsed.byteOrder);
            const auto minDistTiedNotes = records::readPayloadWord(record, 4, parsed.byteOrder);
            spacing->minWidth = minWidth;
            spacing->maxWidth = maxWidth;
            spacing->minDistance = minDistance;
            spacing->minDistTiedNotes = minDistTiedNotes;
            markRecovered(report, "options.musicSpacing.minWidth", record, minWidth);
            markRecovered(report, "options.musicSpacing.maxWidth", record, maxWidth);
            markRecovered(report, "options.musicSpacing.minDistance", record, minDistance);
            markRecovered(report, "options.musicSpacing.minDistTiedNotes", record,
                minDistTiedNotes);
        }
    }
}

} // namespace

bool hasBanner(const std::uint8_t* data, std::size_t size)
{
    return size > bannerSignature.size()
        && std::memcmp(data, bannerSignature.data(), bannerSignature.size()) == 0
        && data[bannerSignature.size()] == 0;
}

void describeSourceIdentity(const std::uint8_t* data, std::size_t size, ImportReport& report)
{
    if (!hasBanner(data, size) || size < headerSize) {
        return;
    }
    report.banner = fixedString(data + 0x20, 0x40);
    report.savingProduct = savingProductFromBanner(report.banner);
    const auto modifiedPlatform = parsePlatform(fixedString(data + 0x09a, 4));
    const auto createdPlatform = parsePlatform(fixedString(data + 0x074, 4));
    report.sourcePlatform = modifiedPlatform != SourcePlatform::Unknown
        ? modifiedPlatform : createdPlatform;
}

musx::dom::DocumentPtr createDocument(
    const container::ParsedContainer& parsed,
    const std::uint8_t* data,
    std::size_t size,
    const std::optional<std::filesystem::path>& sourcePath,
    XmlParser parseXml,
    ImportReport& report)
{
    musx::factory::DocumentFactory::ConstructionOptions constructionOptions;
    constructionOptions.sourcePath = sourcePath;
    auto session = musx::factory::DocumentFactory::begin(std::move(constructionOptions));
    const auto& document = session.getDocument();

    seedPinnedDefaults(document, parseXml, report);
    document->getHeader() = recoverHeader(data, size, report);
    initializeSupportedFields(document, report);
    overlayLegacyOthers(parsed, document, report);

    // Finishing validates the pools and runs musxdom's resolvers once, after every
    // legacy overlay has been applied.
    return std::move(session).finish();
}

} // namespace finale_mus_reader
