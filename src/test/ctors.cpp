#include <ankerl/svector.h>

#include <app/Counter.h>

#include <doctest.h>
#include <fmt/format.h>

namespace {

struct Foo {
    Foo(int a, char b) {}
};

TEST_CASE("ctor_default") {
    auto counts = Counter();
    auto sv = ankerl::svector<Counter::Obj, 4>();
    REQUIRE(Counter::staticDefaultCtor == 0);
    REQUIRE(Counter::staticDtor == 0);

    // default ctor shouldn't be needed
    auto sv2 = ankerl::svector<Foo, 7>();
    REQUIRE(sv2.size() == 0);
}

TEST_CASE("ctor_count") {
    auto counts = Counter();
    // counts.printCounts("begin");
    auto o = Counter::Obj(123, counts);
    // counts.printCounts("one o");

    // creates a vector with copies, no allocation yet
    REQUIRE(counts.ctor == 1);
    auto sv = ankerl::svector<Counter::Obj, 7>(7, o);
    // counts.printCounts("ctor with 7");
    REQUIRE(counts.copyCtor == 7);
    REQUIRE(counts.moveCtor == 0);
    REQUIRE(sv.size() == 7);
}

TEST_CASE("ctor_count_big") {
    auto sv = ankerl::svector<char, 7>(100000, 'x');
    REQUIRE(sv.size() == 100000);
    for (auto c : sv) {
        REQUIRE(c == 'x');
    }
}

TEST_CASE("ctor_default") {
    auto counts = Counter();
    REQUIRE(counts.staticDefaultCtor == 0);
    counts.printCounts("begin");
    // no copies are made, just default ctor
    auto sv = ankerl::svector<Counter::Obj, 3>(100);
    //auto sv = std::vector<Counter::Obj>(100);
    counts.printCounts("after 100");
    REQUIRE(Counter::staticDefaultCtor == 100);
}

} // namespace
