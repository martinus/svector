#include <ankerl/svector.h>

#include <app/Counter.h>

#include <doctest.h>

#include <string>
#include <vector>

// The whole file is C++23 library only. A C++17 or C++20 build has neither std::from_range_t nor
// the range concepts these need, and svector deliberately does not grow the members there either.
#if ANKERL_SVECTOR_HAS_RANGES

#    include <forward_list>
#    include <list>
#    include <ranges>

namespace {

// A range whose sentinel is not its iterator and whose iterator publishes no iterator_category, so
// it cannot be handed to the iterator pair members and takes the materialising path instead.
auto even_numbers(int count) {
    return std::views::iota(0) | std::views::filter([](int i) {
               return i % 2 == 0;
           }) |
           std::views::take(count);
}

auto long_strings(size_t count) -> std::vector<std::string> {
    auto v = std::vector<std::string>();
    for (size_t i = 0; i < count; ++i) {
        v.push_back("element " + std::to_string(i) + ", long enough that it has to allocate for itself");
    }
    return v;
}

} // namespace

TEST_CASE("from_range_construction") {
    SUBCASE("common range, stays inline") {
        auto const source = std::vector<int>{1, 2, 3};
        auto v = ankerl::svector<int, 7>(std::from_range, source);
        REQUIRE(v.size() == 3);
        REQUIRE(std::equal(v.begin(), v.end(), source.begin(), source.end()));
    }

    SUBCASE("common range, has to allocate") {
        auto const source = long_strings(50);
        auto v = ankerl::svector<std::string, 3>(std::from_range, source);
        REQUIRE(v.size() == 50);
        REQUIRE(std::equal(v.begin(), v.end(), source.begin(), source.end()));
    }

    SUBCASE("range with a sentinel and no iterator_category") {
        auto v = ankerl::svector<int, 2>(std::from_range, even_numbers(6));
        REQUIRE(v.size() == 6);
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{0, 2, 4, 6, 8, 10}.begin()));
    }

    SUBCASE("input only range") {
        auto source = std::forward_list<int>{4, 5, 6};
        auto v = ankerl::svector<int, 7>(std::from_range, source);
        REQUIRE(v.size() == 3);
        REQUIRE(v[0] == 4);
        REQUIRE(v[2] == 6);
    }

    SUBCASE("with a named allocator") {
        auto const source = std::vector<int>{1, 2, 3};
        auto v = ankerl::svector<int, 7>(std::from_range, source, std::allocator<int>{});
        REQUIRE(v.size() == 3);
        REQUIRE(v[1] == 2);
    }

    SUBCASE("empty range") {
        auto v = ankerl::svector<int, 7>(std::from_range, std::vector<int>{});
        REQUIRE(v.empty());
    }
}

TEST_CASE("append_range") {
    SUBCASE("onto empty and then across the inline boundary") {
        auto v = ankerl::svector<int, 4>();
        v.append_range(std::vector<int>{1, 2});
        REQUIRE(v.size() == 2);

        v.append_range(std::list<int>{3, 4, 5, 6, 7});
        REQUIRE(v.size() == 7);
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{1, 2, 3, 4, 5, 6, 7}.begin()));
    }

    SUBCASE("a sentinel range appends too") {
        auto v = ankerl::svector<int, 2>{100};
        v.append_range(even_numbers(3));
        REQUIRE(v.size() == 4);
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{100, 0, 2, 4}.begin()));
    }

    SUBCASE("appending nothing changes nothing") {
        auto v = ankerl::svector<int, 4>{1, 2, 3};
        v.append_range(std::vector<int>{});
        REQUIRE(v.size() == 3);
        REQUIRE(v[2] == 3);
    }

    SUBCASE("strings, so construction and destruction are observable") {
        auto counts = Counter();
        {
            auto v = ankerl::svector<std::string, 2>();
            v.append_range(long_strings(30));
            REQUIRE(v.size() == 30);
            REQUIRE(v[29] == long_strings(30)[29]);
        }
        static_cast<void>(counts);
    }
}

TEST_CASE("insert_range") {
    auto const source = std::vector<int>{7, 8};

    SUBCASE("at the front") {
        auto v = ankerl::svector<int, 8>{1, 2, 3};
        auto it = v.insert_range(v.begin(), source);
        REQUIRE(*it == 7);
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{7, 8, 1, 2, 3}.begin()));
    }

    SUBCASE("in the middle") {
        auto v = ankerl::svector<int, 8>{1, 2, 3};
        v.insert_range(v.begin() + 1, source);
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{1, 7, 8, 2, 3}.begin()));
    }

    SUBCASE("at the end") {
        auto v = ankerl::svector<int, 8>{1, 2, 3};
        v.insert_range(v.end(), source);
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{1, 2, 3, 7, 8}.begin()));
    }

    SUBCASE("forcing a reallocation") {
        auto v = ankerl::svector<int, 2>{1, 2};
        v.insert_range(v.begin() + 1, std::vector<int>{3, 4, 5, 6, 7, 8});
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{1, 3, 4, 5, 6, 7, 8, 2}.begin()));
    }

    SUBCASE("a sentinel range in the middle") {
        auto v = ankerl::svector<int, 8>{1, 2, 3};
        v.insert_range(v.begin() + 1, even_numbers(2));
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{1, 0, 2, 2, 3}.begin()));
    }

    SUBCASE("inserting nothing is not an erase") {
        auto v = ankerl::svector<int, 8>{1, 2, 3};
        v.insert_range(v.begin() + 1, std::vector<int>{});
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{1, 2, 3}.begin()));
    }
}

TEST_CASE("assign_range") {
    SUBCASE("replaces, growing past inline") {
        auto v = ankerl::svector<int, 4>{1, 2};
        v.assign_range(std::vector<int>{5, 6, 7, 8, 9, 10});
        REQUIRE(v.size() == 6);
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{5, 6, 7, 8, 9, 10}.begin()));
    }

    SUBCASE("replaces, shrinking") {
        auto v = ankerl::svector<int, 2>();
        v.append_range(std::vector<int>(50, 3));
        v.assign_range(std::vector<int>{1});
        REQUIRE(v.size() == 1);
        REQUIRE(v[0] == 1);
    }

    SUBCASE("assigning an empty range clears") {
        auto v = ankerl::svector<int, 4>{1, 2, 3};
        v.assign_range(std::vector<int>{});
        REQUIRE(v.empty());
    }

    SUBCASE("a sentinel range assigns too") {
        auto v = ankerl::svector<int, 2>{1, 2, 3, 4, 5};
        v.assign_range(even_numbers(3));
        REQUIRE(v.size() == 3);
        REQUIRE(std::equal(v.begin(), v.end(), std::vector<int>{0, 2, 4}.begin()));
    }

    SUBCASE("strings") {
        auto v = ankerl::svector<std::string, 2>{"x"};
        v.assign_range(long_strings(20));
        REQUIRE(v.size() == 20);
        REQUIRE(v[0] == long_strings(20)[0]);
    }
}

// Whatever std::vector does with the same calls, svector does too.
TEST_CASE("range_members_agree_with_std_vector") {
    auto const source = long_strings(25);

    auto sv = ankerl::svector<std::string, 3>(std::from_range, source);
    auto v = std::vector<std::string>(std::from_range, source);
    REQUIRE(std::equal(sv.begin(), sv.end(), v.begin(), v.end()));

    sv.append_range(source);
    v.append_range(source);
    REQUIRE(std::equal(sv.begin(), sv.end(), v.begin(), v.end()));

    sv.insert_range(sv.begin() + 5, source);
    v.insert_range(v.begin() + 5, source);
    REQUIRE(std::equal(sv.begin(), sv.end(), v.begin(), v.end()));

    sv.assign_range(source);
    v.assign_range(source);
    REQUIRE(std::equal(sv.begin(), sv.end(), v.begin(), v.end()));
}

#endif
