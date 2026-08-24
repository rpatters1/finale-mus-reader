// Copyright (c) 2026 Robert G. Patterson
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "coverage/context.h"
#include "coverage/json.h"
#include "coverage/value.h"

namespace finale_mus_reader {
namespace coverage {

template <typename Accessor>
struct Field
{
    std::string_view name;
    Accessor accessor;
};

template <typename Accessor>
constexpr auto field(std::string_view name, Accessor accessor)
{
    return Field<Accessor>{name, std::move(accessor)};
}

namespace detail {

template <typename T>
struct IsOptional : std::false_type {};

template <typename T>
struct IsOptional<std::optional<T>> : std::true_type {};

template <typename T>
Value schemaValue(T&& value)
{
    using ValueType = std::remove_cvref_t<T>;
    if constexpr (std::same_as<ValueType, Value>) {
        return std::forward<T>(value);
    } else if constexpr (IsOptional<ValueType>::value) {
        return value ? schemaValue(*value) : Value{};
    } else if constexpr (std::same_as<ValueType, bool>) {
        return Value(value);
    } else if constexpr (std::is_enum_v<ValueType>) {
        return Value(static_cast<std::int64_t>(value));
    } else if constexpr (std::integral<ValueType>) {
        return Value(static_cast<std::int64_t>(value));
    } else if constexpr (std::floating_point<ValueType>) {
        return Value(static_cast<double>(value));
    } else if constexpr (std::convertible_to<T, std::string_view>) {
        return Value(std::string(std::string_view(value)));
    } else {
        static_assert(std::is_void_v<ValueType>, "coverage schema field has unsupported type");
    }
}

template <typename Object, typename Accessor>
decltype(auto) fieldValue(const Object& object, const SurveyContext& context,
    const Accessor& accessor)
{
    if constexpr (std::invocable<Accessor, const Object&, const SurveyContext&>) {
        return std::invoke(accessor, object, context);
    } else {
        static_assert(std::invocable<Accessor, const Object&>);
        return std::invoke(accessor, object);
    }
}

} // namespace detail

template <typename Object, typename... Fields>
Value observe(const Object& object, const SurveyContext& context, const Fields&... fields)
{
    Value::Object result;
    (result.emplace(std::string(fields.name),
         detail::schemaValue(detail::fieldValue(object, context, fields.accessor))), ...);
    return Value(std::move(result));
}

template <typename Class>
std::string fieldOrigin(const SurveyContext& context, std::string_view member,
    musx::dom::Cmper cmper1 = 0)
{
    const auto* info = context.report.findField<Class>(member, musx::dom::SCORE_PARTID,
        cmper1 ? std::optional<musx::dom::Cmper>(cmper1) : std::nullopt);
    return info ? originName(info->origin) : "absent";
}

template <typename Class>
const TextFieldInfo* textFieldInfo(const SurveyContext& context, std::string_view member,
    musx::dom::Cmper cmper1 = 0)
{
    const auto instance = InstanceKey{typeid(Class), musx::dom::SCORE_PARTID,
        cmper1 ? std::optional<musx::dom::Cmper>(cmper1) : std::nullopt, std::nullopt,
        std::nullopt};
    const auto foundInstance = context.report.textFields.find(instance);
    if (foundInstance == context.report.textFields.end()) return nullptr;
    const auto foundField = foundInstance->second.find(std::string(member));
    return foundField == foundInstance->second.end() ? nullptr : &foundField->second;
}

template <typename Class>
constexpr auto originField(std::string_view name, std::string_view member)
{
    return field(name, [member](const Class&, const SurveyContext& context) {
        return fieldOrigin<Class>(context, member);
    });
}

} // namespace coverage
} // namespace finale_mus_reader
