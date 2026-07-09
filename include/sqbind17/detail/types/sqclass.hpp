#pragma once
#include "holder.hpp"
#include "sqbind17/detail/errors.hpp"
#include "sqbind17/detail/format.hpp"
#include "sqbind17/detail/sqdefinition.hpp"
#include "sqbind17/detail/template/template_getter.hpp"
#include "sqbind17/detail/template/template_setter.hpp"
#include "sqtable.hpp"
#include "sqvm.hpp"
#include <functional>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <utility>

namespace sqbind17 {
namespace detail {

struct NativeClassDataBase {
    std::type_index type;
    std::string name;
    std::shared_ptr<NativeClassDataBase> base;

    NativeClassDataBase(std::type_index type, std::string name, std::shared_ptr<NativeClassDataBase> base)
        : type(type), name(std::move(name)), base(std::move(base)) {
    }

    virtual ~NativeClassDataBase() = default;
    virtual void *cast(void *ptr, const NativeClassDataBase *target) const = 0;
    virtual void destroy(void *ptr) const = 0;
    virtual void *clone(const void *ptr) const = 0;
};

template <class C, class Base> struct NativeClassData : NativeClassDataBase {
    NativeClassData(std::string name, std::shared_ptr<NativeClassDataBase> base)
        : NativeClassDataBase(typeid(C), std::move(name), std::move(base)) {
    }

    void *cast(void *ptr, const NativeClassDataBase *target) const override {
        if (target == this || target->type == typeid(C)) {
            return ptr;
        }
        if constexpr (!std::is_void_v<Base>) {
            if (base != nullptr) {
                return base->cast(static_cast<Base *>(static_cast<C *>(ptr)), target);
            }
        }
        return nullptr;
    }

    void destroy(void *ptr) const override {
        delete static_cast<C *>(ptr);
    }

    void *clone(const void *ptr) const override {
        if constexpr (std::is_copy_constructible_v<C>) {
            return new C(*static_cast<const C *>(ptr));
        } else {
            throw sqbind17::value_error(name + " is not copy constructible");
        }
    }
};

struct NativeInstanceData {
    void *ptr;
    std::shared_ptr<NativeClassDataBase> type;
    bool owned;

    NativeInstanceData(void *ptr, std::shared_ptr<NativeClassDataBase> type, bool owned)
        : ptr(ptr), type(std::move(type)), owned(owned) {
    }
};

inline SQInteger native_instance_release(SQUserPointer ptr, SQInteger) {
    auto *instance = static_cast<NativeInstanceData *>(ptr);
    if (instance != nullptr) {
        if (instance->owned && instance->ptr != nullptr) {
            instance->type->destroy(instance->ptr);
        }
        delete instance;
    }
    return 0;
}

inline SQInteger native_instance_weakref(HSQUIRRELVM vm) {
    sq_weakref(vm, 1);
    return 1;
}

inline SQInteger native_instance_typeof(HSQUIRRELVM vm) {
    SQUserPointer tag = nullptr;
    if (SQ_FAILED(sq_gettypetag(vm, 1, &tag)) || tag == nullptr) {
        sq_pushstring(vm, "native", -1);
        return 1;
    }
    auto *type = static_cast<NativeClassDataBase *>(tag);
    sq_pushstring(vm, type->name.c_str(), static_cast<SQInteger>(type->name.size()));
    return 1;
}

class Class : public std::enable_shared_from_this<Class> {
    using Holder = SQObjectPtrHolder<::SQClass *>;
    using ErrNotFound = sqbind17::key_error;

  public:
    std::shared_ptr<Holder> holder;
    std::shared_ptr<NativeClassDataBase> type_data;
    bool closed;

  public:
    Class(::SQClass *pClass, VM vm, bool closed = true,
          std::shared_ptr<NativeClassDataBase> type_data = nullptr)
        : holder(std::make_shared<Holder>(pClass, vm)), type_data(std::move(type_data)), closed(closed) {};

  public:
    void close() {
        closed = true;
    }

    SQUnsignedInteger getRefCount() {
        return pClass()->_uiRef;
    }

    ::SQClass *pClass() {
        return _class(holder->GetSQObjectPtr());
    }

    SQObjectPtr createInstance(void *ptr, std::shared_ptr<NativeClassDataBase> actual_type, bool owned) {
        if (ptr == nullptr) {
            return SQObjectPtr();
        }
        ::SQInstance *instance = pClass()->CreateInstance();
        instance->_userpointer = new NativeInstanceData(ptr, std::move(actual_type), owned);
        instance->_hook = native_instance_release;
        return SQObjectPtr(instance);
    }

    void setInstanceUpAt(SQInteger index, void *ptr, std::shared_ptr<NativeClassDataBase> actual_type, bool owned) {
        if (ptr == nullptr) {
            throw sqbind17::value_error("cannot construct native instance from null pointer");
        }
        auto *instance = new NativeInstanceData(ptr, std::move(actual_type), owned);
        if (SQ_FAILED(::sq_setinstanceup(holder->GetSQVM(), index, instance))) {
            delete instance;
            throw sqbind17::value_error("failed to attach native instance data");
        }
        ::sq_setreleasehook(holder->GetSQVM(), index, native_instance_release);
    }

  public:
    SQOBJECTPTR_SETTER_TEMPLATE
    void set(SQObjectPtr &sqkey, SQObjectPtr &sqval) {
        VM &vm = holder->GetVM();
        SQObjectPtr &self = holder->GetSQObjectPtr();

        sq_pushobject(*vm, self);
        sq_pushobject(*vm, sqkey);
        sq_pushobject(*vm, sqval);
        sq_newslot(*vm, -3, SQTrue);
        sq_pop(*vm, 1);
    }

  public:
    SQOBJECTPTR_GETTER_TEMPLATE
  protected:
    bool get(SQObjectPtr &key, SQObjectPtr &ret) {
        VM &vm = holder->GetVM();
        SQObjectPtr &self = holder->GetSQObjectPtr();
        if (!(*vm)->Get(self, key, ret, false, DONT_FALL_BACK)) {
            return false;
        }
        return true;
    }

  private:
    std::shared_ptr<detail::Table> _delegate;
    std::shared_ptr<detail::Table> getDelegate() {
        if (_delegate == nullptr) {
            _delegate = std::make_shared<Table>(holder->GetVM());
            auto _get = [this](std::string property) -> SQObjectPtr {
                SQObjectPtr v;
                if (!_delegate->get(property + ".fget", v)) {
                    ::sq_getstackobj(holder->GetSQVM(), 1, &v);
                    ::sq_addref(holder->GetSQVM(), &v);

                    if (_delegate->get(property, v)) {
                        return v;
                    }
                    throw sqbind17::key_error(property + " does not found.");
                }
                ::sq_pushobject(holder->GetSQVM(), v);
                ::sq_push(holder->GetSQVM(), 1);
                ::sq_call(holder->GetSQVM(), 1, SQTrue, SQTrue);
                ::sq_getstackobj(holder->GetSQVM(), -1, &v);
                ::sq_addref(holder->GetSQVM(), &v);
                return v;
            };
            auto _set = [this](std::string property, SQObjectPtr value) {
                SQObjectPtr setter;
                if (!_delegate->get(property + ".fset", setter)) {
                    _delegate->set(std::forward<std::string>(property), std::forward<SQObjectPtr>(value));
                    return;
                }
                ::sq_pushobject(holder->GetSQVM(), setter);
                ::sq_push(holder->GetSQVM(), 1);
                ::sq_pushobject(holder->GetSQVM(), value);
                ::sq_call(holder->GetSQVM(), 2, SQFalse, SQTrue);
            };
            bindFunc("_get", _get);
            bindFunc("_set", _set);
        }
        return _delegate;
    }

  public:
    template <typename TClass, typename P> void defProperty(std::string property, P TClass::*pm) {
        auto fget = detail::to_cpp_function<1>([=](TClass *self) -> P { return self->*pm; });
        auto fset = detail::to_cpp_function<1>([=](TClass *self, P value) { self->*pm = value; });
        getDelegate()->set(std::move(property + ".fget"),
                           std::move(detail::NativeClosure<P(TClass *)>::Create(fget, holder->GetVM())));
        getDelegate()->set(std::move(property + ".fset"),
                           std::move(detail::NativeClosure<void(TClass *, P)>::Create(fset, holder->GetVM())));
    }

    template <typename TClass, typename Getter> void defReadonlyProperty(std::string property, Getter getter) {
        auto fget = detail::to_cpp_function<1>(getter);
        auto fset = detail::to_cpp_function<1>([property](SQObjectPtr, SQObjectPtr) {
            throw sqbind17::value_error(property + " is read-only");
        });
        getDelegate()->set(std::move(property + ".fget"),
                           std::move(detail::NativeClosure<detail::function_signature_t<Getter>>::Create(
                               fget, holder->GetVM())));
        getDelegate()->set(std::move(property + ".fset"),
                           std::move(detail::NativeClosure<void(SQObjectPtr, SQObjectPtr)>::Create(
                               fset, holder->GetVM())));
    }

    template <typename Getter, typename Setter> void defProperty(std::string property, Getter getter, Setter setter) {
        auto fget = detail::to_cpp_function<1>(getter);
        auto fset = detail::to_cpp_function<1>(setter);
        getDelegate()->set(std::move(property + ".fget"),
                           std::move(detail::NativeClosure<detail::function_signature_t<Getter>>::Create(
                               fget, holder->GetVM())));
        getDelegate()->set(std::move(property + ".fset"),
                           std::move(detail::NativeClosure<detail::function_signature_t<Setter>>::Create(
                               fset, holder->GetVM())));
    }

    template <typename P> void defStaticVar(std::string property, P *ptr) {
        auto fget = detail::to_cpp_function<1>([ptr](SQObjectPtr) -> P { return *ptr; });
        auto fset = detail::to_cpp_function<1>([ptr](SQObjectPtr, P value) { *ptr = value; });
        getDelegate()->set(std::move(property + ".fget"),
                           std::move(detail::NativeClosure<P(SQObjectPtr)>::Create(fget, holder->GetVM())));
        getDelegate()->set(std::move(property + ".fset"),
                           std::move(detail::NativeClosure<void(SQObjectPtr, P)>::Create(fset, holder->GetVM())));
    }

  public:
    // bindFunc to current class
    template <typename Func> void bindFunc(std::string funcname, Func &&func, bool withenv = false) {
        set(std::move(funcname), std::move(detail::CreateNativeClosure(std::forward<Func>(func), holder->GetVM())));
    }

    template <typename Func> void bindRawFunc(std::string funcname, Func func) {
        VM &vm = holder->GetVM();
        sq_pushobject(*vm, holder->GetSQObjectPtr());
        sq_pushstring(*vm, funcname.c_str(), static_cast<SQInteger>(funcname.size()));
        sq_newclosure(*vm, func, 0);
        sq_newslot(*vm, -3, SQFalse);
        sq_pop(*vm, 1);
    }
};
} // namespace detail
} // namespace sqbind17

namespace sqbind17 {
namespace detail {

class ClassRegistry;
static std::map<void *, std::shared_ptr<ClassRegistry>> instances;

class ClassRegistry {
    // public:
    //   static std::map<void *, std::shared_ptr<ClassRegistry>> instances;

  public:
    std::map<size_t, std::shared_ptr<Class>> class_map;

  public:
    ClassRegistry() {
    }

  public:
    static std::shared_ptr<ClassRegistry> getInstance(VM vm) {
        auto k = (void *)(vm.vm());
        auto i = instances.find(k);
        if (i == instances.end()) {
            auto ptr = std::make_shared<ClassRegistry>();
            instances[k] = ptr;
            return ptr;
        }
        return i->second;
    }

  public:
    template <class C> std::shared_ptr<Class> find_class_object() {
        size_t key = typeid(std::remove_cv_t<C>).hash_code();
        auto i = class_map.find(key);
        if (i == class_map.end()) {
            return nullptr;
        } else {
            return i->second;
        }
    }

    template <class C> void register_class(std::shared_ptr<Class> clazz) {
        class_map[typeid(std::remove_cv_t<C>).hash_code()] = clazz;
    }
};

// std::map<void *, std::shared_ptr<ClassRegistry>> ClassRegistry::instances;

} // namespace detail
} // namespace sqbind17

namespace sqbind17 {
namespace detail {
template <class C, class Base = void> class ClassDef {
    using Holder = SQObjectPtrHolder<::SQClass *>;

  public:
    std::shared_ptr<Class> holder;
    std::string name;

  public:
    static size_t hash() {
        return typeid(C).hash_code();
    }

  public:
    ClassDef(VM vm, const std::string &name) : holder(nullptr), name(name) {
        auto base = ClassRegistry::getInstance(vm)->find_class_object<Base>();
        if constexpr (!std::is_void_v<Base>) {
            if (base == nullptr) {
                throw sqbind17::value_error("base class must be registered before derived class");
            }
        }
        auto type_data = std::make_shared<NativeClassData<C, Base>>(name, base ? base->type_data : nullptr);
        holder =
            std::make_shared<Class>(::SQClass::Create(_ss(*vm), base ? base->pClass() : nullptr), vm, false, type_data);
        holder->pClass()->_typetag = type_data.get();
        holder->set(std::string("classname"), this->name);
        holder->bindRawFunc("weakref", native_instance_weakref);
        holder->bindRawFunc("_typeof", native_instance_typeof);
        ClassRegistry::getInstance(vm)->register_class<C>(holder);
    }

  public:
    ~ClassDef() {
        holder->close();
    }

    ::SQClass *pClass() {
        return holder->pClass();
    }

  public:
    template <typename P> ClassDef<C, Base> &defProperty(std::string property, P C::*pm) {
        holder->defProperty<C, P>(property, pm);
        return *this;
    }

    template <typename P> ClassDef<C, Base> &var(std::string property, P C::*pm) {
        return defProperty(std::move(property), pm);
    }

    template <typename Getter> ClassDef<C, Base> &defReadonlyProperty(std::string property, Getter getter) {
        holder->defReadonlyProperty<C>(std::move(property), getter);
        return *this;
    }

    template <typename Getter, typename Setter>
    ClassDef<C, Base> &defProperty(std::string property, Getter getter, Setter setter) {
        holder->defProperty(std::move(property), getter, setter);
        return *this;
    }

    template <typename P> ClassDef<C, Base> &defStaticVar(std::string property, P *ptr) {
        holder->defStaticVar(std::move(property), ptr);
        return *this;
    }

    template <typename P> ClassDef<C, Base> &staticVar(std::string property, P *ptr) {
        return defStaticVar(std::move(property), ptr);
    }

    template <typename V> ClassDef<C, Base> &setStaticValue(std::string key, V &&value) {
        holder->set(std::move(key), std::forward<V>(value));
        return *this;
    }

  public:
    // bindFunc to current class
    template <typename Func> ClassDef<C, Base> &bindFunc(std::string funcname, Func &&func, bool withenv = false) {
        if (!holder->closed)
            holder->bindFunc<Func>(funcname, std::forward<Func>(func), withenv);
        return *this;
    }

    template <typename Func> ClassDef<C, Base> &func(std::string funcname, Func &&func) {
        return bindFunc(std::move(funcname), std::forward<Func>(func));
    }

    template <typename Func> ClassDef<C, Base> &bindStaticFunc(std::string funcname, Func &&func) {
        return bindFunc(std::move(funcname), std::forward<Func>(func));
    }

    template <typename Func> ClassDef<C, Base> &staticFunc(std::string funcname, Func &&func) {
        return bindStaticFunc(std::move(funcname), std::forward<Func>(func));
    }

  private:
    using ConstructorCaller = SQInteger (*)(HSQUIRRELVM);

    static std::map<HSQUIRRELVM, std::map<SQInteger, ConstructorCaller>> &constructors() {
        static std::map<HSQUIRRELVM, std::map<SQInteger, ConstructorCaller>> values;
        return values;
    }

    template <typename... Args> static SQInteger ctorImpl(HSQUIRRELVM sqvm) {
        VM vm(sqvm, false);
        auto clazz = ClassRegistry::getInstance(vm)->find_class_object<C>();
        if (clazz == nullptr) {
            return sq_throwerror(sqvm, "native class is not registered");
        }
        try {
            auto args = detail::load_args<2, std::tuple<Args...>>::load(vm);
            C *instance = std::apply([](auto &&...items) { return new C(std::forward<decltype(items)>(items)...); },
                                    std::move(args));
            clazz->setInstanceUpAt(1, instance, clazz->type_data, true);
            return 0;
        }
        catch (const std::exception &e) {
            return sq_throwerror(sqvm, e.what());
        }
    }

    static SQInteger ctorDispatch(HSQUIRRELVM sqvm) {
        auto vm_it = constructors().find(sqvm);
        if (vm_it == constructors().end()) {
            return sq_throwerror(sqvm, "no native constructors are registered");
        }
        SQInteger nargs = sq_gettop(sqvm) - 1;
        auto ctor_it = vm_it->second.find(nargs);
        if (ctor_it == vm_it->second.end()) {
            return sq_throwerror(sqvm, "wrong number of constructor parameters");
        }
        return ctor_it->second(sqvm);
    }

  public:
    template <typename... Args> ClassDef<C, Base> &ctor() {
        constructors()[holder->holder->GetSQVM()][sizeof...(Args)] = &ClassDef<C, Base>::template ctorImpl<Args...>;
        holder->bindRawFunc("constructor", &ClassDef<C, Base>::ctorDispatch);
        return *this;
    }
};
} // namespace detail
} // namespace sqbind17
