#include <ankerl/svector.h>

#include <doctest.h>
#include <limits>
#include <stdexcept>

TEST_CASE("reserve_bad_al") {
    auto sv = ankerl::svector<std::string, 3>();
    auto m = sv.max_size();
    REQUIRE(m == std::numeric_limits<size_t>::max());
    REQUIRE_THROWS_AS(sv.reserve(m), std::bad_alloc);
}
