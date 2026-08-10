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
#include "import/legacy_mapping.h"
#include "records/legacy_record_index.h"
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

// Finale's complete history spans major versions 0 through 27. A major outside that
// range means the tuple was not read the way the file wrote it.
constexpr std::uint8_t maximumMajorVersion = 27;

// A header version is a 32-bit value in the file's own byte order, packed as major in
// bits 31-24, minor in 23-20, maintenance in 19-16, a development-status code in 15-8,
// and build in 7-0. Finale 97 records application version 0x03820401, which is 3.8.2
// build 1, exactly what Finale 27 reports as the creator version for such a file.
SourceVersion decodeVersionValue(std::uint32_t value)
{
    SourceVersion version;
    version.raw = value;
    version.major = static_cast<std::uint8_t>(value >> 24U);
    version.minor = static_cast<std::uint8_t>((value >> 20U) & 0x0fU);
    version.maint = static_cast<std::uint8_t>((value >> 16U) & 0x0fU);
    version.devStatus = static_cast<std::uint8_t>((value >> 8U) & 0xffU);
    version.build = static_cast<std::uint8_t>(value & 0xffU);
    return version;
}

SourceVersion decodeVersion(const std::uint8_t* raw, ByteOrder byteOrder)
{
    const auto bigEndian = (static_cast<std::uint32_t>(raw[0]) << 24U)
        | (static_cast<std::uint32_t>(raw[1]) << 16U)
        | (static_cast<std::uint32_t>(raw[2]) << 8U)
        | raw[3];
    const auto littleEndian = (static_cast<std::uint32_t>(raw[3]) << 24U)
        | (static_cast<std::uint32_t>(raw[2]) << 16U)
        | (static_cast<std::uint32_t>(raw[1]) << 8U)
        | raw[0];

    if (byteOrder == ByteOrder::LittleEndian) {
        return decodeVersionValue(littleEndian);
    }
    if (byteOrder == ByteOrder::BigEndian) {
        return decodeVersionValue(bigEndian);
    }
    // With no classified byte order, prefer the reading whose major version is possible.
    const auto candidate = decodeVersionValue(bigEndian);
    if (candidate.major <= maximumMajorVersion) {
        return candidate;
    }
    return decodeVersionValue(littleEndian);
}

void populateVersion(
    const std::uint8_t* raw, ByteOrder byteOrder, musx::dom::header::FinaleVersion& version)
{
    const auto decoded = decodeVersion(raw, byteOrder);
    version.major = decoded.major;
    version.minor = decoded.minor;
    // Finale omits these from EnigmaXML when they are zero, so match that convention.
    // The development-status code has no mapping to musxdom's names yet, so it is
    // reported through ImportReport rather than guessed at here.
    if (decoded.maint != 0) {
        version.maint = decoded.maint;
    }
    if (decoded.build != 0) {
        version.build = decoded.build;
    }
}

musx::dom::header::FileInfo parseFileInfo(const std::uint8_t* data,
    std::size_t dateOffset, std::size_t tupleOffset, ByteOrder byteOrder)
{
    musx::dom::header::FileInfo info;
    const int month = data[dateOffset + 1];
    const int day = data[dateOffset + 2];
    if (month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        info.year = 1900 + data[dateOffset];
        info.month = month;
        info.day = day;
    }

    populateVersion(data + tupleOffset, byteOrder, info.finaleVersion);
    info.application = fixedString(data + tupleOffset + 4, 4);
    const auto platform = parsePlatform(fixedString(data + tupleOffset + 8, 4));
    info.platform = toDomPlatform(platform);
    populateVersion(data + tupleOffset + 12, byteOrder, info.appVersion);
    populateVersion(data + tupleOffset + 16, byteOrder, info.fileVersion);
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
        header->created = parseFileInfo(data, 0x066, 0x06c, report.byteOrder);
        header->modified = parseFileInfo(data, 0x08c, 0x092, report.byteOrder);
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

// A Coda-banner file carries no version tuple: its whole 0x60-0x200 header region is
// zero apart from a constant pair at 0x80. The version is the number in the product
// banner itself, as in `Finale(TM) 2.6 Copyright 1987 by Coda.`, so that text is the
// only place it can be recovered from.
bool describeCodaBannerIdentity(
    const std::uint8_t* data, std::size_t size, ImportReport& report)
{
    constexpr std::string_view prefix = "Finale(TM) ";
    if (size <= prefix.size()
        || std::memcmp(data, prefix.data(), prefix.size()) != 0) {
        return false;
    }

    report.banner = fixedString(data, (std::min)(size, headerSize));
    const auto product = report.banner.substr(prefix.size());
    const auto productEnd = product.find(" Copyright");
    report.savingProduct = product.substr(0, productEnd);

    SourceVersion version;
    std::size_t consumed = 0;
    std::uint8_t* const components[] = {&version.major, &version.minor, &version.maint};
    for (auto* component : components) {
        std::size_t digits = 0;
        unsigned value = 0;
        while (consumed + digits < report.savingProduct.size()
            && std::isdigit(static_cast<unsigned char>(report.savingProduct[consumed + digits]))) {
            value = value * 10 + static_cast<unsigned>(report.savingProduct[consumed + digits] - '0');
            ++digits;
        }
        if (digits == 0 || value > (std::numeric_limits<std::uint8_t>::max)()) {
            break;
        }
        *component = static_cast<std::uint8_t>(value);
        consumed += digits;
        if (consumed >= report.savingProduct.size() || report.savingProduct[consumed] != '.') {
            break;
        }
        ++consumed;
    }
    if (version.major != 0 && version.major <= maximumMajorVersion) {
        report.sourceVersion = version;
    }
    return true;
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
    if (describeCodaBannerIdentity(data, size, report)) {
        return;
    }
    if (!hasBanner(data, size) || size < headerSize) {
        return;
    }
    report.banner = fixedString(data + 0x20, 0x40);
    report.savingProduct = savingProductFromBanner(report.banner);
    const auto modifiedPlatform = parsePlatform(fixedString(data + 0x09a, 4));
    const auto createdPlatform = parsePlatform(fixedString(data + 0x074, 4));
    report.sourcePlatform = modifiedPlatform != SourcePlatform::Unknown
        ? modifiedPlatform : createdPlatform;

    // Prefer the version of the application that last saved the file, since that is what
    // determined the layout on disk.
    const auto modified = decodeVersion(data + 0x092, report.byteOrder);
    const auto created = decodeVersion(data + 0x06c, report.byteOrder);
    const auto& selected = modified.major != 0 ? modified : created;
    if (selected.major <= maximumMajorVersion) {
        report.sourceVersion = selected;
    } else {
        report.warnings.push_back("Recovered Finale major version "
            + std::to_string(selected.major) + " is outside the valid range 0-"
            + std::to_string(maximumMajorVersion)
            + "; version-gated mappings are skipped.");
    }
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

    mapping::SourceProfile profile;
    profile.epoch = report.formatEpoch;
    profile.version = report.sourceVersion;
    profile.byteOrder = report.byteOrder;
    profile.platform = report.sourcePlatform;
    mapping::applyLegacyMappings(
        records::LegacyRecordIndex::build(parsed), profile, document, report);

    // Finishing validates the pools and runs musxdom's resolvers once, after every
    // legacy overlay has been applied.
    return std::move(session).finish();
}

} // namespace finale_mus_reader
