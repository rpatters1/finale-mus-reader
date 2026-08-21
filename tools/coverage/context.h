// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// What every surveyor receives: one imported document, its report, and a field-origin
// lookup built once per document rather than re-scanned by each surveyor.

#pragma once

#include <string>
#include <unordered_map>

#include "finale_mus_reader/reader.h"
#include "musx/dom/Document.h"

namespace finale_mus_reader::coverage {

class FieldIndex
{
public:
    explicit FieldIndex(const ImportReport& report);

    /// @brief "legacy-mus" / "legacy-behavior" / "finale27-default" / "absent".
    const char* originOf(const std::string& target) const;
    const TextFieldInfo* textInfoOf(const std::string& target) const;

private:
    std::unordered_map<std::string, FieldInfo> byTarget_;
    std::unordered_map<std::string, TextFieldInfo> textByTarget_;
};

struct SurveyContext
{
    const musx::dom::DocumentPtr& document;
    const ImportReport& report;
    const FieldIndex& fields;
};

} // namespace finale_mus_reader::coverage
