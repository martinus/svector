// Tests for https://github.com/martinus/svector/issues/50
//
// Every operation that takes a T const& has to keep working when that reference points into
// the vector itself, e.g. v.push_back(v[0]), even when the call reallocates. This is required
// for std::vector (LWG 526) and all three major standard libraries honour it.
//
// Note that the iterator range overloads are deliberately not covered: the standard says the
// iterators passed to insert(pos, first, last) and assign(first, last) must not point into the
// container, so aliasing there stays undefined.

#include <ankerl/svector.h>
#include <app/Counter.h>

#include <doctest.h>

#include <cstddef>
#include <string>
#include <vector>

namespace {

// long enough that the string allocates, so a use-after-free is not hidden by SSO
auto const marker = std::string("MARKER-long-enough-to-heap-allocate-0123456789");
auto const filler = std::string("filler-long-enough-to-heap-allocate-0123456789");

// Fills with [marker, filler, filler, ...] and leaves the vector exactly at capacity, so the
// operation under test is forced to grow. Both containers end up with identical contents.
template <typename Vec>
void fill_to_capacity(Vec& v) {
    v.push_back(marker);
    v.push_back(filler);
    v.push_back(filler);
    v.shrink_to_fit();
    REQUIRE(v.size() == v.capacity()); // otherwise the test isn't exercising a reallocation
}

template <typename Vec>
auto dump(Vec const& v) -> std::vector<std::string> {
    return std::vector<std::string>(v.begin(), v.end());
}

// Runs op on an svector and on a std::vector holding the same elements, and requires that
// both end up identical. std::vector is the oracle for what the standard mandates.
template <typename Op>
void same_as_std_vector(Op op) {
    auto sv = ankerl::svector<std::string, 3>();
    fill_to_capacity(sv);
    op(sv);

    auto vec = std::vector<std::string>();
    fill_to_capacity(vec);
    op(vec);

    REQUIRE(dump(sv) == dump(vec));
}

} // namespace

TEST_CASE("self_ref_push_back") {
    same_as_std_vector([](auto& v) {
        v.push_back(v[0]);
    });
}

TEST_CASE("self_ref_push_back_move") {
    same_as_std_vector([](auto& v) {
        v.push_back(std::move(v[0]));
    });
}

TEST_CASE("self_ref_push_back_back") {
    same_as_std_vector([](auto& v) {
        v.push_back(v.back());
    });
}

TEST_CASE("self_ref_emplace_back") {
    same_as_std_vector([](auto& v) {
        v.emplace_back(v[0]);
    });
}

TEST_CASE("self_ref_insert_begin") {
    same_as_std_vector([](auto& v) {
        v.insert(v.begin(), v[0]);
    });
}

TEST_CASE("self_ref_insert_middle") {
    same_as_std_vector([](auto& v) {
        v.insert(v.begin() + 1, v[0]);
    });
}

TEST_CASE("self_ref_insert_end") {
    same_as_std_vector([](auto& v) {
        v.insert(v.end(), v[0]);
    });
}

TEST_CASE("self_ref_insert_move") {
    same_as_std_vector([](auto& v) {
        v.insert(v.begin(), std::move(v[0]));
    });
}

TEST_CASE("self_ref_insert_count") {
    same_as_std_vector([](auto& v) {
        v.insert(v.begin(), 100, v[0]);
    });
}

TEST_CASE("self_ref_emplace") {
    same_as_std_vector([](auto& v) {
        v.emplace(v.begin(), v[0]);
    });
}

TEST_CASE("self_ref_emplace_end") {
    same_as_std_vector([](auto& v) {
        v.emplace(v.end(), v[0]);
    });
}

TEST_CASE("self_ref_assign") {
    // the case from the issue: assign() clears first, which destroys the element value refers to
    same_as_std_vector([](auto& v) {
        v.assign(1000, v[0]);
    });
}

TEST_CASE("self_ref_resize") {
    same_as_std_vector([](auto& v) {
        v.resize(1000, v[0]);
    });
}

TEST_CASE("self_ref_without_reallocation") {
    // same operations while there is spare capacity: insert still shifts elements around, so
    // the source can be overwritten even though nothing is reallocated
    auto sv = ankerl::svector<std::string, 3>();
    auto vec = std::vector<std::string>();
    sv.reserve(64);
    vec.reserve(64);
    for (int i = 0; i < 5; ++i) {
        sv.push_back(marker + std::to_string(i));
        vec.push_back(marker + std::to_string(i));
    }

    sv.push_back(sv[0]);
    vec.push_back(vec[0]);
    REQUIRE(dump(sv) == dump(vec));

    sv.insert(sv.begin(), sv[3]);
    vec.insert(vec.begin(), vec[3]);
    REQUIRE(dump(sv) == dump(vec));

    sv.insert(sv.begin() + 2, 4, sv[1]);
    vec.insert(vec.begin() + 2, 4, vec[1]);
    REQUIRE(dump(sv) == dump(vec));
}

TEST_CASE("self_ref_direct_to_indirect") {
    // The inline buffer case is the nasty one: the old elements are destroyed but the memory
    // stays part of the svector, so reading them afterwards is not a heap-use-after-free and
    // sanitizers see nothing. It just silently produced a moved-from value.
    auto v = ankerl::svector<std::string, 2>();
    v.push_back(marker);
    v.push_back(filler);
    REQUIRE(v.size() == v.capacity()); // still inline, next push_back goes to the heap

    v.push_back(v[0]);
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == marker);
    REQUIRE(v[2] == marker);
}

TEST_CASE("self_ref_repeated_growth") {
    // keep appending a self reference across many reallocations
    auto v = ankerl::svector<std::string, 1>();
    v.push_back(marker);
    for (int i = 0; i < 200; ++i) {
        v.push_back(v[0]);
    }
    REQUIRE(v.size() == 201);
    for (auto const& s : v) {
        REQUIRE(s == marker);
    }
}

TEST_CASE("self_ref_counted_type") {
    // no leaks and no double destroys along the way
    Counter counts;
    {
        auto v = ankerl::svector<Counter::Obj, 3>();
        for (size_t i = 0; i < 3; ++i) {
            v.emplace_back(i, counts);
        }
        REQUIRE(v.size() == v.capacity());

        v.push_back(v[0]); // grows: [0,1,2] -> [0,1,2,0]
        REQUIRE(v.back().get() == 0);

        v.insert(v.begin(), v[1]); // -> [1,0,1,2,0]
        REQUIRE(v.front().get() == 1);
        REQUIRE(v.size() == 5);

        auto const expected = v[2].get(); // 1
        v.insert(v.begin(), 20, v[2]);
        REQUIRE(v.size() == 25);
        for (size_t i = 0; i < 20; ++i) {
            REQUIRE(v[i].get() == expected);
        }

        auto const fill_value = v[0].get();
        v.resize(200, v[0]);
        REQUIRE(v.size() == 200);
        REQUIRE(v.back().get() == fill_value);

        v.assign(300, v[0]);
        REQUIRE(v.size() == 300);
        REQUIRE(v.front().get() == fill_value);
        REQUIRE(v.back().get() == fill_value);
    }
    counts.check_all_done();
}

TEST_CASE("self_ref_trivial_type") {
    // trivially copyable types take the memcpy path in uninitialized_move_and_destroy
    auto sv = ankerl::svector<int, 4>();
    auto vec = std::vector<int>();
    for (int i = 0; i < 4; ++i) {
        sv.push_back(i);
        vec.push_back(i);
    }
    sv.shrink_to_fit();
    vec.shrink_to_fit();

    sv.push_back(sv[0]);
    vec.push_back(vec[0]);
    sv.insert(sv.begin(), 50, sv[1]);
    vec.insert(vec.begin(), 50, vec[1]);
    sv.resize(500, sv[0]);
    vec.resize(500, vec[0]);

    REQUIRE(std::vector<int>(sv.begin(), sv.end()) == vec);
}
