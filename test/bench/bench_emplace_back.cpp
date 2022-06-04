#include <ankerl/svector.h>
#include <app/name_of_type.h>
#include <app/nanobench.h>

#include <doctest.h>

#include <absl/container/inlined_vector.h>
#include <boost/container/small_vector.hpp>
#include <fmt/format.h>

#include <fstream>

using namespace std::literals;

template <typename Vec, typename... Args>
void emplace_back(ankerl::nanobench::Bench& bench, size_t num_items, Args&&... args) {
    bench.batch(num_items).warmup(3).minEpochTime(10ms).run(std::string(name_of_type<Vec>()), [&] {
        auto vec = Vec();
        for (size_t i = 0; i < num_items; ++i) {
            vec.emplace_back(std::forward<Args>(args)...);
        }
        ankerl::nanobench::doNotOptimizeAway(vec.data());
    });
}

// https://github.com/wichtounet/articles/blob/master/src/vector_list/bench.cpp
TEST_CASE("bench_emplace_back_int" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 10000;
    auto bench = ankerl::nanobench::Bench();
    bench.title("emplace_back int");
    bench.epochs(100);

    emplace_back<std::vector<int>>(bench, num_items);
    emplace_back<absl::InlinedVector<int, 4>>(bench, num_items);
    emplace_back<boost::container::small_vector<int, 4>>(bench, num_items);
    emplace_back<ankerl::svector<int, 4>>(bench, num_items);

    auto f = std::ofstream("bench_emplace_back_int.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), f);
}

TEST_CASE("bench_emplace_back_string" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 10000;
    auto bench = ankerl::nanobench::Bench();
    bench.title("emplace_back");
    bench.epochs(100);

    emplace_back<std::vector<std::string>>(bench, num_items, "hello");
    emplace_back<absl::InlinedVector<std::string, 7>>(bench, num_items, "hello");
    emplace_back<boost::container::small_vector<std::string, 7>>(bench, num_items, "hello");
    emplace_back<ankerl::svector<std::string, 7>>(bench, num_items, "hello");

    auto f = std::ofstream("bench_emplace_back_string.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), f);
}
