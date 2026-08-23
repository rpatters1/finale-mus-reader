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

#if defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)
void recordTextFieldInfo(ImportReport& report, const InstanceKey& instance, std::string member,
    const text::ConvertedEnigmaText& converted);
void recordTextFieldInfo(ImportReport& report, const InstanceKey& instance, std::string member,
    bool fontWasSynthesized, bool sizeWasSynthesized, bool effectsWereSynthesized);
#else
#define recordTextFieldInfo(...) ((void)0)
#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)

} // namespace texts
} // namespace finale_mus_reader
