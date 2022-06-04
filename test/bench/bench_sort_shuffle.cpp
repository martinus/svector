#include <ankerl/svector.h>
#include <app/name_of_type.h>
#include <app/nanobench.h>

#include <doctest.h>

#include <absl/container/inlined_vector.h>
#include <boost/container/small_vector.hpp>
#include <fmt/format.h>

using namespace std::literals;

template <typename Vec>
void sort_shuffle(size_t num_items) {
    auto title = fmt::format("sort_shuffle {}", name_of_type<Vec>());

    auto vec = Vec();
    auto rng = ankerl::nanobench::Rng(554433);

    // create vector filled with random data
    for (size_t i = 0; i < num_items; ++i) {
        auto r = rng();
        if constexpr (std::is_same_v<typename Vec::value_type, std::string>) {
            vec.emplace_back(std::to_string(r));
        } else {
            vec.emplace_back(r);
        }
    }
    std::sort(vec.begin(), vec.end());

    ankerl::nanobench::Bench().warmup(3).minEpochTime(100ms).run(title, [&] {
        ankerl::nanobench::Rng(123).shuffle(vec); // create a new RNG to ensure we shuffle deterministically
        std::sort(vec.begin(), vec.end());
        if constexpr (std::is_same_v<typename Vec::value_type, std::string>) {
            REQUIRE(vec.front() == "10000017441998304507");
        } else {
            REQUIRE(vec.front() == 5239234720771008);
        }
    });
}

// https://github.com/wichtounet/articles/blob/master/src/vector_list/bench.cpp
TEST_CASE("bench_sort_shuffle_uint64_t" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 1000;
    sort_shuffle<std::vector<uint64_t>>(num_items);
    sort_shuffle<absl::InlinedVector<uint64_t, 7>>(num_items);
    sort_shuffle<boost::container::small_vector<uint64_t, 7>>(num_items);
    sort_shuffle<ankerl::svector<uint64_t, 7>>(num_items);
}

TEST_CASE("bench_sort_shuffle_string" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 1000;
    sort_shuffle<std::vector<std::string>>(num_items);
    sort_shuffle<absl::InlinedVector<std::string, 7>>(num_items);
    sort_shuffle<boost::container::small_vector<std::string, 7>>(num_items);
    sort_shuffle<ankerl::svector<std::string, 7>>(num_items);
}
