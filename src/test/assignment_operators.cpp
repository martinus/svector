#include <ankerl/svector.h>

#include <app/Counter.h>

#include <doctest.h>

TEST_CASE("copy_assignment") {
    auto counts = Counter();
    INFO(counts);
    auto sv = ankerl::svector<Counter::Obj, 3>();
    sv.emplace_back(1, counts);
    sv.emplace_back(2, counts);

    auto sv2 = ankerl::svector<Counter::Obj, 3>();
    sv2.emplace_back(3, counts);

    REQUIRE(counts.copyCtor == 0);
    REQUIRE(counts.dtor == 0);
    counts("created both");
    sv2 = sv;
    counts("after sv2 = sv");

    REQUIRE(sv.size() == 2);
    REQUIRE(sv[0].get() == 1);
    REQUIRE(sv[1].get() == 2);

    REQUIRE(sv2.size() == 2);
    REQUIRE(sv2[0].get() == 1);
    REQUIRE(sv2[1].get() == 2);

    REQUIRE(counts.dtor == 1);
    REQUIRE(counts.copyCtor == 2);
}

TEST_CASE("copy_assignment_self") {
    auto counts = Counter();
    INFO(counts);
    auto sv = ankerl::svector<Counter::Obj, 3>();
    sv.emplace_back(1, counts);
    sv.emplace_back(2, counts);

    // selfassignment
    counts("before selfassignment");
    auto& sv2 = sv;
    sv2 = sv;
    counts("after selfassignment");

    REQUIRE(counts.copyCtor == 0);
    REQUIRE(counts.dtor == 0);
}
