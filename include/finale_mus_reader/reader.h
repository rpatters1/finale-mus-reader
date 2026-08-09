// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
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

/// @brief A Finale version recorded in the file header.
/// @details The header stores each version as a 32-bit value packed one byte per
/// component: major, minor, maint, build. Finale 2002 records `0x07010401` as its
/// application version, which Finale 27 renders as 7.1.4.1 when it converts a legacy
/// file. Major versions run 0-27 across Finale's history.
///
/// The runtime version Finale reports to plug-ins packs the same components
/// differently, placing the minor version in bits 23-20. Do not compare a value from
/// this struct against a runtime version constant.
struct SourceVersion
{
    std::uint32_t raw{};
    std::uint8_t major{};
    std::uint8_t minor{};
    std::uint8_t maint{};
    std::uint8_t build{};
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
    /// @brief The Enigma version recorded by the last application to save the file.
    /// @details Absent when the file has no banner, or when the recovered major version
    /// falls outside Finale's 0-27 range, which means the header layout was not what was
    /// expected. Mapping rows that are gated to a version range apply only when this is
    /// present.
    std::optional<SourceVersion> sourceVersion;
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
