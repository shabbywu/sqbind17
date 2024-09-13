#pragma once

#include "holder.hpp"
#include "sqbind17/detail/errors.hpp"
#include "sqbind17/detail/format.hpp"
#include "sqbind17/detail/function/stack_operation.hpp"
#include "sqbind17/detail/sqdefinition.hpp"
#include "sqbind17/detail/template/template_getter.hpp"
#include "sqbind17/detail/template/template_setter.hpp"
#include "sqtable.h"
#include "sqvm.hpp"

namespace sqbind17 {
namespace detail {
template <class T> class UserData {
    using Holder = SQObjectPtrHolder<::SQUserData *>;
    using ErrNotFound = sqbind17::key_error;

  public:
    std::shared_ptr<Holder> holder;
    std::shared_ptr<detail::Table> delegate;

  public:
    UserData(::SQUserData *pUserData, VM vm) : holder(std::make_shared<Holder>(pUserData, vm)) {
    }
    UserData(::SQUserData *pUserData, detail::Table delegate, VM vm)
        : holder(std::make_shared<Holder>(pUserData, vm)),
          delegate(std::make_shared<detail::Table>(delegate.pTable(), vm)) {
    }

    SQUnsignedInteger getRefCount() {
        return pUserData()->_uiRef;
    }

    ::SQUserData *pUserData() {
        return _userdata(holder->GetSQObjectPtr());
    }

    std::shared_ptr<sqbind17::detail::Table> GetDelegate() {
        return delegate;
    }
    void SetDelegate(std::shared_ptr<sqbind17::detail::Table> delegate) {
        this->delegate = delegate;
    }

  public:
    SQOBJECTPTR_SETTER_TEMPLATE
    void set(SQObjectPtr &sqkey, SQObjectPtr &sqval) {
        VM &vm = holder->GetVM();
        SQObjectPtr &self = holder->GetSQObjectPtr();

        sq_pushobject(*vm, self);
        sq_pushobject(*vm, sqkey);
        sq_pushobject(*vm, sqval);
        sq_newslot(*vm, -3, SQFalse);
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
};
} // namespace detail
} // namespace sqbind17
