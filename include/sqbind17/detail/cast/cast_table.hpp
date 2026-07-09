#pragma once
#include "sqbind17/detail/errors.hpp"
#include "sqbind17/detail/types/sqtable.hpp"
#include "sqbind17/detail/types/sqvm.hpp"
#include <memory>
#include <type_traits>
namespace sqbind17 {
namespace detail {
// cast detail::Table to SQObjectPtr|HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, detail::Table>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    return from.pTable();
}

// cast HSQOBJECT to SQObjectPtr|HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<FromType>, HSQOBJECT>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, detail::Table> &&
                                    !std::is_reference_v<ToType>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    if (from._type == tagSQObjectType::OT_TABLE)
        return detail::Table(_table(from), vm);
    throw sqbind17::value_error("unsupported value");
}

// cast HSQOBJECT|SQObjectPtr to detail::Table&
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<FromType>, HSQOBJECT>> * = nullptr,
          typename std::enable_if_t<std::is_reference_v<ToType> &&
                                    std::is_same_v<std::remove_cv_t<std::remove_reference_t<ToType>>,
                                                   detail::Table>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
    if (from._type != tagSQObjectType::OT_TABLE) {
        throw sqbind17::value_error("unsupported value");
    }
    thread_local std::unique_ptr<detail::Table> value;
    value = std::make_unique<detail::Table>(_table(from), vm);
    return *value;
}

} // namespace detail
} // namespace sqbind17
