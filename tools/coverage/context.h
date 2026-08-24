// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// What every surveyor receives: one imported document and its import report.

#pragma once

#include "finale_mus_reader/reader.h"
#include "musx/dom/Document.h"

namespace finale_mus_reader {
namespace coverage {

struct SurveyContext
{
    const musx::dom::DocumentPtr& document;
    const ImportReport& report;
};

} // namespace coverage
} // namespace finale_mus_reader
