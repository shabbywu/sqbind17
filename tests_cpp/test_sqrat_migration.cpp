#include <sqbind17/sqbind17.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace sqbind17;

struct Counter {
    Counter() = default;
    explicit Counter(int value) : value(value) {
    }

    int get() const {
        return value;
    }

    void set(int next) {
        value = next;
    }

    int value = 0;
    static int shared;
};

int Counter::shared = 3;

struct Animal {
    virtual ~Animal() = default;
    virtual std::string speak() {
        return "silent";
    }
};

struct Cat : Animal {
    std::string speak() override {
        return "meow";
    }
};

static std::string makeSpeak(Animal &animal) {
    return animal.speak();
}

static void setTableValue(detail::Table &table, const char *key, int value) {
    table.set(key, value);
}

static void setArrayValue(detail::Array &array, int index, int value) {
    array.set(index, value);
}

static std::string echoString(std::string value) {
    return value;
}

int main() {
    try {
        detail::GenericVM vm;

        detail::ClassDef<Counter> counter(vm.GetVM(), "Counter");
        counter.ctor<>()
            .ctor<int>()
            .var("value", &Counter::value)
            .defReadonlyProperty("readonlyValue", &Counter::get)
            .staticVar("shared", &Counter::shared)
            .func("set", &Counter::set);
        vm.getroottable()->bind("Counter", counter);

        detail::ClassDef<Animal> animal(vm.GetVM(), "Animal");
        animal.ctor<>().func("speak", &Animal::speak);
        vm.getroottable()->bind("Animal", animal);

        detail::ClassDef<Cat, Animal> cat(vm.GetVM(), "Cat");
        cat.ctor<>().func("speak", &Cat::speak);
        vm.getroottable()->bind("Cat", cat);

        vm.bindFunc("makeSpeak", &makeSpeak);
        vm.bindFunc("setTableValue", &setTableValue);
        vm.bindFunc("setArrayValue", &setArrayValue);
        vm.bindFunc("echoString", &echoString);

        detail::ConstTable(vm.GetVM()).Const("Version", "1.0.0");

        vm.ExecuteString(R"(
        local c = Counter(7);
        if (c.value != 7) throw "bad constructor";
        c.set(9);
        if (c.readonlyValue != 9) throw "bad readonly property";
        c.shared = 42;
        local c2 = Counter();
        if (c2.shared != 42) throw "bad static var";
        if (typeof c2 != "Counter") throw "bad typeof";

        local cat = Cat();
        if (makeSpeak(cat) != "meow") throw "bad inherited cast";
        if (echoString("ok") != "ok") throw "bad string argument";

        local table = {};
        setTableValue(table, "answer", 42);
        if (table.answer != 42) throw "bad table reference";

        local array = [0, 0];
        setArrayValue(array, 1, 12);
        if (array[1] != 12) throw "bad array reference";

        if (Version != "1.0.0") throw "bad const table";
    )");

        return 0;
    }
    catch (const std::exception &e) {
        std::cerr << "test_sqrat_migration failed: " << e.what() << std::endl;
        return 1;
    }
}
