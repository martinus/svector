#include <ankerl/svector.h>

#include <app/Counter.h>

#include <doctest.h>

TEST_CASE("n_eq_0") {
    auto counts = Counter();
    INFO(counts);

    auto sv = ankerl::svector<Counter::Obj, 0>();
    REQUIRE(sv.capacity() == 0);

    sv.emplace_back(123, counts);
    REQUIRE(sv.capacity() == 1);
    sv.emplace_back(124, counts);
    REQUIRE(sv.capacity() == 2);

    sv.emplace_back(125, counts);
    REQUIRE(sv.capacity() == 4);
    REQUIRE(sv.size() == 3);

    sv.clear();
    REQUIRE(sv.empty());
    REQUIRE(sv.capacity() == 4);

    sv.shrink_to_fit();
    REQUIRE(sv.capacity() == 0);
}
