// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "musx/dom/Document.h"
#include "musx/xml/XmlInterface.h"

namespace finale_mus_reader {

/// @brief Parses an EnigmaXML fragment with the XML backend selected by the caller.
/// @details The reader owns document construction and only needs the caller's backend
/// to turn pinned default EnigmaXML into elements the musxdom pool factories accept.
using XmlParser = std::unique_ptr<musx::xml::IXmlDocument> (*)(const char* data, std::size_t size);

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
    /// @brief The pinned Finale 27 baseline that supplied the synthesized defaults.
    /// @details Never `Unknown`: a baseline is always selected, matching
    /// #sourcePlatform when that is known.
    SourcePlatform defaultsPlatform = SourcePlatform::MacOS;
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
    template <typename XmlDocumentType>
    [[nodiscard]] static ImportResult read(const std::filesystem::path& path)
    {
        return readWithParser(path, &parseXml<XmlDocumentType>);
    }

    template <typename XmlDocumentType>
    [[nodiscard]] static ImportResult read(const std::vector<std::uint8_t>& data)
    {
        return readWithParser(data.data(), data.size(), &parseXml<XmlDocumentType>);
    }

    template <typename XmlDocumentType>
    [[nodiscard]] static ImportResult read(const std::uint8_t* data, std::size_t size)
    {
        return readWithParser(data, size, &parseXml<XmlDocumentType>);
    }

private:
    template <typename XmlDocumentType>
    static std::unique_ptr<musx::xml::IXmlDocument> parseXml(const char* data, std::size_t size)
    {
        static_assert(std::is_base_of_v<musx::xml::IXmlDocument, XmlDocumentType>,
            "XmlDocumentType must derive from musx::xml::IXmlDocument");

        auto xmlDocument = std::make_unique<XmlDocumentType>();
        xmlDocument->loadFromBuffer(data, size);
        return xmlDocument;
    }

    static ImportResult readWithParser(const std::filesystem::path& path, XmlParser parseXml);
    static ImportResult readWithParser(
        const std::uint8_t* data, std::size_t size, XmlParser parseXml);
};

} // namespace finale_mus_reader
