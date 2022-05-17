#include <ankerl/svector.h>

#include <doctest.h>
#include <stdexcept>

TEST_CASE("at") {
    auto sv = ankerl::svector<std::string, 3>();
    auto const& svConst = sv;
    REQUIRE_THROWS_AS(sv.at(0), std::out_of_range);      // NOLINT(llvm-else-after-return,readability-else-after-return)
    REQUIRE_THROWS_AS(svConst.at(0), std::out_of_range); // NOLINT(llvm-else-after-return,readability-else-after-return)

    sv.push_back("hello");
    REQUIRE(sv.at(0) == "hello");
    REQUIRE(svConst.at(0) == "hello");
    REQUIRE_THROWS_AS(sv.at(1), std::out_of_range);      // NOLINT(llvm-else-after-return,readability-else-after-return)
    REQUIRE_THROWS_AS(svConst.at(1), std::out_of_range); // NOLINT(llvm-else-after-return,readability-else-after-return)

    sv.resize(100);
    REQUIRE(sv.size() == 100);
    REQUIRE(sv.at(99) == "");
    REQUIRE(svConst.at(99) == "");
    REQUIRE_THROWS_AS(sv.at(100), std::out_of_range);      // NOLINT(llvm-else-after-return,readability-else-after-return)
    REQUIRE_THROWS_AS(svConst.at(100), std::out_of_range); // NOLINT(llvm-else-after-return,readability-else-after-return)
}
