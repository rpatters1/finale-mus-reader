// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace finale_mus_reader {
namespace coverage {

class Value
{
public:
    using Array = std::vector<Value>;
    using Object = std::map<std::string, Value, std::less<>>;
    using Storage = std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, Array, Object>;

    Value() : storage_(nullptr) {}
    Value(bool value) : storage_(value) {}
    template <std::integral T> requires (!std::same_as<T, bool>)
    Value(T value) : storage_(static_cast<std::int64_t>(value)) {}
    template <std::floating_point T>
    Value(T value) : storage_(static_cast<double>(value)) {}
    Value(std::string value) : storage_(std::move(value)) {}
    Value(const char* value) : storage_(std::string(value)) {}
    Value(Array value) : storage_(std::move(value)) {}
    Value(Object value) : storage_(std::move(value)) {}

    [[nodiscard]] std::string toJson() const;

    [[nodiscard]] bool isNull() const { return std::holds_alternative<std::nullptr_t>(storage_); }
    [[nodiscard]] bool isBool() const { return std::holds_alternative<bool>(storage_); }
    [[nodiscard]] bool isInteger() const { return std::holds_alternative<std::int64_t>(storage_); }
    [[nodiscard]] bool isDouble() const { return std::holds_alternative<double>(storage_); }
    [[nodiscard]] bool isNumber() const { return isInteger() || std::holds_alternative<double>(storage_); }
    [[nodiscard]] bool isString() const { return std::holds_alternative<std::string>(storage_); }
    [[nodiscard]] bool isArray() const { return std::holds_alternative<Array>(storage_); }
    [[nodiscard]] bool isObject() const { return std::holds_alternative<Object>(storage_); }

    [[nodiscard]] bool asBool() const { return std::get<bool>(storage_); }
    [[nodiscard]] std::int64_t asInteger() const { return std::get<std::int64_t>(storage_); }
    [[nodiscard]] double asDouble() const { return std::get<double>(storage_); }
    [[nodiscard]] const std::string& asString() const { return std::get<std::string>(storage_); }
    [[nodiscard]] const Array& asArray() const { return std::get<Array>(storage_); }
    [[nodiscard]] Array& asArray() { return std::get<Array>(storage_); }
    [[nodiscard]] const Object& asObject() const { return std::get<Object>(storage_); }
    [[nodiscard]] Object& asObject() { return std::get<Object>(storage_); }

    [[nodiscard]] const Value* find(std::string_view key) const;
    [[nodiscard]] Value* find(std::string_view key);
    [[nodiscard]] bool operator==(const Value&) const = default;

private:
    Storage storage_;
};

} // namespace coverage
} // namespace finale_mus_reader
