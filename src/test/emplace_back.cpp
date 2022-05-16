#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>
#include <fmt/format.h>

#include <string>

TEST_CASE("emplace_back") {
    auto sv = ankerl::svector<std::string, 3>();
    sv.emplace_back("hello");
    REQUIRE(sv.size() == 1);
    REQUIRE(sv.front() == "hello");

    sv.clear();
    REQUIRE(sv.empty());
    REQUIRE(sv.size() == 0);
}

TEST_CASE("emplace_back_counts") {
    Counter counts;
    INFO(counts);
    {
        auto sv = ankerl::svector<Counter::Obj, 5>();

        REQUIRE(sv.capacity() == 5);
        counts.printCounts("begin");
        REQUIRE(counts.ctor == 0);

        for (size_t i = 0; i < 100; ++i) {
            sv.emplace_back(i, counts);
            counts.printCounts("after emplace");
            REQUIRE(counts.ctor == i + 1);
            REQUIRE(counts.dtor == counts.moveCtor);
        }
    }
    counts.printCounts("dtor");
    REQUIRE(counts.dtor == counts.ctor + counts.moveCtor);
}