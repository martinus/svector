#include <ankerl/svector.h>

#include <doctest.h>

template <typename A, typename B>
void cmp(A const& a, B const& b, bool eq, bool lt) {
    CHECK(eq == (a == b));
    CHECK(!eq == (a != b));

    if (eq) {
        CHECK(!lt);
        CHECK(!(a < b));
        CHECK(!(a > b));
        CHECK(a <= b);
        CHECK(a >= b);
    } else {
        CHECK(lt == (a < b));
        CHECK(!lt == (a > b));
        CHECK(lt == (a <= b));
        CHECK(!lt == (a >= b));
    }
}
template <typename A, typename B>
void test_equals(A& a, B& b) {
    cmp(a, b, true, false);

    a.push_back(123456);
    cmp(a, b, false, false);
    b.push_back(123455);
    cmp(a, b, false, false);
    b.clear();
    b.push_back(123457);
    cmp(a, b, false, true);

    a.push_back(1);
    cmp(a, b, false, true);
    b.push_back(2);
    cmp(a, b, false, true);
    b.push_back(2);
    cmp(a, b, false, true);

    a.assign({1, 2, 3, 4});
    b.assign({1, 2, 3, 4});
    cmp(a, b, true, false);
    a.reserve(100);
    cmp(a, b, true, false);

    b.resize(1000);
    cmp(a, b, false, true);
    a.resize(1000);
    cmp(a, b, true, false);
}

TEST_CASE("equals_vec") {
    auto a = std::vector<uint64_t>();
    auto b = std::vector<uint64_t>();
    test_equals(a, b);
}

TEST_CASE("equals_svector") {
    auto a = ankerl::svector<uint64_t, 3>();
    auto b = ankerl::svector<uint64_t, 7>();
    test_equals(a, b);
}
