#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>

#include <cstddef>
#include <stdexcept>
#include <string>

TEST_CASE("resize") {
    Counter counts;
    auto a = ankerl::svector<Counter::Obj, 3>();
    a.resize(100);
    REQUIRE(a.size() == 100);
    for (auto const& x : a) {
        REQUIRE(x == Counter::Obj());
    }

    a.clear();
    for (size_t i = 0; i < 20; ++i) {
        a.emplace_back(i + 100, counts);
    }

    a.resize(10);
    REQUIRE(a.size() == 10);
    for (size_t i = 0; i < a.size(); ++i) {
        REQUIRE(a[i].get() == i + 100);
    }

    a.resize(3);
    REQUIRE(a.size() == 3);
    for (size_t i = 0; i < a.size(); ++i) {
        REQUIRE(a[i].get() == i + 100);
    }

    a.resize(0);
    REQUIRE(a.empty());
}

namespace {

// Constructing throws once the budget runs out.
struct ThrowOnConstruct {
    std::string value;

    static int budget; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    ThrowOnConstruct()
        : value(40, 'x') {
        if (--budget < 0) {
            throw std::runtime_error("ctor");
        }
    }

    ThrowOnConstruct(ThrowOnConstruct const& other)
        : value(other.value) {
        if (--budget < 0) {
            throw std::runtime_error("copy");
        }
    }

    ThrowOnConstruct(ThrowOnConstruct&&) noexcept = default;
    auto operator=(ThrowOnConstruct const&) -> ThrowOnConstruct& = default;
    auto operator=(ThrowOnConstruct&&) noexcept -> ThrowOnConstruct& = default;
    ~ThrowOnConstruct() = default;
};

int ThrowOnConstruct::budget = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

using SvThrowing = ankerl::svector<ThrowOnConstruct, 4>;

} // namespace

// resize_after_reserve() only recorded the new size once the whole construction loop had finished,
// so a throwing constructor left everything it had already built unreachable and unreleased. Run
// under asan, the leak checker is what asserts here. See issue #70.
TEST_CASE("resize_throwing_ctor_does_not_leak") {
    for (size_t start = 0; start <= 6; ++start) {
        for (size_t target = start + 1; target <= 12; ++target) {
            // the cleanup range does not depend on how many were built, first and last is enough
            for (auto const built : {size_t{0}, target - start - 1}) {
                auto v = SvThrowing();
                ThrowOnConstruct::budget = 1000000;
                v.resize(start);

                ThrowOnConstruct::budget = static_cast<int>(built);
                REQUIRE_THROWS_AS(v.resize(target), std::runtime_error);
                ThrowOnConstruct::budget = 1000000;

                // the failed resize did not commit, and nothing it built is left behind
                REQUIRE(v.size() == start);
            }
        }
    }
}

TEST_CASE("resize_with_value_throwing_copy_does_not_leak") {
    ThrowOnConstruct::budget = 1000000;
    auto const filler = ThrowOnConstruct();

    for (size_t start = 0; start <= 6; ++start) {
        for (size_t target = start + 1; target <= 12; ++target) {
            auto v = SvThrowing();
            ThrowOnConstruct::budget = 1000000;
            v.resize(start);

            ThrowOnConstruct::budget = static_cast<int>((target - start) / 2);
            REQUIRE_THROWS_AS(v.resize(target, filler), std::runtime_error);
            ThrowOnConstruct::budget = 1000000;

            REQUIRE(v.size() == start);
        }
    }
}

TEST_CASE("svector_count_ctor_throwing_ctor_does_not_leak") {
    for (size_t count = 1; count <= 12; ++count) {
        ThrowOnConstruct::budget = static_cast<int>(count / 2);
        // the extra cast keeps SvThrowing(count) from parsing as a declaration of count
        REQUIRE_THROWS_AS(static_cast<void>(SvThrowing(count)), std::runtime_error);
        ThrowOnConstruct::budget = 1000000;
    }
}
