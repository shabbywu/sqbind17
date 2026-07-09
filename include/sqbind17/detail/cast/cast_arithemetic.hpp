#pragma once
#include "sqbind17/detail/errors.hpp"
#include "sqbind17/detail/types/sqvm.hpp"
#include <squirrel.h>
#include <type_traits>

namespace sqbind17 {
namespace detail {
// cast SQObjectPtr/HSQOBJECT to arithemetic
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<FromType>, HSQOBJECT>> * = nullptr,
          typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<ToType>> ||
                                    std::is_enum_v<std::decay_t<ToType>>> * = nullptr>
ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    if (from._type == tagSQObjectType::OT_INTEGER || from._type == tagSQObjectType::OT_BOOL)
        return static_cast<ToType>(_integer(from));
    if (from._type == tagSQObjectType::OT_FLOAT)
        return static_cast<ToType>(_float(from));
    throw sqbind17::value_error("unsupported value");
};

// cast arithmetic to arithmetic
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<FromType>> ||
                                    std::is_enum_v<std::decay_t<FromType>>> * = nullptr,
          typename std::enable_if_t<std::is_arithmetic_v<std::decay_t<ToType>> ||
                                    std::is_enum_v<std::decay_t<ToType>>> * = nullptr>
ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    return static_cast<ToType>(from);
};

// cast integral(not include bool) to SQObjectPtr|HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<(std::is_integral_v<std::decay_t<FromType>> ||
                                     std::is_enum_v<std::decay_t<FromType>>) &&
                                    !std::is_same_v<std::decay_t<FromType>, bool>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    return SQObjectPtr(static_cast<SQInteger>(from));
};

// cast floating point to SQObjectPtr|HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_floating_point_v<std::decay_t<FromType>>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    return SQObjectPtr((float)from);
};

// cast bool to SQObjectPtr|HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, bool>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    return SQObjectPtr((FromType)from);
};

} // namespace detail
} // namespace sqbind17
