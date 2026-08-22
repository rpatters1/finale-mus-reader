// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include "import/support/legacy_mapping.h"
#include "import/support/enigma_text.h"

namespace finale_mus_reader {
namespace texts {

void importLaterTextPool(const ImportContext& context);
void importHeaderFileInfoTexts(const ImportContext& context);
void importCodaStoredTexts(const ImportContext& context);

void recordTextFieldInfo(ImportReport& report, std::string target,
    const text::ConvertedEnigmaText& converted);
void recordTextFieldInfo(ImportReport& report, std::string target,
    bool fontWasSynthesized, bool sizeWasSynthesized, bool effectsWereSynthesized);

} // namespace texts
} // namespace finale_mus_reader
