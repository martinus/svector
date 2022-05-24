#include <ankerl/svector.h>
#include <app/Counter.h>
#include <app/VecTester.h>

#include <doctest.h>

TEST_CASE("pop_back") {
    Counter counts;
    INFO(counts);

    auto x = size_t();
    for (size_t i = 0; i < 10; ++i) {
        auto a = ankerl::svector<Counter::Obj, 3>();
        auto b = std::vector<Counter::Obj>();
        for (size_t j = 0; j < i; ++j) {
            a.emplace_back(x, counts);
            b.emplace_back(x, counts);
            assert_eq(a, b);
            ++x;
        }
        for (size_t j = 0; j < i; ++j) {
            assert_eq(a, b);
            a.pop_back();
            b.pop_back();
            assert_eq(a, b);
        }
        assert_eq(a, b);
        REQUIRE(a.size() == 0);
        REQUIRE(a.empty());
    }
}

TEST_CASE("pop_back_trivial") {
    auto x = size_t();
    for (size_t i = 0; i < 10; ++i) {
        auto a = ankerl::svector<size_t, 3>();
        auto b = std::vector<size_t>();
        for (size_t j = 0; j < i; ++j) {
            a.emplace_back(x);
            b.emplace_back(x);
            assert_eq(a, b);
            ++x;
        }
        for (size_t j = 0; j < i; ++j) {
            assert_eq(a, b);
            a.pop_back();
            b.pop_back();
            assert_eq(a, b);
        }
        assert_eq(a, b);
        REQUIRE(a.size() == 0);
        REQUIRE(a.empty());
    }
}
