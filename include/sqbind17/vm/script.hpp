#pragma once

#include "sqbind17/detail/errors.hpp"
#include "sqbind17/detail/function/stack_operation.hpp"
#include "sqbind17/detail/types/sqfunction.hpp"
#include "sqbind17/detail/types/sqvm.hpp"
#include <sqstdio.h>
#include <string>
#include <type_traits>
#include <utility>

namespace sqbind17 {
namespace detail {

class Script {
  public:
    explicit Script(VM vm) : vm(vm) {
    }

    void CompileString(const std::string &source, const std::string &name = "__main__") {
        stack_guard guard(vm);
        reset();
        if (SQ_FAILED(sq_compilebuffer(*vm, source.c_str(), static_cast<SQInteger>(source.size()), name.c_str(), SQTrue))) {
            throw sqbind17::value_error(lastError());
        }
        sq_getstackobj(*vm, -1, &closure);
        sq_addref(*vm, &closure);
    }

    bool CompileString(const std::string &source, std::string &err, const std::string &name = "__main__") {
        try {
            CompileString(source, name);
            return true;
        }
        catch (const std::exception &e) {
            err = e.what();
            return false;
        }
    }

    void CompileFile(const std::string &path) {
        stack_guard guard(vm);
        reset();
        if (SQ_FAILED(sqstd_loadfile(*vm, path.c_str(), SQTrue))) {
            throw sqbind17::value_error(lastError());
        }
        sq_getstackobj(*vm, -1, &closure);
        sq_addref(*vm, &closure);
    }

    bool CompileFile(const std::string &path, std::string &err) {
        try {
            CompileFile(path);
            return true;
        }
        catch (const std::exception &e) {
            err = e.what();
            return false;
        }
    }

    template <class Return = void> std::remove_reference_t<Return> Run() {
        ensureCompiled();
        detail::Closure<std::remove_reference_t<Return>()> func(_closure(closure), vm);
        return func();
    }

    template <class Return, class Env> std::remove_reference_t<Return> Run(Env &&env) {
        ensureCompiled();
        detail::Closure<std::remove_reference_t<Return>()> func(_closure(closure), vm);
        SQObjectPtr pthis = detail::generic_cast<Env, SQObjectPtr>(vm, std::forward<Env>(env));
        func.bindThis(pthis);
        return func();
    }

    bool Run(std::string &err) {
        try {
            Run<void>();
            return true;
        }
        catch (const std::exception &e) {
            err = e.what();
            return false;
        }
    }

    void WriteCompiledFile(const std::string &path) {
        ensureCompiled();
        stack_guard guard(vm);
        sq_pushobject(*vm, closure);
        if (SQ_FAILED(sqstd_writeclosuretofile(*vm, path.c_str()))) {
            throw sqbind17::value_error(lastError());
        }
    }

    bool IsCompiled() const {
        return sq_type(closure) == OT_CLOSURE;
    }

    void reset() {
        closure = SQObjectPtr();
    }

  private:
    VM vm;
    SQObjectPtr closure;

    void ensureCompiled() const {
        if (sq_type(closure) != OT_CLOSURE) {
            throw sqbind17::value_error("script is not compiled");
        }
    }

    std::string lastError() {
        const SQChar *sqErr = nullptr;
        sq_getlasterror(*vm);
        if (sq_gettype(*vm, -1) == OT_NULL) {
            sq_pop(*vm, 1);
            return "unknown squirrel error";
        }
        sq_tostring(*vm, -1);
        sq_getstring(*vm, -1, &sqErr);
        std::string err = sqErr ? sqErr : "unknown squirrel error";
        sq_pop(*vm, 2);
        return err;
    }
};

} // namespace detail
} // namespace sqbind17
