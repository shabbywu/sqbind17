#pragma once
#include "sqbind17/detail/errors.hpp"
#include "sqbind17/detail/types/sqfunction.hpp"
#include "sqbind17/detail/types/sqvm.hpp"
#include <type_traits>
namespace sqbind17 {
namespace detail {
// cast detail::Closure to SQObjectPtr|HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_base_of_v<detail::ClosureBase, std::decay_t<FromType>> &&
                                    !std::is_base_of_v<detail::NativeClosureBase, std::decay_t<FromType>>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    return from.pClosure();
}

// cast detail::NativeClosure to SQObjectPtr|HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_base_of_v<detail::NativeClosureBase, std::decay_t<FromType>>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    return from.pNativeClosure();
}

// cast native cpp function to SQObjectPtr|HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<detail::function_traits<FromType>::value != detail::CppFuntionType::NotFunc &&
                                    !std::is_base_of_v<detail::ClosureBase, std::decay_t<FromType>>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    return detail::CreateNativeClosure(std::forward<FromType>(from), vm).pNativeClosure();
}

////////////////////////////////////////////////////////////////////////////////////////

// cast SQObjectPtr|HSQOBJECT to detail::Closure
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<FromType>, HSQOBJECT>> * = nullptr,
          typename std::enable_if_t<std::is_base_of_v<detail::ClosureBase, std::decay_t<ToType>> &&
                                    !std::is_base_of_v<detail::NativeClosureBase, std::decay_t<ToType>>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    if (from._type == tagSQObjectType::OT_CLOSURE)
        return ToType(_closure(from), vm);
    throw sqbind17::value_error("unsupported value");
}

// cast SQObjectPtr|HSQOBJECT to detail::NativeClosure
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<FromType>, HSQOBJECT>> * = nullptr,
          typename std::enable_if_t<std::is_base_of_v<detail::NativeClosureBase, std::decay_t<ToType>>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    if (from._type == tagSQObjectType::OT_NATIVECLOSURE)
        return ToType(_nativeclosure(from), vm);
    throw sqbind17::value_error("unsupported value");
}

} // namespace detail
} // namespace sqbind17
