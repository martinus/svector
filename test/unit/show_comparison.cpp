#include <ankerl/svector.h>

#include <app/boost_absl.h>

#include <doctest.h>
#include <fmt/format.h>

#include <cstdint>
#include <vector>

// Prints the table the README's "How compact can it get?" section shows, for whatever versions of
// boost and abseil happen to be installed. Skipped by default, run it with
//
//     test-svector -ns -tc=show_comparison
//
namespace {

template <typename T>
auto row() -> std::string {
    return fmt::format("{:>4} /{:>4}", sizeof(T), T{}.capacity());
}

// Same requested inline capacity across all three, so the columns are comparable.
template <size_t N>
void line() {
    fmt::print("{:>10} |", N);
#if ANKERL_SVECTOR_HAS_BOOST()
    fmt::print(" {} |", row<boost::container::small_vector<uint8_t, N>>());
#endif
#if ANKERL_SVECTOR_HAS_ABSL()
    fmt::print(" {} |", row<absl::InlinedVector<uint8_t, N>>());
#endif
    fmt::print(" {}\n", row<ankerl::svector<uint8_t, N>>());
}

} // namespace

TEST_CASE("show_comparison" * doctest::skip()) {
    fmt::print("uint8_t elements, sizeof / inline capacity in bytes\n\n");
    fmt::print("{:>10} |", "asked for");
#if ANKERL_SVECTOR_HAS_BOOST()
    fmt::print("{:>11} |", "boost");
#endif
#if ANKERL_SVECTOR_HAS_ABSL()
    fmt::print("{:>11} |", "absl");
#endif
    fmt::print("{:>11}\n", "svector");

    line<1>();
    line<8>();
    line<16>();
    line<32>();
    line<64>();

    fmt::print("\nstd::vector<uint8_t>: sizeof {}, no inline capacity\n", sizeof(std::vector<uint8_t>));
}
