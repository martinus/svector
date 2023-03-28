#include <ankerl/svector.h>

#include <app/boost_absl.h>

#include <doctest.h>
#include <fmt/format.h>

#include <vector>

template <typename T>
void show(char const* what) {
    fmt::print("{} sizeof({})\n", sizeof(T), what);
    fmt::print("{} capacity({})\n", T{}.capacity(), what);
    fmt::print("{} max_size({})\n\n", T{}.max_size(), what);
}

TEST_CASE("show_sizeof" * doctest::skip()) {
    // static constexpr auto a = sizeof(absl::InlinedVector<uint8_t, 24>);
    // static constexpr auto s = sizeof(ankerl::svector<uint8_t, 31>);
    // static constexpr auto b = sizeof(boost::container::small_vector<uint8_t, 16>);

    show<std::vector<uint8_t>>("std::vector<uint8_t>");
#if ANKERL_SVECTOR_HAS_ABSL()
    show<absl::InlinedVector<uint8_t, 56>>("absl::InlinedVector<uint8_t, 24>");
#endif
#if ANKERL_SVECTOR_HAS_BOOST()
    show<boost::container::small_vector<std::byte, 15>>("boost::container::small_vector<std::byte, 15>");
#endif
    show<ankerl::svector<uint8_t, 1>>("ankerl::svector<uint8_t, 1>");
}
