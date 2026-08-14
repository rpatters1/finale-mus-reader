// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/support/legacy_mapping.h"

namespace finale_mus_reader {
namespace others {

// One importer per musxdom others class, and the registry calls nothing else. The tables a
// class needs, and which epoch each of them covers, stay inside that class's own
// translation unit. Accessors are used instead of static registration so a static archive
// cannot discard an importer nothing else references.

/// @brief Recovers the others::FontDefinition pool, whose four layouts span every epoch.
void importFontDefinitions(const ImportContext& context);

/// @brief Recovers the four others::LayerAttributes objects.
void importLayerAttributes(const ImportContext& context);

/// @brief Recovers page graphic assignments and their embedded-graphic references.
void importPageGraphicAssignments(const ImportContext& context);

/// @brief Recovers graphic assignments referenced by ShapeDef instructions.
void importShapeGraphicAssignments(const ImportContext& context);

/// @brief Recovers ShapeDef objects and their owned instruction and data lists.
void importShapeDefinitions(const ImportContext& context);

} // namespace others
} // namespace finale_mus_reader
