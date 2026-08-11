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
#include "musx/factory/DocumentFactory.h"
#include "musx/xml/XmlInterface.h"

namespace finale_mus_reader {

/// @brief Parses an EnigmaXML fragment with the XML backend selected by the caller.
/// @details The reader owns document construction and only needs the caller's backend
/// to turn pinned default EnigmaXML into elements the musxdom pool factories accept.
using XmlParser = std::unique_ptr<musx::xml::IXmlDocument> (*)(const char* data, std::size_t size);

/// @brief Parses a complete EnigmaXML document with the caller's selected XML backend.
/// @details The pinned baseline uses this to retain a fully formed reference document
/// alongside the filtered pools copied into the import target.
using DocumentParser = musx::dom::DocumentPtr (*)(const char* data, std::size_t size);

enum class FormatEpoch
{
    Unknown,
    /// @brief The era before the `ENIGMA BINARY FILE` signature existed.
    /// @details These files are not headerless. They open with a plain-text product
    /// banner of the form `Finale(TM) 2.6 Copyright 1987 by Coda.`, which is where their
    /// version lives, and they still reserve a 0x200 header. Later eras use `Finale(R)`
    /// and name Coda too, so the distinguishing marks are the `(TM)` spelling and the
    /// absent signature.
    CodaBanner,
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

/// @brief A Finale version recorded in the file.
/// @details Signature-era files store each version as a 32-bit value in the file's own
/// byte order, packed as major in bits 31-24, minor in bits 23-20, maintenance in bits
/// 19-16, a development-status code in bits 15-8, and build in bits 7-0. Finale 97
/// records an application version of `0x03820401`, which is 3.8.2 build 1, matching what
/// Finale 27 reports as the creator version when it converts such a file.
///
/// This is the same packing the running application reports to plug-ins, so the two are
/// directly comparable.
///
/// Major versions run 0-27 across Finale's history and do not order it by themselves:
/// Finale 3.2 through 3.7 and Finale 97 all carry major 3, separated only by minor.
struct SourceVersion
{
    /// @brief The 32-bit value in logical order, or zero when the version came from a
    /// @ref FormatEpoch::CodaBanner product string rather than a header tuple.
    std::uint32_t raw{};
    std::uint8_t major{};
    std::uint8_t minor{};
    std::uint8_t maint{};
    /// @brief Development-status code. Its values are not yet mapped to musxdom's names.
    std::uint8_t devStatus{};
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
        return readWithParser(
            path, &parseXml<XmlDocumentType>, &parseDocument<XmlDocumentType>);
    }

    template <typename XmlDocumentType>
    [[nodiscard]] static ImportResult read(const std::vector<std::uint8_t>& data)
    {
        return readWithParser(data.data(), data.size(),
            &parseXml<XmlDocumentType>, &parseDocument<XmlDocumentType>);
    }

    template <typename XmlDocumentType>
    [[nodiscard]] static ImportResult read(const std::uint8_t* data, std::size_t size)
    {
        return readWithParser(data, size,
            &parseXml<XmlDocumentType>, &parseDocument<XmlDocumentType>);
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

    template <typename XmlDocumentType>
    static musx::dom::DocumentPtr parseDocument(const char* data, std::size_t size)
    {
        return musx::factory::DocumentFactory::create<XmlDocumentType>(data, size);
    }

    static ImportResult readWithParser(const std::filesystem::path& path,
        XmlParser parseXml, DocumentParser parseDocument);
    static ImportResult readWithParser(
        const std::uint8_t* data, std::size_t size,
        XmlParser parseXml, DocumentParser parseDocument);
};

} // namespace finale_mus_reader
