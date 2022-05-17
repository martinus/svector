#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>

TEST_CASE("shrink_to_fit") {
    auto a = ankerl::svector<char, 7>();
    auto c = a.capacity();
    a.shrink_to_fit();
    REQUIRE(a.capacity() == c);

    a.reserve(1000);
    REQUIRE(a.capacity() == 7 * 256);
    a.shrink_to_fit();
    REQUIRE(a.capacity() == c);

    for (size_t i = 0; i < 1000; ++i) {
        a.push_back(static_cast<char>(i));
        c = a.capacity();
        a.shrink_to_fit();
        REQUIRE(c == a.capacity());
        REQUIRE(a.size() == i + 1);
    }
}

TEST_CASE("shrink_to_fit_counts") {
    Counter counts;
    INFO(counts);

    auto a = ankerl::svector<Counter::Obj, 3>();

    // grow
    for (size_t i = 0; i < 1000; ++i) {
        a.emplace_back(i, counts);
        auto c = a.capacity();
        auto total_before = counts.total();
        // counts("before shrink");
        a.shrink_to_fit();
        // counts("after shrink");
        REQUIRE(counts.total() == total_before);
        REQUIRE(c == a.capacity());
        REQUIRE(a.size() == i + 1);
    }

    // shrink
    auto num_shrinks = 0;
    for (size_t i = 0; i < 1000; ++i) {
        // counts("pop before shrink");
        auto dtors_before = counts.dtor;
        auto total_before = counts.total();
        a.pop_back();
        REQUIRE(counts.dtor == dtors_before + 1);
        REQUIRE(counts.total() == total_before + 1);
        REQUIRE(a.size() == 1000 - i - 1);

        auto c = a.capacity();
        a.shrink_to_fit();
        if (a.capacity() < c) {
            ++num_shrinks;
        }
        REQUIRE(a.size() == 1000 - i - 1);
    }
    REQUIRE(a.capacity() == 3);
    REQUIRE(num_shrinks == 9);
}

TEST_CASE("shrink_2") {
    Counter counts;
    INFO(counts);

    auto a = ankerl::svector<Counter::Obj, 3>();

    counts("reserve");
    a.reserve(1000);
    counts("after reserve");
    REQUIRE(counts.total() == 0);
    for (size_t i = 0; i < 100; ++i) {
        a.emplace_back(i + 123, counts);
    }
    counts("after emplace 100");
    REQUIRE(counts.total() == 100);
    REQUIRE(counts.ctor == 100);
    REQUIRE(a.capacity() == (3U << 9U));

    // now shrink back to 3*2^6
    a.shrink_to_fit();
    counts("after shrink");
    REQUIRE(a.capacity() == (3U << 6U));
    REQUIRE(counts.dtor == 100);
    REQUIRE(counts.moveCtor == 100);
    REQUIRE(counts.total() == 300); // 100 ctor + 100 dtor + 100 moveCtor

    a.shrink_to_fit();
    REQUIRE(counts.total() == 300); // 100 ctor + 100 dtor + 100 moveCtor

    for (size_t i = 0; i < 100; ++i) {
        REQUIRE(a[i].get() == 123 + i);
    }

    // pop back until only 3 are left
    while (a.size() > 3) {
        a.pop_back();
    }
    counts("after popback");
    REQUIRE(counts.dtor == 100 + 97);
    a.shrink_to_fit();
    REQUIRE(a.capacity() == 3);
    REQUIRE(counts.dtor == 100 + 97 + 3);
    a.shrink_to_fit();
    REQUIRE(counts.dtor == 100 + 97 + 3);
    a.pop_back();
    REQUIRE(counts.dtor == 100 + 97 + 3 + 1);
    a.shrink_to_fit();
    REQUIRE(counts.dtor == 100 + 97 + 3 + 1);
    a.clear();
    REQUIRE(counts.dtor == 100 + 97 + 3 + 3);
    a.shrink_to_fit();
    REQUIRE(counts.dtor == 100 + 97 + 3 + 3);
    REQUIRE(a.capacity() == 3);
}
