#include <ankerl/svector.h>

#include <doctest.h>
#include <fmt/format.h>

static_assert(sizeof(ankerl::svector<char, 7>) == 8);

TEST_CASE("val") {
    fmt::print("Hello!\n");
}
