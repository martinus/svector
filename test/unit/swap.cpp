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

// swap() has a case for each combination of the two modes rather than going through three whole
// container moves, so every combination needs to be walked, in both size orders. Counter::Obj is
// not trivially copyable, so this is the element by element path.
TEST_CASE("swap_every_mode_combination") {
    Counter counts;
    using Vec = ankerl::svector<Counter::Obj, 4>;
    auto const n = Vec().capacity(); // above this the elements move to the heap
    REQUIRE(n >= 2);

    for (size_t sa = 0; sa <= n * 2; ++sa) {
        for (size_t sb = 0; sb <= n * 2; ++sb) {
            auto a = Vec();
            auto b = Vec();
            for (size_t i = 0; i < sa; ++i) {
                a.emplace_back(i, counts);
            }
            for (size_t i = 0; i < sb; ++i) {
                b.emplace_back(1000 + i, counts);
            }

            a.swap(b);

            REQUIRE(a.size() == sb);
            REQUIRE(b.size() == sa);
            for (size_t i = 0; i < sb; ++i) {
                REQUIRE(a[i].get() == 1000 + i);
            }
            for (size_t i = 0; i < sa; ++i) {
                REQUIRE(b[i].get() == i);
            }
        }
    }
}

// Trivially copyable elements take the whole buffer at once instead, so they need the same walk.
TEST_CASE("swap_every_mode_combination_trivial") {
    using Vec = ankerl::svector<int, 4>;
    auto const n = Vec().capacity();

    for (size_t sa = 0; sa <= n * 2; ++sa) {
        for (size_t sb = 0; sb <= n * 2; ++sb) {
            auto a = Vec();
            auto b = Vec();
            for (size_t i = 0; i < sa; ++i) {
                a.emplace_back(static_cast<int>(i));
            }
            for (size_t i = 0; i < sb; ++i) {
                b.emplace_back(static_cast<int>(1000 + i));
            }

            // through std::swap, which finds the free function by argument dependent lookup
            std::swap(a, b);

            REQUIRE(a.size() == sb);
            REQUIRE(b.size() == sa);
            for (size_t i = 0; i < sb; ++i) {
                REQUIRE(a[i] == static_cast<int>(1000 + i));
            }
            for (size_t i = 0; i < sa; ++i) {
                REQUIRE(b[i] == static_cast<int>(i));
            }
        }
    }
}

namespace {

// One byte, and not trivially copyable, so it takes the element by element path and its inline
// storage starts at offset 1 -- inside the bytes the indirect mode pointer occupies. Every other
// element type in the suite is 8 aligned, which puts its inline storage past the pointer and hides
// whether swap reads that pointer before or after moving elements on top of it.
struct Tiny {
    uint8_t value;

    explicit Tiny(uint8_t v)
        : value(v) {}
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

} // namespace

TEST_CASE("swap_mixed_modes_when_elements_overlap_the_pointer") {
    using Vec = ankerl::svector<Tiny, 1>;
    auto const n = Vec().capacity();
    REQUIRE(n > 1);

    // one side stays inline, the other is on the heap, in both orders
    for (size_t direct_size = 0; direct_size <= n; ++direct_size) {
        for (size_t indirect_size = n + 1; indirect_size <= n + 4; ++indirect_size) {
            for (auto const direct_first : {true, false}) {
                auto small = Vec();
                auto big = Vec();
                for (size_t i = 0; i < direct_size; ++i) {
                    small.emplace_back(static_cast<uint8_t>(i));
                }
                for (size_t i = 0; i < indirect_size; ++i) {
                    big.emplace_back(static_cast<uint8_t>(100 + i));
                }
                REQUIRE(small.size() == direct_size);
                REQUIRE(big.size() == indirect_size);

                if (direct_first) {
                    small.swap(big);
                } else {
                    big.swap(small);
                }

                REQUIRE(small.size() == indirect_size);
                REQUIRE(big.size() == direct_size);
                for (size_t i = 0; i < indirect_size; ++i) {
                    REQUIRE(small[i].value == static_cast<uint8_t>(100 + i));
                }
                for (size_t i = 0; i < direct_size; ++i) {
                    REQUIRE(big[i].value == static_cast<uint8_t>(i));
                }
            }
        }
    }
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
