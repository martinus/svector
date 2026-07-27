// Tests for https://github.com/martinus/svector/issues/54
//
// Moving an svector relocates whatever is in its inline buffer. For trivially copyable elements
// that is a plain byte copy of the whole object, otherwise every element has to be move
// constructed one by one. Both paths have to end up with the same observable result: the target
// holds the elements, the source is left empty and usable, and nothing is leaked or destroyed
// twice.

#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

namespace {

// Not relocatable by copying bytes: it holds a pointer to its own member, so a byte copy leaves
// the copy pointing into the source. A libstdc++ std::string in its small string mode does
// exactly this, this is just the same thing in miniature and without a heap allocation to hide it.
class SelfRef {
    int m_value;
    int* m_ptr;

public:
    explicit SelfRef(int value)
        : m_value(value)
        , m_ptr(&m_value) {}

    SelfRef(SelfRef const& other)
        : m_value(other.m_value)
        , m_ptr(&m_value) {}

    // declaring a move constructor already suppresses both assignment operators, which is all the
    // tests below need: they only ever construct and destroy
    SelfRef(SelfRef&& other) noexcept
        : m_value(other.m_value)
        , m_ptr(&m_value) {}

    // reads through the self pointer, so a byte relocated object reads the source's storage
    [[nodiscard]] auto get() const -> int {
        return *m_ptr;
    }

    [[nodiscard]] auto points_to_itself() const -> bool {
        return m_ptr == &m_value;
    }
};

static_assert(!std::is_trivially_copyable_v<SelfRef>, "otherwise this type tests nothing");

// Move constructs an inline {1, 2, 3} and checks that the elements arrive. data() has to change:
// the source was inline, so its elements are relocated into the target's own buffer instead of an
// allocation changing hands.
template <typename Vec>
void check_move_ctor_from_inline() {
    auto a = Vec{1, 2, 3};
    auto const* a_data = a.data();

    auto b = std::move(a);
    REQUIRE(b.data() != a_data);
    REQUIRE(b.size() == 3);
    REQUIRE(b[0] == 1);
    REQUIRE(b[1] == 2);
    REQUIRE(b[2] == 3);
    REQUIRE(a.empty()); // NOLINT(hicpp-invalid-access-moved,bugprone-use-after-move)
}

} // namespace

TEST_CASE("move_ctor_direct_trivial") {
    // small enough that the whole buffer is copied in one go
    check_move_ctor_from_inline<ankerl::svector<int, 4>>();
}

TEST_CASE("move_ctor_direct_big_inline_capacity") {
    // svector<int, 100> is far past the size cutoff, so this takes the element by element path
    // even though int is trivially copyable
    check_move_ctor_from_inline<ankerl::svector<int, 100>>();
}

TEST_CASE("move_ctor_indirect_steals_allocation") {
    auto a = ankerl::svector<int, 4>();
    for (int i = 0; i < 100; ++i) {
        a.push_back(i);
    }
    auto const* ptr = a.data();

    auto b = std::move(a);
    REQUIRE(b.data() == ptr); // no copy, the allocation itself moved over
    REQUIRE(b.size() == 100);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(b[i] == i);
    }
    REQUIRE(a.empty()); // NOLINT(hicpp-invalid-access-moved,bugprone-use-after-move)
}

TEST_CASE("move_assign_all_four_mode_combinations") {
    auto make = [](size_t count) -> ankerl::svector<int, 4> {
        auto v = ankerl::svector<int, 4>();
        for (size_t i = 0; i < count; ++i) {
            v.push_back(static_cast<int>(i));
        }
        return v;
    };

    // small stays inline, large is heap allocated
    for (auto target_count : {size_t{2}, size_t{100}}) {
        for (auto source_count : {size_t{0}, size_t{3}, size_t{100}}) {
            auto target = make(target_count);
            auto source = make(source_count);

            target = std::move(source);
            REQUIRE(target.size() == source_count);
            for (size_t i = 0; i < source_count; ++i) {
                REQUIRE(target[i] == static_cast<int>(i));
            }
            REQUIRE(source.empty()); // NOLINT(hicpp-invalid-access-moved,bugprone-use-after-move)

            // the moved from vector is empty but fully usable
            source.push_back(42); // NOLINT(hicpp-invalid-access-moved,bugprone-use-after-move)
            REQUIRE(source.size() == 1);
            REQUIRE(source[0] == 42);
        }
    }
}

TEST_CASE("move_self_referencing_element") {
    // SelfRef has to be move constructed so it can fix up its pointer, see above. The source goes
    // out of scope before the checks, so a byte copy leaves the elements pointing at dead storage.
    auto b = ankerl::svector<SelfRef, 4>();
    {
        auto a = ankerl::svector<SelfRef, 4>();
        a.emplace_back(11);
        a.emplace_back(22);

        b = std::move(a);
    }

    REQUIRE(b.size() == 2);
    REQUIRE(b[0].points_to_itself());
    REQUIRE(b[1].points_to_itself());
    REQUIRE(b[0].get() == 11);
    REQUIRE(b[1].get() == 22);
}

TEST_CASE("move_self_referencing_element_ctor") {
    auto a = ankerl::svector<SelfRef, 4>();
    a.emplace_back(7);

    auto b = std::move(a);
    REQUIRE(b.size() == 1);
    REQUIRE(b[0].points_to_itself());
    REQUIRE(b[0].get() == 7);
}

TEST_CASE("move_small_strings") {
    // the real world instance of SelfRef: a short libstdc++ std::string is self referencing too
    auto const short_string = std::string("short"); // no heap allocation

    auto b = ankerl::svector<std::string, 4>();
    {
        auto a = ankerl::svector<std::string, 4>();
        a.push_back(short_string);
        a.push_back(short_string);
        b = std::move(a);
    }

    REQUIRE(b.size() == 2);
    REQUIRE(b[0] == short_string);
    REQUIRE(b[1] == short_string);
}

TEST_CASE("move_leaves_no_leaks") {
    Counter counts;
    {
        auto a = ankerl::svector<Counter::Obj, 3>();
        a.emplace_back(1, counts);
        a.emplace_back(2, counts);

        auto b = std::move(a); // direct
        REQUIRE(b.size() == 2);

        for (size_t i = 0; i < 100; ++i) {
            b.emplace_back(i, counts);
        }
        auto c = std::move(b); // indirect
        REQUIRE(c.size() == 102);

        // move back into a moved from vector, and into itself
        a = std::move(c); // NOLINT(hicpp-invalid-access-moved,bugprone-use-after-move)
        REQUIRE(a.size() == 102);
    }
    counts.check_all_done();
}

TEST_CASE("move_empty") {
    auto a = ankerl::svector<int, 4>();
    auto b = std::move(a);
    REQUIRE(b.empty());
    REQUIRE(a.empty()); // NOLINT(hicpp-invalid-access-moved,bugprone-use-after-move)

    auto c = ankerl::svector<SelfRef, 4>();
    auto d = std::move(c);
    REQUIRE(d.empty());
    REQUIRE(c.empty()); // NOLINT(hicpp-invalid-access-moved,bugprone-use-after-move)
}
