#include <ankerl/svector.h>

#include <valgrind/valgrind.h>

#include <doctest.h>
#include <limits>
#include <stdexcept>

TEST_CASE("reserve_bad_al") {
    if (RUNNING_ON_VALGRIND) {
        // this test doesn't work with valgrind:
        //
        // new/new[] failed and should throw an exception, but Valgrind
        // cannot throw exceptions and so is aborting instead.  Sorry.doesn't work
        return;
    }
    auto sv = ankerl::svector<std::string, 3>();
    auto m = sv.max_size();
    REQUIRE(m == std::numeric_limits<size_t>::max());
    REQUIRE_THROWS_AS(sv.reserve(m), std::bad_alloc);
}
