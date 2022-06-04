#include <ankerl/svector.h>
#include <app/name_of_type.h>
#include <app/nanobench.h>

#include <doctest.h>

#include <absl/container/inlined_vector.h>
#include <boost/container/small_vector.hpp>
#include <fmt/format.h>

#include <fstream>

using namespace std::literals;

template <typename Vec>
void randomaccess(ankerl::nanobench::Bench& bench, size_t num_items) {

    auto vec = Vec();
    auto rng = ankerl::nanobench::Rng(1234);
    for (size_t i = 0; i < num_items; ++i) {
        vec.push_back(rng());
    }

    auto sum = int();
    auto s = vec.size();
    bench.batch(4).warmup(3).minEpochTime(100ms).run(std::string(name_of_type<Vec>()), [&] {
        sum += vec[rng.bounded(s)];
        sum += vec[rng.bounded(s)];
        sum += vec[rng.bounded(s)];
        sum += vec[rng.bounded(s)];
    });
    ankerl::nanobench::doNotOptimizeAway(sum);
}

// https://github.com/wichtounet/articles/blob/master/src/vector_list/bench.cpp
TEST_CASE("bench_randomaccess" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 1000;
    auto bench = ankerl::nanobench::Bench();
    bench.title("random access");

    randomaccess<std::vector<int>>(bench, num_items);
    randomaccess<absl::InlinedVector<int, 7>>(bench, num_items);
    randomaccess<boost::container::small_vector<int, 7>>(bench, num_items);
    randomaccess<ankerl::svector<int, 7>>(bench, num_items);

    auto f = std::ofstream("bench_randomaccess_int.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), f);
}
