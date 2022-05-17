#include <ankerl/svector.h>

#include <doctest.h>

using namespace std::literals;

TEST_CASE("rbegin_rend") {
    auto a = ankerl::svector<char, 7>();
    for (auto c : "hello world!"sv) {
        a.push_back(c);
    }
    auto it = a.rbegin();
    auto itc = a.crbegin();
    REQUIRE(*it == '!');
    REQUIRE(*itc == '!');
    ++it;
    REQUIRE(*it == 'd');
    REQUIRE(*itc == '!');
    ++itc;
    REQUIRE(*itc == 'd');
    *it = 'x';
    REQUIRE(*it == 'x');
    REQUIRE(*itc == 'x');
}
