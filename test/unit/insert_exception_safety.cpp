// Tests for what an insert leaves behind when constructing, moving or assigning a T throws.
// Originally https://github.com/martinus/svector/issues/68, extended by #74.
//
// Making space used to shift the elements from pos out of the way and immediately count the
// resulting gap in size(). Until the caller had constructed into that gap the container was
// claiming raw memory, so a throwing element constructor left it in a state that could not even be
// destroyed. insert_n() threads the new elements through the shift instead, so there is no gap to
// get wrong: whatever throws, and whenever, it finds a container that is a container.
//
// What that is worth differs by path, and these tests pin down which is which:
//  * growing builds the result in a separate allocation, so a failed copy leaves the original
//    alone -- though a failed move cannot, it has already emptied what it was reading
//  * a single element is built by emplace() before anything moves, so that one is all or nothing
//  * an in place insert of several has to assign the new values over elements that are still
//    there, and once one of those fails there is no way back. All that is left then is the basic
//    guarantee, which is also all the standard asks of std::vector: the container is valid, and
//    everything before pos is still untouched.
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

// The types below all fail the same way: every operation that can throw spends from one budget, and
// the throw lands on the operation that finds it empty. Only one of them is ever in play at a time,
// so a single counter says everything four of them would.
int budget = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void tick(char const* what) {
    if (--budget < 0) {
        throw std::runtime_error(what);
    }
}

// never runs out, for setting up a container before the interesting part
constexpr auto unlimited = 1000000;

// Copying throws once the budget runs out, moving never does. This is the ordinary case: a type
// whose copy can fail because it allocates, like std::string.
struct ThrowOnCopy {
    std::string value;

    explicit ThrowOnCopy(std::string v)
        : value(std::move(v)) {}

    ThrowOnCopy(ThrowOnCopy const& other)
        : value(other.value) {
        tick("copy");
    }

    ThrowOnCopy(ThrowOnCopy&&) noexcept = default;

    // an insert copies a new element either way, into a raw slot or over a live one, so the budget
    // has to cover both or half the paths through insert_n() never see a failure
    auto operator=(ThrowOnCopy const& other) -> ThrowOnCopy& {
        tick("copy assign");
        value = other.value;
        return *this;
    }

    auto operator=(ThrowOnCopy&&) noexcept -> ThrowOnCopy& = default;
    ~ThrowOnCopy() = default;
};

static_assert(std::is_nothrow_move_constructible_v<ThrowOnCopy>);
static_assert(std::is_nothrow_move_assignable_v<ThrowOnCopy>);

// Same, but relocating it can throw too. That used to mean the tail could not be shifted back and
// was dropped instead. Nothing about the insert depends on what the move constructor promises any
// more, so this type is treated exactly like the one above.
struct ThrowOnCopyAndMove {
    std::string value;

    explicit ThrowOnCopyAndMove(std::string v)
        : value(std::move(v)) {}

    ThrowOnCopyAndMove(ThrowOnCopyAndMove const& other)
        : value(other.value) {
        tick("copy");
    }

    auto operator=(ThrowOnCopyAndMove const& other) -> ThrowOnCopyAndMove& {
        tick("copy assign");
        value = other.value;
        return *this;
    }

    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    ThrowOnCopyAndMove(ThrowOnCopyAndMove&& other)
        : value(std::move(other.value)) {}

    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    auto operator=(ThrowOnCopyAndMove&& other) -> ThrowOnCopyAndMove& {
        value = std::move(other.value);
        return *this;
    }
    ~ThrowOnCopyAndMove() = default;
};

static_assert(!std::is_nothrow_move_constructible_v<ThrowOnCopyAndMove>);

// Moving throws once the budget runs out. Only reachable while growing, where the elements have to
// be relocated into the new storage, and the one case that can leave us short of a full rollback.
struct ThrowOnMove {
    std::string value;

    explicit ThrowOnMove(std::string v)
        : value(std::move(v)) {}

    ThrowOnMove(ThrowOnMove const& other) = default;

    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    ThrowOnMove(ThrowOnMove&& other)
        : value(std::move(other.value)) {
        tick("move");
    }

    auto operator=(ThrowOnMove const&) -> ThrowOnMove& = default;
    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    auto operator=(ThrowOnMove&& other) -> ThrowOnMove& {
        value = std::move(other.value);
        return *this;
    }
    ~ThrowOnMove() = default;
};

// Assigning throws once the budget runs out, which is what shifting the tail right does.
struct ThrowOnMoveAssign {
    std::string value;

    explicit ThrowOnMoveAssign(std::string v)
        : value(std::move(v)) {}

    ThrowOnMoveAssign(ThrowOnMoveAssign const&) = default;
    ThrowOnMoveAssign(ThrowOnMoveAssign&&) noexcept = default;
    auto operator=(ThrowOnMoveAssign const&) -> ThrowOnMoveAssign& = default;

    // NOLINTNEXTLINE(performance-noexcept-move-constructor)
    auto operator=(ThrowOnMoveAssign&& other) -> ThrowOnMoveAssign& {
        tick("move assign");
        value = std::move(other.value);
        return *this;
    }
    ~ThrowOnMoveAssign() = default;
};

template <typename V>
auto make(size_t count) -> V {
    auto v = V();
    for (auto& s : make_long_strings(count)) {
        v.emplace_back(std::move(s));
    }
    return v;
}

template <typename V>
auto contents(V const& v) -> std::vector<std::string> {
    auto out = std::vector<std::string>();
    for (auto const& e : v) {
        out.push_back(e.value);
    }
    return out;
}

using SvCopy = ankerl::svector<ThrowOnCopy, 4>;

/**
 * @brief Runs an insert of count elements at pos that has to throw, and checks what is left.
 *
 * grows says the insert had to reallocate, and then the answer is simple: it was built in an
 * allocation of its own which is just dropped again, so nothing happened at all.
 *
 * In place there is no way back once an assignment over a live element has failed, and what is
 * promised is only this. Nothing in front of pos is ever read or written by an insert, so it comes
 * out exactly as it went in. Everything from pos on has to at least be readable, which is asan's
 * business rather than the comparison's. And the count is either the old one or the full new one:
 * the new elements are counted in one step each, never half way through one.
 */
template <typename Vec, typename Insert>
void require_throws_and_stays_valid(Vec& v, size_t pos, size_t count, int throw_at, Insert insert) {
    auto const before = contents(v);
    auto const grows = count > v.capacity() - v.size();

    budget = throw_at;
    REQUIRE_THROWS_AS(insert(v), std::runtime_error);
    budget = unlimited;

    auto const after = contents(v);
    if (grows) {
        REQUIRE(after == before);
        return;
    }

    REQUIRE((after.size() == before.size() || after.size() == before.size() + count));
    REQUIRE(after.size() >= pos);
    for (size_t i = 0; i < pos; ++i) {
        REQUIRE(after[i] == before[i]);
    }
    for (auto const& s : after) {
        REQUIRE(s.size() <= 40);
    }
}

} // namespace

TEST_CASE("insert_count_throwing_copy_keeps_container_valid") {
    auto const filler = ThrowOnCopy(std::string(40, 'z'));

    // sizes on both sides of the inline capacity, counts that do and do not force a reallocation
    for (size_t size = 0; size <= 9; ++size) {
        for (size_t pos = 0; pos <= size; ++pos) {
            for (size_t count = 1; count <= 5; ++count) {
                // every copy the insert makes is a place the failure can land, and which of them
                // are into raw slots and which onto live ones depends on all three loops
                for (int throw_at = 0; throw_at < static_cast<int>(count); ++throw_at) {
                    auto v = make<SvCopy>(size);
                    require_throws_and_stays_valid(v, pos, count, throw_at, [&](SvCopy& c) {
                        c.insert(c.cbegin() + pos, count, filler);
                    });
                }
            }
        }
    }
}

TEST_CASE("insert_range_throwing_copy_keeps_container_valid") {
    auto const source = std::vector<ThrowOnCopy>{
        ThrowOnCopy(std::string(40, 'x')),
        ThrowOnCopy(std::string(40, 'y')),
        ThrowOnCopy(std::string(40, 'z')),
    };

    for (size_t size = 0; size <= 9; ++size) {
        for (size_t pos = 0; pos <= size; ++pos) {
            for (int throw_at = 0; throw_at < static_cast<int>(source.size()); ++throw_at) {
                auto v = make<SvCopy>(size);
                require_throws_and_stays_valid(v, pos, source.size(), throw_at, [&](SvCopy& c) {
                    c.insert(c.cbegin() + pos, source.begin(), source.end());
                });
            }
        }
    }
}

// A single element goes through emplace(), which copies it into a temporary of its own before the
// insert starts. The copy is the only part that can fail, and by the time anything has been shifted
// it is already done, so this one is all or nothing however full the container is.
TEST_CASE("insert_single_throwing_copy_keeps_container_intact") {
    auto const filler = ThrowOnCopy(std::string(40, 'z'));

    for (size_t size = 0; size <= 9; ++size) {
        for (size_t pos = 0; pos <= size; ++pos) {
            auto v = make<SvCopy>(size);
            auto const before = contents(v);

            budget = 0;
            REQUIRE_THROWS_AS(v.insert(v.cbegin() + pos, filler), std::runtime_error);
            budget = unlimited;

            REQUIRE(contents(v) == before);
        }
    }
}

TEST_CASE("emplace_throwing_copy_keeps_container_intact") {
    auto const filler = ThrowOnCopy(std::string(40, 'z'));

    for (size_t size = 1; size <= 9; ++size) {
        for (size_t pos = 0; pos < size; ++pos) { // pos == size goes through emplace_back
            auto v = make<SvCopy>(size);
            auto const before = contents(v);

            budget = 0;
            REQUIRE_THROWS_AS(v.emplace(v.cbegin() + pos, filler), std::runtime_error);
            budget = unlimited;

            REQUIRE(contents(v) == before);
        }
    }
}

// A type whose move can throw used to be treated worse than one whose move cannot: the rollback
// could not shift the tail back down, so it dropped it and left the container short. Nothing about
// the insert depends on what the move constructor promises any more, so this type gets exactly the
// same treatment as the one above.
TEST_CASE("insert_throwing_copy_of_a_throwing_move_type_keeps_container_valid") {
    using Vec = ankerl::svector<ThrowOnCopyAndMove, 4>;

    for (size_t size = 0; size <= 9; ++size) {
        for (size_t pos = 0; pos <= size; ++pos) {
            for (size_t count = 1; count <= 3; ++count) {
                budget = unlimited;
                auto v = make<Vec>(size);
                auto const filler = ThrowOnCopyAndMove(std::string(40, 'z'));

                require_throws_and_stays_valid(v, pos, count, 0, [&](Vec& c) {
                    c.insert(c.cbegin() + pos, count, filler);
                });
            }
        }
    }
}

// Growing has to relocate our elements into the new storage, and only there can a move throw. What
// it leaves behind is spread over the new buffer with a hole in the middle, which no size can
// describe, and the size of zero it did have meant the destructor walked past all of it. See
// issue #74, where asan reported the elements moved before the throw as leaked.
TEST_CASE("insert_throwing_move_while_growing_does_not_leak") {
    using Vec = ankerl::svector<ThrowOnMove, 4>;

    // filled to the brim, so the insert below has no choice but to grow
    auto make_full = [](size_t at_least) {
        budget = unlimited;
        auto v = make<Vec>(at_least);
        while (v.size() < v.capacity()) {
            v.emplace_back(std::string(40, 'q'));
        }
        return v;
    };

    for (size_t at_least = 1; at_least <= 9; ++at_least) {
        auto const full = make_full(at_least).size();
        for (size_t pos = 0; pos <= full; ++pos) {
            // relocating is exactly one move per element, and every one of them is a place the
            // throw can land: while the front is still a hole, while the tail is going over, and
            // everywhere in between
            for (size_t spend = 0; spend < full; ++spend) {
                auto v = make_full(at_least);
                auto const filler = ThrowOnMove(std::string(40, 'z'));

                budget = static_cast<int>(spend);
                REQUIRE_THROWS_AS(v.insert(v.cbegin() + pos, filler), std::runtime_error);
                budget = unlimited;

                // we never got far enough to adopt the new storage, so our own elements are still
                // ours and still all there. Some have been moved from, so the count is all that
                // can be checked here -- that the abandoned storage leaked nothing is asan's job.
                REQUIRE(v.size() == full);
            }
        }
    }
}

// Shifting the tail right is the one step that move assigns, so it is the one step a throwing move
// assignment can interrupt. Everything it touches is a live object either way, so the container
// stays valid and destructible, just with the elements in an order nobody promised.
TEST_CASE("insert_throwing_move_assign_leaves_a_valid_container") {
    using Vec = ankerl::svector<ThrowOnMoveAssign, 8>;
    auto num_throws = 0;

    // inside the inline capacity, so the insert stays in place and does shift
    for (size_t size = 1; size <= 7; ++size) {
        for (size_t pos = 0; pos < size; ++pos) {
            // the number of assignments depends on where pos falls, so sweep past the end of it
            for (size_t spend = 0; spend <= size + 1; ++spend) {
                budget = unlimited;
                auto v = make<Vec>(size);
                auto const filler = ThrowOnMoveAssign(std::string(40, 'z'));

                budget = static_cast<int>(spend);
                try {
                    v.insert(v.cbegin() + pos, filler);
                } catch (std::runtime_error const&) {
                    ++num_throws;
                }
                budget = unlimited;

                // the tail is moved out to its new place and counted before anything is assigned,
                // so the element is part of the container whether or not it ever reached pos
                REQUIRE(v.size() == size + 1);

                // reading every one of them is the actual check, asan is what answers it. Their
                // values are whatever a half done shift left, and a moved from string can be
                // anything valid, so there is nothing to compare against.
                for (auto const& e : v) {
                    REQUIRE(e.value.size() <= 40);
                }
            }
        }
    }
    REQUIRE(num_throws > 0);
}
