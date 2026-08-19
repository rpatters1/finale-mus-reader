// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace finale_mus_reader::coverage {

/// @brief The pieces of a `.musx` archive `musx::factory::DocumentFactory::create` needs to
/// reconstruct the same document Finale itself would show, not just the bare EnigmaXML.
/// @details `enigmaXml` alone resolves every font, option, and entry, but two things live
/// beside it in the archive rather than inside it: `notationMetadata` (the archive's
/// `NotationMetadata.xml` member, read for `<fileInfo><scoreDuration>` when the creator names
/// Finale) and `embeddedGraphics` (every member under `graphics/`, keyed by filename exactly
/// as `DocumentFactory::CreateOptions` expects -- `<cmper>.<extension>`). Comparing a
/// companion this reader recovered a `MeasureGraphicAssign`/`PageGraphicAssign` reference for
/// against one built from `enigmaXml` alone would report every embedded graphic as
/// companion-only, since musxdom cannot resolve a reference to a blob it was never given.
struct CompanionArchive
{
    std::vector<std::uint8_t> enigmaXml;
    std::optional<std::vector<char>> notationMetadata;
    std::vector<std::pair<std::string, std::vector<std::uint8_t>>> embeddedGraphics;
};

/// @brief Reads every piece of a Finale-written `.musx` companion that
/// `musx::factory::DocumentFactory::create` accepts, for direct comparison against what this
/// reader recovered from the legacy source it was saved from.
/// @details A `.musx` is a real ZIP archive. Its EnigmaXML lives in one member, `score.dat`,
/// itself gzip-compressed and then scrambled with a rolling XOR keystream; decoding it is
/// `score_encoder.h`'s job (copied verbatim from the sibling denigma project, the algorithm's
/// original home, rather than re-derived here). The other two pieces this returns are stored
/// as ordinary, unscrambled ZIP members. musxdom's own `DocumentFactory` has no container
/// awareness at all and wants exactly these bytes; nothing else in this codebase currently
/// extracts them, since finale_mus_reader's own public API never touches a `.musx` file.
/// @throws std::runtime_error naming which stage failed (open, ZIP structure, `score.dat`
/// missing, inflate) -- this reads a file this project did not write, and a malformed or
/// truncated companion should fail with a specific reason rather than crash or hang.
CompanionArchive readCompanionArchive(const std::filesystem::path& musxPath);

} // namespace finale_mus_reader::coverage
