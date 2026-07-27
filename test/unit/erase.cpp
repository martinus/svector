#include <ankerl/svector.h>

#include <app/Counter.h>
#include <app/VecTester.h>

#include <doctest.h>

#include <string>
#include <vector>

TEST_CASE("erase_single") {
    auto counts = Counter();
    INFO(counts);
    auto sv = ankerl::svector<Counter::Obj, 3>();
    for (size_t i = 0; i < 100; ++i) {
        sv.emplace_back(i, counts);
    }

    REQUIRE(sv.end() == sv.erase(sv.cend()));
    REQUIRE(sv.size() == 100);

    auto* it = sv.erase(sv.begin());
    REQUIRE(it->get() == 1);
    REQUIRE(sv.size() == 99);
    for (size_t i = 0; i < sv.size(); ++i) {
        REQUIRE(sv[i].get() == i + 1);
    }
}

TEST_CASE("erase_range") {
    auto counts = Counter();
    INFO(counts);
    auto sv = ankerl::svector<Counter::Obj, 3>();
    sv.erase(sv.begin(), sv.end());
    REQUIRE(sv.empty());

    for (size_t i = 0; i < 50; ++i) {
        sv.emplace_back(i, counts);
    }
    auto* it = sv.erase(sv.begin() + 20, sv.end());
    REQUIRE(it == sv.end());
    REQUIRE(sv.size() == 20);
    for (size_t i = 0; i < sv.size(); ++i) {
        REQUIRE(sv[i].get() == i);
    }

    sv.erase(sv.begin() + 5, sv.begin() + 10);
    REQUIRE(sv.size() == 15);
    for (size_t i = 0; i < sv.size(); ++i) {
        auto n = i < 5 ? i : i + 5;
        REQUIRE(sv[i].get() == n);
    }

    // erase full range
    sv.erase(sv.begin(), sv.end());
    REQUIRE(sv.empty());

    // push a few elements
    for (size_t i = 0; i < 10; ++i) {
        sv.emplace_back(i, counts);
    }

    // remove all but the last one
    sv.erase(sv.begin(), sv.end() - 1);
    REQUIRE(sv.size() == 1);
    REQUIRE(sv.front().get() == 9);
}

// Erasing an empty range has to leave the container exactly as it was. It used to move the tail
// onto itself, self-move-assigning every element from first onwards and blanking them. See #66.
TEST_CASE("erase_nothing_keeps_the_elements") {
    // 3 stays inline, 50 is indirect
    for (auto const count : {size_t{3}, size_t{50}}) {
        auto const source = make_long_strings(count);
        for (auto const pos : {size_t{0}, count / 2, count}) {
            auto sv = ankerl::svector<std::string, 4>(source.begin(), source.end());
            auto vec = source;

            auto* it = sv.erase(sv.cbegin() + pos, sv.cbegin() + pos);
            vec.erase(vec.cbegin() + pos, vec.cbegin() + pos);
            REQUIRE(it == sv.begin() + pos);
            assert_eq(vec, sv);
        }
    }
}
