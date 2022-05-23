#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>
#include <stdexcept>

TEST_CASE("resize") {
    Counter counts;
    auto a = ankerl::svector<Counter::Obj, 3>();
    // TODO check resize_after_reserve std::destroy
}
