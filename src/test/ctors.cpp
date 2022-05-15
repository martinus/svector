#include <ankerl/svector.h>

#include <app/Counter.h>

#include <doctest.h>
#include <fmt/format.h>

#include <forward_list>

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
    INFO(counts.printCounts("begin"));
    auto o = Counter::Obj(123, counts);
    INFO(counts.printCounts("one o"));

    // creates a vector with copies, no allocation yet
    REQUIRE(counts.ctor == 1);
    auto sv = ankerl::svector<Counter::Obj, 7>(7, o);
    INFO(counts.printCounts("ctor with 7"));
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
    INFO(counts.printCounts("begin"));
    //  no copies are made, just default ctor
    auto sv = ankerl::svector<Counter::Obj, 3>(100);
    // auto sv = std::vector<Counter::Obj>(100);
    INFO(counts.printCounts("after 100"));
    REQUIRE(Counter::staticDefaultCtor == 100);
}

TEST_CASE("ctor_iterators") {
    auto str = std::string_view("hello world!");
    auto sv = ankerl::svector<char, 7>(str.begin(), str.end());
    REQUIRE(sv.size() == str.size());
    for (size_t i = 0; i < sv.size(); ++i) {
        REQUIRE(sv[i] == str[i]);
    }
}

TEST_CASE("ctor_not_random_access_iterator") {
    auto l = std::forward_list<char>();
    auto str = std::string_view("hello world!");
    for (auto it = str.rbegin(); it != str.rend(); ++it) {
        l.push_front(*it);
    }
    auto sv = ankerl::svector<char, 7>(l.begin(), l.end());
    REQUIRE(sv.size() == str.size());
    for (size_t i = 0; i < sv.size(); ++i) {
        REQUIRE(sv[i] == str[i]);
    }
}

TEST_CASE("ctor_copy") {
    auto sv = ankerl::svector<char, 7>();
    for (char c = 'a'; c <= 'z'; ++c) {
        sv.push_back(c);
    }
    REQUIRE(sv.size() == 26);

    auto svCpy(sv);
    REQUIRE(sv.size() == svCpy.size());
    for (size_t i = 0; i < sv.size(); ++i) {
        REQUIRE(sv[i] == svCpy[i]);
        REQUIRE(&sv[i] != &svCpy[i]);
    }

    sv.clear();
    REQUIRE(svCpy.size() == 26);
    auto svCpy2(sv);
    REQUIRE(svCpy2.size() == 0);
    REQUIRE(sv.size() == 0);
    REQUIRE(svCpy2.capacity() == decltype(sv){}.capacity());
}

TEST_CASE("ctor_move") {
    auto counts = Counter();
    INFO(counts.printCounts("begin"));
    auto sv = ankerl::svector<Counter::Obj, 3>();
    for (size_t i = 0; i < 100; ++i) {
        sv.emplace_back(i, counts);
    }
    auto total_before = counts.total();
    INFO(counts.printCounts("before move"));
    auto sv2(std::move(sv));
    auto total_after = counts.total();
    INFO(counts.printCounts("after move"));
    REQUIRE(total_before == total_after);

    // I guarantee that a moved-from value is in the default constructed state. It just makes everything easier.
    REQUIRE(sv.empty()); // NOLINT(hicpp-invalid-access-moved)
    REQUIRE(sv2.size() == 100);
}

TEST_CASE("ctor_move_direct") {}

} // namespace
