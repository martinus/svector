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

TEST_CASE("move_assignment") {
    auto counts = Counter();
    INFO(counts);
    auto a = ankerl::svector<Counter::Obj, 3>();
    a.emplace_back(1, counts);
    a.emplace_back(2, counts);

    auto b = ankerl::svector<Counter::Obj, 3>();
    b.emplace_back(3, counts);
    b.emplace_back(4, counts);
    b.emplace_back(5, counts);

    REQUIRE(counts.moveAssign == 0);
    REQUIRE(counts.dtor == 0);
    counts("before move");
    a = std::move(b);
    counts("after move");
    REQUIRE(counts.moveCtor == 3);
    REQUIRE(counts.dtor == 2 + 3);

    // additionally set moved-from to be clear
    REQUIRE(b.empty()); // NOLINT(hicpp-invalid-access-moved)
    REQUIRE(a.size() == 3);
    REQUIRE(a[0].get() == 3);
    REQUIRE(a[1].get() == 4);
    REQUIRE(a[2].get() == 5);

    auto total_before = counts.total();
    auto* ptr = &a;
    *ptr = std::move(a);
    auto total_after = counts.total();
    REQUIRE(total_after == total_before);
}

TEST_CASE("move_assignment_largeboth") {
    auto counts = Counter();
    INFO(counts);
    {
        auto a = ankerl::svector<Counter::Obj, 3>();
        auto b = ankerl::svector<Counter::Obj, 3>();
        for (size_t i = 0; i < 100; ++i) {
            a.emplace_back(i, counts);
            b.emplace_back(1000 + i, counts);
        }

        auto const* ptr = b.data();

        counts("after_creation");
        auto dtor_before = counts.dtor;
        a = std::move(b);
        counts("after_creation");
        REQUIRE(a.size() == 100);
        REQUIRE(counts.dtor == dtor_before + 100);
        for (size_t i = 0; i < 100; ++i) {
            REQUIRE(a[i].get() == 1000 + i);
        }

        auto c = ankerl::svector<Counter::Obj, 3>();
        counts("before emplace_back");
        c.emplace_back(9999999, counts);
        counts("after emplace_back");

        c = std::move(a);
        counts("after c = std::move(a)");
        REQUIRE(a.empty()); // NOLINT(hicpp-invalid-access-moved)
        REQUIRE(c.size() == 100);
        for (size_t i = 0; i < 100; ++i) {
            REQUIRE(c[i].get() == 1000 + i);
        }

        // make sure its' actually the same memory
        REQUIRE(c.data() == ptr);
        counts("before dtor all");
    }
    counts("finished");
    REQUIRE(counts.ctor == 201);
    REQUIRE(counts.ctor + counts.moveCtor == counts.dtor);
}
