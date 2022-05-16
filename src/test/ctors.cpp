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
    INFO(counts);
    counts("begin");
    auto o = Counter::Obj(123, counts);
    counts("one o");

    // creates a vector with copies, no allocation yet
    REQUIRE(counts.ctor == 1);
    auto sv = ankerl::svector<Counter::Obj, 7>(7, o);
    counts("ctor with 7");
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
    INFO(counts);
    REQUIRE(counts.staticDefaultCtor == 0);
    counts("begin");
    //  no copies are made, just default ctor
    auto sv = ankerl::svector<Counter::Obj, 3>(100);
    // auto sv = std::vector<Counter::Obj>(100);
    counts("after 100");
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
    INFO(counts);
    counts("begin");
    auto sv = ankerl::svector<Counter::Obj, 3>();
    for (size_t i = 0; i < 100; ++i) {
        sv.emplace_back(i, counts);
    }
    auto total_before = counts.total();
    counts("before move");
    auto sv2(std::move(sv));
    auto total_after = counts.total();
    counts("after move");
    REQUIRE(total_before == total_after);

    // I guarantee that a moved-from value is in the default constructed state. It just makes everything easier.
    REQUIRE(sv.empty()); // NOLINT(hicpp-invalid-access-moved)
    REQUIRE(sv2.size() == 100);
}

TEST_CASE("ctor_move_direct") {
    auto counts = Counter();
    INFO(counts);
    counts("begin");
    auto sv = ankerl::svector<Counter::Obj, 3>();
    sv.emplace_back(1, counts);
    sv.emplace_back(2, counts);
    sv.emplace_back(3, counts);
    counts("3 emplace_back");
    REQUIRE(counts.ctor == 3);
    REQUIRE(counts.dtor == 0);
    REQUIRE(counts.moveCtor == 0);

    auto svMv = ankerl::svector<Counter::Obj, 3>(std::move(sv));
    counts("after moved");
    REQUIRE(svMv.size() == 3);
    REQUIRE(svMv.capacity() == 3);
    REQUIRE(counts.ctor == 3);
    REQUIRE(counts.moveCtor == 3);
    REQUIRE(counts.dtor == 3);

    REQUIRE(svMv[0].get() == 1);
    REQUIRE(svMv[1].get() == 2);
    REQUIRE(svMv[2].get() == 3);
    REQUIRE(sv.empty()); // NOLINT(hicpp-invalid-access-moved)
}

TEST_CASE("ctor_move_empty_capacity") {
    auto sv = ankerl::svector<char, 7>(1000, 'x');
    auto c = sv.capacity();
    sv.clear();
    REQUIRE(sv.capacity() == c);
    REQUIRE(sv.empty());

    auto sv2 = ankerl::svector<char, 7>(std::move(sv));
    REQUIRE(sv.capacity() == 7); // NOLINT(hicpp-invalid-access-moved)
    REQUIRE(sv.empty());
    REQUIRE(sv2.capacity() == c);
    REQUIRE(sv2.size() == 0);
}

TEST_CASE("ctor_initializer_list") {
    auto sv = ankerl::svector<char, 5>{{'h', 'e', 'l', 'l', 'o'}};
    REQUIRE(std::string(sv.begin(), sv.end()) == "hello");

    auto sv2 = ankerl::svector<uint64_t, 7>{std::initializer_list<uint64_t>{1, 2, 3, 4, 5, 6, 7, 99999999}};
    REQUIRE(sv2.size() == 8);
    REQUIRE(sv2.back() == 99999999);
    REQUIRE(sv2.front() == 1);
}

} // namespace
