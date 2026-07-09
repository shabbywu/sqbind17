#pragma once
#include "sqbind17/detail/errors.hpp"
#include "sqbind17/detail/malloc.hpp"
#include "sqbind17/detail/types/sqarray.hpp"
#include "sqbind17/detail/types/sqclass.hpp"
#include "sqbind17/detail/types/sqvm.hpp"
#include <string>
#include <type_traits>
#include <utility>
namespace sqbind17 {
namespace detail {

template <typename T> using native_object_t = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
template <typename T> struct is_class_def : std::false_type {};
template <class C, class Base> struct is_class_def<detail::ClassDef<C, Base>> : std::true_type {};

// cast SQObjectPtr/HSQOBJECT to pointer
template <
    typename FromType, typename ToType,
    typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, SQObjectPtr> ||
                              std::is_same_v<std::decay_t<FromType>, HSQOBJECT>> * = nullptr,
    typename std::enable_if_t<std::is_class_v<native_object_t<ToType>> && std::is_pointer_v<ToType>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;

#endif
    using Target = native_object_t<ToType>;
    if (from._type == tagSQObjectType::OT_NULL) {
        return nullptr;
    }
    if (from._type == tagSQObjectType::OT_INSTANCE) {
        auto clazz = ClassRegistry::getInstance(vm)->find_class_object<Target>();
        if (clazz == nullptr || clazz->type_data == nullptr) {
            throw sqbind17::value_error("native class is not registered");
        }
        auto *instance = static_cast<NativeInstanceData *>(_instance(from)->_userpointer);
        if (instance == nullptr || instance->ptr == nullptr || instance->type == nullptr) {
            throw sqbind17::value_error("native instance is not constructed");
        }
        void *casted = instance->type->cast(instance->ptr, clazz->type_data.get());
        if (casted == nullptr) {
            throw sqbind17::value_error("invalid native instance type");
        }
        return static_cast<ToType>(casted);
    }
    throw sqbind17::value_error("unsupported value");
}

// cast SQObjectPtr/HSQOBJECT to class reference
template <
    typename FromType, typename ToType,
    typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, SQObjectPtr> ||
                              std::is_same_v<std::decay_t<FromType>, HSQOBJECT>> * = nullptr,
    typename std::enable_if_t<std::is_class_v<native_object_t<ToType>> && std::is_reference_v<ToType> &&
                              !std::is_same_v<native_object_t<ToType>, detail::Table> &&
                              !std::is_same_v<native_object_t<ToType>, detail::Array> &&
                              !std::is_base_of_v<detail::ClosureBase, native_object_t<ToType>>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    using Target = native_object_t<ToType>;
    auto ptr = generic_cast<FromType, Target *>(vm, std::forward<FromType>(from));
    if (ptr == nullptr) {
        throw sqbind17::value_error("null native instance");
    }
    return *ptr;
}

// cast SQObjectPtr/HSQOBJECT to class value by copy
template <
    typename FromType, typename ToType,
    typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, SQObjectPtr> ||
                              std::is_same_v<std::decay_t<FromType>, HSQOBJECT>> * = nullptr,
    typename std::enable_if_t<std::is_class_v<std::decay_t<ToType>> && !std::is_pointer_v<std::decay_t<ToType>> &&
                              !std::is_reference_v<ToType> && !std::is_same_v<std::decay_t<ToType>, detail::Table> &&
                              !std::is_same_v<std::decay_t<ToType>, detail::Array> &&
                              !std::is_same_v<std::decay_t<ToType>, std::string> &&
                              !std::is_same_v<std::decay_t<ToType>, SQObjectPtr> &&
                              !std::is_same_v<std::decay_t<ToType>, HSQOBJECT> &&
                              !std::is_base_of_v<detail::ClosureBase, std::decay_t<ToType>> &&
                              !is_class_def<std::decay_t<ToType>>::value> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    auto ptr = generic_cast<FromType, ToType *>(vm, std::forward<FromType>(from));
    if (ptr == nullptr) {
        throw sqbind17::value_error("null native instance");
    }
    return *ptr;
}

// cast pointer to SQObjectPtr/HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_class_v<native_object_t<FromType>> && std::is_pointer_v<FromType>> * =
              nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    using Source = native_object_t<FromType>;
    if (from == nullptr) {
        return SQObjectPtr();
    }
    auto clazz = ClassRegistry::getInstance(vm)->find_class_object<Source>();
    if (clazz != nullptr) {
        return clazz->createInstance(const_cast<Source *>(from), clazz->type_data, false);
    }
    // fallback
    return SQObjectPtr(detail::make_userdata(vm, std::forward<FromType>(from)));
}

// cast class reference/value to SQObjectPtr/HSQOBJECT
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_class_v<native_object_t<FromType>> &&
                                    !std::is_pointer_v<std::decay_t<FromType>> &&
                                    !std::is_same_v<std::decay_t<FromType>, detail::Table> &&
                                    !std::is_same_v<std::decay_t<FromType>, detail::Array> &&
                                    !std::is_same_v<std::decay_t<FromType>, std::string> &&
                                    !std::is_same_v<std::decay_t<FromType>, SQObjectPtr> &&
                                    !std::is_same_v<std::decay_t<FromType>, HSQOBJECT> &&
                                    detail::function_traits<std::decay_t<FromType>>::value ==
                                        detail::CppFuntionType::NotFunc &&
                                    !std::is_base_of_v<detail::ClosureBase, std::decay_t<FromType>> &&
                                    !is_class_def<std::decay_t<FromType>>::value> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
#ifdef TRACE_OBJECT_CAST
    std::cout << "[TRACING] cast " << typeid(decltype(from)).name() << " to " << typeid(ToType).name() << std::endl;
#endif
    using Source = native_object_t<FromType>;
    auto clazz = ClassRegistry::getInstance(vm)->find_class_object<Source>();
    if (clazz == nullptr) {
        return SQObjectPtr(detail::make_userdata(vm, new Source(std::forward<FromType>(from))));
    }
    if constexpr (std::is_lvalue_reference_v<FromType>) {
        return clazz->createInstance(const_cast<Source *>(&from), clazz->type_data, false);
    } else {
        return clazz->createInstance(new Source(std::forward<FromType>(from)), clazz->type_data, true);
    }
}

// cast Class/ClassDef to SQObjectPtr|HSQOBJECT so callers can bind classes into tables.
template <typename FromType, typename ToType,
          typename std::enable_if_t<std::is_same_v<std::decay_t<FromType>, detail::Class>> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
    return from.pClass();
}

template <typename FromType, typename ToType,
          typename std::enable_if_t<is_class_def<std::decay_t<FromType>>::value> * = nullptr,
          typename std::enable_if_t<std::is_same_v<std::decay_t<ToType>, SQObjectPtr> ||
                                    std::is_same_v<std::decay_t<ToType>, HSQOBJECT>> * = nullptr>
static ToType generic_cast(detail::VM vm, FromType &&from) {
    return from.pClass();
}

} // namespace detail
} // namespace sqbind17
