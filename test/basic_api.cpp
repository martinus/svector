#include <ankerl/svector.h>

#include <chrono>
#include <doctest.h>
#include <fmt/format.h>

#include <vector>

using namespace std::literals;

static_assert(sizeof(ankerl::svector<char, 7>) == 8);
static_assert(sizeof(ankerl::svector<uint32_t, 5>) == 24);

TEST_CASE("simple") {
    auto sv = ankerl::svector<char, 7>();
    REQUIRE(sv.size() == 0);
    REQUIRE(sv.capacity() == 7);

    sv.push_back('h');
    REQUIRE(sv.size() == 1);
}
