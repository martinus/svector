#include <ankerl/svector.h>
#include <app/name_of_type.h>

#include <doctest.h>
#include <nanobench.h>

#include <absl/container/inlined_vector.h>
#include <boost/container/small_vector.hpp>
#include <fmt/format.h>

#include <fstream>

using namespace std::literals;

template <typename Vec>
void benchPushBack(size_t num_iters, ankerl::nanobench::Bench& bench) {
    auto title = name_of_type<Vec>();

    // determine exact number of push_backs performed
    auto rng = ankerl::nanobench::Rng(123);
    auto num_push = size_t();
    for (size_t i = 0; i < num_iters; ++i) {
        num_push += rng.bounded(16);
    }

    bench.batch(num_push).warmup(10).minEpochTime(100ms).unit("push_back").run(std::string(title), [&] {
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

TEST_CASE("bench_push_back" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_iters = 1000;

    auto bench = ankerl::nanobench::Bench();
    benchPushBack<std::vector<uint8_t>>(num_iters, bench);
    benchPushBack<absl::InlinedVector<uint8_t, 7>>(num_iters, bench);
    benchPushBack<boost::container::small_vector<uint8_t, 7>>(num_iters, bench);
    benchPushBack<ankerl::svector<uint8_t, 7>>(num_iters, bench);

    auto fout = std::ofstream("bench_push_back.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), fout);
}
