#include <ankerl/svector.h>

#include <doctest.h>
#include <fmt/format.h>

static_assert(sizeof(ankerl::svector<char, 7>) == 8);
static_assert(sizeof(ankerl::svector<uint32_t, 5>) == 24);

TEST_CASE("simple") {
    auto sv = ankerl::svector<char, 7>();
    REQUIRE(sv.size() == 0);
    REQUIRE(sv.capacity() == 7);

    sv.push_back('h');
    REQUIRE(sv.size() == 1);
}

TEST_CASE("push_back") {
    auto sv = ankerl::svector<uint32_t, 5>();
    for (uint32_t i = 0; i < 1000; ++i) {
        REQUIRE(sv.size() == i);
        sv.push_back(i);
        REQUIRE(sv.size() == i + 1);

        for (uint32_t j = 0; j < i; ++j) {
            REQUIRE(sv[j] == j);
        }
    }
}