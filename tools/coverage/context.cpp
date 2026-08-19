// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/context.h"

#include "coverage/json.h"

namespace finale_mus_reader::coverage {

FieldIndex::FieldIndex(const ImportReport& report)
{
    for (const auto& info : report.fields) {
        byTarget_.emplace(info.target, info);
    }
}

const char* FieldIndex::originOf(const std::string& target) const
{
    const auto found = byTarget_.find(target);
    return found == byTarget_.end() ? "absent" : originName(found->second.origin);
}

} // namespace finale_mus_reader::coverage
