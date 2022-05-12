#include <ankerl/svector.h>

#include <doctest.h>
#include <fmt/format.h>

static_assert(sizeof(ankerl::svector<char, 7>) == 8);

TEST_CASE("simple") {
    auto sv = ankerl::svector<char, 7>();
    REQUIRE(sv.size() == 0);
    REQUIRE(sv.capacity() == 7);

    sv.push_back('h');
    REQUIRE(sv.size() == 1);
}
