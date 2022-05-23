#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>
#include <stdexcept>

TEST_CASE("reserve") {
    Counter counts;
    auto a = ankerl::svector<Counter::Obj, 3>();
    REQUIRE(a.capacity() < 100);
    a.reserve(100);
    REQUIRE(a.capacity() == 3 << 6); // 192
    a.reserve(192);
    REQUIRE(a.capacity() == 3 << 6); // 192
    a.reserve(193);
    REQUIRE(a.capacity() == 3 << 7); // 384

    // going down does nothing
    a.reserve(0);
    REQUIRE(a.capacity() == 3 << 7);
    a.push_back(Counter::Obj(123, counts));
    REQUIRE(a.size() == 1);
    a.reserve(1);
    REQUIRE(a.size() == 1);
    REQUIRE(a.capacity() == 3 << 7);

    // reserve even more, with data
    a.reserve(385);
    REQUIRE(a.capacity() == 3 << 8);
    REQUIRE(a.size() == 1);
    REQUIRE(a[0].get() == 123);
}
