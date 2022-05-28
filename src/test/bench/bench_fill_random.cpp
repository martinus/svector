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
void fill_random(ankerl::nanobench::Bench& bench, size_t num_items) {
    bench.batch(num_items).warmup(3).minEpochTime(10ms).run(std::string(name_of_type<Vec>()), [&] {
        auto vec = Vec();
        auto rng = ankerl::nanobench::Rng(1234);
        for (size_t i = 0; i < num_items; ++i) {
            auto it = vec.begin() + rng.bounded(vec.size());
            if constexpr (std::is_same_v<typename Vec::value_type, std::string>) {
                vec.emplace(it, "hello");
            } else {
                vec.emplace(it, rng());
            }
        }
        ankerl::nanobench::doNotOptimizeAway(vec.data());
    });
}

TEST_CASE("bench_fill_random_uint64_t" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 1000;
    auto bench = ankerl::nanobench::Bench();
    bench.title("emplace <tt>uint64_t</tt> at random location");
    bench.epochs(100);

    fill_random<std::vector<uint64_t>>(bench, num_items);
    fill_random<absl::InlinedVector<uint64_t, 7>>(bench, num_items);
    fill_random<boost::container::small_vector<uint64_t, 7>>(bench, num_items);
    fill_random<ankerl::svector<uint64_t, 7>>(bench, num_items);

    auto f = std::ofstream("bench_fill_random_uint64_t.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), f);
}

TEST_CASE("bench_fill_random_string" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 1000;
    auto bench = ankerl::nanobench::Bench();
    bench.title("emplace <tt>std::string</tt> at random location");
    bench.epochs(100);

    fill_random<std::vector<std::string>>(bench, num_items);
    fill_random<absl::InlinedVector<std::string, 7>>(bench, num_items);
    fill_random<boost::container::small_vector<std::string, 7>>(bench, num_items);
    fill_random<ankerl::svector<std::string, 7>>(bench, num_items);

    auto f = std::ofstream("bench_fill_random_string.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), f);
}
