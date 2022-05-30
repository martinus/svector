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
void push_back(size_t num_iters, ankerl::nanobench::Bench& bench) {
    auto title = name_of_type<Vec>();

    // determine exact number of push_backs performed
    auto rng = ankerl::nanobench::Rng(123);
    auto num_push = size_t();
    for (size_t i = 0; i < num_iters; ++i) {
        num_push += rng.bounded(16);
    }

    bench.batch(num_push).warmup(10).minEpochTime(100ms).unit("push_back").run(std::string(title), [&] {
        auto vec = Vec();
        for (size_t i = 0; i < num_iters; ++i) {
            vec.push_back(i);
        }
        ankerl::nanobench::doNotOptimizeAway(vec.data());
    });
}

TEST_CASE("bench_push_back" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_iters = 1000;

    auto bench = ankerl::nanobench::Bench();
    push_back<std::vector<uint8_t>>(num_iters, bench);
    push_back<absl::InlinedVector<uint8_t, 7>>(num_iters, bench);
    push_back<boost::container::small_vector<uint8_t, 7>>(num_iters, bench);
    push_back<ankerl::svector<uint8_t, 7>>(num_iters, bench);

    auto fout = std::ofstream("bench_push_back.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), fout);
}
