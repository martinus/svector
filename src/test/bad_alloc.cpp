#include <ankerl/svector.h>

#include <valgrind/valgrind.h>

#include <doctest.h>

#include <limits>
#include <stdexcept>
#include <vector>

#define SANITIZER_ACTIVE 0

#if defined(__has_feature)
#    if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#        undef SANITIZER_ACTIVE
#        define SANITIZER_ACTIVE 1
#    endif
#endif

TEST_CASE("reserve_bad_alloc") {
    if (RUNNING_ON_VALGRIND || SANITIZER_ACTIVE) {
        // this test doesn't work with valgrind or some sanitizers.
        return;
    }
    auto sv = ankerl::svector<std::string, 3>();
    auto m = sv.max_size();
    REQUIRE(m == std::numeric_limits<std::ptrdiff_t>::max());
    REQUIRE_THROWS_AS(sv.reserve(sv.max_size()), std::bad_alloc);
}
