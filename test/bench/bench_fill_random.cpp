#include <ankerl/svector.h>
#include <app/boost_absl.h>
#include <app/name_of_type.h>
#include <app/nanobench.h>

#include <doctest.h>

#include <fmt/format.h>

#include <fstream>

using namespace std::literals;

template <typename Vec>
void fill_random(ankerl::nanobench::Bench& bench, size_t num_items) {
    bench.batch(num_items).warmup(3).minEpochTime(10ms).run(std::string(name_of_type<Vec>()), [&] {
        auto rng = ankerl::nanobench::Rng(1234);
        auto vec = Vec();
        for (size_t i = 0; i < num_items; ++i) {
            auto it = vec.begin() + rng.bounded(static_cast<uint32_t>(vec.size()));
            if constexpr (std::is_same_v<typename Vec::value_type, std::string>) {
                vec.emplace(it, "hello");
            } else {
                vec.emplace(it, i);
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
#if ANKERL_SVECTOR_HAS_ABSL()
    fill_random<absl::InlinedVector<uint64_t, 7>>(bench, num_items);
#endif
#if ANKERL_SVECTOR_HAS_BOOST()
    fill_random<boost::container::small_vector<uint64_t, 7>>(bench, num_items);
#endif
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
#if ANKERL_SVECTOR_HAS_ABSL()
    fill_random<absl::InlinedVector<std::string, 7>>(bench, num_items);
#endif
#if ANKERL_SVECTOR_HAS_BOOST()
    fill_random<boost::container::small_vector<std::string, 7>>(bench, num_items);

#endif
    fill_random<ankerl::svector<std::string, 7>>(bench, num_items);

    auto f = std::ofstream("bench_fill_random_string.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), f);
}
