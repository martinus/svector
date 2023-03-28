#include <ankerl/svector.h>

#include <app/Counter.h>
#include <app/print.h>

#include <doctest.h>

#include <numeric>
#include <ostream>
#include <stdexcept>
#include <string_view>

TEST_CASE("std_erase") {
    auto sv = ankerl::svector<char, 7>(10);
    std::iota(sv.begin(), sv.end(), '0');
    REQUIRE("0123456789" == std::string_view(sv.data(), sv.size()));

    REQUIRE(1 == std::erase(sv, '3'));
    REQUIRE("012456789" == std::string_view(sv.data(), sv.size()));

    REQUIRE(0 == std::erase(sv, '3'));
    REQUIRE("012456789" == std::string_view(sv.data(), sv.size()));

    sv.insert(sv.begin(), 'a');
    sv.insert(sv.begin() + sv.size() / 2, 'a');
    sv.insert(sv.end(), 'a');
    REQUIRE("a0124a56789a" == std::string_view(sv.data(), sv.size()));

    REQUIRE(3 == std::erase(sv, 'a'));
    REQUIRE("012456789" == std::string_view(sv.data(), sv.size()));

    auto num_erased = std::erase_if(sv, [](char x) {
        return (x - '0') % 2 == 0;
    });
    REQUIRE(num_erased == 5);
    REQUIRE("1579" == std::string_view(sv.data(), sv.size()));
}
