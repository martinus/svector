#include <ankerl/svector.h>

#include <absl/container/inlined_vector.h>
#include <boost/container/small_vector.hpp>

#include <chrono>
#include <doctest.h>
#include <fmt/format.h>
#include <nanobench.h>

#include <vector>

using namespace std::literals;

static_assert(sizeof(ankerl::svector<char, 7>) == 8);
static_assert(sizeof(ankerl::svector<uint32_t, 5>) == 24);

TEST_CASE("simple") {
    auto sv = ankerl::svector<char, 7>();
    REQUIRE(sv.size() == 0);
    REQUIRE(sv.capacity() == 7);

    sv.push_back('h');
    REQUIRE(sv.size() == 1);
}

TEST_CASE("push_back") {
    auto sv = ankerl::svector<uint32_t, 5>();
    for (uint32_t i = 0; i < 1000; ++i) {
        REQUIRE(sv.size() == i);
        sv.push_back(i);
        REQUIRE(sv.size() == i + 1);
        REQUIRE(sv.front() == 0);
        REQUIRE(sv.back() == i);

        for (uint32_t j = 0; j < i; ++j) {
            REQUIRE(sv[j] == j);
        }
    }

    uint32_t i = 0;
    for (auto const& val : sv) {
        REQUIRE(val == i);
        ++i;
    }
}

// tests if objects are properly moved
TEST_CASE("iterating_string") {
    auto sv = ankerl::svector<std::string, 7>();
    for (auto i = 0; i < 50; ++i) {
        sv.push_back(std::to_string(i));
    }

    auto i = 0;
    for (auto const& str : sv) {
        REQUIRE(str == std::to_string(i));
        ++i;
    }
}

TEST_CASE("emplace_back") {
    auto sv = ankerl::svector<std::string, 3>();
    sv.emplace_back("hello");
    REQUIRE(sv.size() == 1);
    REQUIRE(sv.front() == "hello");

    sv.clear();
    REQUIRE(sv.empty());
    REQUIRE(sv.size() == 0);
}

template <typename Vec>
void benchPushBack(char const* title, size_t num_elements) {
    ankerl::nanobench::Bench().batch(num_elements).minEpochTime(100ms).run(title, [&] {
        auto rng = ankerl::nanobench::Rng(123);
        for (size_t i = 0; i < 1000; ++i) {
            auto vec = Vec();
            auto num_push = rng.bounded(31);
            for (size_t i = 0; i < num_push; ++i) {
                vec.push_back(i);
            }

            while (!vec.empty()) {
                vec.pop_back();
            }
        }
    });
}

template<typename T>
void show(char const* what) {
    fmt::print("{} sizeof({})\n", sizeof(T), what);
    fmt::print("{} capacity({})\n", T{}.capacity(), what);
    fmt::print("{} max_size({})\n\n", T{}.max_size(), what);
}

TEST_CASE("show_sizeof") {
    std::vector<char>().max_size();
    //static constexpr auto a = sizeof(absl::InlinedVector<uint8_t, 24>);
    //static constexpr auto s = sizeof(ankerl::svector<uint8_t, 31>);
    //static constexpr auto b = sizeof(boost::container::small_vector<uint8_t, 16>);

    show<std::vector<uint8_t>>("std::vector<uint8_t>");
    show<boost::container::small_vector<uint8_t, 47>>("boost::container::small_vector<uint8_t, 15>");
    show<absl::InlinedVector<uint8_t, 56>>("absl::InlinedVector<uint8_t, 24>");
    show<ankerl::svector<uint8_t, 1>>("ankerl::svector<uint8_t, 1>");
}

TEST_CASE("bench_push_back" * doctest::skip()) {
    benchPushBack<ankerl::svector<uint8_t, 7>>("svector<uint8_t, 31>", 1000);
    benchPushBack<std::vector<uint8_t>>("std::vector<uint8_t>", 1000);
    benchPushBack<absl::InlinedVector<uint8_t, 7>>("absl::InlinedVector<uint8_t, 31>", 1000);
    benchPushBack<boost::container::small_vector<uint8_t, 7>>("boost::container::small_vector<uint8_t, 31>", 1000);
}
