#include <ankerl/svector.h>

#include <doctest.h>

#include <string>

TEST_CASE("push_back") {
    auto sv = ankerl::svector<uint32_t, 5>();
    for (uint32_t i = 0; i < 1000; ++i) {
        REQUIRE(sv.size() == i);
        sv.push_back(i);
        REQUIRE(sv.size() == i + 1);
        REQUIRE(sv.front() == 0);
        REQUIRE(sv.back() == i);

        for (uint32_t j = 0; j < i; ++j) {
            REQUIRE(sv[j] == j);
        }
    }

    uint32_t i = 0;
    for (auto const& val : sv) {
        REQUIRE(val == i);
        ++i;
    }
}

// tests if objects are properly moved
TEST_CASE("push_back_and_iterating") {
    auto sv = ankerl::svector<std::string, 7>();
    for (auto i = 0; i < 50; ++i) {
        sv.push_back(std::to_string(i));
    }

    auto i = 0;
    for (auto const& str : sv) {
        REQUIRE(str == std::to_string(i));
        ++i;
    }
}