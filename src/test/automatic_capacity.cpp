#include <ankerl/svector.h>

#include <doctest.h>
#include <stdexcept>

namespace {

template <typename T>
void assert_size_capacity(size_t size, size_t capacity) {
    REQUIRE(sizeof(T) == size);
    REQUIRE(T{}.capacity() == capacity);
}

static_assert(ankerl::detail::automatic_capacity<uint8_t>(0) == 7);
static_assert(ankerl::detail::automatic_capacity<uint8_t>(1) == 7);
static_assert(ankerl::detail::automatic_capacity<uint8_t>(2) == 7);
static_assert(ankerl::detail::automatic_capacity<uint8_t>(3) == 7);
static_assert(ankerl::detail::automatic_capacity<uint8_t>(7) == 7);
static_assert(ankerl::detail::automatic_capacity<uint8_t>(8) == 15);
static_assert(ankerl::detail::automatic_capacity<uint8_t>(15) == 15);
static_assert(ankerl::detail::automatic_capacity<uint8_t>(16) == 23);

TEST_CASE("automatic_capacity") {
    assert_size_capacity<ankerl::svector<uint8_t, 0>>(8, 7);
    assert_size_capacity<ankerl::svector<uint8_t, 1>>(8, 7);
    assert_size_capacity<ankerl::svector<uint8_t, 7>>(8, 7);
    assert_size_capacity<ankerl::svector<uint8_t, 8>>(16, 15);

    assert_size_capacity<ankerl::svector<uint16_t, 0>>(8, 3);
    assert_size_capacity<ankerl::svector<uint16_t, 1>>(8, 3);
    assert_size_capacity<ankerl::svector<uint16_t, 2>>(8, 3);
    assert_size_capacity<ankerl::svector<uint16_t, 3>>(8, 3);
    assert_size_capacity<ankerl::svector<uint16_t, 4>>(16, 4 + 3);
    assert_size_capacity<ankerl::svector<uint16_t, 5>>(16, 4 + 3);
    assert_size_capacity<ankerl::svector<uint16_t, 6>>(16, 4 + 3);
    assert_size_capacity<ankerl::svector<uint16_t, 7>>(16, 4 + 3);
    assert_size_capacity<ankerl::svector<uint16_t, 8>>(24, 8 + 3);
    assert_size_capacity<ankerl::svector<uint16_t, 127>>(256, 127);
}

template <size_t S>
using Foo = std::array<uint8_t, S>;

TEST_CASE("automatic_capacity_15") {
    assert_size_capacity<ankerl::svector<Foo<15>, 0>>(8, 0);
    assert_size_capacity<ankerl::svector<Foo<15>, 1>>(16, 1);
    assert_size_capacity<ankerl::svector<Foo<15>, 2>>(32, 2);

    assert_size_capacity<ankerl::svector<Foo<3>, 0>>(8, 2);
    assert_size_capacity<ankerl::svector<Foo<3>, 2>>(8, 2);
    assert_size_capacity<ankerl::svector<Foo<3>, 3>>(16, 5);
    assert_size_capacity<ankerl::svector<Foo<3>, 5>>(16, 5);
    assert_size_capacity<ankerl::svector<Foo<3>, 6>>(24, 7);
    assert_size_capacity<ankerl::svector<Foo<3>, 8>>(32, 10);
}

} // namespace
