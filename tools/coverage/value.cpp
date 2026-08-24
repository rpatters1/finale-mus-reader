// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#include "coverage/value.h"

#include "coverage/json.h"

namespace finale_mus_reader {
namespace coverage {
std::string Value::toJson() const
{
    if (isNull()) return "null";
    if (isBool()) return jsonBool(asBool());
    if (isInteger()) return std::to_string(asInteger());
    if (std::holds_alternative<double>(storage_)) return std::to_string(std::get<double>(storage_));
    if (isString()) return jsonString(asString());
    std::string result;
    if (isArray()) {
        result = '[';
        bool first = true;
        for (const auto& item : asArray()) {
            if (!first) result += ',';
            first = false;
            result += item.toJson();
        }
        return result + ']';
    }
    result = '{';
    bool first = true;
    for (const auto& [key, value] : asObject()) {
        if (!first) result += ',';
        first = false;
        result += jsonString(key) + ':' + value.toJson();
    }
    return result + '}';
}

const Value* Value::find(std::string_view key) const
{
    if (!isObject()) return nullptr;
    const auto found = asObject().find(key);
    return found == asObject().end() ? nullptr : &found->second;
}

Value* Value::find(std::string_view key)
{
    if (!isObject()) return nullptr;
    const auto found = asObject().find(key);
    return found == asObject().end() ? nullptr : &found->second;
}

} // namespace coverage
} // namespace finale_mus_reader
