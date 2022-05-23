#include <ankerl/svector.h>
#include <app/Counter.h>
#include <app/VecTester.h>

#include <doctest.h>

#include <vector>

TEST_CASE("emplace") {
    auto counts = Counter();
    INFO(counts);

    auto a = ankerl::svector<Counter::Obj, 3>();

    a.emplace(a.cend(), 123, counts);
    REQUIRE(a.size() == 1);
    REQUIRE(a[0].get() == 123);
}

TEST_CASE("emplace_checked") {
    auto counts = Counter();

    for (size_t s = 0; s < 6; ++s) {
        auto vc = VecTester<Counter::Obj, 4>();
        for (size_t ins = 0; ins < s; ++ins) {
            vc.emplace_back(ins, counts);
        }

        for (size_t i = 0; i <= vc.size(); ++i) {
            auto x = vc;
            x.emplace_at(i, 999, counts);
        }
    }
}
