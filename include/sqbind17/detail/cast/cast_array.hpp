#pragma once
#include "sqbind17/detail/errors.hpp"
#include "sqbind17/detail/types/sqarray.hpp"
#include "sqbind17/detail/types/sqvm.hpp"
#include <type_traits>
namespace sqbind17 {
namespace detail {
// cast detail::Array to SQObjectPtr|HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, detail::Array>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    return from.pArray();
}

// cast HSQOBJECT|SQObjectPtr to detail::Array
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<FromType>, HSQOBJECT>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, detail::Array>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    if (from._type == tagSQObjectType::OT_ARRAY)
        return detail::Array(_array(from), vm);
    throw sqbind17::value_error("unsupported value");
}

} // namespace detail
} // namespace sqbind17
