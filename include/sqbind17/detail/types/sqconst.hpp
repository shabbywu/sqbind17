#pragma once

#include "sqbind17/detail/types/sqtable.hpp"
#include "sqbind17/detail/types/sqvm.hpp"
#include <memory>
#include <string>
#include <utility>

namespace sqbind17 {
namespace detail {

class Enumeration {
  public:
    explicit Enumeration(VM vm) : table(vm) {
    }

    template <typename V> Enumeration &constant(std::string name, V &&value) {
        table.set(std::move(name), std::forward<V>(value));
        return *this;
    }

    template <typename V> Enumeration &Const(std::string name, V &&value) {
        return constant(std::move(name), std::forward<V>(value));
    }

    Table &asTable() {
        return table;
    }

  private:
    Table table;
};

class ConstTable {
  public:
    explicit ConstTable(VM vm) : vm(vm), table(nullptr) {
        sq_pushconsttable(*vm);
        HSQOBJECT obj;
        sq_getstackobj(*vm, -1, &obj);
        sq_pop(*vm, 1);
        table = std::make_shared<Table>(_table(obj), vm);
    }

    template <typename V> ConstTable &constant(std::string name, V &&value) {
        table->set(std::move(name), std::forward<V>(value));
        return *this;
    }

    template <typename V> ConstTable &Const(std::string name, V &&value) {
        return constant(std::move(name), std::forward<V>(value));
    }

    ConstTable &Enum(std::string name, Enumeration &enumeration) {
        table->set(std::move(name), enumeration.asTable());
        return *this;
    }

    Table &asTable() {
        return *table;
    }

  private:
    VM vm;
    std::shared_ptr<Table> table;
};

} // namespace detail
} // namespace sqbind17
