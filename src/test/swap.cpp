#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>

#include <vector>

template <typename V>
void test_swap() {
    Counter counts;
    INFO(counts);

    auto a = V();
    auto b = V();

    for (size_t i = 0; i < 3; ++i) {
        a.emplace_back(i, counts);
    }
    REQUIRE(counts.ctor == 3);
    counts("before swap");
    a.swap(b);
    counts("after swap");
    REQUIRE(counts.ctor == 3);
    REQUIRE(a.empty());
    REQUIRE(b.size() == 3);

    for (size_t i = 0; i < 100; ++i) {
        a.emplace_back(111 + i, counts);
    }
    REQUIRE(a.size() == 100);
    REQUIRE(b.size() == 3);
    a.swap(b);
    REQUIRE(a.size() == 3);
    REQUIRE(b.size() == 100);
    REQUIRE(a[0].get() == 0);
    REQUIRE(b.front().get() == 111);

    for (size_t i = 0; i < 100; ++i) {
        a.emplace_back(999 + i, counts);
    }
    auto total_before = counts.total();
    REQUIRE(a.size() == 103);
    REQUIRE(b.size() == 100);

    b.swap(a);
    REQUIRE(counts.total() == total_before);
    REQUIRE(a.size() == 100);
    REQUIRE(b.size() == 103);
    REQUIRE(a.back().get() == 111 + 99);
    REQUIRE(b.back().get() == 999 + 99);

    total_before = counts.total();
    std::swap(a, b);
    REQUIRE(counts.total() == total_before);
    REQUIRE(b.size() == 100);
    REQUIRE(a.size() == 103);
    REQUIRE(b.back().get() == 111 + 99);
    REQUIRE(a.back().get() == 999 + 99);
}

TEST_CASE("swap_stdvector") {
    test_swap<std::vector<Counter::Obj>>();
}

TEST_CASE("swap_svector") {
    test_swap<ankerl::svector<Counter::Obj, 2>>();
}
