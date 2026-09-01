// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

// What every surveyor receives: one imported document and its import report.

#pragma once

#include <stdexcept>

#include "finale_mus_reader/reader.h"
#include "musx/dom/Document.h"

namespace finale_mus_reader {
namespace coverage {

/// @brief A coverage invariant whose failure must terminate the probe.
class ProbeInvariantError : public std::logic_error
{
public:
    using std::logic_error::logic_error;
};

struct SurveyContext
{
    const musx::dom::DocumentPtr& document;
    ImportReport& report;
};

} // namespace coverage
} // namespace finale_mus_reader
