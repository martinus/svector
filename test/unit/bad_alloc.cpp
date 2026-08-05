#include <ankerl/svector.h>

// make sure all this works even when valgrind is not installed
#if __has_include(<valgrind/valgrind.h>)
#    include <valgrind/valgrind.h>
#else
#    ifndef RUNNING_ON_VALGRIND
#        define RUNNING_ON_VALGRIND 0
#    endif
#endif

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

// g++ only gained __has_feature in version 14, so check its own macros too. Without this the
// test below runs under -fsanitize=address and asan aborts on the huge allocation instead of
// letting it throw std::bad_alloc.
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#    undef SANITIZER_ACTIVE
#    define SANITIZER_ACTIVE 1
#endif

// gcc constant folds a size derived from max_size() and then reasons about an array of that many
// elements, which at -O2 with the hardening flags is an -Warray-bounds error and, on gcc 13, an
// internal compiler error. The size is a runtime value in any real use, so make it one here too.
namespace {
auto opaque(size_t value) -> size_t {
    volatile size_t hidden = value;
    return hidden;
}
} // namespace

TEST_CASE("reserve_bad_alloc") {
    if constexpr (RUNNING_ON_VALGRIND || SANITIZER_ACTIVE) {
        // this test doesn't work with valgrind or some sanitizers.
    } else {
        auto sv = ankerl::svector<std::string, 3>();
        auto m = sv.max_size();

        // A count, not a byte count: the ceiling is what alloc() will let through, and it refuses
        // anything whose bytes pass PTRDIFF_MAX. std::vector answers the same.
        REQUIRE(m == static_cast<size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(std::string));

        // max_size() itself is a legal size, so this is a real allocation failure rather than a
        // size error, and stays bad_alloc. One past it is the size error, see reserve_length_error.
        REQUIRE_THROWS_AS(sv.reserve(opaque(sv.max_size())), std::bad_alloc);
    }
}

// Anything past max_size() is a size error rather than a failure to find memory, which is what
// std::vector throws and what this used to get wrong by answering bad_alloc for both.
TEST_CASE("reserve_length_error") {
    auto sv = ankerl::svector<std::string, 3>();
    REQUIRE_THROWS_AS(sv.reserve(opaque(sv.max_size() + 1)), std::length_error);
    REQUIRE_THROWS_AS(sv.resize(opaque(sv.max_size() + 1)), std::length_error);
    REQUIRE_THROWS_AS(sv.resize(opaque(sv.max_size() + 1), "x"), std::length_error);

    // and it is still usable afterwards
    REQUIRE(sv.empty());
    sv.push_back("a");
    REQUIRE(sv.size() == 1);
}

// A count just under max_size() is a legal size, so it goes all the way to the allocator rather
// than being refused. What is being checked is that the doubling on the way there does not wrap
// and hand the allocation something small: it is clamped to max_size(), and that is what fails.
TEST_CASE("growth_below_max_size_reaches_the_allocator") {
    auto sv = ankerl::svector<uint8_t, 7>();
    REQUIRE(sv.max_size() == static_cast<size_t>(std::numeric_limits<std::ptrdiff_t>::max()));
    REQUIRE_THROWS_AS(sv.reserve(opaque(sv.max_size() - 1)), std::bad_alloc);
    REQUIRE(sv.empty());
}
