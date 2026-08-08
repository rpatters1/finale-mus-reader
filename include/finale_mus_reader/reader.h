// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// DocumentFactory's transitive DOM headers use these constants without
// including their defining header.
#include "music_theory/music_theory.hpp"
#include "musx/factory/DocumentFactory.h"
#include "musx/xml/XmlInterface.h"

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
    template <typename XmlDocumentType>
    [[nodiscard]] static ImportResult read(const std::filesystem::path& path)
    {
        return readWithCreator(path, &createDocument<XmlDocumentType>);
    }

    template <typename XmlDocumentType>
    [[nodiscard]] static ImportResult read(const std::vector<std::uint8_t>& data)
    {
        return readWithCreator(data.data(), data.size(), &createDocument<XmlDocumentType>);
    }

    template <typename XmlDocumentType>
    [[nodiscard]] static ImportResult read(const std::uint8_t* data, std::size_t size)
    {
        return readWithCreator(data, size, &createDocument<XmlDocumentType>);
    }

private:
    using DocumentCreator = std::shared_ptr<musx::dom::Document> (*)(
        std::string_view, const std::optional<std::filesystem::path>&);

    template <typename XmlDocumentType>
    static std::shared_ptr<musx::dom::Document> createDocument(
        std::string_view xml, const std::optional<std::filesystem::path>& sourcePath)
    {
        static_assert(std::is_base_of_v<musx::xml::IXmlDocument, XmlDocumentType>,
            "XmlDocumentType must derive from musx::xml::IXmlDocument");

        if (sourcePath) {
            musx::factory::DocumentFactory::CreateOptions createOptions(
                *sourcePath, {}, {});
            return musx::factory::DocumentFactory::create<XmlDocumentType>(
                xml.data(), xml.size(), std::move(createOptions));
        }
        return musx::factory::DocumentFactory::create<XmlDocumentType>(
            xml.data(), xml.size());
    }

    static ImportResult readWithCreator(
        const std::filesystem::path& path, DocumentCreator documentCreator);
    static ImportResult readWithCreator(
        const std::uint8_t* data, std::size_t size, DocumentCreator documentCreator);
};

} // namespace finale_mus_reader
