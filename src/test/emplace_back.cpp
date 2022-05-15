#include <ankerl/svector.h>

#include <doctest.h>

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

