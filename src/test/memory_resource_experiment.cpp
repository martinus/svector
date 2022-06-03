#include <app/nanobench.h>
#include <doctest.h>

#include <memory_resource>

#include <unordered_map>

namespace {

template <size_t Alignment, size_t Size>
struct A {
    alignas(Alignment) std::array<std::byte, Size> m_data{};
};

} // namespace

TEST_CASE("memory_resource") {
    auto mr = std::pmr::unsynchronized_pool_resource();

    auto map = std::pmr::unordered_map<uint64_t, A<8, 102>>();
}
