// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/legacy_mapping.h"

namespace finale_mus_reader {
namespace mapping {

// Mapping tables are reached through accessors rather than registering themselves from
// static initializers. This library is a static archive, and a linker is free to drop an
// object file whose only contribution is a static initializer.
//
// Each table corresponds to one musxdom class. Add a new table by writing its file,
// declaring its accessor here, and listing it in registeredTables() in
// src/import/legacy_mapping.cpp.

[[nodiscard]] const MappingTable& musicSpacingOptionsTable();
[[nodiscard]] const MappingTable& layerAttributesTable();
[[nodiscard]] const MappingTable& fontDefinitionsTable();
[[nodiscard]] const MappingTable& earlyFontDefinitionsTable();
[[nodiscard]] const MappingTable& classFontDefinitionsTable();

} // namespace mapping
} // namespace finale_mus_reader
