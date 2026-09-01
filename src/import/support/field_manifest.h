// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "musx/musx.h"

// Field manifests shared by an importer and the instrumentation that exhaustively
// observes its musxdom target. This is a private reader interface, not public API.

namespace finale_mus_reader {
namespace options {

using MusicSymbolOptionsTarget = musx::dom::options::MusicSymbolOptions;
using MusicSymbolFontType = musx::dom::options::FontOptions::FontType;

struct NarrowMusicSymbolSource
{
    std::uint16_t selector;
    std::size_t word;
};

enum class NarrowMusicSymbolEra
{
    Any,
    AfterCoda,
    Finale35AndLater,
    Finale351AndLater,
    Finale97AndLater,
    ZlibOnly,
};

enum class SharedMusicSymbolEra
{
    None,
    CodaOnly,
    PreZlib,
};

struct MusicSymbolOptionsField
{
    std::string_view memberName;
    std::string_view leafName;
    char32_t MusicSymbolOptionsTarget::*member;
    MusicSymbolFontType fontType;
    std::optional<char32_t MusicSymbolOptionsTarget::*> sharedSource;
    SharedMusicSymbolEra sharedEra = SharedMusicSymbolEra::None;
    NarrowMusicSymbolSource narrowSource;
    NarrowMusicSymbolEra narrowEra = NarrowMusicSymbolEra::Any;
};

std::span<const MusicSymbolOptionsField> musicSymbolOptionsFields();

} // namespace options
} // namespace finale_mus_reader

#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
