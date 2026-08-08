#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace musx {
namespace dom {
class Document;
} // namespace dom
} // namespace musx

namespace finale_mus_reader {

enum class FormatEpoch
{
    Unknown,
    PreBanner,
    UncompressedLegacy,
    DclLegacy,
    ZlibLegacy
};

enum class ByteOrder
{
    Unknown,
    LittleEndian,
    BigEndian
};

enum class SourcePlatform
{
    Unknown,
    MacOS,
    Windows
};

enum class ValueOrigin
{
    LegacyMus,
    Finale27Default
};

struct BlockInfo
{
    std::uint16_t type{};
    std::size_t sourceOffset{};
    std::size_t storedSize{};
    std::size_t decodedSize{};
    bool checksumPresent{};
    bool checksumValid{};
};

struct FieldInfo
{
    std::string target;
    ValueOrigin origin = ValueOrigin::Finale27Default;
    std::size_t blockOffset{};
    std::size_t decodedOffset{};
    std::int64_t rawValue{};
};

struct ImportReport
{
    FormatEpoch formatEpoch = FormatEpoch::Unknown;
    ByteOrder byteOrder = ByteOrder::Unknown;
    SourcePlatform sourcePlatform = SourcePlatform::Unknown;
    std::size_t sourceSize{};
    std::string banner;
    std::string savingProduct;
    std::vector<BlockInfo> blocks;
    std::vector<FieldInfo> fields;
    std::vector<std::string> warnings;
};

struct ImportResult
{
    std::shared_ptr<musx::dom::Document> document;
    ImportReport report;
};

class Reader
{
public:
    [[nodiscard]] static ImportResult read(const std::filesystem::path& path);
    [[nodiscard]] static ImportResult read(const std::vector<std::uint8_t>& data);
    [[nodiscard]] static ImportResult read(const std::uint8_t* data, std::size_t size);
};

} // namespace finale_mus_reader
