#include <ankerl/svector.h>
#include <app/name_of_type.h>

#include <doctest.h>
#include <nanobench.h>

#include <absl/container/inlined_vector.h>
#include <boost/container/small_vector.hpp>
#include <fmt/format.h>

using namespace std::literals;

template <typename Vec>
void benchPushBack(size_t num_iters) {
    auto title = name_of_type<Vec>();

    // determine exact number of push_backs performed
    auto rng = ankerl::nanobench::Rng(123);
    auto num_push = size_t();
    for (size_t i = 0; i < num_iters; ++i) {
        num_push += rng.bounded(16);
    }

    ankerl::nanobench::Bench().batch(num_push).minEpochTime(100ms).run(std::string(title), [&] {
        auto rng = ankerl::nanobench::Rng(123);
        for (size_t i = 0; i < num_iters; ++i) {
            auto vec = Vec();
            auto num_push = rng.bounded(16);
            for (size_t i = 0; i < num_push; ++i) {
                vec.push_back(i);
            }
            ankerl::nanobench::doNotOptimizeAway(vec.data());
        }
    });
}

TEST_CASE("push_back" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_iters = 1000;
    benchPushBack<ankerl::svector<uint8_t, 7>>(num_iters);
    benchPushBack<std::vector<uint8_t>>(num_iters);
    benchPushBack<absl::InlinedVector<uint8_t, 7>>(num_iters);
    benchPushBack<boost::container::small_vector<uint8_t, 7>>(num_iters);
}

template <typename Vec, typename... Args>
void bench_fill_emplace_back(size_t num_items, Args&&... args) {
    auto title = fmt::format("fill_emplace_back {}", name_of_type<Vec>());

    ankerl::nanobench::Bench().batch(num_items).minEpochTime(100ms).run(title, [&] {
        auto vec = Vec();
        for (size_t i = 0; i < num_items; ++i) {
            vec.emplace_back(std::forward<Args>(args)...);
        }
        ankerl::nanobench::doNotOptimizeAway(vec.data());
    });
}

// https://github.com/wichtounet/articles/blob/master/src/vector_list/bench.cpp
TEST_CASE("fill_back" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 1000000;
    bench_fill_emplace_back<ankerl::svector<int, 7>>(num_items);
    bench_fill_emplace_back<std::vector<int>>(num_items);
    bench_fill_emplace_back<absl::InlinedVector<int, 7>>(num_items);
    bench_fill_emplace_back<boost::container::small_vector<int, 7>>(num_items);

    num_items = 1000000;
    bench_fill_emplace_back<ankerl::svector<std::string, 7>>(num_items, "hello");
    bench_fill_emplace_back<std::vector<std::string>>(num_items, "hello");
    bench_fill_emplace_back<absl::InlinedVector<std::string, 7>>(num_items, "hello");
    bench_fill_emplace_back<boost::container::small_vector<std::string, 7>>(num_items, "hello");
}