#include <ankerl/svector.h>

#include <absl/container/inlined_vector.h>
#include <boost/container/small_vector.hpp>
#include <doctest.h>
#include <nanobench.h>

using namespace std::literals;

template <typename Vec>
void benchPushBack(char const* title, size_t num_elements) {
    ankerl::nanobench::Bench().batch(num_elements).minEpochTime(100ms).run(title, [&] {
        auto rng = ankerl::nanobench::Rng(123);
        for (size_t i = 0; i < 1000; ++i) {
            auto vec = Vec();
            auto num_push = rng.bounded(31);
            for (size_t i = 0; i < num_push; ++i) {
                vec.push_back(i);
            }

            while (!vec.empty()) {
                vec.pop_back();
            }
        }
    });
}

TEST_CASE("push_back" * doctest::skip() * doctest::test_suite("bench")) {
    benchPushBack<ankerl::svector<uint8_t, 7>>("svector<uint8_t, 31>", 1000);
    benchPushBack<std::vector<uint8_t>>("std::vector<uint8_t>", 1000);
    benchPushBack<absl::InlinedVector<uint8_t, 7>>("absl::InlinedVector<uint8_t, 31>", 1000);
    benchPushBack<boost::container::small_vector<uint8_t, 7>>("boost::container::small_vector<uint8_t, 31>", 1000);
}
