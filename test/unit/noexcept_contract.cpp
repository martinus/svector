// Tests for https://github.com/martinus/svector/issues/63
//
// svector is meant to be a drop in for std::vector, so it should make the same noexcept promises
// wherever it can, and no promise it cannot keep. The one place it genuinely differs is moving:
// std::vector only ever steals a pointer, while an svector in direct mode has to relocate its
// inline elements, which runs T's move constructor.

#include <ankerl/svector.h>

#include <doctest.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

// Moving this throws, so no container holding it can promise a noexcept move.
struct ThrowingMove {
    int value;

    explicit ThrowingMove(int v)
        : value(v) {}

    ThrowingMove(ThrowingMove const& other) = default;

    ThrowingMove(ThrowingMove&& other) // NOLINT(performance-noexcept-move-constructor)
        : value(other.value) {
        if (value == throwing_value) {
            throw std::runtime_error("move");
        }
    }

    auto operator=(ThrowingMove const&) -> ThrowingMove& = default;
    ~ThrowingMove() = default;

    static constexpr int throwing_value = 42;
};

static_assert(!std::is_nothrow_move_constructible_v<ThrowingMove>);

using SvString = ankerl::svector<std::string, 4>;
using SvThrowing = ankerl::svector<ThrowingMove, 4>;

} // namespace

// A T that moves without throwing keeps the unconditional promise it had before.
static_assert(std::is_nothrow_move_constructible_v<ankerl::svector<int, 4>>);
static_assert(std::is_nothrow_move_assignable_v<ankerl::svector<int, 4>>);
static_assert(std::is_nothrow_move_constructible_v<SvString>);
static_assert(std::is_nothrow_move_assignable_v<SvString>);

// A T that can throw while moving must not be covered up.
static_assert(!std::is_nothrow_move_constructible_v<SvThrowing>);
static_assert(!std::is_nothrow_move_assignable_v<SvThrowing>);

// swap is a move construction plus two move assignments, so it carries the same condition. This
// is the one place where svector cannot match std::vector, whose swap is noexcept for any T
// because it only exchanges pointers.
static_assert(std::is_nothrow_swappable_v<SvString>);
static_assert(!std::is_nothrow_swappable_v<SvThrowing>);

// Only ever named inside noexcept(), which does not evaluate its operand, so these need no
// definition.
extern SvString sv;                  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
extern std::vector<std::string> vec; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// [vector.overview] spells these out as noexcept, so every conforming std::vector agrees and
// svector can be held to the same guarantee portably. Asserting it for std::vector too is what
// makes this a check against the standard rather than against one implementation.
#define STANDARD_REQUIRES_NOEXCEPT(call)     \
    static_assert(noexcept(sv.call), #call); \
    static_assert(noexcept(vec.call), #call)

static_assert(std::is_nothrow_default_constructible_v<SvString>);
static_assert(std::is_nothrow_default_constructible_v<std::vector<std::string>>);

STANDARD_REQUIRES_NOEXCEPT(size());
STANDARD_REQUIRES_NOEXCEPT(capacity());
STANDARD_REQUIRES_NOEXCEPT(empty());
STANDARD_REQUIRES_NOEXCEPT(max_size());
STANDARD_REQUIRES_NOEXCEPT(data());
STANDARD_REQUIRES_NOEXCEPT(begin());
STANDARD_REQUIRES_NOEXCEPT(end());
STANDARD_REQUIRES_NOEXCEPT(cbegin());
STANDARD_REQUIRES_NOEXCEPT(cend());
STANDARD_REQUIRES_NOEXCEPT(rbegin());
STANDARD_REQUIRES_NOEXCEPT(rend());
STANDARD_REQUIRES_NOEXCEPT(crbegin());
STANDARD_REQUIRES_NOEXCEPT(crend());
STANDARD_REQUIRES_NOEXCEPT(clear());

#undef STANDARD_REQUIRES_NOEXCEPT

// Past that the standard leaves it open and the implementations disagree: libstdc++ and the MSVC
// STL declare pop_back noexcept, libc++ does not. None of them can throw in svector, so these are
// our own promise and are checked on their own rather than against whichever std::vector we
// happen to be building with.
static_assert(noexcept(sv.operator[](0)));
static_assert(noexcept(sv.front()));
static_assert(noexcept(sv.back()));
static_assert(noexcept(sv.pop_back()));

// The two that really can throw. at() reports a bad index, and shrink_to_fit() allocates and lets
// a failure through instead of quietly doing nothing the way libc++'s vector does.
static_assert(!noexcept(sv.at(0)));
static_assert(!noexcept(sv.shrink_to_fit()));

TEST_CASE("noexcept_throwing_move_propagates") {
    // Before the fix this called std::terminate instead of unwinding.
    auto a = SvThrowing();
    a.emplace_back(ThrowingMove::throwing_value);
    REQUIRE(a.size() == 1);

    REQUIRE_THROWS_AS(SvThrowing(std::move(a)), std::runtime_error);
}

TEST_CASE("noexcept_throwing_move_assign_propagates") {
    auto a = SvThrowing();
    a.emplace_back(ThrowingMove::throwing_value);

    auto b = SvThrowing();
    REQUIRE_THROWS_AS(b = std::move(a), std::runtime_error);
}

TEST_CASE("noexcept_indirect_move_never_touches_elements") {
    // Once we are indirect the move only takes over the allocation, so even a throwing move
    // constructor is never called. Only the compile time promise has to be conservative.
    auto a = SvThrowing();
    for (int i = 0; i < 100; ++i) {
        a.emplace_back(i + 1000); // never the throwing value, growth has to relocate these
    }
    a.emplace_back(ThrowingMove::throwing_value); // would throw if it were ever moved
    auto const* ptr = a.data();

    auto b = std::move(a);
    REQUIRE(b.data() == ptr);
    REQUIRE(b.size() == 101);
}

TEST_CASE("growth_with_throwing_move_does_not_leak") {
    // Growing relocates the existing elements into freshly allocated storage. When T's move
    // constructor throws partway through, that storage was leaked: it is not owned by anything
    // yet, and the element emplace_back had already built in it was never destroyed. Only
    // reachable with a throwing move, which is why claiming an unconditional noexcept move hid
    // it. Run under asan to see the leak.
    auto v = SvThrowing();
    while (v.size() < v.capacity()) {
        v.emplace_back(0);
    }
    // one element that refuses to be relocated, then force a reallocation
    v.emplace_back(ThrowingMove::throwing_value);
    auto const size_before = v.size();
    while (v.size() < v.capacity()) {
        v.emplace_back(0);
    }

    REQUIRE_THROWS_AS(v.emplace_back(0), std::runtime_error);

    // the failed grow left us alone, we still own everything we owned before
    REQUIRE(v.size() >= size_before);
    REQUIRE(v[size_before - 1].value == ThrowingMove::throwing_value);
}

TEST_CASE("noexcept_vector_growth_keeps_strong_guarantee") {
    // std::vector picks move vs copy with std::move_if_noexcept. While svector claimed an
    // unconditional noexcept move, growing a std::vector of them moved the elements and a
    // throwing move terminated instead of rolling back.
    auto outer = std::vector<SvThrowing>();
    for (int i = 0; i < 8; ++i) {
        outer.emplace_back();
        outer.back().emplace_back(ThrowingMove::throwing_value);
    }

    for (int i = 0; i < 200; ++i) {
        outer.emplace_back(); // reallocates repeatedly, must fall back to copying
    }
    REQUIRE(outer.size() == 208);
    REQUIRE(outer[0][0].value == ThrowingMove::throwing_value);
}
