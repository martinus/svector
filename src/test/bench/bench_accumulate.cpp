#include <ankerl/svector.h>
#include <app/name_of_type.h>

#include <doctest.h>
#include <nanobench.h>

#include <absl/container/inlined_vector.h>
#include <boost/container/small_vector.hpp>
#include <fmt/format.h>

#include <fstream>
#include <numeric>

using namespace std::literals;

template <typename Vec>
void accumulate_numeric(size_t num_items, ankerl::nanobench::Bench& bench) {
    auto vec = Vec();
    auto rng = ankerl::nanobench::Rng(123);
    for (size_t i = 0; i < num_items; ++i) {
        vec.emplace_back(rng());
    }

    bench.batch(num_items).unit("element").warmup(3).minEpochTime(10ms).run(std::string(name_of_type<Vec>()), [&] {
        auto ret = std::accumulate(vec.begin(), vec.end(), uint64_t());
        REQUIRE(ret == 5440569445485746503);
    });
}

// https://github.com/wichtounet/articles/blob/master/src/vector_list/bench.cpp
TEST_CASE("bench_accumulate_numeric" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 1000;
    auto bench = ankerl::nanobench::Bench();
    bench.title("accumulate");
    bench.epochs(100);

    accumulate_numeric<std::vector<uint64_t>>(num_items, bench);
    accumulate_numeric<absl::InlinedVector<uint64_t, 7>>(num_items, bench);
    accumulate_numeric<boost::container::small_vector<uint64_t, 7>>(num_items, bench);
    accumulate_numeric<ankerl::svector<uint64_t, 7>>(num_items, bench);

    auto f = std::ofstream("bench_accumulate_numeric.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), f);
}

/*
    auto sum_len = [](int a, std::string const& str) {
        return a + str.size();
    };
    accumulate<std::vector<std::string>>(num_items, "hello", 0, sum_len);
    accumulate<ankerl::svector<std::string, 7>>(num_items, "hello", 0, sum_len);
    accumulate<absl::InlinedVector<std::string, 7>>(num_items, "hello", 0, sum_len);
    accumulate<boost::container::small_vector<std::string, 7>>(num_items, "hello", 0, sum_len);
    */