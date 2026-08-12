// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/legacy_mapping.h"

namespace finale_mus_reader {
namespace others {

// Concrete tables are grouped by the musxdom pool they populate. Accessors are used
// instead of static registration so a static archive cannot discard an otherwise
// unreferenced table object.
[[nodiscard]] const MappingTable& layerAttributesTable();
[[nodiscard]] const MappingTable& fontDefinitionsTable();
[[nodiscard]] const MappingTable& earlyFontDefinitionsTable();

/// @brief Font definitions for the Coda-banner epoch, which is entirely below the header
/// boundary and therefore needs no version test. Its Windows documents have no version to test.
[[nodiscard]] const MappingTable& codaFontDefinitionsTable();
[[nodiscard]] const MappingTable& classFontDefinitionsTable();

} // namespace others
} // namespace finale_mus_reader
