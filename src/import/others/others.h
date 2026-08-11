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
[[nodiscard]] const MappingTable& classFontDefinitionsTable();

} // namespace others
} // namespace finale_mus_reader
