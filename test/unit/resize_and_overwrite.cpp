#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>

#include <cstddef>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>

namespace {

// Constructs elements [from, to) in place, like op is supposed to do for the raw part of the range.
void construct_range(Counter::Obj* p, size_t from, size_t to, Counter& counts) {
    for (size_t i = from; i < to; ++i) {
        // cast to void* so we don't pick the poisoned operator new(size_t, Counter::Obj*) overload
        ::new (static_cast<void*>(p + i)) Counter::Obj(i, counts);
    }
}

} // namespace

TEST_CASE("resize_and_overwrite_op_receives_count") {
    // op(p, n) must get n == count, exactly like std::string::resize_and_overwrite.
    // Passing the old size instead leaves op unable to tell how much space it may write to.
    auto a = ankerl::svector<char, 4>();
    a.resize(3, 'x');

    size_t seen = 0;
    a.resize_and_overwrite(20, [&](char* p, size_t n) -> size_t {
        seen = n;
        for (size_t i = 3; i < n; ++i) {
            p[i] = 'y';
        }
        return n;
    });

    REQUIRE(seen == 20);
    REQUIRE(a.size() == 20);
    REQUIRE(a[0] == 'x');
    REQUIRE(a[2] == 'x');
    REQUIRE(a[3] == 'y');
    REQUIRE(a[19] == 'y');

    // shrinking too: n is the requested count, not the old size
    a.resize_and_overwrite(5, [&](char* /*p*/, size_t n) -> size_t {
        seen = n;
        return n;
    });
    REQUIRE(seen == 5);
    REQUIRE(a.size() == 5);
}

TEST_CASE("resize_and_overwrite_grow_within_inline_capacity") {
    auto a = ankerl::svector<char, 30>();
    auto const inline_capacity = a.capacity();
    a.resize(2, 'a');

    a.resize_and_overwrite(10, [](char* p, size_t n) -> size_t {
        for (size_t i = 2; i < n; ++i) {
            p[i] = 'b';
        }
        return n;
    });

    REQUIRE(a.size() == 10);
    REQUIRE(a.capacity() == inline_capacity); // no reallocation, still direct
    REQUIRE(a[0] == 'a');
    REQUIRE(a[9] == 'b');
}

TEST_CASE("resize_and_overwrite_grow_beyond_inline_capacity") {
    auto a = ankerl::svector<int, 4>();
    for (int i = 0; i < 3; ++i) {
        a.push_back(i);
    }

    a.resize_and_overwrite(1000, [](int* p, size_t n) -> size_t {
        for (size_t i = 3; i < n; ++i) {
            p[i] = static_cast<int>(i);
        }
        return n;
    });

    REQUIRE(a.size() == 1000);
    REQUIRE(a.capacity() >= 1000);
    for (size_t i = 0; i < a.size(); ++i) {
        REQUIRE(a[i] == static_cast<int>(i)); // the original 3 survived the reallocation
    }
}

TEST_CASE("resize_and_overwrite_returns_less_than_count") {
    auto a = ankerl::svector<char, 4>();
    a.resize_and_overwrite(100, [](char* p, size_t /*n*/) -> size_t {
        for (size_t i = 0; i < 7; ++i) {
            p[i] = static_cast<char>('a' + i);
        }
        return 7; // only kept 7 of the 100 we asked for
    });

    REQUIRE(a.size() == 7);
    REQUIRE(a[0] == 'a');
    REQUIRE(a[6] == 'g');
}

TEST_CASE("resize_and_overwrite_to_zero_and_empty") {
    auto a = ankerl::svector<char, 4>();
    a.resize(10, 'x');

    a.resize_and_overwrite(0, [](char* /*p*/, size_t n) -> size_t {
        REQUIRE(n == 0);
        return 0;
    });
    REQUIRE(a.empty());

    // and on an already empty vector
    a.resize_and_overwrite(0, [](char* /*p*/, size_t /*n*/) -> size_t {
        return 0;
    });
    REQUIRE(a.empty());
}

TEST_CASE("resize_and_overwrite_nontrivial_grow") {
    Counter counts;
    {
        auto a = ankerl::svector<Counter::Obj, 3>();
        for (size_t i = 0; i < 2; ++i) {
            a.emplace_back(i, counts);
        }

        a.resize_and_overwrite(50, [&](Counter::Obj* p, size_t n) -> size_t {
            REQUIRE(n == 50);
            REQUIRE(p[0].get() == 0); // existing elements are readable
            REQUIRE(p[1].get() == 1);
            construct_range(p, 2, n, counts);
            return n;
        });

        REQUIRE(a.size() == 50);
        for (size_t i = 0; i < a.size(); ++i) {
            REQUIRE(a[i].get() == i);
        }
    }
    counts.check_all_done(); // every constructed object destroyed exactly once
}

TEST_CASE("resize_and_overwrite_nontrivial_shrink") {
    Counter counts;
    {
        auto a = ankerl::svector<Counter::Obj, 3>();
        for (size_t i = 0; i < 30; ++i) {
            a.emplace_back(i, counts);
        }

        a.resize_and_overwrite(4, [](Counter::Obj* p, size_t n) -> size_t {
            REQUIRE(n == 4);
            REQUIRE(p[3].get() == 3); // the surviving prefix is untouched
            return n;
        });

        REQUIRE(a.size() == 4);
        for (size_t i = 0; i < a.size(); ++i) {
            REQUIRE(a[i].get() == i);
        }
    }
    counts.check_all_done(); // the 26 dropped elements destroyed exactly once, not twice
}

TEST_CASE("resize_and_overwrite_throwing_op_while_shrinking") {
    // The tail is destroyed before op runs. If the size were not committed first, the
    // destructor would destroy those elements a second time.
    Counter counts;
    {
        auto a = ankerl::svector<Counter::Obj, 3>();
        for (size_t i = 0; i < 30; ++i) {
            a.emplace_back(i, counts);
        }

        REQUIRE_THROWS_AS(a.resize_and_overwrite(4,
                                                 [](Counter::Obj* /*p*/, size_t /*n*/) -> size_t {
                                                     throw std::runtime_error("boom");
                                                 }),
                          std::runtime_error);

        // the vector is still usable and holds exactly the elements that survived
        REQUIRE(a.size() == 4);
        for (size_t i = 0; i < a.size(); ++i) {
            REQUIRE(a[i].get() == i);
        }
    }
    counts.check_all_done();
}

TEST_CASE("resize_and_overwrite_throwing_op_while_growing") {
    // op throws after we reallocated. Everything that existed before must still be alive
    // and get destroyed exactly once.
    Counter counts;
    {
        auto a = ankerl::svector<Counter::Obj, 3>();
        for (size_t i = 0; i < 5; ++i) {
            a.emplace_back(i, counts);
        }

        REQUIRE_THROWS_AS(a.resize_and_overwrite(500,
                                                 [](Counter::Obj* /*p*/, size_t /*n*/) -> size_t {
                                                     throw std::runtime_error("boom");
                                                 }),
                          std::runtime_error);

        REQUIRE(a.size() == 5);
        REQUIRE(a.capacity() >= 500); // the reserve did happen
        for (size_t i = 0; i < a.size(); ++i) {
            REQUIRE(a[i].get() == i);
        }
    }
    counts.check_all_done();
}

TEST_CASE("resize_and_overwrite_skips_initialization") {
    // The whole point: unlike resize(), the new elements are not value initialized.
    Counter counts;
    {
        auto a = ankerl::svector<Counter::Obj, 3>();
        auto const before = counts.defaultCtor;

        a.resize_and_overwrite(20, [&](Counter::Obj* p, size_t n) -> size_t {
            construct_range(p, 0, n, counts);
            return n;
        });

        // op constructed all 20 itself, the container default constructed none of them
        REQUIRE(counts.defaultCtor == before);
        REQUIRE(a.size() == 20);
    }
    counts.check_all_done();
}

TEST_CASE("resize_and_overwrite_matches_read_into_buffer_pattern") {
    // typical use: size the buffer up, fill part of it, report how much was actually written
    auto const source = std::string("hello world");

    auto buf = ankerl::svector<char, 8>();
    buf.resize_and_overwrite(source.size() + 100, [&](char* p, size_t n) -> size_t {
        REQUIRE(n == source.size() + 100);
        std::char_traits<char>::copy(p, source.data(), source.size());
        return source.size();
    });

    REQUIRE(buf.size() == source.size());
    REQUIRE(std::string(buf.begin(), buf.end()) == source);
}
