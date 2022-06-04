#include <ankerl/svector.h>

#include <doctest.h>

#include <array>

namespace {

template <size_t Alignment, size_t Size>
struct A {
    alignas(Alignment) std::array<std::byte, Size> m_data{};
};

} // namespace

static_assert(std::alignment_of_v<A<8, 12>> == 8);
static_assert(std::alignment_of_v<A<4, 12>> == 4);
static_assert(std::alignment_of_v<A<2, 12>> == 2);
static_assert(std::alignment_of_v<A<1, 12>> == 1);

static_assert(sizeof(A<1, 1>) == 1);
static_assert(sizeof(A<1, 2>) == 2);
static_assert(sizeof(A<1, 3>) == 3);
static_assert(sizeof(A<1, 4>) == 4);

static_assert(sizeof(A<2, 1>) == 2);
static_assert(sizeof(A<2, 2>) == 2);
static_assert(sizeof(A<2, 3>) == 4);
static_assert(sizeof(A<2, 4>) == 4);

using V16 = ankerl::svector<A<16, 16>, 3>;
static_assert(sizeof(V16) == size_t(16) * 4);

static_assert(ankerl::detail::alignment_of_svector<A<16, 16>>() == 16);
static_assert(ankerl::detail::size_of_svector<A<16, 16>>(3) == size_t(16) * 4);
