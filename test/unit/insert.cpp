#include <ankerl/svector.h>
#include <app/Counter.h>
#include <app/VecTester.h>

#include <doctest.h>
#include <fmt/format.h>

#include <forward_list>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

TEST_CASE("insert_single") {
    // extremely similar code to "emplace"

    for (size_t s = 0; s < 6; ++s) {
        auto vec_a = ankerl::svector<std::string, 4>();
        auto vec_b = std::vector<std::string>();
        for (size_t ins = 0; ins < s; ++ins) {
            vec_a.emplace_back(std::to_string(ins));
            vec_b.emplace_back(std::to_string(ins));
        }

        for (size_t i = 0; i <= vec_a.size(); ++i) {
            auto va = vec_a;
            auto vb = vec_b;

            // moved
            auto* ita = va.insert(va.cbegin() + std::ptrdiff_t(i), "999");
            auto itb = vb.insert(vb.cbegin() + std::ptrdiff_t(i), "999");
            REQUIRE(*ita == *itb);
            *ita += 'x';
            *itb += 'x';

            // constexpr => copied
            std::string x = "asdf";
            ita = va.insert(va.cbegin() + i, x);
            itb = vb.insert(vb.cbegin() + std::ptrdiff_t(i), x);
            REQUIRE(*ita == x);
            *ita += 'g';
            *itb += 'g';

            assert_eq(va, vb);
        }
    }
}

// iterator insert( const_iterator pos, size_type count, const T& value );
TEST_CASE("insert_copies") {
    Counter counts;
    INFO(counts);

    for (size_t s = 0; s < 6; ++s) {
        auto vec_a = ankerl::svector<Counter::Obj, 4>();
        auto vec_b = std::vector<Counter::Obj>();
        for (size_t ins = 0; ins < s; ++ins) {
            vec_a.emplace_back(ins, counts);
            vec_b.emplace_back(ins, counts);
        }

        for (size_t i = 0; i <= vec_a.size(); ++i) {
            auto va = vec_a;
            auto vb = vec_b;

            // moved
            auto c = Counter::Obj(999, counts);
            auto* ita = va.insert(va.cbegin() + i, 7, c);
            auto itb = vb.insert(vb.cbegin() + std::ptrdiff_t(i), 7, c);
            REQUIRE(*ita == *itb);
            assert_eq(va, vb);
        }
    }
}

template <class T>
class FooInputIterator {
    T* m_ptr{};

public:
    using iterator_category = std::input_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    explicit FooInputIterator(T* ptr)
        : m_ptr(ptr) {}

    auto operator*() const -> T& {
        return *m_ptr;
    }
    auto operator->() const -> T* {
        return m_ptr;
    }
    auto operator++() -> FooInputIterator& {
        ++m_ptr;
        return *this;
    }
    auto operator++(int) -> FooInputIterator {
        return FooInputIterator(m_ptr++);
    }
    friend auto operator!=(FooInputIterator const& a, FooInputIterator const& b) {
        return a.m_ptr != b.m_ptr;
    }
    friend auto operator==(FooInputIterator const& a, FooInputIterator const& b) {
        return a.m_ptr == b.m_ptr;
    }
};

// template< class InputIt > iterator insert(const_iterator pos, InputIt first, InputIt last);
TEST_CASE("insert_input_iterator") {
    Counter counts;
    INFO(counts);

    auto va = std::vector<Counter::Obj>();
    auto vb = ankerl::svector<Counter::Obj, 4>();
    va.emplace_back(1, counts);
    vb.emplace_back(1, counts);
    va.emplace_back(2, counts);
    vb.emplace_back(2, counts);
    va.emplace_back(3, counts);
    vb.emplace_back(3, counts);
    assert_eq(va, vb);

    auto data = std::array<Counter::Obj, 5>{{
        Counter::Obj(10, counts),
        Counter::Obj(11, counts),
        Counter::Obj(12, counts),
        Counter::Obj(13, counts),
        Counter::Obj(14, counts),
    }};

    auto it_begin = FooInputIterator<Counter::Obj>(data.data());
    auto it_end = FooInputIterator<Counter::Obj>(data.data() + data.size());

    counts("before insert");
    assert_eq(va, vb);
    va.insert(va.begin() + 1, it_begin, it_end);
    vb.insert(vb.begin() + 1, it_begin, it_end);
    counts("after insert");
    assert_eq(va, vb);

    va.insert(va.end(), it_begin, it_end);
    vb.insert(vb.end(), it_begin, it_end);
    assert_eq(va, vb);

    auto ita = va.insert(va.begin() + std::ptrdiff_t(va.size()) / 2, data.begin(), data.end());
    auto* itb = vb.insert(vb.begin() + vb.size() / 2, data.begin(), data.end());
    REQUIRE(*ita == *itb);
    assert_eq(va, vb);

    va.insert(va.end(), data.begin(), data.end());
    vb.insert(vb.end(), data.begin(), data.end());
    assert_eq(va, vb);
    va.insert(va.begin(), data.begin(), data.end());
    vb.insert(vb.begin(), data.begin(), data.end());
    assert_eq(va, vb);

    va.insert(va.begin() + 3, data.begin(), data.begin());
    vb.insert(vb.begin() + 3, data.begin(), data.begin());
    assert_eq(va, vb);

    va.insert(va.begin() + 3, it_begin, it_begin);
    vb.insert(vb.begin() + 3, it_begin, it_begin);
    assert_eq(va, vb);
}

// iterator insert(const_iterator pos, std::initializer_list<T> ilist);
// An empty insert has to leave the container exactly as it was. It used to shift by zero, which
// self-move-assigned every element from pos onwards and blanked them. See issue #65.
//
// std::string is what makes this visible: Counter::Obj's move assignment copies its fields, so a
// self-move preserves the value and the corruption cannot be observed. See issue #67.
TEST_CASE("insert_nothing_keeps_the_elements") {
    // long enough that the string allocates, so a self-move can't be hidden by SSO
    auto const a = std::string("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    auto const b = std::string("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    auto const c = std::string("cccccccccccccccccccccccccccccccccccc");
    auto const source = std::vector<std::string>{a, b, c};

    for (size_t pos = 0; pos <= 3; ++pos) {
        auto sv = ankerl::svector<std::string, 4>{a, b, c};
        auto vec = std::vector<std::string>{a, b, c};

        auto* it_sv = sv.insert(sv.cbegin() + pos, 0, a);
        auto it_vec = vec.insert(vec.cbegin() + pos, 0, a);
        REQUIRE(it_sv == sv.begin() + pos); // "or p if n == 0"
        REQUIRE(it_vec == vec.begin() + pos);
        assert_eq(vec, sv);

        // forward iterators, empty range
        sv.insert(sv.cbegin() + pos, source.begin(), source.begin());
        vec.insert(vec.cbegin() + pos, source.begin(), source.begin());
        assert_eq(vec, sv);

        // empty initializer list
        sv.insert(sv.cbegin() + pos, std::initializer_list<std::string>{});
        vec.insert(vec.cbegin() + pos, std::initializer_list<std::string>{});
        assert_eq(vec, sv);
    }
}

// same thing once the vector is indirect, so both storage modes are covered
TEST_CASE("insert_nothing_keeps_the_elements_indirect") {
    auto const vec_source = make_long_strings(50);
    auto sv = ankerl::svector<std::string, 4>(vec_source.begin(), vec_source.end());
    auto vec = vec_source;
    // more elements than fit inline, so this one is indirect
    REQUIRE(sv.size() > ankerl::svector<std::string, 4>().capacity());

    sv.insert(sv.cbegin() + 10, 0, std::string("x"));
    vec.insert(vec.cbegin() + 10, 0, std::string("x"));
    assert_eq(vec, sv);
}

// insert_n() decides whether the elements still fit. It used to ask "s + count > capacity()", and a
// huge count wrapped that sum around to something small, so the check said yes and the in place
// shift wrote past the end of the buffer. See issue #69.
TEST_CASE("insert_count_too_large_throws") {
    auto const huge = (std::numeric_limits<size_t>::max)();

    // max_size() is the first count that cannot work, huge is the one that used to wrap
    for (auto const count : {huge, ankerl::svector<int, 4>::max_size()}) {
        auto v = ankerl::svector<int, 4>{1, 2, 3};
        REQUIRE_THROWS_AS(v.insert(v.begin(), count, 5), std::length_error);

        // the failed insert left it alone
        REQUIRE(v.size() == 3);
        REQUIRE(v[0] == 1);
        REQUIRE(v[2] == 3);
    }
}

TEST_CASE("insert_count_too_large_throws_indirect") {
    auto v = ankerl::svector<int, 4>();
    for (int i = 0; i < 50; ++i) {
        v.push_back(i);
    }

    REQUIRE_THROWS_AS(v.insert(v.begin() + 10, (std::numeric_limits<size_t>::max)(), 5), std::length_error);
    REQUIRE(v.size() == 50);
    REQUIRE(v[10] == 10);
}
