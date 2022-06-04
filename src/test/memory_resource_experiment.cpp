#include <app/name_of_type.h>
#include <app/nanobench.h>
#include <doctest.h>

#include <memory_resource>

#include <array>
#include <unordered_map>

using namespace std::literals;

namespace {

template <size_t Alignment, size_t Size>
struct A {
    alignas(Alignment) std::array<std::byte, Size> m_data{};
};

template <typename Map>
void bench(Map& map) {
    size_t batch_size = 5000;

    // make sure each iteration of the benchmark contains exactly 5000 inserts and one clear.
    // do this at least 10 times so we get reasonable accurate results

    ankerl::nanobench::Bench().batch(batch_size).name(std::string(name_of_type<Map>())).minEpochTime(100ms).run([&] {
        uint64_t key = 0;
        for (size_t i = 0; i < batch_size; ++i) {
            // add a random number for better spread in the map
            key += 0x967f29d1;
            map[key];
        }
        map.clear();
    });
}

} // namespace

TEST_CASE("memory_resource" * doctest::skip() * doctest::test_suite("bench")) {

    using Mapped = A<8, 102>;
    // using Mapped = std::string;

    auto options = std::pmr::pool_options();
    options.max_blocks_per_chunk = 6000;
    options.largest_required_pool_block = 256;

    auto mr = std::pmr::unsynchronized_pool_resource();
    auto pmr_map = std::pmr::unordered_map<uint64_t, Mapped>(0, std::hash<uint64_t>{}, std::equal_to<uint64_t>{}, &mr);
    bench(pmr_map);

    auto std_map = std::unordered_map<uint64_t, Mapped>();
    bench(std_map);
}
