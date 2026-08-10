// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

#include "finale_mus_reader/reader.h"
#include "musx/factory/PoolFactory.h"
#include "musx/xml/XmlInterface.h"

namespace finale_mus_reader {
namespace defaults {

/// @brief The pinned Finale 27 baseline, parsed with the XML backend of the caller.
/// @details The elements are owned by @ref xmlDocument and are only valid while it lives.
struct ParsedDefaultDocument
{
    /// @brief The baseline that was actually selected.
    SourcePlatform platform = SourcePlatform::MacOS;
    /// @brief Owns the parsed EnigmaXML that every element below points into.
    std::unique_ptr<musx::xml::IXmlDocument> xmlDocument;
    /// @brief The complete `<options>` element of the baseline.
    musx::xml::XmlElementPtr options;
    /// @brief The complete `<others>` element of the baseline.
    /// @details Seed a pool from this only through @ref optionLikeOthers. The baseline
    /// `<others>` is a whole score, and everything outside that allowlist is fallback
    /// content that must never reach an imported document.
    musx::xml::XmlElementPtr others;
    /// @brief Accepts the option-like children of @ref others, currently `<layerAtts>`.
    musx::factory::NodeFilter optionLikeOthers;
};

/// @brief Inflates and parses a pinned Finale 27 baseline.
/// @param parseXml The caller's XML backend.
/// @param platform Selects the baseline. `Windows` selects the Windows baseline; every
/// other value, including `Unknown`, selects macOS. @ref ParsedDefaultDocument::platform
/// reports what was actually selected.
/// @throws std::runtime_error if the embedded resource fails validation or lacks the
/// elements the reader seeds from.
[[nodiscard]] ParsedDefaultDocument parseDefault(XmlParser parseXml, SourcePlatform platform);

} // namespace defaults
} // namespace finale_mus_reader
