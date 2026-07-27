// Tests for https://github.com/martinus/svector/issues/68
//
// make_uninitialized_space() shifts the elements from pos out of the way and immediately counts
// the resulting gap in size(). Until the caller has constructed into that gap the container is
// claiming raw memory, so a throwing element constructor used to leave it in a state that could
// not even be destroyed: the destructor ran over slots that were never built, or over slots the
// filling algorithm had already cleaned up, which is a double destroy.
//
// Run under asan, the failures here are use-after-free and leaks rather than wrong values.

#include <ankerl/svector.h>
#include <app/VecTester.h>

#include <doctest.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// Copying throws once the budget runs out, moving never does. This is the ordinary case: a type
// whose copy can fail because it allocates, like std::string.
struct ThrowOnCopy {
    std::string value;

    static int budget; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    explicit ThrowOnCopy(std::string v)
        : value(std::move(v)) {}

    ThrowOnCopy(ThrowOnCopy const& other)
        : value(other.value) {
        if (--budget < 0) {
            throw std::runtime_error("copy");
        }
    }

    ThrowOnCopy(ThrowOnCopy&&) noexcept = default;
    auto operator=(ThrowOnCopy const&) -> ThrowOnCopy& = default;
    auto operator=(ThrowOnCopy&&) noexcept -> ThrowOnCopy& = default;
    ~ThrowOnCopy() = default;

    auto operator==(ThrowOnCopy const& other) const -> bool {
        return value == other.value;
    }
};

int ThrowOnCopy::budget = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static_assert(std::is_nothrow_move_constructible_v<ThrowOnCopy>);
static_assert(std::is_nothrow_move_assignable_v<ThrowOnCopy>);

// Same, but relocating it can throw too, so the container cannot shift the tail back and has to
// fall back to dropping it. All that is promised then is that nothing leaks.
struct ThrowOnCopyAndMove {
    std::string value;

    static int budget; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    explicit ThrowOnCopyAndMove(std::string v)
        : value(std::move(v)) {}

    ThrowOnCopyAndMove(ThrowOnCopyAndMove const& other)
        : value(other.value) {
        if (--budget < 0) {
            throw std::runtime_error("copy");
        }
    }

    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    ThrowOnCopyAndMove(ThrowOnCopyAndMove&& other)
        : value(std::move(other.value)) {}

    auto operator=(ThrowOnCopyAndMove const&) -> ThrowOnCopyAndMove& = default;
    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    auto operator=(ThrowOnCopyAndMove&& other) -> ThrowOnCopyAndMove& {
        value = std::move(other.value);
        return *this;
    }
    ~ThrowOnCopyAndMove() = default;
};

int ThrowOnCopyAndMove::budget = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static_assert(!std::is_nothrow_move_constructible_v<ThrowOnCopyAndMove>);

auto make(size_t count) -> ankerl::svector<ThrowOnCopy, 4> {
    auto v = ankerl::svector<ThrowOnCopy, 4>();
    for (auto& s : make_long_strings(count)) {
        v.emplace_back(std::move(s));
    }
    return v;
}

auto contents(ankerl::svector<ThrowOnCopy, 4> const& v) -> std::vector<std::string> {
    auto out = std::vector<std::string>();
    for (auto const& e : v) {
        out.push_back(e.value);
    }
    return out;
}

} // namespace

TEST_CASE("insert_count_throwing_copy_keeps_container_intact") {
    auto const filler = ThrowOnCopy(std::string(40, 'z'));

    // sizes on both sides of the inline capacity, counts that do and do not force a reallocation
    for (size_t size = 0; size <= 9; ++size) {
        for (size_t pos = 0; pos <= size; ++pos) {
            for (size_t count = 1; count <= 5; ++count) {
                // uninitialized_fill_n destroys whatever it built before rethrowing, so close_gap
                // sees the same empty gap whichever copy failed. First and last is enough.
                for (auto const throw_at : {size_t{0}, count - 1}) {
                    auto v = make(size);
                    auto const before = contents(v);

                    ThrowOnCopy::budget = static_cast<int>(throw_at);
                    REQUIRE_THROWS_AS(v.insert(v.cbegin() + pos, count, filler), std::runtime_error);
                    ThrowOnCopy::budget = 1000000;

                    // moving cannot throw for this type, so the insert rolls all the way back
                    REQUIRE(contents(v) == before);
                }
            }
        }
    }
}

TEST_CASE("insert_range_throwing_copy_keeps_container_intact") {
    auto const source = std::vector<ThrowOnCopy>{
        ThrowOnCopy(std::string(40, 'x')),
        ThrowOnCopy(std::string(40, 'y')),
        ThrowOnCopy(std::string(40, 'z')),
    };

    for (size_t size = 0; size <= 9; ++size) {
        for (size_t pos = 0; pos <= size; ++pos) {
            for (auto const throw_at : {size_t{0}, source.size() - 1}) {
                auto v = make(size);
                auto const before = contents(v);

                ThrowOnCopy::budget = static_cast<int>(throw_at);
                REQUIRE_THROWS_AS(v.insert(v.cbegin() + pos, source.begin(), source.end()), std::runtime_error);
                ThrowOnCopy::budget = 1000000;

                REQUIRE(contents(v) == before);
            }
        }
    }
}

TEST_CASE("insert_single_throwing_copy_keeps_container_intact") {
    auto const filler = ThrowOnCopy(std::string(40, 'z'));

    for (size_t size = 0; size <= 9; ++size) {
        for (size_t pos = 0; pos <= size; ++pos) {
            auto v = make(size);
            auto const before = contents(v);

            ThrowOnCopy::budget = 0;
            REQUIRE_THROWS_AS(v.insert(v.cbegin() + pos, filler), std::runtime_error);
            ThrowOnCopy::budget = 1000000;

            REQUIRE(contents(v) == before);
        }
    }
}

TEST_CASE("emplace_throwing_copy_keeps_container_intact") {
    auto const filler = ThrowOnCopy(std::string(40, 'z'));

    for (size_t size = 1; size <= 9; ++size) {
        for (size_t pos = 0; pos < size; ++pos) { // pos == size goes through emplace_back
            auto v = make(size);
            auto const before = contents(v);

            ThrowOnCopy::budget = 0;
            REQUIRE_THROWS_AS(v.emplace(v.cbegin() + pos, filler), std::runtime_error);
            ThrowOnCopy::budget = 1000000;

            REQUIRE(contents(v) == before);
        }
    }
}

TEST_CASE("insert_throwing_move_leaves_a_destructible_container") {
    // Relocating can throw here, so the tail cannot be shifted back and gets dropped instead. The
    // container is shorter than it was, which the standard allows, but it has to be consistent:
    // asan and the leak checker are what actually assert that.
    for (size_t size = 0; size <= 9; ++size) {
        for (size_t pos = 0; pos <= size; ++pos) {
            for (size_t count = 1; count <= 3; ++count) {
                auto v = ankerl::svector<ThrowOnCopyAndMove, 4>();
                ThrowOnCopyAndMove::budget = 1000000;
                for (auto& s : make_long_strings(size)) {
                    v.emplace_back(std::move(s));
                }

                auto const filler = ThrowOnCopyAndMove(std::string(40, 'z'));
                ThrowOnCopyAndMove::budget = 0;
                REQUIRE_THROWS_AS(v.insert(v.cbegin() + pos, count, filler), std::runtime_error);
                ThrowOnCopyAndMove::budget = 1000000;

                REQUIRE(v.size() <= size);
                // everything still there has to be readable and destroy cleanly
                for (auto const& e : v) {
                    REQUIRE(e.value.size() == 40);
                }
            }
        }
    }
}
