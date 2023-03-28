#include <ankerl/svector.h>
#include <app/boost_absl.h>
#include <app/name_of_type.h>
#include <app/nanobench.h>

#include <doctest.h>

#include <fmt/format.h>

#include <fstream>

using namespace std::literals;

template <typename Vec, typename... Args>
void fill_front(ankerl::nanobench::Bench& bench, size_t num_items, Args&&... args) {
    auto title = fmt::format("fill_front {}", name_of_type<Vec>());

    bench.batch(num_items).warmup(3).minEpochTime(100ms).run(title, [&] {
        auto vec = Vec();
        for (size_t i = 0; i < num_items; ++i) {
            vec.emplace(vec.begin(), std::forward<Args>(args)...);
        }
        ankerl::nanobench::doNotOptimizeAway(vec.data());
    });
}

// https://github.com/wichtounet/articles/blob/master/src/vector_list/bench.cpp
TEST_CASE("bench_fill_front_int" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 1000;
    auto bench = ankerl::nanobench::Bench();
    bench.title("emplace(vec.begin(), ...)");

    fill_front<std::vector<int>>(bench, num_items);
#if ANKERL_SVECTOR_HAS_ABSL()
    fill_front<absl::InlinedVector<int, 7>>(bench, num_items);
#endif
#if ANKERL_SVECTOR_HAS_BOOST()
    fill_front<boost::container::small_vector<int, 7>>(bench, num_items);
#endif
    fill_front<ankerl::svector<int, 7>>(bench, num_items);

    auto f = std::ofstream("bench_fill_front_int.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), f);
}

TEST_CASE("bench_fill_front_string" * doctest::skip() * doctest::test_suite("bench")) {
    auto num_items = 1000;
    auto bench = ankerl::nanobench::Bench();
    bench.title("emplace(vec.begin(), ...)");

    fill_front<std::vector<std::string>>(bench, num_items, "hello");
    fill_front<ankerl::svector<std::string, 7>>(bench, num_items, "hello");
#if ANKERL_SVECTOR_HAS_ABSL()
    fill_front<absl::InlinedVector<std::string, 7>>(bench, num_items, "hello");
#endif
#if ANKERL_SVECTOR_HAS_BOOST()
    fill_front<boost::container::small_vector<std::string, 7>>(bench, num_items, "hello");
#endif

    auto f = std::ofstream("bench_fill_front_string.html");
    bench.render(ankerl::nanobench::templates::htmlBoxplot(), f);
}