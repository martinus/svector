#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>

#include <vector>

template <typename V>
void test_swap() {
    Counter counts;
    INFO(counts);

    auto a = V();
    auto b = V();

    for (size_t i = 0; i < 3; ++i) {
        a.emplace_back(i, counts);
    }
    REQUIRE(counts.ctor == 3);
    counts("before swap");
    a.swap(b);
    counts("after swap");
    REQUIRE(counts.ctor == 3);
    REQUIRE(a.empty());
    REQUIRE(b.size() == 3);

    for (size_t i = 0; i < 100; ++i) {
        a.emplace_back(111 + i, counts);
    }
    REQUIRE(a.size() == 100);
    REQUIRE(b.size() == 3);
    a.swap(b);
    REQUIRE(a.size() == 3);
    REQUIRE(b.size() == 100);
    REQUIRE(a[0].get() == 0);
    REQUIRE(b.front().get() == 111);

    for (size_t i = 0; i < 100; ++i) {
        a.emplace_back(999 + i, counts);
    }
    auto total_before = counts.total();
    REQUIRE(a.size() == 103);
    REQUIRE(b.size() == 100);

    b.swap(a);
    REQUIRE(counts.total() == total_before);
    REQUIRE(a.size() == 100);
    REQUIRE(b.size() == 103);
    REQUIRE(a.back().get() == 111 + 99);
    REQUIRE(b.back().get() == 999 + 99);

    total_before = counts.total();
    std::swap(a, b);
    REQUIRE(counts.total() == total_before);
    REQUIRE(b.size() == 100);
    REQUIRE(a.size() == 103);
    REQUIRE(b.back().get() == 111 + 99);
    REQUIRE(a.back().get() == 999 + 99);
}

TEST_CASE("swap_stdvector") {
    test_swap<std::vector<Counter::Obj>>();
}

TEST_CASE("swap_svector") {
    test_swap<ankerl::svector<Counter::Obj, 2>>();
}

namespace {

// One byte, and not trivially copyable, so it takes the element by element path and its inline
// storage starts at offset 1 -- inside the bytes the indirect mode pointer occupies. Every other
// element type in the suite is 8 aligned, which puts its inline storage past the pointer and hides
// whether swap reads that pointer before or after moving elements on top of it.
struct Tiny {
    uint8_t value;

    explicit Tiny(size_t v)
        : value(static_cast<uint8_t>(v)) {}
    Tiny(Tiny const& other) // NOLINT(modernize-use-equals-default)
        : value(other.value) {}
    auto operator=(Tiny const& other) -> Tiny& { // NOLINT(modernize-use-equals-default,cert-oop54-cpp)
        value = other.value;
        return *this;
    }
    ~Tiny() {} // NOLINT(modernize-use-equals-default)
};

static_assert(!std::is_trivially_copyable_v<Tiny>);
static_assert(alignof(Tiny) < sizeof(void*));

/**
 * @brief Swaps every combination of the two modes, in both size orders.
 *
 * swap() has a case per mode combination rather than one path, so each element type has to walk
 * all of them. Sizes run past the inline capacity on both sides, which covers direct/direct,
 * direct/indirect, indirect/direct and indirect/indirect, and the two orders of each.
 *
 * Calls the member. A qualified std::swap(a, b) would not reach it -- name lookup does not consider
 * ankerl::swap for a qualified call -- so it would quietly test the generic three move template
 * instead, which is what happened to an earlier version of this test.
 */
template <typename Vec, typename Make, typename Get>
void test_all_mode_combinations(Make make, Get get) {
    auto const n = Vec().capacity(); // above this the elements move to the heap
    REQUIRE(n >= 2);

    for (size_t sa = 0; sa <= n * 2; ++sa) {
        for (size_t sb = 0; sb <= n * 2; ++sb) {
            auto a = Vec();
            auto b = Vec();
            for (size_t i = 0; i < sa; ++i) {
                a.emplace_back(make(i));
            }
            for (size_t i = 0; i < sb; ++i) {
                b.emplace_back(make(100 + i));
            }

            a.swap(b);

            REQUIRE(a.size() == sb);
            REQUIRE(b.size() == sa);
            for (size_t i = 0; i < sb; ++i) {
                REQUIRE(get(a[i]) == 100 + i);
            }
            for (size_t i = 0; i < sa; ++i) {
                REQUIRE(get(b[i]) == i);
            }
        }
    }
}

} // namespace

// Not trivially copyable, so element by element.
TEST_CASE("swap_every_mode_combination") {
    Counter counts;
    test_all_mode_combinations<ankerl::svector<Counter::Obj, 4>>(
        [&](size_t i) {
            return Counter::Obj(i, counts);
        },
        [](Counter::Obj const& o) {
            return o.get();
        });
}

// Trivially copyable, so the whole inline buffer goes at once.
TEST_CASE("swap_every_mode_combination_trivial") {
    test_all_mode_combinations<ankerl::svector<size_t, 4>>(
        [](size_t i) {
            return i;
        },
        [](size_t v) {
            return v;
        });
}

// Inline storage overlapping the bytes the indirect pointer lives in.
TEST_CASE("swap_every_mode_combination_overlapping_pointer") {
    test_all_mode_combinations<ankerl::svector<Tiny, 1>>(
        [](size_t i) {
            return Tiny(i);
        },
        [](Tiny const& t) {
            return size_t{t.value};
        });
}

// The free swap() only earns its keep if an unqualified call finds it, which is the form every
// standard algorithm uses internally. Exchanging elements in place move assigns them; three
// container moves only ever move construct. So a non-zero moveAssign is proof the member ran.
TEST_CASE("unqualified_swap_finds_the_member") {
    Counter counts;
    auto a = ankerl::svector<Counter::Obj, 4>();
    auto b = ankerl::svector<Counter::Obj, 4>();
    for (size_t i = 0; i < 3; ++i) {
        a.emplace_back(i, counts);
        b.emplace_back(100 + i, counts);
    }
    REQUIRE(counts.moveAssign == 0);

    using std::swap;
    swap(a, b); // unqualified, so argument dependent lookup reaches ankerl::swap

    REQUIRE(counts.moveAssign > 0);
    REQUIRE(a[0].get() == 100);
    REQUIRE(b[0].get() == 0);
}

TEST_CASE("swap_with_itself_does_nothing") {
    Counter counts;
    auto a = ankerl::svector<Counter::Obj, 4>();
    for (size_t i = 0; i < 3; ++i) {
        a.emplace_back(i, counts);
    }

    auto const total_before = counts.total();
    a.swap(a);
    REQUIRE(counts.total() == total_before);

    REQUIRE(a.size() == 3);
    for (size_t i = 0; i < 3; ++i) {
        REQUIRE(a[i].get() == i);
    }
}
