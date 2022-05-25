#include <ankerl/svector.h>

#include <doctest.h>
#include <stdexcept>

static_assert(ankerl::detail::round_up(1, 8) == 8);
static_assert(ankerl::detail::round_up(7, 8) == 8);
static_assert(ankerl::detail::round_up(8, 8) == 8);
static_assert(ankerl::detail::round_up(9, 8) == 16);
static_assert(ankerl::detail::round_up(15, 8) == 16);
static_assert(ankerl::detail::round_up(16, 8) == 16);
static_assert(ankerl::detail::round_up(17, 8) == 24);


static_assert(ankerl::detail::round_up(1, 3) == 3);
static_assert(ankerl::detail::round_up(1234, 3) == 1236);
static_assert(ankerl::detail::round_up(1236, 3) == 1236);
static_assert(ankerl::detail::round_up(1236, 3) == 1236);
