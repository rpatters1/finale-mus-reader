#include "finale_mus_reader/reader.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "container/mus_container.h"
#include "defaults/default_document.h"
#include "records/legacy_others.h"
#include "musx/musx.h"

namespace finale_mus_reader {
namespace {

constexpr std::size_t headerSize = 0x200;
constexpr std::string_view bannerSignature = "ENIGMA BINARY FILE";

bool hasBanner(const std::uint8_t* data, std::size_t size)
{
    return size > bannerSignature.size()
        && std::memcmp(data, bannerSignature.data(), bannerSignature.size()) == 0
        && data[bannerSignature.size()] == 0;
}

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

ImportResult readImpl(const std::uint8_t* data, std::size_t size,
    const std::optional<std::filesystem::path>& sourcePath)
{
    if (!data || size == 0) {
        throw std::invalid_argument("MUS input is empty");
    }

    const auto parsed = container::parse(data, size);
    const bool bannerPresent = hasBanner(data, size);
    if (parsed.formatEpoch == FormatEpoch::Unknown && !bannerPresent) {
        throw std::invalid_argument("Input is not a recognized legacy Finale MUS file");
    }

    ImportResult result;
    result.report.formatEpoch = parsed.formatEpoch;
    result.report.byteOrder = parsed.byteOrder;
    result.report.sourceSize = size;
    if (bannerPresent && size >= headerSize) {
        result.report.banner = fixedString(data + 0x20, 0x40);
        result.report.savingProduct = savingProductFromBanner(result.report.banner);
        const auto modifiedPlatform = parsePlatform(fixedString(data + 0x09a, 4));
        const auto createdPlatform = parsePlatform(fixedString(data + 0x074, 4));
        result.report.sourcePlatform = modifiedPlatform != SourcePlatform::Unknown
            ? modifiedPlatform : createdPlatform;
    }
    for (const auto& block : parsed.blocks) {
        result.report.blocks.push_back(block.info);
    }
    if (parsed.trailingByteCount != 0) {
        result.report.warnings.push_back(
            "Preserved classification after a terminal block with "
            + std::to_string(parsed.trailingByteCount) + " trailing bytes.");
    }

    result.document = defaults::createMacOSOptionsDocument(sourcePath);
    result.document->getHeader() = recoverHeader(data, size, result.report);
    initializeSupportedFields(result.document, result.report);
    overlayLegacyOthers(parsed, result.document, result.report);

    if (parsed.formatEpoch == FormatEpoch::PreBanner) {
        result.report.warnings.emplace_back(
            "Pre-banner pool directories are unresolved; options remain at Finale 27 defaults.");
    } else if (parsed.formatEpoch == FormatEpoch::ZlibLegacy) {
        result.report.warnings.emplace_back(
            "Later variable logical records are not overlaid yet; options remain at Finale 27 defaults.");
    } else if (parsed.formatEpoch == FormatEpoch::Unknown) {
        result.report.warnings.emplace_back(
            "The banner header was recovered, but the body framing was not recognized.");
    }
    for (const auto& warning : result.report.warnings) {
        musx::util::Logger::log(musx::util::Logger::LogLevel::Warning, warning);
    }
    return result;
}

} // namespace

ImportResult Reader::read(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        throw std::runtime_error("Unable to open MUS input: " + path.string());
    }
    const auto end = input.tellg();
    if (end <= 0) {
        throw std::invalid_argument("MUS input is empty: " + path.string());
    }
    const auto unsignedSize = static_cast<std::uintmax_t>(end);
    if (unsignedSize > std::numeric_limits<std::size_t>::max()
        || unsignedSize > static_cast<std::uintmax_t>(
            std::numeric_limits<std::streamsize>::max())) {
        throw std::length_error("MUS input is too large for this platform");
    }
    std::vector<std::uint8_t> data(static_cast<std::size_t>(unsignedSize));
    input.seekg(0);
    input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!input) {
        throw std::runtime_error("Unable to read complete MUS input: " + path.string());
    }
    return readImpl(data.data(), data.size(), path);
}

ImportResult Reader::read(const std::vector<std::uint8_t>& data)
{
    return read(data.data(), data.size());
}

ImportResult Reader::read(const std::uint8_t* data, std::size_t size)
{
    return readImpl(data, size, std::nullopt);
}

} // namespace finale_mus_reader
