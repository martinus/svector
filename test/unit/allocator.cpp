// Tests for https://github.com/martinus/svector/issues/51
//
// svector takes an allocator, and what that has to mean is the same set of rules std::vector
// follows: memory comes from the allocator, elements are built and destroyed through it when it
// says so, and the propagate_on_container_* traits decide what happens to the allocator itself on
// copy, move and swap.
//
// The one thing that is not supported is a fancy pointer. svector's iterator is a plain T*, so an
// allocator whose pointer is anything else is rejected with a static_assert rather than silently
// misused.

#include <ankerl/svector.h>

#include <doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if __has_include(<memory_resource>)
#    include <memory_resource>
#    define ANKERL_TEST_HAS_PMR 1
#endif

namespace {

// The bytes handed out by every allocator below, so a test can say that everything came back.
struct Ledger {
    size_t allocations = 0;
    size_t deallocations = 0;
    size_t bytes_live = 0;

    void allocated(size_t bytes) {
        ++allocations;
        bytes_live += bytes;
    }

    void deallocated(size_t bytes) {
        ++deallocations;
        REQUIRE(bytes_live >= bytes);
        bytes_live -= bytes;
    }

    [[nodiscard]] auto balanced() const -> bool {
        return allocations == deallocations && bytes_live == 0;
    }
};

/**
 * @brief Counts what it hands out, and is stateless: the ledger is a global.
 *
 * So it is empty, always equal, and says nothing about propagation, which is the shape almost every
 * hand written allocator has. It has to cost an svector nothing at all, see the sizeof checks.
 */
Ledger g_ledger{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template <typename T>
struct CountingAllocator {
    using value_type = T;

    CountingAllocator() = default;

    template <typename U>
    explicit CountingAllocator(CountingAllocator<U> const& /*other*/) {}

    auto allocate(size_t n) -> T* {
        g_ledger.allocated(n * sizeof(T));
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_t n) {
        g_ledger.deallocated(n * sizeof(T));
        ::operator delete(p);
    }
};

// Never called: this allocator is empty, so allocator_traits gives it is_always_equal and svector
// answers the question at compile time. It is here because an allocator is required to have it, and
// a test allocator that is not a conforming allocator would be testing the wrong thing.
template <typename T, typename U>
auto operator==(CountingAllocator<T> const& /*a*/, CountingAllocator<U> const& /*b*/) -> bool {
    return true;
}

template <typename T, typename U>
auto operator!=(CountingAllocator<T> const& a, CountingAllocator<U> const& b) -> bool {
    return !(a == b);
}

/**
 * @brief Has an identity, so two of them can disagree, and propagates by the flags it is given.
 *
 * The three propagate_on_container_* traits are template parameters so one allocator covers the
 * combinations that matter: copy assignment, move assignment and swap each have to do something
 * different depending on them.
 */
template <typename T, bool Pocca, bool Pocma, bool Pocs>
struct TaggedAllocator {
    using value_type = T;
    using propagate_on_container_copy_assignment = std::bool_constant<Pocca>;
    using propagate_on_container_move_assignment = std::bool_constant<Pocma>;
    using propagate_on_container_swap = std::bool_constant<Pocs>;
    using is_always_equal = std::false_type;

    int id = 0;
    Ledger* ledger = nullptr;

    TaggedAllocator(int identity, Ledger* l)
        : id(identity)
        , ledger(l) {}

    template <typename U>
    explicit TaggedAllocator(TaggedAllocator<U, Pocca, Pocma, Pocs> const& other)
        : id(other.id)
        , ledger(other.ledger) {}

    template <typename U>
    struct rebind {
        using other = TaggedAllocator<U, Pocca, Pocma, Pocs>;
    };

    auto allocate(size_t n) -> T* {
        ledger->allocated(n * sizeof(T));
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_t n) {
        ledger->deallocated(n * sizeof(T));
        ::operator delete(p);
    }
};

template <typename T, typename U, bool A, bool B, bool C>
auto operator==(TaggedAllocator<T, A, B, C> const& a, TaggedAllocator<U, A, B, C> const& b) -> bool {
    return a.id == b.id;
}

template <typename T, typename U, bool A, bool B, bool C>
auto operator!=(TaggedAllocator<T, A, B, C> const& a, TaggedAllocator<U, A, B, C> const& b) -> bool {
    return !(a == b);
}

/**
 * @brief Builds every element itself, which is what an svector has to notice.
 *
 * construct() adds an offset that no ordinary constructor would, so a test can tell whether an
 * element went through the allocator or around it. This is the shape std::pmr::polymorphic_allocator
 * has, minus the part where the resource is passed on.
 */
size_t g_constructed = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
size_t g_destroyed = 0;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template <typename T>
struct MarkingAllocator {
    using value_type = T;

    MarkingAllocator() = default;

    template <typename U>
    explicit MarkingAllocator(MarkingAllocator<U> const& /*other*/) {}

    auto allocate(size_t n) -> T* {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_t /*n*/) {
        ::operator delete(p);
    }

    template <typename... Args>
    void construct(T* p, Args&&... args) {
        ++g_constructed;
        ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
    }

    void destroy(T* p) {
        ++g_destroyed;
        p->~T();
    }
};

template <typename T, typename U>
auto operator==(MarkingAllocator<T> const& /*a*/, MarkingAllocator<U> const& /*b*/) -> bool {
    return true;
}

template <typename T, typename U>
auto operator!=(MarkingAllocator<T> const& a, MarkingAllocator<U> const& b) -> bool {
    return !(a == b);
}

using PoccaAll = TaggedAllocator<int, true, true, true>;
using PoccaNone = TaggedAllocator<int, false, false, false>;

// Throws while being copied once armed, for the insert that has to give its fresh allocation back.
bool g_boom_armed = false; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

struct Boom {
    int value = 0;

    Boom() = default;

    explicit Boom(int v)
        : value(v) {}

    Boom(Boom const& other)
        : value(other.value) {
        if (g_boom_armed) {
            throw std::runtime_error("copy");
        }
    }

    Boom(Boom&&) = default;
    auto operator=(Boom const&) -> Boom& = default;
    auto operator=(Boom&&) -> Boom& = default;
    ~Boom() = default;
};

} // namespace

// A stateless allocator has to be free. This is the whole point of the container, and an allocator
// stored as a member instead of an empty base would show up right here.
static_assert(sizeof(ankerl::svector<uint8_t, 1, CountingAllocator<uint8_t>>) == sizeof(ankerl::svector<uint8_t, 1>));
static_assert(sizeof(ankerl::svector<int, 7, CountingAllocator<int>>) == sizeof(ankerl::svector<int, 7>));

// A stateful one is stored, and then it costs what it costs.
static_assert(sizeof(ankerl::svector<int, 7, PoccaAll>) > sizeof(ankerl::svector<int, 7>));

static_assert(std::is_same_v<ankerl::svector<int, 7>::allocator_type, std::allocator<int>>);
static_assert(std::is_same_v<ankerl::svector<int, 7, PoccaAll>::allocator_type, PoccaAll>);

// The default parameter is what keeps every existing spelling working.
static_assert(std::is_same_v<ankerl::svector<int, 7>, ankerl::svector<int, 7, std::allocator<int>>>);

TEST_CASE("allocator_nothing_while_direct") {
    g_ledger = Ledger();
    using Vec = ankerl::svector<int, 7, CountingAllocator<int>>;

    {
        auto sv = Vec();
        for (int i = 0; i < 7; ++i) {
            sv.push_back(i);
        }
        REQUIRE(sv.size() == 7);
        REQUIRE(g_ledger.allocations == 0);

        // one past the inline capacity, and now the allocator is asked
        sv.push_back(7);
        REQUIRE(g_ledger.allocations == 1);
        REQUIRE(g_ledger.bytes_live > 0);
    }
    REQUIRE(g_ledger.balanced());
}

TEST_CASE("allocator_every_growing_path_gives_it_all_back") {
    using Vec = ankerl::svector<std::string, 3, CountingAllocator<std::string>>;

    auto check = [](auto&& body) {
        g_ledger = Ledger();
        body();
        REQUIRE(g_ledger.balanced());
        REQUIRE(g_ledger.allocations > 0);
    };

    check([] {
        auto sv = Vec();
        for (int i = 0; i < 100; ++i) {
            sv.emplace_back(std::to_string(i));
        }
    });

    check([] {
        auto sv = Vec();
        sv.reserve(1000);
    });

    check([] {
        auto sv = Vec(50, std::string("hello"));
        sv.resize(500, std::string("world"));
        sv.resize(2);
        sv.shrink_to_fit();
    });

    check([] {
        // insert_n_new(): builds the result in fresh storage and hands the old one back
        auto sv = Vec(3, std::string("x"));
        sv.insert(sv.begin() + 1, 100, std::string("y"));
        REQUIRE(sv.size() == 103);
        REQUIRE(sv[0] == "x");
        REQUIRE(sv[1] == "y");
        REQUIRE(sv[101] == "x");
    });

    check([] {
        auto src = std::vector<std::string>(80, std::string("from a range"));
        auto sv = Vec();
        sv.insert(sv.begin(), src.begin(), src.end());
        sv.insert(sv.begin() + 40, src.begin(), src.end());
        REQUIRE(sv.size() == 160);
    });

    check([] {
        auto a = Vec(40, std::string("a"));
        auto b = Vec(2, std::string("b"));
        a.swap(b); // mixed indirect / direct
        REQUIRE(a.size() == 2);
        REQUIRE(b.size() == 40);
        auto c = a;
        auto d = std::move(b);
        c = d;
        d = std::move(c);
    });
}

TEST_CASE("allocator_get_allocator_and_ctors") {
    auto ledger = Ledger();
    using Vec = ankerl::svector<int, 4, PoccaNone>;

    auto alloc = PoccaNone(7, &ledger);
    auto sv = Vec(alloc);
    REQUIRE(sv.get_allocator().id == 7);
    REQUIRE(sv.empty());

    auto counted = Vec(size_t{10}, 42, alloc);
    REQUIRE(counted.size() == 10);
    REQUIRE(counted.get_allocator().id == 7);
    REQUIRE(counted[9] == 42);

    auto sized = Vec(size_t{10}, alloc);
    REQUIRE(sized.size() == 10);
    REQUIRE(sized[9] == 0);
    REQUIRE(sized.get_allocator().id == 7);

    auto src = std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8};
    auto from_range = Vec(src.begin(), src.end(), alloc);
    REQUIRE(from_range.size() == 8);
    REQUIRE(from_range.get_allocator().id == 7);

    auto from_list = Vec({1, 2, 3, 4, 5, 6, 7, 8, 9}, alloc);
    REQUIRE(from_list.size() == 9);
    REQUIRE(from_list.get_allocator().id == 7);

    // copying carries the allocator over, which is what select_on_container_copy_construction says
    // for one that does not override it
    auto copied = from_list;
    REQUIRE(copied.get_allocator().id == 7);

    // and so does moving
    auto moved = std::move(copied);
    REQUIRE(moved.get_allocator().id == 7);

    // ... unless one is named, and then it is that one
    auto other_ledger = Ledger();
    auto extended = Vec(from_list, PoccaNone(9, &other_ledger));
    REQUIRE(extended.get_allocator().id == 9);
    REQUIRE(extended == from_list);

    auto moved_elsewhere = Vec(std::move(extended), PoccaNone(11, &other_ledger));
    REQUIRE(moved_elsewhere.get_allocator().id == 11);
    REQUIRE(moved_elsewhere == from_list);
}

TEST_CASE("allocator_propagates_when_asked") {
    auto one = Ledger();
    auto two = Ledger();
    using Vec = ankerl::svector<int, 2, PoccaAll>;

    // doctest runs everything before the SUBCASEs once per subcase, so this is a fixture
    auto a = Vec(size_t{20}, 1, PoccaAll(1, &one));
    auto b = Vec(size_t{20}, 2, PoccaAll(2, &two));

    SUBCASE("copy assignment") {
        REQUIRE(one.bytes_live > 0);

        a = b;
        REQUIRE(a.get_allocator().id == 2);
        REQUIRE(a[0] == 2);
        // a's old memory went back to the allocator it came from, not to b's
        REQUIRE(one.balanced());
    }

    SUBCASE("move assignment") {
        a = std::move(b);
        REQUIRE(a.get_allocator().id == 2);
        REQUIRE(a.size() == 20);
        REQUIRE(a[0] == 2);
        REQUIRE(one.balanced());
    }

    SUBCASE("swap") {
        a.swap(b);
        REQUIRE(a.get_allocator().id == 2);
        REQUIRE(b.get_allocator().id == 1);
        REQUIRE(a[0] == 2);
        REQUIRE(b[0] == 1);
    }
}

TEST_CASE("allocator_stays_put_when_it_does_not_propagate") {
    auto one = Ledger();
    auto two = Ledger();
    using Vec = ankerl::svector<int, 2, PoccaNone>;

    SUBCASE("copy assignment keeps ours") {
        auto a = Vec(size_t{20}, 1, PoccaNone(1, &one));
        auto b = Vec(size_t{20}, 2, PoccaNone(2, &two));

        a = b;
        REQUIRE(a.get_allocator().id == 1);
        REQUIRE(a.size() == 20);
        REQUIRE(a[0] == 2);
    }

    SUBCASE("move assignment across unequal allocators moves the elements") {
        auto a = Vec(size_t{20}, 1, PoccaNone(1, &one));
        auto b = Vec(size_t{20}, 2, PoccaNone(2, &two));
        auto const two_before = two.allocations;

        a = std::move(b);
        REQUIRE(a.get_allocator().id == 1);
        REQUIRE(a.size() == 20);
        REQUIRE(a[0] == 2);

        // b's allocation was never handed over: it is still b's to give back
        REQUIRE(two.allocations == two_before);
        REQUIRE(two.bytes_live > 0);
    }

    SUBCASE("move assignment between equal allocators still steals") {
        auto a = Vec(size_t{20}, 1, PoccaNone(1, &one));
        auto b = Vec(size_t{20}, 2, PoccaNone(1, &one));
        auto const before = one.allocations;

        a = std::move(b);
        REQUIRE(a.size() == 20);
        REQUIRE(a[0] == 2);
        REQUIRE(one.allocations == before); // nothing new was needed, the pointer just changed hands
    }
}

// The noexcept promises pick up a second condition: without propagation an unequal allocator means
// the elements have to be moved one at a time, and that can throw.
static_assert(std::is_nothrow_move_assignable_v<ankerl::svector<int, 4>>);
static_assert(std::is_nothrow_move_assignable_v<ankerl::svector<int, 4, PoccaAll>>);
static_assert(!std::is_nothrow_move_assignable_v<ankerl::svector<int, 4, PoccaNone>>);
static_assert(std::is_nothrow_swappable_v<ankerl::svector<int, 4, PoccaAll>>);
static_assert(!std::is_nothrow_swappable_v<ankerl::svector<int, 4, PoccaNone>>);

TEST_CASE("allocator_builds_the_elements_when_it_wants_to") {
    using Vec = ankerl::svector<int, 4, MarkingAllocator<int>>;
    g_constructed = 0;
    g_destroyed = 0;

    {
        auto sv = Vec();
        auto const inline_capacity = sv.capacity(); // 4 asked for, and whatever the padding allows
        for (size_t i = 0; i < inline_capacity; ++i) {
            sv.push_back(static_cast<int>(i)); // inline, but still the allocator's job to build them
        }
        REQUIRE(g_constructed == inline_capacity);
        REQUIRE(g_destroyed == 0);

        sv.push_back(1000); // grows: the ones already there are relocated through it too
        REQUIRE(g_constructed == 2 * inline_capacity + 1);
        REQUIRE(g_destroyed == inline_capacity);

        sv.insert(sv.begin(), 3, 99);
        sv.resize(50);
        sv.erase(sv.begin(), sv.begin() + 10);

        auto copy = sv;
        REQUIRE(copy == sv);
    }

    // whatever was built was taken apart again, including everything the relocations left behind
    REQUIRE(g_constructed == g_destroyed);
    REQUIRE(g_constructed > 0);
}

TEST_CASE("allocator_relocation_is_not_a_memcpy_when_it_builds_elements") {
    // int is trivially copyable and the object is small, so this is exactly the case svector would
    // relocate by copying its whole inline buffer. An allocator that builds elements itself has to
    // turn that off, or its construct() and destroy() are simply skipped.
    using Vec = ankerl::svector<int, 4, MarkingAllocator<int>>;
    g_constructed = 0;
    g_destroyed = 0;

    auto a = Vec();
    a.push_back(1);
    a.push_back(2);
    REQUIRE(g_constructed == 2);

    auto b = std::move(a);
    REQUIRE(b.size() == 2);
    REQUIRE(b[1] == 2);
    REQUIRE(g_constructed == 4);
    REQUIRE(g_destroyed == 2);

    auto c = Vec();
    c.push_back(3);
    c.swap(b);
    REQUIRE(c.size() == 2);
    REQUIRE(b.size() == 1);
    REQUIRE(b[0] == 3);
}

TEST_CASE("allocator_exception_from_a_constructor_leaks_nothing") {
    // The insert paths build the new elements before anything of ours moves, so a constructor that
    // throws there has to give the fresh allocation back.
    g_ledger = Ledger();
    {
        auto sv = ankerl::svector<Boom, 4, CountingAllocator<Boom>>();
        for (int i = 0; i < 10; ++i) {
            sv.emplace_back(i);
        }
        REQUIRE(g_ledger.allocations > 0);

        g_boom_armed = true;
        REQUIRE_THROWS_AS(sv.insert(sv.begin(), 100, Boom(1)), std::runtime_error);
        g_boom_armed = false;

        // the insert did not happen, and the storage it had already taken is gone again
        REQUIRE(sv.size() == 10);
        REQUIRE(sv[3].value == 3);
    }
    REQUIRE(g_ledger.balanced());
}

#ifdef ANKERL_TEST_HAS_PMR
TEST_CASE("allocator_pmr_passes_its_resource_on") {
    using String = std::pmr::string;
    using Vec = ankerl::svector<String, 2, std::pmr::polymorphic_allocator<String>>;

    auto buffer = std::array<std::byte, 64 * 1024>();
    auto resource = std::pmr::monotonic_buffer_resource(buffer.data(), buffer.size(), std::pmr::null_memory_resource());

    auto sv = Vec(&resource);
    REQUIRE(sv.get_allocator().resource() == &resource);

    for (int i = 0; i < 50; ++i) {
        // long enough that the string has to allocate, which is what shows where it got its memory
        sv.emplace_back("a string too long for any small string optimization to hold on to");
    }

    // every element was built through the allocator, so every one of them is on the arena
    for (auto const& s : sv) {
        REQUIRE(s.get_allocator().resource() == &resource);
    }

    // and so are the ones the growth path relocated, and the ones an insert copies
    sv.insert(sv.begin() + 1, 20, String("another string that is far too long to fit inline", &resource));
    REQUIRE(sv.size() == 70);
    for (auto const& s : sv) {
        REQUIRE(s.get_allocator().resource() == &resource);
    }

    sv.resize(120, String("filled in by resize", &resource));
    for (auto const& s : sv) {
        REQUIRE(s.get_allocator().resource() == &resource);
    }

    // emplace() builds a plain local first, see there, but what lands in the container is still
    // constructed through the allocator and so is still on the arena
    sv.emplace(sv.begin(), "built by emplace, and long enough to have to allocate for itself");
    sv.emplace(sv.end(), "built by emplace at the end, also much too long to fit inline");
    sv.insert(sv.begin() + 2, String("a single inserted string, long enough to allocate", &resource));
    REQUIRE(sv.size() == 123);
    for (auto const& s : sv) {
        REQUIRE(s.get_allocator().resource() == &resource);
    }

    // and nothing on that arena was ever handed back to the wrong place: null_memory_resource() is
    // the upstream, so a single byte taken from anywhere else would have thrown by now
    REQUIRE(sv.front() == "built by emplace, and long enough to have to allocate for itself");
}
#endif
