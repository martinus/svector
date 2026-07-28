// One workload, one container, one process, then exit.
//
// The benchmarks in the doctest suite next to this file run all four containers inside a single
// process, one after the other. That is fine when the workload does not allocate much, and wrong
// when it does: glibc adapts its mmap threshold the first time a large block is freed, so whichever
// container runs first pays mmap and munmap on every iteration and trains the allocator for the
// ones behind it. On an emplace_back of 10000 std::string that is worth a factor of three, and it
// follows the position in the list rather than the container -- put svector first and svector is
// the slow one.
//
// So the numbers the README publishes come from here instead. Each run measures exactly one
// container and then exits, which is also what a program that uses one of them looks like.
// scripts/bench/render_charts.py drives it and takes the median over several runs.
//
//     bench-solo <workload> <container 0..3>

#include <ankerl/svector.h>
#include <app/boost_absl.h>
#include <app/nanobench.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <string>
#include <vector>

using namespace std::literals;

namespace {

constexpr auto small_n = size_t{1000};
constexpr auto append_n = size_t{10000};
constexpr auto accumulate_n = size_t{100};

// Stand-ins so this still compiles without the headers. Selecting them is refused below.
#if ANKERL_SVECTOR_HAS_ABSL()
template <typename T, size_t N>
using AbslVec = absl::InlinedVector<T, N>;
#else
template <typename T, size_t N>
using AbslVec = std::vector<T>;
#endif

#if ANKERL_SVECTOR_HAS_BOOST()
template <typename T, size_t N>
using BoostVec = boost::container::small_vector<T, N>;
#else
template <typename T, size_t N>
using BoostVec = std::vector<T>;
#endif

auto bench(char const* name) -> ankerl::nanobench::Bench {
    auto b = ankerl::nanobench::Bench();
    b.name(name).warmup(3).minEpochTime(100ms);
    return b;
}

template <typename Vec>
void push_back(char const* name) {
    bench(name).batch(small_n).unit("push_back").run([&] {
        auto vec = Vec();
        for (size_t i = 0; i < small_n; ++i) {
            vec.push_back(static_cast<uint8_t>(i));
        }
        ankerl::nanobench::doNotOptimizeAway(vec.data());
    });
}

template <typename Vec>
void emplace_back_string(char const* name) {
    bench(name).batch(append_n).unit("emplace_back").run([&] {
        auto vec = Vec();
        for (size_t i = 0; i < append_n; ++i) {
            vec.emplace_back("hello");
        }
        ankerl::nanobench::doNotOptimizeAway(vec.data());
    });
}

template <typename Vec>
void random_access(char const* name) {
    auto vec = Vec();
    auto rng = ankerl::nanobench::Rng(1234);
    for (size_t i = 0; i < small_n; ++i) {
        vec.push_back(static_cast<int>(rng()));
    }

    auto sum = int();
    auto const s = static_cast<uint32_t>(vec.size());
    bench(name).batch(4).unit("read").run([&] {
        sum += vec[rng.bounded(s)];
        sum += vec[rng.bounded(s)];
        sum += vec[rng.bounded(s)];
        sum += vec[rng.bounded(s)];
    });
    ankerl::nanobench::doNotOptimizeAway(sum);
}

template <typename Vec>
void accumulate(char const* name) {
    auto vec = Vec();
    auto rng = ankerl::nanobench::Rng(123);
    for (size_t i = 0; i < accumulate_n; ++i) {
        vec.emplace_back(rng());
    }
    bench(name).batch(accumulate_n).unit("element").run([&] {
        auto ret = std::accumulate(vec.begin(), vec.end(), uint64_t());
        ankerl::nanobench::doNotOptimizeAway(ret);
    });
}

template <typename Vec>
void shuffle_sort(char const* name) {
    auto vec = Vec();
    auto rng = ankerl::nanobench::Rng(554433);
    for (size_t i = 0; i < small_n; ++i) {
        vec.emplace_back(std::to_string(rng()));
    }
    std::sort(vec.begin(), vec.end());

    bench(name).unit("shuffle+sort").run([&] {
        ankerl::nanobench::Rng(123).shuffle(vec);
        std::sort(vec.begin(), vec.end());
        ankerl::nanobench::doNotOptimizeAway(vec.front());
    });
}

// Grows to small_n elements, each emplaced at a random index, so most of the cost is the shift.
template <typename Vec>
void insert_random(char const* name) {
    bench(name).batch(small_n).unit("insert").run([&] {
        auto rng = ankerl::nanobench::Rng(1234);
        auto vec = Vec();
        for (size_t i = 0; i < small_n; ++i) {
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

// Same but always at the front, the worst case for the shift.
template <typename Vec>
void insert_front(char const* name) {
    bench(name).batch(small_n).unit("insert").run([&] {
        auto vec = Vec();
        for (size_t i = 0; i < small_n; ++i) {
            if constexpr (std::is_same_v<typename Vec::value_type, std::string>) {
                vec.emplace(vec.begin(), "hello");
            } else {
                vec.emplace(vec.begin(), i);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(vec.data());
    });
}

template <typename T>
struct tag {
    using type = T;
};

// Hands the picked container type to op as a tag, so op stays one generic lambda.
template <typename StdVec, typename Absl, typename Boost, typename Svec, typename Op>
void pick(int container, Op op) {
    switch (container) {
    case 0:
        op(tag<StdVec>{});
        break;
    case 1:
        op(tag<Absl>{});
        break;
    case 2:
        op(tag<Boost>{});
        break;
    default:
        op(tag<Svec>{});
        break;
    }
}

char const* const g_workloads[] = {"push_back",
                                   "emplace_back_string",
                                   "random_access",
                                   "accumulate",
                                   "shuffle_sort",
                                   "insert_random_int",
                                   "insert_random_string",
                                   "insert_front_int",
                                   "insert_front_string"};

char const* const g_containers[] = {"std::vector", "absl", "boost", "ankerl::svector"};

auto run(char const* workload, int container, char const* name) -> bool {
    auto is = [&](char const* w) {
        return 0 == std::strcmp(workload, w);
    };

    if (is("push_back")) {
        pick<std::vector<uint8_t>, AbslVec<uint8_t, 7>, BoostVec<uint8_t, 7>, ankerl::svector<uint8_t, 7>>(
            container, [&](auto t) {
                push_back<typename decltype(t)::type>(name);
            });
    } else if (is("emplace_back_string")) {
        pick<std::vector<std::string>, AbslVec<std::string, 7>, BoostVec<std::string, 7>, ankerl::svector<std::string, 7>>(
            container, [&](auto t) {
                emplace_back_string<typename decltype(t)::type>(name);
            });
    } else if (is("random_access")) {
        pick<std::vector<int>, AbslVec<int, 7>, BoostVec<int, 7>, ankerl::svector<int, 7>>(container, [&](auto t) {
            random_access<typename decltype(t)::type>(name);
        });
    } else if (is("accumulate")) {
        pick<std::vector<uint64_t>, AbslVec<uint64_t, 7>, BoostVec<uint64_t, 7>, ankerl::svector<uint64_t, 7>>(
            container, [&](auto t) {
                accumulate<typename decltype(t)::type>(name);
            });
    } else if (is("shuffle_sort")) {
        pick<std::vector<std::string>, AbslVec<std::string, 7>, BoostVec<std::string, 7>, ankerl::svector<std::string, 7>>(
            container, [&](auto t) {
                shuffle_sort<typename decltype(t)::type>(name);
            });
    } else if (is("insert_random_int")) {
        pick<std::vector<uint64_t>, AbslVec<uint64_t, 7>, BoostVec<uint64_t, 7>, ankerl::svector<uint64_t, 7>>(
            container, [&](auto t) {
                insert_random<typename decltype(t)::type>(name);
            });
    } else if (is("insert_random_string")) {
        pick<std::vector<std::string>, AbslVec<std::string, 7>, BoostVec<std::string, 7>, ankerl::svector<std::string, 7>>(
            container, [&](auto t) {
                insert_random<typename decltype(t)::type>(name);
            });
    } else if (is("insert_front_int")) {
        pick<std::vector<uint64_t>, AbslVec<uint64_t, 7>, BoostVec<uint64_t, 7>, ankerl::svector<uint64_t, 7>>(
            container, [&](auto t) {
                insert_front<typename decltype(t)::type>(name);
            });
    } else if (is("insert_front_string")) {
        pick<std::vector<std::string>, AbslVec<std::string, 7>, BoostVec<std::string, 7>, ankerl::svector<std::string, 7>>(
            container, [&](auto t) {
                insert_front<typename decltype(t)::type>(name);
            });
    } else {
        return false;
    }
    return true;
}

} // namespace

auto main(int argc, char** argv) -> int {
    if (argc != 3) {
        std::fputs("usage: bench-solo <workload> <container 0..3>\nworkloads:", stdout);
        for (auto const* w : g_workloads) {
            std::printf(" %s", w);
        }
        std::fputs("\ncontainers:", stdout);
        for (auto const* c : g_containers) {
            std::printf(" %s", c);
        }
        std::fputs("\n", stdout);
        return 1;
    }

    auto const container = std::atoi(argv[2]); // NOLINT(cert-err34-c)
    if (container < 0 || container > 3) {
        std::fputs("container must be 0..3\n", stderr);
        return 1;
    }
    if (container == 1 && !ANKERL_SVECTOR_HAS_ABSL()) {
        std::fputs("built without absl\n", stderr);
        return 2;
    }
    if (container == 2 && !ANKERL_SVECTOR_HAS_BOOST()) {
        std::fputs("built without boost\n", stderr);
        return 2;
    }

    if (!run(argv[1], container, g_containers[container])) {
        std::fprintf(stderr, "unknown workload '%s'\n", argv[1]);
        return 1;
    }
    return 0;
}
